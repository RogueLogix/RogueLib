/**
 * Copyright (c) 2020 RogueLogix
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "RogueLib/Threading/WorkQueue.hpp"
#include <RogueLib/Threading/cpponmulticore/sema.h>
#include <mutex>
#include <list>
#include <boost/bind.hpp>
#include <RogueLib/Threading/DestructorCallback.hpp>
#include <shared_mutex>


namespace RogueLib::Threading {
    static thread_local bool waiting = false;

    class WorkQueue::IMPL {
        std::mutex accessMutex;
        std::list<Item> queue;
        LightweightSemaphore dequeueSemaphore;
        std::atomic_bool destroyed = {false};
        std::atomic_int waitingThreads = {0};
    public:
        Event enqueue(Item item);

        Dequeue dequeue();

        Item dequeueItem(std::shared_ptr<IMPL>& ptr);

        std::weak_ptr<IMPL> selfPtr;

        ~IMPL();
    };

    Event WorkQueue::IMPL::enqueue(Item item) {
        item.whenReady(boost::bind<void>([](std::shared_ptr<IMPL> queue, Item toEnqueue) {
            std::unique_lock<std::mutex> lk(queue->accessMutex);
            queue->queue.push_back(toEnqueue);
            queue->dequeueSemaphore.signal();
        }, selfPtr.lock(), item));
        return item.event();
    }

    WorkQueue::Item WorkQueue::IMPL::dequeueItem(std::shared_ptr<IMPL>& ptr) {
        waitingThreads++;
        waiting = true;
        DestructorCallback waitCallback([&]() {
            waitingThreads--;
            waiting = false;
        });
        // the destructor will know we are here, its ok to release our lock
        ptr.reset();
        // make sure we didnt just destroy the object
        if (!waiting) { // cant access object address space, it may be deconstructed
            return {{[]() {}}};
        }
        // ok, we are cleared to wait
        dequeueSemaphore.wait();
        if (destroyed) {
            return {{[]() {}}};
        }
        std::unique_lock<std::mutex> lk(accessMutex);
        Item item = queue.front();
        queue.pop_front();
        lk.unlock();
        return item;
    }

    WorkQueue::Dequeue WorkQueue::IMPL::dequeue() {
        return {selfPtr};
    }

    WorkQueue::IMPL::~IMPL() {
        destroyed = true;
        dequeueSemaphore.signal(std::numeric_limits<int>::max());
        // if this is being called from a dequeue release, there will be one left waiting.
        while (waitingThreads > (waiting ? 1 : 0)) {}
        waiting = false;
    }
}

namespace RogueLib::Threading {

    class WorkQueue::Dequeue::IMPL {
        std::weak_ptr<WorkQueue::IMPL> queue;
    public:
        IMPL(std::weak_ptr<WorkQueue::IMPL> ptr);

        Item dequeue();

        bool valid();

        WorkQueue getQueue();
    };

    WorkQueue::Item WorkQueue::Dequeue::IMPL::dequeue() {
        auto q = this->queue.lock();
        if (q) {
            return q->dequeueItem(q);
        }
        return {{[]() {}}};
    }

    WorkQueue::Dequeue::IMPL::IMPL(std::weak_ptr<WorkQueue::IMPL> ptr) {
        queue = std::move(ptr);
    }

    bool WorkQueue::Dequeue::IMPL::valid() {
        return !queue.expired();
    }

    WorkQueue WorkQueue::Dequeue::IMPL::getQueue() {
        auto q = WorkQueue();
        q.impl = queue.lock();
        return q;
    }
}

namespace RogueLib::Threading {
    class WorkQueue::Item::IMPL {
        Event event;
        boost::function<void()> function;
        std::mutex eventReadyMutex;
        std::atomic_uint64_t untriggeredWaitEvents = {UINT64_MAX};
        Event readyEvent;
    public:
        std::weak_ptr<IMPL> selfPtr;

        // selfptr is invalid in the constructor, and i need that
        void setVals(boost::function<void()> func, std::vector<Event> waitEvents);

        void whenReady(std::function<void()> callback);

        bool ready();

        Event waitEvent();

        void process();
    };

    void WorkQueue::Item::IMPL::setVals(boost::function<void()> func, std::vector<Event> waitEvents) {
        this->function = std::move(func);
        if (waitEvents.empty()) {
            readyEvent.trigger();
            return;
        }
        untriggeredWaitEvents = waitEvents.size();
        for (auto& waitEvent : waitEvents) {
            waitEvent.registerCallback(boost::bind<void>([](std::shared_ptr<IMPL> item) {
                std::unique_lock<std::mutex> lk(item->eventReadyMutex);
                item->untriggeredWaitEvents--;
                if (item->untriggeredWaitEvents == 0) {
                    lk.unlock();
                    item->readyEvent.trigger();
                }
            }, selfPtr.lock()));
        }
    }

    void WorkQueue::Item::IMPL::whenReady(std::function<void()> callback) {
        readyEvent.registerCallback(std::move(callback));
    }

    bool WorkQueue::Item::IMPL::ready() {
        return untriggeredWaitEvents == 0;
    }

    Event WorkQueue::Item::IMPL::waitEvent() {
        return event;
    }

    void WorkQueue::Item::IMPL::process() {
        readyEvent.wait();
        DestructorCallback callback{[=]() {
            this->event.trigger();
        }};
        function();
    }
}

namespace RogueLib::Threading {
    WorkQueue::WorkQueue() {
        impl = std::make_shared<IMPL>();
        impl->selfPtr = impl;
    }

    Event WorkQueue::enqueue(boost::function<void()> function, std::vector<Event> waitEvents) {
        return impl->enqueue({std::move(function), std::move(waitEvents)});
    }

    WorkQueue::Dequeue WorkQueue::dequeue() {
        return impl->dequeue();
    }

    void WorkQueue::null() {
        impl = nullptr;
    }

    WorkQueue::operator bool() {
        return bool(impl);
    }

    void WorkQueue::reEnqueue(WorkQueue::Item item) {
        impl->enqueue(std::move(item));
    }

    void WorkQueue::addQueueThread() {
        addQueueProcessingThread(*this);
    }

    WorkQueue::Item::Item(boost::function<void()> function, std::vector<Event> waitEvents) {
        impl = std::make_shared<IMPL>();
        impl->selfPtr = impl;
        impl->setVals(std::move(function), std::move(waitEvents));
    }

    void WorkQueue::Item::whenReady(std::function<void()> callback) {
        impl->whenReady(std::move(callback));
    }

    bool WorkQueue::Item::ready() {
        return impl->ready();
    }

    Event WorkQueue::Item::event() {
        return impl->waitEvent();
    }

    void WorkQueue::Item::process() {
        impl->process();
    }

    WorkQueue::Dequeue::Dequeue(std::weak_ptr<WorkQueue::IMPL> ptr) {
        impl = std::make_shared<IMPL>(ptr);
    }

    WorkQueue::Item WorkQueue::Dequeue::dequeue() {
        return impl->dequeue();
    }

    WorkQueue::Dequeue::operator bool() {
        return impl->valid();
    }

    WorkQueue WorkQueue::Dequeue::queue() {
        return impl->getQueue();
    }
}

#include <RogueLib/Threading/Thread.hpp>
#include <utility>

namespace RogueLib::Threading {
    void addQueueProcessingThread(WorkQueue queue, std::function<void()> startup, std::function<void()> shutdown) {
        Thread thread(boost::bind<void>(
                [](WorkQueue::Dequeue dequeue, std::function<void()> start, std::function<void()> end) {
                    start();
                    while (dequeue) {
                        auto item = dequeue.dequeue();
                        try {
                            item.process();
                        } catch (std::exception& ignored) {
                            // yes i ignore the exception, its not my problem that it rolled all the way back to here...
                        }
                    }
                    end();
                }, queue.dequeue(), startup, shutdown));
        thread.start();
    }
}
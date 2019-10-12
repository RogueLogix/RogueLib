#include "RogueLib/Threading/WorkQueue.hpp"
#include <RogueLib/Threading/cpponmulticore/sema.h>
#include <mutex>
#include <boost/bind.hpp>
#include <RogueLib/Threading/DestructorCallback.hpp>


namespace RogueLib::Threading {
    static thread_local bool waiting = false;

    class WorkQueue::IMPL {
        std::mutex accessMutex;
        std::list<Item> queue;
        LightweightSemaphore dequeueSemaphore;
        std::atomic_bool destroyed = false;
        std::atomic_int waitingThreads = 0;
    public:
        Event enqueue(Item item);

        Dequeue dequeue();

        Item dequeueItem(std::shared_ptr<IMPL>& ptr);

        std::weak_ptr<IMPL> selfPtr;

        ~IMPL();
    };

    Event WorkQueue::IMPL::enqueue(Item item) {
        item.whenReady(boost::bind<void>([](std::shared_ptr<IMPL> queue, Item toEnqueue) {
            std::unique_lock lk(queue->accessMutex);
            queue->queue.push_back(toEnqueue);
            queue->dequeueSemaphore.signal();
        }, std::move(selfPtr.lock()), std::move(item)));
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
        std::unique_lock lk(accessMutex);
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
        std::atomic_uint64_t untriggeredWaitEvents = -1;
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
                std::unique_lock lk(item->eventReadyMutex);
                item->untriggeredWaitEvents--;
                if (item->untriggeredWaitEvents == 0) {
                    lk.unlock();
                    item->readyEvent.trigger();
                }
            }, std::move(selfPtr.lock())));
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
        function();
        event.trigger();
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
    void addQueueProcessingThread(WorkQueue queue) {
        Thread thread(boost::bind<void>([](WorkQueue::Dequeue dequeue) {
            while (dequeue) {
                auto item = dequeue.dequeue();
                try {
                    item.process();
                } catch (std::exception& ignored) {
                    // yes i ignore the exception, its not my problem that it rolled all the way back to here...
                }
            }
        }, queue.dequeue()));
        thread.start();
    }
}
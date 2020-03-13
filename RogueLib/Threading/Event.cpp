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

#include "RogueLib/Threading/Event.hpp"

#include <mutex>
#include <condition_variable>
#include <atomic>
#include <vector>

namespace RogueLib::Threading {
    class Event::IMPL {
        std::vector<boost::function<void()>> callbacks;
        std::mutex callbackMutex;
        std::condition_variable waitCV;
        std::atomic_bool wasTriggered = {false};
    
    public:
        void wait();
        
        void trigger();
        
        void registerCallback(boost::function<void()> callback);
        
        ~IMPL();
        
    };
    
    void Event::IMPL::wait() {
        std::unique_lock<std::mutex> lk(callbackMutex);
        if (wasTriggered) {
            return;
        }
        waitCV.wait(lk);
    }
    
    void Event::IMPL::trigger() {
        std::unique_lock<std::mutex> lk(callbackMutex);
        if (wasTriggered) {
            return;
        }
        wasTriggered = true;
        for (auto& callback : callbacks) {
            callback();
            callback.clear();
        }
        lk.unlock();
        waitCV.notify_all();
    }
    
    void Event::IMPL::registerCallback(boost::function<void()> callback) {
        std::unique_lock<std::mutex> lk(callbackMutex);
        if (wasTriggered) {
            callback();
            return;
        }
        callbacks.push_back(callback);
    }
    
    Event::IMPL::~IMPL() {
        trigger();
    }
}

namespace RogueLib::Threading {
    Event::Event() {
        impl = std::make_shared<IMPL>();
    }
    
    void Event::wait() {
        impl->wait();
    }
    
    void Event::trigger() {
        impl->trigger();
    }
    
    void Event::registerCallback(boost::function<void()> callback) {
        impl->registerCallback(callback);
    }
}
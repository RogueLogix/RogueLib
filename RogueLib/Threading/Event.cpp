#include "RogueLib/Threading/Event.hpp"

#include <mutex>
#include <condition_variable>
#include <atomic>
#include <boost/bind/bind.hpp>

namespace RogueLib::Threading {
    class Event::IMPL {
        std::vector<boost::function<void()>> callbacks;
        std::mutex callbackMutex;
        std::condition_variable waitCV;
        std::atomic_bool wasTriggered = false;
    
    public:
        void wait();
        
        void trigger();
        
        void registerCallback(boost::function<void()> callback);
        
        ~IMPL();
        
    };
    
    void Event::IMPL::wait() {
        std::unique_lock lk(callbackMutex);
        if (wasTriggered) {
            return;
        }
        waitCV.wait(lk);
    }
    
    void Event::IMPL::trigger() {
        std::unique_lock lk(callbackMutex);
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
        std::unique_lock lk(callbackMutex);
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
#pragma once

#include <memory>
#include <functional>

#include <mutex>
// yes its a simple macro,but it does make it easier
#define ROGUELIB_SYNCHRONIZED_SCOPE static std::mutex syncMutex;std::unique_lock syncLock(syncMutex);

namespace RogueLib::Threading {
    class Thread {
    public:
        class IMPL;

    private:
        std::shared_ptr<IMPL> impl;
    public:
        Thread();
        
        explicit Thread(std::function<void()> function);
    
        explicit Thread(std::shared_ptr<IMPL> impl);
        
        void start();
        
        bool joinable();
        
        void join();
    
        uint64_t id();
        
        void attach(std::shared_ptr<IMPL> impl);
        
        void detach();
    
        void terminateAtException(bool shouldTerminate);
        
        // should this be implemented?
//        void atThreadEnd(std::function<void()> callback);
        
        void registerThreadSpecificUncaughtExceptionHandler(std::function<void(std::exception exception)> handler);

        static void registerUncaughtExceptionHandler(std::function<void(std::exception exception)> handler);

        static std::uint32_t hardwareConcurrency();
        
        static std::uint32_t physicalConcurrency();
        
        static Thread currentThread();
        
        static void joinAll();
    };
}
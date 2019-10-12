#include "Thread.hpp"

#include <atomic>
#include <thread>
#include <memory>
#include <RogueLib/Logging/Log.hpp>
#include <RogueLib/Exceptions/Exceptions.hpp>
#include <boost/thread.hpp>


namespace RogueLib::Threading {
    
    class Thread::IMPL {
        std::atomic_bool hasFunction = false;
        std::function<void()> function;
        
        std::atomic_bool started = false;
        std::thread internalThread;
        
        std::atomic_uint64_t threadID = std::numeric_limits<uint64_t>::max();
        std::atomic_bool joined = false;
        
        std::atomic_bool shouldTerminateAtException = true;
        std::vector<std::function<void(std::exception)>> uncaughtExceptionHandlers;
        
        std::weak_ptr<IMPL> selfPtr;
    public:
        
        IMPL();
        
        explicit IMPL(std::function<void()> function);
        
        ~IMPL();
        
        void setFunction(std::function<void()> function);
        
        void start();
        
        bool joinable();
        
        void join();
        
        uint64_t id();
        
        void setSelfPtr(std::shared_ptr<IMPL> ptr);
        
        void terminateAtException(bool shouldTerminate);
        
        void registerThreadSpecificUncaughtExceptionHandler(std::function<void(std::exception exception)> handler);
    };
    
    class OutOfThreadIDs : public RogueLib::Exceptions::FatalError {
    public:
        OutOfThreadIDs(const char* function, const char* file, int linenum, const std::string& message,
                       std::string errorType = "OutOfThreadIDs");
    
    public:
    
    };
    
    static thread_local Thread thisThread(std::shared_ptr<Thread::IMPL>(nullptr));
    static std::vector<Thread> runningThreads;
    static std::mutex vectorMutex;
    
    static std::vector<std::function<void(std::exception)>> uncaughtExceptionHandlers;
    
    static std::atomic_uint64_t nextID = 0;
//    static thread_local uint64_t thisThreadID = std::numeric_limits<uint64_t>::max();
    
    static uint64_t generateThreadID() {
        ROGUELIB_STACKTRACE
        ROGUELIB_SYNCHRONIZED_SCOPE
        if (nextID == std::numeric_limits<uint64_t>::max()) {
            // how? that's like 18 quintillion threads created
            throw OutOfThreadIDs(ROGUELIB_EXCEPTION_INFO, "ID limit reached");
        }
        return nextID++;
    }
    
    Thread::IMPL::IMPL() {
        ROGUELIB_STACKTRACE
    }
    
    Thread::IMPL::IMPL(std::function<void()> function) : IMPL() {
        ROGUELIB_STACKTRACE
        setFunction(function);
    }
    
    Thread::IMPL::~IMPL() {
        ROGUELIB_STACKTRACE
        if (internalThread.joinable()) {
            // can only happen when the thread is stopping
            // after all Thread objects go out of scope and it was not joined
            internalThread.detach();
        }
    }
    
    void Thread::IMPL::setFunction(std::function<void()> function) {
        ROGUELIB_STACKTRACE
        hasFunction = true;
        this->function = function;
    }
    
    void Thread::IMPL::start() {
        ROGUELIB_STACKTRACE
        ROGUELIB_SYNCHRONIZED_SCOPE
        if (started || !hasFunction) {
            return;
        }
        started = true;
        internalThread = std::thread([&]() {
            ROGUELIB_STACKTRACE
            thisThread.attach(selfPtr.lock());
            threadID = generateThreadID();
            {
                std::unique_lock lock{vectorMutex};
                runningThreads.emplace_back(thisThread);
            }
            try {
                ROGUELIB_RESTACKTRACE
                function();
            } catch (std::exception& e) {
                ROGUELIB_RESTACKTRACE
                // if an exception made it this far back, ya dun fucked up a-a-ron

                Logging::fatal(e.what());
                for (auto& handler : this->uncaughtExceptionHandlers) {
                    handler(e);
                }
                for (auto& handler : Threading::uncaughtExceptionHandlers) {
                    handler(e);
                }
                // allows a handler to take responsibility for the exception
                if (this->shouldTerminateAtException) {
                    std::terminate();
                }
            }
            {
                // remove this from running threads, as its about to stop
                std::unique_lock lock{vectorMutex};
                auto iterator = runningThreads.begin();
                do {
                    if (iterator.operator*().id() == id()) {
                        runningThreads.erase(iterator);
                        break;
                    }
                } while (++iterator != runningThreads.end());
            }
            thisThread.detach();
            this->threadID = std::numeric_limits<uint64_t>::max();
            this->started = false;
        });
        while (threadID == std::numeric_limits<uint64_t>::max() && started){

        }
    }
    
    bool Thread::IMPL::joinable() {
        return started && internalThread.joinable();
    }
    
    void Thread::IMPL::join() {
        ROGUELIB_STACKTRACE
        joined = true;
        internalThread.join();
    }
    
    uint64_t Thread::IMPL::id() {
        return threadID;
    }
    
    void Thread::IMPL::setSelfPtr(std::shared_ptr<Thread::IMPL> ptr) {
        if (ptr.get() != this) {
            // the fuck are you trying to do?
            return;
        }
        selfPtr = ptr;
    }
    
    void Thread::IMPL::terminateAtException(bool shouldTerminate) {
        shouldTerminateAtException = shouldTerminate;
    }
    
    void Thread::IMPL::registerThreadSpecificUncaughtExceptionHandler
            (std::function<void(std::exception exception)> handler) {
        this->uncaughtExceptionHandlers.emplace_back(handler);
    }
    
    Thread::Thread() {
        ROGUELIB_STACKTRACE
        impl = std::make_shared<IMPL>();
    }
    
    Thread::Thread(std::function<void()> function) {
        ROGUELIB_STACKTRACE
        impl = std::make_shared<IMPL>(function);
        impl->setSelfPtr(impl);
    }
    
    Thread::Thread(std::shared_ptr<Thread::IMPL> impl) {
        ROGUELIB_STACKTRACE
        this->impl = impl;
    }
    
    void Thread::start() {
        impl->start();
    }
    
    bool Thread::joinable() {
        return impl->joinable();
    }
    
    void Thread::join() {
        ROGUELIB_STACKTRACE
        impl->join();
    }
    
    uint64_t Thread::id() {
        return impl->id();
    }
    
    std::uint32_t Thread::hardwareConcurrency() {
        return boost::thread::hardware_concurrency();
    }
    
    std::uint32_t Thread::physicalConcurrency() {
        return boost::thread::physical_concurrency();
    }
    
    Thread Thread::currentThread() {
        return Threading::thisThread;
    }
    
    void Thread::attach(std::shared_ptr<Thread::IMPL> impl) {
        this->impl = impl;
    }
    
    void Thread::detach() {
        impl = nullptr;
    }
    
    void Thread::registerUncaughtExceptionHandler(std::function<void(std::exception)> handler) {
        ROGUELIB_STACKTRACE
        ROGUELIB_SYNCHRONIZED_SCOPE
        uncaughtExceptionHandlers.push_back(handler);
    }
    
    void Thread::joinAll() {
        std::vector<Thread> threads;
        {
            std::unique_lock lock{vectorMutex};
            threads = runningThreads;
        }
        for (auto& thread : threads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
    }
    
    void Thread::terminateAtException(bool shouldTerminate) {
        impl->terminateAtException(shouldTerminate);
    }
    
    void Thread::registerThreadSpecificUncaughtExceptionHandler(std::function<void(std::exception exception)> handler) {
        impl->registerThreadSpecificUncaughtExceptionHandler(handler);
    }
    
    OutOfThreadIDs::OutOfThreadIDs(
            const char* function,
            const char* file,
            int linenum,
            const std::string& message, std::string
            errorType) : FatalError(
            function, file, linenum, message, errorType) {}
}
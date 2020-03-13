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

#pragma once

#include <memory>
#include <functional>

#include <mutex>
// yes its a simple macro,but it does make it easier
#define ROGUELIB_SYNCHRONIZED_SCOPE static std::mutex syncMutex;std::unique_lock<std::mutex> syncLock(syncMutex);

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
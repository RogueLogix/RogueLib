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

#include <RogueLib/Threading/Event.hpp>
#include <vector>

#if __cplusplus >= 201703L // C++ 17
#if __has_include(<boost/bind.hpp>)
// not beacuse the header uses it, but because you ~~probably~~ will
#include <boost/bind.hpp>
#endif
#endif

namespace RogueLib::Threading {
    class WorkQueue {
        class IMPL;
        
        std::shared_ptr<IMPL> impl;
    public:
        
        class Item {
            class IMPL;
            
            std::shared_ptr<IMPL> impl;
        
        public:
            Item(boost::function<void()> function, std::vector<Event> waitEvents = {});
            
            void whenReady(std::function<void()> callback);
            
            bool ready();
            
            Event event();
            
            void process();
        };
        
        
        WorkQueue();
        
        Event enqueue(boost::function<void()> function, std::vector<Event> waitEvents = {});
        
        class Dequeue {
            class IMPL;
            
            std::shared_ptr<Dequeue::IMPL> impl;
        public:
            Dequeue(std::weak_ptr<WorkQueue::IMPL> ptr);
            
            Item dequeue();

            operator bool();

            WorkQueue queue();
        };
        
        Dequeue dequeue();

        void reEnqueue(Item item);
    
        void null();

        operator bool();

        /**
         * WARNING: ignores exceptions propagated out of work items
         */
        void addQueueThread();
    };

    void addQueueProcessingThread(WorkQueue queue, std::function<void()> startup = {}, std::function<void()> shutdown = {});
}
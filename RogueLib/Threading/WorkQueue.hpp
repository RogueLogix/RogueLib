#pragma once

#include <RogueLib/Threading/Event.hpp>

#if __has_include(<boost/bind.hpp>)
// not beacuse the header uses it, but because you ~~probably~~ will
#include <boost/bind.hpp>
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

    void addQueueProcessingThread(WorkQueue queue);
}
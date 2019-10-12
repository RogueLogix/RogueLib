#pragma once

#include <memory>
#include <boost/function.hpp>

namespace RogueLib::Threading{
    class Event{
        class IMPL;
        std::shared_ptr<IMPL> impl;
    public:
        Event();
        
        void wait();
        
        void trigger();
        
        void registerCallback(boost::function<void()> callback);
    };
}
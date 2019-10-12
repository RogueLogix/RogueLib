#pragma once

#include <functional>

namespace RogueLib::Threading {
    /**
     * looks like a smart pointer
     * acts like a smart pointer
     * smells like a smart pointer
     * must be a smart pointer
     *
     * basically a smart pointer without a pointer, use this deleter for other resources
     *
     */
    class DestructorCallback {
        std::function<void()> function;
    public:
        /**
         * created a blank object, callback function specified later
         */
        DestructorCallback();
        
        /**
         * creates object with preset callback function
         * @param function: callback function
         */
        explicit DestructorCallback(std::function<void()> function);
        
        /**
         * calls function, if one was specified
         */
        ~DestructorCallback();
        
        /**
         *
         * sets callback function to new function
         *
         * @param function: new callback function
         * @return: this
         *
         * @warning: does NOT call previously specified function
         */
        DestructorCallback& operator=(std::function<void()> function);
    };
}
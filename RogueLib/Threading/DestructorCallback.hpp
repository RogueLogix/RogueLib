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
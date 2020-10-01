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

#include <stack>
#include <string>
#include <utility>
#include <vector>
#include <memory>

namespace RogueLib::Exceptions {

    class StackTraceRecorder {
    public:
        const char* const function;
        const char* const file;
        int line;

        StackTraceRecorder(const char* function, const char* file, int line);

        ~StackTraceRecorder();

        void updateStackTraceLine(int line);
    };

    class StackTraceElement {
    public:
        const char* const function;
        const char* const file;
        const int line;

        StackTraceElement(StackTraceRecorder* recorder);

        StackTraceElement(StackTraceElement& element);
    };

#ifndef NDEBUG
#define ROGUELIB_STACKTRACE \
static auto _RL_func_ = __func__;\
std::unique_ptr<RogueLib::Exceptions::StackTraceRecorder> roguelibStackTraceUnwindDeletor\
(new RogueLib::Exceptions::StackTraceRecorder(_RL_func_, __FILE__, __LINE__ - 1));
#define ROGUELIB_RESTACKTRACE roguelibStackTraceUnwindDeletor->updateStackTraceLine(__LINE__);
#define ROGUELIB_LAMBDATRACE \
std::unique_ptr<RogueLib::Exceptions::StackTraceRecorder> roguelibStackTraceUnwindDeletor\
(new RogueLib::Exceptions::StackTraceRecorder(_RL_func_, __FILE__, __LINE__ - 1));
#else
#define ROGUELIB_STACKTRACE auto& _RL_func_ = __func__;
#define ROGUELIB_RESTACKTRACE
#define ROGUELIB_LAMBDATRACE
#endif
#define ROGUELIB_EXCEPTION_INFO _RL_func_, __FILE__, __LINE__

    class ErrorBase : public std::exception {
        std::vector<std::shared_ptr<StackTraceElement>> stacktrace;
        std::string message;
        std::string errorType;
        std::string printMessage;
    protected:
        bool fatal;
    public:
        ErrorBase(const char* function, const char* file, int linenum, std::string message, std::string errorType);

        ErrorBase(const char* function, const char* file, int linenum, std::string message, std::string errorType,
                  bool fatal);

        std::string getStackTrace();

        const char* what() const noexcept override;

        std::string msg(){
            return {message};
        }

        std::string type(){
            return {errorType};
        }

        std::string printMsg(){
            return {printMessage};
        }

        bool fatality(){
            return fatal;
        }
    };

    class Error : public ErrorBase {
    public:
        Error(const char* function, const char* file, int linenum, std::string message, std::string errorType);
    };

    class FileNotFound : public Error {
    public:
        FileNotFound(const char* function, const char* file, int linenum, std::string message);
    };

    class InvalidRecursion : public Error {
    public:
        InvalidRecursion(const char* function, const char* file, int linenum, std::string message);
    };

    class InvalidArgument : public Error {
    public:
        InvalidArgument(const char* function, const char* file, int linenum, std::string message);

        InvalidArgument(const char* function, const char* file, int linenum, std::string message,
                        std::string errorType);
    };

    class InvalidState : public Error {
    public:
        InvalidState(const char* function, const char* file, int linenum, const std::string& message,
                     std::string errorType = "InvalidState");

    };

    class InvalidAsyncState : public InvalidState {
    public:
        InvalidAsyncState(const char* function, const char* file, int linenum, const std::string& message,
                          std::string errorType = "InvalidAsyncState");

    };

    class NullPointer : public Error {
    public:
        NullPointer(const char* function, const char* file, int linenum, std::string message);
    };

    class FatalError : public ErrorBase {
    public:
        FatalError(const char* function, const char* file, int linenum, std::string message, std::string errorType);
    };

    class FatalFileNotFound : public FatalError {
    public:
        FatalFileNotFound(const char* function, const char* file, int linenum, std::string message)
                : FatalError(function, file, linenum, std::move(message), "FatalFileNotFound") {
        }
    };

    class FatalInvalidState : public FatalError {
    public:
        FatalInvalidState(const char* function, const char* file, int linenum, const std::string& message,
                          std::string errorType = "FatalInvalidState")
                : FatalError(function, file, linenum, message, std::move(errorType)) {
        }


    };

    class FatalInitFailure : public FatalError {
    public:
        FatalInitFailure(const char* function, const char* file, int linenum, std::string message);
    };

    class Unimplemented : public FatalError {
    public:
        Unimplemented(const char* function, const char* file, int linenum, std::string message);
    };

    std::string getCurrentStackTrace();
}
#pragma once

#include <stack>
#include <string>
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

#ifdef RogueLib_DEBUG
#define ROGUELIB_STACKTRACE \
auto& _func_ = __func__;\
std::unique_ptr<RogueLib::Exceptions::StackTraceRecorder> cubitStackTraceUnwindDeletor\
(new RogueLib::Exceptions::StackTraceRecorder(_func_, __FILE__, __LINE__ - 1));
#define ROGUELIB_RESTACKTRACE cubitStackTraceUnwindDeletor->updateStackTraceLine(__LINE__);
#else
#define ROGUELIB_STACKTRACE auto& _func_ = __func__;
#define ROGUELIB_RESTACKTRACE
#endif
#define ROGUELIB_EXCEPTION_INFO _func_, __FILE__, __LINE__

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

        virtual const char* what() const _GLIBCXX_TXN_SAFE_DYN _GLIBCXX_NOTHROW;
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
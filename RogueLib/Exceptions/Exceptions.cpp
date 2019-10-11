#include "Exceptions.hpp"

namespace RogueLib::Exceptions {

    static thread_local std::stack<StackTraceRecorder*>* current_stacktrace = nullptr;

    StackTraceRecorder::StackTraceRecorder(const char* function, const char* file, int line) :
            function(function), file(file), line(line) {
        if (current_stacktrace == nullptr) {
            current_stacktrace = new std::stack<StackTraceRecorder*>();
        }
        current_stacktrace->push(this);
    }

    StackTraceRecorder::~StackTraceRecorder() {
        current_stacktrace->pop();
        if (current_stacktrace->empty()) {
            delete (current_stacktrace);
            current_stacktrace = nullptr;
        }
    }

    void StackTraceRecorder::updateStackTraceLine(int line) {
        this->line = line;
    }

    StackTraceElement::StackTraceElement(StackTraceRecorder* recorder) : function(recorder->function),
                                                                         file(recorder->file), line(recorder->line) {
    }

    StackTraceElement::StackTraceElement(StackTraceElement& element) : function(element.function), file(element.file),
                                                                       line(element.line) {
    }

    ErrorBase::ErrorBase(const char* function, const char* file, int linenum, std::string message,
                         std::string errorType, bool fatal) : ErrorBase(function, file, linenum, std::move(message),
                                                                        std::move(errorType)) {
        this->fatal = fatal;
    }

    ErrorBase::ErrorBase(const char* function, const char* file, int linenum, std::string message,
                         std::string errorType) {

        this->message = std::move(message);
        this->errorType = std::move(errorType);

        StackTraceRecorder recorder(function, file, linenum);

#ifdef RELEASE
        return;
#endif

        auto stacktraceCopy = *current_stacktrace;
        stacktrace.reserve(stacktraceCopy.size() + 1);

        while (!stacktraceCopy.empty()) {
            stacktrace.emplace_back(new StackTraceElement{stacktraceCopy.top()});
            stacktraceCopy.pop();
        }

        getStackTrace();
    }

    const char* ErrorBase::what() const _GLIBCXX_TXN_SAFE_DYN _GLIBCXX_NOTHROW {
        return printMessage.c_str();
    }

    std::string ErrorBase::getStackTrace() {
        std::string trace;
        trace += errorType + ": " + message + "\n";
#pragma unroll 4
        for (int64_t i = 0; i < stacktrace.size(); i++) {
            auto toPrint = stacktrace[i];
            std::string stringToPrint;

            stringToPrint += "    ";
            stringToPrint += toPrint->function;
            stringToPrint += "(";
            stringToPrint += toPrint->file;
            stringToPrint += ":";
            stringToPrint += std::to_string(toPrint->line);
            stringToPrint += ")\n";

            trace += stringToPrint;
        }
        printMessage = trace;
        return trace;
    }

    Error::Error(const char* function, const char* file, int linenum, std::string message, std::string errorType)
            : ErrorBase(function, file, linenum, std::move(message), std::move(errorType)) {

    }

    FileNotFound::FileNotFound(const char* function, const char* file, int linenum, std::string message) :
            Error(function, file, linenum, std::move(message), "FileNotFound") {
    }

    InvalidRecursion::InvalidRecursion(const char* function, const char* file, int linenum, std::string message) :
            Error(function, file, linenum, std::move(message), "InvalidRecursion") {
    }

    InvalidArgument::InvalidArgument(const char* function, const char* file, int linenum, std::string message) :
            Error(function, file, linenum, std::move(message), "InvalidArgument") {
    }

    InvalidArgument::InvalidArgument(const char* function, const char* file, int linenum, std::string message,
                                     std::string errorType) : Error(function, file, linenum, std::move(message),
                                                                    std::move(errorType)) {
    }

    NullPointer::NullPointer(const char* function, const char* file, int linenum, std::string message) :
            Error(function, file, linenum, std::move(message), "NullPointer") {

    }

    FatalError::FatalError
            (const char* function, const char* file, int linenum, std::string message, std::string errorType) :
            ErrorBase(function, file, linenum, std::move(message), "[FATAL] " + errorType) {
        this->fatal = true;
    }

    FatalInitFailure::FatalInitFailure(const char* function, const char* file, int linenum, std::string message) :
            FatalError(function, file, linenum, std::move(message), "InitFailure") {
    }

    std::string getCurrentStackTrace() {
        ROGUELIB_STACKTRACE

        std::vector<std::shared_ptr<StackTraceElement>> stacktrace;

        auto stacktraceCopy = *current_stacktrace;
        stacktrace.reserve(stacktraceCopy.size());

        while (!stacktraceCopy.empty()) {
            stacktrace.emplace_back(new StackTraceElement{stacktraceCopy.top()});
            stacktraceCopy.pop();
        }

        std::string trace;
        for (int64_t i = 0; i < stacktrace.size(); i++) {
            auto toPrint = stacktrace[i];
            std::string stringToPrint;

            stringToPrint += "    ";
            stringToPrint += toPrint->function;
            stringToPrint += "(";
            stringToPrint += toPrint->file;
            stringToPrint += ":";
            stringToPrint += std::to_string(toPrint->line);
            stringToPrint += ")\n";

            trace += stringToPrint;
        }
        return trace;
    }

    InvalidState::InvalidState(const char* function, const char* file, int linenum, const std::string& message,
                               const std::string errorType) : Error(function, file, linenum, message, errorType) {
    }

    InvalidAsyncState::InvalidAsyncState(const char* function, const char* file, int linenum,
                                         const std::string& message, std::string errorType)
            : InvalidState(function, file, linenum, message, std::move(errorType)) {
    }

    Unimplemented::Unimplemented(const char* function, const char* file, int linenum, std::string message)
            : FatalError(function, file, linenum, std::move(message), "Unimplemented") {
    }
}

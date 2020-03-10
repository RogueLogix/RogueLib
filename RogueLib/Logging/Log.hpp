#pragma once

#include <functional>
#include <memory>
#include <fstream>
#include <string>
#include <mutex>
#include <list>

#include "RogueLib/Exceptions/Exceptions.hpp"

#if __cplusplus == 201703L // C++ 17
#if __has_include("filesystem")

#include <filesystem>

namespace RogueLib::Logging {
    namespace fs = std::filesystem;
}

#endif
#else

#include <experimental/filesystem>

namespace RogueLib::Logging {
    namespace fs = std::experimental::filesystem;
}

#endif

namespace RogueLib::Logging {

    enum class LogLevel {
        // technically bitwise, but because enumclass is a dick i cant use bitwise comparisons
        
        INFO = 0b1,
        WARNING = 0b10,
        ERROR = 0b100,
        PERFORMANCE_WARNING = 0b1000,
        TRACE = 0b10000,
        FATAL = -1, // all 1s in binary, that end bit is the one i care about
        ALL = std::numeric_limits<int>::max(), // 01111111 11111111 11111111 11111111 in binary
        FORCE_WRITE = ALL,
    };
    
    class OutputFile {
        class OutputFileIMPL;
        
        std::shared_ptr<OutputFileIMPL> impl;
    
    public:
        explicit OutputFile(fs::path folder, std::string nameBase, std::vector<LogLevel> applicableLevels = {});
        
        void write(std::string string, LogLevel level);
    };

    class OutputStreamOutput {
        class OutputStreamOutputIMPL;
        
        std::vector<LogLevel> applicableLevels;
        std::shared_ptr<OutputStreamOutputIMPL> impl;
    
    public:

//        OutputStreamOutput(std::ostream& stream, std::vector<LogLevel> applicableLevels = {}, bool buffer = false);
        
        explicit OutputStreamOutput(std::ostream* stream, std::vector<LogLevel> applicableLevels = {},
                                    bool synchronize = true,
                                    bool buffer = false);
        
        void write(std::string string, LogLevel level);
    };
    
    // todo allow removing of an output
    class Log {
        class LogIMPL;
        
        std::shared_ptr<LogIMPL> impl;
        LogLevel defaultLevel;
    
    public:
        Log();
        
        explicit Log(LogLevel defaultLevel);
        
        explicit Log(bool buffer, LogLevel defaultLevel = LogLevel::INFO);
        
        explicit Log(std::string prefix, bool buffer = false, LogLevel defaultLevel = LogLevel::INFO);
        
        explicit Log(std::string prefix, LogLevel defaultLevel = LogLevel::INFO);
        
        Log(Log& log);
        
        Log(Log* log);
        
        Log& operator=(Log* log);
        
        Log addOutput(OutputFile& output);
        
        Log addOutput(OutputStreamOutput& output);
        
        Log addOutput(Log& output);
        
        Log print(std::string string, LogLevel level);
        
        Log print(std::string string);
        
        Log println(std::string string, LogLevel level);
        
        Log println(std::string string);
        
        Log info(std::string string);
        
        Log warning(std::string string);
        
        Log error(std::string string);
        
        Log perfWarn(std::string string);
        
        Log trace(std::string string);
        
        Log fatal(std::string string);
        
        Log ln();
        
        Log operator<<(std::string string);
        
        std::string prefix();
    };
    
    extern Log* fallbackLog;
    
    extern Log* globalLog;
    
    extern thread_local Log* threadLog;
    
    void pushThreadLog(Log& log);

    void popThreadLog();

    class ScopeThreadLog{
    public:
        ScopeThreadLog(Log& log){
            pushThreadLog(log);
        }

        ~ScopeThreadLog(){
            popThreadLog();
        }
    };

    void setGlobalLog(Log& log);

    void allocateFallbackLog();
    
    static inline Log* defaultLog() {
        ROGUELIB_STACKTRACE
        if (threadLog == nullptr) {
            if (globalLog == nullptr) {
                // this should never be hit, *should*
                if(fallbackLog == nullptr){
                    // first time it was hit, need to allocate the fallback
                    allocateFallbackLog();
                }
                fallbackLog->error("FALLBACK LOG USED!");
                #ifdef RogueLib_DEBUG
                // so, you have exceptions, lets get that stack trace
                fallbackLog->error("Current Stack Trace\n" + Exceptions::getCurrentStackTrace());
                #endif
                return fallbackLog;
            }
            return globalLog;
        }
        return threadLog;
    }
    
    /*
     * Because passing a log to functions is tedious
     */
    static inline void info(std::string string, Log* log = nullptr) {
        ROGUELIB_STACKTRACE
        if (log == nullptr) {
            log = defaultLog();
        }
        log->info(std::move(string));
    }
    
    static inline void warning(std::string string, Log* log = nullptr) {
        ROGUELIB_STACKTRACE
        if (log == nullptr) {
            log = defaultLog();
        }
        log->warning(std::move(string));
    }
    
    static inline void error(std::string string, Log* log = nullptr) {
        ROGUELIB_STACKTRACE
        if (log == nullptr) {
            log = defaultLog();
        }
        log->error(std::move(string));
    }
    
    static inline void perfWarn(std::string string, Log* log = nullptr) {
        ROGUELIB_STACKTRACE
        if (log == nullptr) {
            log = defaultLog();
        }
        log->perfWarn(std::move(string));
    }
    
    static inline void trace(std::string string, Log* log = nullptr) {
        ROGUELIB_STACKTRACE
        if (log == nullptr) {
            log = defaultLog();
        }
        log->trace(std::move(string));
    }
    
    static inline void fatal(std::string string, Log* log = nullptr) {
        ROGUELIB_STACKTRACE
        if (log == nullptr) {
            log = defaultLog();
        }
        log->fatal(std::move(string));
    }
}
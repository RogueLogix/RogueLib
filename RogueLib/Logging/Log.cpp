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

#include "Log.hpp"

#include <utility>
#include <shared_mutex>
#include <thread>
#include <atomic>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <condition_variable>


//TODO: buffering

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-pragmas"
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
namespace RogueLib {
    // datetime/loglevel util
    namespace Logging {
        static std::string formatDateTime(const std::string& format) {
            std::time_t time = std::time(nullptr);
            auto localTime = std::localtime(&time);
            std::stringstream str;
            str << std::put_time(localTime, format.c_str());
            return str.str();
        }

//        static std::string date() {
//            return formatDateTime("%D");
//        }
        
        static std::string time() {
            return formatDateTime("%T");
        }
        
        std::string datetime() {
            return formatDateTime("%c");
        }
        
        std::string datetimeNoSpaces() {
            return formatDateTime("%F_%T");
        }
        
        static bool containsLevel(std::vector<LogLevel> vector, LogLevel level) {
            for (const auto& item : vector) {
                if (item == level) {
                    return true;
                }
            }
            return false;
        }
        
        static std::string levelToString(LogLevel level) {
            switch (level) {
                case LogLevel::INFO: {
                    return "[INFO]";
                }
                case LogLevel::WARNING: {
                    return "[WARNING]";
                }
                case LogLevel::ERROR: {
                    return "[ERROR]";
                }
                case LogLevel::PERFORMANCE_WARNING: {
                    return "[PERF_WARN]";
                }
                case LogLevel::FORCE_WRITE: {
                    return "[FORCE_WRITE]";
                }
                case LogLevel::TRACE:{
                    return "[TRACE]";
                }
                case LogLevel::FATAL: {
                    return "[FATAL]";
                }
            }
            return "";
        }
        
    }
    
    // File
    namespace Logging {
        
        class OutputFile::OutputFileIMPL {
            const fs::path folder;
            const std::string nameBase;
            
            std::chrono::system_clock::time_point fileCreated;
            fs::path currentFile;
            std::ofstream outputStream;
            
            std::mutex mutex;
            
            std::thread updateThread;
            std::condition_variable updateCV;
            std::mutex updateCVMutex;
            std::atomic_bool run = {true};
            
            std::vector<LogLevel> levels;
        
        public:
            OutputFileIMPL(fs::path folder, std::string nameBase, std::vector<LogLevel> vector);
            
            ~OutputFileIMPL();
            
            void startFileAutoUpdate();
            
            void createNewFileIfRequired();
            
            void createNewFile();
            
            void write(const std::string& string, LogLevel level);
        };
        
        
        OutputFile::OutputFileIMPL::OutputFileIMPL(fs::path folder, std::string nameBase,
                                                   std::vector<LogLevel> vector)
                :
                folder(std::move(folder)), nameBase(std::move(nameBase)
        ) {
            levels = std::move(vector);
            createNewFile();
        }
        
        OutputFile::OutputFileIMPL::~OutputFileIMPL() {
            run = false;
            updateCV.notify_all();
            while (updateThread.joinable()) {
                updateCV.notify_all();
            }
            updateCVMutex.unlock();
            outputStream.close();
        }
        
        void OutputFile::OutputFileIMPL::startFileAutoUpdate() {
            if (!updateCVMutex.try_lock()) {
                return;
            }
            updateThread = std::thread{[&]() {
                std::unique_lock<std::mutex> lock(updateCVMutex);
                while (run) {
                    createNewFileIfRequired();
                    updateCV.wait_until(lock, fileCreated + std::chrono::hours(24) + std::chrono::minutes(1));
                }
                updateThread.detach();
            }};
            updateCVMutex.unlock();
        }
        
        void OutputFile::OutputFileIMPL::createNewFile() {
            std::unique_lock<std::mutex> lock(mutex);
            
            outputStream.close();
            fileCreated = std::chrono::system_clock::now();
            currentFile = folder.string() + "/" + nameBase + "_" + datetimeNoSpaces() + ".log";
            fs::create_directories(folder);
            outputStream.open(currentFile);
            outputStream << "File Created: " << datetime() << std::endl;
        }
        
        void OutputFile::OutputFileIMPL::createNewFileIfRequired() {
            auto now = std::chrono::system_clock::now();
            if (std::chrono::duration_cast<std::chrono::hours>(now - fileCreated).count() >= 24) {
                createNewFile();
            }
        }
        
        void OutputFile::OutputFileIMPL::write(const std::string& string, LogLevel level) {
            containsLevel(levels, level);
            std::unique_lock<std::mutex> lock(mutex);
            outputStream << string;
        }
        
        OutputFile::OutputFile(fs::path folder, std::string nameBase, std::vector<LogLevel> applicableLevels) : impl(
                new OutputFileIMPL(std::move(folder), std::move(nameBase), std::move(applicableLevels))) {
            impl->startFileAutoUpdate();
        }
        
        void OutputFile::write(std::string string, LogLevel level) {
            impl->write(string, level);
        }
    }
    
    // Log
    namespace Logging {
        class Log::LogIMPL {
            //todo buffering
            std::string prefix;
            std::vector<OutputFile> fileOutputs;
            std::vector<OutputStreamOutput> consoleOutputs;
            std::vector<std::shared_ptr<LogIMPL>> logOutputs;
        public:
            LogIMPL(std::string prefix, bool buffer);
            
            
            void addOutput(OutputFile& output);
            
            void addOutput(OutputStreamOutput& output);
            
            void addOutput(Log& output);
            
            void write(const std::string& message, LogLevel level);
    
            std::string getPrefix();
        };
        
        Log::LogIMPL::LogIMPL(std::string prefix, bool buffer) {
            if(!prefix.empty()){
                prefix = "[" + prefix + "] ";
            }
            this->prefix = std::move(prefix);
            // todo buffering
//                this->buffer = buffer;
        }
        
        void Log::LogIMPL::addOutput(OutputFile& output) {
            fileOutputs.push_back(output);
        }
        
        void Log::LogIMPL::addOutput(OutputStreamOutput& output) {
            consoleOutputs.push_back(output);
        }
        
        void Log::LogIMPL::addOutput(Log& output) {
            logOutputs.emplace_back(output.impl);
        }
        
        void Log::LogIMPL::write(const std::string& message, LogLevel level) {
            if (!logOutputs.empty()) {
                std::string string = prefix + message;
                for (auto& log : logOutputs) {
                    log->write(string, level);
                }
            }
            if (!consoleOutputs.empty() || !fileOutputs.empty()) {
                std::string string = "[" + time() + "] " + levelToString(level) + " " + prefix + message;
                for (auto& output : consoleOutputs) {
                    output.write(string, level);
                }
                for (auto& output : fileOutputs) {
                    output.write(string, level);
                }
            }
        }
    
        std::string Log::LogIMPL::getPrefix() {
            return prefix;
        }
    
        Log::Log() : Log(LogLevel::INFO) {
        }
        
        Log::Log(LogLevel defaultLevel) : Log(false, defaultLevel) {
        }
        
        Log::Log(bool buffer, LogLevel defaultLevel) : Log("", buffer, defaultLevel) {
        }
        
        Log::Log(std::string prefix, LogLevel defaultLevel) : Log(std::move(prefix), false, defaultLevel) {
        }
        
        Log::Log(std::string prefix, bool buffer, LogLevel defaultLevel) {
            this->defaultLevel = defaultLevel;
            impl = std::make_shared<LogIMPL>(prefix, buffer);
        }

        Log::Log(Log* log) {
            impl = log->impl;
            defaultLevel = log->defaultLevel;
        }
        
        
        Log& Log::operator=(Log* log) {
            impl = log->impl;
            defaultLevel = log->defaultLevel;
            return *this;
        }
        
        Log Log::addOutput(OutputFile& output) {
            impl->addOutput(output);
            return *this;
        }
        
        Log Log::addOutput(OutputStreamOutput& output) {
            impl->addOutput(output);
            return *this;
        }
        
        Log Log::addOutput(Log& output) {
            impl->addOutput(output);
            return *this;
        }
        
        Log Log::print(std::string string) {
            print(std::move(string), defaultLevel);
            return *this;
        }
        
        Log Log::print(std::string string, LogLevel level) {
            impl->write(string, level);
            return *this;
        }
        
        Log Log::println(std::string string) {
            println(std::move(string), defaultLevel);
            return *this;
        }
        
        Log Log::println(std::string string, LogLevel level) {
            impl->write(string + "\n", level);
            return *this;
        }
        
        Log Log::info(std::string string) {
            println(std::move(string), LogLevel::INFO);
            return *this;
        }
        
        Log Log::warning(std::string string) {
            println(std::move(string), LogLevel::WARNING);
            return *this;
        }
        
        Log Log::error(std::string string) {
            println(std::move(string), LogLevel::ERROR);
            return *this;
        }
        
        Log Log::perfWarn(std::string string) {
            println(std::move(string), LogLevel::PERFORMANCE_WARNING);
            return *this;
        }
        
        Log Log::trace(std::string string) {
            println(std::move(string), LogLevel::TRACE);
            return *this;
        }
    
        Log Log::fatal(std::string string) {
            println(std::move(string), LogLevel::FATAL);
            return *this;
        }
    
        Log Log::ln() {
            impl->write("\n", LogLevel::FORCE_WRITE);
            return *this;
        }
    
        //todo more operator overloads
        Log Log::operator<<(std::string string) {
            print(std::move(string));
            return *this;
        }
    
        std::string Log::prefix() {
            return impl->getPrefix();
        }
    }
    
    // OutputStream
    namespace Logging {
        class OutputStreamOutput::OutputStreamOutputIMPL {
            std::ostream* ostream;
        public:
            OutputStreamOutputIMPL(std::ostream* ostream, bool buffer);
            
            void write(const std::string& string);
        };
        
        void OutputStreamOutput::OutputStreamOutputIMPL::write(const std::string& string) {
            *ostream << string;
        }
        
        OutputStreamOutput::OutputStreamOutputIMPL::OutputStreamOutputIMPL(std::ostream* ostream, bool buffer) {
            this->ostream = ostream;
        }
        
        OutputStreamOutput::OutputStreamOutput
                (std::ostream* stream, std::vector<LogLevel> applicableLevels, bool synchronize, bool buffer) {
            impl = std::make_shared<OutputStreamOutputIMPL>(stream, buffer);
            this->applicableLevels = std::move(applicableLevels);
        }
        
        void OutputStreamOutput::write(std::string string, LogLevel level) {
//            if (level == LogLevel::FATAL) {
//                // special handling for fatal error messages
//                // instead of only going to the specified stream, they are forced to std::cerr
//                static std::mutex cerrMutex;
//                std::unique_lock lock(cerrMutex);
//                std::cerr << string << std::endl;
//                lock.unlock();
//            }
            bool doWrite = level == LogLevel::FORCE_WRITE || level == LogLevel::FATAL;
            if (!doWrite) {
                for (auto& applicableLevel : applicableLevels) {
                    if (level == applicableLevel || applicableLevel == LogLevel::FORCE_WRITE) {
                        doWrite = true;
                        break;
                    }
                }
            }
            if (doWrite || applicableLevels.empty()) {
                impl->write(string);
            }
        }
    }
    
    // static functions
    namespace Logging {
        Log* fallbackLog;

        Log* globalLog = nullptr;
        Log globalLog_obj;


        thread_local Log* threadLog = nullptr;
        thread_local Log threadLog_obj;

        thread_local std::stack<Log> logs;

        static void updateThreadLog() {
            threadLog_obj = logs.top();
            threadLog = &threadLog_obj;
        }

        void pushThreadLog(Log& log){
            logs.emplace(log);
            updateThreadLog();
        }

        void popThreadLog(){
            logs.pop();
            updateThreadLog();
        }

        void setGlobalLog(Log& log) {
            globalLog_obj = log;
            globalLog = &globalLog_obj;
        }

        void allocateFallbackLog() {
            static std::mutex mutex;
            std::unique_lock<std::mutex> lock(mutex);
            if (fallbackLog != nullptr) {
                // allocated at the same time by two threads
                return;
            }
            // very aware this is unnecessary and is a memory leak,
            // its a fallback, so it stay allocated for lifetime of the program
            fallbackLog = new Log("FALLBACK");
            OutputStreamOutput output(&std::cerr);
            fallbackLog->addOutput(output);
        }
    }
}
#pragma clang diagnostic pop
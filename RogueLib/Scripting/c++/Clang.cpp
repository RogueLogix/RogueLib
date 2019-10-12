#include "Clang.hpp"

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <utility>
#include <atomic>
#include <vector>
#include <dlfcn.h>

namespace BlackCatBowlingSystems::TenPinsDown::Server::DB::Server {
    class JITCompiler::Module::IMPL {
        void* handle;
        std::string codeFile;
    public:
        IMPL(std::filesystem::path SOFile);

        ~IMPL();

        void* getSymbol(std::string name);
    };


    class JITCompiler::IMPL {
        std::filesystem::path workingDir;
    public:
        std::vector<std::string> includeDirectories;

        IMPL(std::filesystem::path workingDirectory);

        Module compile(std::string string);
    };
}
namespace BlackCatBowlingSystems::TenPinsDown::Server::DB::Server {
    namespace fs = std::filesystem;

    JITCompiler::IMPL::IMPL(std::filesystem::path workingDirectory) : workingDir(std::move(workingDirectory)) {
        // create the directory if it doesnt already exist
        fs::create_directories(workingDir);

        // regardless, there could be junk here, so lets clean it out
        for (const fs::path& entry : fs::recursive_directory_iterator(workingDir)) {
            std::remove(entry.c_str());
        }
    }

    JITCompiler::Module JITCompiler::IMPL::compile(std::string string) {
        // runs clang on command line to generate a shared object
        std::string command = "clang -shared -std=c++17 -fPIC ";

        // add working dir include directory
        command += "-I" + fs::absolute(fs::path("./include")).string() + " ";
        for (const auto& dir : includeDirectories) {
            command += "-I" + dir + " ";
        }

        // output file, its in our working directory, passed to the module to load it
        static std::atomic_uint64_t nextOutputFileID = 0;
        uint64_t id = nextOutputFileID++;
        std::filesystem::path outputFile = workingDir;
        outputFile /= "JITLib" + std::to_string(id) + ".so";
        // give that to clang
        command += "-o ";
        command += fs::absolute(outputFile);
        command += " ";

        // write the string out to a file so clang and compile it
        auto codeFile = workingDir / (std::to_string(id) + ".cpp");
        std::ofstream writeStream(codeFile);
        writeStream << string;
        writeStream.flush();
        command += codeFile;

        // command is ready to run, so, lets run it
        if (system(command.c_str())) {
            // something went wrong in the compilation
            throw std::runtime_error("JIT compilation failed");
        }

        // dont need the code output file anymore.
        std::remove(codeFile.c_str());

        return JITCompiler::Module(std::make_shared<Module::IMPL>(outputFile));
    }

    JITCompiler::Module::IMPL::IMPL(std::filesystem::path SOFile) {
        handle = dlopen(fs::absolute(SOFile).c_str(), RTLD_NOW);
        codeFile = fs::absolute(SOFile);
    }

    JITCompiler::Module::IMPL::~IMPL() {
        dlclose(handle);
        std::remove(codeFile.c_str());
    }

    void* JITCompiler::Module::IMPL::getSymbol(std::string name) {
        return dlsym(handle, name.c_str());
    }
}
namespace BlackCatBowlingSystems::TenPinsDown::Server::DB::Server {
    JITCompiler::JITCompiler(std::filesystem::path workingDir) {
        impl = std::make_shared<IMPL>(workingDir);
    }

    JITCompiler::Module JITCompiler::compile(std::string code) {
        return impl->compile(std::move(code));
    }

    void JITCompiler::addIncludeDirectory(std::filesystem::path dir) {
        impl->includeDirectories.emplace_back(fs::absolute(dir).string());
    }

    JITCompiler::Module::Module(std::shared_ptr<IMPL> impl) : impl(impl) {
    }

    void* JITCompiler::Module::getSymbolAddress(std::string name) {
        return impl->getSymbol(std::move(name));
    }
}
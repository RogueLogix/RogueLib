#pragma  once

#include <memory>

#include <filesystem>
#include <functional>
#include <utility>

namespace BlackCatBowlingSystems::TenPinsDown::Server::DB::Server {

    class JITCompiler {
        class IMPL;

        std::shared_ptr<IMPL> impl;
    public:
        class Module {
        public:
            class IMPL;

        private:
            std::shared_ptr<IMPL> impl;
        public:
            Module(){}

            Module(std::shared_ptr<IMPL> impl);

            void* getSymbolAddress(std::string name);

//            void* getUnmangledSymbolAddress(std::string name);

            template<typename Signature>
            std::function<Signature> getFunction(std::string name) {
                auto address = getSymbolAddress(std::move(name));
                std::function<Signature> func = (Signature*)address;
                return func;
            }
        };

        JITCompiler(std::filesystem::path workingDir);

        void addIncludeDirectory(std::filesystem::path dir);

        Module compile(std::string code);
    };
}

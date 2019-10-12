#include "Cling.hpp"

#include <cling/Interpreter/Interpreter.h>
#include <iostream>
#include <filesystem>

namespace RogueLib::Scripting::CPP {
    static const std::string __path = std::filesystem::absolute("./").string();
    static const char* __path_ptr =__path.c_str();
    class Cling::IMPL {
        cling::Interpreter interpreter = cling::Interpreter(1, &__path_ptr, "./lib/cling");
    public:
        IMPL();
    };
}
namespace RogueLib::Scripting::CPP {
    Cling::IMPL::IMPL() {

    }
}
namespace RogueLib::Scripting::CPP {

}

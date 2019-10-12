#pragma  once

#include <memory>

namespace RogueLib::Scripting::CPP{
    class Cling{
        class IMPL;
        std::shared_ptr<IMPL> impl;
    public:

    };
}
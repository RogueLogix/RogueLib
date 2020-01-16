#include "RogueLib/Threading/DestructorCallback.hpp"
namespace RogueLib::Threading{
    DestructorCallback::DestructorCallback(std::function<void()> function) : function(std::move(function)) {
    }

    DestructorCallback::DestructorCallback() {
        function = nullptr;
    }

    DestructorCallback::~DestructorCallback() {
        if (function != nullptr) {
            function();
        }
    }

    DestructorCallback& DestructorCallback::operator=(std::function<void()> function) {
        this->function = std::move(function);
        return *this;
    }
}
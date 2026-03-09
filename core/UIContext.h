#pragma once
#include "ErrorManager.h"

struct UIContext {
    ErrorManager& errorManager;

    UIContext(ErrorManager& e)
        : errorManager(e)
    {}
};
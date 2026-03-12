#pragma once
#include <string>
#include <vector>
#include "ErrorManager.h"

using std::string;
namespace Popup {
    bool showMessage(const string& title, const std::vector<AppError>& errors, const string& buttonLabe);
    bool showFatalMessage(const string& title, const std::vector<AppError>& errors, const string& buttonLabe);
}
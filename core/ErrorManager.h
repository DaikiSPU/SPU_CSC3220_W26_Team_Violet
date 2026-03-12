#pragma once

#include <vector>
#include <string>
#include "AppError.h"
#include "Result.h"

class ErrorManager {
public:
    void addError(ErrorType type, const std::string& message);
    void addError(const AppError& error);

    void clear();
    bool hasError() const;

    const std::vector<AppError>& getErrors() const;

    std::vector<std::string> getErrorMessages() const;

    void addError(const ResultBase& result);

    bool hasDatabaseError() const;
    bool hasSystemError() const;
    bool hasFatalError() const;
private:
    std::vector<AppError> errors;
};
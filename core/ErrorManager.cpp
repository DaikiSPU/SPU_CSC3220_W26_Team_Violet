#include "ErrorManager.h"

void ErrorManager::addError(ErrorType type, const std::string& message)
{
    errors.emplace_back(type, message);
}

void ErrorManager::addError(const AppError& error)
{
    if (error.hasError())
        errors.push_back(error);
}

void ErrorManager::clear()
{
    errors.clear();
}

bool ErrorManager::hasError() const
{
    return !errors.empty();
}

const std::vector<AppError>& ErrorManager::getErrors() const
{
    return errors;
}

std::vector<std::string> ErrorManager::getErrorMessages() const
{
    std::vector<std::string> messages;

    messages.reserve(errors.size());

    for (const auto& e : errors)
    {
        messages.push_back(e.getMessage());
    }

    return messages;
}

void ErrorManager::addError(const ResultBase& result)
{
    if (!result.isSuccess())
        errors.push_back(result.error);
}

bool ErrorManager::hasDatabaseError() const
{
    for (const auto& e : errors)
    {
        if (e.getType() == ErrorType::Database)
            return true;
    }
    return false;
}

bool ErrorManager::hasSystemError() const
{
    for (const auto& e : errors)
    {
        if (e.getType() == ErrorType::System)
            return true;
    }
    return false;
}

bool ErrorManager::hasFatalError() const
{
    for (const auto& e : errors)
    {
        if (e.getType() == ErrorType::Fatal)
            return true;
    }
    return false;
}
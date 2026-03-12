#pragma once
#include <string>

enum class ErrorType {
    None,
    Database,
    Validation,
    Authentication,
    System,
    Fatal
};

class AppError {
public:
    AppError() : type(ErrorType::None) {}

    AppError(ErrorType t, const std::string& msg)
        : type(t), message(msg) {}

    bool hasError() const { return type != ErrorType::None; }

    ErrorType getType() const { return type; }
    const std::string& getMessage() const { return message; }

    void setError(ErrorType t, const std::string& msg)
    {
        type = t;
        message = msg;
    }

    void clear()
    {
        type = ErrorType::None;
        message.clear();
    }

private:
    ErrorType type;
    std::string message;
};
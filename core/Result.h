#pragma once
#include "AppError.h"

// Base result shared logic
struct ResultBase {
    AppError error;

    bool isSuccess() const { 
        return !error.hasError(); 
    }

    void setError(ErrorType type, const std::string& msg) {
        error.setError(type, msg);
    }
};


// Result with value
template<typename T>
struct Result : ResultBase {
    T value{};
};


// Specialization for void
template<>
struct Result<void> : ResultBase {};
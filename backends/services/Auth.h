#pragma once

#include <string>
#include "Database.h"

using std::string;
using std::runtime_error;
class Auth {
    public:
        Auth();
        ~Auth()= default;
        
        static bool verifyUsername();
        Result<bool> verifyPassword(const char* username, const char* inputPassword, size_t password_len, const char* storedHash);
        static bool verifyPin(const int userId, const char* inputPin, size_t password_len);

        /*
            Hashes the password entered by the user.
        */
        static string createHashPassword(const char* inputPassword, size_t password_len);
        /*
            Hashes the pin number entered by the user.
        */
        static string createHashPin(const char* inputPin, size_t password_len);
        
    protected:
    private:

        /*
            Verifies the password entered by the user.
        */
        static bool verifyHash(const char* storedHash, const char* input, size_t password_len);
};
#include "CreateAccountBackend.h"

Result<bool> CreateAccountBackend::registerAccount(string username, string hashedPassword, string hashedPin)
{
    printf("register account backend\n");
    Result<bool> result = db.registerUser(username, hashedPassword, hashedPin);
    if (result.isSuccess())
    {
        result.value = true;
        return result;
    }
    return result;
}
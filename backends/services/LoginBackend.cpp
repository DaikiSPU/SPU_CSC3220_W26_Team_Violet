#include "LoginBackend.h"

Result<string> LoginBackend::getHashPassoword(string username)
{
    Result<string> result = db.getPasswordHash(username);
    return result;
}

Result<int> LoginBackend::setUserInfo(string username)
{
    Result<int> result = db.getUserId(username);
    if (result.isSuccess()) {
        appData.userId = result.value;
        return result;
    }
    return result;
}

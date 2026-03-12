#pragma once
#include "BackendContext.h"

using std::string;

class LoginBackend {
public:
    LoginBackend(BackendContext& backendContext) : db(backendContext.db), appData(backendContext.appData) {}

    Result<string> getHashPassoword(string username);
    Result<int> setUserInfo(string username);

private:
    Database& db;
    AppData& appData;
};
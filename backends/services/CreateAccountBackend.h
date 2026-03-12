#pragma once

#include "Database.h"
#include "BackendContext.h"

using std::string;

class CreateAccountBackend {
    public:
        CreateAccountBackend(BackendContext& backendContext) : db(backendContext.db), appData(backendContext.appData) {}
        Result<bool> registerAccount(string username, string hashedPassword, string hashedPin);
    protected:
    private:
        Database& db;
        AppData& appData;
};
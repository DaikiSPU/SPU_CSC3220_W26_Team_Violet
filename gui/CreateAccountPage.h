#pragma once
#include "Page.h"
#include "Auth.h"
#include "CreateAccountBackend.h"

class Database;

class CreateAccountPage : public Page {
    public:
        CreateAccountPage(UIContext& uiContext, BackendContext& backendContext) : Page(uiContext, backendContext), backend(backendContext) {}
        PageType draw() override;
    protected:
    private:
        // Define card size
        float FORM_WIDTH  = 450.0f;
        float FORM_HEIGHT = 550.0f;

        // Define username, password, and login button width.
        float ELEMENT_WIDTH = 200.0f;
        float ELEMENT_HEIGHT = 30.0f;
        float SMALL_SPACING = 5.0f;
        float BIG_SPACING = 35.0f;

        char username[32] = {};
        char password[32] = {};
        char pin[32] = {};

        Auth auth;
        CreateAccountBackend backend;
};
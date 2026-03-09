#pragma once
#include "PageType.h"
#include "imgui.h"

#include "UIContext.h"
#include "BackendContext.h"
#include "Popup.h"

#include <vector>

class Page {
    public:
        Page(UIContext& uiContext, BackendContext& backendContext) : errorManager(uiContext.errorManager) {}
        virtual ~Page() = default;
        virtual PageType draw() = 0;
        void setErrorMsg(AppError error) { errorManager.addError(error); }
        bool getCloseApp() { return closeApp; }
        bool getStopApp() { return stopApp; }
    protected:
        ErrorManager& errorManager;
        bool closeApp = false;
        bool stopApp = false;
    private:
};
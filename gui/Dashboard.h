#pragma once

#include "Page.h"
#include "States.h"
#include "DashboardBackend.h"
#include <string>
#include <algorithm>

class Database;

using std::string;
class Dashboard : public Page {
    public:
        Dashboard(UIContext& uiContext, BackendContext& backendContext);
        PageType draw() override;
    protected:
    private:
        PageType header(ImGuiViewport* viewport);
        void orderBookWindow();
        void chartWindow();
        void orderEntryWindow();
        void transactionWindow();
        PageType DashboardMenu();
        // Define transaction table size
        const ImVec2 transaction = ImVec2(0, 200);

        void orderHistoryWindow();
        void openOrdersWindow();

        bool orderConfirmPopup();

        // min width cannot support in docker
        // const ImVec2 headerMinSize = ImVec2(200.0f, 100.0f);
        // const ImVec2 orderBookMinSize = ImVec2(180.0f, 300.0f);
        // const ImVec2 orderEntryMinSize = ImVec2(220.0f, 300.0f);
        // const ImVec2 chartMinSize = ImVec2(200.0f, 300.0f);
        // const ImVec2 transactionMinSize = ImVec2(200.0f, 300.0f);

        float headerHeight;

        const float column2MinHeight = 300.0f;

        bool first_init = true;

        // Host window flags (no decoration, fullscreen style)
        ImGuiWindowFlags host_flags =
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoNavFocus |
            ImGuiWindowFlags_NoBackground; 

        ImGuiWindowFlags headerFlags = 
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse;

        // Order book table flags
        ImGuiTableFlags tableFlags =
            ImGuiTableFlags_BordersInnerV |
            ImGuiTableFlags_BordersInnerH |
            ImGuiTableFlags_RowBg |
            ImGuiTableFlags_ScrollY |
            ImGuiTableFlags_SizingStretchProp;

        OrderState orderState = OrderState::None;
        DashboardBackend backend;

        const DashboardData& data = backend.getData();

        int current_market_id;
        int selectedMarketId;
        double qty = 0.0;
        double price = 0.0;

        bool showConfirmOrderPopup = false;
        std::string pendingSide;
        double pendingPrice = 0;
        double pendingQty = 0;

        const int maxCandles = 60;

        int chartScrollX;
        float chartScrollY;
        int visibleCandles = 80;

        bool openPlaceOrderErrorPopup = false;
};
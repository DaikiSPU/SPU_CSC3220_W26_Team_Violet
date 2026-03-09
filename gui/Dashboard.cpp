#include "Dashboard.h"
#include "imgui_internal.h"
#include <vector>
#include <string>

Dashboard::Dashboard(UIContext &uiContext, BackendContext &backendContext) : Page(uiContext, backendContext), backend(backendContext)
{
    backend.refreshHeader(current_market_id);
    if (!data.markets.empty())
    {
        selectedMarketId = 0;
        current_market_id = data.markets[0].first;
    }
}

/*
 Render trading dashboard using DockSpace layout.
 Each major section is its own dockable window.
*/
PageType Dashboard::draw()
{
    if (errorManager.hasFatalError())
    {
        ImGui::OpenPopup("Fatal Error");
    }
    PageType popupResult = PageType::None;

    // Get main viewport
    ImGuiViewport* viewport = ImGui::GetMainViewport();

    popupResult = header(viewport);

    ImVec2 dock_pos = ImVec2(
        viewport->WorkPos.x,
        viewport->WorkPos.y + headerHeight
    );

    ImVec2 dock_size = ImVec2(
        viewport->WorkSize.x,
        viewport->WorkSize.y - headerHeight
    );

    ImGui::SetNextWindowPos(dock_pos);
    ImGui::SetNextWindowSize(dock_size);

    // Begin invisible host window
    ImGui::Begin("MainDockHost", nullptr, host_flags);

    // Create DockSpace node
    ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");

    ImGui::DockSpace(
        dockspace_id,
        ImVec2(0.0f, 0.0f),
        ImGuiDockNodeFlags_None
    );

    if (first_init)
    {
        first_init = false;

        ImGui::DockBuilderRemoveNode(dockspace_id);
        ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->WorkSize);

        // Root
        ImGuiID dock_main_id = dockspace_id;

        // Bottom history (bottom 25%)
        ImGuiID dock_transactions_id = ImGui::DockBuilderSplitNode(
            dock_main_id,
            ImGuiDir_Down,
            0.30f,
            nullptr,
            &dock_main_id
        );

        ImGuiID dock_order_book_id = ImGui::DockBuilderSplitNode(
            dock_main_id,
            ImGuiDir_Left,
            0.25f,
            nullptr,
            &dock_main_id
        );

        ImGuiID dock_order_entry_id = ImGui::DockBuilderSplitNode(
            dock_main_id,
            ImGuiDir_Right,
            0.25f,
            nullptr,
            &dock_main_id
        );

        // Dock windows
        ImGui::DockBuilderDockWindow("OrderBook", dock_order_book_id);
        ImGui::DockBuilderDockWindow("OrderEntry", dock_order_entry_id);
        ImGui::DockBuilderDockWindow("TradeHistory", dock_transactions_id);
        ImGui::DockBuilderDockWindow("Chart", dock_main_id);

        ImGui::DockBuilderFinish(dockspace_id);
    }

    // End host window
    ImGui::End();

    orderBookWindow();

    chartWindow();

    orderEntryWindow();

    transactionWindow();

    if (Popup::showMessage("Fatal Error", errorManager.getErrors(), "CLOSE WINDOW"))
    {
        printf("close app dashboard\n");
        errorManager.clear();
        closeApp = true;
    }

    if (ImGui::IsPopupOpen("Fatal Error"))
    {
        stopApp = true;
    }

    return popupResult;
}

PageType Dashboard::header(ImGuiViewport* viewport)
{
    /* ================= HEADER WINDOW ================= */
    PageType popupResult = PageType::None;
    headerHeight =
        2 * ImGui::GetFrameHeightWithSpacing() +
        ImGui::GetStyle().ItemSpacing.y +
        8.0f;

    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(
        ImVec2(viewport->WorkSize.x, headerHeight)
    );

    // Begin header window
    ImGui::Begin("Header", nullptr, headerFlags);

    long long rawLastPrice = 0;

    // Create top header table
    if (ImGui::BeginTable("HeaderTop", 4, ImGuiTableFlags_SizingStretchSame))
    {
        ImGui::TableNextRow();

        ImGui::TableNextColumn();
        if (ImGui::Button("Market Simulator")) {
            printf("click\n");
            ImGui::OpenPopup("Dashboard Menu");
        }

        ImGui::TableNextColumn();
        ImGui::Text("Market");
        ImGui::SameLine();

        if (ImGui::BeginCombo("##Market", data.markets[selectedMarketId].second.c_str()))
        {
            for (int i = 0; i < data.markets.size(); i++)
            {
                bool isSelected = (selectedMarketId == i);

                if (ImGui::Selectable(data.markets[i].second.c_str(), isSelected))
                {
                    selectedMarketId = i;
                    current_market_id = data.markets[i].first;
                    
                    // You can create a previous market ID variable to avoid checking the same market ID repeatedly.
                    backend.refreshHeader(current_market_id);
                }

                if (isSelected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }

            ImGui::EndCombo();
        }

        ImGui::TableNextColumn();
        double displayLastPrice = data.rawLastPrice / 10000.0;
        ImGui::Text("Last Price: %.4f", displayLastPrice);

        ImGui::TableNextColumn();
        if (data.fluctuation > 0)
        {
            ImGui::TextColored(ImVec4(0,1,0,1), "Fluctuation: +%.2f%%", data.fluctuation);
        }
        else if (data.fluctuation < 0)
        {
            ImGui::TextColored(ImVec4(1,0,0,1), "Fluctuation: %.2f%%", data.fluctuation);
        }
        else
        {
            ImGui::Text("Fluctuation: 0.00%%");
        }

        popupResult = DashboardMenu();

        ImGui::EndTable();
    }

    // Separator line
    ImGui::Separator();

    // Create account summary table
    if (ImGui::BeginTable("HeaderAccount", 4, ImGuiTableFlags_SizingStretchSame))
    {
        ImGui::TableNextRow();

        ImGui::TableNextColumn();
        double displayAvailableCash = data.rawAvailableCash / 10000.0;
        ImGui::Text("Cash: %.4f", displayAvailableCash);

        double displayEquity = data.rawEquity / 10000.0;
        double displayUnrealized = data.rawUnrealized / 10000.0;

        ImGui::TableNextColumn();
        ImGui::Text("Equity: %.2f", displayEquity);

        ImGui::TableNextColumn();

        if (displayUnrealized >= 0)
        {
            ImGui::TextColored(ImVec4(0,1,0,1), "Unrealized P/L: %.2f", displayUnrealized);
        }
        else
        {
            ImGui::TextColored(ImVec4(1,0,0,1), "Unrealized P/L: %.2f", displayUnrealized);
        }

        ImGui::TableNextColumn();
        double displayRealized = data.rawRealized / 10000.0;
        if (displayRealized > 0.0)
        {
            ImGui::TextColored(ImVec4(0, 1, 0, 1),
                "Realized P/L: +%.2f", displayRealized);
        }
        else if (displayRealized < 0.0)
        {
            ImGui::TextColored(ImVec4(1, 0, 0, 1),
                "Realized P/L: %.2f", displayRealized);
        }
        else
        {
            ImGui::Text("Realized P/L: 0.00");
        }

        ImGui::EndTable();
    }

    ImGui::End();

    return popupResult;
}

void Dashboard::orderBookWindow()
{
    auto book = backend.refreshOrderBook(current_market_id);
    // printf("DEBUG market=%d asks=%zu bids=%zu\n",
    //    current_market_id,
    //    book.asks.size(),
    //    book.bids.size());
    /* ================= ORDER BOOK WINDOW ================= */
    // Begin OrderBook window
    ImGui::Begin("OrderBook");

    // Section title
    ImGui::Text("Sell");

    // Begin sell table
    if (ImGui::BeginTable("##OrderBookAsks", 2, tableFlags, ImVec2(0, 0)))
    {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("Price", ImGuiTableColumnFlags_WidthStretch, 1.0f);
        ImGui::TableSetupColumn("Size",  ImGuiTableColumnFlags_WidthStretch, 1.0f);
        ImGui::TableHeadersRow();

        for (const auto& level : book.asks)
        {
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%lld", level.price);

            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%lld", level.size);
        }

        ImGui::EndTable();
    }

    ImGui::Separator();

    // Section title
    ImGui::Text("Buy");

    // Begin buy table
    if (ImGui::BeginTable("##OrderBookBids", 2, tableFlags, ImVec2(0, 0)))
    {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("Price", ImGuiTableColumnFlags_WidthStretch, 1.0f);
        ImGui::TableSetupColumn("Size",  ImGuiTableColumnFlags_WidthStretch, 1.0f);
        ImGui::TableHeadersRow();

        for (const auto& level : book.bids)
        {
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%lld", level.price);

            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%lld", level.size);
        }

        ImGui::EndTable();
    }

    ImGui::End();
}

void Dashboard::chartWindow()
{
    /* ================= CHART WINDOW ================= */
    // Begin chart window
    ImGui::Begin("Chart");

    // Placeholder chart text
    ImGui::Text("Chart");

    ImGui::End();
}

void Dashboard::orderEntryWindow()
{
    /* ================= ORDER ENTRY WINDOW ================= */
    // Begin order entry window
    ImGui::Begin("OrderEntry");

    if (ImGui::BeginTable("OrderButton", 2))
    {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);

        bool isBuySelected = (orderState == OrderState::BUY);

        if (isBuySelected)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.7f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.15f, 0.8f, 0.25f, 1.0f));
        }

        if (ImGui::Button("BUY", ImVec2(-1, 0)))
        {
            orderState = OrderState::BUY;
        }

        if (isBuySelected)
        {
            ImGui::PopStyleColor(2);
        }

        ImGui::TableSetColumnIndex(1);

        bool isSellSelected = (orderState == OrderState::SELL);

        if (isSellSelected)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.7f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.15f, 0.8f, 0.25f, 1.0f));
        }

        if (ImGui::Button("SELL", ImVec2(-1, 0)))
        {
            orderState = OrderState::SELL;
        }

        if (isSellSelected)
        {
            ImGui::PopStyleColor(2);
        }
        ImGui::EndTable();
    }

    ImGui::Separator();

    if (ImGui::BeginTable("OrderForm1", 2, ImGuiTableFlags_SizingStretchProp))
    {
        float price = 0.0f;

        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
        
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Price");

        ImGui::TableSetColumnIndex(1);

        float priceRow = ImGui::GetContentRegionAvail().x;
        ImVec2 priceTextSize = ImGui::CalcTextSize("0");

        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (priceRow - priceTextSize.x));
        ImGui::Text("0");

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Quantity");
        ImGui::TableSetColumnIndex(1);
        ImGui::SetNextItemWidth(-1);
        ImGui::InputInt("##qty", &qty);

        ImGui::EndTable();
    }

    ImGui::Separator();

    if (ImGui::BeginTable("OrderForm2", 2, ImGuiTableFlags_SizingStretchProp))
    {
        float estimated = 0000.00;
        char estimatedText[32];
        snprintf(estimatedText, sizeof(estimatedText), "%.2f", estimated);
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Estimated");

        ImGui::TableSetColumnIndex(1);
        float estimatedRow = ImGui::GetContentRegionAvail().x;
        ImVec2 estimatedTextSize = ImGui::CalcTextSize(estimatedText);

        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (estimatedRow - estimatedTextSize.x));
        ImGui::Text("%s", estimatedText);

        ImGui::EndTable();
    }

    ImGui::Separator();

    if (ImGui::BeginTable("OrderForm3", 2, ImGuiTableFlags_SizingStretchProp))
    {
        float availableCash = 0000.00;
        char availableCashText[32];
        snprintf(availableCashText, sizeof(availableCashText), "%.2f", availableCash);
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Available Cash");

        ImGui::TableSetColumnIndex(1);

        float availableCashRow = ImGui::GetContentRegionAvail().x;
        ImVec2 availableCashTextSize = ImGui::CalcTextSize(availableCashText);

        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (availableCashRow - availableCashTextSize.x));
        ImGui::Text("%s", availableCashText);

        int position = 0;
        char positionhText[32];
        snprintf(positionhText, sizeof(positionhText), "%d", position);
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Position");

        ImGui::TableSetColumnIndex(1);
        float positionRow = ImGui::GetContentRegionAvail().x;
        ImVec2 positionTextSize = ImGui::CalcTextSize(positionhText);

        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (positionRow - positionTextSize.x));
        ImGui::Text("%s", positionhText);

        ImGui::EndTable();
    }

    ImGui::Separator();

    if (ImGui::Button("Place Order", ImVec2(-1, 0))) 
    {
        printf("order button placed\n");
    }


    ImGui::End();
}

void Dashboard::transactionWindow()
{
    /* ================= TRADE HISTORY WINDOW ================= */
    // Begin trade history window
    ImGui::Begin("TradeHistory");

    // Create history table
    if (ImGui::BeginTable("HistoryTable", 6, tableFlags))
    {
        ImGui::TableSetupScrollFreeze(0, 1);

        ImGui::TableSetupColumn("Time");
        ImGui::TableSetupColumn("Market");
        ImGui::TableSetupColumn("Side");
        ImGui::TableSetupColumn("Price");
        ImGui::TableSetupColumn("Quantity");
        ImGui::TableSetupColumn("Status");

        ImGui::TableHeadersRow();

        for (int i = 0; i < 5; ++i)
        {
            ImGui::TableNextRow();

            ImGui::TableNextColumn();
            ImGui::Text("00:00:0%d", i);

            ImGui::TableNextColumn();
            ImGui::Text("SPU");

            ImGui::TableNextColumn();
            ImGui::Text(i % 2 ? "Buy" : "Sell");

            ImGui::TableNextColumn();
            ImGui::Text("100.00");

            ImGui::TableNextColumn();
            ImGui::Text("%d", 10 + i);

            ImGui::TableNextColumn();
            ImGui::Text("Filled");
        }

        ImGui::EndTable();
    }

    ImGui::End();
}

PageType Dashboard::DashboardMenu()
{
    PageType nextPage = PageType::None;
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    // Modal popup
    if (ImGui::BeginPopupModal("Dashboard Menu", nullptr,
        ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::TextUnformatted("Are you sure you want to delete the data?");
        ImGui::Spacing();
        ImGui::TextUnformatted("This action cannot be undone.");
        ImGui::Spacing();

        // Buttons
        if (ImGui::Button("Yes, delete", ImVec2(140, 0))) {
            if (backend.onConfirmDelete().isSuccess())
            {
                printf("deleted\n");
                printf("move on to createPage\n");
                nextPage = PageType::CreateAccount;
            }
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel", ImVec2(140, 0)) || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            printf("canceled\n");
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
    return nextPage;
}
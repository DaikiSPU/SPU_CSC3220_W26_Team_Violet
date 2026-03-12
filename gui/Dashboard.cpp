#include "Dashboard.h"
#include "imgui_internal.h"
#include <vector>
#include <string>

Dashboard::Dashboard(UIContext &uiContext, BackendContext &backendContext) : Page(uiContext, backendContext), backend(backendContext)
{
    backend.refreshHeader(current_market_id);

    const auto& markets = data.markets;

    if (!markets.empty())
    {
        selectedMarketId = 0;
        current_market_id = markets[0].first;
        backend.refreshHeader(current_market_id);
    }
    else
    {
        selectedMarketId = -1;
        current_market_id = 0;
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

        // Bottom history (bottom 30%)
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

        // history系 → 同じ node
        ImGui::DockBuilderDockWindow("Trade History", dock_transactions_id);
        ImGui::DockBuilderDockWindow("Open Orders", dock_transactions_id);
        ImGui::DockBuilderDockWindow("Order History", dock_transactions_id);

        ImGui::DockBuilderDockWindow("Chart", dock_main_id);

        ImGui::DockBuilderFinish(dockspace_id);

    }

    // End host window
    ImGui::End();

    orderBookWindow();

    chartWindow();

    orderEntryWindow();

    transactionWindow();

    openOrdersWindow();

    orderHistoryWindow();

    if (backend.seeMatch())
    {
        backend.refreshHeader(current_market_id);
    }

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

        const auto& markets = data.markets;

        const char* currentMarketLabel = "No Market";
        if (!markets.empty() && selectedMarketId >= 0 && selectedMarketId < (int)markets.size())
        {
            currentMarketLabel = markets[selectedMarketId].second.c_str();
        }

        if (ImGui::BeginCombo("##Market", currentMarketLabel))
        {
            for (int i = 0; i < (int)markets.size(); i++)
            {
                bool isSelected = (selectedMarketId == i);

                if (ImGui::Selectable(markets[i].second.c_str(), isSelected))
                {
                    selectedMarketId = i;
                    current_market_id = markets[i].first;
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

    float halfHeight = ImGui::GetContentRegionAvail().y * 0.5f;
    // Section title
    ImGui::Text("Sell");

    // Begin sell table
    if (ImGui::BeginTable("##OrderBookAsks", 2, tableFlags, ImVec2(0, halfHeight)))
    {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("Price", ImGuiTableColumnFlags_WidthStretch, 1.0f);
        ImGui::TableSetupColumn("Size",  ImGuiTableColumnFlags_WidthStretch, 1.0f);
        ImGui::TableHeadersRow();

        for (const auto& level : book.asks)
        {
            ImGui::TableNextRow();

            if (backend.isMyOrder(level.user_id))
            {
                ImGui::TableSetColumnIndex(0);
                double price = level.price / 10000.0;
                ImGui::TextColored(ImVec4(1.0f,0.8f,0.2f,1.0f), "%.2f", price);

                ImGui::TableSetColumnIndex(1);
                double size = level.size / 10000.0;
                ImGui::TextColored(ImVec4(1.0f,0.8f,0.2f,1.0f), "%.2f", size);
            }
            else
            {
                ImGui::TableSetColumnIndex(0);
                double price = level.price / 10000.0;
                ImGui::Text("%.2f", price);

                ImGui::TableSetColumnIndex(1);
                double size = level.size / 10000.0;
                ImGui::Text("%.2f", size);
            }
        }

        ImGui::EndTable();
    }

    ImGui::Separator();

    // Section title
    ImGui::Text("Buy");

    // Begin buy table
    if (ImGui::BeginTable("##OrderBookBids", 2, tableFlags, ImVec2(0, halfHeight)))
    {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("Price", ImGuiTableColumnFlags_WidthStretch, 1.0f);
        ImGui::TableSetupColumn("Size",  ImGuiTableColumnFlags_WidthStretch, 1.0f);
        ImGui::TableHeadersRow();

        for (const auto& level : book.bids)
        {
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            double price = level.price / 10000.0;
            ImGui::Text("%.2f", price);

            ImGui::TableSetColumnIndex(1);
            double size = level.size / 10000.0;
            ImGui::Text("%.2f", size);
        }

        ImGui::EndTable();
    }

    ImGui::End();
}

void Dashboard::chartWindow()
{
    ImGui::Begin("Chart");

    auto result = backend.getTradeHistory(current_market_id);

    if (!result.isSuccess())
    {
        ImGui::Text("Failed to load chart data");
        ImGui::End();
        return;
    }

    const auto& trades = result.value;

    if (trades.empty())
    {
        ImGui::Text("No market data");
        ImGui::End();
        return;
    }

    struct Candle
    {
        float open = 0.0f;
        float high = 0.0f;
        float low = 0.0f;
        float close = 0.0f;
        std::string timeLabel;
    };

    std::vector<Candle> candles;

    const int tradesPerCandle = 5;
    const int tradeCount = (int)trades.size();

    for (int i = 0; i < tradeCount; i += tradesPerCandle)
    {
        int end = std::min(i + tradesPerCandle, tradeCount);
        if (end <= i)
            break;

        Candle c{};

        c.open = (float)(trades[i].price / 10000.0);
        c.close = (float)(trades[end - 1].price / 10000.0);
        c.high = c.open;
        c.low = c.open;

        if (!trades[i].time.empty())
        {
            if (trades[i].time.size() >= 8)
            {
                if (trades[i].time.size() >= 19)
                    c.timeLabel = trades[i].time.substr(11, 8);
                else
                    c.timeLabel = trades[i].time.substr(0, std::min((size_t)8, trades[i].time.size()));
            }
            else
            {
                c.timeLabel = trades[i].time;
            }
        }
        else
        {
            c.timeLabel = "-";
        }

        for (int j = i; j < end; ++j)
        {
            float p = (float)(trades[j].price / 10000.0);
            c.high = std::max(c.high, p);
            c.low = std::min(c.low, p);
        }

        candles.push_back(c);
    }

    if (candles.empty())
    {
        ImGui::Text("No candle data");
        ImGui::End();
        return;
    }

    ImGuiIO& io = ImGui::GetIO();

    bool hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup);

    if (hovered)
    {
        if (io.KeyCtrl)
        {
            if (io.MouseWheel > 0.0f)
                visibleCandles = std::max(10, visibleCandles - 2);
            else if (io.MouseWheel < 0.0f)
                visibleCandles = std::min((int)candles.size(), visibleCandles + 2);
        }
        else if (io.KeyShift)
        {
            chartScrollX -= (int)(io.MouseWheel * 3.0f);
        }
        else
        {
            chartScrollY += io.MouseWheel;
        }
    }

    const int totalCandles = (int)candles.size();

    if (visibleCandles < 1)
        visibleCandles = 1;

    if (visibleCandles > totalCandles)
        visibleCandles = totalCandles;

    int maxScrollX = std::max(0, totalCandles - visibleCandles);

    if (chartScrollX < 0)
        chartScrollX = 0;

    if (chartScrollX > maxScrollX)
        chartScrollX = maxScrollX;

    int startIndex = chartScrollX;
    int endIndex = std::min(startIndex + visibleCandles, totalCandles);

    if (startIndex >= endIndex)
    {
        ImGui::Text("Invalid chart range");
        ImGui::End();
        return;
    }

    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    ImVec2 canvasSize = ImGui::GetContentRegionAvail();

    if (canvasSize.x < 50.0f) canvasSize.x = 50.0f;
    if (canvasSize.y < 50.0f) canvasSize.y = 50.0f;

    ImDrawList* drawList = ImGui::GetWindowDrawList();

    drawList->AddRectFilled(
        canvasPos,
        ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y),
        IM_COL32(20, 20, 24, 255)
    );

    drawList->AddRect(
        canvasPos,
        ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y),
        IM_COL32(40, 150, 255, 255),
        0.0f,
        0,
        2.0f
    );

    const float leftAxisWidth = 70.0f;
    const float bottomAxisHeight = 28.0f;
    const float topPadding = 10.0f;
    const float rightPadding = 10.0f;

    float plotX0 = canvasPos.x + leftAxisWidth;
    float plotY0 = canvasPos.y + topPadding;
    float plotX1 = canvasPos.x + canvasSize.x - rightPadding;
    float plotY1 = canvasPos.y + canvasSize.y - bottomAxisHeight;

    float plotWidth = plotX1 - plotX0;
    float plotHeight = plotY1 - plotY0;

    if (plotWidth <= 1.0f || plotHeight <= 1.0f)
    {
        ImGui::Dummy(canvasSize);
        ImGui::End();
        return;
    }

    float minPrice = candles[startIndex].low;
    float maxPrice = candles[startIndex].high;

    for (int i = startIndex; i < endIndex; ++i)
    {
        minPrice = std::min(minPrice, candles[i].low);
        maxPrice = std::max(maxPrice, candles[i].high);
    }

    float range = maxPrice - minPrice;
    if (range <= 0.000001f)
        range = 1.0f;

    float verticalShift = chartScrollY * range * 0.1f;
    minPrice += verticalShift;
    maxPrice += verticalShift;

    float shiftedRange = maxPrice - minPrice;
    if (shiftedRange <= 0.000001f)
        shiftedRange = 1.0f;

    auto priceToY = [&](float price) -> float
    {
        float normalized = (price - minPrice) / shiftedRange;
        if (normalized < 0.0f) normalized = 0.0f;
        if (normalized > 1.0f) normalized = 1.0f;
        return plotY1 - normalized * plotHeight;
    };

    const int priceTicks = 5;

    for (int i = 0; i <= priceTicks; ++i)
    {
        float t = (float)i / (float)priceTicks;
        float y = plotY1 - t * plotHeight;
        float price = minPrice + t * shiftedRange;

        drawList->AddLine(
            ImVec2(plotX0, y),
            ImVec2(plotX1, y),
            IM_COL32(60, 60, 70, 180),
            1.0f
        );

        char buf[32];
        snprintf(buf, sizeof(buf), "%.2f", price);

        drawList->AddText(
            ImVec2(canvasPos.x + 6.0f, y - 7.0f),
            IM_COL32(200, 200, 210, 255),
            buf
        );
    }

    drawList->AddLine(
        ImVec2(plotX0, plotY0),
        ImVec2(plotX0, plotY1),
        IM_COL32(180, 180, 190, 255),
        1.0f
    );

    drawList->AddLine(
        ImVec2(plotX0, plotY1),
        ImVec2(plotX1, plotY1),
        IM_COL32(180, 180, 190, 255),
        1.0f
    );

    int visibleCount = endIndex - startIndex;
    float candleWidth = plotWidth / (float)visibleCount;
    if (candleWidth < 1.0f)
        candleWidth = 1.0f;

    float bodyHalfWidth = std::max(1.0f, candleWidth * 0.30f);

    for (int i = 0; i < visibleCount; ++i)
    {
        int idx = startIndex + i;
        const Candle& c = candles[idx];

        float x = plotX0 + i * candleWidth + candleWidth * 0.5f;

        float yOpen = priceToY(c.open);
        float yClose = priceToY(c.close);
        float yHigh = priceToY(c.high);
        float yLow = priceToY(c.low);

        bool up = c.close >= c.open;

        ImU32 color = up
            ? IM_COL32(46, 204, 113, 255)
            : IM_COL32(231, 76, 60, 255);

        float wickTop = std::min(yHigh, yLow);
        float wickBottom = std::max(yHigh, yLow);

        if (wickBottom - wickTop < 1.0f)
            wickBottom = wickTop + 1.0f;

        drawList->AddLine(
            ImVec2(x, wickTop),
            ImVec2(x, wickBottom),
            color,
            1.0f
        );

        float bodyTop = std::min(yOpen, yClose);
        float bodyBottom = std::max(yOpen, yClose);

        if (bodyBottom - bodyTop < 2.0f)
            bodyBottom = bodyTop + 2.0f;

        drawList->AddRectFilled(
            ImVec2(x - bodyHalfWidth, bodyTop),
            ImVec2(x + bodyHalfWidth, bodyBottom),
            color
        );
    }

    int labelStep = std::max(1, visibleCount / 6);

    for (int i = 0; i < visibleCount; i += labelStep)
    {
        int idx = startIndex + i;
        const Candle& c = candles[idx];

        float x = plotX0 + i * candleWidth + candleWidth * 0.5f;

        drawList->AddLine(
            ImVec2(x, plotY1),
            ImVec2(x, plotY1 + 4.0f),
            IM_COL32(180, 180, 190, 255),
            1.0f
        );

        const char* label = c.timeLabel.empty() ? "-" : c.timeLabel.c_str();

        drawList->AddText(
            ImVec2(x - 20.0f, plotY1 + 6.0f),
            IM_COL32(200, 200, 210, 255),
            label
        );
    }

    char zoomBuf[64];
    snprintf(zoomBuf, sizeof(zoomBuf), "Candles: %d  ScrollX: %d", visibleCandles, chartScrollX);
    drawList->AddText(
        ImVec2(plotX0 + 8.0f, plotY0 + 4.0f),
        IM_COL32(220, 220, 220, 255),
        zoomBuf
    );

    ImGui::Dummy(canvasSize);
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
        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
        
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Price");
        ImGui::TableSetColumnIndex(1);
        ImGui::SetNextItemWidth(-1);
        ImGui::InputDouble("##price", &price, 0.01, 1.0, "%.2f");

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Quantity");
        ImGui::TableSetColumnIndex(1);
        ImGui::SetNextItemWidth(-1);
        ImGui::InputDouble("##qty", &qty, 0.01, 1.0, "%.2f");

        ImGui::EndTable();
    }

    ImGui::Separator();

    if (ImGui::BeginTable("OrderForm2", 2, ImGuiTableFlags_SizingStretchProp))
    {
        float estimated = price * qty;
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
        float availableCash = backend.getAvailableCash() / 10000.0;
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

        float position = backend.getPosition() / 10000.0;
        char positionhText[32];
        snprintf(positionhText, sizeof(positionhText), "%.2f", position);
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

    bool validOrder = price > 0 && qty > 0 && orderState != OrderState::None;
    ImGui::BeginDisabled(!validOrder);
    if (ImGui::Button("Place Order", ImVec2(-1, 0)))
    {
        std::string side = (orderState == OrderState::BUY) ? "buy" : "sell";

        if (backend.isSuspiciousOrder(price, qty))
        {
            pendingSide = side;
            pendingPrice = price;
            pendingQty = qty;

            ImGui::OpenPopup("Confirm Suspicious Order");
        }
        else
        {
            Result<void> result =
                backend.placeOrder(current_market_id, side, price * 10000, qty * 10000);

            if (!result.isSuccess())
            {
                printf("PlaceOrderError\n");
                errorManager.addError(result.error);
            }
            else
            {
                backend.refreshHeader(current_market_id);
                orderState = OrderState::None;
            }
        }
        printf("cash after order: %lld\n", backend.getAvailableCash());
    }
    ImGui::EndDisabled();

    if (openPlaceOrderErrorPopup)
    {
        ImGui::OpenPopup("Place Order Error");
        openPlaceOrderErrorPopup = false;
    }

    if (Popup::showMessage("Place Order Error", errorManager.getErrors(), "OK"))
    {
        errorManager.clear();
    }
    if (orderConfirmPopup())
    {
        showConfirmOrderPopup = false;
        pendingSide = "";
        pendingPrice = 0;
        pendingQty = 0;
    }
    ImGui::End();
}

void Dashboard::transactionWindow()
{
    ImGui::Begin("Trade History");

    auto result = backend.getTradeHistory();

    if (!result.isSuccess())
    {
        ImGui::Text("Failed to load trade history");
        ImGui::End();
        return;
    }

    auto& trades = result.value;

    if (trades.empty())
    {
        ImGui::TextDisabled("No trade history yet.");
        ImGui::End();
        return;
    }

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

        int maxRows = std::min((int)trades.size(), 100);

        for (int i = trades.size() - 1; i >= 0 && maxRows > 0; --i, --maxRows)
        {
            const auto& t = trades[i];

            ImGui::TableNextRow();

            ImGui::TableNextColumn();
            ImGui::Text("%s", t.time.c_str());

            ImGui::TableNextColumn();
            ImGui::Text("%s", t.market.c_str());

            ImGui::TableNextColumn();
            if (t.aggressor_side == "buy")
                ImGui::TextColored(ImVec4(0.2f,0.8f,0.2f,1),"Buy");
            else
                ImGui::TextColored(ImVec4(0.9f,0.2f,0.2f,1),"Sell");

            ImGui::TableNextColumn();
            ImGui::Text("%.2f", t.price / 10000.0);

            ImGui::TableNextColumn();
            ImGui::Text("%.2f", t.qty / 10000.0);

            ImGui::TableNextColumn();
            ImGui::Text("%s", t.status.c_str());
        }

        ImGui::EndTable();
    }

    ImGui::End();
}


void Dashboard::openOrdersWindow()
{
    ImGui::Begin("Open Orders");

    auto result = backend.getOpenOrders();

    if (!result.isSuccess())
    {
        ImGui::Text("Failed to load orders");
        return;
    }

    auto& orders = result.value;

    if (orders.empty())
    {
        ImGui::TextDisabled("No open orders.");
        ImGui::End();
        return;
    }

    if (ImGui::BeginTable("OpenOrders", 7, tableFlags))
    {
        ImGui::TableSetupScrollFreeze(0,1);

        ImGui::TableSetupColumn("Time");
        ImGui::TableSetupColumn("Market");
        ImGui::TableSetupColumn("Side");
        ImGui::TableSetupColumn("Price");
        ImGui::TableSetupColumn("Remaining");
        ImGui::TableSetupColumn("Status");
        ImGui::TableSetupColumn("Action");

        ImGui::TableHeadersRow();

        for (int i = orders.size() - 1; i >= 0; --i)
        {
            auto& o = orders[i];

            ImGui::TableNextRow();

            ImGui::TableNextColumn();
            ImGui::Text("%s", o.time.c_str());

            ImGui::TableNextColumn();
            ImGui::Text("%s", o.market.c_str());

            ImGui::TableNextColumn();
            if (o.side == "buy")
                ImGui::TextColored(ImVec4(0.2f,0.8f,0.2f,1), "Buy");
            else
                ImGui::TextColored(ImVec4(0.9f,0.2f,0.2f,1), "Sell");

            ImGui::TableNextColumn();
            ImGui::Text("%.2f", o.price / 10000.0);

            ImGui::TableNextColumn();
            ImGui::Text("%.2f", o.qty_remaining / 10000.0);

            ImGui::TableNextColumn();
            ImGui::Text("%s", o.status.c_str());

            ImGui::TableNextColumn();

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f,0.2f,0.2f,1));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f,0.3f,0.3f,1));

            if (ImGui::Button(("Cancel##" + std::to_string(o.order_id)).c_str()))
            {
                Result<void> result = backend.cancelOrder(o.order_id);

                if (!result.isSuccess())
                {
                    errorManager.addError(result.error);
                    ImGui::OpenPopup("Cancel Order Error");
                }
                else
                {
                    backend.refreshHeader(current_market_id);
                }
            }

            ImGui::PopStyleColor(2);
        }

        ImGui::EndTable();
    }

    if (Popup::showMessage("Cancel Order Error", errorManager.getErrors(), "OK"))
    {
        errorManager.clear();
    }

    ImGui::End();
}

void Dashboard::orderHistoryWindow()
{
    ImGui::Begin("Order History");

    auto result = backend.getOrderHistory();
    if (!result.isSuccess())
    {
        ImGui::Text("Failed to load orders");
        return;
    }

    auto& orders = result.value;

    if (orders.empty())
    {
        ImGui::TextDisabled("No order history yet.");
        ImGui::End();
        return;
    }

    if (ImGui::BeginTable("OrderHistory", 7, tableFlags))
    {
        ImGui::TableSetupScrollFreeze(0,1);

        ImGui::TableSetupColumn("Time");
        ImGui::TableSetupColumn("Market");
        ImGui::TableSetupColumn("Side");
        ImGui::TableSetupColumn("Price");
        ImGui::TableSetupColumn("Quantity");
        ImGui::TableSetupColumn("Remaining");
        ImGui::TableSetupColumn("Status");

        ImGui::TableHeadersRow();

        for (int i = orders.size() - 1; i >= 0; --i)
        {
            auto& o = orders[i];

            ImGui::TableNextRow();

            ImGui::TableNextColumn();
            ImGui::Text("%s", o.time.c_str());

            ImGui::TableNextColumn();
            ImGui::Text("%s", o.marketSymbol.c_str());

            ImGui::TableNextColumn();
            ImGui::Text("%s", o.side.c_str());

            ImGui::TableNextColumn();
            ImGui::Text("%.2f", o.price / 10000.0);

            ImGui::TableNextColumn();
            ImGui::Text("%.2f", o.qty / 10000.0);

            ImGui::TableNextColumn();
            ImGui::Text("%.2f", o.qty_remaining / 10000.0);

            ImGui::TableNextColumn();
            ImGui::Text("%s", o.status.c_str());
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

bool Dashboard::orderConfirmPopup()
{
    bool closed = false;
    if (ImGui::BeginPopupModal("Confirm Suspicious Order", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("This order looks unusual.");
        ImGui::Separator();

        ImGui::Text("Side: %s", pendingSide.c_str());
        ImGui::Text("Price: %.2f", pendingPrice);
        ImGui::Text("Quantity: %.2f", pendingQty);
        ImGui::Text("Estimated: %.2f", pendingPrice * pendingQty);

        ImGui::Spacing();

        if (ImGui::Button("Confirm", ImVec2(120,0)))
        {
            Result<void> result =
                backend.placeOrder(current_market_id,
                                pendingSide,
                                (long long)(pendingPrice * 10000),
                                (long long)(pendingQty * 10000));

            if (!result.isSuccess())
            {
                errorManager.addError(result.error);
                openPlaceOrderErrorPopup = true;
            }
            else
            {
                backend.refreshHeader(current_market_id);
                orderState = OrderState::None;
            }

            closed = true;
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel", ImVec2(120,0)))
        {
            closed = true;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
    return closed;
}
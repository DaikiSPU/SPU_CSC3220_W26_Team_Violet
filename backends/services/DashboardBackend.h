#pragma once

#include "Database.h"
#include "BackendContext.h"

struct DashboardData
{
    std::vector<std::pair<int, std::string>> markets;
    long long rawLastPrice = 0;
    double fluctuation = 0.0;

    long long rawAvailableCash = 0;
    long long rawMarketValue = 0;
    long long rawUnrealized = 0;
    long long rawRealized = 0;
    long long rawEquity = 0;
};

struct TradeHistoryRow
{
    std::string time;
    std::string market;
    std::string side;
    long long price;
    long long qty;
    std::string status;
    std::string aggressor_side;
};

struct OpenOrdersRow
{
    long long order_id;
    std::string time;
    std::string market;
    std::string side;
    long long price;
    long long qty_remaining;
    std::string status;
};

struct market
{
    int marketId;
    std::string marketName;
    std::string marketSymbol;
};

class DashboardBackend {
public:
    DashboardBackend(BackendContext& backendContext);

    void refreshHeader(int marketId);
    OrderBookSnapshot refreshOrderBook(int marketId) const;
    Result<bool> onConfirmDelete();
    Result<void> placeOrder(int currentMarketId, std::string side, int price, int qty);
    const DashboardData& getData() const { return data; }
    long long getAvailableCash();
    long long getPosition(int currentMarketId);
    Result<std::vector<TradeHistoryRow>> getTradeHistory();
    Result<std::vector<OrderHistoryRow>> getOrderHistory();

    Result<std::vector<OpenOrdersRow>> getOpenOrders();
    Result<void> cancelOrder(long long orderId);

    

private:
    DashboardData data;
    Database& db;
    AppData& appData;
    Engine& engine;
    std::vector<market> markets;

    std::string getMarketName(int marketId);
    std::string getMarketSymbol(int marketId);
};
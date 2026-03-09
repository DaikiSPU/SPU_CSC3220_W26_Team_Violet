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

class DashboardBackend {
public:
    DashboardBackend(BackendContext& backendContext) : db(backendContext.db), appData(backendContext.appData), engine(backendContext.engine) {}

    void refreshHeader(int marketId);
    OrderBookSnapshot refreshOrderBook(int marketId) const;
    Result<bool> onConfirmDelete();
    const DashboardData& getData() const { return data; }

    

private:
    DashboardData data;
    Database& db;
    AppData& appData;
    Engine& engine;
};
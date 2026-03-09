#include "DashboardBackend.h"

void DashboardBackend::refreshHeader(int currentMarketId)
{
    int userId = appData.userId;
    data.markets = db.getAvailableMarkets();
    // Last Price
    data.rawLastPrice =
        db.getLastPriceRaw(currentMarketId);

    // Fluctuation
    auto prices =
        db.getPrevAndCurrentPricesRaw(currentMarketId);

    if (prices.second != 0)
    {
        data.fluctuation =
            ((prices.first - prices.second)
             / (double)prices.second) * 100.0;
    }
    else
    {
        data.fluctuation = 0.0;
    }

    // Cash
    data.rawAvailableCash =
        db.getAvailableCash(userId);

    // Position
    long long qty =
        db.getPositionQtyRaw(userId, currentMarketId);

    long long avg =
        db.getPositionAvePriceRaw(userId, currentMarketId);

    if (qty != 0)
    {
        data.rawMarketValue =
            data.rawLastPrice * qty / 100000000;

        data.rawUnrealized =
            (data.rawLastPrice - avg) * qty / 100000000;
    }
    else
    {
        data.rawMarketValue = 0;
        data.rawUnrealized = 0;
    }

    // Equity
    data.rawEquity =
        data.rawAvailableCash + data.rawMarketValue;

    // Realized
    data.rawRealized =
        db.getRealizedPnLRaw(userId);

    printf("Dashboard Header refreshed.\n");
}

OrderBookSnapshot DashboardBackend::refreshOrderBook(int currentMarketId) const
{
    return engine.getOrderBook(currentMarketId);
}

Result<bool> DashboardBackend::onConfirmDelete()
{
    printf("%d\n", appData.userId);
    Result<bool> isDeleted = db.deleteUserAccount(appData.userId);
    return isDeleted;
}

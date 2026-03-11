#include "DashboardBackend.h"

void DashboardBackend::refreshHeader(int currentMarketId)
{
    int userId = appData.userId;
    data.markets = db.getAvailableMarkets();
    // Last Price
    auto lastResult = db.getLastPriceRaw(currentMarketId);
    if (!lastResult.isSuccess())
    {
        printf("refresh header error last price\n");
        return;
    }

    data.rawLastPrice = lastResult.value;

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
    Result<long long> cashResult = db.getAvailableCash(userId);
    if (!cashResult.isSuccess())
    {
        printf("refresh header error\n");
        return;
    }
    data.rawAvailableCash = cashResult.value;

    // Position
    Result<long long> qtyResult = db.getPositionQtyRaw(userId, currentMarketId);
    if (!qtyResult.isSuccess())
    {
        printf("refresh header error\n");
        return;
    }
    long long qty = qtyResult.value;

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

Result<void> DashboardBackend::placeOrder(int currentMarketId, std::string side, int price, int qty)
{
    Result<void> placeOrderResult;
    if (getAvailableCash() < (price * qty) / 10000)
    {
        placeOrderResult.setError(ErrorType::System, "available cash is lower than estimated price.");
        return placeOrderResult;
    }

    Order order;

    order.user_id = appData.userId;
    order.bot_id = 0;

    order.market_id = currentMarketId;

    order.side = side;
    order.type = "limit";

    order.price = price;
    order.qty = qty;
    order.qty_remaining = qty;

    Result<void> result = engine.placeOrder(order);
    if (!result.isSuccess())
    {
        placeOrderResult.setError(ErrorType::System, result.error.getMessage());
        printf("%s\n", result.error.getMessage().c_str());
        return placeOrderResult;
    }
    return placeOrderResult;
}

long long DashboardBackend::getAvailableCash()
{
    return data.rawAvailableCash;
}

long long DashboardBackend::getPosition(int currentMarketId)
{
    return db.getPositionQtyRaw(appData.userId, currentMarketId).value; 
}

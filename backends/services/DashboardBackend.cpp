#include "DashboardBackend.h"

DashboardBackend::DashboardBackend(BackendContext &backendContext) : db(backendContext.db), appData(backendContext.appData), engine(backendContext.engine)
{
    markets.clear();
    auto availableMarkets = db.getAvailableMarkets();

    for (auto& m : availableMarkets)
    {
        int market_id = m.first;

        Result<std::string> marketNameResult = db.getMarketName(market_id);
        Result<std::string> symbolResult = db.getMarketSymbol(market_id);
        std::string marketName = marketNameResult.isSuccess() ? marketNameResult.value : "unkown";
        std::string marketSymbol = symbolResult.isSuccess() ? symbolResult.value : "unkown";

        markets.push_back({market_id, marketName, marketSymbol});
    }
}

void DashboardBackend::refreshHeader(int currentMarketId)
{
    int userId = appData.userId;
    data.markets = db.getAvailableMarkets();

    auto lastResult = db.getLastPriceRaw(currentMarketId);
    if (!lastResult.isSuccess())
    {
        printf("refresh header error last price\n");
        return;
    }

    data.rawLastPrice = lastResult.value;

    auto prices = db.getPrevAndCurrentPricesRaw(currentMarketId);

    if (prices.second != 0)
    {
        data.fluctuation =
            ((prices.first - prices.second) / (double)prices.second) * 100.0;
    }
    else
    {
        data.fluctuation = 0.0;
    }

    Result<long long> cashResult = db.getAvailableCash(userId);
    if (!cashResult.isSuccess())
    {
        printf("refresh header error\n");
        return;
    }
    data.rawAvailableCash = cashResult.value;

    Result<long long> qtyResult = db.getPositionQtyRaw(userId, 0, currentMarketId);
    if (!qtyResult.isSuccess())
    {
        printf("refresh header error\n");
        return;
    }

    data.rawPosition = qtyResult.value;
    long long qty = data.rawPosition;

    long long avg = db.getPositionAvePriceRaw(userId, currentMarketId);

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

    data.rawEquity =
        data.rawAvailableCash + data.rawMarketValue;

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
    if (side == "buy" && getAvailableCash() < (price * qty) / 10000)
    {
        placeOrderResult.setError(ErrorType::System, "available cash is lower than estimated price.");
        return placeOrderResult;
    }

    if (side != "buy" && side != "sell")
    {
        placeOrderResult.setError(ErrorType::Validation, "invalid side");
        return placeOrderResult;
    }

    Order order{};

    // owner
    order.user_id = appData.userId;
    order.bot_id = 0;

    // market
    order.market_id = currentMarketId;

    // order info
    order.side = side;
    order.type = "limit";

    // quantities
    order.price = price;
    order.qty = qty;
    order.qty_remaining = qty;

    // status
    order.status = "open";

    Result<void> result = engine.placeOrder(order);
    if (!result.isSuccess())
    {
        placeOrderResult.setError(ErrorType::System, result.error.getMessage());
        printf("Place Order: %s\n", result.error.getMessage().c_str());
        return placeOrderResult;
    }
    return placeOrderResult;
}

long long DashboardBackend::getAvailableCash()
{
    return data.rawAvailableCash;
}

long long DashboardBackend::getPosition()
{
    return data.rawPosition;
}

Result<std::vector<TradeHistoryRow>> DashboardBackend::getTradeHistory()
{
    auto trades = engine.getTradeHistory();

    std::vector<TradeHistoryRow> rows;

    for (auto& t : trades)
    {
        TradeHistoryRow r;

        r.market = getMarketSymbol(t.market_id);
        r.price = t.price;
        r.qty = t.qty;
        r.time = t.trade_time;

        // side (aggressor)
        r.side = t.aggressor_side;

        // status
        if (t.aggressor_side == "buy")
            r.status = t.buyStatus;
        else
            r.status = t.sellStatus;

        rows.push_back(r);
    }

    Result<std::vector<TradeHistoryRow>> result;
    result.value = rows;
    return result;
}

Result<std::vector<TradeHistoryRow>> DashboardBackend::getTradeHistory(int currentMarketId)
{
    auto trades = engine.getTradeHistory();

    std::vector<TradeHistoryRow> rows;

    for (auto& t : trades)
    {
        if (t.market_id != currentMarketId)
        {
            continue;
        }
        TradeHistoryRow r;

        r.market = getMarketSymbol(t.market_id);
        r.price = t.price;
        r.qty = t.qty;
        r.time = t.trade_time;

        // side (aggressor)
        r.side = t.aggressor_side;

        // status
        if (t.aggressor_side == "buy")
            r.status = t.buyStatus;
        else
            r.status = t.sellStatus;

        rows.push_back(r);
    }

    Result<std::vector<TradeHistoryRow>> result;
    result.value = rows;
    return result;
}

Result<std::vector<OrderHistoryRow>> DashboardBackend::getOrderHistory()
{
    auto orders = engine.getOrderHistory();

    std::vector<OrderHistoryRow> rows;

    for (auto& o : orders)
    {
        OrderHistoryRow r;

        r.order_id = o.order_id;
        time_t t = o.created_at;
        r.time = std::string(ctime(&t));
        r.time.pop_back();
        r.price = o.price;
        r.qty = o.qty;
        r.qty_remaining = o.qty_remaining;
        r.status = o.status;

        r.side = o.side;


        for (auto& m : markets)
        {
            if (m.marketId == o.market_id)
            {
                r.marketSymbol = m.marketSymbol;
                break;
            }
        }

        rows.push_back(r);
    }

    Result<std::vector<OrderHistoryRow>> result;
    result.value = rows;
    return result;
}


Result<std::vector<OpenOrdersRow>> DashboardBackend::getOpenOrders()
{
    std::vector<OpenOrdersRow> rows;

    for (auto& m : markets)
    {
        auto buyOrders = engine.getBuyOrders(m.marketId);
        auto sellOrders = engine.getSellOrders(m.marketId);

        for (auto& o : buyOrders)
        {
            if (o.user_id != appData.userId)
                continue;

            OpenOrdersRow r;

            r.order_id = o.order_id;

            time_t t = o.created_at;
            r.time = std::string(ctime(&t));
            if (!r.time.empty())
                r.time.pop_back();

            r.market = m.marketSymbol;
            r.side = "buy";
            r.price = o.price;
            r.qty_remaining = o.qty_remaining;
            r.status = o.status;

            rows.push_back(r);
        }

        for (auto& o : sellOrders)
        {
            if (o.user_id != appData.userId)
                continue;

            Result<bool> openResult = db.isOrderOpen(o.order_id);
            if (!openResult.isSuccess() || !openResult.value)
                continue;

            OpenOrdersRow r;

            r.order_id = o.order_id;

            time_t t = o.created_at;
            r.time = std::string(ctime(&t));
            if (!r.time.empty())
                r.time.pop_back();

            r.market = m.marketSymbol;
            r.side = "sell";
            r.price = o.price;
            r.qty_remaining = o.qty_remaining;
            r.status = o.status;

            rows.push_back(r);
        }
    }

    Result<std::vector<OpenOrdersRow>> result;
    result.value = rows;
    return result;
}

Result<void> DashboardBackend::cancelOrder(long long orderId)
{
    auto orderResult = engine.getOrder(orderId);

    if (!orderResult.isSuccess())
    {
        Result<void> result;
        result.setError(ErrorType::Validation, orderResult.error.getMessage());
        return result;
    }

    return engine.cancelOrder(orderResult.value);
}

std::string DashboardBackend::getMarketName(int marketId)
{
    for (const auto& m : markets)
    {
        if (m.marketId == marketId)
            return m.marketName;
    }

    return "Unknown";
}

std::string DashboardBackend::getMarketSymbol(int marketId)
{
    for (const auto& m : markets)
    {
        if (m.marketId == marketId)
            return m.marketSymbol;
    }

    return "Unknown";
}

bool DashboardBackend::isSuspiciousOrder(double price, double qty)
{
    long long last = data.rawLastPrice;

    double diff = std::fabs(price - last) / last;

    if (diff > 0.10)   // 10% deviation
        return true;

    long long estimated = price * qty;
    long long cash = getAvailableCash();

    if (estimated > cash * 0.8)
        return true;

    return false;
}

bool DashboardBackend::isMyOrder(int orderUserId)
{
    if (orderUserId == appData.userId)
    {
        return true;
    }
    return false;
}

bool DashboardBackend::seeMatch()
{
    bool sucess = engine.getMatchSuccess();
    if (sucess)
    {
        engine.setMatchSuccess();
        return true;
    }
    return false;
}

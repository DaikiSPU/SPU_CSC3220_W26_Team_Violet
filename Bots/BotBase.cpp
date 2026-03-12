#include "BotBase.h"

BotBase::BotBase(Database &db, Engine &engine, int botId) : db(db), engine(engine), botId(botId)
{
    loadMarketConfig();
}

long long BotBase::getMidPrice(Database &db, int market_id)
{
    // ---- Last Price ----
    long long lastPrice = 0;

    auto lastResult = db.getLastPriceRaw(market_id);

    if (lastResult.isSuccess())
    {
        lastPrice = lastResult.value;
    }

    if (lastPrice == 0)
    {
        auto refResult = db.getReferencePrice(market_id);

        if (refResult.isSuccess())
            lastPrice = refResult.value;
    }

    // ---- OrderBook ----
    auto bids = db.getTopBuyOrders(market_id, 1);
    auto asks = db.getTopSellOrders(market_id, 1);

    long long mid = lastPrice;

    if (!bids.empty() && !asks.empty())
    {
        long long bestBid = bids[0].first;
        long long bestAsk = asks[0].first;

        mid = (bestBid + bestAsk) / 2;
    }

    return mid;
}

Result<void> BotBase::sendLimitOrder(int market_id, const std::string &side, long long price, long long qty)
{
    Result<void> result;

    if (price <= 0 || qty <= 0)
    {
        result.setError(ErrorType::Validation, "invalid bot order");
        return result;
    }

    if (side != "buy" && side != "sell")
    {
        result.setError(ErrorType::Validation, "invalid side");
        return result;
    }

    Order order{};

    order.user_id = 0;
    order.bot_id = botId;

    order.market_id = market_id;

    order.side = side;
    order.type = "limit";

    order.price = price;
    order.qty = qty;
    order.qty_remaining = qty;

    order.status = "open";

    return engine.placeOrder(order);
}

void BotBase::sendMarketOrder(int market_id, const std::string& side, long long qty)
{
    printf("SEND Market ORDER[%s] -> botId=%d qty=%lld\n", side.c_str(), botId, qty);
    Order order;

    order.user_id = 0;
    order.bot_id = botId;
    order.market_id = market_id;
    order.side = side;
    order.type = "market";
    order.price = 0;

    order.qty = qty;
    order.qty_remaining = qty;
    order.status = "open";

    engine.placeOrder(order);
}

void BotBase::loadMarketConfig()
{
    markets.clear();
    auto availableMarkets = db.getAvailableMarkets();

    for (auto& m : availableMarkets)
    {
        int market_id = m.first;

        printf("marketId: %d\n", market_id);

        Result<std::string> marketNameResult = db.getMarketName(market_id);
        Result<long long> tickResult = db.getTickSize(market_id);
        Result<long long> lotResult  = db.getLotSize(market_id);

        std::string marketName = marketNameResult.isSuccess() ? marketNameResult.value : "unkown";
        long long tickSize = tickResult.isSuccess() ? tickResult.value : 100;
        long long lotSize  = lotResult.isSuccess()  ? lotResult.value  : 10000;

        markets.push_back({market_id, marketName, tickSize, lotSize});
    }
}

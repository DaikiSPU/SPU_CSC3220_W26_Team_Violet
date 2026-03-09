#include "BotBase.h"

BotBase::BotBase(Database &db, Engine &engine, int botId) : db(db), engine(engine), botId(botId)
{
    loadMarketConfig();
}

long long BotBase::getMidPrice(Database &db, int market_id)
{
    long long lastPrice = db.getLastPriceRaw(market_id);

    if (lastPrice == 0)
    {
        lastPrice = db.getReferencePrice(market_id).value;
    }

    auto bids = db.getTopBuyOrders(market_id, 1);
    auto asks = db.getTopSellOrders(market_id, 1);

    long long mid;

    if (!bids.empty() && !asks.empty())
    {
        mid = (bids[0].first + asks[0].first) / 2;
        printf("used order book\n");
    }
    else
    {
        mid = lastPrice;
        printf("used lastprice\n");
    }

    printf("GET MID DPRICE -> botId=%d mid=%lld\n", botId, mid);
    return mid;
}

void BotBase::sendLimitOrder(int market_id, const std::string &side, long long price, long long qty)
{
    printf("SEND LITMIT ORDER[%s] -> botId=%d price=%lld qty=%lld\n", side.c_str(), botId, price, qty);
    Order order;

    order.user_id = 0;
    order.bot_id = botId;

    order.market_id = market_id;

    order.side = side;
    order.type = "limit";

    order.price = price;
    order.qty = qty;
    order.qty_remaining = qty;

    engine.placeOrder(order);
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

        Result<long long> tickResult = db.getTickSize(market_id);
        Result<long long> lotResult  = db.getLotSize(market_id);

        long long tickSize = tickResult.isSuccess() ? tickResult.value : 100;
        long long lotSize  = lotResult.isSuccess()  ? lotResult.value  : 10000;

        markets.push_back({market_id, tickSize, lotSize});
    }
}

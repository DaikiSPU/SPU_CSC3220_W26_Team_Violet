#include "MarketMakerBot.h"

MarketMakerBot::MarketMakerBot(Database& db, Engine& engine, int botId) : BotBase(db, engine, botId)
{
    if (botId <= 0)
    {
        printf("Error MAKER constructor\n");
    }
    else
    {
        printf("MAKER constructor OK\n");
    }
    setBotActivityRate(activityRate);
}

void MarketMakerBot::run(int tick)
{
    if (markets.empty())
        return;

    for (auto& m : markets)
    {
        long long mid = getMidPrice(db, m.marketId);

        cancelFarOrders(m.marketId, mid, m.tickSize);

        cancelOldOrders(m.marketId);

        long long qtyMultiplier = (rand() % 10) + 1; // 1 to 5 lots
        long long qty = qtyMultiplier * m.lotSize;

        for (int i=1;i<=levels;i++)
        {
            long long buyPrice  = mid - i*m.tickSize;
            long long sellPrice = mid + i*m.tickSize;

            if (countMyOrdersAtPrice(m.marketId,"sell",sellPrice) < sameOrder)
            {
                sendLimitOrder(m.marketId,"sell", sellPrice, qty);
            }

            if (countMyOrdersAtPrice(m.marketId,"buy",buyPrice) < sameOrder)
            {
                sendLimitOrder(m.marketId,"buy", buyPrice, qty);
            }
        }

        // printf("RUN botId=%d marketId=%d spread=%lld mid=%lld\n", botId, m.marketId, spread, mid);
    }
}

Result<std::unique_ptr<BotBase>> MarketMakerBot::create(Database& db, Engine& engine)
{
    Result<std::unique_ptr<BotBase>> result;

    auto idResult = db.getBotId(std::string(BOT_NAME));

    if (!idResult.isSuccess())
    {
        result.setError(ErrorType::Database, "Failed to get System_MarketMaker bot id");
        return result;
    }

    int botId = idResult.value;

    if (botId <= 0)
    {
        result.setError(ErrorType::Validation, "Invalid MarketMaker bot id");
        return result;
    }

    auto bot = std::make_unique<MarketMakerBot>(db, engine, botId);

    result.value = std::move(bot);
    return result;
}

int MarketMakerBot::countMyOrdersAtPrice(
    int market_id,
    const std::string& side,
    long long price)
{
    int count = 0;

    auto orders =
        (side == "buy") ? engine.getBuyOrders(market_id)
                        : engine.getSellOrders(market_id);

    for (const auto& o : orders)
    {
        if (o.bot_id == botId &&
            o.price == price &&
            o.qty_remaining > 0)
        {
            count++;
        }
    }

    return count;
}

void MarketMakerBot::cancelFarOrders(int marketId, long long mid, long long tickSize)
{
    auto sells = engine.getSellOrders(marketId);

    for (auto& o : sells)
    {
        if (o.bot_id != botId)
            continue;

        if (std::llabs(o.price - mid) > deleteThreshold * tickSize)
        {
            engine.cancelOrder(o.order_id);
        }
    }

    auto buys = engine.getBuyOrders(marketId);

    for (auto& o : buys)
    {
        if (o.bot_id != botId)
            continue;

        if (std::llabs(o.price - mid) > deleteThreshold * tickSize)
        {
            engine.cancelOrder(o.order_id);
        }
    }
}

void MarketMakerBot::cancelOldOrders(int marketId)
{
    auto sells = engine.getSellOrders(marketId);

    for (auto& o : sells)
    {
        if (o.bot_id != botId)
            continue;

        if (std::time(nullptr) - o.created_at > orderMaxAge)
        {
            engine.cancelOrder(o.order_id);
        }
    }

    auto buys = engine.getBuyOrders(marketId);

    for (auto& o : buys)
    {
        if (o.bot_id != botId)
            continue;

        if (std::time(nullptr) - o.created_at > orderMaxAge)
        {
            engine.cancelOrder(o.order_id);
        }
    }
}

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
    printf("RUN botId=%d\n", botId);
    if (markets.empty())
        return;

    for (auto& m : markets)
    {
        printf("Market ID: %d\n", m.marketId);
        long long qtyMultiplier = (rand() % 10) + 1; // 1 to 5 lots
        long long qty = qtyMultiplier * m.lotSize;

        long long spread = 2 * m.tickSize;

        printf("spread: %lld\n", spread);

        long long mid = getMidPrice(db, m.marketId);

        auto cancelBuyResult = db.cancelUnusedBotOrders(
            botId,
            m.marketId,
            "buy",
            mid,
            10 * m.tickSize, // price_distance_limit
            3,   // max_orders_per_side
            30   // max_order_age_seconds
        );

        if (cancelBuyResult.isSuccess())
        {
            engine.markOrdersCancelled(cancelBuyResult.value);
            engine.cleanTopBuyBook(m.marketId);
        }

        auto cancelSellResult = db.cancelUnusedBotOrders(
            botId,
            m.marketId,
            "sell",
            mid,
            10 * m.tickSize, // price_distance_limit
            3,   // max_orders_per_side
            30   // max_order_age_seconds
        );

        if (cancelSellResult.isSuccess())
        {
            engine.markOrdersCancelled(cancelSellResult.value);
            engine.cleanTopSellBook(m.marketId);
        }

        for (int i=1;i<=levels;i++)
        {
            long long buyPrice  = mid - i*m.tickSize;
            long long sellPrice = mid + i*m.tickSize;

            sendLimitOrder(m.marketId,"buy", buyPrice, qty);
            sendLimitOrder(m.marketId,"sell", sellPrice, qty);
        }
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

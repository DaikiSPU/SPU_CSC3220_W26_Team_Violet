#include "PanicBot.h"

PanicBot::PanicBot(Database& db, Engine& engine, int botId) : BotBase(db, engine, botId)
{
    if (botId <= 0)
    {
        printf("Error PANIC constructor\n");
    }
    else
    {
        printf("PANIC constructor OK\n");
    }
    setBotActivityRate(activityRate);
}

void PanicBot::run(int tick)
{
    if (markets.empty())
        return;

    for (auto& m : markets)
    {
        long long mid = getMidPrice(db, m.marketId);
        long long last = db.getLastPriceRaw(m.marketId);

        // ---- CIRCUIT BREAKER ----
        if (last < mid * 0.90)
        {
            printf("Circuit Breaker triggered market=%d\n", m.marketId);
            return;
        }

        // panic trigger (5% drop)
        if (last > mid * 0.95)
            continue;

        long long qtyMultiplier = (rand() % 10) + 5;
        long long qty = qtyMultiplier * m.lotSize;

        sendMarketOrder(m.marketId, "sell", qty);

        printf("PANIC SELL market=%d qty=%lld\n", m.marketId, qty);
    }
}

Result<std::unique_ptr<BotBase>> PanicBot::create(Database& db, Engine& engine)
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

    auto bot = std::make_unique<PanicBot>(db, engine, botId);

    result.value = std::move(bot);
    return result;
}
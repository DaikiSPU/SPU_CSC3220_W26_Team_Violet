#include "GreedBot.h"

GreedBot::GreedBot(Database &db, Engine &engine, int botId) : BotBase(db, engine, botId)
{
    if (botId <= 0)
    {
        printf("Error GREED constructor\n");
    }
    else
    {
        printf("GREED constructor OK\n");
    }
    setBotActivityRate(activityRate);
}

void GreedBot::run(int tick)
{
    if (markets.empty())
        return;

    if (tick % 6 != 0)
        return;

    for (auto& m : markets)
    {
        auto prices = db.getPrevAndCurrentPricesRaw(m.marketId);

        long long latest = prices.first;
        long long prev   = prices.second;

        if (latest == 0)
            latest = db.getReferencePrice(m.marketId).value;

        if (prev == 0)
            prev = latest;

        long long qty = ((rand()%4)+1) * m.lotSize;

        if (latest > prev)
        {
            sendMarketOrder(m.marketId,"buy",qty);
        }
        else if (latest < prev)
        {
            sendMarketOrder(m.marketId,"sell",qty);
        }
    }
}

Result<std::unique_ptr<BotBase>> GreedBot::create(Database& db, Engine& engine)
{
    Result<std::unique_ptr<BotBase>> result;

    auto idResult = db.getBotId(std::string(BOT_NAME));

    if (!idResult.isSuccess())
    {
        result.setError(ErrorType::Database, "Failed to get Bot_Greed bot id");
        return result;
    }

    int botId = idResult.value;

    if (botId <= 0)
    {
        result.setError(ErrorType::Validation, "Invalid Greed bot id");
        return result;
    }

    auto bot = std::make_unique<GreedBot>(db, engine, botId);

    result.value = std::move(bot);
    return result;
}
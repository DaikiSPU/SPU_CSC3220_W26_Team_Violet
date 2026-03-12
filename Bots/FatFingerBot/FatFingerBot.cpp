#include "FatFingerBot.h"
#include <algorithm>

FatFingerBot::FatFingerBot(Database& db, Engine& engine, int botId) : BotBase(db, engine, botId)
{
    if (botId <= 0)
    {
        printf("Error FATFINGER constructor\n");
    }
    else
    {
        printf("FATFINGER constructor OK\n");
    }
    setBotActivityRate(activityRate);
}

void FatFingerBot::run(int tick)
{
    if (markets.empty())
        return;

    if (rand() % 40 != 0)
        return;

    for (auto& m : markets)
    {
        long long mid = getMidPrice(db, m.marketId);

        long long qty = ((rand() % 20) + 10) * m.lotSize;

        bool buy = rand() % 2;

        long long extremeTicks = (rand() % 80) + 40;

        long long price;

        if (buy)
        {
            price = mid + extremeTicks * m.tickSize;
            sendLimitOrder(m.marketId, "buy", price, qty);
        }
        else
        {
            price = std::max(m.tickSize, mid - extremeTicks * m.tickSize);
            sendLimitOrder(m.marketId, "sell", price, qty);
        }
    }
}

Result<std::unique_ptr<BotBase>> FatFingerBot::create(Database& db, Engine& engine)
{
    Result<std::unique_ptr<BotBase>> result;

    auto idResult = db.getBotId(std::string(BOT_NAME));

    if (!idResult.isSuccess())
    {
        result.setError(ErrorType::Database, "Failed to get Bot_FatFinger bot id");
        return result;
    }

    int botId = idResult.value;

    if (botId <= 0)
    {
        result.setError(ErrorType::Validation, "Invalid FatFinger bot id");
        return result;
    }

    auto bot = std::make_unique<FatFingerBot>(db, engine, botId);

    result.value = std::move(bot);
    return result;
}
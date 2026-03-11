#include "SineWaveBot.h"
#include <cmath>

SineWaveBot::SineWaveBot(Database &db, Engine &engine, int botId) : BotBase(db, engine, botId)
{
    if (botId <= 0)
    {
        printf("Error SINEWAVE constructor\n");
    }
    else
    {
        printf("SINEWAVE constructor OK\n");
    }
    setBotActivityRate(activityRate);
}

void SineWaveBot::run(int tick)
{
    if (markets.empty())
        return;

    for (auto& m : markets)
    {
        long long mid = getMidPrice(db, m.marketId);

        double wave = sin(tick / 8.0);

        long long offset = (long long)(wave * 5) * m.tickSize;

        long long price = std::max(m.tickSize, mid + offset);

        std::string side = wave > 0 ? "buy" : "sell";

        long long qty = ((rand()%3)+1) * m.lotSize;

        sendLimitOrder(m.marketId, side, price, qty);
    }
}

Result<std::unique_ptr<BotBase>> SineWaveBot::create(Database& db, Engine& engine)
{
    Result<std::unique_ptr<BotBase>> result;

    auto idResult = db.getBotId(std::string(BOT_NAME));

    if (!idResult.isSuccess())
    {
        result.setError(ErrorType::Database, "Failed to get Bot_SineWave bot id");
        return result;
    }

    int botId = idResult.value;

    if (botId <= 0)
    {
        result.setError(ErrorType::Validation, "Invalid SineWave bot id");
        return result;
    }

    auto bot = std::make_unique<SineWaveBot>(db, engine, botId);

    result.value = std::move(bot);
    return result;
}
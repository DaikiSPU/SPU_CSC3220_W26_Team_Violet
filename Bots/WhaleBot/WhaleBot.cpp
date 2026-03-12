#include "WhaleBot.h"

WhaleBot::WhaleBot(Database& db, Engine& engine, int botId) : BotBase(db, engine, botId)
{
    if (botId <= 0)
    {
        printf("Error WHALE constructor\n");
    }
    else
    {
        printf("WHALE constructor OK\n");
    }
    setBotActivityRate(activityRate);
}

void WhaleBot::run(int tick)
{
    if (markets.empty()) return;

    for (auto& m : markets)
    {
        long long mid = getMidPrice(db, m.marketId);
        auto lastResult = db.getLastPriceRaw(m.marketId);
        if (!lastResult.isSuccess())
            return;

        long long last = lastResult.value;

        if (mid <= 0 || last <= 0) continue;

        int strategy = rand() % 3;

        if (strategy == 0)
        {
            icebergExecution(m.marketId, mid, m.lotSize, m.tickSize);
        }
        else if (strategy == 1)
        {
            momentumTrade(m.marketId, last, mid, m.lotSize);
        }
        else
        {
            liquidityWall(m.marketId, mid, m.lotSize, m.tickSize);
        }
    }
}

Result<std::unique_ptr<BotBase>> WhaleBot::create(Database &db, Engine &engine)
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

    auto bot = std::make_unique<WhaleBot>(db, engine, botId);

    result.value = std::move(bot);
    return result;
}

void WhaleBot::icebergExecution(int marketId, long long mid, long long lotSize, long long tickSize)
{
    int slices = 5 + rand() % 5;

    long long price = mid - tickSize * (rand() % 3);

    for (int i = 0; i < slices; i++)
    {
        long long qty = lotSize * (10 + rand() % 10);

        sendLimitOrder(marketId, "buy", price, qty);
    }

    printf("Whale iceberg executed\n");
}

void WhaleBot::momentumTrade(int marketId, long long last, long long mid, long long lotSize)
{
    long long qty = lotSize * (20 + rand() % 20);
    long long price = 0;

    if (last > mid)
    {
        price = last + 1;
        sendLimitOrder(marketId, "buy", price, qty);
    }
    else
    {
        price = last - 1;
        sendLimitOrder(marketId, "sell", price, qty);
    }

    printf("Whale momentum trade\n");
}

void WhaleBot::liquidityWall(int marketId, long long mid, long long lotSize, long long tickSize)
{
    long long price;
    std::string side;

    if (rand() % 2 == 0)
    {
        side = "buy";
        price = mid - tickSize * 2;
    }
    else
    {
        side = "sell";
        price = mid + tickSize * 2;
    }

    long long qty = lotSize * (50 + rand() % 50);

    sendLimitOrder(marketId, side, price, qty);

    printf("Whale liquidity wall\n");
}
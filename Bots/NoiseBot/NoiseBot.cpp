#include "NoiseBot.h"

NoiseBot::NoiseBot(Database& db, Engine& engine, int botId) : BotBase(db, engine, botId)
{
    if (botId <= 0)
    {
        printf("Error NOISE constructor\n");
    }
    else
    {
        printf("NOISE constructor OK\n");
    }
    setBotActivityRate(activityRate);
}

void NoiseBot::run(int tick)
{
    if (markets.empty())
        return;

    int marketIndex = rand() % markets.size();
    auto m = markets[marketIndex];

    long long mid = getMidPrice(db, m.marketId);

    auto bids = db.getTopBuyOrders(m.marketId, 1);
    auto asks = db.getTopSellOrders(m.marketId, 1);

    std::string side;
    long long price = mid;

    int behavior = rand() % 100;

    // 40%: aggressive order near best price
    if (behavior < 40)
    {
        side = (rand() % 2 == 0) ? "buy" : "sell";

        if (side == "buy")
        {
            if (!asks.empty())
                price = asks[0].first;
            else
                price = mid + m.tickSize;
        }
        else
        {
            if (!bids.empty())
                price = bids[0].first;
            else
                price = mid - m.tickSize;
                if (price <= 0)
                    price = mid;
        }
    }
    // 40%: passive order near the book
    else if (behavior < 80)
    {
        side = (rand() % 2 == 0) ? "buy" : "sell";

        int offsetTicks = (rand() % 2) + 1; // 1 or 2 ticks

        if (side == "buy")
        {
            if (!bids.empty())
                price = bids[0].first - offsetTicks * m.tickSize;
            else
                price = mid - offsetTicks * m.tickSize;
                if (price <= 0)
                    price = mid;
        }
        else
        {
            if (!asks.empty())
                price = asks[0].first + offsetTicks * m.tickSize;
            else
                price = mid + offsetTicks * m.tickSize;
        }
    }
    // 20%: around mid price
    else
    {
        side = (rand() % 2 == 0) ? "buy" : "sell";

        int offsetTicks = (rand() % 3) - 1; // -1, 0, 1
        if (side == "buy")
            price = mid - offsetTicks * m.tickSize;
            if (price <= 0)
                price = mid;
        else
            price = mid + offsetTicks * m.tickSize;
    }

    price = std::max(price, m.tickSize);

    long long qtyMultiplier = (rand() % 10) + 1; // 1 to 5 lots
    long long qty = qtyMultiplier * m.lotSize;

    printf("NoiseBot run: botId=%d market=%d side=%s price=%lld qty=%lld address=%p\n",
           botId, m.marketId, side.c_str(), price, qty, this);

    sendLimitOrder(m.marketId, side, price, qty);
}

Result<std::unique_ptr<BotBase>> NoiseBot::create(Database& db, Engine& engine)
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

    auto bot = std::make_unique<NoiseBot>(db, engine, botId);

    result.value = std::move(bot);
    return result;
}

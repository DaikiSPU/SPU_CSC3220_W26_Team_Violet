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
    if (tick % 2 != 0) return;

    long long base_price = 1000000;

    Order w;
    w.user_id = 0;
    w.bot_id = botId;
    w.market_id = 2;
    w.side = "buy";
    w.type = "limit";
    w.price = base_price - 100000;
    w.qty = 100000000;
    w.qty_remaining = w.qty;
    w.status = "open";

    Result<void> placeOrderResult = engine.placeOrder(w);
    if (placeOrderResult.isSuccess())
    {
        printf("WhaleBot did run\n");
    }
    else
    {
        printf("WhaleBot failed\n");
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

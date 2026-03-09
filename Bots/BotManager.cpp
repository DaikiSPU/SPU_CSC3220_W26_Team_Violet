#include "BotManager.h"
#include <algorithm>
#include <random>

BotManager::BotManager()
{
}

void BotManager::runAll(int tick)
{
    static std::mt19937 rng(std::random_device{}());

    // Randomize bot execution order
    std::shuffle(bots.begin(), bots.end(), rng);

    for (auto& bot : bots)
    {
        if (tick < bot->getNextActionTick())
            continue;

        if (rand() % 100 < bot->getBotActivityRate())
        {
            printf("===== BOTID: %d =====\n", bot->getBotId());
            bot->run(tick);

            int delay = rand() % 5 + 1;
            bot->setNextAcitonTick(tick + delay);

            printf("=======================\n");
        }
    }
}

Result<void> BotManager::createBots(Database& db, Engine& engine)
{
    Result<void> result;

    // MarketMaker
    auto makerResult = MarketMakerBot::create(db, engine);
    if (!makerResult.isSuccess())
    {
        result.setError(ErrorType::Fatal, makerResult.error.getMessage());
        return result;
    }

    addBot(std::move(makerResult.value));

    // NoiseBot
    auto noiseResult = NoiseBot::create(db, engine);
    if (!noiseResult.isSuccess())
    {
        result.setError(ErrorType::Fatal, noiseResult.error.getMessage());
        return result;
    }

    addBot(std::move(noiseResult.value));

    // PanicBot
    auto panicResult = PanicBot::create(db, engine);
    if (!panicResult.isSuccess())
    {
        result.setError(ErrorType::Fatal, panicResult.error.getMessage());
        return result;
    }

    addBot(std::move(panicResult.value));

    // WhaleBot
    auto whaleResult = WhaleBot::create(db, engine);
    if (!whaleResult.isSuccess())
    {
        result.setError(ErrorType::Fatal, whaleResult.error.getMessage());
        return result;
    }

    addBot(std::move(whaleResult.value));

    return result;
}
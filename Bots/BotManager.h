#pragma once

#include <vector>
#include "BotBase.h"
#include "MarketMakerBot.h"
#include "NoiseBot.h"
#include "PanicBot.h"
#include "WhaleBot.h"

class BotManager {
private:
    std::vector<std::unique_ptr<BotBase>> bots;

    int marketMakerBotId;
    int noiseBotId;
    int panicBotId;
    int whaleBotId;
    int fatFingerBotId;
    int sineWaveBotId;
    int greedBotId;

public:
    BotManager();
    Result<void> createBots(Database& db, Engine& engine);
    void addBot(std::unique_ptr<BotBase> bot) {
        bots.push_back(std::move(bot));
    }

    void runAll(int tick);
};
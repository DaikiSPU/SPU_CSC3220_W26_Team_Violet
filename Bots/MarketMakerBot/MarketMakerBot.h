#pragma once
#include "BotBase.h"

class MarketMakerBot : public BotBase {
    public:
        MarketMakerBot(Database& db, Engine& engine, int botId);
        void run(int tick) override;
        static Result<std::unique_ptr<BotBase>> create(
            Database& db,
            Engine& engine
        );
    private:
        static constexpr std::string_view BOT_NAME = "System_MarketMaker";
        const int activityRate = 15;
        const int levels = 3;
};
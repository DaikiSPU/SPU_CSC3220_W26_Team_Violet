#pragma once

#include "BotBase.h"

class WhaleBot : public BotBase {
    public:
        WhaleBot(Database& db, Engine& engine, int botId);
        void run(int tick) override;
        static Result<std::unique_ptr<BotBase>> create(
            Database& db,
            Engine& engine
        );
    private:
        static constexpr std::string_view BOT_NAME = "Bot_Whale";
        const int activityRate = 1;

        void icebergExecution(int market_id, long long mid, long long lotSize, long long tickSize);
        void momentumTrade(int market_id, long long last, long long mid, long long lotSize);
        void liquidityWall(int market_id, long long mid, long long lotSize, long long tickSize);
};
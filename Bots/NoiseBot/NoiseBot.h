#pragma once
#include "BotBase.h"

class NoiseBot : public BotBase 
{
    public:
        NoiseBot(Database& db, Engine& engine, int botId);
        void run(int tick) override;
        static Result<std::unique_ptr<BotBase>> create(
            Database& db,
            Engine& engine
        );
    protected:
    private:
        static constexpr std::string_view BOT_NAME = "System_Noise";
        int activityRate = 1;
};
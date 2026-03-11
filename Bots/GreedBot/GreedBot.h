#pragma once
#include "BotBase.h"

class GreedBot : public BotBase
{
public:
    GreedBot(Database& db, Engine& engine, int botId);
    static Result<std::unique_ptr<BotBase>> create(Database&, Engine&);
    void run(int tick) override;

private:
    static constexpr std::string_view BOT_NAME = "Bot_Greed";
    const int activityRate = 1;
};
#pragma once

#include "Engine.h"
#include "Database.h"

struct MarketInfo
{
    int marketId;
    long long tickSize;
    long long lotSize;
};

enum class BotType
{
    MarketMaker,
    Noise
};

class BotBase {
    public:
        virtual void run(int tick) = 0;
        BotBase(Database& db, Engine& engine, int botId);
        virtual ~BotBase() = default;

        int getBotId() const { return botId; }
        int getBotActivityRate() const { return botActivityRate; }
        void setBotActivityRate(int inputBotActivityRate) { botActivityRate = inputBotActivityRate; }
        int getNextActionTick() const { return nextActionTick; }
        void setNextAcitonTick(const int inputNextActionTick) { nextActionTick = inputNextActionTick; }

    protected:
        // Calculate mid price
        long long getMidPrice(Database& db, int market_id);

        // Send limit order
        void sendLimitOrder(
            int market_id,
            const std::string& side,
            long long price,
            long long qty);

        void sendMarketOrder(int market_id, const std::string& side, long long qty);

        void loadMarketConfig();

        int botId;

        Database& db;
        Engine& engine;

        std::vector<MarketInfo> markets;

    private:
        int nextActionTick = 0;  // next tick when bot can act
        int botActivityRate = 0;
};
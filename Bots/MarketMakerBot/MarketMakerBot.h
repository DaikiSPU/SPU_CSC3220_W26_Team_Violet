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
        const int activityRate = 9;
        const int levels = 3;
        const int sameOrder = 2;
        const int deleteThreshold = 10;
        const int orderMaxAge = 30;
        const int limit = 10000000;

        int countMyOrdersAtPrice(int market_id, const std::string& side, long long price);
        void cancelFarOrders(int marketId, long long mid, long long tick);
        void cancelOldOrders(int marketId);
        void inventoryControl(int marketId);
};
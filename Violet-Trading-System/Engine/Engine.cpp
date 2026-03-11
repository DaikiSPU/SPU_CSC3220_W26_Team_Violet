//
//  Engine.cpp
//  Violet-Trading-System
//

#include "Engine.h"
#include <algorithm>

Engine::Engine(Database& database) : db(database) {
    std::cout << "Matching Engine Initialized." << std::endl;
}

Result<void> Engine::placeOrder(Order new_order)
{
    Result<void> placeOrderResult;

    long long lockedCashAmount = 0;
    bool cashLocked = false;
    bool positionLocked = false;

    bool isUser = new_order.user_id > 0;
    bool isBot  = new_order.bot_id > 0;

    // ---- MARKET ORDER ----
    if (new_order.type == "market")
    {
        Result<std::pair<long long,long long>> dbResult = db.addOrder(new_order);

        if (!dbResult.isSuccess())
        {
            placeOrderResult.setError(ErrorType::Database, dbResult.error.getMessage());
            printf("[ENGINE] ADDORDER %s\n", placeOrderResult.error.getMessage().c_str());
            return placeOrderResult;
        }

        new_order.order_id = dbResult.value.first;
        new_order.created_at = dbResult.value.second;

        orderMarketMap[new_order.order_id] = new_order.market_id;

        matchMarketOrder(new_order);
        return placeOrderResult;
    }

    // ---- BUY ORDER ----
    if (new_order.side == "buy")
    {
        if (isUser)
        {
            long long notional = (new_order.price * new_order.qty) / 10000;

            auto lockResult = db.lockCash(
                new_order.user_id,
                notional
            );

            if (!lockResult.isSuccess())
            {
                placeOrderResult.setError(ErrorType::Validation, lockResult.error.getMessage());
                printf("[ENGINE] BUY %s\n", placeOrderResult.error.getMessage().c_str());
                return placeOrderResult;
            }

            lockedCashAmount = notional;
            cashLocked = true;
        }
    }

    // ---- SELL ORDER ----
    else if (new_order.side == "sell")
    {
        if (isUser || isBot)
        {
            auto lockPosResult = db.lockPosition(
                new_order.user_id, new_order.bot_id,
                new_order.market_id,
                new_order.qty
            );

            if (!lockPosResult.isSuccess())
            {
                placeOrderResult.setError(ErrorType::Validation, lockPosResult.error.getMessage());
                printf("[ENGINE] SELL %s\n", placeOrderResult.error.getMessage().c_str());
                return placeOrderResult;
            }

            positionLocked = true;
        }
    }
    else
    {
        placeOrderResult.setError(ErrorType::Validation, "invalid side");
        printf("[ENGINE] %s\n", placeOrderResult.error.getMessage().c_str());
        return placeOrderResult;
    }

    // ---- INSERT ORDER ----
    Result<std::pair<long long,long long>> dbResult = db.addOrder(new_order);

    if (!dbResult.isSuccess())
    {
        if (cashLocked)
        {
            if (isUser)
                db.releaseCash(new_order.user_id, lockedCashAmount);
            else
                db.releaseCash(new_order.bot_id, lockedCashAmount);
        }

        if (positionLocked)
        {
            if (isUser)
                db.releasePosition(new_order.user_id, new_order.bot_id, new_order.market_id, new_order.qty);
            else
                db.releasePosition(new_order.user_id, new_order.bot_id, new_order.market_id, new_order.qty);
        }

        placeOrderResult.setError(ErrorType::Database, dbResult.error.getMessage());
        return placeOrderResult;
    }

    new_order.order_id = dbResult.value.first;
    new_order.created_at = dbResult.value.second;

    orderMarketMap[new_order.order_id] = new_order.market_id;

    // ---- ORDERBOOK ----
    if (new_order.side == "buy")
        buyBooks[new_order.market_id].push(new_order);
    else
        sellBooks[new_order.market_id].push(new_order);

    // ---- MATCH ----
    match(new_order.market_id);

    return placeOrderResult;
}

OrderBookSnapshot Engine::getOrderBook(int market_id)
{
    OrderBookSnapshot snapshot;

    auto buyCopy = buyBooks[market_id];
    auto sellCopy = sellBooks[market_id];

    // BUY side
    while (!buyCopy.empty())
    {
        Order o = buyCopy.top();
        buyCopy.pop();

        snapshot.bids.push_back({o.price, o.qty_remaining});
    }

    // SELL side
    while (!sellCopy.empty())
    {
        Order o = sellCopy.top();
        sellCopy.pop();

        snapshot.asks.push_back({o.price, o.qty_remaining});
    }

    return snapshot;
}

void Engine::cleanupCancelledAndFilled(int market_id)
{
    cleanTopBuyBook(market_id);
    cleanTopSellBook(market_id);
}

Result<void> Engine::cancelOrder(long long order_id)
{
    Result<void> result;

    auto it = orderMarketMap.find(order_id);

    if (it == orderMarketMap.end())
    {
        result.setError(ErrorType::Validation, "order_id not found in orderMarketMap");
        return result;
    }

    int market_id = it->second;

    Result<void> dbResult = db.updateOrder(order_id, 0, "canceled");

    if (!dbResult.isSuccess())
    {
        result.setError(ErrorType::Database, dbResult.error.getMessage());
        return result;
    }

    // map cleanup
    orderMarketMap.erase(it);

    // memory cleanup
    cleanupCancelledAndFilled(market_id);

    return result;
}

void Engine::cleanTopBuyBook(int market_id)
{
    auto& book = buyBooks[market_id];

    while (!book.empty())
    {
        Order top = book.top();

        // DBの状態を確認
        auto openResult = db.isOrderOpen(top.order_id);

        if (!openResult.isSuccess() || !openResult.value)
        {
            book.pop();
            continue;
        }

        if (top.qty_remaining == 0)
        {
            book.pop();
            continue;
        }

        break;
    }
}

void Engine::cleanTopSellBook(int market_id)
{
    auto& book = sellBooks[market_id];

    while (!book.empty())
    {
        Order top = book.top();

        auto openResult = db.isOrderOpen(top.order_id);

        if (!openResult.isSuccess() || !openResult.value)
        {
            book.pop();
            continue;
        }

        if (top.qty_remaining == 0)
        {
            book.pop();
            continue;
        }

        break;
    }
}

void Engine::match(int market_id)
{
    auto& buys = buyBooks[market_id];
    auto& sells = sellBooks[market_id];

    while (true)
    {
        cleanTopBuyBook(market_id);
        cleanTopSellBook(market_id);
        if (buys.empty() || sells.empty())
            break;

        Order bestBuy = buys.top();
        Order bestSell = sells.top();

        bool sameOwner =
        (bestBuy.user_id == bestSell.user_id && bestBuy.user_id != 0) ||
        (bestBuy.bot_id == bestSell.bot_id && bestBuy.bot_id != 0);

        if (sameOwner)
        {
            break;
        }

        if (bestBuy.price < bestSell.price)
            break;

        buys.pop();
        sells.pop();

        long long tradeQty = std::min(bestBuy.qty_remaining, bestSell.qty_remaining);
        long long tradePrice = bestSell.price;

        long long buyRemainingAfter = bestBuy.qty_remaining - tradeQty;
        long long sellRemainingAfter = bestSell.qty_remaining - tradeQty;

        std::string buyStatus = (buyRemainingAfter == 0) ? "filled" : "open";
        std::string sellStatus = (sellRemainingAfter == 0) ? "filled" : "open";

        Trade t;
        t.market_id = market_id;
        t.buy_order_id = bestBuy.order_id;
        t.sell_order_id = bestSell.order_id;
        t.price = tradePrice;
        t.qty = tradeQty;

        Result<bool> tradeResult = db.recordTradeAndUpdateOrders(
            t,
            bestBuy,
            bestSell,
            buyRemainingAfter,
            sellRemainingAfter,
            buyStatus,
            sellStatus
        );

        if (!tradeResult.isSuccess())
        {
            std::cerr << "[Engine] recordTradeAndUpdateOrders failed: "
                      << tradeResult.error.getMessage()
                      << std::endl;
            return;
        }

        bestBuy.qty_remaining = buyRemainingAfter;
        bestSell.qty_remaining = sellRemainingAfter;

        if (bestBuy.qty_remaining > 0)
            buys.push(bestBuy);

        if (bestSell.qty_remaining > 0)
            sells.push(bestSell);
    }
}

void Engine::matchMarketOrder(Order& incoming)
{
    int market_id = incoming.market_id;

    if (incoming.side == "buy")
    {
        auto& book = sellBooks[market_id];

        while (incoming.qty_remaining > 0)
        {

            if (book.empty())
                break;

            Order bestAsk = book.top();
            book.pop();

            long long tradeQty = std::min(incoming.qty_remaining, bestAsk.qty_remaining);
            long long tradePrice = bestAsk.price;

            long long incomingRemainingAfter = incoming.qty_remaining - tradeQty;
            long long askRemainingAfter = bestAsk.qty_remaining - tradeQty;

            std::string incomingStatus = (incomingRemainingAfter == 0) ? "filled" : "partial";
            std::string askStatus = (askRemainingAfter == 0) ? "filled" : "open";

            Trade t;
            t.market_id = market_id;
            t.buy_order_id = incoming.order_id;
            t.sell_order_id = bestAsk.order_id;
            t.price = tradePrice;
            t.qty = tradeQty;

            Result<bool> tradeResult = db.recordTradeAndUpdateOrders(
                t,
                incoming,
                bestAsk,
                incomingRemainingAfter,
                askRemainingAfter,
                incomingStatus,
                askStatus
            );

            if (!tradeResult.isSuccess())
            {
                std::cerr << "[Engine] recordTradeAndUpdateOrders failed: "
                          << tradeResult.error.getMessage()
                          << std::endl;

                book.push(bestAsk);
                return;
            }

            incoming.qty_remaining = incomingRemainingAfter;
            bestAsk.qty_remaining = askRemainingAfter;

            if (bestAsk.qty_remaining > 0)
                book.push(bestAsk);
        }
    }
    else
    {
        auto& book = buyBooks[market_id];

        while (incoming.qty_remaining > 0)
        {

            if (book.empty())
                break;

            Order bestBid = book.top();
            book.pop();

            long long tradeQty = std::min(incoming.qty_remaining, bestBid.qty_remaining);
            long long tradePrice = bestBid.price;

            long long bidRemainingAfter = bestBid.qty_remaining - tradeQty;
            long long incomingRemainingAfter = incoming.qty_remaining - tradeQty;

            std::string bidStatus = (bidRemainingAfter == 0) ? "filled" : "open";
            std::string incomingStatus = (incomingRemainingAfter == 0) ? "filled" : "partial";

            Trade t;
            t.market_id = market_id;
            t.buy_order_id = bestBid.order_id;
            t.sell_order_id = incoming.order_id;
            t.price = tradePrice;
            t.qty = tradeQty;

            Result<bool> tradeResult = db.recordTradeAndUpdateOrders(
                t,
                bestBid,
                incoming,
                bidRemainingAfter,
                incomingRemainingAfter,
                bidStatus,
                incomingStatus
            );

            if (!tradeResult.isSuccess())
            {
                std::cerr << "[Engine] recordTradeAndUpdateOrders failed: "
                          << tradeResult.error.getMessage()
                          << std::endl;

                book.push(bestBid);
                return;
            }

            bestBid.qty_remaining = bidRemainingAfter;
            incoming.qty_remaining = incomingRemainingAfter;

            if (bestBid.qty_remaining > 0)
                book.push(bestBid);
        }
    }

    if (incoming.qty_remaining > 0)
    {
        Result<void> updateIncomingResult = db.updateOrder(incoming.order_id, incoming.qty_remaining, "canceled");
        if (!updateIncomingResult.isSuccess())
        {
            std::cerr << "[Engine] failed to finalize market order cancel: "
                      << updateIncomingResult.error.getMessage()
                      << std::endl;
        }
    }
    else
    {
        Result<void> updateIncomingResult = db.updateOrder(incoming.order_id, 0, "filled");
        if (!updateIncomingResult.isSuccess())
        {
            std::cerr << "[Engine] failed to finalize market order fill: "
                      << updateIncomingResult.error.getMessage()
                      << std::endl;
        }
    }
}

std::vector<Order> Engine::getBuyOrders(int market_id)
{
    std::vector<Order> orders;
    auto copy = buyBooks[market_id];

    while (!copy.empty())
    {
        orders.push_back(copy.top());
        copy.pop();
    }

    return orders;
}

std::vector<Order> Engine::getSellOrders(int market_id)
{
    std::vector<Order> orders;
    auto copy = sellBooks[market_id];

    while (!copy.empty())
    {
        orders.push_back(copy.top());
        copy.pop();
    }

    return orders;
}
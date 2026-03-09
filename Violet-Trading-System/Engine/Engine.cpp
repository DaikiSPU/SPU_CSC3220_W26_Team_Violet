//
//  Engine.cpp
//  Violet-Trading-System
//

#include "Engine.h"
#include <algorithm>

Engine::Engine(Database& database) : db(database) {
    std::cout << "Matching Engine Initialized." << std::endl;
}

Result<void> Engine::placeOrder(Order new_order) {
    Result<void> placeOrderResult;
    Result<long long> dbIdResult = db.addOrder(new_order);

    if (!dbIdResult.isSuccess())
    {
        std::cerr << "[Engine] addOrder failed: "
                << dbIdResult.error.getMessage()
                << std::endl;
        placeOrderResult.setError(ErrorType::Database, dbIdResult.error.getMessage());
        return placeOrderResult;
    }

    new_order.order_id = dbIdResult.value;

    if (new_order.type == "market")
    {
        matchMarketOrder(new_order);
        return placeOrderResult;
    }
    else
    {
        if (new_order.side == "buy")
            buyBooks[new_order.market_id].push(new_order);
        else
            sellBooks[new_order.market_id].push(new_order);

        match(new_order.market_id);
    }

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

void Engine::markOrdersCancelled(const std::vector<int>& ids)
{
    printf("markOrdersCancelled: %zu\n", ids.size());
    for (int id : ids)
        cancelledOrders.insert(id);
}

void Engine::cleanTopBuyBook(int market_id)
{
    auto& book = buyBooks[market_id];

    printf("cancel orders: %zu\n", cancelledOrders.size());

    while (!book.empty())
    {
        Order top = book.top();

        if (cancelledOrders.count(top.order_id))
        {
            cancelledOrders.erase(top.order_id);   // ★ここ
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

    printf("remained cancel orders: %zu\n", cancelledOrders.size());
}

void Engine::cleanTopSellBook(int market_id)
{
    auto& book = sellBooks[market_id];

    while (!book.empty())
    {
        Order top = book.top();

        if (cancelledOrders.count(top.order_id))
        {
            cancelledOrders.erase(top.order_id);   // ★ここ
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

        if (buys.empty() || sells.empty())
            break;

        Order bestBuy = buys.top();
        Order bestSell = sells.top();

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
            bestBuy.order_id,
            buyRemainingAfter,
            buyStatus,
            bestSell.order_id,
            sellRemainingAfter,
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

            std::string incomingStatus = (incomingRemainingAfter == 0) ? "filled" : "open";
            std::string askStatus = (askRemainingAfter == 0) ? "filled" : "open";

            Trade t;
            t.market_id = market_id;
            t.buy_order_id = incoming.order_id;
            t.sell_order_id = bestAsk.order_id;
            t.price = tradePrice;
            t.qty = tradeQty;

            Result<bool> tradeResult = db.recordTradeAndUpdateOrders(
                t,
                incoming.order_id,
                incomingRemainingAfter,
                incomingStatus,
                bestAsk.order_id,
                askRemainingAfter,
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
            std::string incomingStatus = (incomingRemainingAfter == 0) ? "filled" : "open";

            Trade t;
            t.market_id = market_id;
            t.buy_order_id = bestBid.order_id;
            t.sell_order_id = incoming.order_id;
            t.price = tradePrice;
            t.qty = tradeQty;

            Result<bool> tradeResult = db.recordTradeAndUpdateOrders(
                t,
                bestBid.order_id,
                bidRemainingAfter,
                bidStatus,
                incoming.order_id,
                incomingRemainingAfter,
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
        Result<void> updateIncomingResult = db.updateOrder(incoming.order_id, incoming.qty_remaining, "cancelled");
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
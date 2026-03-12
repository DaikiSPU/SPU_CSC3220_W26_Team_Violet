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

        setOrderHistory(new_order);

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
        if (isUser || (isBot && new_order.bot_id == 7))
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
            db.releaseCash(new_order.user_id, lockedCashAmount);
        }

        if (positionLocked)
        {
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

    setOrderHistory(new_order);

    // ---- MATCH ----
    match(new_order.market_id, new_order.side);

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

        snapshot.bids.push_back({o.user_id, o.bot_id, o.price, o.qty_remaining});
    }

    // SELL side
    while (!sellCopy.empty())
    {
        Order o = sellCopy.top();
        sellCopy.pop();

        snapshot.asks.push_back({o.user_id, o.bot_id, o.price, o.qty_remaining});
    }

    return snapshot;
}

void Engine::cleanupCancelledAndFilled(int market_id)
{
    cleanTopBuyBook(market_id);
    cleanTopSellBook(market_id);
}

Result<void> Engine::cancelOrder(const Order& order)
{
    Result<void> result;

    Order canceledOrder = order;

    long long order_id = order.order_id;

    auto it = orderMarketMap.find(order_id);

    if (it == orderMarketMap.end())
    {
        result.setError(ErrorType::Validation, "order_id not found in orderMarketMap");
        return result;
    }

    int market_id = it->second;

    // ---- DB UPDATE ----
    Result<void> dbResult = db.updateOrder(
        order_id,
        order.qty_remaining,
        "canceled"
    );

    if (!dbResult.isSuccess())
    {
        result.setError(ErrorType::Database, dbResult.error.getMessage());
        return result;
    }

    // ---- RELEASE LOCKS ----
    if (order.side == "buy")
    {
        long long refundAmount = (order.price * order.qty_remaining) / 10000;

        if (order.user_id > 0)
        {
            db.releaseCash(order.user_id, refundAmount);
        }
    }
    else if (order.side == "sell")
    {
        db.releasePosition(
            order.user_id,
            order.bot_id,
            order.market_id,
            order.qty_remaining
        );
    }

    // ---- CLEAN MAP ----
    orderMarketMap.erase(it);

    // ---- CLEAN ORDERBOOK ----
    cleanupCancelledAndFilled(market_id);

    canceledOrder.status = "canceled";
    setOrderHistory(canceledOrder);

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

void Engine::setTrade(Trade& t)
{
    tradeHistory.push_back(t);

    if (tradeHistory.size() > MAX_TRADE_HISTORY)
        tradeHistory.erase(tradeHistory.begin());
}

void Engine::setOrderHistory(Order new_order)
{
    if (new_order.user_id > 0)
    {
        orderHistory.push_back(new_order);
        if (orderHistory.size() > MAX_ORDER_HISTORY)
        {
            orderHistory.erase(orderHistory.begin());
        }
    }
}

void Engine::match(int market_id, const std::string& aggressorSide)
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
            printf("same owner\n");
            printf("bestBuy %lld\n", bestBuy.created_at);
            printf("bestsell %lld\n", bestSell.created_at);
            if (bestBuy.created_at > bestSell.created_at)
                buys.pop();
            else
                sells.pop();
            continue;
        }

        if (bestBuy.price < bestSell.price)
            break;

        buys.pop();
        sells.pop();

        long long tradeQty = std::min(bestBuy.qty_remaining, bestSell.qty_remaining);
        long long tradePrice = bestSell.price;

        long long buyRemainingAfter = bestBuy.qty_remaining - tradeQty;
        long long sellRemainingAfter = bestSell.qty_remaining - tradeQty;

        std::string buyStatus = (buyRemainingAfter == 0) ? "filled" : "partial";
        std::string sellStatus = (sellRemainingAfter == 0) ? "filled" : "partial";

        Trade t;
        t.market_id = market_id;
        t.buy_order_id = bestBuy.order_id;
        t.sell_order_id = bestSell.order_id;
        t.price = tradePrice;
        t.qty = tradeQty;
        t.buyStatus = buyStatus;
        t.sellStatus = sellStatus;

        t.buy_user_id = bestBuy.user_id;
        t.sell_user_id = bestSell.user_id;

        t.buy_bot_id = bestBuy.bot_id;
        t.sell_bot_id = bestSell.bot_id;

        // time
        time_t now = time(nullptr);
        t.trade_time = std::string(ctime(&now));
        t.trade_time.pop_back(); // remove newline

        t.aggressor_side = aggressorSide;


        Result<bool> tradeResult = db.recordTradeAndUpdateOrders(
            t,
            bestBuy,
            bestSell,
            buyRemainingAfter,
            sellRemainingAfter
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

        bestBuy.status = buyStatus;
        bestSell.status = sellStatus;

        if (bestBuy.qty_remaining > 0)
            buys.push(bestBuy);

        if (bestSell.qty_remaining > 0)
            sells.push(bestSell);

        setTrade(t);
        setOrderHistory(bestBuy);
        setOrderHistory(bestSell);

        if (bestSell.user_id > 0 || bestBuy.user_id > 0)
        {
            printf("MATCH!!!!!!!!\n");
            matchSuccess = true;
        }
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
            std::string askStatus = (askRemainingAfter == 0) ? "filled" : "partial";

            Trade t;
            t.market_id = market_id;
            t.buy_order_id = incoming.order_id;
            t.sell_order_id = bestAsk.order_id;
            t.price = tradePrice;
            t.qty = tradeQty;
            t.buyStatus = incomingStatus;
            t.sellStatus = askStatus;

            t.buy_user_id = incoming.user_id;
            t.sell_user_id = bestAsk.user_id;

            t.buy_bot_id = incoming.bot_id;
            t.sell_bot_id = bestAsk.bot_id;

            // time
            time_t now = time(nullptr);
            t.trade_time = std::string(ctime(&now));
            t.trade_time.pop_back();

            t.aggressor_side = "buy";

            Result<bool> tradeResult = db.recordTradeAndUpdateOrders(
                t,
                incoming,
                bestAsk,
                incomingRemainingAfter,
                askRemainingAfter
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

            incoming.status = incomingStatus;
            bestAsk.status = askStatus;

            setTrade(t);
            setOrderHistory(incoming);
            setOrderHistory(bestAsk);

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

            std::string bidStatus = (bidRemainingAfter == 0) ? "filled" : "partial";
            std::string incomingStatus = (incomingRemainingAfter == 0) ? "filled" : "partial";

            Trade t;
            t.market_id = market_id;
            t.buy_order_id = bestBid.order_id;
            t.sell_order_id = incoming.order_id;
            t.price = tradePrice;
            t.qty = tradeQty;
            t.buyStatus = bidStatus;
            t.sellStatus = incomingStatus;

            t.buy_user_id = bestBid.user_id;
            t.sell_user_id = incoming.user_id;

            t.buy_bot_id = bestBid.bot_id;
            t.sell_bot_id = incoming.bot_id;

            // time
            time_t now = time(nullptr);
            t.trade_time = std::string(ctime(&now));
            t.trade_time.pop_back();

            t.aggressor_side = "sell";

            Result<bool> tradeResult = db.recordTradeAndUpdateOrders(
                t,
                bestBid,
                incoming,
                bidRemainingAfter,
                incomingRemainingAfter
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

            bestBid.status = bidStatus;
            incoming.status = incomingStatus;

            setTrade(t);
            setOrderHistory(bestBid);
            setOrderHistory(incoming);

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
    cleanTopBuyBook(market_id);

    std::vector<Order> orders;
    auto copy = buyBooks[market_id];

    while (!copy.empty())
    {
        Order o = copy.top();
        copy.pop();

        auto openResult = db.isOrderOpen(o.order_id);
        if (!openResult.isSuccess() || !openResult.value)
            continue;

        if (o.qty_remaining <= 0)
            continue;

        orders.push_back(o);
    }

    return orders;
}

std::vector<Order> Engine::getSellOrders(int market_id)
{
    cleanTopSellBook(market_id);

    std::vector<Order> orders;
    auto copy = sellBooks[market_id];

    while (!copy.empty())
    {
        Order o = copy.top();
        copy.pop();

        auto openResult = db.isOrderOpen(o.order_id);
        if (!openResult.isSuccess() || !openResult.value)
            continue;

        if (o.qty_remaining <= 0)
            continue;

        orders.push_back(o);
    }

    return orders;
}

Result<Order> Engine::getOrder(long long orderId)
{
    Result<Order> result;

    auto it = orderMarketMap.find(orderId);

    if (it == orderMarketMap.end())
    {
        result.setError(ErrorType::Validation, "order not found");
        return result;
    }

    int marketId = it->second;

    auto buys = getBuyOrders(marketId);
    auto sells = getSellOrders(marketId);

    for (auto& o : buys)
    {
        if (o.order_id == orderId)
        {
            result.value = o;
            return result;
        }
    }

    for (auto& o : sells)
    {
        if (o.order_id == orderId)
        {
            result.value = o;
            return result;
        }
    }

    result.setError(ErrorType::Validation, "order not found");
    return result;
}


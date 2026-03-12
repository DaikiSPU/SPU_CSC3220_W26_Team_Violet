//
//  Engine.h
//  Violet-Trading-System
//

#ifndef ENGINE_H
#define ENGINE_H

#include <queue>
#include <vector>
#include <iostream>
#include <unordered_map> 
#include <unordered_set>
#include "../Models/Models.h"
#include "../Database/Database.h"

#include <ctime>

struct BookContent
{
    long long price;
    long long size;
};

struct OrderBookSnapshot
{
    std::vector<BookContent> bids;
    std::vector<BookContent> asks;
};

struct BuyCompare
{
    bool operator()(const Order& a, const Order& b) const
    {
        if (a.price != b.price) return a.price < b.price;
        return a.order_id > b.order_id;
    }
};

struct SellCompare
{
    bool operator()(const Order& a, const Order& b) const
    {
        if (a.price != b.price) return a.price > b.price;
        return a.order_id > b.order_id;
    }
};

class Engine {
private:
    Database& db; 
    
    std::unordered_map<int, std::priority_queue<Order, std::vector<Order>, BuyCompare>>  buyBooks;
    std::unordered_map<int, std::priority_queue<Order, std::vector<Order>, SellCompare>> sellBooks;

    void match(int market_id, const std::string& side); 
    void matchMarketOrder(Order& incoming);

    void cleanTopBuyBook(int market_id);
    void cleanTopSellBook(int market_id);

    void setTrade(Trade& t);
    void setOrderHistory(Order newOrder);

    std::unordered_map<long long, int> orderMarketMap;

    std::vector<Trade> tradeHistory;

    std::vector<Order> orderHistory;
    const int MAX_ORDER_HISTORY = 200;
    const int MAX_TRADE_HISTORY = 200;




public:
    Engine(Database& database);
    Result<void> placeOrder(Order new_order);
    OrderBookSnapshot getOrderBook(int market_id);

    std::vector<Order> getBuyOrders(int market_id);
    std::vector<Order> getSellOrders(int market_id);

    Result<Order> getOrder(long long orderId);

    void cleanupCancelledAndFilled(int market_id);

    Result<void> cancelOrder(const Order& order);

    std::vector<Trade> getTradeHistory() { return tradeHistory; };

    std::vector<Order> getOrderHistory() { return orderHistory; }
};

#endif

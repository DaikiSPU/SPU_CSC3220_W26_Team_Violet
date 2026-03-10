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

    void match(int market_id); 
    void matchMarketOrder(Order& incoming);

    void cleanTopBuyBook(int market_id);
    void cleanTopSellBook(int market_id);

    std::unordered_map<long long, int> orderMarketMap;

public:
    Engine(Database& database);
    Result<void> placeOrder(Order new_order);
    OrderBookSnapshot getOrderBook(int market_id);

    std::vector<Order> getBuyOrders(int market_id);
    std::vector<Order> getSellOrders(int market_id);

    void cleanupCancelledAndFilled(int market_id);

    Result<void> cancelOrder(long long order_id);
};

#endif

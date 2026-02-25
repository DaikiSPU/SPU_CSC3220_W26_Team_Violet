//
//  Engine.h
//  Violet-Trading-System
//
//  Created by Nam Nguyen on 2/13/26.
//

#ifndef ENGINE_H
#define ENGINE_H

#include <queue>
#include <vector>
#include <iostream>
#include "../Models/Models.h"
#include "../Database/Database.h"

// Comparator: Higher price = Higher priority for BUY orders
struct DescendingPrice {
    bool operator()(const Order& a, const Order& b) {
        return a.price < b.price;
    }
};

// Comparator: Lower price = Higher priority for SELL orders
struct AscendingPrice {
    bool operator()(const Order& a, const Order& b) {
        return a.price > b.price;
    }
};

class Engine {
private:
    Database& db; // Reference to the DB (for saving trades)
    
    // The "Hot Path" Memory
    std::priority_queue<Order, std::vector<Order>, DescendingPrice> buy_orders;
    std::priority_queue<Order, std::vector<Order>, AscendingPrice> sell_orders;

    // Internal matching logic
    void match();

public:
    // Constructor requires a Database reference
    Engine(Database& database);

    // Main entry point for new orders
    void placeOrder(Order new_order);
};

#endif

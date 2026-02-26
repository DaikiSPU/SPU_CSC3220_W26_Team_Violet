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
#include <unordered_map> // For separating markets
#include "../Models/Models.h"
#include "../Database/Database.h"

struct DescendingPrice {
    bool operator()(const Order& a, const Order& b) { return a.price < b.price; }
};

struct AscendingPrice {
    bool operator()(const Order& a, const Order& b) { return a.price > b.price; }
};

class Engine {
private:
    Database& db; 
    
    // Maps separate a unique Buy/Sell queue for EVERY market_id
    std::unordered_map<int, std::priority_queue<Order, std::vector<Order>, DescendingPrice>> buy_books;
    std::unordered_map<int, std::priority_queue<Order, std::vector<Order>, AscendingPrice>> sell_books;

    void match(int market_id); // Match only within a specific market

public:
    Engine(Database& database);
    void placeOrder(Order new_order);
};

#endif

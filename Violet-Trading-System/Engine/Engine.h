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
    
    std::unordered_map<int, std::priority_queue<Order, std::vector<Order>, DescendingPrice>> buy_books;
    std::unordered_map<int, std::priority_queue<Order, std::vector<Order>, AscendingPrice>> sell_books;

    void match(int market_id); 

public:
    Engine(Database& database);
    void placeOrder(Order new_order);
};

#endif

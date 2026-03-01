//
//  Database.h
//  Violet-Trading-System
//

#ifndef DATABASE_H
#define DATABASE_H

#include <sqlite3.h>
#include <string>
#include <iostream>
#include <vector>
#include <utility> 
#include "../Models/Models.h"

class Database {
private:
    sqlite3* db;
    void executeQuery(const std::string& query);

public:
    Database(const std::string& filename);
    ~Database();
    
    void initTables();
    
    std::vector<std::pair<int, std::string>> getAvailableMarkets();
    
    long long addOrder(Order& order);
    void recordTrade(Trade& trade);
    void updateOrder(long long order_id, long long new_qty_remaining, const std::string& new_status);
    
    bool registerUser(const std::string& username, const std::string& password);
    long long loginUser(const std::string& username, const std::string& password);
    
    double getAvailableCash(long long user_id);
    std::vector<OrderHistoryRow> getUserOrderHistory(long long user_id);
    
    std::vector<std::pair<double, int>> getTopBuyOrders(int market_id, int limit = 5);
    std::vector<std::pair<double, int>> getTopSellOrders(int market_id, int limit = 5);
    
    void updateHighScore(const std::string& username, double cash, const std::string& rank);
    void showLeaderboard();
    
    // --- CRUD DELETE REQUIREMENT ---
    void deleteUserAccount(long long user_id, const std::string& username);

    // --- NEW: INVENTORY MANAGEMENT DECLARATIONS ---
    double getTrueAvailableCash(long long user_id);
    double getAvailablePosition(long long user_id, int market_id);
};

#endif

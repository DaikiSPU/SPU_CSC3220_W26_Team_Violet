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

#include "Result.h"

class Database {
private:
    sqlite3* db;
    Result<void> beginTransaction();
    Result<void> commit();
    Result<void> rollback();
    Result<void> executeQuery(const std::string& query);
    Result<std::vector<int>> executeQueryWithResult(const std::string& sql);

public:
    Database(const std::string& filename);
    ~Database();
    
    void initTables();
    
    std::vector<std::pair<int, std::string>> getAvailableMarkets();
    
    Result<std::pair<long long,long long>> addOrder(const Order& order);
    Result<bool> recordTradeAndUpdateOrders(
        const Trade& t,
        long long buy_id,
        long long buy_remaining,
        const std::string& buy_status,
        long long sell_id,
        long long sell_remaining,
        const std::string& sell_status);

    Result<bool> recordTrade(const Trade& trade);
    Result<void> updateOrder(long long order_id, long long new_qty_remaining, const std::string& new_status);
    Result<bool> isOrderOpen(long long order_id);

    Result<bool> hasAnyUser();
    Result<bool> registerUser(const std::string& username, const std::string& password, const std::string& pinHash);
    Result<std::string> getUsername(const int userId);
    Result<int>getUserId(const std::string& username);
    Result<int> getBotId(const std::string& botName);
    long long loginUser(const std::string& username, const std::string& password);
    Result<std::string> getPasswordHash(const std::string& username);
    
    long long getAvailableCash(int user_id);
    long long getLastPriceRaw(int market_id);
    long long getPositionQtyRaw(int user_id, int market_id);
    long long getPositionAvePriceRaw(int user_id, int market_id);
    std::pair<long long, long long> getPrevAndCurrentPricesRaw(int market_id);
    long long getRealizedPnLRaw(long long user_id);
    std::vector<OrderHistoryRow> getUserOrderHistory(int user_id);
    
    std::vector<std::pair<long long, long long>> getTopBuyOrders(int market_id, int limit = 5);
    std::vector<std::pair<long long, long long>> getTopSellOrders(int market_id, int limit = 5);

    Result<long long> getTickSize(int market_id);
    Result<long long> getLotSize(int market_id);
    Result<long long> getReferencePrice(int market_id);
    
    void updateHighScore(const std::string& username, double cash, const std::string& rank);
    void showLeaderboard();
    
    // --- CRUD DELETE REQUIREMENT ---
    Result<bool> deleteUserAccount(long long user_id);

    // --- NEW: INVENTORY MANAGEMENT DECLARATIONS ---
    double getTrueAvailableCash(long long user_id);
    double getAvailablePosition(long long user_id, int market_id);
};

#endif

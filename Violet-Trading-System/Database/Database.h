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
        const Order& buy,
        const Order& sell,
        long long buy_remaining,
        long long sell_remaining,
        const std::string& buy_status,
        const std::string& sell_status);

    Result<bool> recordTrade(const Trade& trade);
    Result<void> updateOrder(long long order_id, long long new_qty_remaining, const std::string& new_status);
    Result<void> applyBuyPosition(int user_id, int bot_id, int market_id, long long qty, long long price);
    Result<void> applySellPosition(int user_id, int bot_id, int market_id, long long qty);
    Result<bool> isOrderOpen(long long order_id);

    Result<bool> hasAnyUser();
    Result<bool> registerUser(const std::string& username, const std::string& password, const std::string& pinHash);
    Result<std::string> getUsername(const int userId);
    Result<int>getUserId(const std::string& username);
    Result<int> getBotId(const std::string& botName);
    long long loginUser(const std::string& username, const std::string& password);
    Result<std::string> getPasswordHash(const std::string& username);
    
    Result<long long> getAvailableCash(int user_id);
    Result<long long> getLastPriceRaw(int market_id);
    Result<long long> getPositionQtyRaw(int user_id, int market_id);
    Result<long long> getBotPositionQtyRaw(int bot_id, int market_id);
    long long getPositionAvePriceRaw(int user_id, int market_id);
    std::pair<long long, long long> getPrevAndCurrentPricesRaw(int market_id);
    long long getRealizedPnLRaw(int user_id);
    std::vector<OrderHistoryRow> getUserOrderHistory(int user_id);
    
    std::vector<std::pair<long long, long long>> getTopBuyOrders(int market_id, int limit = 5);
    std::vector<std::pair<long long, long long>> getTopSellOrders(int market_id, int limit = 5);

    Result<long long> getTickSize(int market_id);
    Result<long long> getLotSize(int market_id);
    Result<long long> getReferencePrice(int market_id);
    
    void updateHighScore(const std::string& username, double cash, const std::string& rank);
    void showLeaderboard();
    
    // --- CRUD DELETE REQUIREMENT ---
    Result<bool> deleteUserAccount(int user_id);

    Result<void> setAvailableCash(int user_id, long long amount);
    Result<void> lockCash(int user_id, long long amount);
    Result<void> releaseCash(int user_id, long long amount);
    Result<void> addCashBalance(int user_id, long long amount);
    Result<void> subtractCashBalance(int user_id, long long amount);
    Result<void> lockPosition(int user_id, int bot_id, int market_id, long long qty);
    Result<void> releasePosition(int user_id,int bot_id,int market_id,long long qty);

    double getAvailablePosition(int user_id, int market_id);
};

#endif

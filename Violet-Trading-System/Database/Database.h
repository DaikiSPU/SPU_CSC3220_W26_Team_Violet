//
//  Database.h
//  Violet-Trading-System
//
//  Created by Nam Nguyen on 2/13/26.
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
    
    // Fetches the open markets
    std::vector<std::pair<int, std::string>> getAvailableMarkets();
    // Returns the ID of the saved order
    long long addOrder(Order& order);
    
    // Saves the trade execution
    void recordTrade(Trade& trade);
    
    // Authentication (Matches ERD-2 Users Table)
    bool registerUser(const std::string& username, const std::string& password);
    long long loginUser(const std::string& username, const std::string& password);
};

#endif

//
//  Database.cpp
//  Violet-Trading-System
//
//  Created by Nam Nguyen on 2/13/26.
//

#include "Database.h"

// Constructor: Opens the database file
Database::Database(const std::string& filename) {
    if (sqlite3_open(filename.c_str(), &db)) {
        std::cerr << "Error opening database: " << sqlite3_errmsg(db) << std::endl;
    } else {
        std::cout << "Connected to database: " << filename << std::endl;
    }
}

// Destructor: Closes the connection
Database::~Database() {
    sqlite3_close(db);
}

// Helper to execute SQL statements
void Database::executeQuery(const std::string& query) {
    char* errMsg = 0;
    if (sqlite3_exec(db, query.c_str(), 0, 0, &errMsg) != SQLITE_OK) {
        std::cerr << "SQL Error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
    }
}

void Database::initTables() {
    // 1. USERS [Matches ERD-3 Source 3]
    // PDF: created_at (TIMESTAMPTZ)
    executeQuery("CREATE TABLE IF NOT EXISTS Users ("
                 "user_id INTEGER PRIMARY KEY AUTOINCREMENT,"
                 "username VARCHAR(32) UNIQUE NOT NULL,"
                 "password_hash TEXT NOT NULL,"
                 "pin_hash TEXT,"
                 "created_at TIMESTAMPTZ NOT NULL,"
                 "updated_at TIMESTAMPTZ);");

    // 2. ACCOUNTS [Matches ERD-3 Source 4]
    // PDF: cash_balance (NUMERIC(12,2))
    executeQuery("CREATE TABLE IF NOT EXISTS Accounts ("
                 "account_id INTEGER PRIMARY KEY AUTOINCREMENT,"
                 "user_id INTEGER UNIQUE NOT NULL,"
                 "cash_balance NUMERIC(12,2) NOT NULL,"
                 "cash_available NUMERIC(12,2) NOT NULL,"
                 "updated_at TIMESTAMPTZ NOT NULL,"
                 "FOREIGN KEY(user_id) REFERENCES Users(user_id));");

    // 3. MARKETS [Matches ERD-3 Source 4]
    // PDF: tick_size (NUMERIC(8,4))
    executeQuery("CREATE TABLE IF NOT EXISTS Markets ("
                 "market_id INTEGER PRIMARY KEY AUTOINCREMENT,"
                 "symbol VARCHAR(16) UNIQUE NOT NULL,"
                 "name VARCHAR(32),"
                 "tick_size NUMERIC(8,4) NOT NULL,"
                 "lot_size NUMERIC(8,4) NOT NULL,"
                 "status VARCHAR(16) NOT NULL CHECK(status IN ('open', 'closed')));");

    // 4. BOTS [Matches ERD-3 Source 4]
    executeQuery("CREATE TABLE IF NOT EXISTS Bots ("
                 "bot_id INTEGER PRIMARY KEY AUTOINCREMENT,"
                 "bot_name VARCHAR(32) UNIQUE NOT NULL,"
                 "bot_type VARCHAR(32) NOT NULL);");

    // 5. ORDERS [Matches ERD-3 Source 5]
    // PDF: created_at (TIMESTAMPTZ), price (NUMERIC(12,4))
    executeQuery("CREATE TABLE IF NOT EXISTS Orders ("
                 "order_id INTEGER PRIMARY KEY AUTOINCREMENT,"
                 "user_id INTEGER,"
                 "bot_id INTEGER,"
                 "market_id INTEGER NOT NULL,"
                 "side VARCHAR(4) NOT NULL CHECK(side IN ('buy','sell')),"
                 "type VARCHAR(16) NOT NULL CHECK(type IN ('limit')),"
                 "price NUMERIC(12,4) NOT NULL,"
                 "qty NUMERIC(12,4) NOT NULL,"
                 "qty_remaining NUMERIC(12,4) NOT NULL,"
                 "status VARCHAR(16) NOT NULL CHECK(status IN ('open', 'partial', 'filled','canceled')),"
                 "created_at TIMESTAMPTZ NOT NULL,"
                 "updated_at TIMESTAMPTZ NOT NULL,"
                 "FOREIGN KEY(market_id) REFERENCES Markets(market_id),"
                 "CHECK ((user_id IS NOT NULL AND bot_id IS NULL) OR (user_id IS NULL AND bot_id IS NOT NULL)));");

    // 6. TRADES [Matches ERD-3 Source 6]
    // PDF: executed_at (TIMESTAMPTZ)
    executeQuery("CREATE TABLE IF NOT EXISTS Trades ("
                 "trade_id INTEGER PRIMARY KEY AUTOINCREMENT,"
                 "market_id INTEGER NOT NULL,"
                 "buy_order_id INTEGER NOT NULL,"
                 "sell_order_id INTEGER NOT NULL,"
                 "price NUMERIC(12,4) NOT NULL,"
                 "qty NUMERIC(12,4) NOT NULL,"
                 "executed_at TIMESTAMPTZ NOT NULL,"
                 "FOREIGN KEY(buy_order_id) REFERENCES Orders(order_id),"
                 "FOREIGN KEY(sell_order_id) REFERENCES Orders(order_id));");

    // 7. TRANSACTIONS [Matches ERD-3 Source 7]
    executeQuery("CREATE TABLE IF NOT EXISTS Transactions ("
                 "txn_id INTEGER PRIMARY KEY AUTOINCREMENT,"
                 "user_id INTEGER NOT NULL,"
                 "trade_id INTEGER NOT NULL,"
                 "market_id INTEGER NOT NULL,"
                 "kind VARCHAR(16) NOT NULL CHECK(kind IN ('buy','sell', 'fee')),"
                 "amount_cash NUMERIC(12,2) NOT NULL,"
                 "amount_qty NUMERIC(12,4) NOT NULL,"
                 "created_at TIMESTAMPTZ NOT NULL,"
                 "FOREIGN KEY(user_id) REFERENCES Users(user_id),"
                 "FOREIGN KEY(trade_id) REFERENCES Trades(trade_id),"
                 "FOREIGN KEY(market_id) REFERENCES Markets(market_id));");

    std::cout << "All tables initialized successfully matching ERD-3.pdf types." << std::endl;
}

long long Database::addOrder(Order& o) {
    std::string sql = "INSERT INTO Orders (user_id, market_id, side, type, price, qty, qty_remaining, status) VALUES (" +
                      std::to_string(o.user_id) + ", " +
                      std::to_string(o.market_id) + ", '" +
                      o.side + "', '" +
                      o.type + "', " +
                      std::to_string(o.price) + ", " +
                      std::to_string(o.qty) + ", " +
                      std::to_string(o.qty_remaining) + ", 'open');";
    
    executeQuery(sql);
    
    // Return the auto-generated ID so the Engine knows it
    return sqlite3_last_insert_rowid(db);
}

void Database::recordTrade(Trade& t) {
    std::string sql = "INSERT INTO Trades (market_id, buy_order_id, sell_order_id, price, qty, executed_at) VALUES (" +
                      std::to_string(t.market_id) + ", " +
                      std::to_string(t.buy_order_id) + ", " +
                      std::to_string(t.sell_order_id) + ", " +
                      std::to_string(t.price) + ", " +
                      std::to_string(t.qty) + ", DATE('now'));";
    
    executeQuery(sql);
    std::cout << " [DB] Trade Recorded: " << t.qty << " shares @ $" << (t.price / 100.0) << std::endl;
}

bool Database::registerUser(const std::string& username, const std::string& password) {
    // 1. Insert into 'Users' table
    std::string sql = "INSERT INTO Users (username, password_hash, created_at) VALUES ('" +
                      username + "', '" + password + "', DATE('now'));";
    
    char* errMsg = 0;
    if (sqlite3_exec(db, sql.c_str(), 0, 0, &errMsg) != SQLITE_OK) {
        std::cerr << "Registration Failed: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    
    // 2. Create the linked 'Account' (Required by ERD-2)
    long long user_id = sqlite3_last_insert_rowid(db);
    std::string sql_acc = "INSERT INTO Accounts (user_id, cash_balance, cash_available, updated_at) VALUES (" +
                          std::to_string(user_id) + ", 0, 0, DATE('now'));";
    executeQuery(sql_acc);
    
    std::cout << "User '" << username << "' registered (ID: " << user_id << ")." << std::endl;
    return true;
}

long long Database::loginUser(const std::string& username, const std::string& password) {
    // 1. Check 'Users' table for matching name AND password
    std::string sql = "SELECT user_id FROM Users WHERE username='" + username +
                      "' AND password_hash='" + password + "';";
    
    sqlite3_stmt* stmt;
    long long user_id = -1; // -1 means "Login Failed"

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            user_id = sqlite3_column_int64(stmt, 0);
            std::cout << "Login Successful! Welcome, " << username << "." << std::endl;
        } else {
            std::cout << "Login Failed: Invalid username or password." << std::endl;
        }
    }
    sqlite3_finalize(stmt);
    return user_id;
}

std::vector<std::pair<int, std::string>> Database::getAvailableMarkets() {
    std::vector<std::pair<int, std::string>> markets;
    sqlite3_stmt* stmt;
    
    // Only fetch markets that are currently open
    const char* sql = "SELECT market_id, symbol FROM Markets WHERE status = 'open';";
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int id = sqlite3_column_int(stmt, 0);
            
            // Extract the text safely
            const unsigned char* text = sqlite3_column_text(stmt, 1);
            std::string symbol = text ? reinterpret_cast<const char*>(text) : "UNKNOWN";
            
            markets.push_back({id, symbol});
        }
    } else {
        std::cerr << "Failed to fetch markets from database.\n";
    }
    
    sqlite3_finalize(stmt);
    return markets;
}

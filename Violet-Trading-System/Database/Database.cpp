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
        
        executeQuery("PRAGMA foreign_keys = ON;");
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
    // --- FRONTEND TABLES ---
    
    // 0a. USERS
    executeQuery("CREATE TABLE IF NOT EXISTS Users ("
                 "user_id INTEGER PRIMARY KEY AUTOINCREMENT,"
                 "username VARCHAR(32) UNIQUE NOT NULL,"
                 "password_hash TEXT NOT NULL,"
                 "pin_hash TEXT,"
                 "created_at TIMESTAMPTZ DEFAULT CURRENT_TIMESTAMP,"
                 "updated_at TIMESTAMPTZ DEFAULT CURRENT_TIMESTAMP);");

    // 0b. ACCOUNTS
    executeQuery("CREATE TABLE IF NOT EXISTS Accounts ("
                 "account_id INTEGER PRIMARY KEY AUTOINCREMENT,"
                 "user_id INTEGER NOT NULL,"
                 "cash_balance NUMERIC(12,2) DEFAULT 0,"
                 "cash_available NUMERIC(12,2) DEFAULT 0,"
                 "updated_at TIMESTAMPTZ DEFAULT CURRENT_TIMESTAMP,"
                 "FOREIGN KEY(user_id) REFERENCES Users(user_id));");

    // 0c. POSITIONS
    executeQuery("CREATE TABLE IF NOT EXISTS Positions ("
                 "position_id INTEGER PRIMARY KEY AUTOINCREMENT,"
                 "user_id INTEGER NOT NULL,"
                 "market_id INTEGER NOT NULL,"
                 "qty NUMERIC(12,4) DEFAULT 0,"
                 "avg_price NUMERIC(12,4) DEFAULT 0,"
                 "FOREIGN KEY(user_id) REFERENCES Users(user_id),"
                 "FOREIGN KEY(market_id) REFERENCES Markets(market_id));");

    // 1. MARKETS 
    executeQuery("CREATE TABLE IF NOT EXISTS Markets ("
                 "market_id INTEGER PRIMARY KEY AUTOINCREMENT," // Maps to SERIAL
                 "symbol VARCHAR(16) UNIQUE NOT NULL,"
                 "name VARCHAR(32),"
                 "tick_size NUMERIC(8,4) NOT NULL CHECK(tick_size > 0),"
                 "lot_size NUMERIC(8,4) NOT NULL CHECK(lot_size > 0),"
                 "status VARCHAR(16) NOT NULL CHECK(status IN ('open', 'closed')));");

    // 2. BOTS 
    executeQuery("CREATE TABLE IF NOT EXISTS Bots ("
                 "bot_id INTEGER PRIMARY KEY AUTOINCREMENT," // Maps to SERIAL
                 "bot_name VARCHAR(32) UNIQUE NOT NULL,"
                 "bot_type VARCHAR(32) NOT NULL);");

    // 3. ORDERS 
    executeQuery("CREATE TABLE IF NOT EXISTS Orders ("
                 "order_id INTEGER PRIMARY KEY AUTOINCREMENT," // Maps to SERIAL
                 "user_id INTEGER," // Maps to FK INTEGER
                 "bot_id INTEGER,"  // Maps to FK INTEGER
                 "market_id INTEGER NOT NULL,"
                 "side VARCHAR(4) NOT NULL CHECK(side IN ('buy','sell')),"
                 "type VARCHAR(16) NOT NULL CHECK(type IN ('limit')),"
                 "price NUMERIC(12,4) NOT NULL CHECK(price > 0),"
                 "qty NUMERIC(12,4) NOT NULL CHECK(qty > 0),"                 // DAIKI FIX: Must be > 0
                 "qty_remaining NUMERIC(12,4) NOT NULL CHECK(qty_remaining >= 0)," // DAIKI FIX: Can be 0
                 "status VARCHAR(16) NOT NULL CHECK(status IN ('open', 'partial', 'filled','canceled')),"
                 "created_at TIMESTAMPTZ NOT NULL,"
                 "updated_at TIMESTAMPTZ NOT NULL,"
                 "FOREIGN KEY(market_id) REFERENCES Markets(market_id),"
                 "CHECK ((user_id IS NOT NULL AND bot_id IS NULL) OR (user_id IS NULL AND bot_id IS NOT NULL)));");

    // 4. TRADES 
    executeQuery("CREATE TABLE IF NOT EXISTS Trades ("
                 "trade_id INTEGER PRIMARY KEY AUTOINCREMENT,"
                 "market_id INTEGER NOT NULL,"
                 "buy_order_id INTEGER NOT NULL,"
                 "sell_order_id INTEGER NOT NULL,"
                 "price NUMERIC(12,4) NOT NULL CHECK(price > 0),"
                 "qty NUMERIC(12,4) NOT NULL CHECK(qty > 0),"
                 "executed_at TIMESTAMPTZ NOT NULL,"
                 "FOREIGN KEY(buy_order_id) REFERENCES Orders(order_id),"
                 "FOREIGN KEY(sell_order_id) REFERENCES Orders(order_id));");

    // 5. TRANSACTIONS 
    executeQuery("CREATE TABLE IF NOT EXISTS Transactions ("
                 "txn_id INTEGER PRIMARY KEY AUTOINCREMENT,"
                 "user_id INTEGER NOT NULL,"
                 "trade_id INTEGER NOT NULL,"
                 "market_id INTEGER NOT NULL,"
                 "kind VARCHAR(16) NOT NULL CHECK(kind IN ('buy','sell', 'fee')),"
                 "amount_cash NUMERIC(12,2) NOT NULL,"
                 "amount_qty NUMERIC(12,4) NOT NULL,"
                 "created_at TIMESTAMPTZ NOT NULL,"
                 "FOREIGN KEY(trade_id) REFERENCES Trades(trade_id),"
                 "FOREIGN KEY(market_id) REFERENCES Markets(market_id));");

    std::cout << "[SYSTEM] Database initialized with strict constraints." << std::endl;
    
    // --- SEED DEFAULT MARKETS ---
    // INSERT OR IGNORE ensures these are only added the very first time the database is created
    executeQuery("INSERT OR IGNORE INTO Markets (market_id, symbol, name, tick_size, lot_size, status) VALUES "
                 "(1, 'SPU', 'Seattle Pacific', 100, 10000, 'open'),"
                 "(2, 'BTC', 'Bitcoin', 100, 10000, 'open');");
}

long long Database::addOrder(Order& o) {
    
    std::string sql = "INSERT INTO Orders (user_id, market_id, side, type, price, qty, qty_remaining, status, created_at, updated_at) VALUES (" +
                      std::to_string(o.user_id) + ", " +
                      std::to_string(o.market_id) + ", '" +
                      o.side + "', '" +
                      o.type + "', " +
                      std::to_string(o.price) + ", " +
                      std::to_string(o.qty) + ", " +
                      std::to_string(o.qty_remaining) + ", 'open', CURRENT_TIMESTAMP, CURRENT_TIMESTAMP);";
    
    executeQuery(sql);
    
    // Return the auto-generated ID so the Engine knows it
    return sqlite3_last_insert_rowid(db);
}

void Database::recordTrade(Trade& t) {
    // Changed DATE('now') to CURRENT_TIMESTAMP for exact seconds
    std::string sql = "INSERT INTO Trades (market_id, buy_order_id, sell_order_id, price, qty, executed_at) VALUES (" +
                      std::to_string(t.market_id) + ", " +
                      std::to_string(t.buy_order_id) + ", " +
                      std::to_string(t.sell_order_id) + ", " +
                      std::to_string(t.price) + ", " +
                      std::to_string(t.qty) + ", CURRENT_TIMESTAMP);";
    
    executeQuery(sql);
    
    
    std::cout << " [DB] Trade Recorded: " << (t.qty / 10000.0) << " shares @ $" << (t.price / 10000.0) << std::endl;
}

void Database::updateOrder(long long order_id, long long new_qty_remaining, const std::string& new_status) {
    // Updates the quantity, the status, and the updated_at timestamp
    std::string sql = "UPDATE Orders SET qty_remaining = " + std::to_string(new_qty_remaining) +
                      ", status = '" + new_status + "', updated_at = CURRENT_TIMESTAMP " +
                      "WHERE order_id = " + std::to_string(order_id) + ";";
    
    executeQuery(sql);
}

bool Database::registerUser(const std::string& username, const std::string& password) {
    // 1. Insert into 'Users' table
    std::string sql = "INSERT INTO Users (username, password_hash, created_at) VALUES ('" +
                      username + "', '" + password + "', CURRENT_TIMESTAMP);";
    
    char* errMsg = 0;
    if (sqlite3_exec(db, sql.c_str(), 0, 0, &errMsg) != SQLITE_OK) {
        std::cerr << "Registration Failed: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    
    // 2. Create the linked 'Account' (Also using CURRENT_TIMESTAMP)
    long long user_id = sqlite3_last_insert_rowid(db);
    std::string sql_acc = "INSERT INTO Accounts (user_id, cash_balance, cash_available, updated_at) VALUES (" + std::to_string(user_id) + ", 0, 0, CURRENT_TIMESTAMP);";
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


// ---------------------------------------------------------
// FRONTEND API IMPLEMENTATIONS
// ---------------------------------------------------------

// 1. Fetch available markets
std::vector<std::pair<int, std::string>> Database::getAvailableMarkets() {
    std::vector<std::pair<int, std::string>> markets;
    sqlite3_stmt* stmt;
    const char* sql = "SELECT market_id, symbol FROM Markets WHERE status = 'open';";
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int id = sqlite3_column_int(stmt, 0);
            const unsigned char* text = sqlite3_column_text(stmt, 1);
            std::string symbol = text ? reinterpret_cast<const char*>(text) : "UNKNOWN";
            markets.push_back({id, symbol});
        }
    }
    sqlite3_finalize(stmt);
    return markets;
}

// 2. Fetch available cash for the Top Bar
double Database::getAvailableCash(long long user_id) {
    double cash = 0.0;
    sqlite3_stmt* stmt;
    const char* sql = "SELECT cash_available FROM Accounts WHERE user_id = ?;";
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int64(stmt, 1, user_id);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            // Divide by 10000.0 to convert backend fixed-point integers to frontend decimals
            cash = sqlite3_column_double(stmt, 0) / 10000.0; 
        }
    }
    sqlite3_finalize(stmt);
    return cash;
}

// 3. Fetch Order History for the Bottom Table
std::vector<OrderHistoryRow> Database::getUserOrderHistory(long long user_id) {
    std::vector<OrderHistoryRow> history;
    sqlite3_stmt* stmt;
    
    // JOIN Orders and Markets to get the Symbol string (e.g. "SPU")
    const char* sql = R"(
        SELECT o.created_at, m.symbol, o.side, o.price, o.qty, o.status 
        FROM Orders o
        JOIN Markets m ON o.market_id = m.market_id
        WHERE o.user_id = ?
        ORDER BY o.created_at DESC;
    )";
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int64(stmt, 1, user_id);
        
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            OrderHistoryRow row;
            
            const unsigned char* t_text = sqlite3_column_text(stmt, 0);
            row.time = t_text ? reinterpret_cast<const char*>(t_text) : "";
            
            const unsigned char* m_text = sqlite3_column_text(stmt, 1);
            row.market_symbol = m_text ? reinterpret_cast<const char*>(m_text) : "";
            
            const unsigned char* s_text = sqlite3_column_text(stmt, 2);
            row.side = s_text ? reinterpret_cast<const char*>(s_text) : "";
            
            // Convert from fixed-point integers to decimals
            row.price = sqlite3_column_double(stmt, 3) / 10000.0;
            row.quantity = sqlite3_column_double(stmt, 4) / 10000.0;
            row.total = row.price * row.quantity;
            
            const unsigned char* st_text = sqlite3_column_text(stmt, 5);
            row.status = st_text ? reinterpret_cast<const char*>(st_text) : "";
            
            history.push_back(row);
        }
    }
    sqlite3_finalize(stmt);
    return history;
}

// 4. Fetch the Order Book (Bids / Buyers)
std::vector<std::pair<double, int>> Database::getTopBuyOrders(int market_id, int limit) {
    std::vector<std::pair<double, int>> book;
    sqlite3_stmt* stmt;
    
    // SQL Logic: Group by price, sum the remaining quantity, highest prices first.
    const char* sql = R"(
        SELECT price, SUM(qty_remaining) FROM Orders 
        WHERE market_id = ? AND side = 'buy' AND status IN ('open', 'partial')
        GROUP BY price 
        ORDER BY price DESC 
        LIMIT ?;
    )";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, market_id);
        sqlite3_bind_int(stmt, 2, limit);

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            // Convert fixed-point engine integers back to UI decimals
            double price = sqlite3_column_double(stmt, 0) / 10000.0;
            int qty = static_cast<int>(sqlite3_column_double(stmt, 1) / 10000.0);
            
            book.push_back({price, qty});
        }
    } else {
        std::cerr << "[SYSTEM] Failed to fetch Top Buy Orders.\n";
    }
    sqlite3_finalize(stmt);
    return book;
}

// 5. Fetch the Order Book (Asks / Sellers)
std::vector<std::pair<double, int>> Database::getTopSellOrders(int market_id, int limit) {
    std::vector<std::pair<double, int>> book;
    sqlite3_stmt* stmt;
    
    // SQL Logic: Group by price, sum the remaining quantity, lowest prices first.
    const char* sql = R"(
        SELECT price, SUM(qty_remaining) FROM Orders 
        WHERE market_id = ? AND side = 'sell' AND status IN ('open', 'partial')
        GROUP BY price 
        ORDER BY price ASC 
        LIMIT ?;
    )";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, market_id);
        sqlite3_bind_int(stmt, 2, limit);

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            // Convert fixed-point engine integers back to UI decimals
            double price = sqlite3_column_double(stmt, 0) / 10000.0;
            int qty = static_cast<int>(sqlite3_column_double(stmt, 1) / 10000.0);
            
            book.push_back({price, qty});
        }
    } else {
        std::cerr << "[SYSTEM] Failed to fetch Top Sell Orders.\n";
    }
    sqlite3_finalize(stmt);
    return book;
}

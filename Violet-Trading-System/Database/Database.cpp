//
//  Database.cpp
//  Violet-Trading-System
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
    executeQuery("CREATE TABLE IF NOT EXISTS Users ("
                 "user_id INTEGER PRIMARY KEY AUTOINCREMENT,"
                 "username VARCHAR(32) UNIQUE NOT NULL,"
                 "password_hash TEXT NOT NULL,"
                 "pin_hash TEXT,"
                 "created_at TIMESTAMPTZ DEFAULT CURRENT_TIMESTAMP,"
                 "updated_at TIMESTAMPTZ DEFAULT CURRENT_TIMESTAMP);");

    executeQuery("CREATE TABLE IF NOT EXISTS Accounts ("
                 "account_id INTEGER PRIMARY KEY AUTOINCREMENT,"
                 "user_id INTEGER NOT NULL,"
                 "cash_balance NUMERIC(12,2) DEFAULT 0,"
                 "cash_available NUMERIC(12,2) DEFAULT 0,"
                 "updated_at TIMESTAMPTZ DEFAULT CURRENT_TIMESTAMP,"
                 "FOREIGN KEY(user_id) REFERENCES Users(user_id));");

    executeQuery("CREATE TABLE IF NOT EXISTS Positions ("
                 "position_id INTEGER PRIMARY KEY AUTOINCREMENT,"
                 "user_id INTEGER NOT NULL,"
                 "market_id INTEGER NOT NULL,"
                 "qty NUMERIC(12,4) DEFAULT 0,"
                 "avg_price NUMERIC(12,4) DEFAULT 0,"
                 "FOREIGN KEY(user_id) REFERENCES Users(user_id),"
                 "FOREIGN KEY(market_id) REFERENCES Markets(market_id),"
                 "UNIQUE(user_id, market_id));"); // <-- NEW UNIQUE CONSTRAINT

    // 1. MARKETS 
    executeQuery("CREATE TABLE IF NOT EXISTS Markets ("
                 "market_id INTEGER PRIMARY KEY AUTOINCREMENT,"
                 "symbol VARCHAR(16) UNIQUE NOT NULL,"
                 "name VARCHAR(32),"
                 "tick_size NUMERIC(8,4) NOT NULL CHECK(tick_size > 0),"
                 "lot_size NUMERIC(8,4) NOT NULL CHECK(lot_size > 0),"
                 "status VARCHAR(16) NOT NULL CHECK(status IN ('open', 'closed')));");

    // 2. BOTS 
    executeQuery("CREATE TABLE IF NOT EXISTS Bots ("
                 "bot_id INTEGER PRIMARY KEY AUTOINCREMENT,"
                 "bot_name VARCHAR(32) UNIQUE NOT NULL,"
                 "bot_type VARCHAR(32) NOT NULL);");

    // 3. ORDERS 
    executeQuery("CREATE TABLE IF NOT EXISTS Orders ("
                 "order_id INTEGER PRIMARY KEY AUTOINCREMENT,"
                 "user_id INTEGER,"
                 "bot_id INTEGER,"
                 "market_id INTEGER NOT NULL,"
                 "side VARCHAR(4) NOT NULL CHECK(side IN ('buy','sell')),"
                 "type VARCHAR(16) NOT NULL CHECK(type IN ('limit')),"
                 "price NUMERIC(12,4) NOT NULL CHECK(price > 0),"
                 "qty NUMERIC(12,4) NOT NULL CHECK(qty > 0),"                 
                 "qty_remaining NUMERIC(12,4) NOT NULL CHECK(qty_remaining >= 0)," 
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
                 
    // --- 6. LEADERBOARD ---
    executeQuery("CREATE TABLE IF NOT EXISTS Leaderboard ("
                 "username TEXT PRIMARY KEY,"
                 "peak_cash NUMERIC(12,2) DEFAULT 0,"
                 "rank_title TEXT,"
                 "updated_at TIMESTAMPTZ DEFAULT CURRENT_TIMESTAMP);");

    std::cout << "[SYSTEM] Database initialized with strict constraints." << std::endl;
    
    // --- SEED 5 MARKETS ---
    executeQuery("INSERT OR IGNORE INTO Markets (market_id, symbol, name, tick_size, lot_size, status) VALUES "
                 "(1, 'SPU', 'Seattle Pacific', 100, 10000, 'open'),"
                 "(2, 'BTC', 'Bitcoin', 100, 10000, 'open'),"
                 "(3, 'AAPL', 'Apple Inc.', 100, 10000, 'open'),"
                 "(4, 'TSLA', 'Tesla Motors', 100, 10000, 'open'),"
                 "(5, 'NVDA', 'Nvidia Corp.', 100, 10000, 'open');");

    // --- SEED ALL 7 BOT PROFILES ---
    executeQuery("INSERT OR IGNORE INTO Users (user_id, username, password_hash, created_at) VALUES "
                 "(998, 'System_Noise', 'SYS', CURRENT_TIMESTAMP),"
                 "(999, 'System_MarketMaker', 'SYS', CURRENT_TIMESTAMP),"
                 "(901, 'Bot_Panic', 'SYS', CURRENT_TIMESTAMP),"
                 "(902, 'Bot_Whale', 'SYS', CURRENT_TIMESTAMP),"
                 "(903, 'Bot_FatFinger', 'SYS', CURRENT_TIMESTAMP),"
                 "(904, 'Bot_SineWave', 'SYS', CURRENT_TIMESTAMP),"
                 "(905, 'Bot_Greed', 'SYS', CURRENT_TIMESTAMP);");
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
    return sqlite3_last_insert_rowid(db);
}

void Database::recordTrade(Trade& t) {
    std::string sql = "INSERT INTO Trades (market_id, buy_order_id, sell_order_id, price, qty, executed_at) VALUES (" +
                      std::to_string(t.market_id) + ", " +
                      std::to_string(t.buy_order_id) + ", " +
                      std::to_string(t.sell_order_id) + ", " +
                      std::to_string(t.price) + ", " +
                      std::to_string(t.qty) + ", CURRENT_TIMESTAMP);";
    
    executeQuery(sql);
    
    long long total_cost = (t.price * t.qty) / 10000;

    std::string update_buyer = "UPDATE Accounts SET cash_available = cash_available - " + std::to_string(total_cost) +
                               ", cash_balance = cash_balance - " + std::to_string(total_cost) +
                               " WHERE user_id = (SELECT user_id FROM Orders WHERE order_id = " + std::to_string(t.buy_order_id) + ");";
    executeQuery(update_buyer);

    std::string update_seller = "UPDATE Accounts SET cash_available = cash_available + " + std::to_string(total_cost) +
                                ", cash_balance = cash_balance + " + std::to_string(total_cost) +
                                " WHERE user_id = (SELECT user_id FROM Orders WHERE order_id = " + std::to_string(t.sell_order_id) + ");";
    executeQuery(update_seller);

    //std::cout << " [DB] Trade Recorded: " << (t.qty / 10000.0) << " shares @ $" << (t.price / 10000.0) << std::endl;
    
    // --- NEW: UPDATE STOCK INVENTORY (POSITIONS) ---
    // Give shares to the buyer
    std::string update_pos_buy = 
        "INSERT INTO Positions (user_id, market_id, qty) "
        "SELECT user_id, " + std::to_string(t.market_id) + ", " + std::to_string(t.qty) + " "
        "FROM Orders WHERE order_id = " + std::to_string(t.buy_order_id) + " "
        "ON CONFLICT(user_id, market_id) DO UPDATE SET qty = qty + " + std::to_string(t.qty) + ";";
    executeQuery(update_pos_buy);

    // Remove shares from the seller
    std::string update_pos_sell = 
        "INSERT INTO Positions (user_id, market_id, qty) "
        "SELECT user_id, " + std::to_string(t.market_id) + ", -" + std::to_string(t.qty) + " "
        "FROM Orders WHERE order_id = " + std::to_string(t.sell_order_id) + " "
        "ON CONFLICT(user_id, market_id) DO UPDATE SET qty = qty - " + std::to_string(t.qty) + ";";
    executeQuery(update_pos_sell);
}

void Database::updateOrder(long long order_id, long long new_qty_remaining, const std::string& new_status) {
    std::string sql = "UPDATE Orders SET qty_remaining = " + std::to_string(new_qty_remaining) +
                      ", status = '" + new_status + "', updated_at = CURRENT_TIMESTAMP " +
                      "WHERE order_id = " + std::to_string(order_id) + ";";
    executeQuery(sql);
}

bool Database::registerUser(const std::string& username, const std::string& password) {
    std::string sql = "INSERT INTO Users (username, password_hash, created_at) VALUES ('" +
                      username + "', '" + password + "', CURRENT_TIMESTAMP);";
    
    char* errMsg = 0;
    if (sqlite3_exec(db, sql.c_str(), 0, 0, &errMsg) != SQLITE_OK) {
        std::cerr << "Registration Failed: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    
    long long user_id = sqlite3_last_insert_rowid(db);
    std::string sql_acc = "INSERT INTO Accounts (user_id, cash_balance, cash_available, updated_at) VALUES (" + 
                          std::to_string(user_id) + ", 100000000, 100000000, CURRENT_TIMESTAMP);";
    executeQuery(sql_acc);
    
    std::cout << "User '" << username << "' registered (ID: " << user_id << ")." << std::endl;
    return true;
}

long long Database::loginUser(const std::string& username, const std::string& password) {
    std::string sql = "SELECT user_id FROM Users WHERE username='" + username +
                      "' AND password_hash='" + password + "';";
    
    sqlite3_stmt* stmt;
    long long user_id = -1; 

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

double Database::getAvailableCash(long long user_id) {
    double cash = 0.0;
    sqlite3_stmt* stmt;
    const char* sql = "SELECT cash_available FROM Accounts WHERE user_id = ?;";
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int64(stmt, 1, user_id);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            cash = sqlite3_column_double(stmt, 0) / 10000.0; 
        }
    }
    sqlite3_finalize(stmt);
    return cash;
}

std::vector<OrderHistoryRow> Database::getUserOrderHistory(long long user_id) {
    std::vector<OrderHistoryRow> history;
    sqlite3_stmt* stmt;
    
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

std::vector<std::pair<double, int>> Database::getTopBuyOrders(int market_id, int limit) {
    std::vector<std::pair<double, int>> book;
    sqlite3_stmt* stmt;
    
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
            double price = sqlite3_column_double(stmt, 0) / 10000.0;
            int qty = static_cast<int>(sqlite3_column_double(stmt, 1) / 10000.0);
            book.push_back({price, qty});
        }
    }
    sqlite3_finalize(stmt);
    return book;
}

std::vector<std::pair<double, int>> Database::getTopSellOrders(int market_id, int limit) {
    std::vector<std::pair<double, int>> book;
    sqlite3_stmt* stmt;
    
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
            double price = sqlite3_column_double(stmt, 0) / 10000.0;
            int qty = static_cast<int>(sqlite3_column_double(stmt, 1) / 10000.0);
            book.push_back({price, qty});
        }
    }
    sqlite3_finalize(stmt);
    return book;
}

// --- NEW: LEADERBOARD LOGIC ---
void Database::updateHighScore(const std::string& username, double cash, const std::string& rank) {
    long long cash_fixed = static_cast<long long>(cash * 10000);
    std::string sql = "INSERT INTO Leaderboard (username, peak_cash, rank_title) "
                      "VALUES ('" + username + "', " + std::to_string(cash_fixed) + ", '" + rank + "') "
                      "ON CONFLICT(username) DO UPDATE SET "
                      "rank_title = excluded.rank_title, "
                      "updated_at = CURRENT_TIMESTAMP, "
                      "peak_cash = MAX(Leaderboard.peak_cash, excluded.peak_cash);";
    executeQuery(sql);
}

void Database::showLeaderboard() {
    sqlite3_stmt* stmt;
    const char* sql = "SELECT username, peak_cash, rank_title FROM Leaderboard ORDER BY peak_cash DESC LIMIT 5;";
    
    std::cout << "\n=== HALL OF FAME: TOP 5 DUELISTS ===\n";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        int pos = 1;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            std::string name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            double cash = sqlite3_column_double(stmt, 1) / 10000.0;
            std::string rank = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            std::cout << pos++ << ". " << name << " - $" << cash << " [" << rank << "]\n";
        }
    }
    sqlite3_finalize(stmt);
    std::cout << "====================================\n";
}

// --- NEW: CRUD DELETE REQUIREMENT ---
void Database::deleteUserAccount(long long user_id, const std::string& username) {
    std::string uid_str = std::to_string(user_id);
    
    // 1. Delete dependent records to satisfy Foreign Key constraints
    executeQuery("DELETE FROM Positions WHERE user_id = " + uid_str + ";");
    executeQuery("DELETE FROM Accounts WHERE user_id = " + uid_str + ";");
    executeQuery("DELETE FROM Leaderboard WHERE username = '" + username + "';");
    
    // 2. Finally delete the user, freeing up the username for re-registration
    executeQuery("DELETE FROM Users WHERE user_id = " + uid_str + ";");
    
    std::cout << "\n[SYSTEM] Account for '" << username << "' has been permanently wiped.\n";
    std::cout << "[SYSTEM] The username is now available for re-registration.\n";
}

// --- NEW: INVENTORY MANAGEMENT ---
double Database::getTrueAvailableCash(long long user_id) {
    double balance = 0.0;
    double locked = 0.0;
    
    sqlite3_stmt* stmt1;
    if (sqlite3_prepare_v2(db, "SELECT cash_balance FROM Accounts WHERE user_id = ?;", -1, &stmt1, 0) == SQLITE_OK) {
        sqlite3_bind_int64(stmt1, 1, user_id);
        if (sqlite3_step(stmt1) == SQLITE_ROW) balance = sqlite3_column_double(stmt1, 0) / 10000.0;
    }
    sqlite3_finalize(stmt1);
    
    // Calculate cash tied up in active Buy orders
    sqlite3_stmt* stmt2;
    if (sqlite3_prepare_v2(db, "SELECT SUM(price * qty_remaining) FROM Orders WHERE user_id = ? AND side = 'buy' AND status IN ('open', 'partial');", -1, &stmt2, 0) == SQLITE_OK) {
        sqlite3_bind_int64(stmt2, 1, user_id);
        if (sqlite3_step(stmt2) == SQLITE_ROW) locked = sqlite3_column_double(stmt2, 0) / 100000000.0; 
    }
    sqlite3_finalize(stmt2);
    
    return balance - locked;
}

double Database::getAvailablePosition(long long user_id, int market_id) {
    double owned = 0.0;
    double locked = 0.0;
    
    sqlite3_stmt* stmt1;
    if (sqlite3_prepare_v2(db, "SELECT qty FROM Positions WHERE user_id = ? AND market_id = ?;", -1, &stmt1, 0) == SQLITE_OK) {
        sqlite3_bind_int64(stmt1, 1, user_id);
        sqlite3_bind_int(stmt1, 2, market_id);
        if (sqlite3_step(stmt1) == SQLITE_ROW) owned = sqlite3_column_double(stmt1, 0) / 10000.0;
    }
    sqlite3_finalize(stmt1);
    
    // Calculate shares tied up in active Sell orders
    sqlite3_stmt* stmt2;
    if (sqlite3_prepare_v2(db, "SELECT SUM(qty_remaining) FROM Orders WHERE user_id = ? AND market_id = ? AND side = 'sell' AND status IN ('open', 'partial');", -1, &stmt2, 0) == SQLITE_OK) {
        sqlite3_bind_int64(stmt2, 1, user_id);
        sqlite3_bind_int(stmt2, 2, market_id);
        if (sqlite3_step(stmt2) == SQLITE_ROW) locked = sqlite3_column_double(stmt2, 0) / 10000.0;
    }
    sqlite3_finalize(stmt2);
    
    return owned - locked;
}

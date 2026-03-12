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
        initTables();
    }
}

// Destructor: Closes the connection
Database::~Database() {
    sqlite3_close(db);
}

Result<void> Database::beginTransaction()
{
    auto result = executeQuery("BEGIN TRANSACTION;");

    if (!result.isSuccess())
    {
        std::cerr << "[DB] BEGIN TRANSACTION failed: "
                  << result.error.getMessage() << std::endl;
    }

    return result;
}

Result<void> Database::commit()
{
    auto result = executeQuery("COMMIT;");

    if (!result.isSuccess())
    {
        std::cerr << "[DB] COMMIT failed: "
                  << result.error.getMessage() << std::endl;
    }

    return result;
}

Result<void> Database::rollback()
{
    auto result = executeQuery("ROLLBACK;");

    if (!result.isSuccess())
    {
        std::cerr << "[DB] ROLLBACK failed: "
                  << result.error.getMessage() << std::endl;
    }

    return result;
}

// Helper to execute SQL statements
Result<void> Database::executeQuery(const std::string& query) {
    Result<void> result;

    char* errMsg = nullptr;

    int rc = sqlite3_exec(db, query.c_str(), nullptr, nullptr, &errMsg);

    if (rc != SQLITE_OK)
    {
        std::string error;

        if (errMsg)
        {
            error = errMsg;
            sqlite3_free(errMsg);
        }
        else
        {
            error = sqlite3_errmsg(db);
        }

        error += " | SQL: " + query;

        std::cerr << "SQL Error: " << error << std::endl;

        result.setError(ErrorType::Database, error);
    }
    return result;
}

Result<std::vector<int>> Database::executeQueryWithResult(const std::string& sql)
{
    Result<std::vector<int>> result;

    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK)
    {
        result.setError(ErrorType::Database, sqlite3_errmsg(db));
        return result;
    }

    while (true)
    {
        rc = sqlite3_step(stmt);

        if (rc == SQLITE_ROW)
        {
            int order_id = sqlite3_column_int(stmt, 0);
            result.value.push_back(order_id);
        }
        else if (rc == SQLITE_DONE)
        {
            break;
        }
        else
        {
            sqlite3_finalize(stmt);
            result.setError(ErrorType::Database, sqlite3_errmsg(db));
            return result;
        }
    }

    sqlite3_finalize(stmt);
    return result;
}

void Database::initTables() {
    executeQuery("PRAGMA foreign_keys = ON;");

    //--------------------------------------------------
    // USERS
    //--------------------------------------------------
    executeQuery(
        "CREATE TABLE IF NOT EXISTS users ("
        "user_id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "username TEXT NOT NULL UNIQUE,"
        "password_hash TEXT NOT NULL,"
        "pin_hash TEXT NOT NULL,"
        "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP"
        ");");

    //--------------------------------------------------
    // BOTS
    //--------------------------------------------------
    executeQuery(
        "CREATE TABLE IF NOT EXISTS bots ("
        "bot_id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "bot_name TEXT NOT NULL UNIQUE,"
        "bot_type TEXT NOT NULL"
        ");");

    //--------------------------------------------------
    // ACCOUNT
    //--------------------------------------------------
    executeQuery(
        "CREATE TABLE IF NOT EXISTS accounts ("
        "account_id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "user_id INTEGER NOT NULL UNIQUE,"
        "cash_balance INTEGER NOT NULL CHECK (cash_balance >= 0),"
        "cash_available INTEGER NOT NULL CHECK (cash_available >= 0),"
        "updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "FOREIGN KEY (user_id) REFERENCES users(user_id)"
        ");");

    //--------------------------------------------------
    // MARKETS
    //--------------------------------------------------
    executeQuery(
        "CREATE TABLE IF NOT EXISTS markets ("
        "market_id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "symbol TEXT NOT NULL UNIQUE,"
        "name TEXT NOT NULL,"
        "reference_price INTEGER NOT NULL CHECK (reference_price > 0),"
        "tick_size INTEGER NOT NULL CHECK (tick_size > 0),"
        "lot_size INTEGER NOT NULL CHECK (lot_size > 0),"
        "status TEXT NOT NULL CHECK (status IN ('open','closed'))"
        ");");

    //--------------------------------------------------
    // POSITIONS
    //--------------------------------------------------
    executeQuery(
        "CREATE TABLE IF NOT EXISTS positions ("
        "position_id INTEGER PRIMARY KEY AUTOINCREMENT,"

        "user_id INTEGER,"
        "bot_id INTEGER,"

        "market_id INTEGER NOT NULL,"

        "qty INTEGER NOT NULL CHECK (qty >= 0),"
        "qty_available INTEGER NOT NULL CHECK (qty_available >= 0),"
        "avg_price INTEGER NOT NULL CHECK (avg_price >= 0),"

        "CHECK ("
            "(user_id IS NOT NULL AND bot_id IS NULL) OR "
            "(user_id IS NULL AND bot_id IS NOT NULL)"
        "),"

        "UNIQUE (user_id, market_id),"
        "UNIQUE (bot_id, market_id),"

        "FOREIGN KEY (user_id) REFERENCES users(user_id),"
        "FOREIGN KEY (bot_id) REFERENCES bots(bot_id),"
        "FOREIGN KEY (market_id) REFERENCES markets(market_id)"
        ");");

    //--------------------------------------------------
    // ORDERS
    //--------------------------------------------------
    executeQuery(
        "CREATE TABLE IF NOT EXISTS orders ("
        "order_id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "user_id INTEGER,"
        "bot_id INTEGER,"
        "market_id INTEGER NOT NULL,"
        "side TEXT NOT NULL CHECK (side IN ('buy','sell')),"
        "type TEXT NOT NULL CHECK (type IN ('limit')),"
        "price INTEGER NOT NULL CHECK (price >= 0),"
        "qty INTEGER NOT NULL CHECK (qty > 0),"
        "qty_remaining INTEGER NOT NULL CHECK (qty_remaining >= 0) CHECK (qty_remaining <= qty),"
        "status TEXT NOT NULL CHECK (status IN ('open','partial','filled','canceled')),"
        "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "CHECK ((user_id IS NOT NULL AND bot_id IS NULL) OR (user_id IS NULL AND bot_id IS NOT NULL)),"
        "CHECK ("
        "(status = 'open' AND qty_remaining = qty)"
        " OR "
        "(status = 'partial' AND qty_remaining > 0 AND qty_remaining < qty)"
        " OR "
        "(status = 'filled' AND qty_remaining = 0)"
        " OR "
        "(status = 'canceled' AND qty_remaining >= 0 AND qty_remaining <= qty)"
        "),"
        "FOREIGN KEY (user_id) REFERENCES users(user_id),"
        "FOREIGN KEY (bot_id) REFERENCES bots(bot_id),"
        "FOREIGN KEY (market_id) REFERENCES markets(market_id)"
        ");");

    //--------------------------------------------------
    // TRADES
    //--------------------------------------------------
    executeQuery(
        "CREATE TABLE IF NOT EXISTS trades ("
        "trade_id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "market_id INTEGER NOT NULL,"
        "buy_order_id INTEGER NOT NULL,"
        "sell_order_id INTEGER NOT NULL,"
        "price INTEGER NOT NULL CHECK (price >= 0),"
        "qty INTEGER NOT NULL CHECK (qty > 0),"
        "executed_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "CHECK (buy_order_id <> sell_order_id),"
        "FOREIGN KEY (market_id) REFERENCES markets(market_id),"
        "FOREIGN KEY (buy_order_id) REFERENCES orders(order_id),"
        "FOREIGN KEY (sell_order_id) REFERENCES orders(order_id)"
        ");");

    //--------------------------------------------------
    // TRANSACTIONS
    //--------------------------------------------------
    executeQuery(
    "CREATE TABLE IF NOT EXISTS transactions ("
    "txn_id INTEGER PRIMARY KEY AUTOINCREMENT,"

    "user_id INTEGER,"
    "bot_id INTEGER,"

    "trade_id INTEGER NOT NULL,"
    "market_id INTEGER NOT NULL,"

    "kind TEXT NOT NULL CHECK (kind IN ('buy','sell','fee')),"
    "amount_cash INTEGER NOT NULL,"
    "amount_qty INTEGER NOT NULL,"

    "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"

    "CHECK ("
    "(user_id IS NOT NULL AND bot_id IS NULL) OR "
    "(user_id IS NULL AND bot_id IS NOT NULL)"
    "),"

    "FOREIGN KEY (user_id) REFERENCES users(user_id),"
    "FOREIGN KEY (bot_id) REFERENCES bots(bot_id),"
    "FOREIGN KEY (trade_id) REFERENCES trades(trade_id),"
    "FOREIGN KEY (market_id) REFERENCES markets(market_id)"
    ");");

                 
    // --- 6. LEADERBOARD ---
    executeQuery("CREATE TABLE IF NOT EXISTS leaderboard ("
                 "username TEXT PRIMARY KEY,"
                 "peak_cash INTEGER DEFAULT 0,"
                 "rank_title TEXT,"
                 "updated_at TEXT DEFAULT CURRENT_TIMESTAMP);");

    std::cout << "[SYSTEM] Database initialized with strict constraints." << std::endl;
    
    // --- SEED 5 MARKETS ---
    executeQuery("INSERT OR IGNORE INTO Markets (symbol, name, reference_price, tick_size, lot_size, status) VALUES "
                 "('SPU', 'Seattle Pacific', 250000, 100, 10000, 'open'),"
                 "('BTC', 'Bitcoin', 290000, 100, 10000, 'open'),"
                 "('AAPL', 'Apple Inc.', 270000, 100, 10000, 'open'),"
                 "('TSLA', 'Tesla Motors', 240000, 100, 10000, 'open'),"
                 "('NVDA', 'Nvidia Corp.', 270000, 100, 10000, 'open') "
                 "ON CONFLICT(symbol) DO NOTHING;");

    // --- SEED ALL 7 BOT PROFILES ---
    executeQuery("INSERT OR IGNORE INTO Bots (bot_name, bot_type) VALUES "
                "('Bot_Panic', 'panic'),"
                "('Bot_Whale', 'whale'),"
                "('Bot_FatFinger', 'fat_finger'),"
                "('Bot_SineWave', 'sine_wave'),"
                "('Bot_Greed', 'greed'),"
                "('System_Noise', 'system'),"
                "('System_MarketMaker', 'market_maker') "
                "ON CONFLICT(bot_name) DO NOTHING;");

    executeQuery("INSERT OR IGNORE INTO positions "
                "(user_id, bot_id, market_id, qty, qty_available, avg_price) "
                "SELECT "
                "NULL, "
                "bots.bot_id, "
                "markets.market_id, "
                "10000000, "
                "10000000, "
                "markets.reference_price "
                "FROM bots, markets "
                "WHERE bots.bot_name = 'System_MarketMaker';");

    executeQuery("CREATE INDEX IF NOT EXISTS idx_trades_market_time "
                "ON trades(market_id, executed_at DESC);");

    executeQuery("CREATE INDEX IF NOT EXISTS idx_orders_orderbook "
                "ON orders(market_id, side, price, created_at) "
                "WHERE status IN ('open','partial');");
}

Result<std::pair<long long,long long>> Database::addOrder(const Order& order)
{
    Result<std::pair<long long,long long>> result;

    const char* sql =
        "INSERT INTO Orders "
        "(user_id, bot_id, market_id, side, type, price, qty, qty_remaining, status, created_at, updated_at) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, 'open', CURRENT_TIMESTAMP, CURRENT_TIMESTAMP) "
        "RETURNING order_id, strftime('%s', created_at);";

    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        result.setError(ErrorType::Database, sqlite3_errmsg(db));
        return result;
    }

    // user_id
    if (order.user_id > 0)
        sqlite3_bind_int(stmt, 1, order.user_id);
    else
        sqlite3_bind_null(stmt, 1);

    // bot_id
    if (order.bot_id > 0)
        sqlite3_bind_int(stmt, 2, order.bot_id);
    else
        sqlite3_bind_null(stmt, 2);

    sqlite3_bind_int(stmt, 3, order.market_id);
    sqlite3_bind_text(stmt, 4, order.side.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, order.type.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 6, order.price);
    sqlite3_bind_int64(stmt, 7, order.qty);
    sqlite3_bind_int64(stmt, 8, order.qty_remaining);

    int rc = sqlite3_step(stmt);

    if (rc == SQLITE_ROW)
    {
        long long order_id = sqlite3_column_int64(stmt, 0);
        long long created_at = sqlite3_column_int64(stmt, 1);

        result.value = {order_id, created_at};
    }
    else
    {
        result.setError(ErrorType::Database, sqlite3_errmsg(db));
    }

    sqlite3_finalize(stmt);

    return result;
}

Result<bool> Database::recordTradeAndUpdateOrders(
    const Trade& t,
    const Order& buy,
    const Order& sell,
    long long buy_remaining,
    long long sell_remaining)
{
    Result<bool> result;
    result.value = false;

    if (!beginTransaction().isSuccess())
    {
        printf("[DB ERROR] BEGIN TRANSACTION failed\n");
        result.setError(ErrorType::Database,"BEGIN failed");
        return result;
    }

    if (!recordTrade(t).isSuccess())
    {
        printf("[DB ERROR] recordTrade failed\n");
        rollback();
        result.setError(ErrorType::Database,"recordTrade failed");
        return result;
    }

    if (!updateOrder(buy.order_id,buy_remaining,t.buyStatus).isSuccess())
    {
        printf("[DB ERROR] updateOrder BUY failed | order_id=%lld\n", buy.order_id);
        rollback();
        return result;
    }

    if (!updateOrder(sell.order_id,sell_remaining,t.sellStatus).isSuccess())
    {
        printf("[DB ERROR] updateOrder SELL failed | order_id=%lld\n", sell.order_id);
        rollback();
        return result;
    }

    long long tradeValue = (t.price * t.qty) / 10000;

    printf("[TRADE] price=%lld qty=%lld value=%lld\n", t.price, t.qty, tradeValue);

    // BUYER cash settlement
    if (buy.user_id > 0)
    {
        printf("[BUYER] subtract cash user=%d amount=%lld\n", buy.user_id, tradeValue);

        if (!subtractCashBalance(buy.user_id, tradeValue).isSuccess())
        {
            printf("[DB ERROR] subtractCashBalance failed user=%d\n", buy.user_id);
            rollback();
            return result;
        }
    }

    // SELLER cash settlement
    if (sell.user_id > 0)
    {
        printf("[SELLER] add cash user=%d amount=%lld\n", sell.user_id, tradeValue);

        if (!addCashBalance(sell.user_id, tradeValue).isSuccess())
        {
            printf("[DB ERROR] addCashBalance failed user=%d\n", sell.user_id);
            rollback();
            return result;
        }

        printf("[SELLER] release cash user=%d amount=%lld\n", sell.user_id, tradeValue);

        if (!releaseCash(sell.user_id, tradeValue).isSuccess())
        {
            printf("[DB ERROR] releaseCash failed user=%d\n", sell.user_id);
            rollback();
            return result;
        }
    }

    printf("[POSITION] BUY BEFORE qty=%lld avail=%lld\n",
        getPositionQtyRaw(buy.user_id, buy.bot_id, t.market_id).value,
        getPositionAvailableRaw(buy.user_id, buy.bot_id, t.market_id).value);

    if (!applyBuyPosition(buy.user_id,buy.bot_id,t.market_id,t.qty,t.price).isSuccess())
    {
        printf("[DB ERROR] applyBuyPosition failed user=%d bot=%d\n",
               buy.user_id, buy.bot_id);
        rollback();
        return result;
    }

    printf("[POSITION] BUY AFTER qty=%lld avail=%lld\n",
        getPositionQtyRaw(buy.user_id, buy.bot_id, t.market_id).value,
        getPositionAvailableRaw(buy.user_id, buy.bot_id, t.market_id).value);

    printf("SELL BEFORE qty=%lld avail=%lld\n",
        getPositionQtyRaw(sell.user_id, sell.bot_id, t.market_id).value,
        getPositionAvailableRaw(sell.user_id, sell.bot_id, t.market_id).value);

    auto sellPosResult = applySellPosition(sell.user_id, sell.bot_id, t.market_id, t.qty);
    if (!sellPosResult.isSuccess())
    {
        printf("[DB ERROR] applySellPosition failed user=%d bot=%d\n",
               sell.user_id, sell.bot_id);

        rollback();
        result.setError(ErrorType::Database, sellPosResult.error.getMessage());
        return result;
    }

    printf("SELL AFTER qty=%lld avail=%lld\n",
        getPositionQtyRaw(sell.user_id, sell.bot_id, t.market_id).value,
        getPositionAvailableRaw(sell.user_id, sell.bot_id, t.market_id).value);

    if (!commit().isSuccess())
    {
        printf("[DB ERROR] COMMIT failed\n");
        rollback();
        return result;
    }

    printf("[TRADE SUCCESS] trade_id committed\n");

    result.value = true;
    return result;
}

Result<bool> Database::recordTrade(const Trade& t)
{
    Result<bool> result;
    result.value = false;

    // ---- TRADE INSERT ----
    std::string sql =
        "INSERT INTO Trades (market_id, buy_order_id, sell_order_id, price, qty, executed_at) VALUES (" +
        std::to_string(t.market_id) + ", " +
        std::to_string(t.buy_order_id) + ", " +
        std::to_string(t.sell_order_id) + ", " +
        std::to_string(t.price) + ", " +
        std::to_string(t.qty) + ", CURRENT_TIMESTAMP);";

    Result<void> tradeInsertResult = executeQuery(sql);
    if (!tradeInsertResult.isSuccess())
    {
        result.setError(ErrorType::Database, tradeInsertResult.error.getMessage());
        return result;
    }

    long long trade_id = sqlite3_last_insert_rowid(db);
    long long total_cost = t.price * t.qty;

    // ---- TRANSACTION INSERT (BUYER) ----
    std::string txn_buy =
        "INSERT INTO Transactions (user_id, bot_id, trade_id, market_id, kind, amount_cash, amount_qty, created_at) "
        "SELECT user_id, bot_id, " + std::to_string(trade_id) + ", " + std::to_string(t.market_id) + ", "
        "'buy', -" + std::to_string(total_cost) + ", " + std::to_string(t.qty) + ", CURRENT_TIMESTAMP "
        "FROM Orders WHERE order_id = " + std::to_string(t.buy_order_id) + ";";

    Result<void> buyerTxnResult = executeQuery(txn_buy);
    if (!buyerTxnResult.isSuccess())
    {
        result.setError(ErrorType::Database, "Failed buyer transaction: " + buyerTxnResult.error.getMessage());
        return result;
    }

    // ---- TRANSACTION INSERT (SELLER) ----
    std::string txn_sell =
        "INSERT INTO Transactions (user_id, bot_id, trade_id, market_id, kind, amount_cash, amount_qty, created_at) "
        "SELECT user_id, bot_id, " + std::to_string(trade_id) + ", " + std::to_string(t.market_id) + ", "
        "'sell', " + std::to_string(total_cost) + ", -" + std::to_string(t.qty) + ", CURRENT_TIMESTAMP "
        "FROM Orders WHERE order_id = " + std::to_string(t.sell_order_id) + ";";

    Result<void> sellerTxnResult = executeQuery(txn_sell);
    if (!sellerTxnResult.isSuccess())
    {
        result.setError(ErrorType::Database, "Failed seller transaction: " + sellerTxnResult.error.getMessage());
        return result;
    }

    result.value = true;
    return result;
}

Result<void> Database::updateOrder(long long order_id, long long new_qty_remaining, const std::string& new_status)
{
    Result<void> result;

    if (new_qty_remaining < 0)
    {
        result.setError(ErrorType::Validation, "qty_remaining must be >= 0");
        return result;
    }

    std::string sql =
        "UPDATE Orders SET qty_remaining = " + std::to_string(new_qty_remaining) +
        ", status = '" + new_status + "', updated_at = CURRENT_TIMESTAMP " +
        "WHERE order_id = " + std::to_string(order_id) + ";";

    Result<void> queryResult = executeQuery(sql);
    if (!queryResult.isSuccess())
    {
        result.setError(ErrorType::Database, queryResult.error.getMessage());
        return result;
    }

    return result;
}

Result<void> Database::applyBuyPosition(
    int user_id,
    int bot_id,
    int market_id,
    long long qty,
    long long price)
{
    Result<void> result;
    sqlite3_stmt* stmt = nullptr;

    const char* sql_user =
        "INSERT INTO positions (user_id, bot_id, market_id, qty, qty_available, avg_price) "
        "VALUES (?, NULL, ?, ?, ?, ?) "
        "ON CONFLICT(user_id, market_id) DO UPDATE SET "
        "qty = qty + excluded.qty, "
        "qty_available = qty_available + excluded.qty;";

    const char* sql_bot =
        "INSERT INTO positions (user_id, bot_id, market_id, qty, qty_available, avg_price) "
        "VALUES (NULL, ?, ?, ?, ?, ?) "
        "ON CONFLICT(bot_id, market_id) DO UPDATE SET "
        "qty = qty + excluded.qty, "
        "qty_available = qty_available + excluded.qty;";

    const char* sql = (user_id > 0) ? sql_user : sql_bot;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        result.setError(ErrorType::Database, sqlite3_errmsg(db));
        return result;
    }

    int idx = 1;

    if (user_id > 0)
        sqlite3_bind_int(stmt, idx++, user_id);
    else
        sqlite3_bind_int(stmt, idx++, bot_id);

    sqlite3_bind_int(stmt, idx++, market_id);
    sqlite3_bind_int64(stmt, idx++, qty);
    sqlite3_bind_int64(stmt, idx++, qty);
    sqlite3_bind_int64(stmt, idx++, price);

    int rc = sqlite3_step(stmt);

    if (rc != SQLITE_DONE)
    {
        sqlite3_finalize(stmt);
        result.setError(ErrorType::Database, sqlite3_errmsg(db));
        return result;
    }

    sqlite3_finalize(stmt);
    return result;
}

Result<void> Database::applySellPosition(int user_id, int bot_id, int market_id, long long qty)
{
    Result<void> result;
    sqlite3_stmt* stmt = nullptr;

    const char* sql =
        "UPDATE positions "
        "SET qty = qty - ? "
        "WHERE market_id = ? "
        "AND qty >= ? "
        "AND ("
        "   (user_id = ? AND bot_id IS NULL) OR "
        "   (bot_id = ? AND user_id IS NULL)"
        ");";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        result.setError(ErrorType::Database, sqlite3_errmsg(db));
        return result;
    }

    sqlite3_bind_int64(stmt, 1, qty);
    sqlite3_bind_int(stmt, 2, market_id);
    sqlite3_bind_int64(stmt, 3, qty);

    if (user_id > 0)
        sqlite3_bind_int(stmt, 4, user_id);
    else
        sqlite3_bind_null(stmt, 4);

    if (bot_id > 0)
        sqlite3_bind_int(stmt, 5, bot_id);
    else
        sqlite3_bind_null(stmt, 5);

    int rc = sqlite3_step(stmt);

    if (rc != SQLITE_DONE)
    {
        sqlite3_finalize(stmt);
        result.setError(ErrorType::Database, sqlite3_errmsg(db));
        return result;
    }

    if (sqlite3_changes(db) == 0)
    {
        sqlite3_finalize(stmt);
        result.setError(ErrorType::Validation, "failed to reduce sell position");
        return result;
    }

    sqlite3_finalize(stmt);
    return result;
}

Result<bool> Database::isOrderOpen(long long order_id)
{
    Result<bool> result;

    const char* sql =
        "SELECT status FROM Orders WHERE order_id = ?;";

    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        result.setError(ErrorType::Database, sqlite3_errmsg(db));
        return result;
    }

    sqlite3_bind_int64(stmt, 1, order_id);

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        std::string status =
            (const char*)sqlite3_column_text(stmt,0);

        result.value =
            (status == "open" || status == "partial");
    }

    sqlite3_finalize(stmt);

    return result;
}

Result<bool> Database::hasAnyUser()
{
    Result<bool> result;
    result.value = false;

    const char* sql = "SELECT 1 FROM users LIMIT 1;";

    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        result.setError(ErrorType::Database, sqlite3_errmsg(db));
        return result;
    }

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        result.value = true;
    }

    sqlite3_finalize(stmt);
    return result;
}

Result<bool> Database::registerUser(const std::string& username,
                                    const std::string& passwordHash,
                                    const std::string& pinHash)
{
    Result<bool> result;
    result.value = false;

    //--------------------------------------------------
    // BEGIN TRANSACTION
    //--------------------------------------------------
    auto tx = beginTransaction();
    if (!tx.isSuccess())
    {
        std::cerr << "[DB] BEGIN TRANSACTION failed: "
                  << tx.error.getMessage() << std::endl;

        result.setError(ErrorType::Database, tx.error.getMessage());
        return result;
    }

    //--------------------------------------------------
    // INSERT USER
    //--------------------------------------------------

    const char* sql =
        "INSERT INTO users "
        "(username, password_hash, pin_hash, created_at, updated_at) "
        "VALUES (?, ?, ?, CURRENT_TIMESTAMP, CURRENT_TIMESTAMP);";

    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        std::cerr << "[DB] Failed to prepare user insert: "
                  << sqlite3_errmsg(db) << std::endl;

        rollback();

        result.setError(ErrorType::Database, sqlite3_errmsg(db));
        return result;
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, passwordHash.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, pinHash.c_str(), -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt);

    if (rc != SQLITE_DONE)
    {
        std::cerr << "[DB] Failed to insert user: "
                  << sqlite3_errmsg(db)
                  << " | username=" << username
                  << std::endl;

        sqlite3_finalize(stmt);
        rollback();

        result.setError(ErrorType::Database, sqlite3_errmsg(db));
        return result;
    }

    sqlite3_finalize(stmt);

    //--------------------------------------------------
    // GET USER ID
    //--------------------------------------------------

    long long user_id = sqlite3_last_insert_rowid(db);

    if (user_id <= 0)
    {
        std::cerr << "[DB] Failed to obtain new user_id" << std::endl;

        rollback();

        result.setError(ErrorType::Database, "Failed to obtain user_id");
        return result;
    }

    //--------------------------------------------------
    // INSERT ACCOUNT
    //--------------------------------------------------

    const char* sql_acc =
        "INSERT INTO accounts "
        "(user_id, cash_balance, cash_available, updated_at) "
        "VALUES (?, 100000000, 100000000, CURRENT_TIMESTAMP);";

    sqlite3_stmt* stmt2 = nullptr;

    if (sqlite3_prepare_v2(db, sql_acc, -1, &stmt2, nullptr) != SQLITE_OK)
    {
        std::cerr << "[DB] Failed to prepare account insert: "
                  << sqlite3_errmsg(db)
                  << " | user_id=" << user_id
                  << std::endl;

        rollback();

        result.setError(ErrorType::Database, sqlite3_errmsg(db));
        return result;
    }

    sqlite3_bind_int64(stmt2, 1, user_id);

    rc = sqlite3_step(stmt2);

    if (rc != SQLITE_DONE)
    {
        std::cerr << "[DB] Failed to create account: "
                  << sqlite3_errmsg(db)
                  << " | user_id=" << user_id
                  << std::endl;

        sqlite3_finalize(stmt2);
        rollback();

        result.setError(ErrorType::Database, sqlite3_errmsg(db));
        return result;
    }

    sqlite3_finalize(stmt2);

    //--------------------------------------------------
    // COMMIT
    //--------------------------------------------------

    auto commitResult = commit();

    if (!commitResult.isSuccess())
    {
        std::cerr << "[DB] COMMIT failed: "
                  << commitResult.error.getMessage()
                  << std::endl;

        result.setError(ErrorType::Database, commitResult.error.getMessage());
        return result;
    }

    //--------------------------------------------------
    // SUCCESS
    //--------------------------------------------------

    result.value = true;
    return result;
}

Result<std::string> Database::getUsername(int userId)
{
    Result<std::string> result;

    const char* sql =
        "SELECT username FROM users WHERE user_id = ?;";

    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    if (rc != SQLITE_OK)
    {
        result.setError(ErrorType::Database, sqlite3_errmsg(db));
        return result;
    }

    sqlite3_bind_int(stmt, 1, userId);

    rc = sqlite3_step(stmt);

    if (rc == SQLITE_ROW)
    {
        const unsigned char* text = sqlite3_column_text(stmt, 0);

        if (!text)
        {
            result.setError(ErrorType::Database, "Username column is NULL");
        }
        else
        {
            result.value = reinterpret_cast<const char*>(text);
        }
    }
    else if (rc == SQLITE_DONE)
    {
        result.setError(ErrorType::Database, "User not found");
    }
    else
    {
        result.setError(ErrorType::Database, sqlite3_errmsg(db));
    }

    sqlite3_finalize(stmt);
    return result;
}

Result<int> Database::getUserId(const std::string& username)
{
    Result<int> result;

    const char* sql =
        "SELECT user_id FROM users WHERE username = ?;";

    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        result.setError(ErrorType::Database, sqlite3_errmsg(db));
        return result;
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt);

    if (rc == SQLITE_ROW)
    {
        result.value = sqlite3_column_int(stmt, 0);
    }
    else if (rc == SQLITE_DONE)
    {
        result.setError(ErrorType::Database, "User not found");
    }
    else
    {
        result.setError(ErrorType::Database, sqlite3_errmsg(db));
    }

    sqlite3_finalize(stmt);
    return result;
}

Result<int> Database::getBotId(const std::string& botName)
{
    Result<int> result;

    const char* sql =
        "SELECT bot_id FROM Bots WHERE bot_name = ?;";

    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        result.setError(ErrorType::Database, sqlite3_errmsg(db));
        result.value = 0;
        return result;
    }

    sqlite3_bind_text(stmt, 1, botName.c_str(), -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt);

    if (rc == SQLITE_ROW)
    {
        result.value = sqlite3_column_int(stmt, 0);
    }
    else
    {
        result.setError(ErrorType::Database, "Bot not found");
        result.value = 0;
    }

    sqlite3_finalize(stmt);

    return result;
}

long long Database::loginUser(const std::string& username, const std::string& password) {
    const char* sql = "SELECT user_id FROM users WHERE username=? AND password_hash=?;";
    
    sqlite3_stmt* stmt;
    long long user_id = -1; 

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) == SQLITE_OK) {
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

Result<std::string> Database::getPasswordHash(const std::string& username)
{
    std::string sql =
        "SELECT password_hash FROM users WHERE username=?;";

    sqlite3_stmt* stmt = nullptr;
    std::string hash;
    Result<std::string> result;

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK)
    {
        sqlite3_bind_text(stmt, 1,
                          username.c_str(),
                          -1,
                          SQLITE_TRANSIENT);

        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            const unsigned char* text =
                sqlite3_column_text(stmt, 0);

            if (text)
                hash = reinterpret_cast<const char*>(text);
        }
        else
        {
            // For security purpose, this message should not be displayed.
            result.setError(ErrorType::Database, "username does not exist");
        }
    }
    else
    {
        result.setError(ErrorType::Database, sqlite3_errmsg(db));
    }

    sqlite3_finalize(stmt);
    result.value = hash;
    return result;
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

Result<std::string> Database::getMarketName(int marketId)
{
    Result<std::string> result;
    sqlite3_stmt* stmt = nullptr;

    const char* sql =
        "SELECT market_name "
        "FROM Markets "
        "WHERE market_id = ?;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        result.setError(ErrorType::Database, sqlite3_errmsg(db));
        return result;
    }

    sqlite3_bind_int(stmt, 1, marketId);

    int rc = sqlite3_step(stmt);

    if (rc == SQLITE_ROW)
    {
        const unsigned char* name = sqlite3_column_text(stmt, 0);
        result.value = std::string(reinterpret_cast<const char*>(name));
    }
    else if (rc == SQLITE_DONE)
    {
        result.setError(ErrorType::Validation, "market not found");
    }
    else
    {
        result.setError(ErrorType::Database, sqlite3_errmsg(db));
    }

    sqlite3_finalize(stmt);
    return result;
}

Result<std::string> Database::getMarketSymbol(int marketId)
{
    Result<std::string> result;
    sqlite3_stmt* stmt = nullptr;

    const char* sql =
        "SELECT symbol "
        "FROM Markets "
        "WHERE market_id = ?;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        result.setError(ErrorType::Database, sqlite3_errmsg(db));
        return result;
    }

    sqlite3_bind_int(stmt, 1, marketId);

    int rc = sqlite3_step(stmt);

    if (rc == SQLITE_ROW)
    {
        const unsigned char* symbol = sqlite3_column_text(stmt, 0);
        result.value = std::string(reinterpret_cast<const char*>(symbol));
    }
    else if (rc == SQLITE_DONE)
    {
        result.setError(ErrorType::Validation, "symbol not found");
    }
    else
    {
        result.setError(ErrorType::Database, sqlite3_errmsg(db));
    }

    sqlite3_finalize(stmt);
    return result;
}


Result<long long> Database::getAvailableCash(int user_id)
{
    Result<long long> result;

    sqlite3_stmt* stmt = nullptr;

    const char* sql =
        "SELECT cash_available "
        "FROM accounts "
        "WHERE user_id = ?;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        result.setError(ErrorType::Database, sqlite3_errmsg(db));
        return result;
    }

    sqlite3_bind_int64(stmt, 1, user_id);

    int rc = sqlite3_step(stmt);

    if (rc == SQLITE_ROW)
    {
        result.value = sqlite3_column_int64(stmt, 0);
    }
    else if (rc == SQLITE_DONE)
    {
        result.value = 0;
    }
    else
    {
        result.setError(ErrorType::Database, sqlite3_errmsg(db));
    }

    sqlite3_finalize(stmt);

    return result;
}

Result<long long> Database::getLastPriceRaw(int market_id)
{
    Result<long long> result;
    sqlite3_stmt* stmt = nullptr;

    const char* sql =
        "SELECT price FROM Trades "
        "WHERE market_id = ? "
        "ORDER BY trade_id DESC LIMIT 1;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        result.setError(ErrorType::Database, sqlite3_errmsg(db));
        return result;
    }

    sqlite3_bind_int(stmt, 1, market_id);

    int rc = sqlite3_step(stmt);

    if (rc == SQLITE_ROW)
    {
        result.value = sqlite3_column_int64(stmt, 0);
    }
    else if (rc == SQLITE_DONE)
    {
        result.value = 0;
    }
    else
    {
        result.setError(ErrorType::Database, sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return result;
    }

    sqlite3_finalize(stmt);
    return result;
}

Result<long long> Database::getPositionQtyRaw(int user_id, int bot_id, int market_id)
{
    Result<long long> result;
    sqlite3_stmt* stmt = nullptr;

    const char* sql =
        "SELECT qty FROM Positions "
        "WHERE market_id = ? "
        "AND ("
        "   (user_id = ? AND bot_id IS NULL) OR "
        "   (bot_id = ? AND user_id IS NULL)"
        ");";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        result.setError(ErrorType::Database, sqlite3_errmsg(db));
        return result;
    }

    sqlite3_bind_int(stmt, 1, market_id);

    // user_id
    if (user_id > 0)
        sqlite3_bind_int(stmt, 2, user_id);
    else
        sqlite3_bind_null(stmt, 2);

    // bot_id
    if (bot_id > 0)
        sqlite3_bind_int(stmt, 3, bot_id);
    else
        sqlite3_bind_null(stmt, 3);

    int rc = sqlite3_step(stmt);

    if (rc == SQLITE_ROW)
    {
        result.value = sqlite3_column_int64(stmt, 0);
    }
    else if (rc == SQLITE_DONE)
    {
        result.value = 0;
    }
    else
    {
        result.setError(ErrorType::Database, sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return result;
    }

    sqlite3_finalize(stmt);
    return result;
}

Result<long long> Database::getPositionAvailableRaw(int user_id, int bot_id, int market_id)
{
    Result<long long> result;
    sqlite3_stmt* stmt = nullptr;

    const char* sql =
        "SELECT qty_available FROM Positions "
        "WHERE market_id = ? "
        "AND ("
        "   (user_id = ? AND bot_id IS NULL) OR "
        "   (bot_id = ? AND user_id IS NULL)"
        ");";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        result.setError(ErrorType::Database, sqlite3_errmsg(db));
        return result;
    }

    // market_id
    sqlite3_bind_int(stmt, 1, market_id);

    // user_id
    if (user_id > 0)
        sqlite3_bind_int(stmt, 2, user_id);
    else
        sqlite3_bind_null(stmt, 2);

    // bot_id
    if (bot_id > 0)
        sqlite3_bind_int(stmt, 3, bot_id);
    else
        sqlite3_bind_null(stmt, 3);

    int rc = sqlite3_step(stmt);

    if (rc == SQLITE_ROW)
    {
        result.value = sqlite3_column_int64(stmt, 0);
    }
    else if (rc == SQLITE_DONE)
    {
        result.value = 0;
    }
    else
    {
        result.setError(ErrorType::Database, sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return result;
    }

    sqlite3_finalize(stmt);
    return result;
}

long long Database::getPositionAvePriceRaw(int user_id, int market_id)
{
    sqlite3_stmt* stmt;
    long long avePrice = 0;

    const char* sql =
        "SELECT avg_price FROM Positions "
        "WHERE user_id = ? AND market_id = ?;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK)
    {
        sqlite3_bind_int64(stmt, 1, user_id);
        sqlite3_bind_int(stmt, 2, market_id);

        if (sqlite3_step(stmt) == SQLITE_ROW)
            avePrice = sqlite3_column_int64(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return avePrice;
}

std::pair<long long, long long> Database::getPrevAndCurrentPricesRaw(int market_id)
{
    sqlite3_stmt* stmt;
    long long latest = 0;
    long long previous = 0;

    const char* sql =
        "SELECT price FROM Trades "
        "WHERE market_id = ? "
        "ORDER BY executed_at DESC "
        "LIMIT 2;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK)
    {
        sqlite3_bind_int(stmt, 1, market_id);

        int count = 0;

        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            if (count == 0)
                latest = sqlite3_column_int64(stmt, 0);
            else if (count == 1)
                previous = sqlite3_column_int64(stmt, 0);

            count++;
        }
    }

    sqlite3_finalize(stmt);
    return {latest, previous};
}

long long Database::getRealizedPnLRaw(int user_id)
{
    sqlite3_stmt* stmt;
    long long pnl = 0;

    const char* sql =
        "SELECT SUM(amount_cash) FROM Transactions "
        "WHERE user_id = ? AND kind = 'sell';";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK)
    {
        sqlite3_bind_int64(stmt, 1, user_id);

        if (sqlite3_step(stmt) == SQLITE_ROW)
            pnl = sqlite3_column_int64(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return pnl;
}

// std::vector<OrderHistoryRow> Database::getUserOrderHistory(int user_id) {
//     std::vector<OrderHistoryRow> history;
//     sqlite3_stmt* stmt;
    
//     const char* sql = R"(
//         SELECT o.created_at, m.symbol, o.side, o.price, o.qty, o.status 
//         FROM Orders o
//         JOIN Markets m ON o.market_id = m.market_id
//         WHERE o.user_id = ?
//         ORDER BY o.created_at DESC;
//     )";
    
//     if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
//         sqlite3_bind_int64(stmt, 1, user_id);
        
//         while (sqlite3_step(stmt) == SQLITE_ROW) {
//             OrderHistoryRow row;
//             const unsigned char* t_text = sqlite3_column_text(stmt, 0);
//             row.time = t_text ? reinterpret_cast<const char*>(t_text) : "";
//             const unsigned char* m_text = sqlite3_column_text(stmt, 1);
//             row.market_symbol = m_text ? reinterpret_cast<const char*>(m_text) : "";
//             const unsigned char* s_text = sqlite3_column_text(stmt, 2);
//             row.side = s_text ? reinterpret_cast<const char*>(s_text) : "";
//             row.price = sqlite3_column_double(stmt, 3);
//             row.quantity = sqlite3_column_double(stmt, 4);
//             row.total = row.price * row.quantity;
//             const unsigned char* st_text = sqlite3_column_text(stmt, 5);
//             row.status = st_text ? reinterpret_cast<const char*>(st_text) : "";
//             history.push_back(row);
//         }
//     }
//     sqlite3_finalize(stmt);
//     return history;
// }

std::vector<std::pair<long long, long long>> Database::getTopBuyOrders(int market_id, int limit)
{
    std::vector<std::pair<long long, long long>> book;
    sqlite3_stmt* stmt;

    const char* sql = R"(
        SELECT price, SUM(qty_remaining) FROM Orders
        WHERE market_id = ? AND side = 'buy' AND status IN ('open', 'partial')
        GROUP BY price
        ORDER BY price DESC
        LIMIT ?;
    )";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK)
    {
        sqlite3_bind_int(stmt, 1, market_id);
        sqlite3_bind_int(stmt, 2, limit);

        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            long long price = sqlite3_column_int64(stmt, 0);
            long long qty   = sqlite3_column_int64(stmt, 1);

            book.push_back({price, qty});
        }
    }

    sqlite3_finalize(stmt);
    return book;
}

std::vector<std::pair<long long, long long>> Database::getTopSellOrders(int market_id, int limit)
{
    std::vector<std::pair<long long, long long>> book;
    sqlite3_stmt* stmt;

    const char* sql = R"(
        SELECT price, SUM(qty_remaining)
        FROM Orders
        WHERE market_id = ? AND side = 'sell' AND status IN ('open', 'partial')
        GROUP BY price
        ORDER BY price ASC
        LIMIT ?;
    )";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK)
    {
        sqlite3_bind_int(stmt, 1, market_id);
        sqlite3_bind_int(stmt, 2, limit);

        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            long long price = sqlite3_column_int64(stmt, 0);
            long long qty   = sqlite3_column_int64(stmt, 1);

            book.push_back({price, qty});
        }
    }

    sqlite3_finalize(stmt);
    return book;
}

// tick_size: The minimum price increment allowed for orders in this market.
Result<long long> Database::getTickSize(int market_id)
{
    Result<long long> result;
    sqlite3_stmt* stmt;

    const char* sql =
        "SELECT tick_size "
        "FROM markets "
        "WHERE market_id = ?;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        result.value = 0;
        result.setError(ErrorType::Database, sqlite3_errmsg(db));
        return result;
    }

    sqlite3_bind_int(stmt, 1, market_id);

    long long tickSize = 0;

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        tickSize = sqlite3_column_int64(stmt, 0);
    }

    sqlite3_finalize(stmt);

    result.value = tickSize;
    return result;
}

// lot_size: The minimum quantity increment allowed for orders in this market.
Result<long long> Database::getLotSize(int market_id)
{
    Result<long long> result;
    sqlite3_stmt* stmt;

    const char* sql =
        "SELECT lot_size "
        "FROM markets "
        "WHERE market_id = ?;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        result.value = 1;
        result.setError(ErrorType::Database, sqlite3_errmsg(db));
        return result;
    }

    sqlite3_bind_int(stmt, 1, market_id);

    long long lotSize = 0;

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        lotSize = sqlite3_column_int64(stmt, 0);
    }

    sqlite3_finalize(stmt);

    if (lotSize <= 0)
    {
        lotSize = 1;
    }

    result.value = lotSize;

    return result;
}

Result<long long> Database::getReferencePrice(int market_id)
{
    sqlite3_stmt* stmt;

    const char* sql =
        "SELECT reference_price "
        "FROM markets "
        "WHERE market_id = ?;";

    Result<long long> result;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        result.value = 0;
        result.setError(ErrorType::Database, sqlite3_errmsg(db));
        return result;
    }

    sqlite3_bind_int(stmt, 1, market_id);

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        result.value = sqlite3_column_int64(stmt, 0);
    }
    else
    {
        result.value = 0;
        result.setError(ErrorType::Database, "reference price not found");
    }

    sqlite3_finalize(stmt);

    return result;
}

// --- NEW: LEADERBOARD LOGIC ---
void Database::updateHighScore(const std::string& username, double cash, const std::string& rank) {
    long long cash_fixed = static_cast<long long>(cash * 10000);
    std::string sql = "INSERT INTO leaderboard (username, peak_cash, rank_title) "
                      "VALUES ('" + username + "', " + std::to_string(cash_fixed) + ", '" + rank + "') "
                      "ON CONFLICT(username) DO UPDATE SET "
                      "rank_title = excluded.rank_title, "
                      "updated_at = CURRENT_TIMESTAMP, "
                      "peak_cash = MAX(leaderboard.peak_cash, excluded.peak_cash);";
    executeQuery(sql);
}

void Database::showLeaderboard() {
    sqlite3_stmt* stmt;
    const char* sql = "SELECT username, peak_cash, rank_title FROM leaderboard ORDER BY peak_cash DESC LIMIT 5;";
    
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
Result<bool> Database::deleteUserAccount(int userId)
{
    Result<bool> resultDelete;

    std::string uid_str = std::to_string(userId);

    // Get username
    Result<std::string> resultUsername = getUsername(userId);
    if (!resultUsername.isSuccess())
    {
        resultDelete.setError(ErrorType::Database, "Failed to get username for deletion.");
        return resultDelete;
    }

    std::string username = resultUsername.value;

    // ---------- BEGIN TRANSACTION ----------
    Result<void> tx = beginTransaction();
    if (!tx.isSuccess())
    {
        resultDelete.setError(ErrorType::Database, tx.error.getMessage());
        return resultDelete;
    }

    // 1. Delete dependent records
    Result<void> r1 = executeQuery("DELETE FROM Positions WHERE user_id = " + uid_str + ";");
    if (!r1.isSuccess())
    {
        rollback();
        resultDelete.setError(ErrorType::Database, "Failed to delete positions.");
        return resultDelete;
    }

    Result<void> r2 = executeQuery("DELETE FROM accounts WHERE user_id = " + uid_str + ";");
    if (!r2.isSuccess())
    {
        rollback();
        resultDelete.setError(ErrorType::Database, "Failed to delete accounts.");
        return resultDelete;
    }

    Result<void> r3 = executeQuery("DELETE FROM leaderboard WHERE username = '" + username + "';");
    if (!r3.isSuccess())
    {
        rollback();
        resultDelete.setError(ErrorType::Database, "Failed to delete leaderboard entry.");
        return resultDelete;
    }

    // 2. Delete user
    Result<void> r4 = executeQuery("DELETE FROM users WHERE user_id = " + uid_str + ";");
    if (!r4.isSuccess())
    {
        rollback();
        resultDelete.setError(ErrorType::Database, "Failed to delete user.");
        return resultDelete;
    }

    // ---------- COMMIT ----------
    Result<void> commitResult = commit();
    if (!commitResult.isSuccess())
    {
        rollback();
        resultDelete.setError(ErrorType::Database, commitResult.error.getMessage());
        return resultDelete;
    }

    std::cout << "\n[SYSTEM] Account for '" << username << "' has been permanently wiped.\n";
    std::cout << "[SYSTEM] The username is now available for re-registration.\n";

    resultDelete.value = true;
    return resultDelete;
}

Result<void> Database::setAvailableCash(int user_id, long long amount)
{
    Result<void> result;

    if (amount < 0)
    {
        result.setError(ErrorType::Validation, "available cash cannot be negative");
        return result;
    }

    const char* sql =
        "UPDATE accounts "
        "SET cash_available = ?, "
        "updated_at = CURRENT_TIMESTAMP "
        "WHERE user_id = ?;";

    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        result.setError(ErrorType::Database, sqlite3_errmsg(db));
        return result;
    }

    sqlite3_bind_int64(stmt, 1, amount);
    sqlite3_bind_int64(stmt, 2, user_id);

    int rc = sqlite3_step(stmt);

    if (rc != SQLITE_DONE)
    {
        result.setError(ErrorType::Database, sqlite3_errmsg(db));
    }

    sqlite3_finalize(stmt);
    return result;
}

Result<void> Database::lockCash(int user_id, long long amount)
{
    Result<void> result;

    const char* sql =
        "UPDATE accounts "
        "SET cash_available = cash_available - ?, "
        "updated_at = CURRENT_TIMESTAMP "
        "WHERE user_id = ? AND cash_available >= ?;";

    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        result.setError(ErrorType::Database, sqlite3_errmsg(db));
        return result;
    }

    sqlite3_bind_int64(stmt, 1, amount);
    sqlite3_bind_int64(stmt, 2, user_id);
    sqlite3_bind_int64(stmt, 3, amount);

    int rc = sqlite3_step(stmt);

    if (rc != SQLITE_DONE)
        result.setError(ErrorType::Database, sqlite3_errmsg(db));
    else if (sqlite3_changes(db) == 0)
        result.setError(ErrorType::Validation, "insufficient cash");

    sqlite3_finalize(stmt);
    return result;
}

Result<void> Database::releaseCash(int user_id, long long amount)
{
    Result<void> result;

    const char* sql =
        "UPDATE accounts "
        "SET cash_available = cash_available + ?, "
        "updated_at = CURRENT_TIMESTAMP "
        "WHERE user_id = ?;";

    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        result.setError(ErrorType::Database, sqlite3_errmsg(db));
        return result;
    }

    sqlite3_bind_int64(stmt, 1, amount);
    sqlite3_bind_int64(stmt, 2, user_id);

    int rc = sqlite3_step(stmt);

    if (rc != SQLITE_DONE)
    {
        result.setError(ErrorType::Database, sqlite3_errmsg(db));
    }

    sqlite3_finalize(stmt);
    return result;
}

Result<void> Database::addCashBalance(int user_id, long long amount)
{
    Result<void> result;

    if (amount < 0)
    {
        result.setError(ErrorType::Validation, "Amount must be positive");
        return result;
    }

    const char* sql =
        "UPDATE accounts "
        "SET cash_balance = cash_balance + ?, "
        "updated_at = CURRENT_TIMESTAMP "
        "WHERE user_id = ?;";

    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        result.setError(ErrorType::Database, sqlite3_errmsg(db));
        return result;
    }

    sqlite3_bind_int64(stmt, 1, amount);
    sqlite3_bind_int64(stmt, 2, user_id);

    int rc = sqlite3_step(stmt);

    if (rc != SQLITE_DONE)
    {
        result.setError(ErrorType::Database, sqlite3_errmsg(db));
    }

    sqlite3_finalize(stmt);

    return result;
}

Result<void> Database::subtractCashBalance(int user_id, long long amount)
{
    Result<void> result;

    if (amount < 0)
    {
        result.setError(ErrorType::Validation, "Amount must be positive");
        return result;
    }

    const char* sql =
        "UPDATE accounts "
        "SET cash_balance = cash_balance - ?, "
        "updated_at = CURRENT_TIMESTAMP "
        "WHERE user_id = ? AND cash_balance >= ?;";

    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        result.setError(ErrorType::Database, sqlite3_errmsg(db));
        return result;
    }

    sqlite3_bind_int64(stmt, 1, amount);
    sqlite3_bind_int64(stmt, 2, user_id);
    sqlite3_bind_int64(stmt, 3, amount);

    int rc = sqlite3_step(stmt);

    if (rc != SQLITE_DONE)
    {
        result.setError(ErrorType::Database, sqlite3_errmsg(db));
    }

    sqlite3_finalize(stmt);

    return result;
}

Result<void> Database::lockPosition(
    int user_id,
    int bot_id,
    int market_id,
    long long qty)
{
    // if (user_id > 0)
    // {
    //     printf("qty: %lld\n", qty);
    //     printf("available qty: %lld\n", getPositionAvailableRaw(user_id, bot_id, market_id).value);
    //     printf("now raw qty: %lld\n", getPositionQtyRaw(user_id, bot_id, market_id).value);
    // }
    Result<void> result;

    sqlite3_stmt* stmt = nullptr;

    const char* sql =
        "UPDATE positions "
        "SET qty_available = qty_available - ? "
        "WHERE market_id = ? "
        "AND qty_available >= ? "
        "AND ("
        "    (user_id = ? AND bot_id IS NULL) OR "
        "    (bot_id = ? AND user_id IS NULL)"
        ");";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        result.setError(ErrorType::Database, sqlite3_errmsg(db));
        return result;
    }

    sqlite3_bind_int64(stmt,1,qty);
    sqlite3_bind_int(stmt,2,market_id);
    sqlite3_bind_int64(stmt,3,qty);
    if (user_id > 0)
        sqlite3_bind_int64(stmt, 4, user_id);
    else
        sqlite3_bind_null(stmt, 4);

    // bot_id
    if (bot_id > 0)
        sqlite3_bind_int64(stmt, 5, bot_id);
    else
        sqlite3_bind_null(stmt, 5);

    int rc = sqlite3_step(stmt);

    if (rc != SQLITE_DONE)
    {
        sqlite3_finalize(stmt);
        result.setError(ErrorType::Database, sqlite3_errmsg(db));
        return result;
    }

    if (sqlite3_changes(db) == 0)
    {
        sqlite3_finalize(stmt);
        result.setError(ErrorType::Validation,"insufficient available position");
        return result;
    }

    sqlite3_finalize(stmt);
    return result;
}

Result<void> Database::releasePosition(
    int user_id,
    int bot_id,
    int market_id,
    long long qty)
{
    Result<void> result;

    sqlite3_stmt* stmt = nullptr;

    const char* sql =
        "UPDATE positions "
        "SET qty_available = qty_available + ? "
        "WHERE market_id = ? "
        "AND ("
        "    (user_id = ? AND bot_id IS NULL) OR "
        "    (bot_id = ? AND user_id IS NULL)"
        ");";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        result.setError(ErrorType::Database, sqlite3_errmsg(db));
        return result;
    }

    sqlite3_bind_int64(stmt,1,qty);
    sqlite3_bind_int(stmt,2,market_id);
    if (user_id > 0)
        sqlite3_bind_int(stmt,3,user_id);
    else
        sqlite3_bind_null(stmt,3);

    if (bot_id > 0)
        sqlite3_bind_int(stmt,4,bot_id);
    else
        sqlite3_bind_null(stmt,4);

    int rc = sqlite3_step(stmt);

    if (rc != SQLITE_DONE)
    {
        sqlite3_finalize(stmt);
        result.setError(ErrorType::Database, sqlite3_errmsg(db));
        return result;
    }

    sqlite3_finalize(stmt);
    return result;
}

double Database::getAvailablePosition(int user_id, int market_id)
{
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

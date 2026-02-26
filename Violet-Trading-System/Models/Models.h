//
//  Models.h
//  Violet-Trading-System
//
//  Created by Nam Nguyen on 2/13/26.
//

#ifndef MODELS_H
#define MODELS_H

#include <string>
#include <vector>

// Core Identity
struct User {
    long long user_id;           // PK
    std::string username;
    std::string password_hash;
    std::string pin_hash;
    std::string created_at;
};

// Financial Account
struct Account {
    long long account_id;        // PK
    long long user_id;           // FK -> User
    long long cash_balance;      // Stored in cents (e.g., $100.00 = 10000)
    long long cash_available;    // Balance minus open orders
};

// Market Configuration
struct Market {
    long long market_id;         // PK
    std::string symbol;          // e.g., "BTC/USD"
    long long tick_size;         // Min price movement (in cents)
    long long lot_size;          // Min quantity movement
    std::string status;          // "open" or "closed"
};

// Bot Identity
struct Bot {
    long long bot_id;            // PK
    std::string bot_name;
    std::string bot_type;        // e.g., "market_maker"
};

// Order Book Entry
struct Order {
    long long order_id;          // PK
    long long user_id;           // Optional FK
    long long bot_id;            // Optional FK
    long long market_id;         // FK
    std::string side;            // "buy" or "sell"
    std::string type;            // "limit" or "market"
    long long price;             // Stored as fixed-point cents
    long long qty;               // Original quantity
    long long qty_remaining;     // For partial fills
    std::string status;          // "open", "filled", "partial", "canceled"
};

// Matched Execution
struct Trade {
    long long trade_id;          // PK
    long long market_id;         // FK
    long long buy_order_id;      // FK -> Order
    long long sell_order_id;     // FK -> Order
    long long price;             // Execution price
    long long qty;               // Execution quantity
    std::string executed_at;     // Timestamp
};

// Audit Trail
struct Transaction {
    long long txn_id;            // PK
    long long user_id;           // FK
    long long trade_id;          // FK
    std::string kind;            // "buy", "sell", or "fee"
    long long amount_cash;       // Impact on cash balance
    long long amount_qty;        // Impact on holdings
    std::string created_at;      // Timestamp
};

// --- API STRUCTURES FOR UI ---
struct OrderHistoryRow {
    std::string time;
    std::string market_symbol; // e.g., "SPU"
    std::string side;          // "Buy" or "Sell"
    double price;
    double quantity;
    double total;
    std::string status;        // "Filled", "Open", etc.
};

#endif // MODELS_H

//
//  Models.h
//  Violet-Trading-System
//

#ifndef MODELS_H
#define MODELS_H

#include <string>
#include <vector>

// --- SHARED PROGRESSION LOGIC ---
// Use this function to display the user's Badge/Title in the UI
inline std::string getTraderRank(double cash) {
    if (cash < 25000) return "ROOKIE DUELIST";
    if (cash < 100000) return "ELITE TRADER";
    if (cash < 500000) return "MARKET MASTER";
    if (cash < 1000000) return "DUEL KING";
    return "LEGENDARY WHALE";
}

// Core Identity
struct User {
    long long user_id;           
    std::string username;
    std::string password_hash;
    std::string pin_hash;
    std::string created_at;
};

// Financial Account
struct Account {
    long long account_id;        
    long long user_id;           
    long long cash_balance;      
    long long cash_available;    
};

// Market Configuration
struct Market {
    long long market_id;         
    std::string symbol;          
    long long tick_size;         
    long long lot_size;          
    std::string status;          
};

// Bot Identity
struct Bot {
    long long bot_id;            
    std::string bot_name;
    std::string bot_type;        
};

// Order Book Entry
struct Order {
    long long order_id;          
    long long user_id;           
    long long bot_id;            
    long long market_id;         
    std::string side;            
    std::string type;            
    long long price;             // Stored as fixed-point cents
    long long qty;               
    long long qty_remaining;     
    std::string status;          
};

// Matched Execution
struct Trade {
    long long trade_id;          
    long long market_id;         
    long long buy_order_id;      
    long long sell_order_id;     
    long long price;             
    long long qty;               
    std::string executed_at;     
};

// Audit Trail
struct Transaction {
    long long txn_id;            
    long long user_id;           
    long long trade_id;          
    std::string kind;            
    long long amount_cash;       
    long long amount_qty;        
    std::string created_at;      
};

// --- API STRUCTURES FOR UI ---
struct OrderHistoryRow {
    std::string time;
    std::string market_symbol; 
    std::string side;          
    double price;
    double quantity;
    double total;
    std::string status;        
};

#endif // MODELS_H

#include <iostream>
#include <string>
#include <cstdlib> 
#include <ctime>   
#include <cmath>   
#include "Database/Database.h"
#include "Engine/Engine.h"

using namespace std;

void clearInput() {
    cin.clear();
    cin.ignore(10000, '\n');
}

int showDailyIntel(int bot_markets[]) {
    string stocks[] = {"SPU", "BTC", "AAPL", "TSLA", "NVDA"};
    cout << "\n[!] --- DAILY INTEL PACK OPENED --- [!]\n";
    
    if (rand() % 100 == 0) {
        cout << ">> ULTRA RARE: 'SWORDS OF REVEALING LIGHT' PULLED!\n";
        cout << ">> Effect: The Market is FROZEN for 3 turns. Bots cannot move!\n";
        cout << "------------------------------------------\n";
        return 3; 
    }
    
    int hintIdx = rand() % 5;
    // Change the intro text
    cout << "\n[!] --- NEW MARKET INTELLIGENCE RECEIVED --- [!]\n";

    // Change the hint cases to sound more like a "Scout Report"
    switch(hintIdx) {
        case 0: cout << ">> SCOUT: " << stocks[bot_markets[0]-1] << " is showing massive panic selling.\n"; break;
        case 1: cout << ">> SCOUT: The Whale has moved to " << stocks[bot_markets[1]-1] << ".\n"; break;
        case 2: cout << ">> SCOUT: Tracking 'Fat Finger' errors in " << stocks[bot_markets[2]-1] << ".\n"; break;
        case 3: cout << ">> SCOUT: " << stocks[bot_markets[3]-1] << " is entering a Sine Wave cycle.\n"; break;
        case 4: cout << ">> SCOUT: FOMO Dragon (Greed Bot) spotted in " << stocks[bot_markets[4]-1] << ".\n"; break;
    }
    cout << "------------------------------------------\n";
    return 0; 
}

void runMarketSimulation(Engine& engine, Database& db, int tick, int assigned_markets[], bool shadowRealmActive) {
    long long base_price = 1000000; 

    // --- 1. SYSTEM MARKET MAKERS ---
    // In Speed Duel, the MM provides more liquidity (40 shares instead of 20) 
    // to absorb the higher volatility.
    for (int m = 1; m <= 5; m++) {
        Order mm_buy;
        mm_buy.user_id = 999; mm_buy.market_id = m;
        mm_buy.side = "buy"; mm_buy.type = "limit";
        mm_buy.price = base_price - 20000; 
        mm_buy.qty = 40 * 10000; mm_buy.qty_remaining = mm_buy.qty;
        mm_buy.status = "open";
        Order mm_sell = mm_buy; mm_sell.side = "sell"; mm_sell.price = base_price + 20000; 
        engine.placeOrder(mm_buy); engine.placeOrder(mm_sell);
    }

    // --- 2. TRAP CARD: SHADOW REALM COLLAPSE ---
    // The Panic Bot now dumps 200,000 shares (20 shares) per market, 
    // creating a much deeper crash.
    if (shadowRealmActive) {
        for (int m = 1; m <= 5; m++) {
            Order panic;
            panic.user_id = 901; panic.market_id = m;
            panic.side = "sell"; panic.type = "limit";
            panic.price = base_price - 150000; 
            panic.qty = 200000; panic.qty_remaining = panic.qty; panic.status = "open";
            engine.placeOrder(panic);
        }
    }

    // --- 3. THE PANIC BOT (Standard) ---
    // Fires 20% of the time (up from 10%) to keep you on your toes.
    if (rand() % 5 == 0) { 
        Order p; p.user_id = 901; p.market_id = assigned_markets[0]; p.side = "sell"; p.type = "limit";
        p.price = base_price - 50000; p.qty = 100000; p.qty_remaining = p.qty; p.status = "open";
        engine.placeOrder(p);
    }

    // --- 4. THE WHALE ---
    // Appears every 2 ticks (50% uptime) to provide a "Speed Duel" safety floor.
    if (tick % 2 == 0) {
        Order w; w.user_id = 902; w.market_id = assigned_markets[1]; w.side = "buy"; w.type = "limit";
        w.price = base_price - 100000; w.qty = 100000000; w.qty_remaining = w.qty; w.status = "open";
        engine.placeOrder(w);
    }

    // --- 5. THE FAT FINGER ---
    // Increased quantity to 200,000. Catching one of these is now a massive windfall.
    Order f; f.user_id = 903; f.market_id = assigned_markets[2]; f.side = (rand()%2==0)?"buy":"sell";
    f.type = "limit"; f.price = (rand()%20==0)? base_price*10 : base_price; f.qty = 200000;
    f.qty_remaining = f.qty; f.status = "open"; engine.placeOrder(f);
    
    // --- 6. THE SINE WAVE ---
    // Higher amplitude (300,000) means wider swings and bigger profit potential.
    Order s; s.user_id = 904; s.market_id = assigned_markets[3]; s.side = (sin(tick*0.5)>0)?"sell":"buy";
    s.type = "limit"; s.price = base_price + (sin(tick*0.5)*300000); s.qty = 150000;
    s.qty_remaining = s.qty; s.status = "open"; engine.placeOrder(s);
    
    // --- 7. THE GREED BOT (FOMO) ---
    // Now outbids by $0.10 (1000 units) instead of $0.01. 
    // This bot will chase the price up the book extremely fast.
    auto tb = db.getTopBuyOrders(assigned_markets[4], 1);
    long long fp = (!tb.empty()) ? (tb[0].first * 10000) + 1000 : base_price;
    Order g; g.user_id = 905; g.market_id = assigned_markets[4]; g.side = "buy"; g.type = "limit";
    g.price = fp; g.qty = 80000; g.qty_remaining = g.qty; g.status = "open"; engine.placeOrder(g);
}

int main() {
    cout << "========================================\n";
    cout << "      VIOLET TRADING SYSTEM v1.0        \n";
    cout << "========================================\n";
    Database db("violet.db");
    db.initTables();
    Engine engine(db);
    long long current_user_id = -1;
    string username, password;
    while (current_user_id == -1) {
        cout << "\n[1] Login\n[2] Register\n[3] Exit\n> Choose option: ";
        int choice; if (!(cin >> choice)) { clearInput(); continue; }
        if (choice == 1) {
            cout << "User: "; cin >> username; cout << "Pass: "; cin >> password;
            current_user_id = db.loginUser(username, password);
        } else if (choice == 2) {
            cout << "New User: "; cin >> username; cout << "New Pass: "; cin >> password;
            db.registerUser(username, password);
        } else if (choice == 3) return 0;
    }

    srand(time(0));
    int bot_markets[5] = {1, 2, 3, 4, 5};
    for (int i = 0; i < 5; i++) { int r = rand() % 5; int t = bot_markets[i]; bot_markets[i] = bot_markets[r]; bot_markets[r] = t; }
    
    int freeze_counter = showDailyIntel(bot_markets); 
    int tick_counter = 0;

    while (true) {
        // --- NEW: SPEED DUEL SHUFFLE & INTEL REFRESH ---
        if (tick_counter > 0 && tick_counter % 20 == 0) {
            cout << "\n==========================================";
            cout << "\n[!] ALARM: THE BOTS ARE SHIFTING POSITIONS!";
            
            // Shuffle the bot_markets array again
            for (int i = 0; i < 5; i++) { 
                int r = rand() % 5; 
                int t = bot_markets[i]; 
                bot_markets[i] = bot_markets[r]; 
                bot_markets[r] = t; 
            }
            
            // Pull a fresh Intel Card
            freeze_counter = showDailyIntel(bot_markets); 
            cout << "==========================================\n";
        }
        
        if (freeze_counter > 0) {
            cout << "\n[STATUS] MARKET IS FROZEN (" << freeze_counter << " turns remaining)\n";
            freeze_counter--;
        } else {
            // --- SPEED DUEL: 5X BURST ENGINE ---
            for (int i = 0; i < 5; i++) {
                bool shadowRealm = (rand() % 15 == 0); // Increased to ~7% for more chaos
                if (shadowRealm && i == 0) { // Only print the warning once per burst
                    cout << "\n!!! [TRAP CARD] SHADOW REALM COLLAPSE: FLASH CRASH !!!\n";
                }
                runMarketSimulation(engine, db, tick_counter++, bot_markets, shadowRealm);
            }
        }

        double cash = db.getAvailableCash(current_user_id);
        string currentRank = getTraderRank(cash);
        db.updateHighScore(username, cash, currentRank); 

        if (cash >= 1000000.0) { cout << "\n$$$ VICTORY: YOU ARE THE DUEL KING $$$\n"; break; }
        else if (cash <= 0.0) { cout << "\n!!! BANKRUPTCY: SENT TO THE SHADOW REALM !!!\n"; break; }

        cout << "\n--- [" << currentRank << "] ---\n";
        cout << "CAPITAL: $" << cash << "\n";
        
        // --- NEW: INVENTORY DISPLAY ---
        cout << "INVENTORY: ";
        bool has_shares = false;
        auto current_markets = db.getAvailableMarkets();
        for (const auto& m : current_markets) {
            double qty = db.getAvailablePosition(current_user_id, m.first);
            if (qty > 0) {
                cout << "[" << m.second << ": " << qty << "]  ";
                has_shares = true;
            }
        }
        if (!has_shares) cout << "Empty";
        cout << "\n------------------------------\n";
        
        // Updated Menu with the [6] WAIT option
        cout << "[1] SUMMON (Buy)  [2] TRIBUTE (Sell)  [3] VIEW FIELD  [4] LEADERBOARD  [5] FORFEIT  [6] WAIT\n> ";
        int option; if (!(cin >> option)) { clearInput(); continue; }
        
        if (option == 1 || option == 2) {
            auto markets = db.getAvailableMarkets();
            cout << "\n--- SELECT TARGET ---\n";
            for (const auto& m : markets) cout << "[" << m.first << "] " << m.second << "  ";
            cout << "\nID: "; int mid; cin >> mid;
            string s_name = "Market #" + to_string(mid);
            for (const auto& m : markets) if (m.first == mid) s_name = m.second;

            string action = (option == 1) ? "SUMMON" : "TRIBUTE";
            cout << "\n--- INITIATING " << action << " SEQUENCE for " << s_name << " ---\n";
            double p, q; 
            cout << ">> ENERGY LEVEL (Price): $"; cin >> p; 
            cout << ">> SUMMON POWER (Qty): "; cin >> q;

            // 1. Math Safety Check
            if (p <= 0 || q <= 0 || (p * q) > 10000000.0) {
                cout << "[!] EXCHANGE REJECTED: Invalid amounts or exceeds $10M limit.\n";
                continue; 
            }

            // 2. Portfolio Anti-Cheat Check
            if (option == 1) { // Buying
                double true_cash = db.getTrueAvailableCash(current_user_id);
                if ((p * q) > true_cash) {
                    cout << "[!] INSUFFICIENT FUNDS: You need $" << (p * q) << " but only have $" << true_cash << " available.\n";
                    continue; // Rejects the order
                }
            } else if (option == 2) { // Selling
                double avail_qty = db.getAvailablePosition(current_user_id, mid);
                if (q > avail_qty) {
                    cout << "[!] INSUFFICIENT MONSTERS: You only have " << avail_qty << " available shares of " << s_name << " to tribute.\n";
                    continue; // Rejects the order
                }
            }

            // If it passes the checks, place the order!
            Order o; o.user_id = current_user_id; o.market_id = mid; o.side = (option == 1) ? "buy" : "sell";
            o.type = "limit"; o.price = p * 10000; o.qty = q * 10000; o.qty_remaining = o.qty; o.status = "open";
            engine.placeOrder(o);
            cout << "-> Action logged for " << s_name << "!\n";

        } else if (option == 3) {
            auto markets = db.getAvailableMarkets();
            for (const auto& m : markets) cout << "[" << m.first << "] " << m.second << "  ";
            cout << "\nView Field ID: "; int vid; cin >> vid;
            string s_name = "Market #" + to_string(vid);
            for (const auto& m : markets) if (m.first == vid) s_name = m.second;
            
            auto buys = db.getTopBuyOrders(vid, 5); auto sells = db.getTopSellOrders(vid, 5);

            cout << "\n==== BATTLE FIELD: " << s_name << " ====\n";
            cout << " [ENEMY MONSTERS - Buy from these] \n";
            for (int i = (int)sells.size()-1; i>=0; i--) {
                cout << "  <-- ENERGY: $" << sells[i].first << " [POWER: " << sells[i].second << "]\n";
            }
            cout << "  ---------- NO MAN'S LAND ----------\n";
            for (const auto& b : buys) {
                cout << "  --> ENERGY: $" << b.first << " [POWER: " << b.second << "]\n";
            }
            cout << " [YOUR ALLY GUARDS - Sell to these] \n";
            cout << "====================================\n";


        } else if (option == 4) {
            db.showLeaderboard();
        } else if (option == 5) {
            cout << "\n=== DANGER ZONE ===\n";
            cout << "Are you sure you want to DELETE YOUR ACCOUNT?\n";
            cout << "This will wipe your cash, erase your Leaderboard score, and free your username. (y/n): ";
            char confirm;
            cin >> confirm;
            
            if (confirm == 'y' || confirm == 'Y') {
                db.deleteUserAccount(current_user_id, username);
                cout << "-> Sent to the Shadow Realm. Restart the game to register again!\n";
                break; 
            } else {
                cout << "-> Account deletion canceled. The duel continues!\n";
            }
        } else if (option == 6) {
            cout << "-> You hold your position and watch the market move...\n";
            // The loop will naturally restart, advancing the tick_counter and updating the bots!
        }
        
    }
    return 0;
}

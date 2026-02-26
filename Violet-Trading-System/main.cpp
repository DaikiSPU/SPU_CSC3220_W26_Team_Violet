//
//  main.cpp
//  Violet-Trading-System
//

#include <iostream>
#include <string>
#include <limits>
#include "Database/Database.h"
#include "Engine/Engine.h"

using namespace std;

// Helper to clear bad input (e.g., typing letters instead of numbers)
void clearInput() {
    cin.clear();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

int main() {
    cout << "========================================\n";
    cout << "      VIOLET TRADING SYSTEM v1.0        \n";
    cout << "========================================\n";
    
    // 1. Initialize Backend
    Database db("violet.db");
    db.initTables();
    Engine engine(db);
    
    long long current_user_id = -1;
    string username, password;

    // 2. The Login Loop
    while (current_user_id == -1) {
        cout << "\n[1] Login\n[2] Register\n[3] Exit\n> Choose an option: ";
        int choice;
        if (!(cin >> choice)) { clearInput(); continue; }

        if (choice == 1) {
            cout << "Username: "; cin >> username;
            cout << "Password: "; cin >> password;
            current_user_id = db.loginUser(username, password);
        } else if (choice == 2) {
            cout << "New Username: "; cin >> username;
            cout << "New Password: "; cin >> password;
            db.registerUser(username, password);
        } else if (choice == 3) {
            cout << "Shutting down system...\n";
            return 0;
        } else {
            cout << "[ERROR] Invalid choice.\n";
        }
    }

// 3. The Main Trading Loop
    while (true) {
        cout << "\n--- TRADING MENU ---\n";
        cout << "[1] Place BUY Order\n";
        cout << "[2] Place SELL Order\n";
        cout << "[3] Log Out & Exit\n";
        cout << "> Choose an option: ";
        
        int option;
        if (!(cin >> option)) { clearInput(); continue; }

        if (option == 1 || option == 2) {
            
            // 1. Fetch the markets dynamically from SQLite
            auto available_markets = db.getAvailableMarkets();
            
            if (available_markets.empty()) {
                cout << "[SYSTEM] No markets are currently open for trading.\n";
                continue;
            }

            // 2. Print them to the screen
            cout << "\n--- AVAILABLE MARKETS ---\n";
            for (const auto& m : available_markets) {
                cout << "[" << m.first << "] " << m.second << "\n";
            }
            cout << "> Enter the Market ID you want to trade: ";
            
            int selected_market;
            if (!(cin >> selected_market)) { 
                cout << "[ERROR] Invalid input.\n"; 
                clearInput(); 
                continue; 
            }

            // 3. CREATE AND SEND THE ORDER
            Order o;
            o.user_id = current_user_id;  
            o.market_id = selected_market; 
            o.side = (option == 1) ? "buy" : "sell"; 
            o.type = "limit";             
            o.status = "open";            

            double input_price, input_qty;
            
            cout << "Enter Price (e.g., 500.50): $";
            if (!(cin >> input_price)) { cout << "[ERROR] Invalid price.\n"; clearInput(); continue; }
            
            cout << "Enter Quantity (e.g., 10.5): ";
            if (!(cin >> input_qty)) { cout << "[ERROR] Invalid quantity.\n"; clearInput(); continue; }
            
            // --- VALIDATION ---
            if (input_price <= 0 || input_qty <= 0) {
                cout << "[ERROR] Price and Quantity must be strictly greater than zero.\n";
                continue;
            }

            // MULTIPLIER CONVERSION: Human Decimals -> Engine Integers
            o.price = static_cast<long long>(input_price * 10000); 
            o.qty = static_cast<long long>(input_qty * 10000);     
            o.qty_remaining = o.qty;                               

            // Send to backend
            engine.placeOrder(o);
            cout << "-> Successfully sent " << o.side << " order for Market #" << o.market_id << " to the Matching Engine.\n";

        } else if (option == 3) { // This must be OUTSIDE the `if (option == 1 || 2)` block
            cout << "Logging out...\n";
            break;
        } else {
            cout << "[ERROR] Invalid option.\n";
        }
    }

    return 0;
    
}

//
//  Engine.cpp
//  Violet-Trading-System
//
//  Created by Nam Nguyen on 2/13/26.
//

#include "Engine.h"

Engine::Engine(Database& database) : db(database) {
    std::cout << "Matching Engine Initialized." << std::endl;
}

void Engine::placeOrder(Order new_order) {
    // 1. SAVE to DB first (Persistence)
    // This ensures we have an ID before we put it in memory
    long long db_id = db.addOrder(new_order);
    new_order.order_id = db_id;

    // 2. Add to Memory (Hot Path)
    if (new_order.side == "buy") {
        buy_orders.push(new_order);
        std::cout << "[Engine] Buy Order #" << db_id << " pushed to book." << std::endl;
    } else {
        sell_orders.push(new_order);
        std::cout << "[Engine] Sell Order #" << db_id << " pushed to book." << std::endl;
    }

    // 3. Attempt to match
    match();
}

void Engine::match() {
    while (!buy_orders.empty() && !sell_orders.empty()) {
        Order best_buy = buy_orders.top();
        Order best_sell = sell_orders.top();

        // Check for Price Cross
        if (best_buy.price >= best_sell.price) {
            
            // EXECUTE TRADE
            Trade t;
            t.market_id = best_buy.market_id;
            t.buy_order_id = best_buy.order_id;
            t.sell_order_id = best_sell.order_id;
            t.price = best_sell.price; // Trade happens at the older price (Maker's price)
            t.qty = std::min(best_buy.qty, best_sell.qty); // Trade the smaller amount
            
            // Save to DB
            db.recordTrade(t);

            // Pop them both for now (Simple version)
            buy_orders.pop();
            sell_orders.pop();
            
        } else {
            break; // No more matches possible
        }
    }
}

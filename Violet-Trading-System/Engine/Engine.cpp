//
//  Engine.cpp
//  Violet-Trading-System
//
//  Created by Nam Nguyen on 2/13/26.
//

#include "Engine.h"
#include <algorithm>

Engine::Engine(Database& database) : db(database) {
    std::cout << "Matching Engine Initialized." << std::endl;
}

void Engine::placeOrder(Order new_order) {
    long long db_id = db.addOrder(new_order);
    new_order.order_id = db_id;

    // Push to the specific market's order book
    if (new_order.side == "buy") {
        buy_books[new_order.market_id].push(new_order);
        std::cout << "[Engine] Buy Order #" << db_id << " placed in Market " << new_order.market_id << std::endl;
    } else {
        sell_books[new_order.market_id].push(new_order);
        std::cout << "[Engine] Sell Order #" << db_id << " placed in Market " << new_order.market_id << std::endl;
    }

    // Attempt to match only the market that just got liquidity
    match(new_order.market_id);
}

void Engine::match(int market_id) {
    // Grab the specific queues for this market
    auto& buys = buy_books[market_id];
    auto& sells = sell_books[market_id];

    while (!buys.empty() && !sells.empty()) {
        Order best_buy = buys.top();
        Order best_sell = sells.top();

    if (best_buy.price >= best_sell.price) {
            
            // Calculate exact partial fill quantity
            long long trade_qty = std::min(best_buy.qty_remaining, best_sell.qty_remaining);
            
            Trade t;
            t.market_id = market_id;
            t.buy_order_id = best_buy.order_id;
            t.sell_order_id = best_sell.order_id;
            t.price = best_sell.price; 
            t.qty = trade_qty; 
            
            db.recordTrade(t);

            // Pop them out temporarily
            buys.pop();
            sells.pop();
            
            // Deduct the traded shares
            best_buy.qty_remaining -= trade_qty;
            best_sell.qty_remaining -= trade_qty;
            
            // Determine new statuses for the Database
            std::string buy_status = (best_buy.qty_remaining == 0) ? "filled" : "partial";
            std::string sell_status = (best_sell.qty_remaining == 0) ? "filled" : "partial";

            // SYNCHRONIZE WITH DATABASE
            db.updateOrder(best_buy.order_id, best_buy.qty_remaining, buy_status);
            db.updateOrder(best_sell.order_id, best_sell.qty_remaining, sell_status);
            
            // If they still have shares left over, put them back in the queue!
            if (best_buy.qty_remaining > 0) buys.push(best_buy);
            if (best_sell.qty_remaining > 0) sells.push(best_sell);
            
    } else {
            break; // No cross, stop matching
        }
    }
}

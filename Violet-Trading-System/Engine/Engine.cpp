//
//  Engine.cpp
//  Violet-Trading-System
//

#include "Engine.h"
#include <algorithm>

Engine::Engine(Database& database) : db(database) {
    std::cout << "Matching Engine Initialized." << std::endl;
}

void Engine::placeOrder(Order new_order) {
    long long db_id = db.addOrder(new_order);
    new_order.order_id = db_id;

    if (new_order.side == "buy") {
        buy_books[new_order.market_id].push(new_order);
    } else {
        sell_books[new_order.market_id].push(new_order);
    }

    match(new_order.market_id);
}

void Engine::match(int market_id) {
    auto& buys = buy_books[market_id];
    auto& sells = sell_books[market_id];

    while (!buys.empty() && !sells.empty()) {
        Order best_buy = buys.top();
        Order best_sell = sells.top();

        if (best_buy.price >= best_sell.price) {
            
            long long trade_qty = std::min(best_buy.qty_remaining, best_sell.qty_remaining);
            
            Trade t;
            t.market_id = market_id;
            t.buy_order_id = best_buy.order_id;
            t.sell_order_id = best_sell.order_id;
            t.price = best_sell.price; 
            t.qty = trade_qty; 
            
            db.recordTrade(t);

            buys.pop();
            sells.pop();
            
            best_buy.qty_remaining -= trade_qty;
            best_sell.qty_remaining -= trade_qty;
            
            std::string buy_status = (best_buy.qty_remaining == 0) ? "filled" : "partial";
            std::string sell_status = (best_sell.qty_remaining == 0) ? "filled" : "partial";

            db.updateOrder(best_buy.order_id, best_buy.qty_remaining, buy_status);
            db.updateOrder(best_sell.order_id, best_sell.qty_remaining, sell_status);
            
            if (best_buy.qty_remaining > 0) buys.push(best_buy);
            if (best_sell.qty_remaining > 0) sells.push(best_sell);
            
        } else {
            break; 
        }
    }
}

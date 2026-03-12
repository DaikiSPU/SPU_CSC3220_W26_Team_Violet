=================================================
TEAM VIOLET - THE DUELIST'S TRADING SIMULATOR
CSC 3220 - Applications Programming
=================================================

Hi Professor,

Welcome to the Violet Trading System! Under the hood, this is a strict, high-performance C++ Limit Order Book (LOB) matching engine. But to make your grading experience actually fun, we gamified the terminal into a retro, high-stakes strategy game.

HOW TO COMPILE (2 Easy Steps):
You do NOT need to install external SQLite libraries.
1. gcc -c sqlite3.c -o sqlite3.o
2. g++ -std=c++17 main.cpp Database/Database.cpp Engine/Engine.cpp sqlite3.o -I. -IDatabase -IEngine -I/Users/YourNameHERE/SPU_CSC3220_W26_Team_Violet/core -o violet_engine

Run: ./violet_engine (Mac/Linux) or violet_engine.exe (Windows)

=================================================
TUTORIAL: YOUR FIRST 5 MINUTES (START HERE)
=================================================
Terminal games can be confusing. Follow these exact steps to make your first successful trade and see our C++ engine in action!

STEP 1: THE LOGIN
Press [2] and type a username and password to Register. You will instantly be given $10,000 in starting capital.

STEP 2: READ THE FIELD
Press [3] to VIEW FIELD and pick a market (like [2] for BTC). Your terminal will display a screen that looks like this:

==== BATTLE FIELD: BTC ====
 [ENEMY MONSTERS] (The Top Half - Bots selling shares)
  <-- ENERGY: $103 [POWER: 8]
  <-- ENERGY: $102 [POWER: 40]  <-- CHEAPEST WAY TO BUY
  ---------- NO MAN'S LAND ---------- (The Spread)
  --> ENERGY: $101 [POWER: 11]  <-- BEST PLACE TO SELL
  --> ENERGY: $98  [POWER: 40] 
 [YOUR ALLY GUARDS] (The Bottom Half - Bots buying shares)
====================================

STEP 3: MAKE A TRADE
To make money, you want to buy from the lowest Enemy Monster, and sell to the highest Ally Guard.
1. Press [1] to SUMMON (Buy). 
2. When it asks for "ENERGY LEVEL (Price)", type the lowest price sitting right above NO MAN'S LAND (in the example above, you would type 102).
3. When it asks for "SUMMON POWER (Qty)", type a small number like 10.
Boom! You just executed a trade against a live bot, and the database has automatically settled your cash. 

=================================================
THE RULES & THE "OVERSHOOT" LEADERBOARD
=================================================
Your goal is to day-trade your $10,000 up to $1,000,000. If your capital hits $0, you go bankrupt and lose. 

THE "OVERSHOOT" RULE (How High Scores Work):
The game ends the exact moment your cash crosses $1,000,000. However, our SQLite Leaderboard ranks players by their highest peak cash. 

To get the #1 High Score, your goal is to "Overshoot" the target. Don't just inch past $1,000,000—set up a massive final trade that catapults you to $2,000,000+ in a single turn! Press [4] on the main menu to see the Leaderboard.

=================================================
THE BOSS MONSTERS (CHEAT SHEET)
=================================================
The 5 markets (SPU, BTC, AAPL, TSLA, NVDA) are haunted by specific bot algorithms. Here is exactly how to exploit them for maximum profit:

1. The Whale (Deep-Sea Leviathan)

The Look: A massive, immovable Buy order of 10,000,000 units ($1,000.00) in the "Ally Guards" section.

The Outplay: This is a Price Floor. Because the Whale has so much "Power," the price cannot drop below it.

The Combo: SUMMON shares at $0.01 above the Whale. Since your risk is capped by the Whale's massive order, you can hold these safely until the price ticks up, then flip them for a risk-free gain.

2. The Greed Bot (FOMO Dragon)

The Look: It instantly outbids your highest buy order by exactly $0.10 in the "Speed Duel" version.

The Outplay: This bot is a Price Pusher. It experiences "Fear Of Missing Out" and will chase any price you set.

The Combo: Place "Fake" buy orders for 1 share. Each turn you WAIT, the Dragon will outbid you, pushing the market price higher. Once the price is high enough, TRIBUTE your real inventory to the Dragon at the inflated premium.
+2

3. The Sine Wave (Clockwork Golem)

The Look: The price moves in a perfect, predictable curve based on the turn counter (tick_counter).

The Outplay: This is a Mean Reversion bot. It oscillates between a fixed high and low.

The Combo: Use [6] WAIT to watch the cycle. Do not buy on the way down; wait until the price hits the absolute "trough" of the wave. Buy the bottom, then wait for the "crest" to sell.

4. The Fat Finger (Chaos Sorcerer)

The Look: Acts normal 95% of the time, then suddenly places a Buy order at 10x the market price.

The Outplay: This bot simulates human error. It is looking to buy shares at any cost.

The Combo: Always leave a "Trap" Sell order deep in the Enemy Monsters section at a ridiculous price (e.g., $1,000.00). Eventually, the Sorcerer will "Fat Finger" the trade and buy your shares for a 1000% profit.
+1

5. The Panic Seller (Spooked Spirit)

The Look: Suddenly dumps massive volume, crashing the price by $15.00+ instantly.

The Outplay: This bot triggers during the Shadow Realm Collapse.

The Combo: When the alert flashes, DO NOT PANIC. The market is not broken; it is on sale. Use [1] SUMMON to buy as much as possible at the $85.00 crash price. When the Spirit stops dumping, the other bots will restore the price to $100.00, giving you an instant 15% gain.
--- TRAP CARDS (SPECIAL EVENTS) ---
* Shadow Realm Collapse: 4% chance every turn of a market-wide Flash Crash. Do not panic—buy the heavily discounted shares and sell when the bots recover!
* Swords of Revealing Light: Your Daily Intel has a 1% chance to pull this card, freezing the engine for 3 turns. Use this pause to safely place massive trades while the bots are paralyzed.

Enjoy the duel!

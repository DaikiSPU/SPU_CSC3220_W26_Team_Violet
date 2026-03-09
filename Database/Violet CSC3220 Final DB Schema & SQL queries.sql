-- Better visuals for terminal uses
.mode box
.headers on

-- Database Schema (DDL)
PRAGMA foreign_keys = ON;
-- Create Traders Table (Parent)
CREATE TABLE Traders (
trader_id INTEGER PRIMARY KEY AUTOINCREMENT,
username TEXT NOT NULL UNIQUE,
cash_balance NUMERIC NOT NULL DEFAULT 0.00
CHECK (cash_balance >= 0)
);
-- Create OrderBook Table (Child)
CREATE TABLE OrderBook (
order_id INTEGER PRIMARY KEY AUTOINCREMENT,
trader_id INTEGER NOT NULL,
symbol TEXT NOT NULL,
side TEXT NOT NULL
CHECK (side IN ('BUY', 'SELL')),
quantity INTEGER NOT NULL
CHECK (quantity >= 0),
price NUMERIC NOT NULL
CHECK (price > 0),
status TEXT NOT NULL DEFAULT 'OPEN'
CHECK (status IN ('OPEN', 'FILLED', 'CANCELLED')),
FOREIGN KEY (trader_id)
REFERENCES Traders(trader_id)
);

-- CRUD Operations (DML)
-- Create
-- 1. A formal transaction
BEGIN TRANSACTION;
INSERT INTO Traders (username, cash_balance)
VALUES ('HFT_Whale_03', 5000000.00);
INSERT INTO OrderBook (trader_id, symbol, side, quantity, price)
VALUES (last_insert_rowid(), 'SPU', 'BUY', 5000, 49.50);
COMMIT;
-- 2. The Setup: Adding a second trader and a SELL order
INSERT INTO Traders (username, cash_balance)
VALUES ('SPU_Alpha_Trader', 50000.00);
INSERT INTO OrderBook (trader_id, symbol, side, quantity, price)
VALUES (last_insert_rowid(), 'SPU', 'SELL', 5000, 49.50);

-- Read
-- Retrieves open orders alongside the trader's username and cash balance
SELECT Traders.username, Traders.cash_balance, OrderBook.side, OrderBook.quantity, OrderBook.price
From OrderBook
JOIN Traders ON OrderBook.trader_id = Traders.trader_id
WHERE OrderBook.side = 'BUY' AND OrderBook.status = 'OPEN';

-- Update
-- Transaction: Simulates matching two open orders (order 1 and order 2)
BEGIN TRANSACTION;
-- 1. Update the OrderBook; Set at least two columns (status and quantity)
UPDATE OrderBook
SET status = 'FILLED', quantity = 0
WHERE order_id IN (1, 2);
-- 2. Update the related Traders table
UPDATE Traders
SET cash_balance = cash_balance - 247500.00
WHERE trader_id = 1;
-- Credit the seller
UPDATE Traders
SET cash_balance = cash_balance + 247500.00
WHERE trader_id = 2;
COMMIT;
-- Retrieve all orders again to confirm changes
SELECT * FROM OrderBook;
SELECT * FROM Traders;

-- Delete
-- Transaction: Completely remove a trader and all of their active orders from the exchange
BEGIN TRANSACTION;
-- 1. Delete the related child records first to prevent a Foreign Key violation
DELETE FROM OrderBook
WHERE trader_id = 2;
-- 2. Delete the parent record from the Traders table
DELETE FROM Traders
WHERE trader_id = 2;

COMMIT;
-- Verify the deletion was successful
SELECT * FROM Traders;
SELECT * FROM OrderBook;


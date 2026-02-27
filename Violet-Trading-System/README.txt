=================================================
TEAM VIOLET - LIMIT ORDER BOOK SIMULATION
CSC 3220 - Applications Programming
=================================================

Hi Professor,

To make grading as easy as possible across different operating systems (Windows/Mac/Linux), we have included the SQLite amalgamation source files (sqlite3.c and sqlite3.h) directly in this directory. 

You do NOT need to install any external SQLite libraries or configure linker paths to grade this project.

HOW TO COMPILE:
Please open your terminal in this root directory and run the following command:

g++ -std=c++17 main.cpp Database/Database.cpp Engine/Engine.cpp sqlite3.c -o violet_engine

HOW TO RUN:
Mac/Linux:  ./violet_engine
Windows:    violet_engine.exe

FEATURES INCLUDED:
- Strict Data Constraints (NUMERIC/TIMESTAMPTZ)
- Fixed-Point Integer Math to prevent float rounding errors
- API endpoints prepared for UI integration (Order Book, Cash Balances, History)
- In-memory matching engine with separated market queues
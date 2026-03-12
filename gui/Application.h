#pragma once

#include "UIContext.h"
#include "BackendContext.h"
#include "PageManager.h"
#include "MainGui.h"
#include "BotManager.h"
#include "Engine.h"

#include <chrono>

class Application {
    public:
        Application() : db("violet.db"), errorManager(), backendContext{ db, appData, engine }, 
            uiContext{ errorManager }, pageManager(uiContext, backendContext), engine(db) {}
        void init();
        void run();
        void update();
    private:
        // Manages current page and handles page transitions.
        Database db;
        AppData appData;
        ErrorManager errorManager;
        UIContext uiContext;
        BackendContext backendContext;
        PageManager pageManager;
        MainGui gui;
        BotManager botManager;
        Engine engine;

        std::chrono::steady_clock::time_point lastTime = std::chrono::steady_clock::now();
        const double tickInterval = 0.1; // seconds (10 ticks per second)
        double accumulator = 0.0;

        int tick = 0;
        bool simulationInitialized = false;

        bool isRunning = true;
        Engine& getEngine() { return engine; }
        Database& getDatabase() { return db; }
};
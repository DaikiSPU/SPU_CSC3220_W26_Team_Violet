#pragma once
#include "Database.h"
#include "AppData.h"
#include "Engine.h"

struct BackendContext {
    Database& db;
    AppData& appData;
    Engine& engine;
};
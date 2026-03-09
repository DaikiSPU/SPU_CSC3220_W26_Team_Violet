#include "Application.h"

void Application::init()
{
    // User exists in db?
    auto result = db.hasAnyUser();
    if (result.value)
    {
        pageManager.setPage(PageType::Login);
    }
    else
    {
        pageManager.setPage(PageType::CreateAccount);
    }
    // pageManager.setPage(PageType::Dashboard);
}

void Application::run() 
{
    gui.run(pageManager, *this);
}

void Application::update()
{
    auto now = std::chrono::steady_clock::now();

    double deltaTime =
        std::chrono::duration<double>(now - lastTime).count();

    lastTime = now;

    accumulator += deltaTime;

    PageType currentPageType = pageManager.getCurrentPageType();

    if (currentPageType == PageType::Dashboard)
    {
        if (!simulationInitialized)
        {
            auto botResult = botManager.createBots(db, engine);

            if (!botResult.isSuccess())
            {
                printf("error occurred\n");
                errorManager.addError(botResult);
            }
            simulationInitialized = true;
        }

        // catch up ticks
        int maxTicks = 5;

        while (accumulator >= tickInterval && maxTicks--)
        {
            tick++;
            botManager.runAll(tick);
            accumulator -= tickInterval;
        }
    }
}
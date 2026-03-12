#include "PageManager.h"
#include "CreateAccountPage.h"
#include "LoginPage.h"
#include "Dashboard.h"

void PageManager::setPage(PageType type)
{
    currentPageType = type;
    switch (type)
    {   
        case PageType::CreateAccount:
            currentPage = std::make_unique<CreateAccountPage>(uiContext, backendContext);
            break;

        case PageType::Login:
            currentPage = std::make_unique<LoginPage>(uiContext, backendContext);
            break;

        case PageType::Dashboard:
            currentPage = std::make_unique<Dashboard>(uiContext, backendContext);
            break;

        case PageType::None:
            break;
    }
}

void PageManager::draw()
{
    PageType next = PageType::None;
    if (currentPage) 
    {
        next = currentPage->draw();
    }

    if (next != PageType::None) 
    {
        setPage(next);
    }
}

void PageManager::sendErrorPage(AppError error)
{
    currentPage->setErrorMsg(error);
}

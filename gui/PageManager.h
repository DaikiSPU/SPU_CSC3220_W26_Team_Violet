#pragma once
#include <memory>
#include "Page.h"
#include "PageType.h"

class PageManager {
public:
    PageManager(UIContext& uiContext, BackendContext& backendContext) : uiContext(uiContext), backendContext(backendContext) {}
    void setPage(PageType type);
    void draw();
    const PageType getCurrentPageType() { return currentPageType; }
    void sendErrorPage(AppError error);

    Page* getCurrentPage() const { return currentPage.get(); }
private:
    std::unique_ptr<Page> currentPage;
    PageType currentPageType = PageType::None;
    UIContext& uiContext;
    BackendContext& backendContext;
};
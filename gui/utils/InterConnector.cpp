#include "stdafx.h"

#include "../main_window/MainWindow.h"
#include "../main_window/MainPage.h"
#include "../main_window/contact_list/ContactListModel.h"
#ifdef __APPLE__
#include "../utils/macos/mac_support.h"
#endif //__APPLE__

#include "InterConnector.h"

namespace Utils
{
    InterConnector& InterConnector::instance()
    {
        static InterConnector instance = InterConnector();
        return instance;
    }

    InterConnector::InterConnector()
        : MainWindow_(nullptr)
        , dragOverlay_(false)
    {
        //
    }

    InterConnector::~InterConnector()
    {
        //
    }

    void InterConnector::setMainWindow(Ui::MainWindow* window)
    {
        MainWindow_ = window;
    }

    Ui::MainWindow* InterConnector::getMainWindow() const
    {
        return MainWindow_;
    }

    Ui::HistoryControlPage* InterConnector::getHistoryPage(const QString& aimId) const
    {
        if (MainWindow_)
        {
            return MainWindow_->getHistoryPage(aimId);
        }

        return nullptr;
    }

    Ui::ContactDialog* InterConnector::getContactDialog() const
    {
        if (!MainWindow_)
        {
            return nullptr;
        }

        auto mainPage = MainWindow_->getMainPage();
        if (!mainPage)
        {
            return nullptr;
        }

        return mainPage->getContactDialog();
    }

    void InterConnector::insertTopWidget(const QString& aimId, QWidget* widget)
    {
        if (MainWindow_)
            MainWindow_->insertTopWidget(aimId, widget);
    }

    void InterConnector::removeTopWidget(const QString& aimId)
    {
        if (MainWindow_)
            MainWindow_->removeTopWidget(aimId);
    }


    void InterConnector::showSidebar(const QString& aimId, int page)
    {
        if (MainWindow_)
            MainWindow_->showSidebar(aimId, page);
    }

    void InterConnector::setSidebarVisible(bool show)
    {
        if (MainWindow_)
            MainWindow_->setSidebarVisible(show);
    }

    bool InterConnector::isSidebarVisible() const
    {
        if (MainWindow_)
            return MainWindow_->isSidebarVisible();

        return false;
    }

    void InterConnector::restoreSidebar()
    {
        if (MainWindow_)
            MainWindow_->restoreSidebar();
    }

    void InterConnector::setDragOverlay(bool enable)
    {
        dragOverlay_ = enable;
    }

    bool InterConnector::isDragOverlay() const
    {
        return dragOverlay_;
    }

    void InterConnector::setFocusOnInput()
    {
        if (MainWindow_)
            MainWindow_->setFocusOnInput();
    }

    void InterConnector::onSendMessage(const QString& contact)
    {
        if (MainWindow_)
            MainWindow_->onSendMessage(contact);
    }
}

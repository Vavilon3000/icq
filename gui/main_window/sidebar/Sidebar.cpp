#include "stdafx.h"
#include "Sidebar.h"
#include "MenuPage.h"
#include "ProfilePage.h"
#include "../contact_list/ContactListModel.h"
#include "../../controls/SemitransparentWindowAnimated.h"
#include "../../utils/InterConnector.h"
#include "../../utils/utils.h"

namespace
{
    const int sidebar_default_width = 320;
    const int sidebar_max_width = 428;

    const int ANIMATION_DURATION = 200;
    const int max_step = 200;
    const int min_step = 0;
}

namespace Ui
{
    Sidebar::Sidebar(QWidget* _parent, bool _isEmbedded)
        : QStackedWidget(_parent)
        , semiWindow_(new SemitransparentWindowAnimated(_parent, ANIMATION_DURATION))
        , animSidebar_(new QPropertyAnimation(this, QByteArrayLiteral("anim"), this))
        , anim_(min_step)
    {
        semiWindow_->hide();

        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        setAttribute(Qt::WA_NoMousePropagation);

        animSidebar_->setDuration(ANIMATION_DURATION);

        pages_.insert(menu_page, new MenuPage(this));
        pages_.insert(profile_page, new ProfilePage(this));

        for (auto page = pages_.cbegin(), end = pages_.cend(); page != end; ++page)
            insertWidget(page.key(), page.value());

        auto width = Utils::scale_value(_isEmbedded? sidebar_max_width: sidebar_default_width);
        setFixedWidth(width);
        setSidebarWidth(width);

        if (!_isEmbedded)
        {
            setParent(semiWindow_);
            connect(&Utils::InterConnector::instance(), &Utils::InterConnector::closeAnySemitransparentWindow, this, &Sidebar::hideAnimated);
            connect(animSidebar_, &QPropertyAnimation::finished, this, &Sidebar::onAnimationFinished);
        }
        connect(Logic::getContactListModel(), &Logic::ContactListModel::contact_removed, this, &Sidebar::contactRemoved, Qt::QueuedConnection);
    }

    void Sidebar::showAnimated()
    {
        updateSize();

        semiWindow_->raise();
        semiWindow_->Show();

        show();
        animSidebar_->stop();
        animSidebar_->setCurrentTime(0);
        animSidebar_->setStartValue(min_step);
        animSidebar_->setEndValue(max_step);
        animSidebar_->start();
    }

    void Sidebar::hideAnimated()
    {
        semiWindow_->Hide();

        animSidebar_->stop();
        animSidebar_->setCurrentTime(0);
        animSidebar_->setStartValue(max_step);
        animSidebar_->setEndValue(min_step);
        animSidebar_->start();
    }

    void Sidebar::updateSize()
    {
        semiWindow_->updateSize();
        move(semiWindow_->width() - width(), 0);
        setFixedHeight(semiWindow_->height());
    }

    void Sidebar::contactRemoved(QString aimId)
    {
    }

    void Sidebar::updateWidth()
    {
    }

    void Sidebar::onAnimationFinished()
    {
        if (animSidebar_->endValue() == min_step)
            hide();
    }

    void Sidebar::preparePage(const QString& aimId, SidebarPages page)
    {
        if (pages_.contains(page))
        {
            pages_[page]->initFor(aimId);
            if (isVisible() && currentIndex() == menu_page)
                pages_[page]->setPrev(currentAimId_);
            else
                pages_[page]->setPrev(QString());

            setCurrentIndex(page);
            currentAimId_ = aimId;
        }
    }

    void Sidebar::showAllMembers()
    {
        if (currentIndex() == menu_page)
            qobject_cast<MenuPage*>(currentWidget())->allMemebersClicked();
    }

    int Sidebar::currentPage() const
    {
        return currentIndex();
    }

    QString Sidebar::currentAimId() const
    {
        return currentAimId_;
    }

    void Sidebar::setSidebarWidth(int width)
    {
        for (auto page : pages_)
            page->setSidebarWidth(width);
    }

    void Sidebar::setAnim(int _val)
    {
        anim_ = _val;
        auto w = width() * (anim_ / (double)max_step);
        move(semiWindow_->width() - w, 0);
    }

    int Sidebar::getAnim() const
    {
        return anim_;
    }
}

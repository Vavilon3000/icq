#include "stdafx.h"
#include "SearchDropDown.h"
#include "SearchWidget.h"
#include "../../utils/utils.h"
#include "../../utils/InterConnector.h"
#include "../../controls/CommonStyle.h"
#include "../../controls/LabelEx.h"
#include "../../controls/CustomButton.h"
#include "../../fonts.h"
#include "../sidebar/SidebarUtils.h"
#include "../GroupChatOperations.h"
#include "../MainWindow.h"
#include "../MainPage.h"

namespace
{
    const auto menuItemHeight = 40;
    const auto menuItemLeftPadding = 22;
    const auto placeholderImgHeight = 56;
    const auto placeholderLabelHeight = 19;
}

namespace Ui
{
    SearchDropdown::SearchDropdown(QWidget* _parent, SearchWidget* _searchWidget, CustomButton* _pencil)
        : QWidget(_parent)
        , searchWidget_(_searchWidget)
        , searchPencil_(_pencil)
        , menuWidget_(new QWidget(this))
        , placeholderUseSearch_(new QWidget(this))
    {
        auto rootLayout = Utils::emptyVLayout(this);

        auto menuLayout = Utils::emptyVLayout(menuWidget_);
        const auto itemHeight = Utils::scale_value(menuItemHeight);
        auto addButton = [&itemHeight, this, menuLayout](auto& _btn, const auto& _iconName, const auto& _caption)
        {
            _btn = new ActionButton(menuWidget_, _iconName, _caption, itemHeight, 0, Utils::scale_value(menuItemLeftPadding));
            _btn->setCursor(QCursor(Qt::PointingHandCursor));
            _btn->setFixedHeight(itemHeight);
            Utils::grabTouchWidget(_btn);
            menuLayout->addWidget(_btn);
        };
        addButton(addContact_, qsl(":/resources/i_add_contact_100.png"), QT_TRANSLATE_NOOP("search", "Add contact"));
        addButton(createGroupChat_, qsl(":/resources/i_groupchat_100.png"), QT_TRANSLATE_NOOP("search", "New groupchat"));
        //addButton(createChannel_, qsl(":/resources/i_groupchat_100.png"), QT_TRANSLATE_NOOP("search", "New channel"));
        //addButton(createLiveChat_, qsl(":/resources/i_groupchat_100.png"), QT_TRANSLATE_NOOP("search", "New livechat"));
        rootLayout->addWidget(menuWidget_);

        auto phLayout = Utils::emptyVLayout(placeholderUseSearch_);
        auto useSearchImage = new QWidget(placeholderUseSearch_);
        useSearchImage->setStyleSheet("image: url(:/resources/placeholders/placeholder_search_contact_100.png); margin: 0;");
        useSearchImage->setFixedHeight(Utils::scale_value(placeholderImgHeight));
        phLayout->addWidget(useSearchImage);

        auto useSearchLabel = new LabelEx(placeholderUseSearch_);
        useSearchLabel->setColor(CommonStyle::getColor(CommonStyle::Color::TEXT_LIGHT));
        useSearchLabel->setFont(Fonts::appFontScaled(14, Fonts::FontWeight::Normal));
        useSearchLabel->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Fixed);
        useSearchLabel->setAlignment(Qt::AlignCenter);
        useSearchLabel->setText(QT_TRANSLATE_NOOP("placeholders", "Use the search to add contacts"));
        useSearchLabel->setFixedHeight(Utils::scale_value(placeholderLabelHeight));
        phLayout->addWidget(useSearchLabel);

        rootLayout->addWidget(placeholderUseSearch_);

        connect(addContact_,        &ActionButton::clicked, this, &SearchDropdown::addContact);
        connect(createGroupChat_,   &ActionButton::clicked, this, &SearchDropdown::createChat);
        //connect(createChannel_,     &ActionButton::clicked, this, &SearchDropdown::createChat);
        //connect(createLiveChat_,    &ActionButton::clicked, this, &SearchDropdown::createChat);

        connect(searchWidget_, &SearchWidget::activeChanged,this, &SearchDropdown::onSearchActiveChanged);
        connect(searchWidget_, &SearchWidget::searchBegin,  this, &SearchDropdown::hide);

        connect(searchPencil_, &CustomButton::clicked, this, &SearchDropdown::onPencilClicked);
    }

    void SearchDropdown::updatePencilIcon(const pencilIcon _pencilIcon)
    {
        switch (_pencilIcon)
        {
        case pencilIcon::pi_pencil:
            searchPencil_->setImage(qsl(":/resources/create_chat_100.png"));
            searchPencil_->setActiveImage(qsl(":/resources/create_chat_100.png"));
            searchPencil_->setHoverImage(qsl(":/resources/create_chat_100.png"));
        	break;

        case pencilIcon::pi_cross:
            searchPencil_->setImage(qsl(":/controls/close_a_100"));
            searchPencil_->setActiveImage(qsl(":/controls/close_d_100"));
            searchPencil_->setHoverImage(qsl(":/controls/close_d_100"));
            break;
        default:
            break;
        }
    }

    void SearchDropdown::hideEvent(QHideEvent * _event)
    {
        updatePencilIcon(pencilIcon::pi_pencil);
        QWidget::hideEvent(_event);
    }

    void SearchDropdown::showPlaceholder()
    {
        menuWidget_->hide();
        placeholderUseSearch_->show();

        updatePencilIcon(pencilIcon::pi_cross);
        show();
    }

    void SearchDropdown::showMenuFull()
    {
        addContact_->show();
        createGroupChat_->show();
        //createChannel_->show();
        //createLiveChat_->show();

        menuWidget_->show();
        placeholderUseSearch_->hide();

        updatePencilIcon(pencilIcon::pi_cross);
        show();
    }

    void SearchDropdown::showMenuAddContactOnly()
    {
        addContact_->show();
        createGroupChat_->hide();
        //createChannel_->hide();
        //createLiveChat_->hide();

        menuWidget_->show();
        placeholderUseSearch_->hide();

        updatePencilIcon(pencilIcon::pi_cross);
        show();
    }

    void SearchDropdown::createChat()
    {
        hide();

        Ui::createGroupChat({});

        emit Utils::InterConnector::instance().closeAnySemitransparentWindow();
    }

    void SearchDropdown::addContact()
    {
        const auto mainWindow = Utils::InterConnector::instance().getMainWindow();
        if (mainWindow)
        {
            auto mainPage = mainWindow->getMainPage();
            if (mainPage)
                mainPage->setSearchFocus();
        }

        showPlaceholder();
        searchWidget_->setPlaceholderText(QT_TRANSLATE_NOOP("search_widget", "Phone, UIN, Name, Email"));
    }

    void SearchDropdown::onSearchActiveChanged(bool _isActive)
    {
        if (_isActive && !isVisible())
            showMenuAddContactOnly();
    }

    void SearchDropdown::onPencilClicked()
    {
        if (isVisible())
        {
            hide();
        }
        else
        {
            Utils::InterConnector::instance().setSearchFocus();
            showMenuFull();
        }
    }
}

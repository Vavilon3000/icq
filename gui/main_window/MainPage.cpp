#include "stdafx.h"
#include "MainPage.h"

#include "ContactDialog.h"
#include "GroupChatOperations.h"
#include "IntroduceYourself.h"
#include "TitleBar.h"
#include "livechats/LiveChatsHome.h"
#include "livechats/LiveChatProfile.h"
#include "contact_list/ChatMembersModel.h"
#include "contact_list/Common.h"
#include "contact_list/ContactList.h"
#include "contact_list/ContactListModel.h"
#include "contact_list/ContactListWidget.h"
#include "contact_list/RecentsModel.h"
#include "contact_list/SearchWidget.h"
#include "contact_list/SearchModelDLG.h"
#include "contact_list/UnknownsModel.h"
#include "contact_list/TopPanel.h"
#include "contact_list/SelectionContactsForGroupChat.h"
#include "contact_list/SearchDropDown.h"
#include "livechats/LiveChatsModel.h"
#include "history_control/MessagesModel.h"
#include "search_contacts/SearchContactsWidget.h"
#include "settings/GeneralSettingsWidget.h"
#include "settings/SettingsTab.h"
#include "sidebar/Sidebar.h"
#include "settings/SettingsThemes.h"
#include "../core_dispatcher.h"
#include "../gui_settings.h"
#include "../my_info.h"
#include "../controls/BackButton.h"
#include "../controls/CommonStyle.h"
#include "../controls/CustomButton.h"
#include "../controls/FlatMenu.h"
#include "../controls/TextEmojiWidget.h"
#include "../controls/WidgetsNavigator.h"
#include "../controls/SemitransparentWindowAnimated.h"
#include "../controls/HorScrollableView.h"
#include "../controls/TransparentScrollBar.h"
#include "../utils/InterConnector.h"
#include "../utils/utils.h"
#include "../utils/log/log.h"
#include "../voip/IncomingCallWindow.h"
#include "../voip/VideoWindow.h"
#include "MainWindow.h"
#include "MainMenu.h"
#include "smiles_menu/Store.h"

#ifdef __APPLE__
#   include "../utils/macos/mac_support.h"
#endif

namespace
{
    const int balloon_size = 20;
    const int unreads_padding = 6;
    const int counter_padding = 4;
    const int back_spacing = 16;
    const int unreads_minimum_extent = balloon_size;
    const int min_step = 0;
    const int max_step = 100;
    const int BACK_WIDTH = 52;
    const int BACK_HEIGHT = 48;
    const int MENU_WIDTH = 240;
    const int CL_POPUP_WIDTH = 360;
    const int ANIMATION_DURATION = 200;
    const int TOP_WIDGET_HEIGHT = 56;
}

namespace Ui
{
    UnreadsCounter::UnreadsCounter(QWidget* _parent)
        : QWidget(_parent)
    {
        connect(Logic::getRecentsModel(), &Logic::RecentsModel::updated, this, static_cast<void(UnreadsCounter::*)()>(&UnreadsCounter::update), Qt::QueuedConnection);
    }

    void UnreadsCounter::paintEvent(QPaintEvent* e)
    {
        QPainter p(this);
        const auto borderColor = QColor(Qt::transparent);
        const auto bgColor = CommonStyle::getColor(CommonStyle::Color::GREEN_FILL);
        const auto textColor = QColor(ql1s("#ffffff"));
        Utils::drawUnreads(
            &p,
            Fonts::appFontScaled(13, Fonts::FontWeight::Medium),
            bgColor,
            textColor,
            borderColor,
            Logic::getRecentsModel()->totalUnreads(),
            Utils::scale_value(balloon_size), 0, 0
            );
    }

    HeaderBack::HeaderBack(QWidget* _parent)
        : QPushButton(_parent)
    {

    }

    void HeaderBack::paintEvent(QPaintEvent* e)
    {
        QPushButton::paintEvent(e);
        QPainter p(this);
        QPixmap pix(Utils::parse_image_name(qsl(":/controls/arrow_left_100")));
        Utils::check_pixel_ratio(pix);
        double ratio = Utils::scale_bitmap(1);

        p.drawPixmap(QPoint(0, height() / 2 - pix.height() / 2 / ratio), pix);
    }

    void HeaderBack::resizeEvent(QResizeEvent* e)
    {
        emit resized();
        QPushButton::resizeEvent(e);
    }

    MainPage* MainPage::_instance = nullptr;
    MainPage* MainPage::instance(QWidget* _parent)
    {
        assert(_instance || _parent);

        if (!_instance)
            _instance = new MainPage(_parent);

        return _instance;
    }

    void MainPage::reset()
    {
        if (_instance)
        {
            delete _instance;
            _instance = nullptr;
        }
    }

    void MainPage::addButtonToTop(QWidget* _button)
    {
        myTopWidget_->addWidget(_button);
    }

    QString MainPage::getMainWindowQss()
    {
        auto style = Utils::LoadStyle(qsl(":/qss/main_window"));
        return style;
    }

    MainPage::MainPage(QWidget* _parent)
        : QWidget(_parent)
        , unknownsHeader_(nullptr)
        , contactListWidget_(new ContactList(this))
        , searchWidget_(new SearchWidget(this))
        , videoWindow_(nullptr)
        , pages_(new WidgetsNavigator(this))
        , contactDialog_(new ContactDialog(this))
        , searchContacts_(nullptr)
        , generalSettings_(new GeneralSettingsWidget(this))
        , themesSettings_(new ThemesSettingsWidget(this))
        , stickersStore_(nullptr)
        , liveChatsPage_(new LiveChatHome(this))
        , noContactsYetSuggestions_(nullptr)
        , introduceYourselfSuggestions_(nullptr)
        , needShowIntroduceYourself_(false)
        , settingsTimer_(new QTimer(this))
        , recvMyInfo_(false)
        , animCLWidth_(new QPropertyAnimation(this, QByteArrayLiteral("clWidth"), this))
        , clSpacer_(new QWidget())
        , clHostLayout_(new QHBoxLayout())
        , leftPanelState_(LeftPanelState::spreaded)
        , myTopWidget_(nullptr)
        , mainMenu_(new MainMenu(this))
        , semiWindow_(new SemitransparentWindowAnimated(this, ANIMATION_DURATION))
        , headerWidget_(new QWidget(this))
        , headerLabel_(new LabelEx(this))
        , embeddedSidebar_(new Sidebar(this, true))
        , NeedShowUnknownsHeader_(false)
        , menuVisible_(false)
        , currentTab_(RECENTS)
        , anim_(min_step)
        , liveChats_(new LiveChats(this))

    {
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::showPlaceholder, this, &MainPage::showPlaceholder);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::liveChatSelected, this, &MainPage::liveChatSelected, Qt::QueuedConnection);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::compactModeChanged, this, &MainPage::compactModeChanged, Qt::QueuedConnection);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::contacts, this, &MainPage::contactsClicked, Qt::QueuedConnection);

        setStyleSheet(getMainWindowQss());

        this->setProperty("Invisible", true);
        horizontalLayout_ = Utils::emptyHLayout(this);

        originalLayout = qobject_cast<QHBoxLayout*>(layout());

        contactsWidget_ = new QWidget();
        Testing::setAccessibleName(contactsWidget_, "AS contactsWidget_");
        contactsWidget_->setObjectName(qsl("contactsWidgetStyle"));
        contactsLayout = Utils::emptyVLayout(contactsWidget_);

        myTopWidget_ = new TopPanelWidget(this, searchWidget_);
        myTopWidget_->setFixedHeight(Utils::scale_value(TOP_WIDGET_HEIGHT));
        contactsLayout->addWidget(myTopWidget_);

        //auto searchPencil = new CustomButton(this, qsl(":/resources/create_chat_100.png"));
        //searchPencil->setHoverImage(qsl(":/resources/create_chat_100.png"));
        //searchPencil->setFixedSize(Utils::scale_value(32), Utils::scale_value(32));
        //searchPencil->setCursor(Qt::PointingHandCursor);
        //searchPencil->setAlign(Qt::AlignLeft);
        //searchPencil->setOffsets(Utils::scale_value(-4), 0);
        //myTopWidget_->addWidget(searchPencil);

        //searchDropdown_ = new SearchDropdown(this, searchWidget_, searchPencil);
        //contactsLayout->addWidget(searchDropdown_);
        //searchDropdown_->hide();

        mainMenu_->setFixedWidth(Utils::scale_value(MENU_WIDTH));
        mainMenu_->hide();

        hideSemiWindow();

        connect(myTopWidget_, &Ui::TopPanelWidget::back, this, &MainPage::openRecents, Qt::QueuedConnection);
        connect(myTopWidget_, &Ui::TopPanelWidget::burgerClicked, this, &MainPage::showMainMenu, Qt::QueuedConnection);

        {
            CustomButton *back = nullptr;

            unknownsHeader_ = new QWidget(this);
            int height = ::ContactList::GetRecentsParams(Logic::MembersWidgetRegim::CONTACT_LIST).unknownsItemHeight();
            unknownsHeader_->setFixedHeight(height);
            unknownsHeader_->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);
            unknownsHeader_->setVisible(false);
            {
                int horOffset = ::ContactList::GetRecentsParams(Logic::MembersWidgetRegim::CONTACT_LIST).itemHorPadding();

                back = new CustomButton(this, qsl(":/controls/arrow_left_100"));
                back->setFixedSize(QSize(Utils::scale_value(BACK_WIDTH), Utils::scale_value(BACK_HEIGHT)));
                back->setStyleSheet(qsl("background: transparent; border-style: none;"));
                back->setCursor(Qt::PointingHandCursor);
                back->setOffsets(horOffset, 0);
                back->setAlign(Qt::AlignLeft);

                auto layout = Utils::emptyHLayout(unknownsHeader_);
                layout->setContentsMargins(0, 0, horOffset + BACK_WIDTH, 0);
                layout->addWidget(back);

                auto unknownBackButtonLabel = new LabelEx(this);
                unknownBackButtonLabel->setAlignment(Qt::AlignCenter);
                unknownBackButtonLabel->setFixedHeight(Utils::scale_value(48));
                unknownBackButtonLabel->setFont(Fonts::appFontScaled(16, Fonts::FontWeight::Medium));
                unknownBackButtonLabel->setColor(QColor(ql1s("#999999")));
                unknownBackButtonLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
                unknownBackButtonLabel->setText(QT_TRANSLATE_NOOP("contact_list", "New contacts"));

                layout->addWidget(unknownBackButtonLabel);
            }
            contactsLayout->addWidget(unknownsHeader_);
            connect(back, &QPushButton::clicked, this, [=]()
            {
                emit Utils::InterConnector::instance().unknownsGoBack();
            });
            connect(&Utils::InterConnector::instance(), &Utils::InterConnector::unknownsGoSeeThem, this, &MainPage::changeCLHeadToUnknownSlot);
            connect(&Utils::InterConnector::instance(), &Utils::InterConnector::unknownsGoBack, this, &MainPage::changeCLHeadToSearchSlot);
        }

        auto hlayout = Utils::emptyHLayout();
        searchButton_ = new Ui::CustomButton(this, qsl(":/resources/search_big_100.png"));
        searchButton_->setFixedSize(Utils::scale_value(28), Utils::scale_value(40));
        searchButton_->setStyleSheet(qsl("QPushButton:focus { border: none; outline: none; } QPushButton:flat { border: none; outline: none; }"));
        searchButton_->setCursor(QCursor(Qt::PointingHandCursor));
        hlayout->addWidget(searchButton_);
        contactsLayout->addLayout(hlayout);

        auto mailLayout = Utils::emptyHLayout();
        mailLayout->setContentsMargins(Utils::scale_value(6), 0, 0, 0);
        contactsLayout->addLayout(mailLayout);
        connect(searchButton_, &Ui::CustomButton::clicked, this, &MainPage::setSearchFocus, Qt::QueuedConnection);
        searchButton_->hide();

        contactsLayout->addWidget(contactListWidget_);

        connect(contactListWidget_, &Ui::ContactList::tabChanged, this, &MainPage::tabChanged, Qt::QueuedConnection);

        contactsLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Minimum));

        pagesLayout_ = Utils::emptyVLayout();
        pagesLayout_->addWidget(headerWidget_);
        headerWidget_->setFixedHeight(CommonStyle::getTopPanelHeight());
        headerWidget_->setObjectName(qsl("header"));
        auto l = Utils::emptyHLayout(headerWidget_);
        l->addSpacerItem(new QSpacerItem(Utils::scale_value(back_spacing), 0, QSizePolicy::Fixed));
        headerBack_ = new HeaderBack(headerWidget_);
        headerBack_->setText(QT_TRANSLATE_NOOP("main_page", "Back to chats"));
        headerBack_->setFont(Fonts::appFontScaled(15, Fonts::FontWeight::Medium));
        headerBack_->adjustSize();
        headerBack_->setObjectName(qsl("header_back"));
        headerBack_->setCursor(Qt::PointingHandCursor);
        connect(headerBack_, &Ui::HeaderBack::clicked, this, &MainPage::headerBack);
        l->addWidget(headerBack_);
        headerLabel_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        headerLabel_->setAlignment(Qt::AlignCenter);
        headerLabel_->setText(qsl("label"));
        headerLabel_->setFont(Fonts::appFontScaled(15, Fonts::FontWeight::Medium));
        headerLabel_->setColor(QColor(ql1s("#979797")));
        l->addWidget(headerLabel_);
        l->addSpacerItem(new QSpacerItem(headerBack_->width() + Utils::scale_value(back_spacing), 0, QSizePolicy::Fixed));
        headerWidget_->hide();
        counter_ = new UnreadsCounter(headerWidget_);
        counter_->setFixedSize(Utils::scale_value(balloon_size * 2), Utils::scale_value(balloon_size));
        connect(headerBack_, &HeaderBack::resized, this, [this]() {
            counter_->move(headerBack_->width() + Utils::scale_value(counter_padding + back_spacing), (headerWidget_->height() - Utils::scale_value(balloon_size)) / 2);});

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::showHeader, this, &MainPage::showHeader, Qt::QueuedConnection);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::myProfileBack, this, &MainPage::headerBack);

        pagesLayout_->addWidget(pages_);

        {
            pages_->addWidget(contactDialog_);
            pages_->addWidget(generalSettings_);
            pages_->addWidget(liveChatsPage_);
            pages_->addWidget(themesSettings_);
            pages_->addWidget(embeddedSidebar_);

            pages_->push(contactDialog_);
        }
        originalLayout->addWidget(clSpacer_);

        clHostLayout_->addWidget(contactsWidget_);
        originalLayout->addLayout(clHostLayout_);
        originalLayout->addLayout(pagesLayout_);

        originalLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Minimum));
        originalLayout->setAlignment(Qt::AlignLeft);
        setFocus();

        connect(contactListWidget_, &ContactList::itemSelected, contactDialog_, &ContactDialog::onContactSelected, Qt::QueuedConnection);
        connect(contactDialog_, &Ui::ContactDialog::sendMessage, contactListWidget_, &Ui::ContactList::onSendMessage, Qt::QueuedConnection);

        connect(contactListWidget_, &ContactList::itemSelected, this, &MainPage::onContactSelected, Qt::QueuedConnection);
        connect(contactListWidget_, &ContactList::itemSelected, this, &MainPage::hideRecentsPopup, Qt::QueuedConnection);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::profileSettingsShow, this, &MainPage::onProfileSettingsShow, Qt::QueuedConnection);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::themesSettingsShow, this, &MainPage::onThemesSettingsShow, Qt::QueuedConnection);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::generalSettingsShow, this, &MainPage::onGeneralSettingsShow, Qt::QueuedConnection);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::profileSettingsBack, pages_, &Ui::WidgetsNavigator::pop, Qt::QueuedConnection);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::generalSettingsBack, pages_, &Ui::WidgetsNavigator::pop, Qt::QueuedConnection);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::themesSettingsBack, pages_, &Ui::WidgetsNavigator::pop, Qt::QueuedConnection);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::attachPhoneBack, pages_, &Ui::WidgetsNavigator::pop, Qt::QueuedConnection);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::attachUinBack, pages_, &Ui::WidgetsNavigator::pop, Qt::QueuedConnection);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::popPagesToRoot, this, &MainPage::popPagesToRoot, Qt::DirectConnection);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::startSearchInDialog, this, &MainPage::startSearhInDialog, Qt::QueuedConnection);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::setSearchFocus, this, &MainPage::setSearchFocus, Qt::QueuedConnection);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::themesSettingsOpen, this, &MainPage::themesSettingsOpen, Qt::QueuedConnection);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::searchEnd, this, &MainPage::searchEnd, Qt::QueuedConnection);

        connect(searchWidget_, &Ui::SearchWidget::searchBegin, this, &MainPage::searchBegin, Qt::QueuedConnection);
        connect(searchWidget_, &Ui::SearchWidget::searchEnd, this, &MainPage::searchEnd, Qt::QueuedConnection);
        connect(searchWidget_, &Ui::SearchWidget::inputEmpty, this, &MainPage::searchInputClear, Qt::QueuedConnection);
        connect(searchWidget_, &Ui::SearchWidget::searchIconClicked, this, &MainPage::spreadCL, Qt::QueuedConnection);

        contactListWidget_->getContactListWidget()->connectSearchWidget(searchWidget_);

        connect(
            &Utils::InterConnector::instance(),
            &Utils::InterConnector::historyControlReady,
            Logic::GetMessagesModel(),
            &Logic::MessagesModel::contactChanged,
            Qt::QueuedConnection);

        connect(&Ui::GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipShowVideoWindow, this, &MainPage::onVoipShowVideoWindow, Qt::DirectConnection);
        connect(&Ui::GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipCallIncoming, this, &MainPage::onVoipCallIncoming, Qt::DirectConnection);
        connect(&Ui::GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipCallIncomingAccepted, this, &MainPage::onVoipCallIncomingAccepted, Qt::DirectConnection);
        connect(&Ui::GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipCallDestroyed, this, &MainPage::onVoipCallDestroyed, Qt::DirectConnection);

        QTimer::singleShot(Ui::period_for_start_stats_settings_ms, this, &MainPage::post_stats_with_settings);
        QObject::connect(settingsTimer_, &QTimer::timeout, this, &MainPage::post_stats_with_settings);
        settingsTimer_->start(Ui::period_for_stats_settings_ms);

        connect(Ui::GetDispatcher(), &core_dispatcher::myInfo, this, &MainPage::myInfo, Qt::UniqueConnection);

        connect(contactDialog_, &ContactDialog::clicked, this, &MainPage::hideRecentsPopup);
        connect(searchWidget_, &SearchWidget::activeChanged, this, &MainPage::searchActivityChanged);
        connect(Ui::GetDispatcher(), &core_dispatcher::historyUpdate, contactDialog_, &ContactDialog::onContactSelectedToLastMessage, Qt::QueuedConnection);

        animBurger_ = new QPropertyAnimation(this, QByteArrayLiteral("anim"), this);
        animBurger_->setDuration(ANIMATION_DURATION);
        connect(animBurger_, &QPropertyAnimation::finished, this, &MainPage::animFinished, Qt::QueuedConnection);

        connect(mainMenu_, &Ui::MainMenu::createGroupChat, this, &MainPage::createGroupChat, Qt::QueuedConnection);
        connect(mainMenu_, &Ui::MainMenu::addContact, this, &MainPage::onAddContactClicked, Qt::QueuedConnection);
        connect(mainMenu_, &Ui::MainMenu::contacts, this, &MainPage::contactsClicked, Qt::QueuedConnection);
        connect(mainMenu_, &Ui::MainMenu::settings, this, &MainPage::settingsClicked, Qt::QueuedConnection);
        connect(mainMenu_, &Ui::MainMenu::myProfile, this, &MainPage::myProfileClicked, Qt::QueuedConnection);
        connect(mainMenu_, &Ui::MainMenu::about, this, &MainPage::aboutClicked, Qt::QueuedConnection);
        connect(mainMenu_, &Ui::MainMenu::contactUs, this, &MainPage::contactUsClicked, Qt::QueuedConnection);

        connect(pages_, &QStackedWidget::currentChanged, this, &MainPage::currentPageChanged, Qt::QueuedConnection);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::showStickersStore, this, &MainPage::showStickersStore);

        initAccessability();
    }


    void MainPage::initAccessability()
    {
        Testing::setAccessibleName(unknownsHeader_, qsl("AS unknownsHeader_"));
        Testing::setAccessibleName(searchWidget_, qsl("AS searchWidget_"));
        Testing::setAccessibleName(contactDialog_, qsl("AS contactDialog_"));
        // null Testing::setAccessibleName(videoWindow_, "AS videoWindow_");
        Testing::setAccessibleName(pages_, qsl("AS pages_"));
        // null Testing::setAccessibleName(searchContacts_, "AS searchContacts_");
        Testing::setAccessibleName(generalSettings_, qsl("AS generalSettings_"));
        Testing::setAccessibleName(liveChatsPage_, qsl("AS liveChatsPage_"));
        Testing::setAccessibleName(themesSettings_, qsl("AS themesSettings_"));
        // null Testing::setAccessibleName(noContactsYetSuggestions_, "AS noContactsYetSuggestions_");
        Testing::setAccessibleName(contactListWidget_, qsl("AS contactListWidget_"));
        //Testing::setAccessibleName(settingsTimer_, "AS settingsTimer_");
        // null Testing::setAccessibleName(introduceYourselfSuggestions_, "AS introduceYourselfSuggestions_");
        //Testing::setAccessibleName(needShowIntroduceYourself_, "AS needShowIntroduceYourself_");
        //Testing::setAccessibleName(liveChats_, "AS liveChats_");
        //Testing::setAccessibleName(recvMyInfo_, "AS recvMyInfo_");
        //Testing::setAccessibleName(animCLWidth_, "AS animCLWidth_");
        Testing::setAccessibleName(clSpacer_, qsl("AS clSpacer_"));
        //Testing::setAccessibleName(clHostLayout_, "AS clHostLayout_");
        //Testing::setAccessibleName(leftPanelState_, "AS leftPanelState_");
        //Testing::setAccessibleName(NeedShowUnknownsHeader_, "AS NeedShowUnknownsHeader_");
        Testing::setAccessibleName(myTopWidget_, qsl("AS myTopWidget_"));
        //Testing::setAccessibleName(currentTab_, "AS currentTab_");
        Testing::setAccessibleName(mainMenu_, qsl("AS mainMenu_"));
        Testing::setAccessibleName(semiWindow_, qsl("AS semiWindow_"));
        //Testing::setAccessibleName(anim_, "AS anim_");
        Testing::setAccessibleName(headerWidget_, qsl("AS headerWidget_"));
        Testing::setAccessibleName(headerLabel_, qsl("AS headerLabel_"));
        Testing::setAccessibleName(embeddedSidebar_, qsl("AS embeddedSidebar_"));
        //Testing::setAccessibleName(menuVisible_, "AS menuVisible_");
        // null Testing::setAccessibleName(introduceYourselfSuggestions_, "AS introduceYourselfSuggestions_");

    }

    MainPage::~MainPage()
    {
        if (videoWindow_)
        {
            delete videoWindow_;
            videoWindow_ = nullptr;
        }
    }

    void MainPage::setCLWidth(int _val)
    {
        auto compact_width = ::ContactList::ItemWidth(false, false, true);
        auto normal_width = ::ContactList::ItemWidth(false, false, false);
        contactsWidget_->setFixedWidth(_val);
        contactListWidget_->setItemWidth(_val);
        contactListWidget_->setFixedWidth(_val);
        unknownsHeader_->setFixedWidth(_val);
        myTopWidget_->setFixedWidth(_val);
        bool isCompact = (_val == compact_width);
        contactListWidget_->setPictureOnlyView(isCompact);
        searchButton_->setVisible(isCompact && currentTab_ == RECENTS && !NeedShowUnknownsHeader_);

        TopPanelWidget::Mode m = TopPanelWidget::NORMAL;
        if (isCompact)
            m = TopPanelWidget::COMPACT;
        else if (leftPanelState_ == LeftPanelState::spreaded || _val != normal_width)
            m = TopPanelWidget::SPREADED;
        myTopWidget_->setMode(m);

        if (leftPanelState_ == LeftPanelState::picture_only && _val == compact_width && clHostLayout_->count() == 0)
        {
            clSpacer_->setFixedWidth(0);
            clHostLayout_->addWidget(contactsWidget_);
        }
    }

    int MainPage::getCLWidth() const
    {
        return contactListWidget_->width();
    }

    void MainPage::animateVisibilityCL(int _newWidth, bool _withAnimation)
    {
        int startValue = getCLWidth();
        int endValue = _newWidth;

        int duration = _withAnimation ? 200 : 0;

        animCLWidth_->stop();
        animCLWidth_->setDuration(duration);
        animCLWidth_->setStartValue(startValue);
        animCLWidth_->setEndValue(endValue);
        animCLWidth_->setEasingCurve(QEasingCurve::InQuad);
        animCLWidth_->start();
    }

    void MainPage::resizeEvent(QResizeEvent*)
    {
        auto cl_width = ::ContactList::ItemWidth(false, false, leftPanelState_ == LeftPanelState::picture_only);
        if (leftPanelState_ == LeftPanelState::spreaded
            || (::ContactList::IsPictureOnlyView() != (leftPanelState_ == LeftPanelState::picture_only)))
        {
            setLeftPanelState(::ContactList::IsPictureOnlyView() ? LeftPanelState::picture_only : LeftPanelState::normal, false);
            cl_width = ::ContactList::ItemWidth(false, false, leftPanelState_ == LeftPanelState::picture_only);
        }
        else
        {
            animateVisibilityCL(cl_width, false);
        }

        if (pages_->currentWidget() == embeddedSidebar_)
            embeddedSidebar_->setFixedWidth(width() - cl_width);

        myTopWidget_->setFixedWidth(cl_width);

        mainMenu_->setFixedHeight(height());
        semiWindow_->setFixedSize(size());
    }

    void MainPage::hideMenu()
    {
        if (!menuVisible_)
            return;

        menuVisible_ = false;
        animBurger_->stop();
        animBurger_->setStartValue(max_step);
        animBurger_->setEndValue(min_step);
        animBurger_->start();

        hideSemiWindow();
    }

    bool MainPage::isMenuVisible() const
    {
        return mainMenu_->isVisible() && animBurger_->state() != QVariantAnimation::Running;
    }

    bool MainPage::isMenuVisibleOrOpening() const
    {
        return menuVisible_;
    }

    void MainPage::showSemiWindow()
    {
        semiWindow_->setFixedSize(size());
        semiWindow_->raise();
        semiWindow_->Show();
    }

    void MainPage::hideSemiWindow()
    {
        semiWindow_->Hide();
    }

    bool MainPage::isSemiWindowVisible() const
    {
        return semiWindow_->isSemiWindowVisible();
    }

    int MainPage::getContactDialogWidth(int _mainPageWidth)
    {
        return _mainPageWidth - ::ContactList::ItemWidth(false, false, false);
    }

    void MainPage::setAnim(int _val)
    {
        anim_ = _val;
        auto w = -1 * mainMenu_->width() + mainMenu_->width() * (anim_ / (double)max_step);
        mainMenu_->move(w, 0);
    }

    int MainPage::getAnim() const
    {
        return anim_;
    }

    void MainPage::animFinished()
    {
        if (anim_ == min_step)
            mainMenu_->hide();
    }

    void MainPage::setSearchFocus()
    {
        if (semiWindow_->isSemiWindowVisible())
            return;

        auto curPage = pages_->currentWidget();

        if (curPage != contactDialog_)
        {
            pages_->push(contactDialog_);
            contactListWidget_->changeTab(RECENTS, true);
            currentTab_ = RECENTS;
            myTopWidget_->setBack(false);
        }

        if (::ContactList::IsPictureOnlyView())
        {
            setLeftPanelState(LeftPanelState::spreaded, true, true);
        }
        searchWidget_->setFocus();
    }

    void MainPage::onProfileSettingsShow(QString _uin)
    {
        if (!_uin.isEmpty())
        {
            showSidebar(_uin, profile_page);
            return;
        }

        embeddedSidebar_->preparePage(_uin, profile_page);
        embeddedSidebar_->setFixedWidth(pages_->width());
        pages_->push(embeddedSidebar_);
    }

    void MainPage::raiseVideoWindow()
    {
        if (!videoWindow_)
        {
            videoWindow_ = new(std::nothrow) VideoWindow();
        }

        if (!!videoWindow_ && !videoWindow_->isHidden())
        {
            videoWindow_->showNormal();
            videoWindow_->activateWindow();
        }
    }

    void MainPage::nextChat()
    {
        if (Logic::getContactListModel()->selectedContact().isEmpty())
            return;

        if (contactListWidget_->currentTab() == RECENTS)
        {
            const auto nextContact = Logic::getRecentsModel()->nextAimId(Logic::getContactListModel()->selectedContact());
            if (!nextContact.isEmpty())
            {
                emit Logic::getContactListModel()->select(nextContact, -1);
            }
        }
    }

    void MainPage::prevChat()
    {
        if (Logic::getContactListModel()->selectedContact().isEmpty())
            return;

        if (contactListWidget_->currentTab() == RECENTS)
        {
            const auto prevContact = Logic::getRecentsModel()->prevAimId(Logic::getContactListModel()->selectedContact());
            if (!prevContact.isEmpty())
            {
                emit Logic::getContactListModel()->select(prevContact, -1);
            }
        }
    }

    void MainPage::onGeneralSettingsShow(int _type)
    {
        pages_->push(generalSettings_);
        generalSettings_->setType(_type);
    }

    void MainPage::onThemesSettingsShow(bool _showBackButton, QString _aimId)
    {
        themesSettings_->setBackButton(_showBackButton);
        themesSettings_->setTargetContact(_aimId);
        pages_->push(themesSettings_);
    }

    void MainPage::clearSearchMembers()
    {
        contactListWidget_->update();
    }

    void MainPage::cancelSelection()
    {
        assert(contactDialog_);
        contactDialog_->cancelSelection();
    }

    void MainPage::createGroupChat()
    {
        menuVisible_ = false;
        animBurger_->stop();
        animBurger_->setStartValue(max_step);
        animBurger_->setEndValue(min_step);
        animBurger_->start();

        Ui::createGroupChat({});

        hideSemiWindow();
    }

    void MainPage::myProfileClicked()
    {
        searchButton_->hide();
        contactListWidget_->changeTab(SETTINGS);
        auto settingsTab = contactListWidget_->getSettingsTab();
        if (settingsTab)
            settingsTab->settingsProfileClicked();

        emit Utils::InterConnector::instance().profileSettingsShow(QString());
        emit Utils::InterConnector::instance().showHeader(QT_TRANSLATE_NOOP("main_page", "My profile"));
    }

    void MainPage::aboutClicked()
    {
        emit Utils::InterConnector::instance().generalSettingsShow((int)Utils::CommonSettingsType::CommonSettingsType_About);
        emit Utils::InterConnector::instance().showHeader(QT_TRANSLATE_NOOP("main_page", "About app"));
    }

    void MainPage::contactUsClicked()
    {
        emit Utils::InterConnector::instance().generalSettingsShow((int)Utils::CommonSettingsType::CommonSettingsType_ContactUs);
        emit Utils::InterConnector::instance().showHeader(QT_TRANSLATE_NOOP("main_page", "Contact Us"));
    }

    ContactDialog* MainPage::getContactDialog() const
    {
        assert(contactDialog_);
        return contactDialog_;
    }

    HistoryControlPage* MainPage::getHistoryPage(const QString& _aimId) const
    {
        return contactDialog_->getHistoryPage(_aimId);
    }

    void MainPage::insertTopWidget(const QString& _aimId, QWidget* _widget)
    {
        contactDialog_->insertTopWidget(_aimId, _widget);
    }

    void MainPage::removeTopWidget(const QString& _aimId)
    {
        contactDialog_->removeTopWidget(_aimId);
    }

    void MainPage::showSidebar(const QString& _aimId, int _page)
    {
        if (searchContacts_ && pages_->currentWidget() == searchContacts_)
        {
            embeddedSidebar_->preparePage(_aimId, (SidebarPages)_page);
            embeddedSidebar_->setFixedWidth(pages_->width());
            pages_->push(embeddedSidebar_);
        }
        else
        {
            contactDialog_->showSidebar(_aimId, _page);
        }
    }

    bool MainPage::isSidebarVisible() const
    {
        return contactDialog_->isSidebarVisible();
    }

    void MainPage::setSidebarVisible(bool _show)
    {
        if (pages_->currentWidget() == embeddedSidebar_)
        {
            pages_->pop();
        }
        else
        {
            contactDialog_->setSidebarVisible(_show);
        }
    }

    void MainPage::restoreSidebar()
    {
        auto cont = Logic::getContactListModel()->selectedContact();
        if (isSidebarVisible() && !cont.isEmpty())
            showSidebar(cont, menu_page);
        else
            setSidebarVisible(false);
    }

    bool MainPage::isContactDialog() const
    {
        if (!pages_)
            return false;

        return pages_->currentWidget() == contactDialog_;
    }

    void MainPage::onContactSelected(QString _contact)
    {
        pages_->poproot();
        headerWidget_->hide();

        if (searchContacts_)
        {
            pages_->removeWidget(searchContacts_);
            searchContacts_->deleteLater();
            searchContacts_ = nullptr;
        }

        emit Utils::InterConnector::instance().showPlaceholder(Utils::PlaceholdersType::PlaceholdersType_SetExistanseOffIntroduceYourself);
        emit Utils::InterConnector::instance().showPlaceholder(Utils::PlaceholdersType::PlaceholdersType_HideFindFriend);
    }

    void MainPage::onAddContactClicked()
    {
        if (!searchContacts_)
        {
            searchContacts_ = new SearchContactsWidget(this);
            connect(searchContacts_, &SearchContactsWidget::active, this, &MainPage::hideRecentsPopup);
            pages_->addWidget(searchContacts_);
        }
        pages_->push(searchContacts_);
        searchContacts_->onFocus();
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::search_open_page);
        emit Utils::InterConnector::instance().showHeader(QT_TRANSLATE_NOOP("main_page", "Add contact"));
    }

    void MainPage::showStickersStore()
    {
        if (!stickersStore_)
        {
            stickersStore_ = new Stickers::Store(this);
            //connect(stickersStore_, &Stickers::Store::active, this, &MainPage::hideRecentsPopup);
            pages_->addWidget(stickersStore_);
        }
        pages_->push(stickersStore_);

        //stickersStore_->onFocus();

        emit Utils::InterConnector::instance().showHeader(QT_TRANSLATE_NOOP("main_page", "Stickers"));
    }

    void MainPage::contactsClicked()
    {
        menuVisible_ = false;
        animBurger_->stop();
        animBurger_->setStartValue(max_step);
        animBurger_->setEndValue(min_step);
        animBurger_->start();

        SelectContactsWidget contacts(nullptr, Logic::MembersWidgetRegim::CONTACT_LIST_POPUP, QT_TRANSLATE_NOOP("popup_window", "Contacts"), QString(), Ui::MainPage::instance());
        emit Utils::InterConnector::instance().searchEnd();
        contacts.setFixedWidth(Utils::scale_value(CL_POPUP_WIDTH));
        Logic::getContactListModel()->updatePlaceholders();
        contacts.show();

        hideSemiWindow();
    }

    void MainPage::settingsClicked()
    {
        searchButton_->hide();
        contactListWidget_->changeTab(SETTINGS);
        auto settingsTab = contactListWidget_->getSettingsTab();
        if (settingsTab)
            settingsTab->settingsGeneralClicked();

        emit Utils::InterConnector::instance().generalSettingsShow((int)Utils::CommonSettingsType::CommonSettingsType_General);
        emit Utils::InterConnector::instance().showHeader(QT_TRANSLATE_NOOP("main_page", "General settings"));
    }

    void MainPage::searchBegin()
    {
        contactListWidget_->setSearchMode(true);
    }

    void MainPage::searchEnd()
    {
        if (NeedShowUnknownsHeader_)
            changeCLHead(true);

        searchWidget_->clearInput();

        emit Utils::InterConnector::instance().hideNoSearchResults();
        emit Utils::InterConnector::instance().hideSearchSpinner();
        emit Utils::InterConnector::instance().disableSearchInDialog();

        contactListWidget_->setSearchMode(false);
        contactListWidget_->setSearchInDialog(false);
    }

    void MainPage::searchInputClear()
    {
        emit Utils::InterConnector::instance().hideNoSearchResults();
        emit Utils::InterConnector::instance().hideSearchSpinner();

        if (!contactListWidget_->getSearchInDialog())
            contactListWidget_->setSearchMode(false);
    }

    void MainPage::onVoipCallIncoming(const std::string& _account, const std::string& _contact)
    {
        assert(!_account.empty());
        assert(!_contact.empty());

        if (!_account.empty() && !_contact.empty())
        {
            const std::string callId = _account + "#" + _contact;

            auto it = incomingCallWindows_.find(callId);
            if (incomingCallWindows_.end() == it || !it->second)
            {
                std::shared_ptr<IncomingCallWindow> window(new(std::nothrow) IncomingCallWindow(_account, _contact));
                assert(!!window);

                if (!!window)
                {
                    window->showFrame();
                    incomingCallWindows_[callId] = window;
                }
            }
            else
            {
                std::shared_ptr<IncomingCallWindow> wnd = it->second;
                wnd->showFrame();
            }
        }
    }

    void MainPage::destroyIncomingCallWindow(const std::string& _account, const std::string& _contact)
    {
        assert(!_account.empty());
        assert(!_contact.empty());

        if (!_account.empty() && !_contact.empty())
        {
            const std::string call_id = _account + "#" + _contact;
            auto it = incomingCallWindows_.find(call_id);
            if (incomingCallWindows_.end() != it)
            {
                auto window = it->second;
                assert(!!window);

                if (!!window)
                {
                    window->hideFrame();
                }
            }
        }
    }

    void MainPage::onVoipCallIncomingAccepted(const voip_manager::ContactEx& _contactEx)
    {
        destroyIncomingCallWindow(_contactEx.contact.account, _contactEx.contact.contact);

        Utils::InterConnector::instance().getMainWindow()->closeGallery();
    }

    void MainPage::onVoipCallDestroyed(const voip_manager::ContactEx& _contactEx)
    {
        destroyIncomingCallWindow(_contactEx.contact.account, _contactEx.contact.contact);
    }

    void MainPage::showVideoWindow()
    {
        if (videoWindow_)
        {
            if (videoWindow_->isMinimized())
            {
                videoWindow_->showNormal();
            }

            videoWindow_->activateWindow();
#ifndef _WIN32
            videoWindow_->raise();
#endif
        }
    }

    void MainPage::notifyApplicationWindowActive(const bool isActive)
    {
        if (contactDialog_)
            contactDialog_->notifyApplicationWindowActive(isActive);
    }

    void MainPage::recentsTabActivate(bool _selectUnread)
    {
        assert(!!contactListWidget_);
        if (contactListWidget_)
        {
            contactListWidget_->changeTab(RECENTS);

            if (_selectUnread)
            {
                QString aimId = Logic::getRecentsModel()->nextUnreadAimId();
                if (aimId.isEmpty())
                    aimId = Logic::getUnknownsModel()->nextUnreadAimId();
                if (!aimId.isEmpty())
                    contactListWidget_->select(aimId);
            }
        }
    }

    void MainPage::selectRecentChat(const QString& _aimId)
    {
        assert(!!contactListWidget_);
        if (contactListWidget_)
        {
            if (!_aimId.isEmpty())
                contactListWidget_->select(_aimId);
        }
    }

    void MainPage::settingsTabActivate(Utils::CommonSettingsType _item)
    {
        assert(!!contactListWidget_);
        if (contactListWidget_)
        {
            contactListWidget_->settingsClicked();

            switch (_item)
            {
            case Utils::CommonSettingsType::CommonSettingsType_General:
            case Utils::CommonSettingsType::CommonSettingsType_VoiceVideo:
            case Utils::CommonSettingsType::CommonSettingsType_Notifications:
            case Utils::CommonSettingsType::CommonSettingsType_Themes:
            case Utils::CommonSettingsType::CommonSettingsType_About:
            case Utils::CommonSettingsType::CommonSettingsType_ContactUs:
            case Utils::CommonSettingsType::CommonSettingsType_AttachPhone:
            case Utils::CommonSettingsType::CommonSettingsType_AttachUin:
                Utils::InterConnector::instance().generalSettingsShow((int)_item);
                break;
            case Utils::CommonSettingsType::CommonSettingsType_Profile:
                Utils::InterConnector::instance().profileSettingsShow(QString());
                break;
            default:
                break;
            }

            if (_item == Utils::CommonSettingsType::CommonSettingsType_VoiceVideo && contactListWidget_)
                contactListWidget_->selectSettingsVoipTab();
        }
    }

    void MainPage::onVoipShowVideoWindow(bool _enabled)
    {
        if (!videoWindow_)
        {
            videoWindow_ = new(std::nothrow) VideoWindow();
            Ui::GetDispatcher()->getVoipController().updateActivePeerList();
        }

        if (!!videoWindow_)
        {
            if (_enabled)
            {
                videoWindow_->showFrame();
            }
            else
            {
                videoWindow_->hideFrame();

                bool wndMinimized = false;
                bool wndHiden = false;
                if (QWidget* parentWnd = window())
                {
                    wndHiden = !parentWnd->isVisible();
                    wndMinimized = parentWnd->isMinimized();
                }

                if (!Utils::foregroundWndIsFullscreened() && !wndMinimized && !wndHiden)
                {
                    raise();
                }
            }
        }
    }

    void MainPage::hideInput()
    {
        contactDialog_->hideInput();
    }

    QWidget* MainPage::showNoContactsYetSuggestions(QWidget* _parent)
    {
        if (!noContactsYetSuggestions_)
        {
            noContactsYetSuggestions_ = new QWidget(_parent);
            noContactsYetSuggestions_->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
            noContactsYetSuggestions_->setStyleSheet(qsl("background-color: %1;")
                .arg(CommonStyle::getFrameColor().name()));
            {
                auto l = Utils::emptyVLayout(noContactsYetSuggestions_);
                l->setAlignment(Qt::AlignCenter);
                {
                    auto p = new QWidget(noContactsYetSuggestions_);
                    p->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Preferred);
                    auto pl = Utils::emptyHLayout(p);
                    pl->setAlignment(Qt::AlignCenter);
                    {
                        auto logoWidget = new QWidget(p);
                        logoWidget->setObjectName(build::is_icq() ? qsl("logoWidget") : qsl("logoWidgetAgent"));
                        logoWidget->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);
                        logoWidget->setFixedSize(Utils::scale_value(80), Utils::scale_value(80));
                        pl->addWidget(logoWidget);
                    }
                    l->addWidget(p);
                }
                {
                    auto p = new QWidget(noContactsYetSuggestions_);
                    p->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Preferred);
                    auto pl = Utils::emptyHLayout(p);
                    pl->setAlignment(Qt::AlignCenter);
                    {
                        auto w = new Ui::TextEmojiWidget(p, Fonts::appFontScaled(24), CommonStyle::getColor(CommonStyle::Color::TEXT_PRIMARY), Utils::scale_value(44));
                        w->setSizePolicy(QSizePolicy::Policy::Preferred, w->sizePolicy().verticalPolicy());
                        w->setText(
                            build::is_icq()?
                            QT_TRANSLATE_NOOP("placeholders", "Install ICQ on mobile")
                            : QT_TRANSLATE_NOOP("placeholders", "Install Mail.Ru Agent on mobile")
                        );
                        pl->addWidget(w);
                    }
                    l->addWidget(p);
                }
                {
                    auto p = new QWidget(noContactsYetSuggestions_);
                    p->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Preferred);
                    auto pl = Utils::emptyHLayout(p);
                    pl->setAlignment(Qt::AlignCenter);
                    {
                        auto w = new Ui::TextEmojiWidget(p, Fonts::appFontScaled(24), CommonStyle::getColor(CommonStyle::Color::TEXT_PRIMARY), Utils::scale_value(30));
                        w->setSizePolicy(QSizePolicy::Policy::Preferred, w->sizePolicy().verticalPolicy());
                        w->setText(QT_TRANSLATE_NOOP("placeholders", "to synchronize your contacts"));
                        pl->addWidget(w);
                    }
                    l->addWidget(p);
                }
                {
                    auto p = new QWidget(noContactsYetSuggestions_);
                    p->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Preferred);
                    auto pl = new QHBoxLayout(p);
                    pl->setContentsMargins(0, Utils::scale_value(28), 0, 0);
                    pl->setSpacing(Utils::scale_value(8));
                    pl->setAlignment(Qt::AlignCenter);
                    {
                        auto appStoreButton = new QPushButton(p);
                        appStoreButton->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);
                        appStoreButton->setFlat(true);

                        const auto appStoreImage = qsl(":/placeholders/appstore_%1_100").
                            arg(Ui::get_gui_settings()->get_value(settings_language, QString()).toUpper());

                        const auto appStoreImageStyle = qsl("QPushButton { border-image: url(%1); } QPushButton:hover { border-image: url(%2); } QPushButton:focus { border: none; outline: none; }")
                            .arg(appStoreImage, appStoreImage);

                        Utils::ApplyStyle(appStoreButton, appStoreImageStyle);

                        appStoreButton->setFixedSize(Utils::scale_value(152), Utils::scale_value(44));
                        appStoreButton->setCursor(Qt::PointingHandCursor);

                        _parent->connect(appStoreButton, &QPushButton::clicked, []()
                        {
                            QDesktopServices::openUrl(build::is_icq() ?
                                QUrl(qsl("https://app.appsflyer.com/id302707408?pid=icq_win"))
                                : QUrl(qsl("https://app.appsflyer.com/id335315530?pid=agent_win"))
                            );
                            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::cl_empty_ios);
                        });
                        pl->addWidget(appStoreButton);

                        auto googlePlayWidget = new QPushButton(p);
                        googlePlayWidget->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);
                        googlePlayWidget->setFlat(true);

                        const auto googlePlayImage = qsl(":/placeholders/gplay_%1_100")
                            .arg(Ui::get_gui_settings()->get_value(settings_language, QString()).toUpper());
                        const auto googlePlayStyle = qsl("QPushButton { border-image: url(%1); } QPushButton:hover { border-image: url(%2); } QPushButton:focus { border: none; outline: none; }")
                            .arg(googlePlayImage, googlePlayImage);

                        Utils::ApplyStyle(googlePlayWidget, googlePlayStyle);

                        googlePlayWidget->setFixedSize(Utils::scale_value(152), Utils::scale_value(44));
                        googlePlayWidget->setCursor(Qt::PointingHandCursor);
                        _parent->connect(googlePlayWidget, &QPushButton::clicked, []()
                        {
                            QDesktopServices::openUrl(build::is_icq() ?
                                QUrl(qsl("https://app.appsflyer.com/com.icq.mobile.client?pid=icq_win"))
                                : QUrl(qsl("https://app.appsflyer.com/ru.mail?pid=agent_win"))
                            );
                            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::cl_empty_android);
                        });
                        pl->addWidget(googlePlayWidget);
                    }
                    l->addWidget(p);
                }
            }
        }
        return noContactsYetSuggestions_;
    }

    void MainPage::showPlaceholder(Utils::PlaceholdersType _placeholdersType)
    {
        switch(_placeholdersType)
        {
        case Utils::PlaceholdersType::PlaceholdersType_HideFindFriend:
            if (noContactsYetSuggestions_)
            {
                noContactsYetSuggestions_->hide();
                pages_->removeWidget(noContactsYetSuggestions_);
            }
            if (pages_->currentWidget() != embeddedSidebar_)
                pages_->poproot();
            break;

        case Utils::PlaceholdersType::PlaceholdersType_FindFriend:
            pages_->insertWidget(1, showNoContactsYetSuggestions(pages_));
            pages_->poproot();
            break;

        case Utils::PlaceholdersType::PlaceholdersType_SetExistanseOnIntroduceYourself:
            needShowIntroduceYourself_ = true;
            break;

        case Utils::PlaceholdersType::PlaceholdersType_SetExistanseOffIntroduceYourself:
            if (needShowIntroduceYourself_)
            {
                if (introduceYourselfSuggestions_)
                {
                    introduceYourselfSuggestions_->hide();
                    pages_->removeWidget(introduceYourselfSuggestions_);
                }

                if (pages_->currentWidget() != embeddedSidebar_)
                    pages_->poproot();

                needShowIntroduceYourself_ = false;
            }
            break;

        case Utils::PlaceholdersType::PlaceholdersType_IntroduceYourself:
            if (!needShowIntroduceYourself_)
                break;

            pages_->insertWidget(0, showIntroduceYourselfSuggestions(pages_));
            pages_->poproot();
            break;

        default:
            break;
        }
    }

    QWidget* MainPage::showIntroduceYourselfSuggestions(QWidget* _parent)
    {
        if (!introduceYourselfSuggestions_)
        {
            introduceYourselfSuggestions_ = new IntroduceYourself(MyInfo()->aimId(), MyInfo()->friendlyName(), _parent);
            introduceYourselfSuggestions_->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
            introduceYourselfSuggestions_->setStyleSheet(qsl("border: 20px solid red;"));
        }
        return introduceYourselfSuggestions_;
    }

    void MainPage::post_stats_with_settings()
    {
        Utils::getStatsSender()->trySendStats();
    }

    void MainPage::myInfo()
    {
        if (MyInfo()->friendlyName().isEmpty())
        {
            if (!recvMyInfo_)
            {
                emit Utils::InterConnector::instance().showPlaceholder(Utils::PlaceholdersType::PlaceholdersType_SetExistanseOnIntroduceYourself);
                emit Utils::InterConnector::instance().showPlaceholder(Utils::PlaceholdersType::PlaceholdersType_IntroduceYourself);
            }
        }
        else
        {
            emit Utils::InterConnector::instance().showPlaceholder(Utils::PlaceholdersType::PlaceholdersType_SetExistanseOffIntroduceYourself);
        }
        recvMyInfo_ = true;
    }

    void MainPage::openCreatedGroupChat()
    {
        auto connect_id = connect(GetDispatcher(), &core_dispatcher::openChat, this, [=](QString _aimId) { QTimer::singleShot(500, this, [=](){ contactListWidget_->select(_aimId); } ); });
        QTimer::singleShot(3000, this, [=] { disconnect(connect_id); } );
    }

    void MainPage::popPagesToRoot()
    {
        pages_->poproot();
    }

    void MainPage::liveChatSelected()
    {
        pages_->push(liveChatsPage_);
    }

    void MainPage::spreadCL()
    {
        if (::ContactList::IsPictureOnlyView())
        {
            setLeftPanelState(LeftPanelState::spreaded, true, true);
            searchWidget_->setFocus();
        }
    }

    void MainPage::hideRecentsPopup()
    {
        if (leftPanelState_ == LeftPanelState::spreaded)
            setLeftPanelState(LeftPanelState::picture_only, true);
    }


    void MainPage::setLeftPanelState(LeftPanelState _newState, bool _withAnimation, bool _for_search, bool _force)
    {
        assert(_newState > LeftPanelState::min && _newState < LeftPanelState::max);

        if (leftPanelState_ == _newState && !_force)
            return;

        int new_cl_width = 0;
        leftPanelState_ = _newState;
        if (leftPanelState_ == LeftPanelState::normal)
        {
            clSpacer_->setFixedWidth(0);
            if (clHostLayout_->count() == 0)
                clHostLayout_->addWidget(contactsWidget_);

            new_cl_width = ::ContactList::ItemWidth(false, false, false);
            contactListWidget_->setPictureOnlyView(false);
            unknownsHeader_->setVisible(NeedShowUnknownsHeader_ && leftPanelState_ != LeftPanelState::picture_only);
            animateVisibilityCL(new_cl_width, _withAnimation);

            if (NeedShowUnknownsHeader_)
                myTopWidget_->hide();
        }
        else if (leftPanelState_ == LeftPanelState::picture_only)
        {
            new_cl_width = ::ContactList::ItemWidth(false, false, true);
            searchWidget_->clearInput();
            unknownsHeader_->setVisible(NeedShowUnknownsHeader_ && leftPanelState_ != LeftPanelState::picture_only);
            animateVisibilityCL(new_cl_width, _withAnimation);

            if (currentTab_ != RECENTS && currentTab_ != SETTINGS)
                contactListWidget_->changeTab(RECENTS);

            if (NeedShowUnknownsHeader_)
                myTopWidget_->show();

            //searchDropdown_->hide();
        }
        else if (leftPanelState_ == LeftPanelState::spreaded)
        {
            clSpacer_->setFixedWidth(::ContactList::ItemWidth(false, false, true));
            clHostLayout_->removeWidget(contactsWidget_);

            contactsLayout->setParent(contactsWidget_);
            new_cl_width = Utils::scale_value(360);

            contactListWidget_->setItemWidth(new_cl_width);

            unknownsHeader_->setVisible(NeedShowUnknownsHeader_ && leftPanelState_ != LeftPanelState::picture_only);
            animateVisibilityCL(new_cl_width, _withAnimation);
        }
        else
        {
            assert(false && "Left Panel state does not exist.");
        }
    }

    void MainPage::searchActivityChanged(bool _isActive)
    {
        if (!_isActive && leftPanelState_ == LeftPanelState::spreaded)
            hideRecentsPopup();

        myTopWidget_->searchActivityChanged(_isActive);
    }

    bool MainPage::isVideoWindowActive()
    {
        return (videoWindow_ && videoWindow_->isActiveWindow());
    }

    void MainPage::setFocusOnInput()
    {
        if (contactDialog_)
            contactDialog_->setFocusOnInputWidget();
    }

    void MainPage::clearSearchFocus()
    {
        searchWidget_->clearFocus();
    }

    void MainPage::onSendMessage(const QString& contact)
    {
        if (contactDialog_)
            contactDialog_->onSendMessage(contact);
    }

    void MainPage::startSearhInDialog(QString _aimid)
    {
        changeCLHead(false);

        setSearchFocus();

        Logic::getSearchModelDLG()->setSearchInDialog(_aimid);

        searchWidget_->clearInput();
        contactListWidget_->setSearchMode(true);
        contactListWidget_->setSearchInDialog(true);
    }

    void MainPage::changeCLHead(bool _showUnknownHeader)
    {
        if (currentTab_ == SETTINGS)
            return;

        unknownsHeader_->setVisible(_showUnknownHeader && leftPanelState_ != LeftPanelState::picture_only);

        if (_showUnknownHeader)
        {
            if (leftPanelState_ == LeftPanelState::normal)
                myTopWidget_->hide();
        }
        else
        {
            myTopWidget_->show();
        }
    }

    void MainPage::changeCLHeadToSearchSlot()
    {
        NeedShowUnknownsHeader_ = false;
        changeCLHead(false);
        myTopWidget_->setBack(false);
        searchButton_->setVisible(leftPanelState_ == LeftPanelState::picture_only && currentTab_ == RECENTS && !NeedShowUnknownsHeader_);
    }

    void MainPage::changeCLHeadToUnknownSlot()
    {
        NeedShowUnknownsHeader_ = true;
        changeCLHead(true);
        myTopWidget_->setBack(true);
        searchButton_->hide();
    }

    void MainPage::openRecents()
    {
        emit Utils::InterConnector::instance().unknownsGoBack();
        contactListWidget_->changeTab(RECENTS);
    }

    void MainPage::showMainMenu()
     {
        if (menuVisible_)
            return;

        menuVisible_ = true;
        showSemiWindow();
        mainMenu_->setFixedHeight(height());
        mainMenu_->raise();
        mainMenu_->show();

        animBurger_->stop();
        animBurger_->setStartValue(min_step);
        animBurger_->setEndValue(max_step);
        animBurger_->start();
    }

    void MainPage::compactModeChanged()
    {
        setLeftPanelState(leftPanelState_, false, false, true);
    }

    void MainPage::tabChanged(int tab)
    {
        if (tab != currentTab_ && leftPanelState_ == LeftPanelState::spreaded)
        {
            setLeftPanelState(LeftPanelState::picture_only, false);
        }

        if (tab != currentTab_)
        {
            if (tab == RECENTS)
                myTopWidget_->show();
            else
                myTopWidget_->hide();

            currentTab_ = tab;
        }

        searchButton_->setVisible(leftPanelState_ == LeftPanelState::picture_only && tab == RECENTS && !NeedShowUnknownsHeader_);
    }

    void MainPage::themesSettingsOpen()
    {
        contactListWidget_->openThemeSettings();
    }

    void MainPage::headerBack()
    {
        headerWidget_->hide();
        pages_->poproot();
        contactListWidget_->changeTab(RECENTS);
    }

    void MainPage::showHeader(const QString& text)
    {
        headerLabel_->setText(text);
        headerWidget_->show();
    }

    void MainPage::currentPageChanged(int _index)
    {
        emit Utils::InterConnector::instance().currentPageChanged();
    }
}

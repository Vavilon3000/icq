#include "stdafx.h"
#include "MenuPage.h"
#include "SidebarUtils.h"
#include "../MainPage.h"
#include "../GroupChatOperations.h"
#include "../history_control/MessagesModel.h"
#include "../contact_list/AbstractSearchModel.h"
#include "../contact_list/ContactList.h"
#include "../contact_list/ContactListWidget.h"
#include "../contact_list/ContactListModel.h"
#include "../contact_list/RecentsModel.h"
#include "../contact_list/UnknownsModel.h"
#include "../contact_list/ChatMembersModel.h"
#include "../contact_list/ContactListItemDelegate.h"
#include "../contact_list/ContactListItemRenderer.h"
#include "../contact_list/SelectionContactsForGroupChat.h"
#include "../contact_list/SearchWidget.h"
#include "../../my_info.h"
#include "../../cache/avatars/AvatarStorage.h"
#include "../../controls/CustomButton.h"
#include "../../controls/ContactAvatarWidget.h"
#include "../../controls/TextEditEx.h"
#include "../../controls/LabelEx.h"
#include "../../controls/CommonStyle.h"
#include "../../controls/ContextMenu.h"
#include "../../controls/TransparentScrollBar.h"
#include "../../core_dispatcher.h"
#include "../../utils/utils.h"
#include "../../utils/Text2DocConverter.h"
#include "../../utils/gui_coll_helper.h"
#include "../../utils/InterConnector.h"

namespace
{
    const int avatar_margin = 8;
    const int avatar_size = 80;
    const int BACK_WIDTH = 52;
    const int BACK_HEIGHT = 48;
    const int HOR_PADDING = 16;
    const int NAME_MARGIN = 12;
    const int ITEM_HEIGHT = 40;
    const int LEFT_MARGIN = 52;
    const int add_contact_margin = 16;
    const int desc_length = 100;
    const int checkbox_width = 44;
    const int checkbox_height = 24;
    const int checkbox_bottom_margin = 12;
    const int not_member_space = 8;
    const int search_hor_spacing = 4;
    const int search_ver_spacing = 8;

    enum widgets
    {
        main = 0,
        list = 1,
    };

    enum currentListTab
    {
        all = 0,
        block = 1,
        admins = 2,
        pending = 3,
        privacy = 4,
    };

    QMap<QString, QVariant> makeData(const QString& command, const QString& aimId)
    {
        QMap<QString, QVariant> result;
        result[qsl("command")] = command;
        result[qsl("aimid")] = aimId;
        return result;
    }
}

namespace Ui
{
    MenuPage::MenuPage(QWidget* parent)
        : SidebarPage(parent)
        , currentTab_(all)
    {
        init();
    }

    void MenuPage::paintEvent(QPaintEvent* _e)
    {
        QStyleOption opt;
        opt.init(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
        QWidget::paintEvent(_e);
        p.fillRect(rect(), CommonStyle::getFrameColor());
    }

    void MenuPage::resizeEvent(QResizeEvent *e)
    {
        mainWidget_->setFixedWidth(e->size().width());
        return SidebarPage::resizeEvent(e);
    }

    void MenuPage::updateWidth()
    {
        name_->adjustHeight(width_ - Utils::scale_value(avatar_size + 2 * HOR_PADDING + NAME_MARGIN));
        youAreNotAMember_->adjustHeight(width_ - Utils::scale_value(2 * HOR_PADDING));
        firstLine_->setLineWidth(width_);
        secondLine_->setLineWidth(width_);
        thirdLine_->setLineWidth(width_);
        approveAllLine_->setLineWidth(width_);
        searchWidget_->setFixedWidth(width_);
        approveAllWidget_->setFixedWidth(width_ + Utils::scale_value(HOR_PADDING));
        notificationsButton_->setFixedWidth(width_ - Utils::scale_value(HOR_PADDING + checkbox_width));
        delegate_->setFixedWidth(width_);
        delegate_->setRightMargin(Utils::scale_value(HOR_PADDING));
        addContact_->setFixedWidth(width_ - Utils::scale_value(2 * HOR_PADDING));
        description_->adjustHeight(width_ - Utils::scale_value(avatar_size + 2 * HOR_PADDING + NAME_MARGIN));
        allMembersLabel_->setFixedWidth(width_ - Utils::scale_value(LEFT_MARGIN + HOR_PADDING) - allMembersCount_->width());
        blockLabel_->setFixedWidth(width_ - Utils::scale_value(LEFT_MARGIN + HOR_PADDING) - blockCount_->width());
        pendingLabel_->setFixedWidth(width_ - Utils::scale_value(LEFT_MARGIN + HOR_PADDING) - pendingCount_->width());

        avatarName_->setFixedWidth(width_);
        addToChat_->setFixedWidth(width_);
        favoriteButton_->setFixedWidth(width_);
        copyLink_->setFixedWidth(width_);
        themesButton_->setFixedWidth(width_);
        privacyButton_->setFixedWidth(width_);
        eraseHistoryButton_->setFixedWidth(width_);
        ignoreButton_->setFixedWidth(width_);
        quitAndDeleteButton_->setFixedWidth(width_);
        spamButtonAuth_->setFixedWidth(width_);
        deleteButton_->setFixedWidth(width_);
        spamButton_->setFixedWidth(width_);
        admins_->setFixedWidth(width_);
        allMembers_->setFixedWidth(width_);
        blockList_->setFixedWidth(width_);
        pendingList_->setFixedWidth(width_);
    }

    void MenuPage::initFor(const QString& aimId)
    {
        const auto newContact = (currentAimId_ != aimId);
        currentAimId_ = aimId;
        currentTab_ = all;

        avatar_->UpdateParams(currentAimId_, Logic::getContactListModel()->getDisplayName(currentAimId_));
        name_->setPlainText(QString());
        QTextCursor cursorName = name_->textCursor();
        Logic::Text2Doc(Logic::getContactListModel()->getDisplayName(currentAimId_), cursorName, Logic::Text2DocHtmlMode::Pass, false);
        if (newContact)
        {
            moreLabel_->hide();
            pendingList_->hide();
            description_->hide();
            blockList_->hide();
            privacyButton_->hide();
            nameLayout_->setAlignment(Qt::AlignVCenter);
            nameLayout_->invalidate();

            allMembersCount_->setText(QString());
            blockCount_->setText(QString());
            pendingCount_->setText(QString());
            chatMembersModel_->clear();
        }

        bool isFavorite = Logic::getRecentsModel()->isFavorite(currentAimId_);
        favoriteButton_->setImage(isFavorite ? qsl(":/resources/i_del_fav_100.png") : qsl(":/resources/i_add_fav_100.png"));
        favoriteButton_->setText(isFavorite ? QT_TRANSLATE_NOOP("sidebar", "Remove from favorites") : QT_TRANSLATE_NOOP("sidebar", "Add to favorites"));

        bool isMuted = Logic::getContactListModel()->isMuted(currentAimId_);
        notificationsCheckbox_->blockSignals(true);
        notificationsCheckbox_->setChecked(!isMuted);
        notificationsCheckbox_->blockSignals(false);
        notificationsCheckbox_->adjustSize();

        delegate_->setRenderRole(true);

        info_.reset();
        bool isChat = Logic::getContactListModel()->isChat(currentAimId_);
        avatarName_->setEnabled(!isChat);
        avatarName_->setCursor(isChat ? QCursor(Qt::ArrowCursor) : (Qt::PointingHandCursor));

        copyLink_->setText(isChat ? QT_TRANSLATE_NOOP("sidebar", "Share link") : QT_TRANSLATE_NOOP("sidebar", "Share contact"));
        if (newContact)
            copyLink_->setVisible(!isChat);

        if (isChat)
        {
            chatMembersModel_->isShortView_ = false;
            chatMembersModel_->loadAllMembers(currentAimId_, 1);
            delegate_->setRegim((chatMembersModel_->isAdmin() || chatMembersModel_->isModer()) ? Logic::ADMIN_MEMBERS : Logic::CONTACT_LIST);
        }
        else
        {
            chatMembersModel_->initForSingle(currentAimId_);
            delegate_->setRegim(Logic::CONTACT_LIST);
        }

        const auto notAMember = Logic::getContactListModel()->getYourRole(aimId) == ql1s("notamember");

        if (notAMember)
            moreLabel_->setVisible(false);

        quitAndDeleteButton_->setVisible(isChat);
        allMembers_->setVisible(isChat && !notAMember);
        admins_->setVisible(isChat && !notAMember);

        bool isNotAuth = Logic::getContactListModel()->isNotAuth(currentAimId_);
        bool isIgnored = Logic::getIgnoreModel()->getMemberItem(currentAimId_) != nullptr;

        deleteButton_->setVisible(!isNotAuth && !isChat);
        spamButtonAuth_->setVisible(!isNotAuth && !isChat);
        addContact_->setVisible(isNotAuth);
        addContactSpacer_->setVisible(isNotAuth);
        addContactSpacerTop_->setVisible(isNotAuth);
        spamButton_->setVisible(isNotAuth);
        addToChat_->setVisible(!isIgnored && !notAMember && !isNotAuth);
        addToChat_->setText(isChat ? QT_TRANSLATE_NOOP("sidebar", "Add to chat") : QT_TRANSLATE_NOOP("sidebar", "Create groupchat"));
        addToChat_->update();
        firstLine_->setVisible(!isNotAuth);
        secondLine_->setVisible(!isNotAuth && !notAMember);
        thirdLine_->setVisible(!isNotAuth && !isIgnored);
        favoriteButton_->setVisible(!isNotAuth && !notAMember);
        notificationsCheckbox_->setVisible(!isNotAuth && !notAMember);
        notificationsButton_->setVisible(!isNotAuth && !notAMember);
        themesButton_->setVisible(!isNotAuth && !notAMember);

        notMemberTopSpacer_->setVisible(notAMember);
        notMemberBottomSpacer_->setVisible(notAMember);
        youAreNotAMember_->setVisible(notAMember);

        stackedWidget_->setCurrentIndex(main);
        updateWidth();
    }

    void MenuPage::init()
    {
        stackedWidget_ = new QStackedWidget(this);
        stackedWidget_->setContentsMargins(0, 0, 0, 0);
        auto layout = Utils::emptyVLayout(this);
        layout->addWidget(stackedWidget_);
        area_ = CreateScrollAreaAndSetTrScrollBarV(stackedWidget_);
        stackedWidget_->insertWidget(main, area_);

        mainWidget_ = new QWidget(area_);
        mainWidget_->setStyleSheet(qsl("background: transparent;"));
        area_->setContentsMargins(0, 0, 0, 0);
        area_->setWidget(mainWidget_);
        area_->setWidgetResizable(true);
        area_->setFrameStyle(QFrame::NoFrame);
        area_->setStyleSheet(qsl("background: transparent;"));
        area_->horizontalScrollBar()->setEnabled(false);

        setStyleSheet(Utils::LoadStyle(qsl(":/qss/sidebar")));
        rootLayout_ = Utils::emptyVLayout(mainWidget_);
        rootLayout_->setAlignment(Qt::AlignTop);

        initAvatarAndName();
        initAddContactAndSpam();
        initFavoriteNotificationsSearchTheme();
        initChatMembers();
        initEraseIgnoreDelete();
        initListWidget();
        connectSignals();

        stackedWidget_->setCurrentIndex(main);
        Utils::grabTouchWidget(area_->viewport(), true);
        Utils::grabTouchWidget(mainWidget_);
        connect(QScroller::scroller(area_->viewport()), &QScroller::stateChanged, this, &MenuPage::touchScrollStateChanged, Qt::QueuedConnection);
    }

    void MenuPage::initAvatarAndName()
    {
        {
            avatarName_ = new ClickedWidget(mainWidget_);
            Utils::grabTouchWidget(avatarName_);
            avatarName_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
            auto horLayout = Utils::emptyHLayout(avatarName_);
            horLayout->setContentsMargins(Utils::scale_value(HOR_PADDING), Utils::scale_value(avatar_margin), Utils::scale_value(HOR_PADDING), Utils::scale_value(avatar_margin));
            horLayout->setAlignment(Qt::AlignLeft);
            {
                auto vLayout = Utils::emptyVLayout();
                vLayout->setAlignment(Qt::AlignTop);
                avatar_ = new ContactAvatarWidget(avatarName_, QString(), QString(), Utils::scale_value(avatar_size), true);
                avatar_->setAttribute(Qt::WA_TransparentForMouseEvents);
                vLayout->addWidget(avatar_);
                horLayout->addLayout(vLayout);
            }
            {
                nameLayout_ = Utils::emptyVLayout();
                nameLayout_->setAlignment(Qt::AlignVCenter);
                nameLayout_->setContentsMargins(Utils::scale_value(NAME_MARGIN), 0, 0, 0);
                name_ = new TextEditEx(avatarName_, Fonts::appFontScaled(17, Fonts::FontWeight::Medium), CommonStyle::getColor(CommonStyle::Color::TEXT_PRIMARY), false, false);
                name_->setStyleSheet(qsl("background-color: transparent;"));
                name_->setFrameStyle(QFrame::NoFrame);
                name_->setContentsMargins(0, 0, 0, 0);
                name_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                name_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                name_->setAttribute(Qt::WA_TransparentForMouseEvents);
                name_->setContextMenuPolicy(Qt::NoContextMenu);
                nameLayout_->addWidget(name_);

                description_ = new TextEditEx(avatarName_, Fonts::appFontScaled(15), CommonStyle::getColor(CommonStyle::Color::TEXT_SECONDARY), true, false);
                description_->setFrameStyle(QFrame::NoFrame);
                description_->setContentsMargins(0, 0, 0, 0);
                description_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                description_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                description_->setTextInteractionFlags(Qt::TextEditorInteraction);
                description_->setContextMenuPolicy(Qt::DefaultContextMenu);
                description_->setReadOnly(true);
                description_->setCursorWidth(0);

                nameLayout_->addWidget(description_);

                {
                    auto hLayout = Utils::emptyHLayout();
                    hLayout->setAlignment(Qt::AlignLeft);
                    moreLabel_ = new LabelEx(mainWidget_);
                    moreLabel_->setFont(Fonts::appFontScaled(14));
                    moreLabel_->setColor(CommonStyle::getColor(CommonStyle::Color::GREEN_TEXT));
                    moreLabel_->setText(QT_TRANSLATE_NOOP("sidebar", "More"));
                    moreLabel_->setCursor(QCursor(Qt::PointingHandCursor));
                    hLayout->addWidget(moreLabel_);
                    nameLayout_->addLayout(hLayout);
                }

                horLayout->addLayout(nameLayout_);
            }
            avatarName_->setCursor(QCursor(Qt::PointingHandCursor));
            rootLayout_->addWidget(avatarName_);
        }
    }

    void MenuPage::initAddContactAndSpam()
    {
        addContactSpacerTop_ = new QWidget(mainWidget_);
        addContactSpacerTop_->setFixedSize(1, Utils::scale_value(add_contact_margin));
        Utils::grabTouchWidget(addContactSpacerTop_);
        rootLayout_->addWidget(addContactSpacerTop_);
        {
            auto horLayout = Utils::emptyHLayout();
            horLayout->setContentsMargins(Utils::scale_value(HOR_PADDING), 0, Utils::scale_value(HOR_PADDING), 0);
            horLayout->setAlignment(Qt::AlignLeft);
            addContact_ = new CustomButton(mainWidget_, QString());
            addContact_->setText(QT_TRANSLATE_NOOP("sidebar", "ADD CONTACT"));
            addContact_->setAlign(Qt::AlignHCenter);
            addContact_->setCursor(QCursor(Qt::PointingHandCursor));
            Utils::ApplyStyle(addContact_, CommonStyle::getGreenButtonStyle());
            horLayout->addWidget(addContact_);
            rootLayout_->addLayout(horLayout);
        }

        addContactSpacer_ = new QWidget(mainWidget_);
        addContactSpacer_->setFixedSize(1, Utils::scale_value(add_contact_margin));
        Utils::grabTouchWidget(addContactSpacer_);
        rootLayout_->addWidget(addContactSpacer_);

        spamButton_ = new ActionButton(mainWidget_, qsl(":/resources/i_report_100.png"), QT_TRANSLATE_NOOP("sidebar", "Report spam"), Utils::scale_value(ITEM_HEIGHT), 0, Utils::scale_value(HOR_PADDING));
        spamButton_->setCursor(QCursor(Qt::PointingHandCursor));
        Utils::grabTouchWidget(spamButton_);
        rootLayout_->addWidget(spamButton_);

        firstLine_ = new LineWidget(mainWidget_, 0, 0, 0, 0);
        Utils::grabTouchWidget(firstLine_);
        rootLayout_->addWidget(firstLine_);

        notMemberTopSpacer_ = new QWidget(mainWidget_);
        notMemberTopSpacer_->setFixedHeight(Utils::scale_value(not_member_space));
        rootLayout_->addWidget(notMemberTopSpacer_);
        auto horLayout = Utils::emptyHLayout();
        horLayout->setContentsMargins(Utils::scale_value(HOR_PADDING), 0, Utils::scale_value(HOR_PADDING), 0);
        youAreNotAMember_ = new TextEditEx(mainWidget_, Fonts::appFontScaled(15), CommonStyle::getColor(CommonStyle::Color::TEXT_RED), false, false);
        youAreNotAMember_->setPlainText(QString());
        QTextCursor cursor = youAreNotAMember_->textCursor();
        QString youAreNotAMemberDescription = QT_TRANSLATE_NOOP("sidebar", "Unfortunatelly, you have been deleted and cannot see the members of this chat or message them.");
        Logic::Text2Doc(youAreNotAMemberDescription, cursor, Logic::Text2DocHtmlMode::Pass, false);
        youAreNotAMember_->setFrameStyle(QFrame::NoFrame);
        youAreNotAMember_->setContentsMargins(0, 0, 0, 0);
        youAreNotAMember_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        youAreNotAMember_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        youAreNotAMember_->setContextMenuPolicy(Qt::NoContextMenu);
        horLayout->addWidget(youAreNotAMember_);
        rootLayout_->addLayout(horLayout);
        notMemberBottomSpacer_ = new QWidget(mainWidget_);
        notMemberBottomSpacer_->setFixedHeight(Utils::scale_value(not_member_space));
        rootLayout_->addWidget(notMemberBottomSpacer_);
        youAreNotAMember_->hide();
        notMemberTopSpacer_->hide();
        notMemberBottomSpacer_->hide();
    }

    void MenuPage::initFavoriteNotificationsSearchTheme()
    {
        favoriteButton_ = new ActionButton(mainWidget_, qsl(":/resources/sidebar_favorite_100.png"), QString(), Utils::scale_value(ITEM_HEIGHT), 0, Utils::scale_value(HOR_PADDING));
        favoriteButton_->setCursor(QCursor(Qt::PointingHandCursor));
        Utils::grabTouchWidget(favoriteButton_);
        rootLayout_->addWidget(favoriteButton_);

        copyLink_ = new ActionButton(mainWidget_, qsl(":/resources/i_share_100.png"), QT_TRANSLATE_NOOP("sidebar", "Share link"), Utils::scale_value(ITEM_HEIGHT), 0, Utils::scale_value(HOR_PADDING));
        copyLink_->setCursor(QCursor(Qt::PointingHandCursor));
        Utils::grabTouchWidget(copyLink_);
        rootLayout_->addWidget(copyLink_);

        {
            auto horLayout = Utils::emptyHLayout();
            notificationsButton_ = new CustomButton(mainWidget_, qsl(":/resources/i_notice_100.png"));
            notificationsButton_->setFont(Fonts::appFontScaled(16));
            notificationsButton_->setOffsets(Utils::scale_value(HOR_PADDING), 0);
            notificationsButton_->setText(QT_TRANSLATE_NOOP("sidebar", "Notifications"));
            notificationsButton_->setTextColor("#454545");
            notificationsButton_->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
            notificationsButton_->setAlign(Qt::AlignLeft);
            notificationsButton_->setFocusPolicy(Qt::NoFocus);
            notificationsButton_->setFixedHeight(Utils::scale_value(ITEM_HEIGHT));
            notificationsButton_->adjustSize();
            Utils::grabTouchWidget(notificationsButton_);
            horLayout->addWidget(notificationsButton_);
            notificationsCheckbox_ = new QCheckBox(mainWidget_);
            notificationsCheckbox_->setObjectName(qsl("greenSwitcher"));
            notificationsCheckbox_->adjustSize();
            notificationsCheckbox_->setCursor(QCursor(Qt::PointingHandCursor));
            notificationsCheckbox_->setFixedSize(Utils::scale_value(checkbox_width), Utils::scale_value(checkbox_height));
            notificationsCheckbox_->setStyleSheet(qsl("outline: none;"));
            Utils::grabTouchWidget(notificationsCheckbox_);
            horLayout->addWidget(notificationsCheckbox_);
            horLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Preferred));
            rootLayout_->addLayout(horLayout);
        }

        themesButton_ = new ActionButton(mainWidget_, qsl(":/resources/i_wallpaper_100.png"), QT_TRANSLATE_NOOP("sidebar", "Wallpaper"), Utils::scale_value(ITEM_HEIGHT), 0, Utils::scale_value(HOR_PADDING));
        themesButton_->setCursor(QCursor(Qt::PointingHandCursor));
        Utils::grabTouchWidget(themesButton_);
        rootLayout_->addWidget(themesButton_);

        privacyButton_ = new ActionButton(mainWidget_, qsl(":/resources/i_settings_100.png"), QT_TRANSLATE_NOOP("sidebar", "Chat settings"), Utils::scale_value(ITEM_HEIGHT), 0, Utils::scale_value(HOR_PADDING));
        privacyButton_->setCursor(QCursor(Qt::PointingHandCursor));
        Utils::grabTouchWidget(privacyButton_);
        rootLayout_->addWidget(privacyButton_);

        secondLine_ = new LineWidget(mainWidget_, 0, 0, 0, 0);
        Utils::grabTouchWidget(secondLine_);
        rootLayout_->addWidget(secondLine_);
    }

    void MenuPage::initChatMembers()
    {
        chatMembersModel_ = new Logic::ChatMembersModel(mainWidget_);
        chatMembersModel_->setSelectEnabled(false);
        chatMembersModel_->setFlag(Logic::HasMouseOver);
        delegate_ = new Logic::ContactListItemDelegate(mainWidget_, Logic::MEMBERS_LIST);
        {
            auto horLayout = Utils::emptyHLayout();
            {
                auto verLayout = Utils::emptyVLayout();
                addToChat_ = new ActionButton(mainWidget_, qsl(":/resources/i_plus_100.png"), QT_TRANSLATE_NOOP("sidebar", "Add to chat"), Utils::scale_value(ITEM_HEIGHT), 0, Utils::scale_value(HOR_PADDING));
                addToChat_->setColor(CommonStyle::getColor(CommonStyle::Color::GREEN_TEXT).name());
                addToChat_->setFont(Fonts::appFontScaled(16, Fonts::FontWeight::Medium));
                addToChat_->setFixedHeight(Utils::scale_value(ITEM_HEIGHT));
                addToChat_->setCursor(QCursor(Qt::PointingHandCursor));
                Utils::grabTouchWidget(addToChat_);
                verLayout->addWidget(addToChat_);
                verLayout->setAlignment(Qt::AlignLeft);
                {
                    const auto color = QColor(ql1s("#454545"));
                    admins_ = new ClickedWidget(mainWidget_);
                    admins_->setFixedHeight(Utils::scale_value(ITEM_HEIGHT));
                    admins_->setCursor(QCursor(Qt::PointingHandCursor));
                    auto horLayout2 = Utils::emptyHLayout(admins_);
                    horLayout2->setContentsMargins(Utils::scale_value(LEFT_MARGIN), 0, Utils::scale_value(HOR_PADDING), 0);
                    auto adminsLabel = new LabelEx(admins_);
                    adminsLabel->setColor(color);
                    adminsLabel->setFont(Fonts::appFontScaled(16));
                    adminsLabel->setText(QT_TRANSLATE_NOOP("sidebar", "Admins"));
                    horLayout2->addWidget(adminsLabel);
                    verLayout->addWidget(admins_);
                    Utils::grabTouchWidget(admins_);

                    allMembers_ = new ClickedWidget(mainWidget_);
                    allMembers_->setCursor(QCursor(Qt::PointingHandCursor));
                    allMembers_->setFixedHeight(Utils::scale_value(ITEM_HEIGHT));
                    auto horLayout3 = Utils::emptyHLayout(allMembers_);
                    horLayout3->setContentsMargins(Utils::scale_value(LEFT_MARGIN), 0, Utils::scale_value(HOR_PADDING), 0);
                    allMembersLabel_ = new LabelEx(allMembers_);
                    allMembersLabel_->setColor(color);
                    allMembersLabel_->setFont(Fonts::appFontScaled(16));
                    allMembersLabel_->setText(QT_TRANSLATE_NOOP("sidebar", "Members"));
                    horLayout3->addWidget(allMembersLabel_);
                    allMembersCount_ = new LabelEx(allMembers_);
                    allMembersCount_->setColor(color);
                    allMembersCount_->setFont(Fonts::appFontScaled(16));
                    allMembersCount_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
                    horLayout3->addWidget(allMembersCount_);
                    Utils::grabTouchWidget(allMembers_);
                    verLayout->addWidget(allMembers_);

                    pendingList_ = new ClickedWidget(mainWidget_);
                    pendingList_->setCursor(QCursor(Qt::PointingHandCursor));
                    pendingList_->setFixedHeight(Utils::scale_value(ITEM_HEIGHT));
                    auto horLayout5 = Utils::emptyHLayout(pendingList_);
                    horLayout5->setContentsMargins(Utils::scale_value(LEFT_MARGIN), 0, Utils::scale_value(HOR_PADDING), 0);
                    pendingLabel_ = new LabelEx(pendingList_);
                    pendingLabel_->setColor(color);
                    pendingLabel_->setFont(Fonts::appFontScaled(16));
                    pendingLabel_->setText(QT_TRANSLATE_NOOP("sidebar", "Waiting for approval"));
                    horLayout5->addWidget(pendingLabel_);
                    pendingCount_ = new LabelEx(pendingList_);
                    pendingCount_->setColor(color);
                    pendingCount_->setFont(Fonts::appFontScaled(16));
                    pendingCount_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
                    horLayout5->addWidget(pendingCount_);
                    Utils::grabTouchWidget(pendingList_);
                    verLayout->addWidget(pendingList_);

                    blockList_ = new ClickedWidget(mainWidget_);
                    blockList_->setCursor(QCursor(Qt::PointingHandCursor));
                    blockList_->setFixedHeight(Utils::scale_value(ITEM_HEIGHT));
                    auto horLayout4 = Utils::emptyHLayout(blockList_);
                    horLayout4->setContentsMargins(Utils::scale_value(LEFT_MARGIN), 0, Utils::scale_value(HOR_PADDING), 0);
                    blockLabel_ = new LabelEx(blockList_);
                    blockLabel_->setColor(color);
                    blockLabel_->setFont(Fonts::appFontScaled(16));
                    blockLabel_->setText(QT_TRANSLATE_NOOP("sidebar", "Blocked people"));
                    horLayout4->addWidget(blockLabel_);
                    blockCount_ = new LabelEx(blockList_);
                    blockCount_->setColor(color);
                    blockCount_->setFont(Fonts::appFontScaled(16));
                    blockCount_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
                    horLayout4->addWidget(blockCount_);
                    Utils::grabTouchWidget(blockList_);
                    verLayout->addWidget(blockList_);
                }
                horLayout->addLayout(verLayout);
            }
            rootLayout_->addLayout(horLayout);
        }

        thirdLine_ = new LineWidget(mainWidget_, 0, 0, 0, 0);
        Utils::grabTouchWidget(thirdLine_);
        rootLayout_->addWidget(thirdLine_);
    }

    void MenuPage::initEraseIgnoreDelete()
    {
        eraseHistoryButton_ = new ActionButton(mainWidget_, qsl(":/resources/i_history_100.png"), QT_TRANSLATE_NOOP("sidebar", "Clear history"), Utils::scale_value(ITEM_HEIGHT), 0, Utils::scale_value(HOR_PADDING));
        eraseHistoryButton_->setCursor(QCursor(Qt::PointingHandCursor));
        Utils::grabTouchWidget(eraseHistoryButton_);
        rootLayout_->addWidget(eraseHistoryButton_);

        ignoreButton_ = new ActionButton(mainWidget_, qsl(":/resources/i_ignore_100.png"), QT_TRANSLATE_NOOP("sidebar", "Ignore"), Utils::scale_value(ITEM_HEIGHT), 0, Utils::scale_value(HOR_PADDING));
        ignoreButton_->setCursor(QCursor(Qt::PointingHandCursor));
        Utils::grabTouchWidget(ignoreButton_);
        rootLayout_->addWidget(ignoreButton_);

        quitAndDeleteButton_ = new ActionButton(mainWidget_, qsl(":/resources/i_delete_100.png"), QT_TRANSLATE_NOOP("sidebar", "Leave and delete"), Utils::scale_value(ITEM_HEIGHT), 0, Utils::scale_value(HOR_PADDING));
        quitAndDeleteButton_->setCursor(QCursor(Qt::PointingHandCursor));
        quitAndDeleteButton_->setColor(qsl("#e83b3f"));
        Utils::grabTouchWidget(quitAndDeleteButton_);
        rootLayout_->addWidget(quitAndDeleteButton_);

        spamButtonAuth_ = new ActionButton(mainWidget_, qsl(":/resources/i_report_100.png"), QT_TRANSLATE_NOOP("sidebar", "Report spam"), Utils::scale_value(ITEM_HEIGHT), 0, Utils::scale_value(HOR_PADDING));
        spamButtonAuth_->setCursor(QCursor(Qt::PointingHandCursor));
        Utils::grabTouchWidget(spamButtonAuth_);
        rootLayout_->addWidget(spamButtonAuth_);

        deleteButton_ = new ActionButton(mainWidget_, qsl(":/resources/i_delete_100.png"), QT_TRANSLATE_NOOP("sidebar", "Delete"), Utils::scale_value(ITEM_HEIGHT), 0, Utils::scale_value(HOR_PADDING));
        deleteButton_->setCursor(QCursor(Qt::PointingHandCursor));
        deleteButton_->setColor(qsl("#e83b3f"));
        Utils::grabTouchWidget(deleteButton_);
        rootLayout_->addWidget(deleteButton_);

        rootLayout_->addSpacerItem(new QSpacerItem(0, QWIDGETSIZE_MAX, QSizePolicy::Preferred, QSizePolicy::Expanding));

    }

    void MenuPage::initListWidget()
    {
        listWidget_ = new QWidget(stackedWidget_);
        stackedWidget_->insertWidget(list, listWidget_);
        auto vLayout = Utils::emptyVLayout(listWidget_);
        vLayout->setAlignment(Qt::AlignTop);
        {
            backButton_ = new CustomButton(listWidget_, qsl(":/controls/arrow_left_100"));
            backButton_->setFixedSize(QSize(Utils::scale_value(BACK_WIDTH), Utils::scale_value(BACK_HEIGHT)));
            backButton_->setStyleSheet(qsl("background: transparent; border-style: none;"));
            backButton_->setCursor(QCursor(Qt::PointingHandCursor));
            backButton_->setOffsets(Utils::scale_value(HOR_PADDING), 0);
            backButton_->setAlign(Qt::AlignLeft);
            auto topWidget = new QWidget(listWidget_);
            auto hLayout = Utils::emptyHLayout(topWidget);
            hLayout->setContentsMargins(0, 0, Utils::scale_value(HOR_PADDING + BACK_WIDTH), 0);
            hLayout->addWidget(backButton_);
            listLabel_ = new LabelEx(listWidget_);
            Utils::grabTouchWidget(listLabel_);
            listLabel_->setAlignment(Qt::AlignCenter);
            listLabel_->setFixedHeight(Utils::scale_value(40));
            listLabel_->setColor(CommonStyle::getColor(CommonStyle::Color::TEXT_LIGHT));
            listLabel_->setFont(Fonts::appFontScaled(16, Fonts::FontWeight::Medium));
            listLabel_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
            hLayout->addWidget(listLabel_);
            vLayout->addWidget(topWidget);

            privacyWidget_ = new QWidget(listWidget_);
            auto privacyLayout = Utils::emptyVLayout(privacyWidget_);
            privacyLayout->setContentsMargins(Utils::scale_value(HOR_PADDING), 0, Utils::scale_value(HOR_PADDING), 0);
            {
                auto vlayout = Utils::emptyVLayout();
                vlayout->setContentsMargins(0, 0, 0, Utils::scale_value(checkbox_bottom_margin));
                vlayout->setAlignment(Qt::AlignTop);
                auto horLayout = Utils::emptyHLayout();
                linkToChat_ = new LabelEx(listWidget_);
                linkToChat_->setFont(Fonts::appFontScaled(16));
                linkToChat_->setColor(CommonStyle::getColor(CommonStyle::Color::TEXT_PRIMARY));
                linkToChat_->setText(QT_TRANSLATE_NOOP("groupchats", "Link to chat"));
                linkToChat_->setWordWrap(true);
                horLayout->addWidget(linkToChat_);

                linkToChatCheckBox_ = new QCheckBox(listWidget_);
                linkToChatCheckBox_->setObjectName(qsl("greenSwitcher"));
                linkToChatCheckBox_->adjustSize();
                linkToChatCheckBox_->setCursor(QCursor(Qt::PointingHandCursor));
                linkToChatCheckBox_->setFixedSize(Utils::scale_value(checkbox_width), Utils::scale_value(checkbox_height));
                horLayout->addWidget(linkToChatCheckBox_);

                auto linkToChatLayout = Utils::emptyHLayout();
                linkToChatAbout_ = new LabelEx(listWidget_);
                linkToChatAbout_->setFont(Fonts::appFontScaled(13));
                linkToChatAbout_->setColor(CommonStyle::getColor(CommonStyle::Color::TEXT_LIGHT));
                linkToChatAbout_->setText(QT_TRANSLATE_NOOP("groupchats", "Ability to join chat by link"));
                linkToChatAbout_->setWordWrap(true);
                linkToChatLayout->addWidget(linkToChatAbout_);
                vlayout->addLayout(horLayout);
                vlayout->addLayout(linkToChatLayout);
                privacyLayout->addLayout(vlayout);
            }
            {
                auto vlayout = Utils::emptyVLayout();
                vlayout->setContentsMargins(0, 0, 0, Utils::scale_value(checkbox_bottom_margin));
                vlayout->setAlignment(Qt::AlignTop);
                auto horLayout = Utils::emptyHLayout();
                publicButton_ = new LabelEx(listWidget_);
                publicButton_->setFont(Fonts::appFontScaled(16));
                publicButton_->setColor(CommonStyle::getColor(CommonStyle::Color::TEXT_PRIMARY));
                publicButton_->setText(QT_TRANSLATE_NOOP("groupchats", "Public chat"));
                publicButton_->setWordWrap(true);
                horLayout->addWidget(publicButton_);

                publicCheckBox_ = new QCheckBox(listWidget_);
                publicCheckBox_->setObjectName(qsl("greenSwitcher"));
                publicCheckBox_->adjustSize();
                publicCheckBox_->setCursor(QCursor(Qt::PointingHandCursor));
                publicCheckBox_->setFixedSize(Utils::scale_value(checkbox_width), Utils::scale_value(checkbox_height));
                horLayout->addWidget(publicCheckBox_);

                auto publicLayout = Utils::emptyHLayout();
                publicAbout_ = new LabelEx(listWidget_);
                publicAbout_->setFont(Fonts::appFontScaled(13));
                publicAbout_->setColor(CommonStyle::getColor(CommonStyle::Color::TEXT_LIGHT));
                publicAbout_->setText(QT_TRANSLATE_NOOP("groupchats", "The chat will appear in the app's showcase and any user can find it in the list"));
                publicAbout_->setWordWrap(true);
                publicLayout->addWidget(publicAbout_);
                vlayout->addLayout(horLayout);
                vlayout->addLayout(publicLayout);
                privacyLayout->addLayout(vlayout);
            }
            {
                auto vlayout = Utils::emptyVLayout();
                vlayout->setContentsMargins(0, 0, 0, Utils::scale_value(checkbox_bottom_margin));
                vlayout->setAlignment(Qt::AlignTop);
                auto horLayout = Utils::emptyHLayout();
                readOnly_ = new LabelEx(listWidget_);
                readOnly_->setFont(Fonts::appFontScaled(16));
                readOnly_->setColor(CommonStyle::getColor(CommonStyle::Color::TEXT_PRIMARY));
                readOnly_->setText(QT_TRANSLATE_NOOP("groupchats", "Read only"));
                readOnly_->setWordWrap(true);
                horLayout->addWidget(readOnly_);

                readOnlyCheckBox_ = new QCheckBox(listWidget_);
                readOnlyCheckBox_->setObjectName(qsl("greenSwitcher"));
                readOnlyCheckBox_->adjustSize();
                readOnlyCheckBox_->setCursor(QCursor(Qt::PointingHandCursor));
                readOnlyCheckBox_->setFixedSize(Utils::scale_value(checkbox_width), Utils::scale_value(checkbox_height));
                horLayout->addWidget(readOnlyCheckBox_);

                auto readOnlyLayout = Utils::emptyHLayout();
                readOnlyAbout_ = new LabelEx(listWidget_);
                readOnlyAbout_->setFont(Fonts::appFontScaled(13));
                readOnlyAbout_->setColor(CommonStyle::getColor(CommonStyle::Color::TEXT_LIGHT));
                readOnlyAbout_->setText(QT_TRANSLATE_NOOP("groupchats", "New members can read, approved by admin members can send"));
                readOnlyAbout_->setWordWrap(true);
                readOnlyLayout->addWidget(readOnlyAbout_);
                vlayout->addLayout(horLayout);
                vlayout->addLayout(readOnlyLayout);
                privacyLayout->addLayout(vlayout);
            }
            {
                auto vlayout = Utils::emptyVLayout();
                vlayout->setContentsMargins(0, 0, 0, Utils::scale_value(checkbox_bottom_margin));
                vlayout->setAlignment(Qt::AlignTop);
                auto horLayout = Utils::emptyHLayout();
                approvedButton_ = new LabelEx(listWidget_);
                approvedButton_->setFont(Fonts::appFontScaled(16));
                approvedButton_->setColor(CommonStyle::getColor(CommonStyle::Color::TEXT_PRIMARY));
                approvedButton_->setText(QT_TRANSLATE_NOOP("groupchats", "Join with Approval"));
                approvedButton_->setWordWrap(true);
                horLayout->addWidget(approvedButton_);

                approvedCheckBox_ = new QCheckBox(listWidget_);
                approvedCheckBox_->setObjectName("greenSwitcher");
                approvedCheckBox_->adjustSize();
                approvedCheckBox_->setCursor(QCursor(Qt::PointingHandCursor));
                approvedCheckBox_->setFixedSize(Utils::scale_value(checkbox_width), Utils::scale_value(checkbox_height));
                horLayout->addWidget(approvedCheckBox_);

                auto approvalLayout = Utils::emptyHLayout();
                approvalAbout_ = new LabelEx(listWidget_);
                approvalAbout_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
                approvalAbout_->setFont(Fonts::appFontScaled(13));
                approvalAbout_->setColor(CommonStyle::getColor(CommonStyle::Color::TEXT_LIGHT));
                approvalAbout_->setText(QT_TRANSLATE_NOOP("groupchats", "Admin approval required to join"));
                approvalAbout_->setWordWrap(true);
                approvalLayout->addWidget(approvalAbout_);
                vlayout->addLayout(horLayout);
                vlayout->addLayout(approvalLayout);
                privacyLayout->addLayout(vlayout);
            }
            {
                auto vlayout = Utils::emptyVLayout();
                vlayout->setContentsMargins(0, 0, 0, Utils::scale_value(checkbox_bottom_margin));
                vlayout->setAlignment(Qt::AlignTop);
                auto horLayout = Utils::emptyHLayout();
                ageRestrictions_ = new LabelEx(listWidget_);
                ageRestrictions_->setFont(Fonts::appFontScaled(16));
                ageRestrictions_->setColor(CommonStyle::getColor(CommonStyle::Color::TEXT_PRIMARY));
                ageRestrictions_->setText(QT_TRANSLATE_NOOP("groupchats", "Age restriction"));
                ageRestrictions_->setWordWrap(true);
                horLayout->addWidget(ageRestrictions_);

                ageCheckBox_ = new QCheckBox(listWidget_);
                ageCheckBox_->setObjectName(qsl("greenSwitcher"));
                ageCheckBox_->adjustSize();
                ageCheckBox_->setCursor(QCursor(Qt::PointingHandCursor));
                ageCheckBox_->setFixedSize(Utils::scale_value(checkbox_width), Utils::scale_value(checkbox_height));
                horLayout->addWidget(ageCheckBox_);

                auto ageLayout = Utils::emptyHLayout();
                ageAbout_ = new LabelEx(listWidget_);
                ageAbout_->setFont(Fonts::appFontScaled(13));
                ageAbout_->setColor(CommonStyle::getColor(CommonStyle::Color::TEXT_LIGHT));
                ageAbout_->setText(QT_TRANSLATE_NOOP("groupchats", "Members must be of legal age to join"));
                ageAbout_->setWordWrap(true);
                ageLayout->addWidget(ageAbout_);
                vlayout->addLayout(horLayout);
                vlayout->addLayout(ageLayout);
                privacyLayout->addLayout(vlayout);
            }

            vLayout->addWidget(privacyWidget_);

            contactListWidget_ = new QWidget(listWidget_);
            auto contactListLayout = Utils::emptyVLayout(contactListWidget_);
            {
                auto searchLayout = Utils::emptyHLayout();
                searchWidget_ = new SearchWidget(listWidget_, Utils::scale_value(search_hor_spacing));
                searchWidget_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
                searchLayout->addWidget(searchWidget_);
                searchLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Preferred));
                contactListLayout->addLayout(searchLayout);
                contactListLayout->addSpacerItem(new QSpacerItem(0, Utils::scale_value(search_ver_spacing), QSizePolicy::Preferred, QSizePolicy::Fixed));
            }
            cl_ = new Ui::ContactListWidget(listWidget_, Logic::MEMBERS_LIST, chatMembersModel_);
            cl_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
            cl_->setContentsMargins(0, 0, 0, 0);
            cl_->setClDelegate(delegate_);
            cl_->getSearchModel()->setSelectEnabled(false);
            contactListLayout->addWidget(cl_);
            vLayout->addWidget(contactListWidget_);
            {
                approveAllWidget_ = new QWidget(listWidget_);
                auto verLayout = Utils::emptyVLayout(approveAllWidget_);
                verLayout->setContentsMargins(0, 0, 0, Utils::scale_value(HOR_PADDING));
                approveAllLine_ = new LineWidget(listWidget_, 0, 0, 0, 0);
                verLayout->addWidget(approveAllLine_);
                auto hlayout = Utils::emptyHLayout();
                hlayout->setContentsMargins(0, 0, Utils::scale_value(2 * HOR_PADDING), 0);
                hlayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding));
                approveAll_ = new LabelEx(listWidget_);
                approveAll_->setFont(Fonts::appFontScaled(16));
                approveAll_->setColor(CommonStyle::getColor(CommonStyle::Color::GREEN_TEXT));
                approveAll_->setText(QT_TRANSLATE_NOOP("sidebar", "Approve All"));
                approveAll_->setCursor(QCursor(Qt::PointingHandCursor));
                hlayout->addWidget(approveAll_);
                verLayout->addLayout(hlayout);
            }
            vLayout->addWidget(approveAllWidget_);
        }
    }

    void MenuPage::connectSignals()
    {
        connect(favoriteButton_, &Ui::ActionButton::clicked, this, &MenuPage::favoritesClicked, Qt::QueuedConnection);
        connect(copyLink_, &Ui::ActionButton::clicked, this, &MenuPage::copyLinkClicked, Qt::QueuedConnection);
        connect(themesButton_, &Ui::ActionButton::clicked, this, &MenuPage::themesClicked, Qt::QueuedConnection);
        connect(privacyButton_, &Ui::ActionButton::clicked, this, &MenuPage::privacyClicked, Qt::QueuedConnection);
        connect(eraseHistoryButton_, &Ui::ActionButton::clicked, this, &MenuPage::eraseHistoryClicked, Qt::QueuedConnection);
        connect(ignoreButton_, &Ui::ActionButton::clicked, this, &MenuPage::ignoreClicked, Qt::QueuedConnection);
        connect(notificationsCheckbox_, &QCheckBox::stateChanged, this, &MenuPage::notificationsChecked, Qt::QueuedConnection);
        connect(publicCheckBox_, &QCheckBox::stateChanged, this, &MenuPage::publicChanged, Qt::QueuedConnection);
        connect(linkToChatCheckBox_, &QCheckBox::stateChanged, this, &MenuPage::linkToChatClicked, Qt::QueuedConnection);
        connect(ageCheckBox_, &QCheckBox::stateChanged, this, &MenuPage::ageClicked, Qt::QueuedConnection);
        connect(readOnlyCheckBox_, &QCheckBox::stateChanged, this, &MenuPage::readOnlyClicked, Qt::QueuedConnection);
        connect(approvedCheckBox_, &QCheckBox::stateChanged, this, &MenuPage::approvedChanged, Qt::QueuedConnection);
        connect(quitAndDeleteButton_, &Ui::ActionButton::clicked, this, &MenuPage::quitClicked, Qt::QueuedConnection);
        connect(addToChat_, &Ui::ActionButton::clicked, this, &MenuPage::addToChatClicked, Qt::QueuedConnection);
        connect(addContact_, &Ui::CustomButton::clicked, this, &MenuPage::addContactClicked, Qt::QueuedConnection);
        connect(spamButton_, &Ui::ActionButton::clicked, this, &MenuPage::spamClicked, Qt::QueuedConnection);
        connect(spamButtonAuth_, &Ui::ActionButton::clicked, this, &MenuPage::spamClicked, Qt::QueuedConnection);
        connect(deleteButton_, &Ui::ActionButton::clicked, this, &MenuPage::removeClicked, Qt::QueuedConnection);

        connect(avatarName_, &Ui::ClickedWidget::clicked, this, &MenuPage::avatarClicked, Qt::QueuedConnection);

        connect(allMembers_, &Ui::ClickedWidget::clicked, this, &MenuPage::allMemebersClicked, Qt::QueuedConnection);
        connect(backButton_, &Ui::CustomButton::clicked, this, &MenuPage::backButtonClicked, Qt::QueuedConnection);

        connect(Logic::getRecentsModel(), &Logic::RecentsModel::favoriteChanged, this, &MenuPage::contactChanged, Qt::QueuedConnection);
        connect(Logic::getContactListModel(), &Logic::ContactListModel::contactChanged, this, &MenuPage::contactChanged, Qt::QueuedConnection);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::chatInfo, this, &MenuPage::chatInfo);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::chatBlocked, this, &MenuPage::chatBlocked);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::chatPending, this, &MenuPage::chatPending);
        connect(cl_, static_cast<void(Ui::ContactListWidget::*)(const QString&)>(&Ui::ContactListWidget::itemClicked), this, &MenuPage::contactClicked, Qt::QueuedConnection);
        cl_->connectSearchWidget(searchWidget_);

        connect(moreLabel_, &Ui::LabelEx::clicked, this, &MenuPage::moreClicked, Qt::QueuedConnection);
        connect(admins_, &Ui::ClickedWidget::clicked, this, &MenuPage::adminsClicked, Qt::QueuedConnection);
        connect(blockList_, &Ui::ClickedWidget::clicked, this, &MenuPage::blockedClicked, Qt::QueuedConnection);
        connect(pendingList_, &Ui::ClickedWidget::clicked, this, &MenuPage::pendingClicked, Qt::QueuedConnection);
        connect(Logic::GetMessagesModel(), &Logic::MessagesModel::chatEvent, this, &MenuPage::chatEvent, Qt::QueuedConnection);

        connect(approveAll_, &Ui::LabelEx::clicked, this, &MenuPage::approveAllClicked, Qt::QueuedConnection);

        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::setChatRoleResult, this, &MenuPage::actionResult, Qt::QueuedConnection);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::blockMemberResult, this, &MenuPage::actionResult, Qt::QueuedConnection);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::pendingListResult, this, &MenuPage::actionResult, Qt::QueuedConnection);

        connect(Logic::getContactListModel(), &Logic::ContactListModel::youRoleChanged, this, &MenuPage::chatRoleChanged, Qt::QueuedConnection);
    }

    void MenuPage::initDescription(const QString& description, bool full)
    {
        QString normalizedDesc = description == ql1s(" ") ? QString() : description;
        description_->setPlainText(QString());
        QTextCursor cursorDesc = description_->textCursor();
        Logic::Text2Doc(normalizedDesc, cursorDesc, Logic::Text2DocHtmlMode::Pass, false);
        moreLabel_->setVisible(normalizedDesc.length() > desc_length);
        if (!full)
        {
            if (normalizedDesc.length() > desc_length)
            {
                const QString newDescription = normalizedDesc.leftRef(desc_length) % ql1s("...");

                description_->setPlainText(QString());
                cursorDesc = description_->textCursor();
                Logic::Text2Doc(newDescription, cursorDesc, Logic::Text2DocHtmlMode::Pass, false);

                moreLabel_->show();
            }
            moreLabel_->setText(QT_TRANSLATE_NOOP("sidebar", "more"));
        }
        else
        {
            moreLabel_->setText(QT_TRANSLATE_NOOP("sidebar", "hide"));
        }


        auto role = Logic::getContactListModel()->getYourRole(currentAimId_);
        bool showEdit = role != ql1s("notamember") && role != ql1s("readonly");

        if (showEdit && info_ && (info_->Controlled_ == false || info_->YourRole_ == ql1s("admin") || info_->Creator_ == MyInfo()->aimId()))
        {
            moreLabel_->setText(QT_TRANSLATE_NOOP("sidebar", "edit"));
            moreLabel_->setVisible(true);
        }

        description_->setVisible(!normalizedDesc.isEmpty());

        nameLayout_->setAlignment(normalizedDesc.isEmpty() ? Qt::AlignVCenter : Qt::AlignTop);
        nameLayout_->invalidate();

        updateWidth();
    }

    void MenuPage::blockUser(const QString& aimId, bool blockUser)
    {
        auto cont = chatMembersModel_->getMemberItem(aimId);
        if (!cont)
            return;

        const auto confirmed = Utils::GetConfirmationWithTwoButtons(
            QT_TRANSLATE_NOOP("popup_window", "CANCEL"),
            QT_TRANSLATE_NOOP("popup_window", "YES"),
            blockUser ? QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to block user in this chat?") : QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to unblock user?"),
            cont->getFriendly(),
            nullptr);

        if (confirmed)
        {
            Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
            collection.set_value_as_qstring("aimid", currentAimId_);
            collection.set_value_as_qstring("contact", aimId);
            collection.set_value_as_bool("block", blockUser);
            Ui::GetDispatcher()->post_message_to_core(qsl("chats/block"), collection.get());

            if (!blockUser)
            {
                if (info_ && info_->BlockedCount_ == 1)
                    backButtonClicked();
                chatMembersModel_->loadBlocked();
            }
        }
    }

    void MenuPage::readOnly(const QString& _aimId, bool _readonly)
    {
        auto cont = chatMembersModel_->getMemberItem(_aimId);
        if (!cont)
            return;

        const auto confirmed = Utils::GetConfirmationWithTwoButtons(
            QT_TRANSLATE_NOOP("popup_window", "CANCEL"),
            QT_TRANSLATE_NOOP("popup_window", "YES"),
            _readonly ? QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to ban user to write in this chat?") : QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to allow user to write in this chat?"),
            cont->getFriendly(),
            nullptr);

        if (confirmed)
        {
            Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
            collection.set_value_as_qstring("aimid", currentAimId_);
            collection.set_value_as_qstring("contact", _aimId);
            collection.set_value_as_qstring("role", _readonly ? qsl("readonly") : qsl("member"));
            Ui::GetDispatcher()->post_message_to_core(qsl("chats/role/set"), collection.get());
        }
    }

    void MenuPage::actionResult(int)
    {
        if (currentTab_ == block)
            chatMembersModel_->loadBlocked();
        else if (currentTab_ == pending)
            chatMembersModel_->loadPending();
        else
            chatMembersModel_->loadAllMembers();
    }

    void MenuPage::approveAllClicked()
    {
        if (currentTab_ != pending)
            return;

        const auto& members = chatMembersModel_->getMembers();
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("aimid", currentAimId_);

        core::ifptr<core::iarray> contacts_array(collection->create_array());
        contacts_array->reserve(members.size());

        for (const auto& iter : members)
        {
            core::ifptr<core::ivalue> val(collection->create_value());
            const auto aimIdUtf8 = iter.AimId_.toUtf8();
            val->set_as_string(aimIdUtf8.data(), aimIdUtf8.size());
            contacts_array->push_back(val.get());
        }

        collection.set_value_as_array("contacts", contacts_array.get());
        collection.set_value_as_bool("approve", true);
        Ui::GetDispatcher()->post_message_to_core(qsl("chats/pending/resolve"), collection.get());

        backButtonClicked();
    }

    void MenuPage::changeRole(const QString& aimId, bool moder)
    {
        auto cont = chatMembersModel_->getMemberItem(aimId);
        if (!cont)
            return;

        const auto confirmed = Utils::GetConfirmationWithTwoButtons(
            QT_TRANSLATE_NOOP("popup_window", "CANCEL"),
            QT_TRANSLATE_NOOP("popup_window", "YES"),
            moder ? QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to make user admin in this chat?") : QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to revoke admin role?"),
            cont->getFriendly(),
            nullptr);

        if (confirmed)
        {
            Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
            collection.set_value_as_qstring("aimid", currentAimId_);
            collection.set_value_as_qstring("contact", aimId);
            collection.set_value_as_qstring("role", moder ? qsl("moder") : qsl("member"));
            Ui::GetDispatcher()->post_message_to_core(qsl("chats/role/set"), collection.get());
        }
    }

    void MenuPage::approve(const QString& aimId, bool approve)
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("aimid", currentAimId_);
        core::ifptr<core::iarray> contacts_array(collection->create_array());
        contacts_array->reserve(1);
        core::ifptr<core::ivalue> val(collection->create_value());
        const auto aimIdUtf8 = aimId.toUtf8();
        val->set_as_string(aimIdUtf8.data(), aimIdUtf8.size());
        contacts_array->push_back(val.get());
        collection.set_value_as_array("contacts", contacts_array.get());
        collection.set_value_as_bool("approve", approve);
        Ui::GetDispatcher()->post_message_to_core(qsl("chats/pending/resolve"), collection.get());

        if (info_ && info_->PendingCount_ == 1)
            backButtonClicked();
    }

    void MenuPage::changeTab(int tab)
    {
        stackedWidget_->setCurrentIndex(list);

        switch (tab)
        {
        case all:
            chatMembersModel_->clear();
            chatMembersModel_->loadAllMembers(currentAimId_, 0);
            if (Logic::getContactListModel()->isChat(currentAimId_))
                delegate_->setRegim(
                (chatMembersModel_->isAdmin() || chatMembersModel_->isModer()) ?
                    Logic::ADMIN_MEMBERS : Logic::CONTACT_LIST
                );
            else
                delegate_->setRegim(Logic::CONTACT_LIST);

            listLabel_->setText(QT_TRANSLATE_NOOP("sidebar", "Members"));
            currentTab_ = all;
            approveAllWidget_->hide();
            privacyWidget_->hide();
            contactListWidget_->show();
            break;

        case block:
            chatMembersModel_->loadBlocked();
            if (chatMembersModel_->isAdmin() || chatMembersModel_->isModer())
                delegate_->setRegim(Logic::MEMBERS_LIST);
            else
                delegate_->setRegim(Logic::CONTACT_LIST);
            listLabel_->setText(QT_TRANSLATE_NOOP("sidebar", "Blocked people"));
            currentTab_ = block;
            approveAllWidget_->hide();
            privacyWidget_->hide();
            contactListWidget_->show();
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::livechat_blocked);
            break;

        case admins:
            chatMembersModel_->loadAllMembers(currentAimId_, 0);
            chatMembersModel_->adminsOnly();
            if (chatMembersModel_->isAdmin())
                delegate_->setRegim(Logic::MEMBERS_LIST);
            else
                delegate_->setRegim(Logic::CONTACT_LIST);
            listLabel_->setText(QT_TRANSLATE_NOOP("sidebar", "Admins"));
            currentTab_ = admins;
            approveAllWidget_->hide();
            privacyWidget_->hide();
            contactListWidget_->show();
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::livechat_admins);
            break;

        case pending:
            chatMembersModel_->loadPending();
            delegate_->setRegim(Logic::PENDING_MEMBERS);
            listLabel_->setText(QT_TRANSLATE_NOOP("sidebar", "Waiting for approval"));
            currentTab_ = pending;
            approveAllWidget_->show();
            privacyWidget_->hide();
            contactListWidget_->show();
            break;

        case privacy:
            listLabel_->setText(QT_TRANSLATE_NOOP("sidebar", "Chat settings"));
            currentTab_ = privacy;
            approveAllWidget_->hide();
            privacyWidget_->show();
            contactListWidget_->hide();
            break;

        default:
            assert(!"wrong index of sidebar page");
            break;
        }

        if (searchWidget_)
            searchWidget_->searchCompleted();
    }

    void MenuPage::contactChanged(const QString& aimid)
    {
        if (aimid != currentAimId_ || currentTab_ != all)
            return;

        int index = stackedWidget_->currentIndex();
        int curTab = currentTab_;
        initFor(currentAimId_);
        stackedWidget_->setCurrentIndex(index);
        currentTab_ = curTab;
    }

    void MenuPage::contactClicked(const QString& aimId)
    {
        auto global_cursor_pos = mapFromGlobal(QCursor::pos());
        if (!rect().contains(global_cursor_pos))
            return;

        if (global_cursor_pos.x() > width_)
            return;

        auto minXofDeleteImage = ::ContactList::GetXOfRemoveImg(width_);
        auto minXOfApprove = minXofDeleteImage - Utils::scale_value(48);
        bool advMenu = (chatMembersModel_->isAdmin() || chatMembersModel_->isModer() || (info_ && info_->Controlled_ == false));

        if (advMenu && global_cursor_pos.x() > minXofDeleteImage &&
            global_cursor_pos.x() <= (minXofDeleteImage + Utils::scale_value(20)) &&
            chatMembersModel_)
        {
            if (currentTab_ == all && (chatMembersModel_->isModer() || chatMembersModel_->isAdmin()))
            {
                auto menu = new ContextMenu(this);
                auto cont = chatMembersModel_->getMemberItem(aimId);
                bool myInfo = cont->AimId_ == MyInfo()->aimId();
                if (cont->Role_ != ql1s("admin") && chatMembersModel_->isAdmin())
                {
                    if (cont->Role_ == ql1s("moder"))
                        menu->addActionWithIcon(
                            QIcon(Utils::parse_image_name(qsl(":/context_menu/admin_off_100"))),
                            QT_TRANSLATE_NOOP("sidebar", "Revoke admin role"),
                            makeData(qsl("revoke_admin"), aimId));
                    else
                        menu->addActionWithIcon(
                            QIcon(Utils::parse_image_name(qsl(":/context_menu/admin_100"))),
                            QT_TRANSLATE_NOOP("sidebar", "Make admin"),
                            makeData(qsl("make_admin"), aimId));
                }

                if (chatMembersModel_->isAdmin() || chatMembersModel_->isModer())
                {
                    if (cont->Role_ == ql1s("member"))
                        menu->addActionWithIcon(
                        QIcon(Utils::parse_image_name(qsl(":/context_menu/readonly_100"))),
                        QT_TRANSLATE_NOOP("sidebar", "Ban to write"),
                        makeData(qsl("readonly"), aimId));
                    else if (cont->Role_ == ql1s("readonly"))
                        menu->addActionWithIcon(
                        QIcon(Utils::parse_image_name(qsl(":/context_menu/readonly_off_100"))),
                        QT_TRANSLATE_NOOP("sidebar", "Allow to write"),
                        makeData(qsl("revoke_readonly"), aimId));
                }

                if ((cont->Role_ != ql1s("admin") && cont->Role_ != ql1s("moder")) || myInfo)
                    menu->addActionWithIcon(
                        QIcon(Utils::parse_image_name(qsl(":/context_menu/delete_100"))),
                        QT_TRANSLATE_NOOP("sidebar", "Delete from chat"),
                        makeData(qsl("remove"), aimId));

                if (!myInfo)
                    menu->addActionWithIcon(
                        QIcon(Utils::parse_image_name(qsl(":/context_menu/profile_100"))),
                        QT_TRANSLATE_NOOP("sidebar", "Profile"),
                        makeData(qsl("profile"), aimId));

                if (cont->Role_ != ql1s("admin") && cont->Role_ != ql1s("moder"))
                    menu->addActionWithIcon(
                        QIcon(Utils::parse_image_name(qsl(":/context_menu/block_100"))),
                        QT_TRANSLATE_NOOP("sidebar", "Block"),
                        makeData(qsl("block"), aimId));

                if (!myInfo)
                    menu->addActionWithIcon(
                        QIcon(Utils::parse_image_name(qsl(":/context_menu/spam_100"))),
                        QT_TRANSLATE_NOOP("sidebar", "Report spam"),
                        makeData(qsl("spam"), aimId));

                menu->invertRight(true);
                connect(menu, &ContextMenu::triggered, this, &MenuPage::menu, Qt::QueuedConnection);
                connect(menu, &ContextMenu::triggered, menu, &ContextMenu::deleteLater, Qt::QueuedConnection);
                connect(menu, &ContextMenu::aboutToHide, menu, &ContextMenu::deleteLater, Qt::QueuedConnection);
                menu->popup(QCursor::pos());
            }
            else if (currentTab_ == block)
            {
                blockUser(aimId, false);
            }
            else if (currentTab_ == admins)
            {
                changeRole(aimId, false);
            }
            else if (currentTab_ == pending)
            {
                approve(aimId, false);
            }
            else
            {
                deleteMemberDialog(chatMembersModel_, aimId, Logic::MEMBERS_LIST, this);
            }
        }
        else if (currentTab_ == pending &&
            global_cursor_pos.x() > minXOfApprove &&
            global_cursor_pos.x() <= (minXOfApprove + Utils::scale_value(32)) &&
            chatMembersModel_)
        {
            approve(aimId, true);
        }
        else
        {
            Utils::InterConnector::instance().showSidebar(aimId, profile_page);
        }
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::profile_members_list);
    }

    void MenuPage::moreClicked()
    {
        const QString text = moreLabel_->text();
        if (text == QT_TRANSLATE_NOOP("sidebar", "more"))
            initDescription(info_.get() ? info_->About_ : QString(), true);
        else if (text == QT_TRANSLATE_NOOP("sidebar", "hide"))
            initDescription(info_.get() ? info_->About_ : QString(), false);
        else
            Utils::InterConnector::instance().showSidebar(currentAimId_, profile_page);
    }

    void MenuPage::adminsClicked()
    {
        changeTab(admins);
    }

    void MenuPage::chatRoleChanged(const QString& contact)
    {
        if (contact == currentAimId_)
            initFor(currentAimId_);
    }

    void MenuPage::blockedClicked()
    {
        changeTab(block);
    }

    void MenuPage::pendingClicked()
    {
        changeTab(pending);
    }

    void MenuPage::avatarClicked()
    {
        Utils::InterConnector::instance().showSidebar(currentAimId_, profile_page);
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::profile_sidebar);
    }

    void MenuPage::chatEvent(const QString& aimId)
    {
        if (aimId == currentAimId_)
        {
            if (currentTab_ == pending)
                chatMembersModel_->loadPending();
            else
                chatMembersModel_->loadAllMembers();
        }
    }

    void MenuPage::menu(QAction* action)
    {
        const auto params = action->data().toMap();
        const auto command = params[qsl("command")].toString();
        const auto aimId = params[qsl("aimid")].toString();

        if (command == ql1s("remove"))
        {
            deleteMemberDialog(chatMembersModel_, aimId, Logic::MEMBERS_LIST, this);
        }

        if (command == ql1s("profile"))
        {
            Utils::InterConnector::instance().showSidebar(aimId, profile_page);
        }

        if (command == ql1s("spam"))
        {
            if (Logic::getContactListModel()->blockAndSpamContact(aimId))
            {
                Logic::getContactListModel()->removeContactFromCL(aimId);
                GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::spam_members_list);
            }
        }

        if (command == ql1s("block"))
        {
            blockUser(aimId, true);
        }

        if (command == ql1s("readonly"))
        {
            readOnly(aimId, true);
        }

        if (command == ql1s("revoke_readonly"))
        {
            readOnly(aimId, false);
        }

        if (command == ql1s("make_admin"))
        {
            changeRole(aimId, true);
        }

        if (command == ql1s("revoke_admin"))
        {
            changeRole(aimId, false);
        }
    }

    void MenuPage::backButtonClicked()
    {
        chatMembersModel_->loadAllMembers();
        stackedWidget_->setCurrentIndex(main);
        currentTab_ = all;
    }

    void MenuPage::allMemebersClicked()
    {
        changeTab(all);
    }

    void MenuPage::favoritesClicked()
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("contact", currentAimId_);
        Ui::GetDispatcher()->post_message_to_core(Logic::getRecentsModel()->isFavorite(currentAimId_) ? qsl("unfavorite") : qsl("favorite"), collection.get());
        emit Utils::InterConnector::instance().closeAnySemitransparentWindow();
    }

    void MenuPage::copyLinkClicked()
    {
        if (currentAimId_.isEmpty())
            return;

        QString link;
        const auto icqLink = ql1s("https://icq.com/");
        if (Logic::getContactListModel()->isChat(currentAimId_))
        {
            if (!info_)
                return;

            link = icqLink % ql1s("chat/") % info_->Stamp_;
        }
        else
        {
            link = icqLink % currentAimId_;
        }

        Ui::SelectContactsWidget shareDialog(
            nullptr,
            Logic::MembersWidgetRegim::SHARE_LINK,
            QT_TRANSLATE_NOOP("popup_window", "Share link"),
            QT_TRANSLATE_NOOP("popup_window", "COPY LINK"),
            Ui::MainPage::instance(),
            true);

        const auto action = shareDialog.show();
        if (action == QDialog::Accepted)
        {
            const auto contact = shareDialog.getSelectedContact();
            if (!contact.isEmpty())
            {
                emit Utils::InterConnector::instance().addPageToDialogHistory(Logic::getContactListModel()->selectedContact());
                Logic::getContactListModel()->setCurrent(contact, -1, true);
                Ui::GetDispatcher()->sendMessageToContact(contact, link);
            }
            else
            {
                QApplication::clipboard()->setText(link);
            }

            emit Utils::InterConnector::instance().closeAnySemitransparentWindow();
        }
    }

    void MenuPage::themesClicked()
    {
        emit Utils::InterConnector::instance().themesSettingsShow(true, currentAimId_);
        emit Utils::InterConnector::instance().closeAnySemitransparentWindow();
    }

    void MenuPage::privacyClicked()
    {
        changeTab(privacy);
    }

    void MenuPage::eraseHistoryClicked()
    {
        const auto confirmed = Utils::GetConfirmationWithTwoButtons(
            QT_TRANSLATE_NOOP("popup_window", "CANCEL"),
            QT_TRANSLATE_NOOP("popup_window", "YES"),
            QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to erase chat history?"),
            Logic::getContactListModel()->getDisplayName(currentAimId_),
            nullptr);

        if (confirmed)
            Logic::GetMessagesModel()->eraseHistory(currentAimId_);
    }

    void MenuPage::ignoreClicked()
    {
        if (Logic::getContactListModel()->ignoreContactWithConfirm(currentAimId_))
        {
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::ignore_sidebar);
            Utils::InterConnector::instance().setSidebarVisible(false);
        }
    }

    void MenuPage::quitClicked()
    {
        auto confirmed = Utils::GetConfirmationWithTwoButtons(
            QT_TRANSLATE_NOOP("popup_window", "CANCEL"),
            QT_TRANSLATE_NOOP("popup_window", "YES"),
            QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to leave chat?"),
            Logic::getContactListModel()->getDisplayName(currentAimId_),
            nullptr);
        if (confirmed)
        {
            Logic::getContactListModel()->removeContactFromCL(currentAimId_);
            GetDispatcher()->getVoipController().setDecline(currentAimId_.toUtf8().data(), false);
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::delete_sidebar);
        }
    }

    void MenuPage::removeClicked()
    {
        auto confirmed = Utils::GetConfirmationWithTwoButtons(
            QT_TRANSLATE_NOOP("popup_window", "CANCEL"),
            QT_TRANSLATE_NOOP("popup_window", "YES"),
            QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to delete contact?"),
            Logic::getContactListModel()->getDisplayName(currentAimId_),
            nullptr);
        if (confirmed)
        {
            Logic::getContactListModel()->removeContactFromCL(currentAimId_);
            GetDispatcher()->getVoipController().setDecline(currentAimId_.toUtf8().data(), false);
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::delete_sidebar);
        }
    }

    void MenuPage::touchScrollStateChanged(QScroller::State st)
    {
        const auto isOn = st != QScroller::Inactive;
        moreLabel_->setAttribute(Qt::WA_TransparentForMouseEvents, isOn);
        addContact_->setAttribute(Qt::WA_TransparentForMouseEvents, isOn);
        addToChat_->setAttribute(Qt::WA_TransparentForMouseEvents, isOn);
        favoriteButton_->setAttribute(Qt::WA_TransparentForMouseEvents, isOn);
        copyLink_->setAttribute(Qt::WA_TransparentForMouseEvents, isOn);
        themesButton_->setAttribute(Qt::WA_TransparentForMouseEvents, isOn);
        privacyButton_->setAttribute(Qt::WA_TransparentForMouseEvents, isOn);
        eraseHistoryButton_->setAttribute(Qt::WA_TransparentForMouseEvents, isOn);
        ignoreButton_->setAttribute(Qt::WA_TransparentForMouseEvents, isOn);
        quitAndDeleteButton_->setAttribute(Qt::WA_TransparentForMouseEvents, isOn);
        spamButtonAuth_->setAttribute(Qt::WA_TransparentForMouseEvents, isOn);
        deleteButton_->setAttribute(Qt::WA_TransparentForMouseEvents, isOn);
        spamButton_->setAttribute(Qt::WA_TransparentForMouseEvents, isOn);
        admins_->setAttribute(Qt::WA_TransparentForMouseEvents, isOn);
        allMembers_->setAttribute(Qt::WA_TransparentForMouseEvents, isOn);
        blockList_->setAttribute(Qt::WA_TransparentForMouseEvents, isOn);
        pendingList_->setAttribute(Qt::WA_TransparentForMouseEvents, isOn);
    }

    void MenuPage::addToChatClicked()
    {
        if (!Logic::getContactListModel()->isChat(currentAimId_))
        {
            createGroupChat({ currentAimId_ });
            return;
        }

        if (!chatMembersModel_->isFullListLoaded_)
        {
            chatMembersModel_->clear();
            chatMembersModel_->loadAllMembers();
        }

        SelectContactsWidget select_members_dialog(chatMembersModel_, Logic::MembersWidgetRegim::SELECT_MEMBERS,
            QT_TRANSLATE_NOOP("sidebar", "Add to chat"), QT_TRANSLATE_NOOP("popup_window", "DONE"), this);
        connect(this, &MenuPage::updateMembers, &select_members_dialog, &SelectContactsWidget::UpdateMembers, Qt::QueuedConnection);

        if (select_members_dialog.show() == QDialog::Accepted)
        {
            postAddChatMembersFromCLModelToCore(currentAimId_);
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::groupchat_add_member_sidebar);
        }
        else
        {
            Logic::getContactListModel()->clearChecked();
        }
    }

    void MenuPage::chatInfo(qint64 seq, const std::shared_ptr<Data::ChatInfo>& info)
    {
        if (!info.get())
            return;

        if (info->AimId_ == currentAimId_)
        {
            info_ = info;
            allMembersCount_->setText(QString::number(info_->MembersCount_));
            allMembers_->setVisible(info_->MembersCount_ > 0);

            initDescription(info_->About_);

            int blockedCount = info_->BlockedCount_;
            copyLink_->setVisible(info_->Live_);
            blockCount_->setText(QString::number(blockedCount));
            blockList_->setVisible(blockedCount != 0);
            int pendingCount = info_->PendingCount_;
            const auto pendingCountStr = QString::number(pendingCount);
            pendingCount_->setText(pendingCountStr);
            approveAll_->setText(QT_TRANSLATE_NOOP("sidebar", "Approve All (") % pendingCountStr % ql1c(')'));
            pendingList_->setVisible(info->ApprovedJoin_ && pendingCount != 0 && (info->YourRole_ == ql1s("admin") || info_->YourRole_ == ql1s("moder") || info_->Creator_ == MyInfo()->aimId()));
            if (currentTab_ == all)
            {
                chatMembersModel_->updateInfo(seq, info_, info_->MembersCount_ == info_->Members_.size());

                if (Logic::getContactListModel()->isChat(currentAimId_))
                    delegate_->setRegim((chatMembersModel_->isAdmin() || chatMembersModel_->isModer()) ? Logic::ADMIN_MEMBERS : (info_->Controlled_ == false ? Logic::MEMBERS_LIST : Logic::CONTACT_LIST));
                else
                    delegate_->setRegim(Logic::CONTACT_LIST);
            }

            if (info_->YourRole_ == ql1s("admin") || info_->Creator_ == MyInfo()->aimId())
            {
                privacyButton_->show();

                publicCheckBox_->blockSignals(true);
                publicCheckBox_->setChecked(info_->Public_);
                publicCheckBox_->blockSignals(false);
                publicCheckBox_->setEnabled(info_->Live_);
                publicButton_->setColor(info_->Live_ ? CommonStyle::getColor(CommonStyle::Color::TEXT_PRIMARY) : CommonStyle::getColor(CommonStyle::Color::TEXT_LIGHT));

                linkToChatCheckBox_->blockSignals(true);
                linkToChatCheckBox_->setChecked(info_->Live_);
                linkToChatCheckBox_->blockSignals(false);

                readOnlyCheckBox_->blockSignals(true);
                readOnlyCheckBox_->setChecked(info_->DefaultRole_ == ql1s("readonly"));
                readOnlyCheckBox_->blockSignals(false);

                approvedCheckBox_->blockSignals(true);
                approvedCheckBox_->setChecked(info_->ApprovedJoin_);
                approvedCheckBox_->blockSignals(false);

                ageCheckBox_->blockSignals(true);
                ageCheckBox_->setChecked(info->AgeRestriction_);
                ageCheckBox_->blockSignals(false);
                ageCheckBox_->setEnabled(info_->Live_);
                ageRestrictions_->setColor(info_->Live_ ? CommonStyle::getColor(CommonStyle::Color::TEXT_PRIMARY) : CommonStyle::getColor(CommonStyle::Color::TEXT_LIGHT));
            }

            if (currentTab_ == admins)
                chatMembersModel_->adminsOnly();

            emit updateMembers();
            updateWidth();
        }
    }

    void MenuPage::chatBlocked(const QVector<Data::ChatMemberInfo>&)
    {
    }

    void MenuPage::chatPending(const QVector<Data::ChatMemberInfo>& info)
    {
        approveAll_->setText(QT_TRANSLATE_NOOP("sidebar", "Approve All (") % QString::number(info.size()) % ql1c(')'));
    }

    void MenuPage::notificationsChecked(int state)
    {
        Logic::getRecentsModel()->muteChat(currentAimId_, state == Qt::Unchecked);
        GetDispatcher()->post_stats_to_core(state == Qt::Unchecked ? core::stats::stats_event_names::mute_sidebar : core::stats::stats_event_names::unmute);
    }


    void MenuPage::publicChanged(int state)
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("aimid", currentAimId_);
        collection.set_value_as_bool("public", state == Qt::Checked);
        Ui::GetDispatcher()->post_message_to_core(qsl("chats/mod/public"), collection.get());
    }

    void MenuPage::approvedChanged(int state)
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("aimid", currentAimId_);
        collection.set_value_as_bool("approved", state == Qt::Checked);
        Ui::GetDispatcher()->post_message_to_core(qsl("chats/mod/join"), collection.get());
    }

    void MenuPage::linkToChatClicked(int state)
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("aimid", currentAimId_);
        collection.set_value_as_bool("link", state == Qt::Checked);
        Ui::GetDispatcher()->post_message_to_core(qsl("chats/mod/link"), collection.get());
    }

    void MenuPage::ageClicked(int state)
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("aimid", currentAimId_);
        collection.set_value_as_bool("age", state == Qt::Checked);
        Ui::GetDispatcher()->post_message_to_core(qsl("chats/mod/age"), collection.get());
    }

    void MenuPage::readOnlyClicked(int state)
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("aimid", currentAimId_);
        collection.set_value_as_bool("ro", state == Qt::Checked);
        Ui::GetDispatcher()->post_message_to_core(qsl("chats/mod/ro"), collection.get());
    }

    void MenuPage::addContactClicked()
    {
        Logic::getContactListModel()->addContactToCL(currentAimId_);
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::add_user_sidebar);

        emit Utils::InterConnector::instance().closeAnySemitransparentWindow();
    }

    void MenuPage::spamClicked()
    {
        if (Logic::getContactListModel()->blockAndSpamContact(currentAimId_))
        {
            Logic::getContactListModel()->removeContactFromCL(currentAimId_);
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::spam_sidebar);
        }
    }
}

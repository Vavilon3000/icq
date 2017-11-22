#include "stdafx.h"
#include "HistoryControlPage.h"

#include "HistoryControlPageThemePanel.h"
#include "MessageItem.h"
#include "MessagesModel.h"
#include "MessageStyle.h"
#include "MessagesScrollArea.h"
#include "ServiceMessageItem.h"
#include "ChatEventItem.h"
#include "VoipEventItem.h"
#include "MentionCompleter.h"
#include "auth_widget/AuthWidget.h"
#include "complex_message/ComplexMessageItem.h"
#include "selection_panel/SelectionPanel.h"
#include "../ContactDialog.h"
#include "../GroupChatOperations.h"
#include "../MainPage.h"
#include "../MainWindow.h"
#include "../contact_list/ChatMembersModel.h"
#include "../contact_list/ContactList.h"
#include "../contact_list/ContactListModel.h"
#include "../contact_list/RecentsModel.h"
#include "../contact_list/UnknownsModel.h"
#include "../contact_list/SelectionContactsForGroupChat.h"
#include "../contact_list/contact_profile.h"
#include "../sidebar/Sidebar.h"
#include "../input_widget/InputWidget.h"
#include "../../core_dispatcher.h"
#include "../../theme_settings.h"
#include "../../cache/themes/themes.h"
#include "../../controls/CommonStyle.h"
#include "../../controls/ContextMenu.h"
#include "../../controls/LabelEx.h"
#include "../../controls/TextEmojiWidget.h"
#include "../../utils/gui_coll_helper.h"
#include "../../utils/InterConnector.h"
#include "../../utils/Text2DocConverter.h"
#include "../../utils/utils.h"
#include "../../utils/log/log.h"
#include "../../utils/SChar.h"
#include "../../my_info.h"
#include "HistoryButtonDown.h"

#include <boost/range/adaptor/reversed.hpp>

#ifdef __APPLE__
#include "../../utils/macos/mac_support.h"
#endif
namespace
{

    const QString button_down_imageDefault = qsl(":/themes/standard/100/history_down/history_down.png");
    const QString button_down_imageOver = qsl(":/themes/standard/100/history_down/history_down_hover.png");
    const QString button_mentions_imageDefault = qsl(":/themes/standard/100/history_down/mentions_button");
    const QString button_mentions_imageOver = qsl(":/themes/standard/100/history_down/mentions_button_active");

    const auto button_down_size = 48;
    const auto button_mentions_size = 48;
    const auto button_down_offset_y = 0;
    const auto button_down_offset_x = 12;
    const auto button_shift = 20;

    const auto prevChatButtonWidth = 32;
    const auto chatNamePaddingLeft = 20;

    const auto mentionCompleterTimeout = 200;

    bool isRemovableWidget(QWidget *w);
    bool isUpdateableWidget(QWidget *w);
    int name_fixed_height = 48;
    int scroll_by_key_delta = 20;

    QMap<QString, QVariant> makeData(const QString& _command, const QString& _aimId)
    {
        QMap<QString, QVariant> result;
        result[qsl("command")] = _command;
        result[qsl("aimid")] = _aimId;
        return result;
    }
}

namespace Ui
{

    enum class HistoryControlPage::WidgetRemovalResult
    {
        Min,

        Removed,
        NotFound,
        PersistentWidget,

        Max
    };

    ClickWidget::ClickWidget(QWidget* _parent)
        : QWidget(_parent)
    {
    }

    void ClickWidget::mouseReleaseEvent(QMouseEvent *_e)
    {
        emit clicked();
        QWidget::mouseReleaseEvent(_e);
    }

    TopWidget::TopWidget(QWidget* parent)
        : QStackedWidget(parent)
        , lastIndex_(0)
    {
    }

    void TopWidget::showThemeWidget(bool _toCurrent, ThemePanelCallback _callback)
    {
        auto theme = qobject_cast<HistoryControlPageThemePanel*>(widget(Theme));
        if (!theme)
            return;

        theme->setCallback(_callback);
        theme->setSelectionToAll(!_toCurrent);
        theme->setShowSetThemeButton(_toCurrent);

        setCurrentIndex(Theme);
    }

    void TopWidget::showSelectionWidget()
    {
        if (currentIndex() == Selection)
            return;

        lastIndex_ = currentIndex();
        setCurrentIndex(Selection);
    }

    void TopWidget::hideSelectionWidget()
    {
        if (currentIndex() == Theme)
            if (auto theme = qobject_cast<HistoryControlPageThemePanel*>(widget(Theme)))
                theme->cancelThemePressed();
        setCurrentIndex(lastIndex_);
    }

    MessagesWidgetEventFilter::MessagesWidgetEventFilter(
        QWidget* _buttonsWidget,
        const QString& _contactName,
        TextEmojiWidget* _contactNameWidget,
        MessagesScrollArea *_scrollArea,
        QWidget* _firstOverlay,
        QWidget* _secondOverlay,
        HistoryControlPage* _dialog,
        const QString& _aimId
    )
        : QObject(_dialog)
        , ButtonsWidget_(_buttonsWidget)
        , ScrollArea_(_scrollArea)
        , NewPlateShowed_(false)
        , ScrollDirectionDown_(false)
        , Width_(0)
        , Dialog_(_dialog)
        , ContactName_(_contactName)
        , ContactNameWidget_(_contactNameWidget)
        , FirstOverlay_(_firstOverlay)
        , SecondOverlay_(_secondOverlay)
        , Ref_(std::make_shared<bool>(false))
    {
        assert(ContactNameWidget_);
        assert(ScrollArea_);

        if (ContactName_.isEmpty())
        {
            std::weak_ptr<bool> wr_ref = Ref_;
            Logic::getContactListModel()->getContactProfile(_aimId, [this, wr_ref, _aimId](Logic::profile_ptr profile, int32_t)
            {
                auto ref = wr_ref.lock();
                if (!ref)
                    return;

                QString name;
                if (profile)
                {
                    name = profile->get_friendly();
                    if (name.isEmpty())
                    {
                        name = profile->get_first_name();
                        auto lastName = profile->get_last_name();
                        if (!name.isEmpty() && !lastName.isEmpty())
                            name += ql1c(' ') % lastName;
                    }
                }
                ResetContactName(name.isEmpty() ? _aimId : name);
            });
        }

        ContactNameWidget_->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);

        FirstOverlay_->stackUnder(SecondOverlay_);
        ScrollArea_->stackUnder(FirstOverlay_);
    }

    void MessagesWidgetEventFilter::resetNewPlate()
    {
        NewPlateShowed_ = false;
    }

    QString MessagesWidgetEventFilter::getContactName() const
    {
        return ContactName_;
    }

    bool MessagesWidgetEventFilter::eventFilter(QObject* _obj, QEvent* _event)
    {
        if (_event->type() == QEvent::Resize)
        {
            const auto rect = qobject_cast<QWidget*>(_obj)->contentsRect();

            if (rect.isValid())
            {
                updateSizes();
                ScrollArea_->setGeometry(rect);

                Dialog_->updateFooterButtonsPositions();

                Dialog_->positionMentionCompleter(rect);
            }

            qobject_cast<Ui::ServiceMessageItem*>(SecondOverlay_)->setNew();
            if (Width_ != rect.width())
            {
                Logic::GetMessagesModel()->setItemWidth(rect.width());
            }

            Width_ = rect.width();

            ResetContactName(ContactName_);
        }
        else if (_event->type() == QEvent::Paint)
        {
            QDate date;
            auto newFound = false;
            auto dateVisible = true;
            qint64 firstVisibleId = -1;

            ScrollArea_->enumerateWidgets(
                [this, &firstVisibleId, &newFound, &date, &dateVisible]
            (QWidget *widget, const bool isVisible)
            {
                if (widget->visibleRegion().isEmpty() || !isVisible)
                {
                    return true;
                }

                if (auto msgItem = qobject_cast<Ui::MessageItem*>(widget))
                {
                    if (firstVisibleId == -1)
                    {
                        date = msgItem->date();
                        firstVisibleId = msgItem->getId();
                    }

                    return true;
                }

                if (auto complexMsgItem = qobject_cast<Ui::ComplexMessage::ComplexMessageItem*>(widget))
                {
                    if (firstVisibleId == -1)
                    {
                        date = complexMsgItem->getDate();
                        firstVisibleId = complexMsgItem->getId();
                    }

                    return true;
                }

                if (auto serviceItem = qobject_cast<Ui::ServiceMessageItem*>(widget))
                {
                    if (serviceItem->isNew())
                    {
                        newFound = true;
                        NewPlateShowed_ = true;
                    }
                    else
                    {
                        dateVisible = false;
                    }
                }

                return true;
            },
                false
                );

            if (!newFound && NewPlateShowed_)
            {
                Dialog_->newPlateShowed();
                NewPlateShowed_ = false;
            }

            bool visible = date.isValid();

            if (date != Date_)
            {
                Date_ = date;
                qobject_cast<Ui::ServiceMessageItem*>(FirstOverlay_)->setDate(Date_);
                FirstOverlay_->adjustSize();
            }

            const auto isFirstOverlayVisible = (dateVisible && visible);
            FirstOverlay_->setAttribute(Qt::WA_WState_Hidden, !isFirstOverlayVisible);
            FirstOverlay_->setAttribute(Qt::WA_WState_Visible, isFirstOverlayVisible);

            qint64 newPlateId = Dialog_->getNewPlateId();
            bool newPlateOverlay = newPlateId != -1 && newPlateId < firstVisibleId && !newFound && !NewPlateShowed_;

            if (newPlateOverlay && visible)
            {
                if (!SecondOverlay_->testAttribute(Qt::WA_WState_Visible))
                    SecondOverlay_->show();
            }
            else
            {
                if (!SecondOverlay_->testAttribute(Qt::WA_WState_Hidden))
                    SecondOverlay_->hide();
            }
        }
        else if (_event->type() == QEvent::KeyPress)
        {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(_event);
            bool applePageUp = (platform::is_apple() && keyEvent->modifiers().testFlag(Qt::KeyboardModifier::ControlModifier) && keyEvent->key() == Qt::Key_Up);
            bool applePageDown = (platform::is_apple() && keyEvent->modifiers().testFlag(Qt::KeyboardModifier::ControlModifier) && keyEvent->key() == Qt::Key_Down);
            bool applePageEnd = (platform::is_apple() && ((keyEvent->modifiers().testFlag(Qt::KeyboardModifier::MetaModifier) && keyEvent->key() == Qt::Key_Right) || keyEvent->key() == Qt::Key_End));
            if (keyEvent->matches(QKeySequence::Copy))
            {
                const auto result = ScrollArea_->getSelectedText();

                if (!result.isEmpty())
                {
                    QApplication::clipboard()->setText(result);
                }
            }
            else if (keyEvent->matches(QKeySequence::Paste) || keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter)
            {
                auto contactDialog = Utils::InterConnector::instance().getContactDialog();
                if (contactDialog)
                {
                    if (auto inputWidget = contactDialog->getInputWidget())
                    {
                        QApplication::sendEvent(inputWidget, _event);
                    }
                }
            }
            if (keyEvent->key() == Qt::Key_Up && !applePageUp)
            {
                ScrollArea_->scroll(UP, Utils::scale_value(scroll_by_key_delta));
            }
            else if (keyEvent->key() == Qt::Key_Down && !applePageDown)
            {
                ScrollArea_->scroll(DOWN, Utils::scale_value(scroll_by_key_delta));
            }
            else if ((keyEvent->modifiers() == Qt::CTRL && keyEvent->key() == Qt::Key_End) || applePageEnd)
            {
                Dialog_->scrollToBottom();
            }
            else if (keyEvent->key() == Qt::Key_PageUp || applePageUp)
            {
                ScrollArea_->scroll(UP, Dialog_->height());
            }
            else if (keyEvent->key() == Qt::Key_PageDown || applePageDown)
            {
                ScrollArea_->scroll(DOWN, Dialog_->height());
            }
        }

        return QObject::eventFilter(_obj, _event);
    }

    void MessagesWidgetEventFilter::ResetContactName(const QString& _contactName)
    {
        if (!ContactNameWidget_)
            return;

        ContactName_ = _contactName;

        auto contactNameMaxWidth = Utils::InterConnector::instance().getContactDialog()->width();
        contactNameMaxWidth -= ButtonsWidget_->width();

        int diff = ContactNameWidget_->rect().width() - ContactNameWidget_->contentsRect().width();
        contactNameMaxWidth -= diff;
        contactNameMaxWidth -= Utils::scale_value(20);

        QFontMetrics m(ContactNameWidget_->font());
        QString elidedString = m.elidedText(ContactName_, Qt::ElideRight, contactNameMaxWidth).simplified();

        ContactNameWidget_->setText(elidedString);
        ContactNameWidget_->setFixedWidth(ContactNameWidget_->getCompiledWidth());
        ContactNameWidget_->setFixedHeight(Utils::scale_value(name_fixed_height));
    }

    void MessagesWidgetEventFilter::updateSizes()
    {
        FirstOverlay_->setFixedWidth(Dialog_->rect().width());
        FirstOverlay_->move(Dialog_->rect().topLeft());
        SecondOverlay_->setGeometry(Dialog_->rect().x(), Dialog_->rect().y() + FirstOverlay_->height() * 0.7, Dialog_->rect().width(), Dialog_->rect().height());
    }

    enum class HistoryControlPage::State
    {
        Min,

        Idle,
        Fetching,
        Inserting,

        Max
    };

    QTextStream& operator<<(QTextStream& _oss, const HistoryControlPage::State _arg)
    {
        switch (_arg)
        {
        case HistoryControlPage::State::Idle: _oss << ql1s("IDLE"); break;
        case HistoryControlPage::State::Fetching: _oss << ql1s("FETCHING"); break;
        case HistoryControlPage::State::Inserting: _oss << ql1s("INSERTING"); break;

        default:
            assert(!"unexpected state value");
            break;
        }

        return _oss;
    }

    HistoryControlPage::HistoryControlPage(QWidget* _parent, const QString& _aimId)
        : QWidget(_parent)
        , setThemeId_(-1)
        , typingWidget_(new QWidget(this))
        , authWidget_(nullptr)
        , isContactStatusClickable_(false)
        , isMessagesRequestPostponed_(false)
        , isMessagesRequestPostponedDown_(false)
        , isPublicChat_(false)
        , chatMembersModel_(nullptr)
        , contactStatus_(new LabelEx(this))
        , messagesArea_(new MessagesScrollArea(this, typingWidget_))
        , newPlatePosition_(-1)
        , chatInfoSequence_(-1)
        , aimId_(_aimId)
        , seenMsgId_(-1)
        , messagesOverlayFirst_(new ServiceMessageItem(this, true))
        , messagesOverlaySecond_(new ServiceMessageItem(this, true))
        , state_(State::Idle)
        , topWidget_(new TopWidget(this))
        , mentionCompleter_(new MentionCompleter(this))
        , buttonDown_(nullptr)
        , buttonMentions_(nullptr)
        , buttonDir_(0)
        , buttonDownCurve_(QEasingCurve::InSine)
        , bNewMessageForceShow_(false)
        , buttonDownCurrentTime_(0)
        , quoteId_(-1)
        , prevTypingTime_(0)
        , typedTimer_(new QTimer(this))
        , lookingTimer_(new QTimer(this))
        , mentionTimer_(new QTimer(this))
    {
        initButtonDown();
        initMentionsButton();

        QObject::connect(messagesArea_, &MessagesScrollArea::updateHistoryPosition, this, &HistoryControlPage::onUpdateHistoryPosition);

        messagesOverlayFirst_->setContact(_aimId);
        messagesOverlaySecond_->setContact(_aimId);

        const auto style = Utils::LoadStyle(qsl(":/qss/history_control"));
        setStyleSheet(style);
        auto mainTopWidget = new QWidget(this);
        mainTopWidget->setStyleSheet(style);
        mainTopWidget->setFixedHeight(CommonStyle::getTopPanelHeight());
        mainTopWidget->setObjectName(qsl("topWidget"));

        auto topLayout = Utils::emptyHLayout(mainTopWidget);
        topLayout->setContentsMargins(0, 0, Utils::scale_value(16), 0);

        topWidgetLeftPadding_ = new QWidget(this);
        topWidgetLeftPadding_->setFixedWidth(Utils::scale_value(chatNamePaddingLeft));
        topWidgetLeftPadding_->setAttribute(Qt::WA_TransparentForMouseEvents);
        topLayout->addWidget(topWidgetLeftPadding_);

        prevChatButton_ = new CustomButton(this, qsl(":/controls/arrow_back_d_100"));
        prevChatButton_->setHoverImage(qsl(":/controls/arrow_back_d_100_hover"));
        prevChatButton_->setFocusPolicy(Qt::NoFocus);
        prevChatButton_->setCursor(Qt::PointingHandCursor);
        prevChatButton_->setFixedWidth(Utils::scale_value(prevChatButtonWidth));
        prevChatButton_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        prevChatButton_->setAlign(Qt::AlignVCenter | Qt::AlignHCenter);
        topLayout->addWidget(prevChatButton_);

        setPrevChatButtonVisible(false);

        connect(prevChatButton_, &CustomButton::clicked, &Utils::InterConnector::instance(), &Utils::InterConnector::switchToPrevDialog);
        //connect(&Utils::InterConnector::instance(), &Utils::InterConnector::noPagesInDialogHistory,   this, [this](){ setPrevChatButtonVisible(false); });
        //connect(&Utils::InterConnector::instance(), &Utils::InterConnector::pageAddedToDialogHistory, this, [this](){ setPrevChatButtonVisible(true);  });

        contactWidget_ = new QWidget(mainTopWidget);
        {
            QSizePolicy sizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
            sizePolicy.setHorizontalStretch(1);
            contactWidget_->setSizePolicy(sizePolicy);
        }
        auto nameStatusVerLayout = Utils::emptyVLayout(contactWidget_);
        nameStatusVerLayout->setSizeConstraint(QLayout::SetDefaultConstraint);

        auto nameLayout = new QHBoxLayout(contactWidget_);
        nameWidget_ = new ClickWidget(contactWidget_);
        auto v = Utils::emptyVLayout(nameWidget_);

        contactName_ = new TextEmojiWidget(
            contactWidget_,
            Fonts::appFontScaled(16, Fonts::FontWeight::Medium),
            CommonStyle::getColor(CommonStyle::Color::TEXT_PRIMARY),
            Utils::scale_value(24));

        contactName_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        contactName_->setAttribute(Qt::WA_TransparentForMouseEvents);
        v->addWidget(contactName_);
        nameLayout->addWidget(nameWidget_);
        nameWidget_->setCursor(Qt::PointingHandCursor);

        connect(nameWidget_, &ClickWidget::clicked, this, &HistoryControlPage::nameClicked, Qt::UniqueConnection);

        nameLayout->addSpacerItem(new QSpacerItem(QWIDGETSIZE_MAX, 0, QSizePolicy::Expanding, QSizePolicy::Preferred));
        nameStatusVerLayout->addLayout(nameLayout);

        auto statusHorLayout = Utils::emptyHLayout();
        auto contactStatusLayout = new QHBoxLayout();
        contactStatusLayout->setContentsMargins(0, 0, 0, Utils::scale_value(8));
        contactStatusWidget_ = new QWidget(this);
        statusHorLayout->addWidget(contactStatusWidget_);

        setContactStatusClickable(false);

        nameStatusVerLayout->addLayout(statusHorLayout);
        topLayout->addWidget(contactWidget_);

        auto buttonsWidget = new QWidget(mainTopWidget);
        auto buttonsLayout = Utils::emptyHLayout(buttonsWidget);
        buttonsLayout->setSpacing(Utils::scale_value(16));

        searchButton_ = new QPushButton(buttonsWidget);
        searchButton_->setObjectName(qsl("searchButton"));
        searchButton_->setToolTip(QT_TRANSLATE_NOOP("tooltips", "Call"));
        buttonsLayout->addWidget(searchButton_, 0, Qt::AlignRight);

        callButton_ = new QPushButton(buttonsWidget);
        callButton_->setObjectName(qsl("callButton"));
        callButton_->setToolTip(QT_TRANSLATE_NOOP("tooltips", "Call"));
        buttonsLayout->addWidget(callButton_, 0, Qt::AlignRight);

        videoCallButton_ = new QPushButton(buttonsWidget);
        videoCallButton_->setObjectName(qsl("videoCallButton"));
        videoCallButton_->setToolTip(QT_TRANSLATE_NOOP("tooltips", "Video call"));
        buttonsLayout->addWidget(videoCallButton_, 0, Qt::AlignRight);

        addMemberButton_ = new QPushButton(buttonsWidget);
        addMemberButton_->setObjectName(qsl("addMemberButton"));
        addMemberButton_->setToolTip(QT_TRANSLATE_NOOP("tooltips", "Add member"));
        Testing::setAccessibleName(addMemberButton_, qsl("AddContactToChat"));
        buttonsLayout->addWidget(addMemberButton_, 0, Qt::AlignRight);

        moreButton_ = new QPushButton(buttonsWidget);
        moreButton_->setObjectName(qsl("optionButton"));
        moreButton_->setToolTip(QT_TRANSLATE_NOOP("tooltips", "Chat options"));
        Testing::setAccessibleName(moreButton_, qsl("ShowChatMenu"));
        buttonsLayout->addWidget(moreButton_, 0, Qt::AlignRight);
        topLayout->addWidget(buttonsWidget, 0, Qt::AlignRight);

        auto selection = new Selection::SelectionPanel(messagesArea_, this);

        connect(messagesArea_, &MessagesScrollArea::messagesSelected,   topWidget_, &TopWidget::showSelectionWidget);
        connect(messagesArea_, &MessagesScrollArea::messagesDeselected, topWidget_, &TopWidget::hideSelectionWidget);

        topWidget_->insertWidget(TopWidget::Main, mainTopWidget);
        topWidget_->insertWidget(TopWidget::Theme, new HistoryControlPageThemePanel(this));
        topWidget_->insertWidget(TopWidget::Selection, selection);
        topWidget_->setCurrentIndex(TopWidget::Main);

        messagesOverlayFirst_->setAttribute(Qt::WA_TransparentForMouseEvents);
        eventFilter_ = new MessagesWidgetEventFilter(
            buttonsWidget,
            Logic::getContactListModel()->selectedContactName(),
            contactName_,
            messagesArea_,
            messagesOverlayFirst_,
            messagesOverlaySecond_,
            this, aimId_);
        installEventFilter(eventFilter_);

        typingWidget_->setObjectName(qsl("typingWidget"));
        {
            auto twl = new QHBoxLayout(typingWidget_);
            twl->setContentsMargins(Utils::scale_value(24), 0, 0, 0);
            twl->setSpacing(Utils::scale_value(7));
            twl->setAlignment(Qt::AlignLeft);
            {
                typingWidgets_.twt = new TextEmojiWidget(typingWidget_, Fonts::appFontScaled(12), MessageStyle::getTypingColor(), Utils::scale_value(20));
                typingWidgets_.twt->setSizePolicy(QSizePolicy::Policy::Preferred, typingWidgets_.twt->sizePolicy().verticalPolicy());
                typingWidgets_.twt->setText(qsl(" "));
                typingWidgets_.twt->setVisible(false);

                twl->addWidget(typingWidgets_.twt);

                typingWidgets_.twa = new QLabel(typingWidget_);
                typingWidgets_.twa->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Preferred);
                typingWidgets_.twa->setContentsMargins(0, Utils::scale_value(11), 0, 0);

                typingWidgets_.twm = new QMovie(Utils::parse_image_name(qsl(":/resources/gifs/typing_animation_100.gif")), QByteArray(), typingWidgets_.twa);
                typingWidgets_.twa->setMovie(typingWidgets_.twm);
                typingWidgets_.twa->setVisible(false);
                twl->addWidget(typingWidgets_.twa);

                connect(GetDispatcher(), &core_dispatcher::typingStatus, this, &HistoryControlPage::typingStatus);
            }
        }
        typingWidget_->show();

        connect(addMemberButton_, &QPushButton::clicked, this, &HistoryControlPage::addMember, Qt::QueuedConnection);
        connect(searchButton_, &QPushButton::clicked, this, &HistoryControlPage::searchButtonClicked, Qt::QueuedConnection);
        connect(videoCallButton_, &QPushButton::clicked, this, &HistoryControlPage::callVideoButtonClicked, Qt::QueuedConnection);
        connect(callButton_, &QPushButton::clicked, this, &HistoryControlPage::callAudioButtonClicked, Qt::QueuedConnection);
        connect(moreButton_, &QPushButton::clicked, this, &HistoryControlPage::moreButtonClicked, Qt::QueuedConnection);

        moreButton_->setFocusPolicy(Qt::NoFocus);
        moreButton_->setCursor(Qt::PointingHandCursor);
        videoCallButton_->setFocusPolicy(Qt::NoFocus);
        videoCallButton_->setCursor(Qt::PointingHandCursor);
        callButton_->setFocusPolicy(Qt::NoFocus);
        callButton_->setCursor(Qt::PointingHandCursor);
        searchButton_->setFocusPolicy(Qt::NoFocus);
        searchButton_->setCursor(Qt::PointingHandCursor);
        addMemberButton_->setFocusPolicy(Qt::NoFocus);
        addMemberButton_->setCursor(Qt::PointingHandCursor);

        messagesArea_->setFocusPolicy(Qt::StrongFocus);

        officialMark_ = new QPushButton(contactWidget_);
        officialMark_->setObjectName(qsl("officialMark"));
        officialMark_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        contactStatusLayout->addWidget(officialMark_);
        officialMark_->setVisible(Logic::getContactListModel()->isOfficial(_aimId) && !Logic::getContactListModel()->isChat(_aimId));

        contactStatus_->setColor(CommonStyle::getColor(CommonStyle::Color::TEXT_LIGHT));
        contactStatus_->setFont(Fonts::appFontScaled(12));
        contactStatusLayout->addWidget(contactStatus_);

        contactStatusWidget_->setLayout(contactStatusLayout);

        auto contact = Logic::getContactListModel()->getContactItem(_aimId);
#ifndef STRIP_VOIP
        if (contact && contact->is_chat())
        {
#endif //STRIP_VOIP
            callButton_->hide();
            videoCallButton_->hide();
#ifndef STRIP_VOIP
        }
        else
        {
            addMemberButton_->hide();
        }
#endif //STRIP_VOIP

#ifdef STRIP_VOIP
        // TODO: Should remove the lines below when STRIP_VOIP is removed.
        if (contact && !contact->is_chat())
        {
            addMemberButton_->hide();
        }
#endif //STRIP_VOIP

        QObject::connect(
            Logic::getContactListModel(),
            &Logic::ContactListModel::contactChanged,
            this,
            &HistoryControlPage::contactChanged,
            Qt::QueuedConnection);

        QObject::connect(Logic::GetMessagesModel(), &Logic::MessagesModel::ready, this, &HistoryControlPage::sourceReady, Qt::QueuedConnection);
        QObject::connect(Logic::GetMessagesModel(), &Logic::MessagesModel::canFetchMore, this, &HistoryControlPage::fetchMore, Qt::QueuedConnection);

        QObject::connect(
            Logic::GetMessagesModel(),
            &Logic::MessagesModel::updated,
            this,
            &HistoryControlPage::updated,
            (Qt::ConnectionType)(Qt::QueuedConnection | Qt::UniqueConnection));

        QObject::connect(
            Logic::GetMessagesModel(),
            &Logic::MessagesModel::newMessageReceived,
            this,
            &HistoryControlPage::onNewMessageReceived,
            Qt::UniqueConnection);

        QObject::connect(Logic::GetMessagesModel(), &Logic::MessagesModel::deleted, this, &HistoryControlPage::deleted, Qt::QueuedConnection);
        QObject::connect(Logic::GetMessagesModel(), &Logic::MessagesModel::messageIdFetched, this, &HistoryControlPage::messageKeyUpdated, Qt::QueuedConnection);

        QObject::connect(
            Logic::GetMessagesModel(),
            &Logic::MessagesModel::indentChanged,
            this,
            &HistoryControlPage::indentChanged
        );

        QObject::connect(
            Logic::GetMessagesModel(),
            &Logic::MessagesModel::hasAvatarChanged,
            this,
            &HistoryControlPage::hasAvatarChanged
        );

        QObject::connect(Logic::getRecentsModel(), &Logic::RecentsModel::readStateChanged, this, &HistoryControlPage::update, Qt::QueuedConnection);
        QObject::connect(Logic::getUnknownsModel(), &Logic::UnknownsModel::readStateChanged, this, &HistoryControlPage::update, Qt::QueuedConnection);

        QObject::connect(
            this, &HistoryControlPage::requestMoreMessagesSignal,
            this, &HistoryControlPage::requestMoreMessagesSlot,
            Qt::QueuedConnection
        );

        QObject::connect(
            messagesArea_, &MessagesScrollArea::fetchRequestedEvent,
            this, &HistoryControlPage::onReachedFetchingDistance
        );

        QObject::connect(messagesArea_, &MessagesScrollArea::needCleanup, this, &HistoryControlPage::unloadWidgets);

        QObject::connect(messagesArea_, &MessagesScrollArea::scrollMovedToBottom, this, &HistoryControlPage::scrollMovedToBottom);

        QObject::connect(messagesArea_, &MessagesScrollArea::itemRead, this, &HistoryControlPage::itemRead);

        QObject::connect(this, &HistoryControlPage::insertNextMessageSignal, this, &HistoryControlPage::insertNextMessageSlot, Qt::DirectConnection);

        QObject::connect(
            this,
            &HistoryControlPage::needRemove,
            this,
            &HistoryControlPage::removeWidget,
            Qt::QueuedConnection
        );

        QObject::connect(
            Logic::GetMessagesModel(),
            &Logic::MessagesModel::changeDlgState,
            this,
            &HistoryControlPage::changeDlgState,
            Qt::QueuedConnection);

        QObject::connect(Ui::GetDispatcher(), &Ui::core_dispatcher::setChatRoleResult, this, &HistoryControlPage::actionResult, Qt::QueuedConnection);
        QObject::connect(Ui::GetDispatcher(), &Ui::core_dispatcher::blockMemberResult, this, &HistoryControlPage::actionResult, Qt::QueuedConnection);

        Utils::InterConnector::instance().insertTopWidget(_aimId, topWidget_);

        typedTimer_->setInterval(3000);
        connect(typedTimer_, &QTimer::timeout, this, &HistoryControlPage::onTypingTimer);
        lookingTimer_->setInterval(30000);
        connect(lookingTimer_, &QTimer::timeout, this, &HistoryControlPage::onLookingTimer);

        auto contactDialog = Utils::InterConnector::instance().getContactDialog();
        if (contactDialog)
        {
            auto inputWidget = contactDialog->getInputWidget();
            if (inputWidget)
                connect(mentionCompleter_, &MentionCompleter::contactSelected, inputWidget, &InputWidget::insertMention, Qt::QueuedConnection);
        }
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::hideMentionCompleter, mentionCompleter_, &MentionCompleter::hide, Qt::QueuedConnection);

        mentionCompleter_->setDialogAimId(aimId_);
        mentionCompleter_->hide();

        connect(mentionTimer_, &QTimer::timeout, this, [this]
        {
            updateMentionsButton();
            mentionTimer_->stop();

        });
    }

    void HistoryControlPage::initButtonDown()
    {
        buttonDown_ = new HistoryButtonDown(this, button_down_imageDefault);
        buttonDown_->setHoverImage(button_down_imageOver);
        buttonDown_->setActiveImage(button_down_imageOver);
        buttonDown_->setDisabledImage(button_down_imageOver);
        buttonDown_->setPressedImage(button_down_imageOver);

        connect(buttonDown_, &HistoryButtonDown::clicked, messagesArea_, &MessagesScrollArea::buttonDownClicked, Qt::DirectConnection);
        connect(buttonDown_, &HistoryButtonDown::clicked, this, &HistoryControlPage::onButtonDownClicked, Qt::DirectConnection);
        connect(buttonDown_, &HistoryButtonDown::sendWheelEvent, messagesArea_, &MessagesScrollArea::onWheelEvent, Qt::DirectConnection);

        const auto size = Utils::scale_value(button_down_size);
        buttonDown_->setFixedSize(size, size);
        buttonDown_->setCursor(Qt::PointingHandCursor);

        int x = 0;
        int y = 0;
        buttonDown_->move(x, y);
        QObject::connect(buttonDown_, &HistoryButtonDown::clicked, this, &HistoryControlPage::scrollToBottomByButton);

        buttonDown_->hide();
        buttonDownTimer_ = new QTimer(this);
        buttonDownTimer_->setInterval(16);
        QObject::connect(buttonDownTimer_, &QTimer::timeout, this, &HistoryControlPage::onButtonDownMove);
    }

    void HistoryControlPage::initMentionsButton()
    {
        buttonMentions_ = new HistoryButtonMentions(this, button_mentions_imageDefault);
        buttonMentions_->setHoverImage(button_mentions_imageOver);
        buttonMentions_->setActiveImage(button_mentions_imageOver);
        buttonMentions_->setDisabledImage(button_mentions_imageOver);
        buttonMentions_->setPressedImage(button_mentions_imageOver);

        connect(buttonMentions_, &HistoryButtonMentions::clicked, this, &HistoryControlPage::onButtonMentionsClicked, Qt::DirectConnection);
        connect(buttonMentions_, &HistoryButtonMentions::sendWheelEvent, messagesArea_, &MessagesScrollArea::onWheelEvent, Qt::DirectConnection);

        const auto size = Utils::scale_value(button_down_size);
        buttonMentions_->setFixedSize(size, size);
        buttonMentions_->setCursor(Qt::PointingHandCursor);

        int x = 0;
        int y = 0;
        buttonMentions_->move(x, y);

        QObject::connect(GetDispatcher(), &core_dispatcher::mentionMe, this, &HistoryControlPage::mentionMe, Qt::QueuedConnection);

        buttonMentions_->hide();

        QObject::connect(Logic::GetMessagesModel(), &Logic::MessagesModel::mentionRead, this, &HistoryControlPage::onMentionRead);
    }

    void HistoryControlPage::onMentionRead(const QString& _contact, const qint64 _messageId)
    {
        if (_contact != aimId_)
            return;

        updateMentionsButton();
    }

    void HistoryControlPage::onButtonMentionsClicked(bool)
    {
        auto nextMention = Logic::GetMessagesModel()->getNextUnreadMention(aimId_);
        if (!nextMention)
        {
            buttonMentions_->setCount(0);
            buttonMentions_->hide();

            return;
        }

        const auto mentionId = nextMention->Id_;

        emit Logic::getContactListModel()->select(aimId_, mentionId, mentionId);
    }

    void HistoryControlPage::updateMentionsButton()
    {
        const int32_t mentionsCount = Logic::GetMessagesModel()->getMentionsCount(aimId_);

        buttonMentions_->setCount(mentionsCount);

        const bool buttonMentionsVisible = !!mentionsCount;

        if (buttonMentions_->isVisible() != buttonMentionsVisible)
            buttonMentions_->setVisible(buttonMentionsVisible);
    }

    void HistoryControlPage::itemRead(const qint64 _id, const bool _visible)
    {
        Logic::GetMessagesModel()->onMessageItemRead(aimId_, _id, _visible);
    }

    void HistoryControlPage::updateMentionsButtonDelayed()
    {
        if (mentionTimer_->isActive())
            mentionTimer_->stop();

        mentionTimer_->start(500);
    }

    void HistoryControlPage::mentionMe(const QString& _contact, Data::MessageBuddySptr _mention)
    {
        if (aimId_ != _contact)
            return;

        if (!isVisible())
            return;

        updateMentionsButtonDelayed();

    }

    void HistoryControlPage::updateWidgetsTheme()
    {
        messagesArea_->enumerateWidgets(
            [this](QWidget *widget, const bool)
        {
            if (auto item = qobject_cast<Ui::MessageItem*>(widget))
            {
                item->updateData();
                return true;
            }

            if (auto item = qobject_cast<Ui::ComplexMessage::ComplexMessageItem*>(widget))
            {
                item->onStyleChanged();
                return true;
            }

            if (auto item = qobject_cast<Ui::ServiceMessageItem*>(widget))
            {
                item->updateStyle();
                return true;
            }

            if (auto item = qobject_cast<Ui::VoipEventItem*>(widget))
            {
                item->updateStyle();
                return true;
            }

            return true;
        }, false);

        messagesOverlayFirst_->updateStyle();
        messagesOverlaySecond_->updateStyle();
        auto theme = get_qt_theme_settings()->themeForContact(aimId_);
        if (theme)
        {
            QColor typingColor = theme->typing_.text_color_;
            typingWidgets_.twt->setColor(typingColor);

            if (typingWidgets_.twm)
                typingWidgets_.twm->deleteLater();

            if (theme->typing_.light_gif_ == 0)
            {
                typingWidgets_.twm = new QMovie(Utils::parse_image_name(qsl(":/resources/gifs/typing_animation_100.gif")), QByteArray(), typingWidgets_.twa);
            }
            else
            {
                typingWidgets_.twm = new QMovie(Utils::parse_image_name(qsl(":/resources/gifs/typing_animation_100_white.gif")), QByteArray(), typingWidgets_.twa);
            }
            typingWidgets_.twm->setScaledSize(QSize(Utils::scale_value(16), Utils::scale_value(8)));
            typingWidgets_.twa->setMovie(typingWidgets_.twm);
            updateTypingWidgets();
        }
    }

    void HistoryControlPage::typingStatus(const Logic::TypingFires& _typing, bool _isTyping)
    {
        if (_typing.aimId_ != aimId_)
            return;

        const QString name = _typing.getChatterName();

        // if not from multichat

        if (_isTyping)
        {
            typingChattersAimIds_.insert(name);
            updateTypingWidgets();
        }
        else
        {
            typingChattersAimIds_.remove(name);
            if (typingChattersAimIds_.empty())
            {
                hideTypingWidgets();
            }
            else
            {
                updateTypingWidgets();
            }
        }
    }

    void HistoryControlPage::indentChanged(const Logic::MessageKey& _key, bool _indent)
    {
        assert(!_key.isEmpty());

        auto widget = messagesArea_->getItemByKey(_key);
        if (!widget)
        {
            return;
        }

        auto pageItem = qobject_cast<HistoryControlPageItem*>(widget);
        if (!pageItem)
        {
            return;
        }

        pageItem->setTopMargin(_indent);
    }

    void HistoryControlPage::hasAvatarChanged(const Logic::MessageKey& _key, bool _hasAvatar)
    {
        assert(!_key.isEmpty());

        auto widget = messagesArea_->getItemByKey(_key);
        if (!widget)
        {
            return;
        }

        auto pageItem = qobject_cast<HistoryControlPageItem*>(widget);
        if (!pageItem)
        {
            return;
        }

        pageItem->setHasAvatar(_hasAvatar);
    }

    void HistoryControlPage::updateTypingWidgets()
    {
        if (!typingChattersAimIds_.empty() && typingWidgets_.twa && typingWidgets_.twm && typingWidgets_.twt)
        {
            QString named;
            if (Logic::getContactListModel()->isChat(aimId_))
            {
                for (const auto& chatter :  Utils::as_const(typingChattersAimIds_))
                {
                    if (named.length())
                        named += ql1s(", ");
                    named += chatter;
                }
            }
            typingWidgets_.twa->setVisible(true);
            typingWidgets_.twm->start();
            typingWidgets_.twt->setVisible(true);
            if (named.length() && typingChattersAimIds_.size() == 1)
                typingWidgets_.twt->setText(named % ql1c(' ') % QT_TRANSLATE_NOOP("chat_page", "typing"));
            else if (named.length() && typingChattersAimIds_.size() > 1)
                typingWidgets_.twt->setText(named % ql1c(' ') % QT_TRANSLATE_NOOP("chat_page", "are typing"));
            else
                typingWidgets_.twt->setText(QT_TRANSLATE_NOOP("chat_page", "typing"));
        }
    }
    void HistoryControlPage::hideTypingWidgets()
    {
        if (typingWidgets_.twa && typingWidgets_.twm && typingWidgets_.twt)
        {
            typingWidgets_.twa->setVisible(false);
            typingWidgets_.twm->stop();
            typingWidgets_.twt->setVisible(false);
            typingWidgets_.twt->setText(QString());
        }
    }

    HistoryControlPage::~HistoryControlPage()
    {
        Utils::InterConnector::instance().removeTopWidget(aimId_);
    }

    void HistoryControlPage::appendAuthControlIfNeed()
    {
        auto contactItem = Logic::getContactListModel()->getContactItem(aimId_);
        if (contactItem && contactItem->is_chat())
            return;

        if (authWidget_)
            return;

        if (!contactItem || contactItem->is_not_auth())
        {
            authWidget_ = new AuthWidget(messagesArea_, aimId_);
            authWidget_->setProperty("permanent", true);
            messagesArea_->insertWidget(Logic::MessageKey::MIN, authWidget_);

            connect(authWidget_, &AuthWidget::addContact, this, &HistoryControlPage::authAddContact);
            connect(authWidget_, &AuthWidget::spamContact, this, &HistoryControlPage::authBlockContact);
            connect(authWidget_, &AuthWidget::deleteContact, this, &HistoryControlPage::authDeleteContact);

            connect(Logic::getContactListModel(), &Logic::ContactListModel::contact_added, this, &HistoryControlPage::contactAuthorized);
        }
    }

    bool HistoryControlPage::isScrolling() const
    {
        return !messagesArea_->isScrollAtBottom();
    }

    QWidget* HistoryControlPage::getWidgetByKey(const Logic::MessageKey& _key) const
    {
        return messagesArea_->getItemByKey(_key);
    }

    HistoryControlPage::WidgetRemovalResult HistoryControlPage::removeExistingWidgetByKey(const Logic::MessageKey& _key)
    {
        auto widget = messagesArea_->getItemByKey(_key);
        if (!widget)
        {
            return WidgetRemovalResult::NotFound;
        }

        if (!isRemovableWidget(widget))
        {
            return WidgetRemovalResult::PersistentWidget;
        }

        messagesArea_->removeWidget(widget);

        return WidgetRemovalResult::Removed;
    }

    void HistoryControlPage::replaceExistingWidgetByKey(const Logic::MessageKey& _key, QWidget* _widget)
    {
        assert(_key.hasId());
        assert(_widget);

        messagesArea_->replaceWidget(_key, _widget);
    }

    void HistoryControlPage::contactAuthorized(const QString& _aimId, bool _res)
    {
        if (_res)
        {
            if (authWidget_ && aimId_ == _aimId)
            {
                messagesArea_->removeWidget(authWidget_);

                authWidget_ = nullptr;
            }
        }
    }

    void HistoryControlPage::authAddContact(const QString& _aimId)
    {
        Logic::getContactListModel()->addContactToCL(_aimId);
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::add_user_auth_widget);
    }

    void HistoryControlPage::authBlockContact(const QString& _aimId)
    {
        Logic::getContactListModel()->blockAndSpamContact(_aimId, false);
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::spam_auth_widget);
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::ignore_auth_widget);

        emit Utils::InterConnector::instance().profileSettingsBack();
    }

    void HistoryControlPage::authDeleteContact(const QString& _aimId)
    {
        Logic::getContactListModel()->removeContactFromCL(_aimId);

        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("contact", _aimId);
        Ui::GetDispatcher()->post_message_to_core(qsl("dialogs/hide"), collection.get());

        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::delete_auth_widget);
    }

    void HistoryControlPage::newPlateShowed()
    {
        qint64 oldNew = newPlatePosition_;
        newPlatePosition_ = -1;
        Logic::GetMessagesModel()->updateNew(aimId_, oldNew, true);
    }

    static Data::DlgState getDlgState(const QString& aimId, bool fromDialog = false)
    {
        Data::DlgState state = Logic::getRecentsModel()->getDlgState(aimId, fromDialog);
        if (state.AimId_ != aimId)
            state = Logic::getUnknownsModel()->getDlgState(aimId, fromDialog);
        return state;
    }

    void HistoryControlPage::update(const QString& _aimId)
    {
        if (_aimId != aimId_)
            return;

        if (!isVisible())
        {
            Logic::GetMessagesModel()->updateNew(aimId_, newPlatePosition_);

            const Data::DlgState state = getDlgState(aimId_);
            newPlatePosition_ = state.LastMsgId_ == state.YoursLastRead_ ? -1 : state.YoursLastRead_;
        }
    }

    void HistoryControlPage::scrollTo(const Logic::MessageKey& key, Logic::scroll_mode_type _scrollMode)
    {
        messagesArea_->scrollTo(key, _scrollMode);
    }

    void HistoryControlPage::updateItems()
    {
        messagesArea_->updateItems();
    }

    void HistoryControlPage::copy(const QString& _text)
    {
        const auto selectionText = messagesArea_->getSelectedText();

#ifdef __APPLE__

        if (!selectionText.isEmpty())
            MacSupport::replacePasteboard(selectionText);
        else
            MacSupport::replacePasteboard(_text);

#else

        if (!selectionText.isEmpty())
            QApplication::clipboard()->setText(selectionText);
        else
            QApplication::clipboard()->setText(_text);

#endif
    }

    void HistoryControlPage::quoteText(const QVector<Data::Quote>& q)
    {
        const auto quotes = messagesArea_->getQuotes();
        messagesArea_->clearSelection();
        emit quote(quotes.isEmpty() ? q : quotes);
    }

    void HistoryControlPage::forwardText(const QVector<Data::Quote>& q)
    {
        const auto quotes = messagesArea_->getQuotes();
        messagesArea_->clearSelection();
        forwardMessage(quotes.isEmpty() ? q : quotes, true);
    }

    void HistoryControlPage::updateState(bool _close)
    {
        Logic::GetMessagesModel()->updateNew(aimId_, newPlatePosition_, _close);

        const Data::DlgState state = getDlgState(aimId_, !_close);

        if (_close)
        {
            newPlatePosition_ = state.LastMsgId_;
            eventFilter_->resetNewPlate();
        }
        else
        {
            if (state.UnreadCount_ == 0)
            {
                newPlatePosition_ = -1;
            }
            else
            {
                qint64 normalized = Logic::GetMessagesModel()->normalizeNewMessagesId(aimId_, state.YoursLastRead_);
                newPlatePosition_ = normalized;
                if (normalized != state.YoursLastRead_)
                    Logic::GetMessagesModel()->updateNew(aimId_, normalized, _close);
            }
        }

        updateItems();
    }

    qint64 HistoryControlPage::getNewPlateId() const
    {
        return newPlatePosition_;
    }

    void HistoryControlPage::sourceReady(const QString& _aimId, bool _is_search, int64_t _mess_id, int64_t _countAfter, Logic::scroll_mode_type _scrollMode)
    {
        if (_aimId != aimId_)
        {
            return;
        }

        assert(itemsData_.empty());

        const bool recvLastMsg = Logic::GetMessagesModel()->getRecvLastMsg(aimId_);
        messagesArea_->setMessageId(_mess_id);
        setRecvLastMessage(recvLastMsg);
        messagesArea_->setIsSearch(_is_search);

        switchToFetchingState(__FUNCLINEA__);

        if (bNewMessageForceShow_ || _scrollMode == Logic::scroll_mode_type::unread)
        {
            if (_mess_id != -1)
                bNewMessageForceShow_ = false;
        }

        Logic::GetMessagesModel()->updateNew(aimId_, newPlatePosition_);

        const auto widgets = Logic::GetMessagesModel()->tail(aimId_, messagesArea_, _is_search, _mess_id);

        for (const auto& item : boost::adaptors::reverse(widgets))
        {
            if (item.second)
                itemsData_.emplace_back(item.first, item.second, Logic::MessagesModel::REQUESTED, item.second->isDeleted());
        }

        if (!itemsData_.empty())
        {
            switchToInsertingState(__FUNCLINEA__);

            postInsertNextMessageSignal(__FUNCLINEA__, !_is_search, _mess_id, _countAfter, _scrollMode);
        }

        if (widgets.size() < Logic::GetMessagesModel()->preloadCount())
        {
            requestMoreMessagesAsync(__FUNCLINEA__);
        }
    }

    bool HistoryControlPage::connectToMessageItem(const Ui::MessageItem* messageItem) const
    {
        const auto list = {
            connect(messageItem, &MessageItem::copy, this, &HistoryControlPage::copy, Qt::QueuedConnection),
            connect(messageItem, &MessageItem::quote, this, &HistoryControlPage::quoteText, Qt::QueuedConnection),
            connect(messageItem, &MessageItem::forward, this, &HistoryControlPage::forwardText, Qt::QueuedConnection),
            connect(messageItem, &MessageItem::avatarMenuRequest, this, &HistoryControlPage::avatarMenuRequest, Qt::QueuedConnection)
        };

        return std::all_of(list.begin(), list.end(), [](const auto& x) { return x; });
    }

    bool HistoryControlPage::connectToComplexMessageItem(const Ui::ComplexMessage::ComplexMessageItem* complexMessageItem) const
    {
        const auto list = {
            connect(complexMessageItem, &ComplexMessage::ComplexMessageItem::copy, this, &HistoryControlPage::copy, Qt::QueuedConnection),
            connect(complexMessageItem, &ComplexMessage::ComplexMessageItem::quote, this, &HistoryControlPage::quoteText, Qt::QueuedConnection),
            connect(complexMessageItem, &ComplexMessage::ComplexMessageItem::forward, this, &HistoryControlPage::forwardText, Qt::QueuedConnection),
            connect(complexMessageItem, &ComplexMessage::ComplexMessageItem::avatarMenuRequest, this, &HistoryControlPage::avatarMenuRequest, Qt::QueuedConnection)
        };
        return std::all_of(list.begin(), list.end(), [](const auto& x) { return x; });
    }

    void HistoryControlPage::insertNextMessageSlot(bool _isMoveToBottomIfNeed, int64_t _mess_id, int64_t _countAfter, Logic::scroll_mode_type _scrollMode)
    {
        __INFO("smooth_scroll", "entering signal handler\n""    type=<insertNextMessageSlot>\n""    state=<" << state_ << ">\n""    items size=<" << itemsData_.size() << ">");

        if (itemsData_.empty())
        {
            switchToIdleState(__FUNCLINEA__);

            if (isMessagesRequestPostponed_)
            {
                __INFO("smooth_scroll", "resuming postponed messages request\n""    requested at=<" << dbgWherePostponed_ << ">");

                isMessagesRequestPostponed_ = false;
                dbgWherePostponed_ = nullptr;

                requestMoreMessagesAsync(__FUNCLINEA__, isMessagesRequestPostponedDown_);
            }

            return;
        }

        bool isScrollAtBottom = messagesArea_->isScrollAtBottom();
        int unreadCount = 0;

        auto update_unreads = [isScrollAtBottom, &unreadCount](const ItemData& _data)
        {
            if (!isScrollAtBottom && _data.Mode_ == Logic::MessagesModel::BASE && !_data.Key_.isOutgoing())
                ++unreadCount;
        };

        MessagesScrollArea::WidgetsList insertWidgets;
        while (!itemsData_.empty())
        {
            auto data = itemsData_.front();
            itemsData_.pop_front();

            if (data.IsDeleted_)
            {
                continue;
            }

            __INFO(
                "history_control",
                "inserting widget\n"
                "    key=<" << data.Key_.getId() << ";" << data.Key_.getInternalId() << ">");

            auto existing = getWidgetByKey(data.Key_);
            if (existing)
            {
                auto isNewItemChatEvent = qobject_cast<Ui::ChatEventItem*>(data.Widget_);

                if (!isUpdateableWidget(existing) && !isNewItemChatEvent)
                {
                    {
                        auto messageItem = qobject_cast<Ui::MessageItem*>(existing);
                        auto newMessageItem = qobject_cast<Ui::MessageItem*>(data.Widget_);

                        if (messageItem)
                            messageItem->setDeliveredToServer(!data.Key_.isPending());

                        if (messageItem && newMessageItem)
                        {
                            messageItem->updateWith(*newMessageItem);
                        }
                    }

                    {
                        auto messageItem = qobject_cast<Ui::ComplexMessage::ComplexMessageItem*>(existing);
                        auto newMessageItem = qobject_cast<Ui::ComplexMessage::ComplexMessageItem*>(data.Widget_);

                        if (messageItem && newMessageItem)
                        {
                            messageItem->setDeliveredToServer(!data.Key_.isPending());
                            messageItem->updateWith(*newMessageItem);
                        }
                    }

                    __INFO("history_control", "widget insertion discarded (persistent widget)\n""	key=<" << data.Key_.getId() << ";" << data.Key_.getInternalId() << ">");
                    data.Widget_->deleteLater();

                    continue;
                }

                const auto isNewTabletReplacement = (
                    existing->property("New").toBool() ||
                    data.Widget_->property("New").toBool());

                if (isNewTabletReplacement)
                {
                    removeExistingWidgetByKey(data.Key_);
                }
                else
                {
                    auto messageItem = qobject_cast<Ui::MessageItem*>(existing);
                    auto newMessageItem = qobject_cast<Ui::MessageItem*>(data.Widget_);

                    if (messageItem && newMessageItem)
                    {
                        messageItem->setDeliveredToServer(!data.Key_.isPending());
                        messageItem->updateWith(*newMessageItem);

                        data.Widget_->deleteLater();

                        if (!data.Key_.isOutgoing() && (data.Mode_ == Logic::MessagesModel::BASE))
                        {
                            auto name = newMessageItem->getMchatSenderAimId();
                            const auto contact = Logic::getContactListModel()->getContactItem(name);
                            if (contact)
                                name = contact->Get()->GetDisplayName();
                            typingChattersAimIds_.remove(name);
                            if (typingChattersAimIds_.empty())
                                hideTypingWidgets();
                            else
                                updateTypingWidgets();
                        }

                        update_unreads(data);

                        continue;
                    }

                    auto complexMessageItem = qobject_cast<Ui::ComplexMessage::ComplexMessageItem*>(existing);
                    auto newComplexMessageItem = qobject_cast<Ui::ComplexMessage::ComplexMessageItem*>(data.Widget_);

                    if (complexMessageItem && newComplexMessageItem)
                    {
                        complexMessageItem->setDeliveredToServer(!data.Key_.isPending());
                        complexMessageItem->updateWith(*newComplexMessageItem);

                        data.Widget_->deleteLater();

                        continue;
                    }

                    if (data.Key_.isDate())
                    {
                        data.Widget_->deleteLater();

                        continue;
                    }

                    removeExistingWidgetByKey(data.Key_);
                }
            }
            else
            {
                update_unreads(data);
            }

            __TRACE("history_control", "inserting widget position info\n""	key=<" << data.Key_.getId() << ";" << data.Key_.getInternalId() << ">");

            // prepare widget for insertion

            auto messageItem = qobject_cast<Ui::MessageItem*>(data.Widget_);
            if (messageItem)
            {
                if (!connectToMessageItem(messageItem))
                    assert(!"can not connect to messageItem");

                if (!data.Key_.isOutgoing() && data.Mode_ == Logic::MessagesModel::BASE)
                {
                    auto name = messageItem->getMchatSenderAimId();
                    auto contact = Logic::getContactListModel()->getContactItem(name);
                    if (contact)
                        name = contact->Get()->GetDisplayName();
                    typingChattersAimIds_.remove(name);
                    if (typingChattersAimIds_.empty())
                        hideTypingWidgets();
                    else
                        updateTypingWidgets();
                }
            }
            else
            {
                auto complexMessageItem = qobject_cast<Ui::ComplexMessage::ComplexMessageItem*>(data.Widget_);
                if (complexMessageItem)
                {
                    if (!connectToComplexMessageItem(complexMessageItem))
                        assert(!"can not connect to complexMessageItem");
                }
                else if (auto layout = data.Widget_->layout())
                {
                    auto index = 0;
                    while (auto child = layout->itemAt(index++))
                    {
                        auto childWidget = child->widget();
                        if (auto messageItem = qobject_cast<Ui::MessageItem*>(childWidget))
                        {
                            if (!connectToMessageItem(messageItem))
                                assert(!"can not connect to messageItem");
                        }
                        else if (auto complexMessageItem = qobject_cast<Ui::ComplexMessage::ComplexMessageItem*>(childWidget))
                        {
                            if (!connectToComplexMessageItem(complexMessageItem))
                                assert(!"can not connect to complexMessageItem");;
                        }
                    }
                }
            }

            // insert and display the widget

            insertWidgets.emplace_back(
                std::make_pair(data.Key_, data.Widget_)
            );
        }

        //qDebug("insertWidgets:%i", insertWidgets.size());

        if (!insertWidgets.empty())
            messagesArea_->insertWidgets(insertWidgets, _isMoveToBottomIfNeed, _mess_id, _countAfter, _scrollMode);

        if (unreadCount)
        {
            buttonDown_->addUnreadMessages(unreadCount);
        }
        else
        {
            buttonDown_->setUnreadMessages(0);
        }

        postInsertNextMessageSignal(__FUNCLINEA__, _isMoveToBottomIfNeed);

        if (!_isMoveToBottomIfNeed)
        {
            changeDlgState(dlgState_);
        }
    }

    void HistoryControlPage::removeWidget(const Logic::MessageKey& _key)
    {
        if (isScrolling())
        {
            emit needRemove(_key);
            return;
        }

        __TRACE(
            "history_control",
            "requested to remove the widget\n"
            "	key=<" << _key.getId() << ";" << _key.getInternalId() << ">");


        removeRequests_.erase(_key);

        const auto result = removeExistingWidgetByKey(_key);
        assert(result > WidgetRemovalResult::Min);
        assert(result < WidgetRemovalResult::Max);
    }

    bool HistoryControlPage::touchScrollInProgress() const
    {
        return messagesArea_->touchScrollInProgress();
    }

    void HistoryControlPage::unloadWidgets(QVector<Logic::MessageKey> _keysToUnload)
    {
        assert(!_keysToUnload.empty());

        if (_keysToUnload.empty())
            return;

        std::sort(_keysToUnload.begin(), _keysToUnload.end());

        const auto &lastKey = _keysToUnload.constLast();
        assert(!lastKey.isEmpty());

        const auto keyAfterLast = Logic::GetMessagesModel()->findFirstKeyAfter(aimId_, lastKey);

        for (auto it = _keysToUnload.cbegin(), end = std::prev(_keysToUnload.cend()); it != end; ++it)
        {
            const auto& key = *it;
            assert(!key.isEmpty());

            emit needRemove(key);
        }

        if (!keyAfterLast.isEmpty())
        {
            Logic::GetMessagesModel()->setLastKey(keyAfterLast, aimId_);
        }
    }

    void HistoryControlPage::loadChatInfo(bool _isFullListLoaded)
    {
        _isFullListLoaded |= Utils::InterConnector::instance().isSidebarVisible();
        const bool isAdmin = chatMembersModel_ && (chatMembersModel_->isAdmin() || chatMembersModel_->isModer());
        const int count = isAdmin ? chatMembersModel_->getMembersCount() : (_isFullListLoaded ? Logic::MaxMembersLimit : 0);
        chatInfoSequence_ = Logic::ChatMembersModel::loadAllMembers(aimId_, count, this);
    }

    void HistoryControlPage::initStatus()
    {
        Logic::ContactItem* contact = Logic::getContactListModel()->getContactItem(aimId_);
        if (!contact)
            return;

        if (contact->is_chat())
        {
            loadChatInfo(false);
            officialMark_->setVisible(false);
        }
        else
        {
            contactStatus_->setText(Logic::getContactListModel()->getStatusString(aimId_));

            officialMark_->setVisible(Logic::getContactListModel()->isOfficial(aimId_));
        }
    }

    void HistoryControlPage::updateName()
    {
        Logic::ContactItem* contact = Logic::getContactListModel()->getContactItem(aimId_);
        if (contact)
            eventFilter_->ResetContactName(contact->Get()->GetDisplayName());
    }

    void HistoryControlPage::chatInfoFailed(qint64 _seq, core::group_chat_info_errors _errorCode)
    {
        if (Logic::ChatMembersModel::receiveMembers(chatInfoSequence_, _seq, this))
        {
            if (_errorCode == core::group_chat_info_errors::not_in_chat || _errorCode == core::group_chat_info_errors::blocked)
            {
                contactStatus_->setText(QT_TRANSLATE_NOOP("groupchats", "You are not a member of this chat"));
                Logic::getContactListModel()->setYourRole(aimId_, qsl("notamember"));
                addMemberButton_->hide();
                nameWidget_->setCursor(Qt::ArrowCursor);
                disconnect(nameWidget_, &ClickWidget::clicked, this, &HistoryControlPage::nameClicked);
            }
        }
    }

    void HistoryControlPage::chatInfo(qint64 _seq, const std::shared_ptr<Data::ChatInfo>& _info)
    {
        if (!Logic::ChatMembersModel::receiveMembers(chatInfoSequence_, _seq, this))
        {
            return;
        }

        __INFO(
            "chat_info",
            "incoming chat info event\n"
            "    contact=<" << aimId_ << ">\n"
            "    your_role=<" << _info->YourRole_ << ">"
        );

        addMemberButton_->setVisible(_info->YourRole_ != ql1s("notamember"));
        eventFilter_->ResetContactName(_info->Name_);
        setContactStatusClickable(true);

        const QString state = QString::number(_info->MembersCount_) % ql1c(' ')
            % Utils::GetTranslator()->getNumberString(
                _info->MembersCount_,
                QT_TRANSLATE_NOOP3("chat_page", "member", "1"),
                QT_TRANSLATE_NOOP3("chat_page", "members", "2"),
                QT_TRANSLATE_NOOP3("chat_page", "members", "5"),
                QT_TRANSLATE_NOOP3("chat_page", "members", "21")
            );
        contactStatus_->setText(state);
        isPublicChat_ = _info->Public_;
        if (chatMembersModel_ == nullptr)
        {
            chatMembersModel_ = new Logic::ChatMembersModel(_info, this);
            if (chatMembersModel_->isAdmin() || chatMembersModel_->isModer())
                loadChatInfo(false);
        }
        else
        {
            chatMembersModel_->updateInfo(_info, false);
            emit updateMembers();
        }

        nameWidget_->setCursor(Qt::PointingHandCursor);
        connect(nameWidget_, &ClickWidget::clicked, this, &HistoryControlPage::nameClicked, Qt::UniqueConnection);
    }

    void HistoryControlPage::contactChanged(const QString& _aimId)
    {
        assert(!aimId_.isEmpty());

        if (_aimId == aimId_)
        {
            initStatus();
            updateName();
        }
    }

    void HistoryControlPage::editMembers()
    {
        Utils::InterConnector::instance().showSidebar(aimId_, SidebarPages::all_members);
    }

    void HistoryControlPage::onNewMessageReceived(const QString& _aimId)
    {
        if (_aimId != aimId_)
            return;

        messagesArea_->enableViewportShifting(true);
        //if (messagesArea_->getIsSearch() && messagesArea_->isScrollAtBottom())
        //    messagesArea_->setIsSearch(false);
    }

    void HistoryControlPage::open()
    {
        Utils::InterConnector::instance().insertTopWidget(aimId_, topWidget_);

        initStatus();

        int new_theme_id = get_qt_theme_settings()->contactOpenned(aimId());
        if (setThemeId_ != new_theme_id)
        {
            updateWidgetsTheme();
            setThemeId_ = new_theme_id;
        }
        else
        {
            messagesOverlayFirst_->updateStyle();
            messagesOverlaySecond_->updateStyle();
        }

        eventFilter_->updateSizes();

        resumeVisibleItems();

        updateMentionsButtonDelayed();
    }

    void HistoryControlPage::showMainTopPanel()
    {
        topWidget_->setCurrentIndex(TopWidget::Main);
    }

    void HistoryControlPage::showThemesTopPanel(bool _showSetToCurrent, ThemePanelCallback _callback)
    {
        Logic::getRecentsModel()->sendLastRead(aimId_);
        topWidget_->showThemeWidget(_showSetToCurrent, _callback);
    }

    bool HistoryControlPage::requestMoreMessagesAsync(const char* _dbgWhere, bool _isMoveToBottomIfNeed)
    {
        if (isStateFetching())
        {
            __INFO(
                "smooth_scroll",
                "requesting more messages\n"
                "    status=<cancelled>\n"
                "    reason=<already fetching>\n"
                "    from=<" << _dbgWhere << ">"
            );

            return false;
        }

        if (isStateInserting())
        {
            __INFO(
                "smooth_scroll",
                "requesting more messages\n"
                "    status=<postponed>\n"
                "    reason=<inserting>\n"
                "    from=<" << _dbgWhere << ">"
            );

            postponeMessagesRequest(_dbgWhere, _isMoveToBottomIfNeed);

            return true;
        }

        __INFO(
            "smooth_scroll",
            "requesting more messages\n"
            "    status=<requested>\n"
            "    from=<" << _dbgWhere << ">"
        );

        switchToFetchingState(__FUNCLINEA__);

        emit requestMoreMessagesSignal(_isMoveToBottomIfNeed);

        return true;
    }

    const QString& HistoryControlPage::aimId() const
    {
        return aimId_;
    }

    void HistoryControlPage::cancelSelection()
    {
        assert(messagesArea_);
        messagesArea_->cancelSelection();
    }

    void HistoryControlPage::messageKeyUpdated(const QString& _aimId, const Logic::MessageKey& _key)
    {
        assert(_key.hasId());

        if (_aimId != aimId_)
        {
            return;
        }

        messagesArea_->updateItemKey(_key);
        Ui::HistoryControlPageItem* msg = Logic::GetMessagesModel()->getById(aimId_, _key, messagesArea_);
        if (msg)
        {
            itemsData_.emplace_back(_key, msg, Logic::MessagesModel::PENDING, msg->isDeleted());
            if (!isStateInserting())
            {
                if (isStateIdle())
                {
                    switchToFetchingState(__FUNCLINEA__);
                }
                switchToInsertingState(__FUNCLINEA__);
                postInsertNextMessageSignal(__FUNCLINEA__);
            }
        }
        else
        {
            removeExistingWidgetByKey(_key);
        }
    }

    void HistoryControlPage::updated(const QVector<Logic::MessageKey>& _list, const QString& _aimId, unsigned _mode)
    {
        if (_aimId != aimId_)
        {
            return;
        }

        const auto isHole = (_mode == Logic::MessagesModel::HOLE);
        if (isHole)
        {
            eventFilter_->resetNewPlate();
        }

        for (const auto& key : _list)
        {
            auto msg = Logic::GetMessagesModel()->getById(aimId_, key, messagesArea_);
            if (!msg)
            {
                continue;
            }

            itemsData_.emplace_back(key, msg, _mode, msg->isDeleted());

            if (key.getType() == core::message_type::chat_event)
            {
                updateChatInfo();
            }
        }

        setRecvLastMessage(Logic::GetMessagesModel()->getRecvLastMsg(aimId_));

        if (!itemsData_.empty() && !isStateInserting())
        {
            if (isStateIdle())
            {
                switchToFetchingState(__FUNCLINEA__);
            }

            switchToInsertingState(__FUNCLINEA__);

            postInsertNextMessageSignal(__FUNCLINEA__);
        }

        if (!messagesArea_->isViewportFull())
        {
            requestMoreMessagesAsync(__FUNCLINEA__);
        }
    }

    void HistoryControlPage::deleted(const QVector<Logic::MessageKey>& _list, const QString& _aimId)
    {
        if (_aimId != aimId_)
            return;

        setRecvLastMessage(Logic::GetMessagesModel()->getRecvLastMsg(aimId_));

        for (const auto& keyToRemove : _list)
        {
            removeExistingWidgetByKey(keyToRemove);

            itemsData_.remove_if([&keyToRemove](const auto& x) { return x.Key_ == keyToRemove; });
        }
    }

    void HistoryControlPage::requestMoreMessagesSlot(bool _isMoveToBottomIfNeed)
    {
        const auto widgets = Logic::GetMessagesModel()->more(aimId_, messagesArea_, _isMoveToBottomIfNeed);

        setRecvLastMessage(Logic::GetMessagesModel()->getRecvLastMsg(aimId_));

        for (const auto& item : boost::adaptors::reverse(widgets))
        {
            if (item.second)
                itemsData_.emplace_back(item.first, item.second, Logic::MessagesModel::REQUESTED, item.second->isDeleted());
        }

        switchToInsertingState(__FUNCLINEA__);

        postInsertNextMessageSignal(__FUNCLINEA__, _isMoveToBottomIfNeed);

        if (!widgets.empty() && !messagesArea_->isViewportFull())
        {
            requestMoreMessagesAsync(__FUNCLINEA__, _isMoveToBottomIfNeed);
        }

        //if (!messagesArea_->isViewportFull() && widgets.isEmpty() && messagesArea_->getIsSearch())
        //{
        //    requestMoreMessagesAsync(__FUNCLINEA__, !_isMoveToBottomIfNeed);
        //}
    }

    void HistoryControlPage::downPressed()
    {
        buttonDown_->setUnreadMessages(0);
        scrollToBottom();
    }

    void HistoryControlPage::scrollMovedToBottom()
    {
        buttonDown_->setUnreadMessages(0);
    }

    void HistoryControlPage::autoScroll(bool _enabled)
    {
        if (!_enabled)
        {
            return;
        }
        //	unloadWidgets();
        buttonDown_->setUnreadMessages(0);
    }

    void HistoryControlPage::searchButtonClicked()
    {
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::cl_search_dialog);
        emit Utils::InterConnector::instance().startSearchInDialog(aimId_);
    }

    void HistoryControlPage::callAudioButtonClicked()
    {
        Ui::GetDispatcher()->getVoipController().setStartA(aimId_.toUtf8(), false);
        if (MainPage* mainPage = MainPage::instance())
        {
            mainPage->raiseVideoWindow();
        }
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::call_from_chat);
    }
    void HistoryControlPage::callVideoButtonClicked()
    {
        Ui::GetDispatcher()->getVoipController().setStartV(aimId_.toUtf8(), false);
        if (MainPage* mainPage = MainPage::instance())
        {
            mainPage->raiseVideoWindow();
        }
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::videocall_from_chat);
    }

    void HistoryControlPage::moreButtonClicked()
    {
        if (Utils::InterConnector::instance().isSidebarVisible())
        {
            Utils::InterConnector::instance().setSidebarVisible(false);
        }
        else
        {
            Utils::InterConnector::instance().showSidebar(aimId_, SidebarPages::menu_page);
        }
    }

    void HistoryControlPage::focusOutEvent(QFocusEvent* _event)
    {
        QWidget::focusOutEvent(_event);
    }

    void HistoryControlPage::wheelEvent(QWheelEvent* _event)
    {
        if (!hasFocus())
        {
            return;
        }

        return QWidget::wheelEvent(_event);
    }

    void HistoryControlPage::addMember()
    {
        auto contact = Logic::getContactListModel()->getContactItem(aimId_);
        if (!contact)
            return;

        if (!contact->is_chat() || !chatMembersModel_)
            return;

        assert(!!chatMembersModel_);

        if (!chatMembersModel_->isFullListLoaded_)
        {
            chatMembersModel_->loadAllMembers();
        }

        SelectContactsWidget select_members_dialog(chatMembersModel_, Logic::MembersWidgetRegim::SELECT_MEMBERS,
            QT_TRANSLATE_NOOP("groupchats", "Add to chat"), QT_TRANSLATE_NOOP("popup_window", "DONE"), this);
        emit Utils::InterConnector::instance().searchEnd();
        connect(this, &HistoryControlPage::updateMembers, &select_members_dialog, &SelectContactsWidget::UpdateMembers, Qt::QueuedConnection);

        if (select_members_dialog.show() == QDialog::Accepted)
        {
            postAddChatMembersFromCLModelToCore(aimId_);
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::groupchat_add_member_dialog);
        }
        else
        {
            Logic::getContactListModel()->clearChecked();
        }
    }

    void HistoryControlPage::renameContact()
    {
        QString result_chat_name;

        auto result = Utils::NameEditor(
            this,
            Logic::getContactListModel()->getDisplayName(aimId_),
            QT_TRANSLATE_NOOP("popup_window", "SAVE"),
            QT_TRANSLATE_NOOP("popup_window", "Contact name"),
            result_chat_name);

        if (result && !result_chat_name.isEmpty())
        {
            Logic::getContactListModel()->renameContact(aimId_, result_chat_name);
        }
    }

    void HistoryControlPage::setState(const State _state, const char* _dbgWhere)
    {
        assert(_state > State::Min);
        assert(_state < State::Max);
        assert(state_ > State::Min);
        assert(state_ < State::Max);

        __INFO(
            "smooth_scroll",
            "switching state\n"
            "    from=<" << state_ << ">\n"
            "    to=<" << _state << ">\n"
            "    where=<" << _dbgWhere << ">\n"
            "    items num=<" << itemsData_.size() << ">"
        );

        state_ = _state;
    }

    bool HistoryControlPage::isState(const State _state) const
    {
        assert(state_ > State::Min);
        assert(state_ < State::Max);
        assert(_state > State::Min);
        assert(_state < State::Max);

        return (state_ == _state);
    }

    bool HistoryControlPage::isStateFetching() const
    {
        return isState(State::Fetching);
    }

    bool HistoryControlPage::isStateIdle() const
    {
        return isState(State::Idle);
    }

    bool HistoryControlPage::isStateInserting() const
    {
        return isState(State::Inserting);
    }

    void HistoryControlPage::postInsertNextMessageSignal(const char * _dbgWhere, bool _isMoveToBottomIfNeed, int64_t _mess_id, int64_t _countAfter, Logic::scroll_mode_type _scrollMode)
    {
        __INFO(
            "smooth_scroll",
            "posting signal\n"
            "    type=<insertNextMessageSignal>\n"
            "    from=<" << _dbgWhere << ">"
        );

        //qDebug("postInsertNextMessageSignal %s", _dbgWhere);
        emit insertNextMessageSignal(_isMoveToBottomIfNeed, _mess_id, _countAfter, _scrollMode);
    }

    void HistoryControlPage::postponeMessagesRequest(const char *_dbgWhere, bool _isDown)
    {
        assert(isStateInserting());

        dbgWherePostponed_ = _dbgWhere;
        isMessagesRequestPostponed_ = true;
        isMessagesRequestPostponedDown_ = _isDown;
    }

    void HistoryControlPage::switchToIdleState(const char* _dbgWhere)
    {
        setState(State::Idle, _dbgWhere);
    }

    void HistoryControlPage::switchToInsertingState(const char* _dbgWhere)
    {
        setState(State::Inserting, _dbgWhere);
    }

    void HistoryControlPage::switchToFetchingState(const char* _dbgWhere)
    {
        setState(State::Fetching, _dbgWhere);
    }

    void HistoryControlPage::setContactStatusClickable(bool _isEnabled)
    {
        if (_isEnabled)
        {
            contactStatusWidget_->setCursor(Qt::PointingHandCursor);
            connect(contactStatus_, &LabelEx::clicked, this, &HistoryControlPage::editMembers, Qt::QueuedConnection);
        }
        else
        {
            contactStatusWidget_->setCursor(Qt::ArrowCursor);
            disconnect(contactStatus_, &LabelEx::clicked, this, &HistoryControlPage::editMembers);
        }
        isContactStatusClickable_ = _isEnabled;
    }

    void HistoryControlPage::updateChatInfo()
    {
        if (!Logic::getContactListModel()->isChat(aimId_))
            return;

        if (chatMembersModel_ == nullptr)
        {
            loadChatInfo(false);
        }
        else
        {
            loadChatInfo(chatMembersModel_->isFullListLoaded_);
        }
    }

    void HistoryControlPage::onReachedFetchingDistance(bool _isMoveToBottomIfNeed)
    {
        __INFO(
            "smooth_scroll",
            "initiating messages preloading..."
        );

        requestMoreMessagesAsync(__FUNCLINEA__, _isMoveToBottomIfNeed);
    }

    void HistoryControlPage::fetchMore(const QString& _aimId)
    {
        if (_aimId != aimId_)
        {
            return;
        }

        requestMoreMessagesAsync(__FUNCLINEA__);
    }

    void HistoryControlPage::nameClicked()
    {
        if (Utils::InterConnector::instance().isSidebarVisible())
            Utils::InterConnector::instance().setSidebarVisible(false);
        else
            if (Logic::getContactListModel()->isChat(aimId_))
                Utils::InterConnector::instance().showSidebar(aimId_, menu_page);
            else
                emit Utils::InterConnector::instance().profileSettingsShow(aimId_);
    }

    void HistoryControlPage::showEvent(QShowEvent* _event)
    {
        appendAuthControlIfNeed();
        updateFooterButtonsPositions();

        QWidget::showEvent(_event);
    }

    void HistoryControlPage::updateFooterButtonsPositions()
    {
        const auto rect = messagesArea_->geometry();

        auto size = Utils::scale_value(button_down_size);
        auto x = Utils::scale_value(button_down_offset_x);
        auto y = Utils::scale_value(button_down_offset_y);

        /// 0.35 is experimental offset to fully hided
        setButtonDownPositions(rect.width() - size - x, rect.height() - size - y, rect.height() + 0.35*size - y);

        if (buttonDown_->isVisible())
        {
            y += size;

            size = Utils::scale_value(button_mentions_size);
        }

        setButtonMentionPositions(rect.width() - size - x, rect.height() - size - y, rect.height() + 0.35*size - y);
    }

    void HistoryControlPage::setButtonDownPositions(int x_showed, int y_showed, int y_hided)
    {
        buttonDownShowPosition_.setX(x_showed);
        buttonDownShowPosition_.setY(y_showed);

        buttonDownHidePosition_.setX(x_showed);
        buttonDownHidePosition_.setY(y_hided);

        if (buttonDown_->isVisible())
            buttonDown_->move(buttonDownShowPosition_);
    }

    void HistoryControlPage::setButtonMentionPositions(int x_showed, int y_showed, int y_hided)
    {
        QPoint buttonShowPosition;
        buttonShowPosition.setX(x_showed);
        buttonShowPosition.setY(y_showed);

        buttonMentions_->move(buttonShowPosition);
    }

    void HistoryControlPage::positionMentionCompleter(const QRect & _areaRect)
    {
        mentionCompleter_->setFixedWidth(_areaRect.width());
        mentionCompleter_->move(_areaRect.left(), _areaRect.height() - mentionCompleter_->height());
    }

    MentionCompleter* HistoryControlPage::getMentionCompleter()
    {
        return mentionCompleter_;
    }

    void HistoryControlPage::changeDlgState(const Data::DlgState& _dlgState)
    {
        auto contact = Logic::getContactListModel()->getContactItem(aimId_);
        if (!contact)
            return;

        if (contact->is_chat())
            changeDlgStateChat(_dlgState);
        else
            changeDlgStateContact(_dlgState);
    }

    void HistoryControlPage::changeDlgStateChat(const Data::DlgState& _dlgState)
    {
        if (_dlgState.AimId_ != aimId_)
            return;

        dlgState_ = _dlgState;

        bool incomingFound = false;

        messagesArea_->enumerateWidgets([&_dlgState, &incomingFound](QWidget* _item, const bool)
        {
            if (qobject_cast<Ui::ServiceMessageItem*>(_item))
            {
                return true;
            }

            auto pageItem = qobject_cast<Ui::HistoryControlPageItem*>(_item);
            if (!pageItem)
            {
                return true;
            }

            auto isOutgoing = false;
            auto isVoipMessage = false;

            if (auto messageItem = qobject_cast<Ui::MessageItem*>(_item))
            {
                isOutgoing = messageItem->isOutgoing();
            }
            else if (auto complexMessageItem = qobject_cast<Ui::ComplexMessage::ComplexMessageItem*>(_item))
            {
                isOutgoing = complexMessageItem->isOutgoing();
            }
            else if (auto voipMessageItem = qobject_cast<Ui::VoipEventItem*>(_item))
            {
                isOutgoing = voipMessageItem->isOutgoing();
                isVoipMessage = true;
            }

            if (!isOutgoing)
                incomingFound = true;

            if (incomingFound)
            {
                pageItem->setLastStatus(LastStatus::None);
                return true;
            }

            const auto itemId = pageItem->getId();
            assert(itemId >= -1);

            const bool itemHasId = itemId != -1;

            const bool isOutgoingNonVoipMessage = isOutgoing && !isVoipMessage;
            const auto isItemDeliveredToPeer = itemHasId;

            if (isItemDeliveredToPeer)
            {
                pageItem->setLastStatus(LastStatus::DeliveredToPeer);
                return true;
            }

            const auto markItemAsLastSend = isOutgoingNonVoipMessage && !itemHasId;
            if (markItemAsLastSend)
            {
                pageItem->setLastStatus(LastStatus::Pending);
                return true;
            }

            pageItem->setLastStatus(LastStatus::None);
            return true;

        }, false);
    }

    void HistoryControlPage::changeDlgStateContact(const Data::DlgState& _dlgState)
    {
        if (_dlgState.AimId_ != aimId_)
            return;

        dlgState_ = _dlgState;

        Ui::HistoryControlPageItem* lastReadItem = nullptr;

        messagesArea_->enumerateWidgets([&_dlgState, &lastReadItem](QWidget* _item, const bool)
        {
            if (qobject_cast<Ui::ServiceMessageItem*>(_item) /*|| qobject_cast<Ui::ChatEventItem*>(_item)*/)
            {
                return true;
            }

            auto pageItem = qobject_cast<Ui::HistoryControlPageItem*>(_item);
            if (!pageItem)
            {
                return true;
            }

            auto isOutgoing = false;
            auto isVoipMessage = false;

            if (auto messageItem = qobject_cast<Ui::MessageItem*>(_item))
            {
                isOutgoing = messageItem->isOutgoing();
            }
            else if (auto complexMessageItem = qobject_cast<Ui::ComplexMessage::ComplexMessageItem*>(_item))
            {
                isOutgoing = complexMessageItem->isOutgoing();
            }
            else if (auto voipMessageItem = qobject_cast<Ui::VoipEventItem*>(_item))
            {
                isOutgoing = voipMessageItem->isOutgoing();
                isVoipMessage = true;
            }

            if (lastReadItem)
            {
                pageItem->setLastStatus(LastStatus::None);
                return true;
            }

            const auto itemId = pageItem->getId();
            assert(itemId >= -1);

            const bool isChatEvent = !!qobject_cast<Ui::ChatEventItem*>(_item);
            const bool itemHasId = itemId != -1;
            const bool isItemRead = itemHasId && (itemId <= _dlgState.TheirsLastRead_);

            const bool markItemAsLastRead = (isOutgoing && isItemRead) || (!isOutgoing && !isChatEvent);
            if (!lastReadItem && markItemAsLastRead)
            {
                lastReadItem = pageItem;
                return true;
            }

            const bool isOutgoingNonVoipMessage = isOutgoing && !isVoipMessage;
            const auto isItemDeliveredToPeer = itemHasId && (itemId <= _dlgState.TheirsLastDelivered_);
            const auto markItemAsLastDeliveredToPeer = isOutgoingNonVoipMessage && isItemDeliveredToPeer;

            if (markItemAsLastDeliveredToPeer)
            {
                pageItem->setLastStatus(LastStatus::DeliveredToPeer);
                return true;
            }

            const auto markItemAsLastDeliveredToServer = (isOutgoingNonVoipMessage && itemHasId);
            if (markItemAsLastDeliveredToServer && !lastReadItem)
            {
                pageItem->setLastStatus(LastStatus::DeliveredToServer);
                return true;
            }

            const auto markItemAsLastSend = isOutgoingNonVoipMessage && !itemHasId;
            if (markItemAsLastSend)
            {
                pageItem->setLastStatus(LastStatus::Pending);
                return true;
            }

            pageItem->setLastStatus(LastStatus::None);
            return true;

        }, false);

        if (lastReadItem)
            lastReadItem->setLastStatus(LastStatus::Read);
    }

    void HistoryControlPage::avatarMenuRequest(const QString& _aimId)
    {
        auto menu = new ContextMenu(this);
        menu->addActionWithIcon(QIcon(Utils::parse_image_name(qsl(":/context_menu/mention_100"))), QT_TRANSLATE_NOOP("avatar_menu", "Mention"), makeData(qsl("mention"), _aimId));

        if (chatMembersModel_ && (chatMembersModel_->isModer() || chatMembersModel_->isAdmin()))
        {
            auto cont = chatMembersModel_->getMemberItem(_aimId);
            if (!cont)
                return;

            bool myInfo = cont->AimId_ == MyInfo()->aimId();
            if (cont->Role_ != ql1s("admin") && chatMembersModel_->isAdmin())
            {
                if (cont->Role_ == ql1s("moder"))
                    menu->addActionWithIcon(QIcon(Utils::parse_image_name(qsl(":/context_menu/admin_off_100"))), QT_TRANSLATE_NOOP("sidebar", "Revoke admin role"), makeData(qsl("revoke_admin"), _aimId));
                else
                    menu->addActionWithIcon(QIcon(Utils::parse_image_name(qsl(":/context_menu/admin_100"))), QT_TRANSLATE_NOOP("sidebar", "Make admin"), makeData(qsl("make_admin"), _aimId));
            }

            if (cont->Role_ == ql1s("member"))
                menu->addActionWithIcon(QIcon(Utils::parse_image_name(qsl(":/context_menu/readonly_100"))), QT_TRANSLATE_NOOP("sidebar", "Ban to write"), makeData(qsl("make_readonly"), _aimId));
            else if (cont->Role_ == ql1s("readonly"))
                menu->addActionWithIcon(QIcon(Utils::parse_image_name(qsl(":/context_menu/readonly_off_100"))), QT_TRANSLATE_NOOP("sidebar", "Allow to write"), makeData(qsl("revoke_readonly"), _aimId));

            if (myInfo || (cont->Role_ != ql1s("admin") && cont->Role_ != ql1s("moder")))
                menu->addActionWithIcon(QIcon(Utils::parse_image_name(qsl(":/context_menu/delete_100"))), QT_TRANSLATE_NOOP("sidebar", "Delete from chat"), makeData(qsl("remove"), _aimId));
            if (!myInfo)
                menu->addActionWithIcon(QIcon(Utils::parse_image_name(qsl(":/context_menu/profile_100"))), QT_TRANSLATE_NOOP("sidebar", "Profile"), makeData(qsl("profile"), _aimId));
            if (cont->Role_ != ql1s("admin") && cont->Role_ != ql1s("moder"))
                menu->addActionWithIcon(QIcon(Utils::parse_image_name(qsl(":/context_menu/block_100"))), QT_TRANSLATE_NOOP("sidebar", "Block"), makeData(qsl("block"), _aimId));
            if (!myInfo)
                menu->addActionWithIcon(QIcon(Utils::parse_image_name(qsl(":/context_menu/spam_100"))), QT_TRANSLATE_NOOP("sidebar", "Report spam"), makeData(qsl("spam"), _aimId));
        }

        connect(menu, &ContextMenu::triggered, this, &HistoryControlPage::avatarMenu, Qt::QueuedConnection);
        connect(menu, &ContextMenu::triggered, menu, &ContextMenu::deleteLater, Qt::QueuedConnection);
        connect(menu, &ContextMenu::aboutToHide, menu, &ContextMenu::deleteLater, Qt::QueuedConnection);

        menu->popup(QCursor::pos());
    }

    void HistoryControlPage::avatarMenu(QAction* _action)
    {
        const auto params = _action->data().toMap();
        const auto command = params[qsl("command")].toString();
        const auto aimId = params[qsl("aimid")].toString();

        if (command == ql1s("mention"))
        {
            mention(aimId);
        }
        else if (command == ql1s("remove"))
        {
            deleteMemberDialog(chatMembersModel_, aimId, Logic::MEMBERS_LIST, this);
        }
        else if (command == ql1s("profile"))
        {
            Utils::InterConnector::instance().showSidebar(aimId, profile_page);
        }
        else if (command == ql1s("spam"))
        {
            if (Logic::getContactListModel()->blockAndSpamContact(aimId))
            {
                Logic::getContactListModel()->removeContactFromCL(aimId);
                GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::spam_chat_avatar);
            }
        }
        else if (command == ql1s("block"))
        {
            blockUser(aimId, true);
        }
        else if (command == ql1s("make_admin"))
        {
            changeRole(aimId, true);
        }
        else if (command == ql1s("make_readonly"))
        {
            readonly(aimId, true);
        }
        else if (command == ql1s("revoke_readonly"))
        {
            readonly(aimId, false);
        }
        else if (command == ql1s("revoke_admin"))
        {
            changeRole(aimId, false);
        }
    }

    void HistoryControlPage::blockUser(const QString& _aimId, bool _blockUser)
    {
        auto cont = chatMembersModel_->getMemberItem(_aimId);
        if (!cont)
            return;

        const auto confirmed = Utils::GetConfirmationWithTwoButtons(
            QT_TRANSLATE_NOOP("popup_window", "CANCEL"),
            QT_TRANSLATE_NOOP("popup_window", "YES"),
            _blockUser ? QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to block user in this chat?") : QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to unblock user?"),
            cont->getFriendly(),
            nullptr);

        if (confirmed)
        {
            Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
            collection.set_value_as_qstring("aimid", aimId_);
            collection.set_value_as_qstring("contact", _aimId);
            collection.set_value_as_bool("block", _blockUser);
            Ui::GetDispatcher()->post_message_to_core(qsl("chats/block"), collection.get());
        }
    }

    void HistoryControlPage::changeRole(const QString& _aimId, bool _moder)
    {
        auto cont = chatMembersModel_->getMemberItem(_aimId);
        if (!cont)
            return;

        const auto confirmed = Utils::GetConfirmationWithTwoButtons(
            QT_TRANSLATE_NOOP("popup_window", "CANCEL"),
            QT_TRANSLATE_NOOP("popup_window", "YES"),
            _moder ? QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to make user admin in this chat?") : QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to revoke admin role?"),
            cont->getFriendly(),
            nullptr);

        if (confirmed)
        {
            Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
            collection.set_value_as_qstring("aimid", aimId_);
            collection.set_value_as_qstring("contact", _aimId);
            collection.set_value_as_qstring("role", _moder ? qsl("moder") : qsl("member"));
            Ui::GetDispatcher()->post_message_to_core(qsl("chats/role/set"), collection.get());
        }
    }

    void HistoryControlPage::readonly(const QString& _aimId, bool _readonly)
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
            collection.set_value_as_qstring("aimid", aimId_);
            collection.set_value_as_qstring("contact", _aimId);
            collection.set_value_as_qstring("role", _readonly ? qsl("readonly") : qsl("member"));
            Ui::GetDispatcher()->post_message_to_core(qsl("chats/role/set"), collection.get());
        }
    }


    void HistoryControlPage::mention(const QString & _aimId)
    {
        auto contactDialog = Utils::InterConnector::instance().getContactDialog();
        if (contactDialog)
        {
            const auto senders = Logic::GetMessagesModel()->getSenders(aimId_, false);
            auto inputWidget = contactDialog->getInputWidget();
            if (inputWidget && !senders.empty())
            {
                const auto it = std::find_if(
                    senders.begin(),
                    senders.end(),
                    [&_aimId](const auto& s) { return s.aimId_ == _aimId; });

                if (it != senders.end() && !it->friendlyName_.isEmpty())
                    inputWidget->insertMention(it->aimId_, it->friendlyName_);
            }
            contactDialog->setFocusOnInputWidget();
        }
    }

    void HistoryControlPage::setRecvLastMessage(bool _value)
    {
        messagesArea_->setRecvLastMessage(_value);
    }

    void HistoryControlPage::actionResult(int)
    {
        if (chatMembersModel_)
            chatMembersModel_->loadAllMembers();
    }

    bool HistoryControlPage::contains(const QString& _aimId) const
    {
        return messagesArea_->contains(_aimId);
    }

    bool HistoryControlPage::containsWidgetWithKey(const Logic::MessageKey& _key) const
    {
        return getWidgetByKey(_key) != nullptr;
    }

    void HistoryControlPage::resumeVisibleItems()
    {
        assert(messagesArea_);
        if (messagesArea_)
        {
            messagesArea_->resumeVisibleItems();
        }
    }

    void HistoryControlPage::suspendVisisbleItems()
    {
        assert(messagesArea_);

        if (messagesArea_)
        {
            messagesArea_->suspendVisibleItems();
        }
    }

    void HistoryControlPage::scrollToBottomByButton()
    {
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chat_down_button);
        scrollToBottom();
    }

    void HistoryControlPage::scrollToBottom()
    {
        if (messagesArea_->getIsSearch())
            Logic::GetMessagesModel()->scrollToBottom(aimId_);

        messagesArea_->scrollToBottom();
    }

    void HistoryControlPage::onUpdateHistoryPosition(int32_t position, int32_t offset)
    {
        const auto shift = Utils::scale_value(button_shift);
        if (offset - position > shift)
            startShowButtonDown();
        else
            startHideButtonDown();
    }

    void HistoryControlPage::startShowButtonDown()
    {
        if (buttonDir_ != -1)
        {
            buttonDir_ = -1;
            buttonDown_->show();

            buttonDownCurrentTime_ = QDateTime::currentMSecsSinceEpoch();

            buttonDownTimer_->stop();
            buttonDownTimer_->start();
        }
    }

    void HistoryControlPage::startHideButtonDown()
    {
        if (buttonDir_ != 1)
        {
            buttonDir_ = 1;
            buttonDownCurrentTime_ = QDateTime::currentMSecsSinceEpoch();

            buttonDownTimer_->stop();
            buttonDownTimer_->start();

        }
    }

    void HistoryControlPage::onButtonDownMove()
    {
        if (buttonDir_ != 0)
        {
            qint64 cur = QDateTime::currentMSecsSinceEpoch();
            qint64 dt = cur - buttonDownCurrentTime_;
            buttonDownCurrentTime_ = cur;

            /// 16 ms = 1... 32ms = 2..
            buttonDownTime_ += (dt / 16.f) * buttonDir_ * 1.f / 15.f;
            if (buttonDownTime_ > 1.f)
                buttonDownTime_ = 1.f;
            else if (buttonDownTime_ < 0.f)
                buttonDownTime_ = 0.f;

            /// interpolation
            float val = buttonDownCurve_.valueForProgress(buttonDownTime_);
            QPoint p = buttonDownShowPosition_ + val * (buttonDownHidePosition_ - buttonDownShowPosition_);


            buttonDown_->move(p);

            if (!(buttonDownTime_ > 0.f && buttonDownTime_ < 1.f))
            {
                if (buttonDir_ == 1)
                {
                    buttonDown_->hide();
                }

                updateFooterButtonsPositions();

                buttonDownTimer_->stop();
            }
        }
    }

    void HistoryControlPage::onButtonDownClicked(bool)
    {
        /// subscribe message model to aimId_
        /// emit subscribe(aimId_);
    }

    void HistoryControlPage::showNewMessageForce()
    {
        bNewMessageForceShow_ = true;
    }

    void HistoryControlPage::showMentionCompleter()
    {
        positionMentionCompleter(rect());

        mentionCompleter_->setDialogAimId(aimId_);
        mentionCompleter_->beforeShow();

        mentionCompleter_->recalcHeight();
        mentionCompleter_->raise();

        QTimer::singleShot(mentionCompleterTimeout, Qt::CoarseTimer, mentionCompleter_, &MentionCompleter::show);
    }

    void HistoryControlPage::setQuoteId(qint64 _quote_id)
    {
        quoteId_ = _quote_id;
    }

    void HistoryControlPage::setPrevChatButtonVisible(const bool _visible)
    {
        prevChatButton_->setVisible(_visible);
        topWidgetLeftPadding_->setVisible(!_visible);
    }

    void postTypingState(const QString& _contact, const core::typing_status _state)
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);

        collection.set_value_as_qstring("contact", _contact);
        collection.set_value_as_int("status", (int32_t)_state);

        Ui::GetDispatcher()->post_message_to_core(qsl("message/typing"), collection.get());
    }

    void HistoryControlPage::onTypingTimer()
    {
        postTypingState(aimId_, core::typing_status::typed);

        prevTypingTime_ = 0;

        typedTimer_->stop();

    }

    void HistoryControlPage::onLookingTimer()
    {
        postTypingState(aimId_, core::typing_status::looking);
    }

    void HistoryControlPage::inputTyped()
    {
        typedTimer_->stop();
        typedTimer_->start();

        const qint64 currentTime = QDateTime::currentMSecsSinceEpoch();

        if ((currentTime - prevTypingTime_) >= 1000)
        {
            postTypingState(aimId_, core::typing_status::typing);

            lookingTimer_->stop();
            lookingTimer_->start();

            prevTypingTime_ = currentTime;
        }
    }

    void HistoryControlPage::pageOpen()
    {
        postTypingState(aimId_, core::typing_status::looking);

        lookingTimer_->stop();
        lookingTimer_->start();
    }

    void HistoryControlPage::pageLeave()
    {
        typedTimer_->stop();
        lookingTimer_->stop();

        postTypingState(aimId_, core::typing_status::none);

        mentionCompleter_->hide();
    }

    void HistoryControlPage::notifyApplicationWindowActive(const bool _active)
    {
        if (!Utils::InterConnector::instance().getMainWindow()->isMainPage())
            return;

        lookingTimer_->stop();

        if (_active)
        {
            postTypingState(aimId_, core::typing_status::looking);

            lookingTimer_->start();
        }
        else
        {
            typedTimer_->stop();

            postTypingState(aimId_, core::typing_status::none);
        }
    }

}

namespace
{
    bool isRemovableWidget(QWidget *_w)
    {
        assert(_w);

        const auto messageItem = qobject_cast<Ui::MessageItem*>(_w);
        if (!messageItem)
        {
            return true;
        }

        return messageItem->isRemovable();
    }

    bool isUpdateableWidget(QWidget* _w)
    {
        assert(_w);

        const auto messageItem = qobject_cast<Ui::MessageItem*>(_w);
        if (messageItem)
        {
            return messageItem->isUpdateable();
        }

        const auto complexItem = qobject_cast<Ui::ComplexMessage::ComplexMessageItem*>(_w);
        if (complexItem)
        {
            return complexItem->isUpdateable();
        }

        return true;
    }
}

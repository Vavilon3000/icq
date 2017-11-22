#include "stdafx.h"
#include "TrayIcon.h"

#include "RecentMessagesAlert.h"
#include "../MainWindow.h"
#include "../MainPage.h"
#include "../contact_list/ContactListModel.h"
#include "../contact_list/RecentItemDelegate.h"
#include "../contact_list/RecentsModel.h"
#include "../contact_list/UnknownsModel.h"
#include "../sounds/SoundsManager.h"
#include "../../core_dispatcher.h"
#include "../../gui_settings.h"
#include "../../my_info.h"
#include "../../controls/ContextMenu.h"
#include "../../utils/gui_coll_helper.h"
#include "../../utils/InterConnector.h"
#include "../../utils/utils.h"
#include "../../utils/log/log.h"
#include "../history_control/MessagesModel.h"


#ifdef _WIN32
//#   include "toast_notifications/win32/ToastManager.h"
typedef HRESULT (__stdcall *QueryUserNotificationState)(QUERY_USER_NOTIFICATION_STATE *pquns);
typedef BOOL (__stdcall *QuerySystemParametersInfo)(__in UINT uiAction, __in UINT uiParam, __inout_opt PVOID pvParam, __in UINT fWinIni);
#else

#ifdef __APPLE__
#   include "notification_center/macos/NotificationCenterManager.h"
#   include "../../utils/macos/mac_support.h"
#endif

#endif //_WIN32

#ifdef _WIN32
#   include <Shobjidl.h>
extern HICON qt_pixmapToWinHICON(const QPixmap &p);
const QString pinPath = qsl("\\Microsoft\\Internet Explorer\\Quick Launch\\User Pinned\\TaskBar\\ICQ.lnk");
#endif // _WIN32

namespace
{
    int init_mail_timeout = 5000;

#ifdef _WIN32
    HICON createHIconFromQIcon(const QIcon &icon, int xSize, int ySize) {
        if (!icon.isNull()) {
            const QPixmap pm = icon.pixmap(icon.actualSize(QSize(xSize, ySize)));
            if (!pm.isNull()) {
                return qt_pixmapToWinHICON(pm);
            }
        }
        return nullptr;
    }
#endif //_WIN32
}

namespace Ui
{
    TrayIcon::TrayIcon(MainWindow* parent)
        : QObject(parent)
        , systemTrayIcon_(new QSystemTrayIcon(this))
        , emailSystemTrayIcon_(nullptr)
        , Menu_(new ContextMenu(parent))
        , MessageAlert_(new RecentMessagesAlert(new Logic::RecentItemDelegate(this), AlertType::alertTypeMessage))
        , MentionAlert_(new RecentMessagesAlert(new Logic::RecentItemDelegate(this), AlertType::alertTypeMentionMe))
        , MailAlert_(new  RecentMessagesAlert(new Logic::RecentItemDelegate(this), AlertType::alertTypeEmail))
        , MainWindow_(parent)
        , first_start_(true)
        , Base_(build::is_icq() ?
            qsl(":/logo/ico_icq") :
            qsl(":/logo/ico_agent"))
        , Unreads_(build::is_icq() ?
            qsl(":/logo/ico_icq_unread") :
            qsl(":/logo/ico_agent_unread"))
#ifdef _WIN32
        , TrayBase_(build::is_icq() ?
            qsl(":/logo/ico_icq_tray") :
            qsl(":/logo/ico_agent_tray"))
        , TrayUnreads_(build::is_icq() ?
            qsl(":/logo/ico_icq_unread_tray") :
            qsl(":/logo/ico_agent_unread_tray"))
#else
        , TrayBase_(build::is_icq() ?
            qsl(":/logo/ico_icq"):
            qsl(":/logo/ico_agent"))
        , TrayUnreads_(build::is_icq() ?
            qsl(":/logo/ico_icq_unread") :
            qsl(":/logo/ico_agent_unread"))
#endif //_WIN32
        , MailCount_(0)
#ifdef _WIN32
        , ptbl(nullptr)
        , overlayIcon_(nullptr)
        , UnreadsCount_(0)
#endif //_WIN32
    {
#ifdef _WIN32
        if (QSysInfo().windowsVersion() >= QSysInfo::WV_WINDOWS7)
        {
            HRESULT hr = CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&ptbl));
            if (FAILED(hr))
                ptbl = nullptr;
        }
#endif //_WIN32
        init();

        InitMailStatusTimer_ = new QTimer(this);
        InitMailStatusTimer_->setInterval(init_mail_timeout);
        InitMailStatusTimer_->setSingleShot(true);

        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::im_created, this, &TrayIcon::loggedIn, Qt::QueuedConnection);

        connect(MessageAlert_, &RecentMessagesAlert::messageClicked, this, &TrayIcon::messageClicked, Qt::QueuedConnection);
        connect(MailAlert_, &RecentMessagesAlert::messageClicked, this, &TrayIcon::messageClicked, Qt::QueuedConnection);
        connect(MentionAlert_, &RecentMessagesAlert::messageClicked, this, &TrayIcon::messageClicked, Qt::QueuedConnection);

        connect(MessageAlert_, &RecentMessagesAlert::changed, this, &TrayIcon::updateAlertsPosition, Qt::QueuedConnection);
        connect(MailAlert_, &RecentMessagesAlert::changed, this, &TrayIcon::updateAlertsPosition, Qt::QueuedConnection);
        connect(MentionAlert_, &RecentMessagesAlert::changed, this, &TrayIcon::updateAlertsPosition, Qt::QueuedConnection);

        connect(Logic::getContactListModel(), &Logic::ContactListModel::contactChanged, this, [this](const QString&)
        {
            updateIcon();
        }, Qt::QueuedConnection);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::myInfo, this, &TrayIcon::myInfo);
        connect(Ui::GetDispatcher(), &core_dispatcher::needLogin, this, &TrayIcon::loggedOut, Qt::QueuedConnection);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::historyControlPageFocusIn, this, &TrayIcon::clearNotifications, Qt::QueuedConnection);
        connect(Logic::getContactListModel(), &Logic::ContactListModel::selectedContactChanged, this, &TrayIcon::clearNotifications, Qt::QueuedConnection);
        connect(Logic::getContactListModel(), &Logic::ContactListModel::contact_removed, this, &TrayIcon::clearNotifications, Qt::QueuedConnection);
        connect(Logic::getRecentsModel(), &Logic::RecentsModel::readStateChanged, this, &TrayIcon::clearNotifications, Qt::QueuedConnection);
        connect(Logic::getUnknownsModel(), &Logic::UnknownsModel::readStateChanged, this, &TrayIcon::clearNotifications, Qt::QueuedConnection);
        connect(Ui::GetDispatcher(), &core_dispatcher::activeDialogHide, this, &TrayIcon::clearNotifications, Qt::QueuedConnection);
        connect(Ui::GetDispatcher(), &core_dispatcher::mailStatus, this, &TrayIcon::mailStatus, Qt::QueuedConnection);

#ifdef __APPLE__
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::mailBoxOpened, this, [this]()
        {
            clearNotifications(qsl("mail"));
        });
#endif

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::mailBoxOpened, this, &TrayIcon::mailBoxOpened, Qt::QueuedConnection);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::logout, this, &TrayIcon::logout, Qt::QueuedConnection);
#ifdef __APPLE__
        setVisible(get_gui_settings()->get_value(settings_show_in_menubar, false));
#endif //__APPLE__

    }

    TrayIcon::~TrayIcon()
    {
        cleanupOverlayIcon();
        disconnect(get_gui_settings());
    }

    void TrayIcon::cleanupOverlayIcon()
    {
#ifdef _WIN32
        if (overlayIcon_)
        {
            DestroyIcon(overlayIcon_);
            overlayIcon_ = nullptr;
        }
#endif
    }

    void TrayIcon::openMailBox(const QString& _mailId)
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);

        collection.set_value_as_qstring("email", Email_);

        Ui::GetDispatcher()->post_message_to_core(qsl("mrim/get_key"), collection.get(), this, [this, _mailId](core::icollection* _collection)
        {
            Utils::openMailBox(Email_, QString::fromUtf8(Ui::gui_coll_helper(_collection, false).get_value_as_string("key")), _mailId);
        });
    }

    void TrayIcon::Hide()
    {
        MessageAlert_->hide();
        MessageAlert_->markShowed();
        forceUpdateIcon();
    }

    void TrayIcon::forceUpdateIcon()
    {
        updateIcon(true);
    }

    void TrayIcon::setVisible(bool visible)
    {
        systemTrayIcon_->setVisible(visible);
        if (emailSystemTrayIcon_)
            emailSystemTrayIcon_->setVisible(visible);
    }

    void TrayIcon::updateEmailIcon()
    {
        if (MailCount_ && canShowNotifications(true))
            showEmailIcon();
        else
            hideEmailIcon();
    }

    void TrayIcon::setMacIcon()
    {
#ifdef __APPLE__
        QString state = MyInfo()->state().toLower();

        if (state != ql1s("offline"))
            state = qsl("online");

        bool unreads = (Logic::getRecentsModel()->totalUnreads() + Logic::getUnknownsModel()->totalUnreads()) != 0;
        QString iconResource(qsl(":/menubar/%1_%2_%3%4_100").
            arg(build::is_icq() ? qsl("icq") : qsl("agent"),
                state,
                MacSupport::currentTheme(),
                unreads ? qsl("_unread") : QString())
        );
        emailIcon_ = QIcon(Utils::parse_image_name(qsl(":/menubar/mail_%1_100"))
                           .arg(MacSupport::currentTheme()));

        if (emailSystemTrayIcon_)
            emailSystemTrayIcon_->setIcon(emailIcon_);

        QIcon icon(Utils::parse_image_name(iconResource));
        systemTrayIcon_->setIcon(icon);
#endif
    }

    void TrayIcon::myInfo()
    {
        setMacIcon();
    }

    void TrayIcon::updateAlertsPosition()
    {
        TrayPosition pos = getTrayPosition();
        QRect availableGeometry = QDesktopWidget().availableGeometry();

        int screenMarginX = Utils::scale_value(20) - Ui::get_gui_settings()->get_shadow_width();
        int screenMarginY = screenMarginX;
        int screenMarginYMention = screenMarginY;
        int screenMarginYMail = screenMarginY;

        if (MessageAlert_->isVisible())
        {
            screenMarginYMention += (MessageAlert_->height() - Utils::scale_value(12));
            screenMarginYMail = screenMarginYMention;
        }

        if (MentionAlert_->isVisible())
        {
            screenMarginYMail += (MentionAlert_->height() - Utils::scale_value(12));
        }


        switch (pos)
        {
        case Ui::TOP_RIGHT:
            MessageAlert_->move(availableGeometry.topRight().x() - MessageAlert_->width() - screenMarginX, availableGeometry.topRight().y() + screenMarginY);
            MentionAlert_->move(availableGeometry.topRight().x() - MentionAlert_->width() - screenMarginX, availableGeometry.topRight().y() + screenMarginYMention);
            MailAlert_->move(availableGeometry.topRight().x() - MailAlert_->width() - screenMarginX, availableGeometry.topRight().y() + screenMarginYMail);
            break;

        case Ui::BOTTOM_LEFT:
            MessageAlert_->move(availableGeometry.bottomLeft().x() + screenMarginX, availableGeometry.bottomLeft().y() - MessageAlert_->height() - screenMarginY);
            MentionAlert_->move(availableGeometry.bottomLeft().x() + screenMarginX, availableGeometry.bottomLeft().y() - MentionAlert_->height() - screenMarginYMention);
            MailAlert_->move(availableGeometry.bottomLeft().x() + screenMarginX, availableGeometry.bottomLeft().y() - MailAlert_->height() - screenMarginYMail);
            break;

        case Ui::BOTOOM_RIGHT:
            MessageAlert_->move(availableGeometry.bottomRight().x() - MessageAlert_->width() - screenMarginX, availableGeometry.bottomRight().y() - MessageAlert_->height() - screenMarginY);
            MentionAlert_->move(availableGeometry.bottomRight().x() - MentionAlert_->width() - screenMarginX, availableGeometry.bottomRight().y() - MentionAlert_->height() - screenMarginYMention);
            MailAlert_->move(availableGeometry.bottomRight().x() - MailAlert_->width() - screenMarginX, availableGeometry.bottomRight().y() - MailAlert_->height() - screenMarginYMail);
            break;

        case Ui::TOP_LEFT:
        default:
            MessageAlert_->move(availableGeometry.topLeft().x() + screenMarginX, availableGeometry.topLeft().y() + screenMarginY);
            MentionAlert_->move(availableGeometry.topLeft().x() + screenMarginX, availableGeometry.topLeft().y() + screenMarginYMention);
            MailAlert_->move(availableGeometry.topLeft().x() + screenMarginX, availableGeometry.topLeft().y() + screenMarginYMail);
            break;
        }
    }

    QIcon TrayIcon::createIcon(const bool _withBase)
    {
        auto createPixmap = [this](const int _size, const int _count, const bool _withBase)
        {
            QImage baseImage = _withBase ? TrayBase_.pixmap(QSize(_size, _size)).toImage() : QImage();
            return QPixmap::fromImage(Utils::iconWithCounter(_size, _count, QColor(ql1s("#f23c34")), Qt::white, std::move(baseImage)));
        };

        QIcon iconOverlay;
        if (UnreadsCount_ > 0)
        {
            iconOverlay.addPixmap(createPixmap(16, UnreadsCount_, _withBase));
            iconOverlay.addPixmap(createPixmap(32, UnreadsCount_, _withBase));
        }
        return iconOverlay;
    }

    void TrayIcon::updateIcon(const bool _force, const bool _updateOverlay)
    {
        auto count = Logic::getRecentsModel()->totalUnreads() + Logic::getUnknownsModel()->totalUnreads();
        if (count == UnreadsCount_ && !_force)
            return;

        UnreadsCount_ = count;

#ifdef _WIN32
        if (_updateOverlay)
        {
            cleanupOverlayIcon();
            if (ptbl)
            {
                overlayIcon_ = UnreadsCount_ > 0 ? createHIconFromQIcon(createIcon(), GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON)) : nullptr;
                ptbl->SetOverlayIcon((HWND)MainWindow_->winId(), overlayIcon_, L"");
            }
            else
            {
                MainWindow_->setWindowIcon(UnreadsCount_ > 0 ? Unreads_ : Base_);
            }
        }
#endif //_WIN32

        if (!platform::is_apple())
        {
            systemTrayIcon_->setIcon(UnreadsCount_ > 0 ? createIcon(true): TrayBase_);
        }
        else
        {
            setMacIcon();
        }
    }

    void TrayIcon::clearNotifications(const QString& aimId)
    {
        if (aimId.isEmpty() || !Notifications_.contains(aimId))
            return;

        Notifications_.removeAll(aimId);
#if defined (_WIN32)
//        if (toastSupported())
//          ToastManager_->HideNotifications(aimId);
#elif defined (__APPLE__)
        if (ncSupported())
        {
            NotificationCenterManager_->HideNotifications(aimId);
        }
#endif //_WIN32
    }

    void TrayIcon::dlgStates(const QVector<Data::DlgState>& _states)
    {
        for (const auto& _state : _states)
        {
            bool canNotify = _state.Visible_ && (!ShowedMessages_.contains(_state.AimId_) || (_state.LastMsgId_ != -1 && ShowedMessages_[_state.AimId_] < _state.LastMsgId_));
            if (_state.GetText().isEmpty())
                canNotify = false;

            if (Logic::getUnknownsModel()->contactIndex(_state.AimId_).isValid())
                canNotify = false;

            if (!_state.Outgoing_)
            {
                if (_state.UnreadCount_ != 0 && canNotify && !Logic::getContactListModel()->isMuted(_state.AimId_) && !_state.hasMentionMe_)
                {
                    ShowedMessages_[_state.AimId_] = _state.LastMsgId_;
                    if (canShowNotifications(_state.AimId_ == ql1s("mail")))
                        showMessage(_state, AlertType::alertTypeMessage);
#ifdef __APPLE__
                    if (!MainWindow_->isActive() || MacSupport::previewIsShown())
#else
                    if (!MainWindow_->isActive())
#endif //__APPLE__
                        GetSoundsManager()->playIncomingMessage();
                }
            }

            if (_state.Visible_)
                markShowed(_state.AimId_);

        }

        updateIcon();

#ifdef __APPLE__
        NotificationCenterManager::updateBadgeIcon(Logic::getRecentsModel()->totalUnreads() + Logic::getUnknownsModel()->totalUnreads());
#endif

    }

    void TrayIcon::mentionMe(const QString& _contact, Data::MessageBuddySptr _mention)
    {
        if (canShowNotifications(false))
        {
            Data::DlgState state;
            state.header_ = QChar(0xD83D) % QChar(0xDC4B) % ql1c(' ') % QT_TRANSLATE_NOOP("notifications_alert", "You have been mentioned");
            state.AimId_ = _contact;
            state.SearchedMsgId_ = _mention->Id_;

            auto text = Logic::GetMessagesModel()->formatRecentsText(*_mention);

            state.SetText(text);
            showMessage(state, AlertType::alertTypeMentionMe);

            GetSoundsManager()->playIncomingMessage();
        }
    }

    void TrayIcon::newMail(const QString& email, const QString& from, const QString& subj, const QString& id)
    {
        Email_ = email;

        if (canShowNotifications(true))
        {
            Data::DlgState state;
            state.AimId_ = qsl("mail");
            state.Friendly_ = from;
            state.header_ = Email_;
            state.MailId_ = id;
            state.SetText(subj);
            showMessage(state, AlertType::alertTypeEmail);
            GetSoundsManager()->playIncomingMail();
        }

        showEmailIcon();
    }

    void TrayIcon::mailStatus(const QString& email, unsigned count, bool init)
    {
        Email_ = email;
        MailCount_ = count;

        if (first_start_)
        {
            bool visible = !!count;
            if (visible)
            {
                showEmailIcon();
            }
            else
            {
                hideEmailIcon();
            }

            first_start_ = false;
        }
        else if (!count)
        {
            hideEmailIcon();
        }

        if (canShowNotifications(true))
        {
            if (!init && !InitMailStatusTimer_->isActive() && !MailAlert_->isVisible())
                return;

            if (count == 0 && init)
            {
                InitMailStatusTimer_->start();
                return;
            }

            Data::DlgState state;
            state.AimId_ = qsl("mail");
            state.header_ = Email_;
            state.Friendly_ = QT_TRANSLATE_NOOP("tray_menu", "New email");
            state.SetText(QString::number(count) + Utils::GetTranslator()->getNumberString(count,
                QT_TRANSLATE_NOOP3("tray_menu", " new email", "1"),
                QT_TRANSLATE_NOOP3("tray_menu", " new emails", "2"),
                QT_TRANSLATE_NOOP3("tray_menu", " new emails", "5"),
                QT_TRANSLATE_NOOP3("tray_menu", " new emails", "21")
                ));

            MailAlert_->updateMailStatusAlert(state);
            if (count == 0)
            {
                MailAlert_->hide();
                return;
            }
            else if (MailAlert_->isVisible())
            {
                return;
            }

            showMessage(state, AlertType::alertTypeEmail);
        }
    }

    void TrayIcon::messageClicked(const QString& _aimId, const QString& mailId, const qint64 mentionId, const AlertType _alertType)
    {
        if (_aimId.isEmpty())
        {
            assert(false);
            return;
        }

        bool notificationCenterSupported = false;

#if defined (_WIN32)
        if (!toastSupported())
#elif defined (__APPLE__)
        notificationCenterSupported = ncSupported();
#endif //_WIN32
        {
            switch (_alertType)
            {
                case AlertType::alertTypeEmail:
                {
                    if (!notificationCenterSupported)
                    {
                        MailAlert_->hide();
                        MailAlert_->markShowed();
                    }

                    GetDispatcher()->post_stats_to_core(mailId.isEmpty() ?
                        core::stats::stats_event_names::alert_mail_common :
                        core::stats::stats_event_names::alert_mail_letter);

                    openMailBox(mailId);

                    return;
                }
                case AlertType::alertTypeMessage:
                {
                    if (!notificationCenterSupported)
                    {
                        MessageAlert_->hide();
                        MessageAlert_->markShowed();
                    }

                    Utils::InterConnector::instance().getMainWindow()->skipRead();

                    Logic::getContactListModel()->setCurrent(_aimId, -1, true);

                    break;
                }
                case AlertType::alertTypeMentionMe:
                {
                    if (!notificationCenterSupported)
                    {
                        MentionAlert_->hide();
                        MentionAlert_->markShowed();
                    }

                    emit Logic::getContactListModel()->select(_aimId, mentionId, mentionId);

                    break;
                }
                default:
                {
                    assert(false);
                    return;
                }
            }

            MainWindow_->activateFromEventLoop();
            MainWindow_->hideMenu();
            emit Utils::InterConnector::instance().closeAnyPopupMenu();
            emit Utils::InterConnector::instance().closeAnyPopupWindow();
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::alert_click);
        }
    }

    QString alertType2String(const AlertType _alertType)
    {
        switch (_alertType)
        {
            case AlertType::alertTypeMessage:
                return qsl("message");
            case AlertType::alertTypeEmail:
                return qsl("mail");
            case AlertType::alertTypeMentionMe:
                return qsl("mention");
            default:
                assert(false);
                return qsl("unknown");
        }
    }

    void TrayIcon::showMessage(const Data::DlgState& state, const AlertType _alertType)
    {
        Notifications_ << state.AimId_;

        const auto isMail = (_alertType == AlertType::alertTypeEmail);
#if defined (_WIN32)
//        if (toastSupported())
//        {
//          ToastManager_->DisplayToastMessage(state.AimId_, state.GetText());
//          return;
//        }
#elif defined (__APPLE__)
        if (ncSupported())
        {
            auto displayName = isMail ? state.Friendly_ : Logic::getContactListModel()->getDisplayName(state.AimId_);

            if (_alertType == AlertType::alertTypeMentionMe)
            {
                displayName = QChar(0xD83D) % QChar(0xDC4B) % ql1c(' ') % QT_TRANSLATE_NOOP("notifications_alert", "You have been mentioned in ") % displayName;
            }

            NotificationCenterManager_->DisplayNotification(
                alertType2String(_alertType),
                state.AimId_,
                state.senderNick_,
                state.GetText(),
                state.MailId_,
                displayName,
                QString::number(state.SearchedMsgId_));
            return;
        }
#endif //_WIN32

        RecentMessagesAlert* alert = nullptr;
        switch (_alertType)
        {
        case AlertType::alertTypeMessage:
            alert = MessageAlert_;
            break;
        case AlertType::alertTypeEmail:
            alert = MailAlert_;
            break;
        case AlertType::alertTypeMentionMe:
            alert = MentionAlert_;
            break;
        default:
            assert(false);
            return;
        }

        alert->addAlert(state);

        TrayPosition pos = getTrayPosition();
        QRect availableGeometry = QDesktopWidget().availableGeometry();

        int screenMarginX = Utils::scale_value(20) - Ui::get_gui_settings()->get_shadow_width();
        int screenMarginY = Utils::scale_value(20) - Ui::get_gui_settings()->get_shadow_width();
        if (isMail && MessageAlert_->isVisible())
            screenMarginY += (MessageAlert_->height() - Utils::scale_value(12));

        switch (pos)
        {
        case Ui::TOP_RIGHT:
            alert->move(availableGeometry.topRight().x() - alert->width() - screenMarginX, availableGeometry.topRight().y() + screenMarginY);
            break;

        case Ui::BOTTOM_LEFT:
            alert->move(availableGeometry.bottomLeft().x() + screenMarginX, availableGeometry.bottomLeft().y() - alert->height() - screenMarginY);
            break;

        case Ui::BOTOOM_RIGHT:
            alert->move(availableGeometry.bottomRight().x() - alert->width() - screenMarginX, availableGeometry.bottomRight().y() - alert->height() - screenMarginY);
            break;

        case Ui::TOP_LEFT:
        default:
            alert->move(availableGeometry.topLeft().x() + screenMarginX, availableGeometry.topLeft().y() + screenMarginY);
            break;
        }

        alert->show();
        if (Menu_->isVisible())
            Menu_->raise();
    }

    TrayPosition TrayIcon::getTrayPosition() const
    {
        QRect availableGeometry = QDesktopWidget().availableGeometry();
        QRect iconGeometry = systemTrayIcon_->geometry();

        QString ag = qsl("availableGeometry x: %1, y: %2, w: %3, h: %4 ").arg(availableGeometry.x()).arg(availableGeometry.y()).arg(availableGeometry.width()).arg(availableGeometry.height());
        QString ig = qsl("iconGeometry x: %1, y: %2, w: %3, h: %4").arg(iconGeometry.x()).arg(iconGeometry.y()).arg(iconGeometry.width()).arg(iconGeometry.height());
        Log::trace(qsl("tray"), ag + ig);

#ifdef __linux__
        if (iconGeometry.isEmpty())
            return TOP_RIGHT;
#endif //__linux__

        bool top = abs(iconGeometry.y() - availableGeometry.topLeft().y()) < abs(iconGeometry.y() - availableGeometry.bottomLeft().y());
        if (abs(iconGeometry.x() - availableGeometry.topLeft().x()) < abs(iconGeometry.x() - availableGeometry.topRight().x()))
            return top ? TOP_LEFT : BOTTOM_LEFT;
        else
            return top ? TOP_RIGHT : BOTOOM_RIGHT;
    }

    void TrayIcon::initEMailIcon()
    {
#ifdef __APPLE__
        emailIcon_ = QIcon(Utils::parse_image_name(qsl(":/menubar/mail_%1_100"))
            .arg(MacSupport::currentTheme()));
#else
        emailIcon_ = QIcon(qsl(":/resources/main_window/tray_email.ico"));
#endif //__APPLE__
    }

    void TrayIcon::showEmailIcon()
    {
        if (platform::is_linux() || emailSystemTrayIcon_ || !canShowNotifications(true))
            return;

        emailSystemTrayIcon_ = new QSystemTrayIcon(this);
        emailSystemTrayIcon_->setIcon(emailIcon_);
        emailSystemTrayIcon_->setVisible(true);
        connect(emailSystemTrayIcon_, &QSystemTrayIcon::activated, this, &TrayIcon::onEmailIconClick, Qt::QueuedConnection);
    }

    void TrayIcon::hideEmailIcon()
    {
        if (platform::is_linux() || !emailSystemTrayIcon_)
            return;

        emailSystemTrayIcon_->setVisible(false);
        delete emailSystemTrayIcon_;
        emailSystemTrayIcon_ = nullptr;
    }

    void TrayIcon::init()
    {
        MessageAlert_->hide();
        MailAlert_->hide();

        MainWindow_->setWindowIcon(Base_);
        updateIcon(true, false);
        systemTrayIcon_->setToolTip(build::is_icq() ? qsl("ICQ") : qsl("Mail.Ru Agent"));

        initEMailIcon();

#if defined __linux__
        Menu_->addActionWithIcon(QIcon(), QT_TRANSLATE_NOOP("tray_menu","Open"), parent(), SLOT(activateFromEventLoop()));
        Menu_->addActionWithIcon(QIcon(), QT_TRANSLATE_NOOP("tray_menu","Quit"), parent(), SLOT(exit()));
#elif defined _WIN32
        Menu_->addActionWithIcon(QIcon(Utils::parse_image_name(qsl(":/context_menu/quit_100"))), QT_TRANSLATE_NOOP("tray_menu", "Quit"), parent(), SLOT(exit()));
#endif //__linux__

        systemTrayIcon_->setContextMenu(Menu_);
        connect(systemTrayIcon_, &QSystemTrayIcon::activated, this, &TrayIcon::activated, Qt::QueuedConnection);
        systemTrayIcon_->show();
    }

    void TrayIcon::markShowed(const QString& aimId)
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("aimid", aimId);
        Ui::GetDispatcher()->post_message_to_core(qsl("dlg_state/hide"), collection.get());
    }

    bool TrayIcon::canShowNotificationsWin() const
    {
#ifdef _WIN32
        if (QSysInfo().windowsVersion() >= QSysInfo::WV_VISTA)
        {
            static QueryUserNotificationState query;
            if (!query)
            {
                HINSTANCE shell32 = LoadLibraryW(L"shell32.dll");
                if (shell32)
                {
                    query = (QueryUserNotificationState)GetProcAddress(shell32, "SHQueryUserNotificationState");
                }
            }

            if (query)
            {
                QUERY_USER_NOTIFICATION_STATE state;
                if (query(&state) == S_OK && state != QUNS_ACCEPTS_NOTIFICATIONS)
                    return false;
            }
        }
#endif //_WIN32
        return true;
    }

    void TrayIcon::mailBoxOpened()
    {
        hideEmailIcon();
    }

    void TrayIcon::logout()
    {
        first_start_ = true;

        hideEmailIcon();
    }

    void TrayIcon::onEmailIconClick(QSystemTrayIcon::ActivationReason)
    {
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::tray_mail);

        openMailBox(QString());
    }

    bool TrayIcon::canShowNotifications(bool isMail) const
    {
        // TODO: must be based on the type of notification - is it message, birthday-notify or contact-coming-notify.
        if (!get_gui_settings()->get_value<bool>(isMail ? settings_notify_new_mail_messages : settings_notify_new_messages, true))  return false;

#ifdef _WIN32
        if (QSysInfo().windowsVersion() == QSysInfo::WV_XP)
        {
            static QuerySystemParametersInfo query;
            if (!query)
            {
                HINSTANCE user32 = LoadLibraryW(L"user32.dll");
                if (user32)
                {
                    query = (QuerySystemParametersInfo)GetProcAddress(user32, "SystemParametersInfoW");
                }
            }

            if (query)
            {
                BOOL result = FALSE;
                if (query(SPI_GETSCREENSAVERRUNNING, 0, &result, 0) && result)
                    return false;
            }
        }
        else
        {
            if (!canShowNotificationsWin())
                return false;
        }

        if (isMail)
            return systemTrayIcon_->isSystemTrayAvailable() && systemTrayIcon_->supportsMessages();

#endif //_WIN32
#ifdef __APPLE__
        if (isMail)
            return get_gui_settings()->get_value<bool>(settings_notify_new_mail_messages, true) && get_gui_settings()->get_value<bool>(settings_show_in_menubar, false);

        return systemTrayIcon_->isSystemTrayAvailable() && systemTrayIcon_->supportsMessages() && (!MainWindow_->isActive() || MacSupport::previewIsShown());
#else
        static bool trayAvailable = systemTrayIcon_->isSystemTrayAvailable();
        static bool msgsSupport = systemTrayIcon_->supportsMessages();
        return trayAvailable && msgsSupport && !MainWindow_->isActive();
#endif
    }

    void TrayIcon::activated(QSystemTrayIcon::ActivationReason reason)
    {
        if (platform::is_windows() || platform::is_apple())
        {
            if (reason == QSystemTrayIcon::Trigger)
                MainWindow_->activate();
        }
        else //linux
        {
            if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::MiddleClick)
                MainWindow_->activate();
        }
    }

    void TrayIcon::loggedIn()
    {
        auto updIcon = [this]() { updateIcon(); };
        connect(Logic::getRecentsModel(), &Logic::RecentsModel::dlgStatesHandled, this, &TrayIcon::dlgStates, Qt::QueuedConnection);
        connect(Logic::getRecentsModel(), &Logic::RecentsModel::updated, this, updIcon, Qt::QueuedConnection);
        connect(Logic::getUnknownsModel(), &Logic::UnknownsModel::dlgStatesHandled, this, &TrayIcon::dlgStates, Qt::QueuedConnection);
        connect(Logic::getUnknownsModel(), &Logic::UnknownsModel::updated, this, updIcon, Qt::QueuedConnection);
        connect(Ui::GetDispatcher(), &core_dispatcher::newMail, this, &TrayIcon::newMail, Qt::QueuedConnection);
        connect(Ui::GetDispatcher(), &core_dispatcher::mentionMe, this, &TrayIcon::mentionMe, Qt::QueuedConnection);
    }

    void TrayIcon::loggedOut(const bool _is_auth_error)
    {
#ifdef __APPLE__
        NotificationCenterManager::updateBadgeIcon(0);
#endif
    }

#if defined (_WIN32)
    bool TrayIcon::toastSupported()
    {
        return false;
        /*
        if (QSysInfo().windowsVersion() > QSysInfo::WV_WINDOWS8_1)
        {
        if (!ToastManager_.get())
        {
        ToastManager_.reset(new ToastManager());
        connect(ToastManager_.get(), SIGNAL(messageClicked(QString)), this, SLOT(messageClicked(QString)), Qt::QueuedConnection);
        }
        return true;
        }
        return false;
        */
    }
#elif defined (__APPLE__)
    bool TrayIcon::ncSupported()
    {
        if (QSysInfo().macVersion() > QSysInfo::MV_10_7)
        {
            if (!NotificationCenterManager_.get())
            {
                NotificationCenterManager_.reset(new NotificationCenterManager());
                connect(NotificationCenterManager_.get(), &NotificationCenterManager::messageClicked, this, &TrayIcon::messageClicked, Qt::QueuedConnection);
                connect(NotificationCenterManager_.get(), &NotificationCenterManager::osxThemeChanged, this, &TrayIcon::myInfo, Qt::QueuedConnection);
            }
            return true;
        }
        return false;
    }
#endif //_WIN32
}

#include "stdafx.h"
#include "MessageAlertWidget.h"
#include "RecentMessagesAlert.h"
#include "../contact_list/RecentItemDelegate.h"
#include "../contact_list/ContactList.h"
#include "../../utils/InterConnector.h"
#include "../../utils/utils.h"
#include "../../controls/CommonStyle.h"
#include "../../controls/TextEmojiWidget.h"
#include "../../core_dispatcher.h"
#include "../../gui_settings.h"
#include "../../main_window/MainWindow.h"

namespace
{
    const int max_alerts_count = 3;
    const int max_alerts_count_mail = 1;
    const int alert_visible_time = 5000; //5 sec
    const int alert_hide_animation_time = 2000; //2 sec
    const int view_all_widget_height = 40;
    const int bottom_space_height = 8;
    const int header_height = 28;
    const int left_margin = 16;

    const QString NOTIFICATION_STYLE =
        qsl("QWidget#topWidget { background-color: rgba(255, 255, 255, 95%); } ");
    const QString LINE_STYLE =
        qsl("QWidget { background-color: #d7d7d7; } ");
}

namespace Ui
{
    RecentMessagesAlert::RecentMessagesAlert(Logic::RecentItemDelegate* delegate, const AlertType _alertType)
        : QWidget(nullptr, Qt::ToolTip | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint)
        , Delegate_(delegate)
        , Layout_(new QVBoxLayout())
        , AlertsCount_(0)
        , CloseButton_(new QPushButton(this))
        , ViewAllWidget_(new QWidget(this))
        , Timer_(new QTimer(this))
        , Animation_(new QPropertyAnimation(this, QByteArrayLiteral("windowOpacity"), this))
        , Height_(0)
        , CursorIn_(false)
        , ViewAllWidgetVisible_(false)
        , alertType_(_alertType)
    {
        MaxAlertCount_ = (_alertType == AlertType::alertTypeMessage) ? max_alerts_count : max_alerts_count_mail;

        init();
    }

    RecentMessagesAlert::~RecentMessagesAlert()
    {

    }

    void RecentMessagesAlert::enterEvent(QEvent* e)
    {
        CursorIn_ = true;
        setWindowOpacity(1);
        Animation_->stop();
        Timer_->start();
        return QWidget::enterEvent(e);
    }

    void RecentMessagesAlert::leaveEvent(QEvent* e)
    {
        CursorIn_ = false;
        return QWidget::leaveEvent(e);
    }

    void RecentMessagesAlert::mouseReleaseEvent(QMouseEvent* e)
    {
        QPoint p = e->pos();
        if (p.y() >= CloseButton_->y() && p.y() <= CloseButton_->y() + CloseButton_->height())
        {
            QLayoutItem* item = Layout_->itemAt(1);
            if (item->widget())
            {
                if (Ui::MessageAlertWidget* alert = qobject_cast<Ui::MessageAlertWidget*>(item->widget()))
                {
                    messageAlertClicked(alert->id(), alert->mailId(), alert->mentionId());
                }
            }
        }
        else if (e->button() == Qt::RightButton)
        {
            closeAlert();
        }
        return QWidget::mouseReleaseEvent(e);
    }

    void RecentMessagesAlert::showEvent(QShowEvent *e)
    {
        emit changed();
        return QWidget::showEvent(e);
    }

    void RecentMessagesAlert::hideEvent(QHideEvent *e)
    {
        emit changed();
        return QWidget::hideEvent(e);
    }

    void RecentMessagesAlert::init()
    {
        Delegate_->setRegim(::Logic::MembersWidgetRegim::FROM_ALERT);

        setAttribute(Qt::WA_ShowWithoutActivating);
        QVBoxLayout* topLayout = Utils::emptyVLayout();
        QWidget* topWidget = new QWidget(this);
        topWidget->setCursor(Qt::PointingHandCursor);
        setLayout(topLayout);
        topLayout->addWidget(topWidget);

        Utils::ApplyStyle(topWidget, NOTIFICATION_STYLE);
        topWidget->setObjectName(qsl("topWidget"));
        topWidget->setStyle(QApplication::style());
        topWidget->setLayout(Layout_);

        QHBoxLayout* layout = Utils::emptyHLayout();
        Utils::ApplyStyle(CloseButton_, CommonStyle::getCloseButtonStyle());
        emailLabel_ = new TextEmojiWidget(this, Fonts::appFontScaled(14), CommonStyle::getColor(CommonStyle::Color::TEXT_SECONDARY));
        emailLabel_->setFixedWidth(width());
        layout->addSpacerItem(new QSpacerItem(Utils::scale_value(left_margin), 0, QSizePolicy::Fixed));
        layout->addWidget(emailLabel_);
        emailLabel_->hide();
        layout->addSpacerItem(new QSpacerItem(QWIDGETSIZE_MAX, 0, QSizePolicy::Expanding));
        CloseButton_->setStyle(QApplication::style());
        CloseButton_->setFixedHeight(Utils::scale_value(header_height));
        layout->addWidget(CloseButton_);

        Layout_->addLayout(layout);
        Layout_->setSpacing(0);
        Layout_->setMargin(0);
        Height_ += CloseButton_->height();

        Height_ += Ui::get_gui_settings()->get_shadow_width() * 2;

        QVBoxLayout* viewAllLayout = Utils::emptyVLayout();
        viewAllLayout->addWidget(ViewAllWidget_);
        ViewAllWidget_->setFixedSize(Delegate_->sizeHintForAlert().width(), Utils::scale_value(view_all_widget_height));
        ViewAllWidget_->setContentsMargins(QMargins(0, 0, 0, 0));

        QWidget* lineWidget = new QWidget();
        lineWidget->setFixedSize(ViewAllWidget_->contentsRect().width(), Utils::scale_value(1));
        Utils::ApplyStyle(lineWidget, LINE_STYLE);
        lineWidget->setStyle(QApplication::style());

        QVBoxLayout* widgetLayout = Utils::emptyVLayout();
        widgetLayout->setAlignment(Qt::AlignTop);
        widgetLayout->setSpacing(Utils::scale_value(12));
        widgetLayout->addWidget(lineWidget);

        LabelEx* viewAllLabel = new LabelEx(this);
        viewAllLabel->setFont(Fonts::appFontScaled(16));
        viewAllLabel->setColor(CommonStyle::getColor(CommonStyle::Color::GREEN_TEXT));
        viewAllLabel->setText(QT_TRANSLATE_NOOP("notifications_alert", "View all"));
        viewAllLabel->setCursor(QCursor(Qt::PointingHandCursor));
        viewAllLabel->setAlignment(Qt::AlignCenter);
        widgetLayout->addWidget(viewAllLabel);
        ViewAllWidget_->setLayout(widgetLayout);
        Layout_->addWidget(ViewAllWidget_);

        QWidget* space = new QWidget(this);
        space->setFixedSize(Delegate_->sizeHintForAlert().width() + Ui::get_gui_settings()->get_shadow_width() * 2, Utils::scale_value(bottom_space_height));
        space->setAttribute(Qt::WA_TranslucentBackground);
        Layout_->addWidget(space);
        Height_ += Utils::scale_value(bottom_space_height);

        ViewAllWidget_->hide();

        setFixedSize(Delegate_->sizeHintForAlert().width() + Ui::get_gui_settings()->get_shadow_width() * 2, Height_);

        Timer_->setInterval(alert_visible_time);
        Timer_->setSingleShot(true);
        connect(Timer_, &QTimer::timeout, this, &RecentMessagesAlert::startAnimation, Qt::QueuedConnection);
        connect(CloseButton_, &QPushButton::clicked, this, &RecentMessagesAlert::closeAlert, Qt::QueuedConnection);
        connect(CloseButton_, &QPushButton::clicked, this, &RecentMessagesAlert::statsCloseAlert, Qt::QueuedConnection);

        connect(viewAllLabel, &Ui::LabelEx::clicked, this, &RecentMessagesAlert::viewAll, Qt::QueuedConnection);

        Utils::addShadowToWindow(this);

        Animation_->setDuration(alert_hide_animation_time);
        Animation_->setStartValue(1);
        Animation_->setEndValue(0);
        connect(Animation_, &QPropertyAnimation::finished, this, &RecentMessagesAlert::closeAlert, Qt::QueuedConnection);
    }

    bool RecentMessagesAlert::isMailAlert() const
    {
        return (alertType_ == AlertType::alertTypeEmail);
    }

    bool RecentMessagesAlert::isMessageAlert() const
    {
        return (alertType_ == AlertType::alertTypeMessage);
    }

    bool RecentMessagesAlert::isMentionAlert() const
    {
        return (alertType_ == AlertType::alertTypeMentionMe);
    }

    void RecentMessagesAlert::startAnimation()
    {
#ifdef __linux__
        closeAlert();
        return;
#endif //__linux__
        if (CursorIn_)
            Timer_->start();
        else
            Animation_->start();
    }

    void RecentMessagesAlert::messageAlertClicked(const QString& aimId, const QString& mailId, qint64 mentionId)
    {
        closeAlert();
        Utils::InterConnector::instance().getMainWindow()->closePopups();

        emit messageClicked(aimId, mailId, mentionId, alertType_);
    }

    void RecentMessagesAlert::messageAlertClosed(const QString& /*aimId*/, const QString& /*mailId*/, qint64 /*mentionId*/)
    {
        closeAlert();
        Utils::InterConnector::instance().getMainWindow()->closePopups();
    }

    void RecentMessagesAlert::closeAlert()
    {
        hide();
        setWindowOpacity(1);
        Animation_->stop();
        markShowed();
    }

    void RecentMessagesAlert::statsCloseAlert()
    {
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::alert_close);
    }

    void RecentMessagesAlert::viewAll()
    {
        messageAlertClicked(QString(), QString(), -1);
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::alert_viewall);
    }

    void RecentMessagesAlert::addAlert(const Data::DlgState& state)
    {
        if (!isMessageAlert())
        {
            emailLabel_->setText(state.header_);
            emailLabel_->show();
        }

        bool showViewAll = (AlertsCount_ == MaxAlertCount_ && isMessageAlert());

        int i = 0;

        while (QLayoutItem* item = Layout_->itemAt(i))
        {
            auto alert = qobject_cast<Ui::MessageAlertWidget*>(item->widget());
            if (alert && alert->id() == state.AimId_)
            {
                Height_ -= Delegate_->sizeHintForAlert().height();
                Layout_->removeWidget(alert);
                alert->deleteLater();
                showViewAll = ViewAllWidget_->isVisible() && AlertsCount_ == MaxAlertCount_ && isMessageAlert();
                --AlertsCount_;
            }

            ++i;
        }

        if (AlertsCount_ == MaxAlertCount_)
        {
            QLayoutItem* item = Layout_->itemAt(AlertsCount_);
            QWidget* w = item ? item->widget() : nullptr;
            if (w)
            {
                Height_ -= Delegate_->sizeHintForAlert().height();
                Layout_->removeWidget(w);
                w->deleteLater();
                --AlertsCount_;
            }
        }

        MessageAlertWidget* widget = new MessageAlertWidget(state, Delegate_, this);
        connect(widget, &MessageAlertWidget::clicked, this, &RecentMessagesAlert::messageAlertClicked, Qt::DirectConnection);
        connect(widget, &MessageAlertWidget::closed, this, &RecentMessagesAlert::messageAlertClosed, Qt::DirectConnection);
        Height_ +=Delegate_->sizeHintForAlert().height();
        Layout_->insertWidget(1, widget);
        ++AlertsCount_;

        if (showViewAll)
        {
            if (!ViewAllWidgetVisible_)
            {
                ViewAllWidgetVisible_ = true;
                ViewAllWidget_->show();
                Height_ += Utils::scale_value(view_all_widget_height);
            }
        }
        else
        {
            if (ViewAllWidgetVisible_)
            {
                ViewAllWidgetVisible_ = false;
                ViewAllWidget_->hide();
                Height_ -= Utils::scale_value(view_all_widget_height);
            }
        }

        setFixedHeight(Height_);
        setWindowOpacity(1);
        Animation_->stop();
        Timer_->start();

        emit changed();
    }

    void RecentMessagesAlert::markShowed()
    {
        if (!isMessageAlert())
            return;

        int i = 0;
        while (QLayoutItem* item = Layout_->itemAt(i))
        {
            auto alert = qobject_cast<Ui::MessageAlertWidget*>(item->widget());
            if (alert)
            {
                Height_ -= Delegate_->sizeHintForAlert().height();
                Layout_->removeWidget(alert);
                alert->deleteLater();
                --AlertsCount_;
            }
            else // there is no need to increment if item was deleted
            {
                ++i;
            }
        }

        if (ViewAllWidgetVisible_)
        {
            ViewAllWidgetVisible_ = false;
            ViewAllWidget_->hide();
            Height_ -= Utils::scale_value(view_all_widget_height);
        }

        setFixedHeight(Height_);
    }

    bool RecentMessagesAlert::updateMailStatusAlert(const Data::DlgState& state)
    {
        if (!isMessageAlert())
        {
            emailLabel_->setText(state.header_);
            emailLabel_->show();
        }

        bool showViewAll = (AlertsCount_ == MaxAlertCount_ && isMessageAlert());
        int i = 0;

        bool mailStatusFound = false;
        while (QLayoutItem* item = Layout_->itemAt(i))
        {
            auto alert = qobject_cast<Ui::MessageAlertWidget*>(item->widget());
            if (alert && alert->id() == state.AimId_ && alert->mailId().isEmpty())
            {
                Height_ -= Delegate_->sizeHintForAlert().height();
                Layout_->removeWidget(alert);
                alert->deleteLater();
                showViewAll = ViewAllWidget_->isVisible() && AlertsCount_ == MaxAlertCount_ && isMessageAlert();
                --AlertsCount_;
                mailStatusFound = true;
            }
            ++i;
        }

        if (!mailStatusFound)
            return false;

        if (AlertsCount_ == MaxAlertCount_)
        {
            QLayoutItem* item = Layout_->itemAt(AlertsCount_);
            QWidget* w = item ? item->widget() : nullptr;
            if (w)
            {
                Height_ -= Delegate_->sizeHintForAlert().height();
                Layout_->removeWidget(w);
                w->deleteLater();
                --AlertsCount_;
            }
        }

        MessageAlertWidget* widget = new MessageAlertWidget(state, Delegate_, this);
        connect(widget, &MessageAlertWidget::clicked, this, &RecentMessagesAlert::messageAlertClicked, Qt::DirectConnection);
        connect(widget, &MessageAlertWidget::closed, this, &RecentMessagesAlert::messageAlertClosed, Qt::DirectConnection);
        Height_ +=Delegate_->sizeHintForAlert().height();
        Layout_->insertWidget(1, widget);
        ++AlertsCount_;

        setFixedHeight(Height_);

        emit changed();

        return true;
    }
}

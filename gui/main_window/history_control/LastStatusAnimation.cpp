#include "stdafx.h"

#include "LastStatusAnimation.h"
#include "../../controls/CommonStyle.h"
#include "../../utils/utils.h"
#include "../../theme_settings.h"
#include "MessageStyle.h"
#include "HistoryControlPageItem.h"

namespace Ui
{
    std::unique_ptr<QPixmap> pendingIcon;
    std::unique_ptr<QPixmap> pendingIcon_w;
    std::unique_ptr<QPixmap> deliveredToServerIcon;
    std::unique_ptr<QPixmap> deliveredToServerIcon_w;
    std::unique_ptr<QPixmap> deliveredToClientgIcon;
    std::unique_ptr<QPixmap> deliveredToClientgIcon_w;
    std::unique_ptr<QPixmap> pendingRecentsIcon;
    std::unique_ptr<QPixmap> pendingRecentsIcon_w;

    const int pendingTimeout = 500;

    bool initStatusesPixmaps()
    {
        if (pendingIcon)
            return true;

        pendingIcon.reset(new QPixmap(Utils::parse_image_name(qsl(":/delivery/time_100"))));
        pendingIcon_w.reset(new QPixmap(Utils::parse_image_name(qsl(":/delivery/time_w_100"))));
        deliveredToServerIcon.reset(new QPixmap(Utils::parse_image_name(qsl(":/delivery/send_100"))));
        deliveredToServerIcon_w.reset(new QPixmap(Utils::parse_image_name(qsl(":/delivery/send_w_100"))));
        deliveredToClientgIcon.reset(new QPixmap(Utils::parse_image_name(qsl(":/delivery/delivered_100"))));
        deliveredToClientgIcon_w.reset(new QPixmap(Utils::parse_image_name(qsl(":/delivery/delivered_w_100"))));
        pendingRecentsIcon.reset(new QPixmap(Utils::parse_image_name(qsl(":/delivery/time_recents_100"))));
        pendingRecentsIcon_w.reset(new QPixmap(Utils::parse_image_name(qsl(":/delivery/time_recents_w_100"))));

        Utils::check_pixel_ratio(*pendingIcon);
        Utils::check_pixel_ratio(*pendingIcon_w);
        Utils::check_pixel_ratio(*deliveredToServerIcon);
        Utils::check_pixel_ratio(*deliveredToServerIcon_w);
        Utils::check_pixel_ratio(*deliveredToClientgIcon);
        Utils::check_pixel_ratio(*deliveredToClientgIcon_w);
        Utils::check_pixel_ratio(*pendingRecentsIcon);
        Utils::check_pixel_ratio(*pendingRecentsIcon_w);

        return true;
    }

    LastStatusAnimation::LastStatusAnimation(HistoryControlPageItem* _parent)
        : QObject(_parent)
        , widget_(_parent)
        , isActive_(true)
        , isPlay_(false)
        , show_(false)
        , pendingTimer_(nullptr)
        , lastStatus_(LastStatus::None)
    {
        assert(_parent);

        static bool res = initStatusesPixmaps();
    }

    LastStatusAnimation::~LastStatusAnimation()
    {
    }

    QPixmap LastStatusAnimation::getPendingPixmap(const bool _selected)
    {
        if (!pendingIcon)
            initStatusesPixmaps();

        return (_selected ? *pendingRecentsIcon_w : *pendingRecentsIcon);
    }

    void LastStatusAnimation::startPendingTimer()
    {
        if (pendingTimer_)
        {
            assert(false);
            return;
        }

        pendingTimer_ = new QTimer(this);
        connect(pendingTimer_, &QTimer::timeout, this, [this]()
        {
            stopPendingTimer();

            widget_->repaint();
        });

        pendingTimer_->start(pendingTimeout);
    }

    void LastStatusAnimation::stopPendingTimer()
    {
        if (!pendingTimer_)
            return;

        pendingTimer_->stop();

        delete pendingTimer_;

        pendingTimer_ = nullptr;
    }

    bool LastStatusAnimation::isPendingTimerActive() const
    {
        return (!!pendingTimer_);
    }

    void LastStatusAnimation::setLastStatus(LastStatus _lastStatus)
    {
        if (lastStatus_ == _lastStatus)
            return;

        lastStatus_ = _lastStatus;

        if (lastStatus_ != LastStatus::Pending)
            stopPendingTimer();

        switch (lastStatus_)
        {
            case LastStatus::None:
                show_ = false;
                break;
            case LastStatus::Read:
                break;
            case LastStatus::Pending:
            {
                startPendingTimer();
                show_ = true;
                break;
            }
            case LastStatus::DeliveredToServer:
            case LastStatus::DeliveredToPeer:
            {
                show_ = false;
                break;
            }
            default:
                break;
        }

        widget_->repaint();
    }

    void LastStatusAnimation::hideStatus()
    {
        if (show_)
        {
            show_ = false;
            widget_->repaint();
        }
    }

    void LastStatusAnimation::showStatus()
    {
        if (!show_)
        {
            show_ = true;
            widget_->repaint();
        }
    }

    void LastStatusAnimation::drawLastStatus(
        QPainter& _p,
        const int _x,
        const int _y,
        const int _dx,
        const int _dy) const
    {
        if (!show_)
            return;

        QPixmap statusIcon;

        bool light = false;

        auto theme = get_qt_theme_settings()->themeForContact(widget_->getAimid());
        if (theme)
        {
            light = theme->isDeliveryStatusLight();
        }

        switch (lastStatus_)
        {
            case LastStatus::None:
            case LastStatus::Read:
            {
                assert(false);
                break;
            }
            case LastStatus::Pending:
            {
                statusIcon = light ? *pendingIcon_w : *pendingIcon;

                if (isPendingTimerActive())
                    return;

                break;
            }
            case LastStatus::DeliveredToServer:
            {
                statusIcon = light ? *deliveredToServerIcon_w : *deliveredToServerIcon;
                break;
            }
            case LastStatus::DeliveredToPeer:
            {
                statusIcon = light ? *deliveredToClientgIcon_w : *deliveredToClientgIcon;
                break;
            }
            default:
                break;
        }

        _p.drawPixmap(_x, _y, _dx, _dy, statusIcon);
    }

}

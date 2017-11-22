#include "stdafx.h"

#include "HistoryControlPageItem.h"
#include "../../cache/themes/themes.h"
#include "../../theme_settings.h"
#include "../../utils/utils.h"
#include "../../cache/avatars/AvatarStorage.h"
#include "MessageStyle.h"
#include "LastStatusAnimation.h"

namespace Ui
{
	HistoryControlPageItem::HistoryControlPageItem(QWidget *parent)
		: QWidget(parent)
        , Selected_(false)
        , HasTopMargin_(false)
        , HasAvatar_(false)
        , HasAvatarSet_(false)
        , isDeleted_(false)
        , lastStatus_(LastStatus::None)
        , lastStatusAnimation_(nullptr)
        , QuoteAnimation_(parent)
	{
	}

    void HistoryControlPageItem::clearSelection()
    {
        if (Selected_)
        {
            update();
        }

        Selected_ = false;
    }

    bool HistoryControlPageItem::hasAvatar() const
    {
        assert(HasAvatarSet_);
        return HasAvatar_;
    }

    bool HistoryControlPageItem::hasTopMargin() const
    {
        return HasTopMargin_;
    }

    void HistoryControlPageItem::select()
    {
        if (!Selected_)
        {
            update();
        }

        Selected_ = true;
    }

    void HistoryControlPageItem::setTopMargin(const bool value)
    {
        if (HasTopMargin_ == value)
        {
            return;
        }

        HasTopMargin_ = value;

        updateGeometry();
    }

    bool HistoryControlPageItem::isSelected() const
    {
        return Selected_;
    }

    void HistoryControlPageItem::onActivityChanged(const bool /*isActive*/)
    {
    }

    void HistoryControlPageItem::onVisibilityChanged(const bool /*isVisible*/)
    {
    }

    void HistoryControlPageItem::onDistanceToViewportChanged(const QRect& /*_widgetAbsGeometry*/, const QRect& /*_viewportVisibilityAbsRect*/)
    {}

    void HistoryControlPageItem::setHasAvatar(const bool value)
    {
        HasAvatar_ = value;
        HasAvatarSet_ = true;

        updateGeometry();
    }

    void HistoryControlPageItem::setContact(const QString& _aimId)
    {
        aimId_ = _aimId;
    }

    void HistoryControlPageItem::setSender(const QString& /*_sender*/)
    {

    }

    themes::themePtr HistoryControlPageItem::theme() const
    {
        return get_qt_theme_settings()->themeForContact(aimId_);
    }

    void HistoryControlPageItem::setDeliveredToServer(const bool _delivered)
    {

    }

    void HistoryControlPageItem::drawLastReadAvatar(QPainter& _p, const QString& _aimid, const QString& _friendly, int _rightPadding)
    {
        const QRect rc = rect();

        const int avatarSize = MessageStyle::getLastReadAvatarSize();
        const int size = Utils::scale_bitmap(avatarSize);

        bool isDefault = false;
        QPixmap avatar = *Logic::GetAvatarStorage()->GetRounded(_aimid, _friendly, size, QString(), isDefault, false, false);

        if (!avatar.isNull())
        {
            _p.drawPixmap(
                rc.right() - avatarSize - MessageStyle::getLastReadAvatarMargin() - _rightPadding,
                rc.bottom() - avatarSize,
                avatarSize,
                avatarSize,
                avatar);
        }
    }

    void HistoryControlPageItem::drawLastStatusIconImpl(QPainter& _p, int _rightPadding, int _bottomPadding)
    {
        if (lastStatusAnimation_)
        {
            const QRect rc = rect();

            const auto iconSize = MessageStyle::getLastStatusIconSize();

            lastStatusAnimation_->drawLastStatus(
                _p,
                rc.right() - iconSize.width() - _rightPadding,
                rc.bottom() - iconSize.height() - _bottomPadding,
                iconSize.width(),
                iconSize.height());
        }
    }

    void HistoryControlPageItem::drawLastStatusIcon(QPainter& _p,  LastStatus _lastStatus, const QString& _aimid, const QString& _friendly, int _rightPadding)
    {
        switch (lastStatus_)
        {
        case LastStatus::None:
            break;
        case LastStatus::Pending:
        case LastStatus::DeliveredToServer:
        case LastStatus::DeliveredToPeer:
            drawLastStatusIconImpl(_p, _rightPadding + MessageStyle::getLastStatusIconMargin(), 0);
            break;
        case LastStatus::Read:
            drawLastReadAvatar(_p, _aimid, _friendly, _rightPadding);
            break;
        default:
            break;
        }
    }

    qint64 HistoryControlPageItem::getId() const
    {
        return -1;
    }

    void HistoryControlPageItem::setDeleted(const bool _isDeleted)
    {
        isDeleted_ = _isDeleted;
    }

    bool HistoryControlPageItem::isDeleted() const
    {
        return isDeleted_;
    }

    void HistoryControlPageItem::setLastStatus(LastStatus _lastStatus)
    {
        if (lastStatus_ == _lastStatus)
            return;

        lastStatus_ = _lastStatus;
        if (lastStatus_ != LastStatus::None)
        {
            if (!lastStatusAnimation_)
                lastStatusAnimation_ = new LastStatusAnimation(this);
        }
        if (lastStatusAnimation_)
            lastStatusAnimation_->setLastStatus(lastStatus_);
    }

    LastStatus HistoryControlPageItem::getLastStatus() const
    {
        return lastStatus_;
    }

    void HistoryControlPageItem::setAimid(const QString &aimId)
    {
        assert(!aimId.isEmpty());

        if (aimId == aimId_)
        {
            return;
        }

        assert(aimId_.isEmpty());
        aimId_ = aimId;
    }

    void HistoryControlPageItem::showMessageStatus()
    {
        if (lastStatus_ == LastStatus::DeliveredToPeer || lastStatus_ == LastStatus::DeliveredToServer)
        {
            if (lastStatusAnimation_)
                lastStatusAnimation_->showStatus();
        }
    }


    void HistoryControlPageItem::hideMessageStatus()
    {
        if (lastStatus_ == LastStatus::DeliveredToPeer || lastStatus_ == LastStatus::DeliveredToServer)
        {
            if (lastStatusAnimation_)
                lastStatusAnimation_->hideStatus();
        }
    }
}

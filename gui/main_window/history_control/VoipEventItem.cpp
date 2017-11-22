#include "stdafx.h"

#include "../../core_dispatcher.h"

#include "../../cache/avatars/AvatarStorage.h"

#include "../../utils/InterConnector.h"
#include "../../utils/PainterPath.h"
#include "../../utils/utils.h"

#include "../../themes/ThemePixmap.h"

#include "MessageStatusWidget.h"
#include "MessageStyle.h"
#include "VoipEventInfo.h"

#include "VoipEventItem.h"

#include "../contact_list/ContactListModel.h"

#include "../../cache/themes/themes.h"
#include "../../gui_settings.h"
#include "../../theme_settings.h"

namespace Ui
{

    namespace
    {
        int32_t getIconRightPadding();

        int32_t getIconTopPadding();

        int32_t getTextBaselineY();

    }

    VoipEventItem::VoipEventItem(const HistoryControl::VoipEventInfoSptr& eventInfo)
        : MessageItemBase(nullptr)
        , EventInfo_(eventInfo)
        , IsAvatarHovered_(false)
        , IsBubbleHovered_(false)
        , timestampHoverEnabled_(true)
        , TimeWidget_(nullptr)
        , id_(-1)
    {
        assert(EventInfo_);
    }

    VoipEventItem::VoipEventItem(
        QWidget *parent,
        const HistoryControl::VoipEventInfoSptr& eventInfo)
        : MessageItemBase(parent)
        , EventInfo_(eventInfo)
        , IsAvatarHovered_(false)
        , IsBubbleHovered_(false)
        , timestampHoverEnabled_(true)
        , TimeWidget_(new MessageTimeWidget(this))
        , id_(-1)
    {
        assert(EventInfo_);

        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        if (!EventInfo_->isVisible())
        {
            assert(!"invisible voip events are not allowed in history control");
            setFixedHeight(0);
            return;
        }

        setAttribute(Qt::WA_TranslucentBackground);

        if (EventInfo_->isClickable())
        {
            setMouseTracking(true);
        }

        Icon_ = eventInfo->loadIcon(false);
        HoverIcon_ = eventInfo->loadIcon(true);

        TimeWidget_->setContact(EventInfo_->getContactAimid());
        TimeWidget_->setTime(EventInfo_->getTimestamp());
        TimeWidget_->hideAnimated();
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::showMessageHiddenControls, this, &VoipEventItem::showHiddenControls, Qt::QueuedConnection);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::hideMessageHiddenControls, this, &VoipEventItem::hideHiddenControls, Qt::QueuedConnection);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::setTimestampHoverActionsEnabled, this, &VoipEventItem::setTimestampHoverEnabled);
    }

    QString VoipEventItem::formatRecentsText() const
    {
        return EventInfo_->formatEventText();
    }

    void VoipEventItem::updateHeight()
    {
        int height = MessageStyle::getMinBubbleHeight() + MessageStyle::getTopMargin(hasTopMargin());
        setFixedHeight(height);
    }

    void VoipEventItem::setTopMargin(const bool value)
    {
        HistoryControlPageItem::setTopMargin(value);

        if (!EventInfo_->isVisible())
        {
            return;
        }

        updateHeight();
    }

    void VoipEventItem::setHasAvatar(const bool value)
    {
        if (!isOutgoing() && value)
        {
            auto isDefault = false;
            Avatar_ = Logic::GetAvatarStorage()->GetRounded(
                EventInfo_->getContactAimid(),
                EventInfo_->getContactFriendly(),
                Utils::scale_bitmap(MessageStyle::getAvatarSize()),
                QString(),
                Out isDefault,
                false,
                false
            );

            assert(Avatar_);
        }
        else
        {
            Avatar_.reset();
        }

        HistoryControlPageItem::setHasAvatar(value);
    }

    void VoipEventItem::mouseMoveEvent(QMouseEvent *event)
    {
        const auto prevHovered = (IsAvatarHovered_ || IsBubbleHovered_);

        const auto pos = event->pos();
        IsAvatarHovered_ = isAvatarHovered(pos);
        IsBubbleHovered_ = isBubbleHovered(pos);

        const auto isHovered = (IsAvatarHovered_ || IsBubbleHovered_);
        if (isHovered)
        {
            setCursor(Qt::PointingHandCursor);
            if (!prevHovered)
            {
                QTimer::singleShot(MessageTimestamp::hoverTimestampShowDelay,
                    Qt::CoarseTimer,
                    this,
                    [this]()
                    {
                        if (IsAvatarHovered_ || IsBubbleHovered_)
                            showHiddenControls();
                    }
                );
            }
        }
        else
        {
            setCursor(Qt::ArrowCursor);

            if (timestampHoverEnabled_)
                hideHiddenControls();
        }

        update();

        MessageItemBase::mouseMoveEvent(event);
    }

    void VoipEventItem::mouseReleaseEvent(QMouseEvent *event)
    {
        const auto pos = event->pos();

        if (isBubbleHovered(pos) && event->button() == Qt::LeftButton)
        {
            const auto &contactAimid = EventInfo_->getContactAimid();
            assert(!contactAimid.isEmpty());
            Ui::GetDispatcher()->getVoipController().setStartV(contactAimid.toUtf8(), false);

            return;
        }

        if (isAvatarHovered(pos))
        {
            Utils::openDialogOrProfile(EventInfo_->getContactAimid());
        }

        MessageItemBase::mouseReleaseEvent(event);
    }

    void VoipEventItem::leaveEvent(QEvent* _event)
    {
        MessageItemBase::leaveEvent(_event);

        IsAvatarHovered_ = false;
        IsBubbleHovered_ = false;

        if (timestampHoverEnabled_)
            hideHiddenControls();

        update();
    }

    void VoipEventItem::paintEvent(QPaintEvent *)
    {
        if (!BubbleRect_.isValid())
        {
            return;
        }

        QPainter p(this);

        p.setRenderHint(QPainter::Antialiasing);
        p.setRenderHint(QPainter::TextAntialiasing);

        p.setPen(Qt::NoPen);
        p.setBrush(Qt::NoBrush);

        const auto isOutgoing = this->isOutgoing();

        if (Bubble_.isEmpty())
        {
            Bubble_ = Utils::renderMessageBubble(BubbleRect_, MessageStyle::getBorderRadius(), isOutgoing);
            assert(!Bubble_.isEmpty());
        }

        assert(BubbleRect_.width() > 0);

        int theme_id = get_qt_theme_settings()->themeIdForContact(EventInfo_->getContactAimid());//theme()->get_id();

        const auto bodyBrush = Ui::MessageStyle::getBodyBrush(isOutgoing, IsBubbleHovered_, theme_id);

        p.fillPath(Bubble_, bodyBrush);

        const auto baseY = BubbleRect_.top();

        auto cursorX = MessageStyle::getLeftMargin(isOutgoing);

        if (!isOutgoing)
        {
            if (Avatar_)
            {
                p.drawPixmap(
                    getAvatarRect(),
                    *Avatar_
                );
            }

            cursorX += MessageStyle::getAvatarSize();
            cursorX += MessageStyle::getAvatarRightMargin();
        }

        auto &icon = (IsBubbleHovered_ ? HoverIcon_ : Icon_);
        if (icon)
        {
            cursorX += MessageStyle::getBubbleHorPadding();
            if (isOutgoing)
                cursorX += MessageStyle::getTimeMaxWidth();

            icon->Draw(p, cursorX, baseY + getIconTopPadding());
            cursorX += icon->GetWidth();

            cursorX += getIconRightPadding();
        }
        else
        {
            cursorX += MessageStyle::getBubbleHorPadding();
        }

        if (TimeWidgetGeometry_.isValid())
        {
            auto eventText = (
                IsBubbleHovered_ ?
                QT_TRANSLATE_NOOP("chat_event", "Call back") :
                EventInfo_->formatEventText());

            const auto eventTextFont = MessageStyle::getTextFont();

            auto textWidth = isOutgoing ? (TimeWidgetGeometry_.right() + getIconRightPadding() + cursorX) : (TimeWidgetGeometry_.left() - getIconRightPadding() - cursorX);
            textWidth = std::max(0, textWidth);

            QFontMetrics fontMetrics(eventTextFont);
            eventText = fontMetrics.elidedText(eventText, Qt::ElideRight, textWidth);

            p.setPen(getTextColor(IsBubbleHovered_));
            p.setFont(eventTextFont);
            p.drawText(
                cursorX,
                baseY + getTextBaselineY(),
                eventText);
        }
        const auto lastStatus = getLastStatus();
        if (lastStatus != LastStatus::None)
        {
            drawLastStatusIcon(p,
                lastStatus,
                EventInfo_->getContactAimid(),
                EventInfo_->getContactFriendly(),
                isOutgoing ? 0 : MessageStyle::getTimeMaxWidth());
        }
    }

    void VoipEventItem::resizeEvent(QResizeEvent *event)
    {
        QRect newBubbleRect(QPoint(0, 0), event->size());
        const auto isOutgoing = this->isOutgoing();
        QMargins margins(
            MessageStyle::getLeftMargin(isOutgoing),
            MessageStyle::getTopMargin(hasTopMargin()),
            MessageStyle::getRightMargin(isOutgoing),
            0
        );

        if (isOutgoing)
        {
            margins.setLeft(margins.left() + MessageStyle::getTimeMaxWidth());
        }
        else
        {
            margins.setLeft(
                margins.left() + MessageStyle::getAvatarSize() + MessageStyle::getAvatarRightMargin()
            );
            margins.setRight(margins.right() + MessageStyle::getTimeMaxWidth());
        }

        newBubbleRect = newBubbleRect.marginsRemoved(margins);

        if (BubbleRect_ != newBubbleRect)
        {
            BubbleRect_ = (newBubbleRect.isValid() ? newBubbleRect : QRect());
            Bubble_ = QPainterPath();
        }

        const auto timeWidgetSize = TimeWidget_->sizeHint();

        int timeX = 0;
        if (isOutgoing)
            timeX = BubbleRect_.left() - timeWidgetSize.width() - MessageStyle::getTimeMarginX();
        else
            timeX = BubbleRect_.right() + MessageStyle::getTimeMarginX();

        auto timeY = BubbleRect_.bottom();
        timeY -= MessageStyle::getTimeMarginY();
        timeY -= timeWidgetSize.height();

        QRect timeWidgetGeometry(
            timeX,
            timeY,
            timeWidgetSize.width(),
            timeWidgetSize.height()
        );

        TimeWidget_->setGeometry(timeWidgetGeometry);
        TimeWidget_->showIfNeeded();

        TimeWidgetGeometry_ = timeWidgetGeometry;

        HistoryControlPageItem::resizeEvent(event);
    }

    void VoipEventItem::hideEvent(QHideEvent *)
    {
        if (TimeWidget_ && get_gui_settings()->get_value<bool>(settings_hide_message_timestamps, true))
            TimeWidget_->hide();
    }

    QRect VoipEventItem::getAvatarRect() const
    {
        assert(!BubbleRect_.isEmpty());
        assert(hasAvatar());

        QRect result(
            MessageStyle::getLeftMargin(isOutgoing()),
            BubbleRect_.top(),
            MessageStyle::getAvatarSize(),
            MessageStyle::getAvatarSize()
        );

        return result;
    }

    bool VoipEventItem::isAvatarHovered(const QPoint &mousePos) const
    {
        if (!hasAvatar())
        {
            return false;
        }

        return getAvatarRect().contains(mousePos);
    }

    bool VoipEventItem::isBubbleHovered(const QPoint &mousePos) const
    {
        if (!EventInfo_->isClickable())
        {
            return false;
        }

        const auto isHovered = (
            (mousePos.y() > MessageStyle::getTopMargin(hasTopMargin())) &&
            (mousePos.y() < height()) &&
            (mousePos.x() > BubbleRect_.left()) &&
            (mousePos.x() < BubbleRect_.right())
            );

        return isHovered;
    }

    bool VoipEventItem::isOutgoing() const
    {
        return !EventInfo_->isIncomingCall();
    }

    void VoipEventItem::setLastStatus(LastStatus _lastStatus)
    {
        if (getLastStatus() != _lastStatus)
        {
            assert(_lastStatus == LastStatus::None || _lastStatus == LastStatus::Read);
            HistoryControlPageItem::setLastStatus(_lastStatus);
            update();
        }
    }

    void VoipEventItem::setId(const qint64 _id)
    {
        id_ = _id;
    }

    qint64 VoipEventItem::getId() const
    {
        return id_;
    }

    void VoipEventItem::updateStyle()
    {
        if (TimeWidget_)
            TimeWidget_->update();
        update();
    }

    void VoipEventItem::setQuoteSelection()
    {
        /// TODO-quote
        assert(0);
    }

    QColor VoipEventItem::getTextColor(const bool isHovered)
    {
        return isHovered ? QColor(ql1s("#ffffff")) : MessageStyle::getTextColor();
    }

    void VoipEventItem::showHiddenControls()
    {
        if (get_gui_settings()->get_value<bool>(settings_hide_message_timestamps, true) && isVisible() && TimeWidget_)
            TimeWidget_->showAnimated();

        showMessageStatus();
    }

    void VoipEventItem::hideHiddenControls()
    {
        const bool canHide = get_gui_settings()->get_value<bool>(settings_hide_message_timestamps, true)
            && isVisible()
            && TimeWidget_
            && !IsBubbleHovered_;

        if (canHide)
            TimeWidget_->hideAnimated();

        hideMessageStatus();
    }

    void VoipEventItem::setTimestampHoverEnabled(const bool _enabled)
    {
        timestampHoverEnabled_ = _enabled;
    }

    namespace
    {

        int32_t getIconRightPadding()
        {
            return Utils::scale_value(12);
        }

        int32_t getIconTopPadding()
        {
            return Utils::scale_value(9);
        }

        int32_t getTextBaselineY()
        {
            return Utils::scale_value(21);
        }
    }
}

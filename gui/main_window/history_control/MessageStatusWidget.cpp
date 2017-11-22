#include "stdafx.h"
#include "MessageStatusWidget.h"

#include "../../utils/InterConnector.h"
#include "../../utils/utils.h"
#include "MessageItem.h"
#include "MessageStyle.h"
#include "../../gui_settings.h"
#include "../../theme_settings.h"

namespace
{
    static const auto minOpacityValue = 0.0;
    static const auto maxOpacityValue = 0.8;
    static const auto animDuration = 200;
}

namespace Ui
{
    MessageTimeWidget::MessageTimeWidget(HistoryControlPageItem *messageItem)
        : QWidget(messageItem)
        , visible_(true)
    {
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::forceShowMessageTimestamps, this, &MessageTimeWidget::showAnimated, Qt::QueuedConnection);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::forceHideMessageTimestamps, this, &MessageTimeWidget::hideAnimated, Qt::QueuedConnection);
    }

    void MessageTimeWidget::setTime(const int32_t timestamp)
    {
        TimeText_ = QDateTime::fromTime_t(timestamp).toString(qsl("HH:mm"));

        QFontMetrics m(MessageStyle::getTimeFont());

        TimeTextSize_ = m.boundingRect(TimeText_).size();
        const auto textWidth = m.width(TimeText_);
        if (TimeTextSize_.width() != textWidth)
            TimeTextSize_.setWidth(textWidth);

        updateGeometry();

        update();
    }

    QSize MessageTimeWidget::sizeHint() const
    {
        return TimeTextSize_;
    }

    void MessageTimeWidget::paintEvent(QPaintEvent *)
    {
        if (!visible_)
        {
            return;
        }

        const auto height = sizeHint().height();
        const auto cursorX = 0;
        const auto textBaseline = height;

        QPainter p(this);
        p.setFont(MessageStyle::getTimeFont());
        p.setPen(QPen(getTimeColor()));
        p.drawText(cursorX, textBaseline, TimeText_);
    }

    QColor MessageTimeWidget::getTimeColor() const
    {
        auto curTheme = Ui::get_qt_theme_settings()->themeForContact(aimId_);
        if (!curTheme)
            return MessageStyle::getTimeColor();

        return curTheme->preview_stickers_.time_color_;
    }

    void MessageTimeWidget::showAnimated()
    {
        if (visible_)
            return;

        visible_ = true;

        repaint();
    }

    void MessageTimeWidget::hideAnimated()
    {
        if (!visible_)
            return;

        visible_ = false;

        repaint();
    }

    void MessageTimeWidget::showIfNeeded()
    {
        if (!get_gui_settings()->get_value<bool>(settings_hide_message_timestamps, true))
            showAnimated();
    }
}
#include "stdafx.h"

#include "HistoryButtonDown.h"
#include "../../utils/utils.h"
#include "../../fonts.h"
#include "../../controls/CommonStyle.h"

UI_NS_BEGIN

int getUnreadsBubbleSize()
{
    return Utils::scale_value(20);
}

int getUnreadsFontSize()
{
    return Utils::scale_value(12);
}



HistoryButtonDown::HistoryButtonDown(QWidget* _parent, const QString& _imageName) :
    CustomButton(_parent, _imageName),
    numUnreads_(0)
{
}

HistoryButtonDown::HistoryButtonDown(QWidget* _parent, const QPixmap& _pixmap) :
    CustomButton(_parent, _pixmap),
    numUnreads_(0)
{
}

void HistoryButtonDown::paintEvent(QPaintEvent *_event)
{
    CustomButton::paintEvent(_event);

    if (numUnreads_ > 0)
    {
        // paint circle
        QPainter painter(this);

        const auto font = Fonts::appFont(getUnreadsFontSize(), Fonts::FontWeight::Medium);

        QPoint size = Utils::getUnreadsSize(&painter, font, true, numUnreads_, getUnreadsBubbleSize());

        const int btn_size = Utils::scale_value(40);
        const int area_size = Utils::scale_value(48);

        const int x = area_size - size.x();
        const int y = 0;

        const auto borderColor = QColor(Qt::transparent);
        const auto bgColor = CommonStyle::getColor(CommonStyle::Color::GREEN_FILL);
        const auto textColor = QColor(ql1s("#ffffff"));

        Utils::drawUnreads(&painter, font, bgColor, textColor, borderColor, numUnreads_, getUnreadsBubbleSize(), x, y);
    }
}

void HistoryButtonDown::setUnreadMessages(int num_unread)
{
    numUnreads_ = num_unread;
    repaint();
}

void HistoryButtonDown::addUnreadMessages(int num_add)
{
    numUnreads_ += num_add;
    repaint();
}

void HistoryButtonDown::wheelEvent(QWheelEvent * _event)
{
    emit sendWheelEvent(_event);
}



//////////////////////////////////////////////////////////////////////////
// HistoryButtonMentions
//////////////////////////////////////////////////////////////////////////

HistoryButtonMentions::HistoryButtonMentions(QWidget* _parent, const QString& _imageName) :
    CustomButton(_parent, _imageName),
    count_(0)
{
}

HistoryButtonMentions::HistoryButtonMentions(QWidget* _parent, const QPixmap& _pixmap) :
    CustomButton(_parent, _pixmap),
    count_(0)
{
}

void HistoryButtonMentions::paintEvent(QPaintEvent *_event)
{
    CustomButton::paintEvent(_event);

    if (count_ > 0)
    {
        const auto font = Fonts::appFont(getUnreadsFontSize(), Fonts::FontWeight::Medium);

        /// paint circle
        QPainter painter(this);
        QPoint size = Utils::getUnreadsSize(&painter, font, true, count_, getUnreadsBubbleSize());

        const int btn_size = Utils::scale_value(40);
        const int area_size = Utils::scale_value(48);

        const int x = area_size - size.x();
        const int y = 0;


        const auto borderColor = QColor(Qt::transparent);
        const auto bgColor = CommonStyle::getColor(CommonStyle::Color::GREEN_FILL);
        const auto textColor = QColor(ql1s("#ffffff"));

        Utils::drawUnreads(&painter, font, bgColor, textColor, borderColor, count_, getUnreadsBubbleSize(), x, y);
    }
}

void HistoryButtonMentions::wheelEvent(QWheelEvent * _event)
{
    emit sendWheelEvent(_event);
}

void HistoryButtonMentions::setCount(int32_t _count)
{
    count_ = _count;
    repaint();
}

UI_NS_END
#include "stdafx.h"
#include "TitleBar.h"
#include "MainWindow.h"

#include "../controls/CommonStyle.h"
#include "../core_dispatcher.h"
#include "../fonts.h"
#include "../my_info.h"
#include "../utils/gui_coll_helper.h"
#include "../utils/InterConnector.h"
#include "../utils/utils.h"
#include "contact_list/ContactListModel.h"
#include "contact_list/RecentsModel.h"
#include "contact_list/UnknownsModel.h"

namespace Ui
{
    UnreadWidget::UnreadWidget(QWidget* _parent, bool _drawBadgeBorder, int32_t _badgeFontSize)
        : QWidget(_parent)
        , pressed_(false)
        , hovered_(false)
        , hoverable_(true)
        , unreads_(0)
        , drawBadgeBorder_(_drawBadgeBorder)
        , fontSize_(_badgeFontSize)
    {
        setCursor(Qt::PointingHandCursor);
        setFixedSize(Utils::scale_value(Ui::TitleBar::icon_width),
                     Utils::scale_value(Ui::TitleBar::icon_height));
    }

    void UnreadWidget::setHoverVisible(bool _hoverVisible)
    {
        hoverable_ = _hoverVisible;
    }

    QPixmap UnreadWidget::renderToPixmap(unsigned _unreadsCount, bool _hoveredState, bool _pressedState)
    {
        auto pxSize = size();
        if (Utils::is_mac_retina())
            pxSize *= 2;

        QPixmap px(pxSize);
        Utils::check_pixel_ratio(px);
        px.fill(Qt::transparent);

        auto iconName = pathIcon_;
        if (_pressedState)
            iconName = pathIconPressed_;
        else if (_hoveredState)
            iconName = pathIconHovered_;

        QPainter p(&px);
        p.setRenderHint(QPainter::Antialiasing);
        auto icon = QPixmap(Utils::parse_image_name(iconName));
        Utils::check_pixel_ratio(icon);
        p.drawPixmap(0, 0, icon);

        if (_unreadsCount > 0)
        {
            const auto bgColor = CommonStyle::getColor(CommonStyle::Color::GREEN_FILL);
            const auto textColor = QColor(ql1s("#ffffff"));
            const auto borderColor = drawBadgeBorder_ ? QWidget::palette().color(QWidget::backgroundRole()) : QColor();

            auto font = Fonts::appFontScaled(fontSize_, Fonts::FontWeight::Medium);
            auto balloonSizeScaled = Utils::getUnreadsSize(&p, font, drawBadgeBorder_, _unreadsCount, Utils::scale_value(Ui::TitleBar::balloon_size));
            Utils::drawUnreads(
                               &p,
                               font,
                               bgColor,
                               textColor,
                               borderColor,
                               _unreadsCount,
                               Utils::scale_value(Ui::TitleBar::balloon_size),
                               Utils::scale_value(Ui::TitleBar::icon_width) - balloonSizeScaled.x(),
                               Utils::scale_value(Ui::TitleBar::balloon_size / 5)
                               );
        }
        return px;
    }

    void UnreadWidget::paintEvent(QPaintEvent *e)
    {
        auto px = renderToPixmap(unreads_, hoverable_ && hovered_, pressed_);
        QPainter p(this);
        p.drawPixmap(0, 0, px);
    }

    void UnreadWidget::mousePressEvent(QMouseEvent *e)
    {
        pressed_ = true;
        update();
        QWidget::mousePressEvent(e);
    }

    void UnreadWidget::mouseReleaseEvent(QMouseEvent *e)
    {
        pressed_ = false;
        update();
        emit clicked();
        QWidget::mouseReleaseEvent(e);
    }

    void UnreadWidget::enterEvent(QEvent * e)
    {
        hovered_ = true;
        update();
        QWidget::enterEvent(e);
    }

    void UnreadWidget::leaveEvent(QEvent * e)
    {
        hovered_ = false;
        update();
        QWidget::leaveEvent(e);
    }

    void UnreadWidget::setUnreads(unsigned _unreads)
    {
        unreads_ = _unreads;
        update();
    }

    UnreadMsgWidget::UnreadMsgWidget(QWidget* parent)
        : UnreadWidget(parent, false, 14)
    {
        pathIcon_ = qsl(":/titlebar/capture_recents_100");
        pathIconHovered_ = qsl(":/titlebar/capture_recents_100_active");
        pathIconPressed_ = qsl(":/titlebar/capture_recents_100_active");

        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::im_created, this, &UnreadMsgWidget::loggedIn, Qt::QueuedConnection);
        connect(this, &UnreadMsgWidget::clicked, this, &UnreadMsgWidget::openNextUnread);
    }

    void UnreadMsgWidget::loggedIn()
    {
        connect(Logic::getRecentsModel(), &Logic::RecentsModel::dlgStatesHandled, this, &UnreadMsgWidget::updateIcon, Qt::QueuedConnection);
        connect(Logic::getRecentsModel(), &Logic::RecentsModel::updated, this, &UnreadMsgWidget::updateIcon, Qt::QueuedConnection);
        connect(Logic::getUnknownsModel(), &Logic::UnknownsModel::dlgStatesHandled, this, &UnreadMsgWidget::updateIcon, Qt::QueuedConnection);
        connect(Logic::getUnknownsModel(), &Logic::UnknownsModel::updated, this, &UnreadMsgWidget::updateIcon, Qt::QueuedConnection);
        connect(Logic::getContactListModel(), &Logic::ContactListModel::contactChanged, this, &UnreadMsgWidget::updateIcon, Qt::QueuedConnection);
    }

    void UnreadMsgWidget::openNextUnread()
    {
        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::titlebar_message);

        emit Utils::InterConnector::instance().activateNextUnread();
    }

    void UnreadMsgWidget::updateIcon()
    {
        const auto count = Logic::getRecentsModel()->totalUnreads() + Logic::getUnknownsModel()->totalUnreads();
        setUnreads(count);
    }

    UnreadMailWidget::UnreadMailWidget(QWidget* parent)
        : UnreadWidget(parent, false, 14)
    {
        pathIcon_ = qsl(":/titlebar/capture_mail_100");
        pathIconHovered_ = qsl(":/titlebar/capture_mail_100_active");
        pathIconPressed_ = qsl(":/titlebar/capture_mail_100_active");

        connect(Ui::GetDispatcher(), &core_dispatcher::mailStatus, this, &UnreadMailWidget::mailStatus, Qt::QueuedConnection);
        connect(this, &UnreadMailWidget::clicked, this, &UnreadMailWidget::openMailBox);
    }

    void UnreadMailWidget::mailStatus(QString _email, unsigned _unreads, bool)
    {
        Email_ = _email;
        setUnreads(_unreads);
    }

    void UnreadMailWidget::openMailBox()
    {
        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::titlebar_mail);

        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("email", Email_.isEmpty() ? MyInfo()->aimId() : Email_);
        Ui::GetDispatcher()->post_message_to_core(qsl("mrim/get_key"), collection.get(), this, [this](core::icollection* _collection)
        {
            Utils::openMailBox(Email_, QString::fromUtf8(Ui::gui_coll_helper(_collection, false).get_value_as_string("key")), QString());
        });
    }
}

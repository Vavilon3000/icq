#include "stdafx.h"
#include "MentionItemDelegate.h"
#include "Common.h"

#include "../../cache/avatars/AvatarStorage.h"
#include "ContactListModel.h"

namespace Logic
{
    MentionItemDelegate::MentionItemDelegate(QObject * parent)
        : QItemDelegate(parent)
    {

    }

    void MentionItemDelegate::paint(QPainter * _painter, const QStyleOptionViewItem & _option, const QModelIndex & _index) const
    {
        const auto item = _index.data(Qt::DisplayRole).value<MentionModelItem>();
        const auto model = qobject_cast<const MentionModel*>(_index.model());
        const auto isSelected = (_option.state & QStyle::State_Selected);

        _painter->save();
        _painter->setRenderHint(QPainter::Antialiasing);
        _painter->setRenderHint(QPainter::TextAntialiasing);
        _painter->setRenderHint(QPainter::SmoothPixmapTransform);

        const auto isServiceItem = item.aimId_.startsWith(ql1c('~'));
        const auto inHoverColor = !isServiceItem && isSelected;
        ContactList::RenderMouseState(*_painter, inHoverColor, false, _option.rect);

        _painter->translate(_option.rect.topLeft());

        if (isServiceItem)
        {
            renderServiceItem(*_painter, _option, item);
            _painter->restore();
            return;
        }

        bool isDef = false;
        auto avatar = Logic::GetAvatarStorage()->GetRounded(
            item.aimId_,
            item.friendlyName_,
            Utils::scale_bitmap(avatarSize()),
            QString(),//Logic::getContactListModel()->getState(_item.aimId_),
            isDef,
            false,
            false
        );

        if (!avatar->isNull())
        {
            const auto ratio = Utils::scale_bitmap(1);
            ContactList::RenderAvatar(*_painter, itemLeftPadding(), (_option.rect.height() - avatar->height() / ratio) / 2, *avatar, avatarSize());
        }

        //ContactList::RenderContactName(_painter, )

        QPen pen;
        pen.setColor(CommonStyle::getColor(CommonStyle::Color::TEXT_PRIMARY));
        _painter->setPen(pen);

        auto font = Fonts::appFontScaled(14, Fonts::FontWeight::Normal);
        _painter->setFont(font);

        Utils::drawText(*_painter, QPointF(textLeftPadding(), _option.rect.height() / 2), Qt::AlignVCenter, item.friendlyName_);

        _painter->restore();
    }

    QSize MentionItemDelegate::sizeHint(const QStyleOptionViewItem & _option, const QModelIndex & _index) const
    {
        auto height = itemHeight();

        const auto model = qobject_cast<const MentionModel*>(_index.model());
        if (model && model->isServiceItem(_index))
            height = serviceItemHeight();

        return QSize(_option.rect.width(), height);
    }

    int MentionItemDelegate::itemHeight() const
    {
        return Utils::scale_value(32);
    }

    int MentionItemDelegate::serviceItemHeight() const
    {
        return Utils::scale_value(20);
    }

    void MentionItemDelegate::renderServiceItem(QPainter& _painter, const QStyleOptionViewItem & _option, const MentionModelItem & _item) const
    {
        _painter.save();
        _painter.setRenderHint(QPainter::Antialiasing);
        _painter.setRenderHint(QPainter::TextAntialiasing);

        static auto pen(CommonStyle::getColor(CommonStyle::Color::TEXT_LIGHT));
        _painter.setPen(pen);

        static auto font(Fonts::appFontScaled(12, platform::is_windows_vista_or_late() ? Fonts::FontWeight::Medium : Fonts::FontWeight::Normal));
        _painter.setFont(font);

        Utils::drawText(_painter, QPointF(itemLeftPadding(), _option.rect.height() / 2), Qt::AlignVCenter, _item.friendlyName_);

        _painter.restore();
    }

    int MentionItemDelegate::itemLeftPadding() const
    {
        return Utils::scale_value(12);
    }

    int MentionItemDelegate::textLeftPadding() const
    {
        return itemLeftPadding() + Utils::scale_value(32);
    }

    int MentionItemDelegate::avatarSize() const
    {
        return Utils::scale_value(20);
    }
}

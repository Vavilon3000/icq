#pragma once

#include "Common.h"
#include "MentionModel.h"

namespace Logic
{
    class MentionItemDelegate : public QItemDelegate
    {
        Q_OBJECT
    public:
        MentionItemDelegate(QObject* parent);

        void paint(QPainter* _painter, const QStyleOptionViewItem& _option, const QModelIndex& _index) const override;

        QSize sizeHint(const QStyleOptionViewItem &_option, const QModelIndex &_index) const override;

        int itemHeight() const;
        int serviceItemHeight() const;

    private:
        void renderServiceItem(QPainter& _painter, const QStyleOptionViewItem& _option, const MentionModelItem& _item) const;

        int itemLeftPadding() const;
        int textLeftPadding() const;
        int avatarSize() const;

    };
}

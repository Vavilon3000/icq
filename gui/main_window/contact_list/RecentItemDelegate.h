#pragma once

#include "../../types/message.h"
#include "../../types/typing.h"
#include "Common.h"

namespace Logic
{
    class RecentItemDelegate : public AbstractItemDelegateWithRegim
    {
        bool shouldRenderCompact(const QModelIndex& _index) const;

    public:

        RecentItemDelegate(QObject* parent);

        void paint(QPainter* _painter, const QStyleOptionViewItem& _option, const QModelIndex& _index) const override;
        void paint(QPainter* _painter, const QStyleOptionViewItem& _option, const Data::DlgState& _dlgState, bool _dragOverlay, bool _renderAsInCL = true) const;

        QSize sizeHint(const QStyleOptionViewItem &_option, const QModelIndex &_index) const override;
        QSize sizeHintForAlert() const;

        void addTyping(const TypingFires& _typing);
        void removeTyping(const TypingFires& _typing);

        void setPictOnlyView(bool _pictOnlyView);
        bool getPictOnlyView() const;

        virtual void blockState(bool value) override;
        virtual void setDragIndex(const QModelIndex& index) override;
        virtual void setFixedWidth(int _newWidth) override;
        virtual void setRegim(int _regim) override;

    private:

        std::list<TypingFires> typings_;

        struct ItemKey
        {
            const bool IsSelected;

            const bool IsHovered;

            const int UnreadDigitsNumber;

            ItemKey(const bool isSelected, const bool isHovered, const int unreadDigitsNumber);

            bool operator < (const ItemKey &_key) const;
        };

        bool StateBlocked_;

        QModelIndex DragIndex_;

        ContactList::ViewParams viewParams_;

        mutable QSet<QString> pendingsDialogs;
    };
}

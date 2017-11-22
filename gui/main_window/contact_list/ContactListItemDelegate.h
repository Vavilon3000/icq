#pragma once

#include "Common.h"

namespace Logic
{
    class ChatMembersModel;

	class ContactListItemDelegate : public AbstractItemDelegateWithRegim
	{
	public:
		ContactListItemDelegate(QObject* parent, int _regim, ChatMembersModel* chatMembersModel = nullptr);

		virtual ~ContactListItemDelegate();

		void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

		QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

        virtual void setFixedWidth(int width) override;

        virtual void blockState(bool value) override;

        virtual void setDragIndex(const QModelIndex& index) override;

        virtual void setRegim(int _regim) override;

        void setLeftMargin(int margin);

        void setRightMargin(int margin);

        void setRenderRole(bool render);

	private:
        bool StateBlocked_;
        bool renderRole_;
        QModelIndex DragIndex_;
        ContactList::ViewParams viewParams_;
        ChatMembersModel* chatMembersModel_;
    };
}

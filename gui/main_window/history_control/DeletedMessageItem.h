#pragma once

#include "HistoryControlPageItem.h"

namespace Ui
{
    class DeletedMessageItem : public HistoryControlPageItem
    {
        QString formatRecentsText() const override { return QString(); }

    public:
        explicit DeletedMessageItem(QWidget* _parent);
        virtual ~DeletedMessageItem();

		virtual void setQuoteSelection() override;
    };

}
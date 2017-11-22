#pragma once

#include "HistoryControlPageItem.h"

namespace Ui
{

    namespace MessageTimestamp
    {
        static const auto hoverTimestampShowDelay = 200;
    }

    class MessageItemBase : public HistoryControlPageItem
    {
        Q_OBJECT

    public:
        MessageItemBase(QWidget* _parent);

        virtual ~MessageItemBase() = 0;

        virtual bool isOutgoing() const = 0;
    };

}


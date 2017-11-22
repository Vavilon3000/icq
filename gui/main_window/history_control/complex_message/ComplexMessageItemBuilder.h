#pragma once

#include "../../../namespaces.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

class ComplexMessageItem;

namespace ComplexMessageItemBuilder
{
    std::unique_ptr<ComplexMessageItem> makeComplexItem(
        QWidget *_parent,
        const int64_t _id,
        const QDate _date,
        const int64_t _prev,
        const QString& _text,
        const QString& _chatAimid,
        const QString& _senderAimid,
        const QString& _senderFriendly,
        const QVector<Data::Quote>& _quotes,
        const Data::MentionMap& _mentions,
        HistoryControl::StickerInfoSptr _sticker,
        const bool _isOutgoing,
        const bool _isNotAuth);

}

UI_COMPLEX_MESSAGE_NS_END
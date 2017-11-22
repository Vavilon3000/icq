#include "stdafx.h"
#include "MessagesModel.h"

#include "ChatEventItem.h"
#include "DeletedMessageItem.h"
#include "MessageItem.h"
#include "MessageStyle.h"
#include "ServiceMessageItem.h"
#include "ContentWidgets/FileSharingWidget.h"

#include "../contact_list/ContactListModel.h"
#include "../contact_list/RecentsModel.h"
#include "../contact_list/UnknownsModel.h"
#include "../history_control/ChatEventInfo.h"
#include "../history_control/FileSharingInfo.h"
#include "../history_control/VoipEventInfo.h"
#include "../history_control/VoipEventItem.h"
#include "../history_control/complex_message/ComplexMessageItem.h"
#include "../history_control/complex_message/ComplexMessageItemBuilder.h"

#include "../../core_dispatcher.h"
#include "../../gui_settings.h"
#include "../../my_info.h"
#include "../../utils/log/log.h"
#include "../../cache/emoji/Emoji.h"
#include "../../utils/utils.h"
#include "../../utils/gui_coll_helper.h"
#include "../../utils/InterConnector.h"
#include <boost/scope_exit.hpp>
#include <boost/range/adaptor/map.hpp>
#include <boost/range/adaptor/reversed.hpp>

#define CHECK_THREAD {assert(QThread::currentThread() == qApp->thread());}

namespace
{
    QString NormalizeAimId(const QString& _aimId)
    {
        const int pos = _aimId.indexOf(ql1s("@uin.icq"));
        return pos == -1 ? _aimId : _aimId.left(pos);
    }

    QString GetChatFriendly(const QString& _aimId, const QString& _chatFriendly)
    {
        const QString normalized = NormalizeAimId(_aimId);
        const QString clFriendly = Logic::getContactListModel()->getDisplayName(normalized);
        if (clFriendly == normalized)
        {
            return _chatFriendly.isEmpty() ? _aimId : _chatFriendly;
        }

        return clFriendly;
    }

    template<typename T>
    bool isThereHole(T first, T last)
    {
        if (std::distance(first, last) < 2)
            return false;

        auto prev = (*std::prev(last))->Prev_;

        typedef std::reverse_iterator<T> RIter;
        for (auto it = std::next(RIter(last)), end = RIter(first); it != end; ++it)
        {
            if ((*it)->Id_ != prev && !(*it)->IsDeleted())
                return true;

            prev = (*it)->Prev_;
        }
        return false;
    }

    template<typename T>
    T findHole(T first, T last)
    {
        if (std::distance(first, last) < 2)
            return last;

        for (auto it = first; it != last; ++it)
        {
            const auto next = std::next(first);
            if (next != last)
            {
                if ((*next)->Prev_ != (*it)->Id_)
                    return next;
            }
        }
        return last;
    }
}

namespace Logic
{
    const MessageKey MessageKey::MAX(std::numeric_limits<qint64>::max(), std::numeric_limits<qint64>::max() - 1, QString(), -1, 0, core::message_type::base, false, preview_type::none, control_type::ct_message);
    const MessageKey MessageKey::MIN(2, 1, QString(), -1, 0, core::message_type::base, false, preview_type::none, control_type::ct_message);

    MessageKey::MessageKey()
        : id_(-1)
        , prev_(-1)
        , type_(core::message_type::base)
        , controlType_(control_type::ct_message)
        , time_(-1)
        , pendingId_(-1)
        , previewType_(preview_type::none)
        , outgoing_(false)
    {
    }

    MessageKey::MessageKey(
        const qint64 _id,
        const control_type _control_type)
            : id_(_id)
            , prev_(-1)
            , type_(core::message_type::base)
            , controlType_(_control_type)
            , time_(-1)
            , pendingId_(-1)
            , previewType_(preview_type::none)
            , outgoing_(false)

    {
    }

    MessageKey::MessageKey(
        const qint64 _id,
        const qint64 _prev,
        const QString& _internalId,
        const int _pendingId,
        const qint32 _time,
        const core::message_type _type,
        const bool _outgoing,
        const preview_type _previewType,
        const control_type _control_type)
        : id_(_id)
        , prev_(_prev)
        , internalId_(_internalId)
        , type_(_type)
        , controlType_(_control_type)
        , time_(_time)
        , pendingId_(_pendingId)
        , previewType_(_previewType)
        , outgoing_(_outgoing)
    {
        assert(type_ > core::message_type::min);
        assert(type_ < core::message_type::max);
        assert(previewType_ > preview_type::min);
        assert(previewType_ < preview_type::max);
    }

    bool MessageKey::operator<(const MessageKey& _other) const
    {
        return compare(_other);
    }

    bool MessageKey::hasId() const
    {
        return (id_ != -1);
    }

    bool MessageKey::checkInvariant() const
    {
        if (outgoing_)
        {
            if (!hasId() && internalId_.isEmpty())
            {
                return false;
            }
        }
        else
        {
            if (!internalId_.isEmpty())
            {
                return false;
            }
        }

        return true;
    }

    bool MessageKey::isChatEvent() const
    {
        assert(type_ >= core::message_type::min);
        assert(type_ <= core::message_type::max);

        return (type_ == core::message_type::chat_event);
    }

    bool MessageKey::isOutgoing() const
    {
        return outgoing_;
    }

    void MessageKey::setId(const int64_t _id)
    {
        assert(_id >= -1);

        id_ = _id;
    }

    qint64 MessageKey::getId() const
    {
        return id_;
    }

    void MessageKey::setOutgoing(const bool _isOutgoing)
    {
        outgoing_ = _isOutgoing;

        if (!hasId() && outgoing_)
        {
            assert(!internalId_.isEmpty());
        }
    }

    void MessageKey::setControlType(const control_type _controlType)
    {
        controlType_ = _controlType;
    }

    QString MessageKey::toLogStringShort() const
    {
        return ql1s("id=") % QString::number(id_) % ql1s(";prev=") % QString::number(prev_);
    }

    bool MessageKey::isFileSharing() const
    {
        assert(type_ >= core::message_type::min);
        assert(type_ <= core::message_type::max);

        return (type_ == core::message_type::file_sharing);
    }

    bool MessageKey::isVoipEvent() const
    {
        assert(type_ >= core::message_type::min);
        assert(type_ <= core::message_type::max);

        return (type_ == core::message_type::voip_event);
    }

    bool MessageKey::isDate() const
    {
        return (controlType_ == control_type::ct_date);
    }

    bool MessageKey::isSticker() const
    {
        assert(type_ >= core::message_type::min);
        assert(type_ <= core::message_type::max);

        return (type_ == core::message_type::sticker);
    }

    control_type MessageKey::getControlType() const
    {
        return controlType_;
    }

    core::message_type MessageKey::getType() const
    {
        return type_;
    }

    void MessageKey::setType(core::message_type _type)
    {
        type_ = _type;
    }

    preview_type MessageKey::getPreviewType() const
    {
        return previewType_;
    }

    qint64 MessageKey::getPrev() const
    {
        return prev_;
    }

    const QString& MessageKey::getInternalId() const
    {
        return internalId_;
    }

    int MessageKey::getPendingId() const
    {
        return pendingId_;
    }

    bool MessageKey::compare(const MessageKey& _rhs) const
    {
        if (!internalId_.isEmpty() && internalId_ == _rhs.internalId_)
        {
            return controlType_ < _rhs.controlType_;
        }

        if (pendingId_ != -1 && _rhs.pendingId_ == -1)
        {
            if (controlType_ != _rhs.controlType_)
                return controlType_ < _rhs.controlType_;

            if (controlType_ == _rhs.controlType_)
                return false;

            return time_ == _rhs.time_ ? false : time_ < _rhs.time_;
        }

        if (pendingId_ == -1 && _rhs.pendingId_ != -1)
        {
            if (controlType_ != _rhs.controlType_)
                return controlType_ < _rhs.controlType_;

            if (controlType_ == _rhs.controlType_)
                return true;

            return time_ == _rhs.time_ ? true : time_ < _rhs.time_;
        }

        if ((id_ != -1) && (_rhs.id_ != -1))
        {
            if (id_ != _rhs.id_)
            {
                return (id_ < _rhs.id_);
            }
        }

        if (pendingId_ != -1 && _rhs.pendingId_ != -1)
        {
            if (pendingId_ != _rhs.pendingId_)
            {
                return (pendingId_ < _rhs.pendingId_);
            }
            else if (internalId_ != _rhs.internalId_)
            {
                return time_ < _rhs.time_;
            }
        }

        return (controlType_ < _rhs.controlType_);
    }

    bool Message::isBase() const
    {
        assert(key_.getType() > core::message_type::min);
        assert(key_.getType() < core::message_type::max);

        return (key_.getType() == core::message_type::base);
    }

    bool Message::isChatEvent() const
    {
        assert(key_.getType() > core::message_type::min);
        assert(key_.getType() < core::message_type::max);

        assert(!buddy_ || key_.getType() == getBuddy()->GetType());
        return (key_.getType() == core::message_type::chat_event);
    }

    bool Message::isDate() const
    {
        return (key_.isDate());
    }

    bool Message::isDeleted() const
    {
        return deleted_;
    }

    void Message::setDeleted(const bool _deleted)
    {
        deleted_ = _deleted;

        if (buddy_)
        {
            buddy_->SetDeleted(_deleted);
        }
    }

    bool Message::isFileSharing() const
    {
        assert(key_.getType() > core::message_type::min);
        assert(key_.getType() < core::message_type::max);
        assert((key_.getType() != core::message_type::file_sharing) || fileSharing_);
        assert(!buddy_ || key_.getType() == buddy_->GetType());

        return (key_.getType() == core::message_type::file_sharing);
    }

    bool Message::isOutgoing() const
    {
        assert(!buddy_ || key_.isOutgoing() == getBuddy()->IsOutgoing());
        return key_.isOutgoing();
    }

    bool Message::isPending() const
    {
        return (!key_.hasId() && !key_.getInternalId().isEmpty());
    }

    bool Message::isStandalone() const
    {
        return true;
        //return (IsFileSharing() || IsSticker() || IsChatEvent() || IsVoipEvent() || IsPreview() || IsDeleted() || IsPending());
    }

    bool Message::isSticker() const
    {
        assert(key_.getType() > core::message_type::min);
        assert(key_.getType() < core::message_type::max);
        assert((key_.getType() != core::message_type::sticker) || sticker_);

        return (key_.getType() == core::message_type::sticker);
    }

    bool Message::isVoipEvent() const
    {
        assert(key_.getType() > core::message_type::min);
        assert(key_.getType() < core::message_type::max);
        assert((key_.getType() != core::message_type::voip_event) || voipEvent_);

        return (key_.getType() == core::message_type::voip_event);
    }

    bool Message::isPreview() const
    {
        return (key_.getPreviewType() != preview_type::none);
    }

    const HistoryControl::ChatEventInfoSptr& Message::getChatEvent() const
    {
        assert(!chatEvent_ || isChatEvent());

        return chatEvent_;
    }

    const QString& Message::getChatSender() const
    {
        return chatSender_;
    }

    const HistoryControl::FileSharingInfoSptr& Message::getFileSharing() const
    {
        assert(!fileSharing_ || isFileSharing());

        return fileSharing_;
    }


    const HistoryControl::StickerInfoSptr& Message::getSticker() const
    {
        assert(!sticker_ || isSticker());

        return sticker_;
    }

    const HistoryControl::VoipEventInfoSptr& Message::getVoipEvent() const
    {
        assert(!voipEvent_ || isVoipEvent());

        return voipEvent_;
    }

    void Message::setChatEvent(const HistoryControl::ChatEventInfoSptr& _info)
    {
        assert(!chatEvent_);
        assert(!_info || (key_.getType() == core::message_type::chat_event));

        chatEvent_ = _info;
    }

    void Message::setChatSender(const QString& _chatSender)
    {
        chatSender_ = _chatSender;
    }

    void Message::setFileSharing(const HistoryControl::FileSharingInfoSptr& _info)
    {
        assert(!fileSharing_);
        assert(!_info || (key_.getType() == core::message_type::file_sharing));

        fileSharing_ = _info;
    }

    void Message::setSticker(const HistoryControl::StickerInfoSptr& _info)
    {
        assert(!sticker_);
        assert(!_info || (key_.getType() == core::message_type::sticker));

        sticker_ = _info;
    }

    void Message::setVoipEvent(const HistoryControl::VoipEventInfoSptr& _info)
    {
        assert(!voipEvent_);
        assert(!_info || (key_.getType() == core::message_type::voip_event));

        voipEvent_ = _info;
    }

    void Message::applyModification(const Data::MessageBuddy& _modification)
    {
        assert(key_.getId() == _modification.Id_);

        EraseEventData();

        if (_modification.IsBase())
        {
            key_.setType(core::message_type::base);

            return;
        }

        if (_modification.IsChatEvent())
        {
            key_.setType(core::message_type::base);

            auto key = getKey();
            key.setType(core::message_type::chat_event);
            setKey(key);
            setChatEvent(_modification.GetChatEvent());
            getBuddy()->SetType(core::message_type::chat_event);

            return;
        }

        buddy_->ApplyModification(_modification);
    }

    void Message::EraseEventData()
    {
        fileSharing_.reset();
        sticker_.reset();
        chatEvent_.reset();
        voipEvent_.reset();
    }

    void Message::setBuddy(std::shared_ptr<Data::MessageBuddy> _buddy)
    {
        buddy_ = _buddy;
    }

    std::shared_ptr<Data::MessageBuddy> Message::getBuddy() const
    {
        return buddy_;
    }

    const QString& Message::getAimId() const
    {
        return aimId_;
    }

    void Message::setKey(const MessageKey& _key)
    {
        key_ = _key;
    }

    const MessageKey& Message::getKey() const
    {
        return key_;
    }

    const QDate& Message::getDate()
    {
        return date_;
    }

    void Message::setDate(const QDate& _date)
    {
        date_ = _date;
    }

    void Message::setChatFriendly(const QString& _friendly)
    {
        chatFriendly_ = _friendly;
    }

    const QString& Message::getChatFriendly()
    {
        return chatFriendly_;
    }


    //////////////////////////////////////////////////////////////////////////
    // ContactDialog
    //////////////////////////////////////////////////////////////////////////

    void ContactDialog::setLastRequestedMessage(const qint64 _message)
    {
        if (!lastRequestedMessage_)
        {
            lastRequestedMessage_ = std::make_unique<qint64>(_message);
            return;
        }

        *lastRequestedMessage_ = _message;
    }

    qint64 ContactDialog::getLastRequestedMessage() const
    {
        if (!lastRequestedMessage_)
        {
            return -1;
        }

        return *lastRequestedMessage_;
    }

    bool ContactDialog::isLastRequestedMessageEmpty() const
    {
        return !lastRequestedMessage_;
    }

    void ContactDialog::setInitMessage(qint64 _message) noexcept
    {
        initMessage_ = _message;
    }

    qint64 ContactDialog::getInitMessage() const noexcept
    {
        return initMessage_;
    }

    void ContactDialog::setFirstUnreadMessage(qint64 _message) noexcept
    {
        firstUnreadMessage_ = _message;
    }

    qint64 ContactDialog::getFirstUnreadMessage() const noexcept
    {
        return firstUnreadMessage_;
    }

    void ContactDialog::setMessageCountAfter(qint64 _count) noexcept
    {
        messageCountAfter_ = _count;
    }

    qint64 ContactDialog::getMessageCountAfter() const noexcept
    {
        return messageCountAfter_;
    }

    bool ContactDialog::isJumpToMessageEnabled() const noexcept
    {
        return needToJumpToMessage_;
    }

    void ContactDialog::enableJumpToMessage(bool _enable) noexcept
    {
        needToJumpToMessage_ = _enable;
    }

    void ContactDialog::setScrollMode(scroll_mode_type _type) noexcept
    {
        scrollMode_ = _type;
    }

    scroll_mode_type ContactDialog::getScrollMode() const noexcept
    {
        return scrollMode_;
    }

    void ContactDialog::enablePostponeUpdate(bool _enable) noexcept
    {
        postponeUpdate_ = _enable;
    }

    bool ContactDialog::isPostponeUpdateEnabled() const noexcept
    {
        return postponeUpdate_;
    }

    MessagesMap& ContactDialog::getMessages()
    {
        if (!messages_)
            messages_ = std::make_unique<MessagesMap>();

        return *messages_;
    }

    MessagesMap& ContactDialog::getPendingMessages()
    {
        if (!pendingMessages_)
            pendingMessages_ = std::make_unique<MessagesMap>();

        return *pendingMessages_;
    }

    DatesMap& ContactDialog::getDatesMap()
    {
        if (!dateItems_)
            dateItems_ = std::make_unique<DatesMap>();

        return *dateItems_;
    }

    const MessageKey& ContactDialog::getLastKey() const
    {
        return lastKey_;
    }

    const MessageKey& ContactDialog::getFirstKey() const
    {
        return firstKey_;
    }

    void ContactDialog::setLastKey(const MessageKey& _key)
    {
        lastKey_ = _key;
    }

    void ContactDialog::deleteLastKey()
    {
        lastKey_.setEmpty();
    }

    void ContactDialog::setFirstKey(const MessageKey& _key)
    {
        firstKey_ = _key;
    }

    bool ContactDialog::hasItemsInBetween(const MessageKey& _l, const MessageKey& _r) const
    {
        if (!messages_)
            return false;

        const auto iterL = messages_->upper_bound(_l);
        const auto iterR = messages_->lower_bound(_r);

        for (auto iter = iterL; iter != iterR; ++iter)
        {
            if (!iter->second.isDeleted())
            {
                return true;
            }
        }

        return false;
    }

    std::shared_ptr<Data::MessageBuddy> ContactDialog::addDateItem(const QString& _aimId, const MessageKey& _key, const QDate& _date)
    {
        assert(_date.isValid());
        assert(_key.hasId());
        assert(_key.getControlType() == control_type::ct_date);

        auto message = std::make_shared<Data::MessageBuddy>();
        message->Id_ = _key.getId();
        message->Prev_ = _key.getPrev();
        message->AimId_ = _aimId;
        message->SetTime(0);
        message->SetDate(_date);
        message->SetType(core::message_type::undefined);

        Message dateMessage(_aimId);
        dateMessage.setKey(_key);
        dateMessage.setDate(_date);
        getDatesMap().emplace(_date, dateMessage);

        __INFO(
            "gui_dates",
            "added gui date item\n"
            "    contact=<" << _aimId << ">\n"
            "    id=<" << _key.getId() << ">\n"
            "    prev=<" << _key.getPrev() << ">"
            );

        return message;
    }

    bool ContactDialog::hasDate(const QDate& _date) const
    {
        if (!dateItems_)
            return false;

        return (dateItems_->count(_date) > 0);
    }

    void ContactDialog::removeDateItem(const QDate& _date)
    {
        getDatesMap().erase(_date);
    }

    void ContactDialog::removeDateItems()
    {
        dateItems_.reset();
    }

    Logic::MessageKey ContactDialog::findFirstKeyAfter(const Logic::MessageKey& _key) const
    {
        CHECK_THREAD
        assert(!_key.isEmpty());

        if (!messages_)
        {
            return Logic::MessageKey();
        }

        auto messageIter = messages_->upper_bound(_key);
        if ((messageIter == messages_->begin()) ||
            (messageIter == messages_->end()))
        {
            return Logic::MessageKey();
        }

        return messageIter->first;
    }

    Logic::MessageKey ContactDialog::getKey(qint64 _id) const
    {
        CHECK_THREAD
        if (messages_)
        {
            const auto end = messages_->end();
            const auto it = std::find_if(messages_->begin(), end, [_id](const auto& _msg) { return _msg.first.getId() == _id; });
            if (it != end)
                return (*it).first;
        }
        return Logic::MessageKey();
    }

    int64_t ContactDialog::getLastMessageId() const
    {
        CHECK_THREAD
        if (!messages_)
        {
            return -1;
        }

        if (!messages_->empty())
        {
            return messages_->crbegin()->first.getId();
        }

        return -1;
    }

    bool ContactDialog::hasPending() const
    {
        CHECK_THREAD
        if (!pendingMessages_)
        {
            return false;
        }

        return !pendingMessages_->empty();
    }

    void ContactDialog::setNewKey(const MessageKey& _key)
    {
        CHECK_THREAD
        if (!newKey_)
        {
            newKey_ = std::make_unique<MessageKey>(_key);
        }

        *newKey_ = _key;
    }

    const MessageKey* ContactDialog::getNewKey() const
    {
        CHECK_THREAD
        if (!newKey_)
        {
            return nullptr;
        }

        return newKey_.get();
    }

    void ContactDialog::resetNewKey()
    {
        newKey_.reset();
    }

    void ContactDialog::setRecvLastMessage(bool _value)
    {
        recvLastMessage_ = _value;
    }

    bool ContactDialog::getRecvLastMessage() const
    {
        return recvLastMessage_;
    }

    SendersVector ContactDialog::getSenders(bool _includeSelf) const
    {
        CHECK_THREAD
        SendersVector result;

        if (!messages_)
            return result;

        for (const auto& it : boost::adaptors::reverse(*messages_))
        {
            const auto& msg = it.second;
            if (!msg.isChatEvent() && !msg.isDate())
            {
                if (msg.isOutgoing() && !_includeSelf)
                    continue;

                const auto buddy = it.second.getBuddy();
                if (!buddy)
                    continue;

                const auto sender = NormalizeAimId(buddy->Chat_? buddy->GetChatSender(): buddy->AimId_);
                QString friendly;
                if (buddy->Chat_)
                    friendly = GetChatFriendly(sender, buddy->ChatFriendly_);
                else
                {
                    if (Logic::getContactListModel()->contains(sender))
                        friendly = Logic::getContactListModel()->getDisplayName(sender);
                    else
                        friendly = buddy->ChatFriendly_;
                }

                if (!friendly.isEmpty())
                {
                    if (!std::any_of(result.begin(), result.end(), [&sender](const auto& s) { return s.aimId_ == sender; }))
                    {
                        result.emplace_back(sender, friendly);
                    }
                }
            }
        }
        return result;
    }

    //////////////////////////////////////////////////////////////////////////
    // MessagesModel
    //////////////////////////////////////////////////////////////////////////
    std::unique_ptr<MessagesModel> g_messages_model;


    MessagesModel::MessagesModel(QObject *parent)
        : QObject(parent)
        , itemWidth_(0)
    {
        setObjectName(qsl("MessagesModel"));
        const bool connections[] = {
        connect(
            Ui::GetDispatcher(),
            &Ui::core_dispatcher::messageBuddies,
            this,
            &MessagesModel::messageBuddies,
            Qt::QueuedConnection),

        connect(
            Ui::GetDispatcher(),
            &Ui::core_dispatcher::messageIdsFromServer,
            this,
            &MessagesModel::messageIdsFromServer,
            Qt::QueuedConnection),

        connect(
            Ui::GetDispatcher(),
            &Ui::core_dispatcher::messagesDeleted,
            this,
            &MessagesModel::messagesDeleted,
            Qt::QueuedConnection),

        connect(
            Ui::GetDispatcher(),
            &Ui::core_dispatcher::messagesDeletedUpTo,
            this,
            &MessagesModel::messagesDeletedUpTo,
            Qt::QueuedConnection),

        connect(
            Ui::GetDispatcher(),
            &Ui::core_dispatcher::messagesModified,
            this,
            &MessagesModel::messagesModified,
            Qt::QueuedConnection),

        connect(
            Ui::GetDispatcher(),
            &Ui::core_dispatcher::dlgStates,
            this,
            &MessagesModel::dlgStates,
            Qt::QueuedConnection),

        connect(
            Ui::GetDispatcher(),
            &Ui::core_dispatcher::fileSharingUploadingResult,
            this,
            &MessagesModel::fileSharingUploadingResult,
            Qt::DirectConnection),

            connect(
                Ui::GetDispatcher(),
                &Ui::core_dispatcher::mentionMe,
                this,
                &MessagesModel::mentionMe,
                Qt::QueuedConnection),
        };

        Q_UNUSED(connections);
        assert(std::all_of(connections, connections + sizeof(connections), [](bool res) { return res; }));
    }

    void MessagesModel::dlgStates(const QVector<Data::DlgState>& _states)
    {
        CHECK_THREAD
        for (const auto& _state : _states)
            emit changeDlgState(_state);
    }

    void MessagesModel::fileSharingUploadingResult(
        const QString& _seq, bool /*success*/, const QString& /*localPath*/, const QString& /*uri*/, int /*contentType*/, bool _isFileTooBig)
    {
        CHECK_THREAD
        if (_isFileTooBig)
            failedUploads_ << _seq;
    }

    static void traceBuddies(const Data::MessageBuddies& _buddies, qint64 _seq)
    {
        for (const auto &buddy : _buddies)
        {
            if (!buddy->IsFileSharing())
            {
                continue;
            }

            __TRACE(
                "fs",
                "incoming file sharing message in model\n" <<
                "    seq=<" << _seq << ">\n" <<
                buddy->GetFileSharing()->ToLogString());
        }
    }

    void MessagesModel::messageBuddiesUnloadUnusedMessages(const Data::MessageBuddySptr& _buddiesFirst, const QString& _aimId, bool& _hole)
    {
        CHECK_THREAD
        auto& dialog = getContactDialog(_aimId);
        auto& dialogMessages = dialog.getMessages();

        const auto idFirst = _buddiesFirst->Id_;

        QVector<MessageKey> deletedValues;

        auto iter = dialogMessages.begin();
        while (iter != dialogMessages.end())
        {
            if (iter->first.isPending())
            {
                ++iter;
                continue;
            }

            if (iter->first.getId() < idFirst)
            {
                qDebug() << "unload less" << iter->first.getId() << "    " << iter->second.getBuddy()->GetText();
                deletedValues << iter->first;

                if (iter->first.isDate())
                {
                    dialog.removeDateItem(iter->second.getDate());
                }

                if (iter->first.getId() <= dialog.getLastRequestedMessage())
                {
                    dialog.setLastRequestedMessage(-1);
                }

                iter = dialogMessages.erase(iter);

                continue;
            }

            if (iter->first.isDate() && (iter->second.getDate() == _buddiesFirst->GetDate()))
            {
                deletedValues << iter->first;

                dialog.removeDateItem(iter->second.getDate());

                iter = dialogMessages.erase(iter);

                continue;
            }

            ++iter;
        }

        const MessageKey key = (dialogMessages.empty() ? _buddiesFirst->ToKey() : dialogMessages.cbegin()->first);

        if (dialog.getLastKey() < key)
            dialog.setLastKey(key);

        sequences_.clear();
        dialog.setLastRequestedMessage(-1);

        _hole = !deletedValues.isEmpty();

        if (_hole)
            emitDeleted(deletedValues, _aimId);
    }

    void MessagesModel::messageBuddiesUnloadUnusedMessagesToBottom(const Data::MessageBuddySptr& _buddiesLast, const QString& _aimId, bool& _hole)
    {
        CHECK_THREAD
        auto& dialog = getContactDialog(_aimId);
        auto& dialogMessages = dialog.getMessages();

        const auto idLast = _buddiesLast->Id_;

        QVector<MessageKey> deletedValues;

        auto iter = dialogMessages.begin();
        while (iter != dialogMessages.end())
        {
            if (iter->first.isPending())
            {
                ++iter;
                continue;
            }

            const auto currentId = iter->first.getId();
            if (currentId > idLast)
            {
                qDebug() << "unload greater" << currentId << "    " << iter->second.getBuddy()->GetText();
                deletedValues << iter->first;

                if (iter->first.isDate())
                {
                    dialog.removeDateItem(iter->second.getDate());
                }

                dialog.setRecvLastMessage(false);

                /*if (iter->first.getId() <= dialog.getLastRequestedMessage())
                {
                    dialog.setLastRequestedMessage(-1);
                }*/

                iter = dialogMessages.erase(iter);

                continue;
            }

            if (iter->first.isDate() && (iter->second.getDate() == _buddiesLast->GetDate()))
            {
                deletedValues << iter->first;

                dialog.removeDateItem(iter->second.getDate());

                iter = dialogMessages.erase(iter);

                continue;
            }

            ++iter;
        }


        const MessageKey key = (dialogMessages.empty() ? _buddiesLast->ToKey() : dialogMessages.crbegin()->first);

        if (key < dialog.getFirstKey())
            dialog.setFirstKey(key);

        sequences_.clear();
        dialog.setLastRequestedMessage(-1);

        _hole = !deletedValues.isEmpty();

        if (_hole)
            emitDeleted(deletedValues, _aimId);
    }

    static const char* optionToStr(Ui::MessagesBuddiesOpt option)
    {
        switch (option)
        {
        case Ui::MessagesBuddiesOpt::Requested:
            return "Requested option";
        case Ui::MessagesBuddiesOpt::FromServer:
            return "FromServer option";
        case Ui::MessagesBuddiesOpt::DlgState:
            return "DlgState option";
        case Ui::MessagesBuddiesOpt::Intro:
            return "Intro option";
        case Ui::MessagesBuddiesOpt::Pending:
            return "Pending option";
        case Ui::MessagesBuddiesOpt::Init:
            return "Init option";
        case Ui::MessagesBuddiesOpt::MessageStatus:
            return "MessageStatus option";
        default:
            return "invalid option";
        }
    }

    MessagesModel::UpdatedMessageKeys MessagesModel::messageBuddiesInsertMessages(
        const Data::MessageBuddies& _buddies,
        const QString& _aimId,
        const qint64 _modelFirst,
        const Ui::MessagesBuddiesOpt _option,
        const qint64 _seq,
        const bool _hole,
        int64_t _mess_id,
        int64_t _last_mess_id,
        model_regim _regim,
        bool _containsMessageId)

    {
        CHECK_THREAD
        auto& dialog = getContactDialog(_aimId);
        auto& dialogMessages = dialog.getMessages();

        const bool wasEmptyDialog = dialogMessages.empty();

        const auto modelLastBefore = lastMessageId(dialogMessages);
        const auto modelFirstBefore = !wasEmptyDialog ? dialogMessages.cbegin()->first.getId() : -1;

        if (_regim == model_regim::jump_to_bottom)
        {
            messagesDeletedUpTo(_aimId, -1);
            dialog.setInitMessage(-1);
            dialog.enableJumpToMessage(false);
            dialog.enablePostponeUpdate(false);
            dialog.setLastRequestedMessage(-1);
        }

        const auto messId = dialog.getInitMessage();

        const auto isMultichat = Logic::getContactListModel()->isChat(_aimId);

        std::vector<MessagesMapIter> insertedMessages;

        for (const auto& msg : _buddies)
        {
            const auto key = msg->ToKey();
            const auto it = std::find_if(dialogMessages.begin(), dialogMessages.end(), [&key](const auto& _msg) { return _msg.first == key; });
            const auto messageAlreadyInserted = it != dialogMessages.end();

            if (messageAlreadyInserted)
            {
                auto buddy = it->second.getBuddy();
                if (buddy && !buddy->Quotes_.empty() && !msg->Quotes_.empty())
                {
                    if (buddy->Quotes_.constFirst().chatName_.isEmpty())
                    {
                        QString newHeader(msg->Quotes_.constFirst().chatName_);
                        if (newHeader.isEmpty() && !msg->Quotes_.constFirst().chatId_.isEmpty())
                        {
                            const auto contact = Logic::getContactListModel()->getContactItem(msg->Quotes_.constFirst().chatId_);
                            if (contact)
                                newHeader = contact->Get()->GetDisplayName();
                        }
                        if (!newHeader.isEmpty())
                        {
                            for (auto& q : buddy->Quotes_)
                                q.chatName_ = newHeader;
                        }
                    }
                }
                continue;
            }
            else if (!msg->Quotes_.empty() && msg->Quotes_.constFirst().chatName_.isEmpty() && !msg->Quotes_.constFirst().chatId_.isEmpty())
            {
                QString newHeader;
                const auto contact = Logic::getContactListModel()->getContactItem(msg->Quotes_.constFirst().chatId_);
                if (contact)
                    newHeader = contact->Get()->GetDisplayName();
                if (!newHeader.isEmpty())
                {
                    for (auto& q : msg->Quotes_)
                        q.chatName_ = newHeader;
                }
            }

            Message newMessage(_aimId);
            newMessage.setKey(key);
            newMessage.setChatSender(msg->GetChatSender());
            newMessage.setChatFriendly(msg->ChatFriendly_);
            newMessage.setFileSharing(msg->GetFileSharing());
            newMessage.setSticker(msg->GetSticker());
            newMessage.setChatEvent(msg->GetChatEvent());
            newMessage.setVoipEvent(msg->GetVoipEvent());
            newMessage.setDeleted(msg->IsDeleted());
            newMessage.setBuddy(msg);

            auto insertPos = dialogMessages.emplace(newMessage.getKey(), newMessage);
            if (insertPos.second)
                insertedMessages.push_back(insertPos.first);
        }

        //qDebug("insertedMessages %i", insertedMessages.size());

        QVector<MessageKey> updatedValues = processInsertedMessages(insertedMessages, _aimId, isMultichat);

        updateMessagesMarginsAndAvatars(_aimId);

        bool isReadyEmited = false;
        const auto hasSeq = sequences_.contains(_seq);
        if (_regim == model_regim::first_load && hasSeq && messId == -1) {
            qDebug() << "emit ready1";
            dialog.enableJumpToMessage(false);
            emit ready(_aimId, false /* search */, -1 /* _mess_id */, dialog.getMessageCountAfter(), dialog.getScrollMode());
            isReadyEmited = true;
        }

        const bool isNewMessageRequest = seqAndNewMessageRequest.contains(_seq);
        if (messId > 0 && dialog.isJumpToMessageEnabled())
        {
            dialog.enableJumpToMessage(false);
            dialog.enablePostponeUpdate(false);
            auto resultId = messId;
            if (_containsMessageId)
            {
                resultId = getMessageIdToJump(dialog.getMessages(), messId);
                if (resultId < 0)
                    qDebug() << "fall back: contains MessageId, but can not load required message with id " << messId;
                qDebug() << "emit ready2";
                emit ready(_aimId, true /* search */, resultId, dialog.getMessageCountAfter(), messId == resultId ? dialog.getScrollMode() : scroll_mode_type::to_deleted);
                isReadyEmited = true;
            }
            else
            {
                qDebug() << "fall back: can not load required message with id " << messId;
                scrollToBottom(_aimId); // jump to bottom since there is no the message
            }
        }

        const auto modelLastAfter = lastMessageId(dialogMessages);
        const auto modelFirstAfter = !dialogMessages.empty() ? dialogMessages.cbegin()->first.getId() : -1;

        const bool areSameBounds = !wasEmptyDialog && (modelLastAfter == modelLastBefore) && (modelFirstAfter == modelFirstBefore);

        const bool isSubscribed = subscribed_.contains(_aimId);
        bool needUpdate = false;
        if (_option != Ui::MessagesBuddiesOpt::Requested
            || _regim == model_regim::jump_to_bottom
            || areSameBounds
            || (isNewMessageRequest && !isSubscribed)
            || seqHoleOnTheEdge.contains(_seq))
        {
            needUpdate = true;
            if (!_seq && !isReadyEmited && !updatedValues.isEmpty())
            {
                if (dialog.isPostponeUpdateEnabled() && dialog.isJumpToMessageEnabled())
                    dialog.enableJumpToMessage(false);
                emit newMessageReceived(_aimId);
            }

        }
        else if (isSubscribed)
        {
            subscribed_.removeAll(_aimId);
            emit canFetchMore(_aimId);
        }
        return { std::move(updatedValues) , needUpdate };
    }

    QVector<Logic::MessageKey> MessagesModel::processInsertedMessages(const std::vector<MessagesMapIter>& insertedMessages, const QString& _aimId, bool isMultiChat)
    {
        auto& dialog = getContactDialog(_aimId);
        auto& dialogMessages = dialog.getMessages();
        QVector<MessageKey> updatedValues;
        for (const auto& iter : insertedMessages)
        {
            if (!updatedValues.contains(iter->first))
                updatedValues << iter->first;

            const auto &msgBuddy = iter->second.getBuddy();

            auto prevMsg = previousMessage(dialogMessages, iter);
            const auto havePrevMsg = (prevMsg != dialogMessages.end());

            auto nextMsg = nextMessage(dialogMessages, iter);
            const auto haveNextMsg = (nextMsg != dialogMessages.end());

            bool hasAvatar = iter->first.isVoipEvent() ? (!msgBuddy->IsOutgoingVoip()) : (!iter->first.isOutgoing());

            if (hasAvatar && havePrevMsg)
            {
                hasAvatar = msgBuddy->hasAvatarWith(*prevMsg->second.getBuddy(), isMultiChat);
            }
            msgBuddy->SetHasAvatar(hasAvatar);

            if (haveNextMsg)
            {
                hasAvatar = !nextMsg->first.isOutgoing() && nextMsg->second.getBuddy()->hasAvatarWith(*msgBuddy, isMultiChat);
                if (nextMsg->second.getBuddy()->HasAvatar() != hasAvatar)
                {
                    nextMsg->second.getBuddy()->SetHasAvatar(hasAvatar);
                    emit hasAvatarChanged(nextMsg->first, hasAvatar);
                }
            }

            removeDateItemIfOutdated(_aimId, *msgBuddy);

            const auto dateItemAlreadyExists = dialog.hasDate(msgBuddy->GetDate());
            const auto scheduleForDatesProcessing = (!dateItemAlreadyExists && !msgBuddy->IsDeleted());
            if (scheduleForDatesProcessing)
            {
                auto dateKey = msgBuddy->ToKey();
                dateKey.setType(core::message_type::undefined);
                dateKey.setControlType(control_type::ct_date);

                Message newMessage(_aimId);
                newMessage.setDate(msgBuddy->GetDate());
                newMessage.setKey(dateKey);
                newMessage.setDeleted(msgBuddy->IsDeleted());

                const auto isDateDiffer = (havePrevMsg && (msgBuddy->GetDate() != prevMsg->second.getBuddy()->GetDate()));
                const auto insertDateTablet = (!havePrevMsg || isDateDiffer);

                if (insertDateTablet)
                {
                    newMessage.setBuddy(dialog.addDateItem(_aimId, newMessage.getKey(), newMessage.getDate()));
                    auto insertPos = dialogMessages.emplace(newMessage.getKey(), newMessage);
                    hasAvatar = iter->first.isVoipEvent() ? (!msgBuddy->IsOutgoingVoip()) : (!iter->first.isOutgoing());
                    msgBuddy->SetHasAvatar(hasAvatar);
                    updatedValues << newMessage.getKey();
                }
            }
        }
        return updatedValues;
    }

    static void traceMessageBuddies(const Data::MessageBuddies& _buddies, qint64 _messageId)
    {
        qDebug() << "buddies size " << _buddies.size();
        if (_messageId != -1)
        {
            for (const auto& x : _buddies)
                qDebug() << x->Id_ << " is " << (x->Id_ > _messageId ? "greater" : (x->Id_ == _messageId ? "equal" : "less")) << "  " << x->GetText() << "    prev is " << x->Prev_ << (x->IsDeleted() ? "   deleted" : "");
        }
        else
        {
            for (const auto& x : _buddies)
                qDebug() << x->Id_ << "  " << x->GetText() << "    prev is " << x->Prev_ << (x->IsDeleted() ? "   deleted" : "");
        }
    }

    bool MessagesModel::isIgnoreBuddies(const QString& _aimId, const Data::MessageBuddies& _buddies, Ui::MessagesBuddiesOpt _option, bool _havePending, qint64 _seq, int64_t _last_msgid, model_regim _regim, bool _containsMessageId)
    {
        CHECK_THREAD
        const auto isInit = (_option == Ui::MessagesBuddiesOpt::Init);
        const auto isDlgState = (isInit || _option == Ui::MessagesBuddiesOpt::DlgState || _option == Ui::MessagesBuddiesOpt::MessageStatus);
        const auto isPending = (_option == Ui::MessagesBuddiesOpt::Pending);

        auto& dialog = getContactDialog(_aimId);
        auto& dialogMessages = dialog.getMessages();
        const auto messId = dialog.getInitMessage();

        bool ignoreBuddies = false;
        if (!_buddies.isEmpty() && _regim != model_regim::jump_to_bottom && _seq <= 0)
        {
            if (dialogMessages.empty())
            {
                if (messId > 0)
                {
                    if (_buddies.constFirst()->Id_ > messId && _buddies.constFirst()->Prev_ != -1)
                    {
                        __TRACE(
                            "history",
                            _aimId << " 1 out of range buddie " << _buddies.constFirst()->Id_ << "   " << _buddies.constFirst()->GetText());
                        qDebug() << "1 out of range buddie " << _buddies.constFirst()->Id_ << "   " << _buddies.constFirst()->GetText();
                        ignoreBuddies = true;
                    }
                }
            }
            else
            {
                const auto first = dialogMessages.cbegin()->first.getId();

                if (messId > 0)
                {
                    const auto prevMessage = previousMessageWithId(dialogMessages, dialogMessages.end(), IncludeDeleted::Yes);
                    if (prevMessage != dialogMessages.end())
                    {
                        const auto last = prevMessage->first.getId();
                        if (_buddies.constFirst()->Prev_ > last && !_containsMessageId)
                        {
                            __TRACE(
                                "history",
                                _aimId << " prev mesessage " << last << "   " << prevMessage->second.getBuddy()->GetText() << '\n'
                                    << "2 out of range buddie " << _buddies.constFirst()->Id_ << "   " << _buddies.constFirst()->GetText());
                            qDebug() << "prev mesessage " << last << "   " << prevMessage->second.getBuddy()->GetText();
                            qDebug() << "2 out of range buddie " << _buddies.constFirst()->Id_ << "   " << _buddies.constFirst()->GetText();
                            requestMessages(_aimId, last, RequestDirection::ToNew, Prefetch::No, JumpToBottom::No, FirstRequest::No, Reason::HoleOnTheEdge);
                            ignoreBuddies = true;
                        }
                    }
                }
                else
                {
                    const auto prevMessage = previousMessageWithId(dialogMessages, dialogMessages.end(), IncludeDeleted::Yes);
                    if (prevMessage != dialogMessages.end())
                    {
                        const auto last = prevMessage->first.getId();
                        if (_buddies.constFirst()->Prev_ > last)
                        {
                            __TRACE(
                                "history",
                                _aimId << "prev mesessage " << last << "   " << prevMessage->second.getBuddy()->GetText() << '\n'
                                << "3 out of range buddie " << _buddies.constFirst()->Id_ << "   " << _buddies.constFirst()->GetText());
                            qDebug() << "prev mesessage " << last << "   " << prevMessage->second.getBuddy()->GetText();
                            qDebug() << "3 out of range buddie " << _buddies.constFirst()->Id_ << "   " << _buddies.constFirst()->GetText();
                            requestMessages(_aimId, last, RequestDirection::ToNew, Prefetch::No, JumpToBottom::No, FirstRequest::No, Reason::HoleOnTheEdge);
                            ignoreBuddies = true;
                        }
                    }
                }
            }

            BOOST_SCOPE_EXIT_ALL(&) {
                if (ignoreBuddies)
                {
                    const auto lastId = lastMessageId(dialogMessages);
                    if (lastId > 0 && lastId != _last_msgid)
                        dialog.setRecvLastMessage(false);
                }
            };
        }
        return ignoreBuddies;
    }

    void MessagesModel::messageBuddies(Data::MessageBuddies _buddies, const QString& _aimId, Ui::MessagesBuddiesOpt _option, bool _havePending, qint64 _seq, int64_t _last_msgid)
    {
        CHECK_THREAD
        BOOST_SCOPE_EXIT_ALL(&) {
            seqAndToOlder_.remove(_seq);
            seqAndJumpBottom_.remove(_seq);
            sequences_.removeAll(_seq);
            seqAndNewMessageRequest.remove(_seq);
            seqHoleOnTheEdge.remove(_seq);
            qDebug() << "messageBuddies end -----------------------------------------------";
        };

        qDebug() << "messageBuddies start -----------------------------------------------";
        qDebug() << optionToStr(_option);

        assert(_option > Ui::MessagesBuddiesOpt::Min);
        assert(_option < Ui::MessagesBuddiesOpt::Max);

        auto regim = model_regim::normal_load;
        if (seqAndJumpBottom_.contains(_seq))
            regim = model_regim::jump_to_bottom;

        auto& dialog = getContactDialog(_aimId);
        auto& dialogMessages = dialog.getMessages();
        auto messId = dialog.getInitMessage();

        bool buddiesContainsId = messId > 0 && containsMessageId(_buddies, messId);

        const auto isInit = (_option == Ui::MessagesBuddiesOpt::Init);
        const auto isDlgState = (isInit || _option == Ui::MessagesBuddiesOpt::DlgState || _option == Ui::MessagesBuddiesOpt::MessageStatus || _option == Ui::MessagesBuddiesOpt::Intro);
        const auto isPending = (_option == Ui::MessagesBuddiesOpt::Pending);

        if (build::is_debug())
        {
            //traceMessageBuddies(_buddies, dialog.getInitMessage());
            traceBuddies(_buddies, _seq);
        }

        PendingsCount pendingsCount;

        const bool dialogWasEmpty = dialogMessages.empty();

        if (!_buddies.isEmpty() && (_havePending || isDlgState || isPending))
            pendingsCount = processPendingMessage(_buddies, _aimId, _option);

        if (messId > 0 && dialog.getFirstUnreadMessage() == -1 && dialog.getScrollMode() == scroll_mode_type::unread)
        {
            const auto firstUnread = getMessageIdByPrevId(_buddies, messId);
            // change init message to scroll to first unread
            if (firstUnread != -1)
            {
                qDebug() << "replace init message " << messId << " with " << firstUnread;
                dialog.setFirstUnreadMessage(firstUnread);
                dialog.setInitMessage(firstUnread);
                messId = firstUnread;
                buddiesContainsId = true;
            }
        }

        const auto ignore = (pendingsCount.removed == 0) && isIgnoreBuddies(_aimId, _buddies, _option, _havePending, _seq, _last_msgid, regim, buddiesContainsId);
        if (ignore)
            return;

        qint64 modelFirst = -1;
        if (dialogWasEmpty)
            regim = model_regim::first_load;
        else if (!dialogMessages.empty())
            modelFirst = dialogMessages.cbegin()->first.getPrev();

        int64_t mess_id = -1;

        if (dialogWasEmpty
            && seqAndToOlder_.contains(_seq)
            && seqAndToOlder_[_seq] != -1)
        {
            regim = model_regim::jump_to_msg;
            mess_id = seqAndToOlder_[_seq];
        }

        bool is_bottom_upload = false;
        if (seqAndToOlder_.contains(_seq) && seqAndToOlder_[_seq] != -1)
            is_bottom_upload = true;

        if (!_buddies.isEmpty() && _buddies.constLast()->Id_ == _last_msgid)
            getContactDialog(_aimId).setRecvLastMessage(true);

        if (_buddies.isEmpty())
        {
            if (sequences_.contains(_seq))
                getContactDialog(_aimId).setLastRequestedMessage(-1);

            updateMessagesMarginsAndAvatars(_aimId);

            return;
        }

        if ((!sequences_.contains(_seq) && _buddies.constLast()->Id_ < modelFirst) || !requestedContact_.contains(_aimId))
        {
            updateMessagesMarginsAndAvatars(_aimId);

            return;
        }

        bool hole = false;
        hole |= is_bottom_upload;
        const auto inserted = messageBuddiesInsertMessages(_buddies, _aimId, modelFirst, _option, _seq, hole, mess_id, _last_msgid, regim, buddiesContainsId);
        if (!inserted.keys.isEmpty() && !_buddies.isEmpty())
        {
            const auto newReqIt = seqAndNewMessageRequest.constFind(_seq);
            if (newReqIt != seqAndNewMessageRequest.cend())
            {
                const auto param = newReqIt.value();

                if (messId != -1)
                {
                    const auto hasLess = std::any_of(_buddies.cbegin(), _buddies.cend(), [messId](const auto& x) { return x->Id_ < messId; });
                    const auto hasGreater = std::any_of(_buddies.cbegin(), _buddies.cend(), [messId](const auto& x) { return x->Id_ > messId; });
                    if (hasLess && hasGreater)
                    {
                        bool oldHole = hole;
                        messageBuddiesUnloadUnusedMessages(_buddies.constFirst(), _aimId, hole);
                        messageBuddiesUnloadUnusedMessagesToBottom(_buddies.constLast(), _aimId, hole);
                        hole |= oldHole;
                    }
                    else if (hasLess)
                    {
                        messageBuddiesUnloadUnusedMessages(_buddies.constFirst(), _aimId, hole);
                    }
                    else if (hasGreater)
                    {
                        messageBuddiesUnloadUnusedMessagesToBottom(_buddies.constLast(), _aimId, hole);
                    }
                    if (hole)
                        dialog.setInitMessage(-1);
                }
                else
                {
                    messageBuddiesUnloadUnusedMessages(_buddies.constFirst(), _aimId, hole);
                }
            }
        }
        if (inserted.needUpdate)
            emitUpdated(inserted.keys, _aimId, hole ? HOLE : BASE);

        if (isInit || isDlgState || regim == model_regim::jump_to_msg)
            updateLastSeen(_aimId);
    }

    enum class Position
    {
        top,
        inside,
        bottom,
        outTop,
        outBottom
    };

    static Position idsPositionDialog(const QVector<qint64>& _ids, qint64 messageFirst, qint64 messageLast)
    {
        if (_ids.first() >= messageFirst && _ids.last() <= messageLast)
            return Position::inside;
        if (_ids.first() < messageFirst && _ids.last() >= messageFirst)
            return Position::top;
        if (_ids.first() > messageFirst && _ids.first() < messageLast && _ids.last() > messageLast)
            return Position::bottom;
        if (_ids.last() < messageFirst)
            return Position::outTop;
        return Position::outBottom;
    }

    static qint64 findBottomBound(const QVector<qint64>& _ids, const MessagesMap& _messages)
    {
        for (const auto& pair : boost::adaptors::reverse(_messages))
        {
            const auto id = pair.first.getId();
            if (id > 0 && _ids.first() >= id)
                return id;
        }
        return -1;
    }

    static qint64 findTopBound(const QVector<qint64>& _ids, const MessagesMap& _messages)
    {
        for (const auto& pair : _messages)
        {
            const auto id = pair.first.getId();
            if (id > 0 && id >= _ids.last())
                return id;
        }
        return -1;
    }

    void MessagesModel::requestNewMessages(const QString& _aimId, const QVector<qint64>& _ids, MessagesMap& _dialogMessages)
    {
        CHECK_THREAD
        const auto dialogMessagesEnd = _dialogMessages.end();
        const auto prevMessage = previousMessageWithId(_dialogMessages, dialogMessagesEnd, IncludeDeleted::Yes);
        const auto modelLast = prevMessage != dialogMessagesEnd ? prevMessage->first.getId() : -1;
        const auto position = idsPositionDialog(_ids, _dialogMessages.cbegin()->first.getId(), modelLast);
        const auto idsFirst = _ids.constFirst();
        switch (position)
        {
        case Position::inside:
        {
            requestMessages(_aimId, modelLast, RequestDirection::ToOlder, Prefetch::No, JumpToBottom::No, FirstRequest::No, Reason::ServerUpdate);
        }
        break;
        case Position::top:
        {
            const auto pos = findTopBound(_ids, _dialogMessages);
            requestMessages(_aimId, pos, RequestDirection::ToOlder, Prefetch::No, JumpToBottom::No, FirstRequest::No, Reason::ServerUpdate);
        }
        break;
        case Position::bottom:
        {
            const auto pos = findBottomBound(_ids, _dialogMessages);
            requestMessages(_aimId, pos, RequestDirection::ToNew, Prefetch::No, JumpToBottom::No, FirstRequest::No, Reason::ServerUpdate);
        }
        break;
        case Position::outTop:
        {
            requestMessages(_aimId, _dialogMessages.begin()->first.getId(), RequestDirection::ToOlder, Prefetch::No, JumpToBottom::No, FirstRequest::No, Reason::ServerUpdate);
        }
        break;
        case Position::outBottom:
        {
            requestMessages(_aimId, modelLast, RequestDirection::ToNew, Prefetch::No, JumpToBottom::No, FirstRequest::No, Reason::ServerUpdate);
        }
        break;
        }
    }

    void MessagesModel::messageIdsFromServer(QVector<qint64> _ids, const QString& _aimId, qint64 _seq)
    {
        CHECK_THREAD
        qDebug() << "recieve from server " << _ids.size() << ": " << _ids;

        if (_ids.isEmpty())
            return;

        auto& dialog = getContactDialog(_aimId);
        const auto messageId = dialog.getInitMessage();
        auto& dialogMessages = dialog.getMessages();
        if (dialogMessages.empty())
        {
            if (messageId == -1)
            {
                requestLastNewMessages(_aimId, _ids);
            }
            else
            {
                const auto idx = _ids.indexOf(messageId);
                if (idx != -1)
                {
                    requestMessages(_aimId, messageId, RequestDirection::Bidirection, Prefetch::No, JumpToBottom::No, FirstRequest::No, Reason::ServerUpdate);
                }
                else
                {
                    qDebug() << "fall back. There is no message with messageId in new messages";
                    dialog.setInitMessage(-1);
                    dialog.enableJumpToMessage(false);
                    requestMessages(_aimId, -1, RequestDirection::ToOlder, Prefetch::No, JumpToBottom::No, FirstRequest::No, Reason::ServerUpdate);
                }
            }
        }
        else
        {
            const auto dialogMessagesEnd = dialogMessages.end();
            const auto firstMessage = firstMessageWithId(dialogMessages, dialogMessages.begin());
            if (firstMessage != dialogMessagesEnd && dialogMessages.size() > moreCount())
            {
                const auto first = firstMessage->first.getId();
                const auto firstPrev = firstMessage->first.getPrev();
                _ids.erase(std::remove_if(_ids.begin(), _ids.end(), [first, firstPrev](auto id) {
                    if (firstPrev > 0 && firstPrev == id)
                        return false;
                    return id < first;
                }), _ids.end());
                if (_ids.isEmpty())
                    return;
            }

            if (messageId == -1)
            {
                requestNewMessages(_aimId, _ids, dialogMessages);
            }
            else
            {
                const auto idsFirst = _ids.constFirst();
                if (messageId == idsFirst)
                {
                    requestMessages(_aimId, idsFirst, RequestDirection::ToNew, Prefetch::No, JumpToBottom::No, FirstRequest::No, Reason::ServerUpdate);
                }
                else if (messageId == _ids.constLast())
                {
                    requestLastNewMessages(_aimId, _ids);
                }
                else if (_ids.contains(messageId))
                {
                    requestMessages(_aimId, messageId, RequestDirection::Bidirection, Prefetch::No, JumpToBottom::No, FirstRequest::No, Reason::ServerUpdate);
                }
                else
                {
                    const auto prevMessage = previousMessageWithId(dialogMessages, dialogMessagesEnd, IncludeDeleted::Yes);
                    if (prevMessage != dialogMessagesEnd)
                    {
                        const auto last = prevMessage->first.getId();
                        if (std::any_of(_ids.cbegin(), _ids.cend(), [last, messageId](qint64 id) { return id > messageId && id < last; })) // hole in dialog
                        {
                            requestMessages(_aimId, messageId, RequestDirection::ToNew, Prefetch::No, JumpToBottom::No, FirstRequest::No, Reason::ServerUpdate);
                            return;
                        }
                    }

                    if (firstMessage != dialogMessagesEnd)
                    {
                        const auto first = firstMessage->first.getId();
                        if (std::any_of(_ids.cbegin(), _ids.cend(), [first, messageId](qint64 id) { return id < messageId && id >= first; })) // hole in dialog
                            requestMessages(_aimId, messageId, RequestDirection::ToOlder, Prefetch::No, JumpToBottom::No, FirstRequest::No, Reason::ServerUpdate);
                        else
                            requestNewMessages(_aimId, _ids, dialogMessages);
                    }
                }
            }
        }
    }

    void MessagesModel::updateLastSeen(const QString& _aimid)
    {
        CHECK_THREAD
        auto dlgState = Logic::getRecentsModel()->getDlgState(_aimid);
        if (dlgState.AimId_ != _aimid)
            dlgState = Logic::getUnknownsModel()->getDlgState(_aimid);

        emit changeDlgState(dlgState);
    }

    template<typename T>
    static T firstNonDeleted(T first, T last)
    {
        while (first != last)
        {
            if (!(*first).second.isDeleted())
                return first;
            ++first;
        }
        return last;
    }

    qint64 MessagesModel::getMessageIdToJump(const MessagesMap& _messages, qint64 _id)
    {
        CHECK_THREAD
        const auto f_end = _messages.end();
        const auto f_id_it = std::find_if(_messages.begin(), f_end, [_id](const auto& x) { return x.first.getId() == _id; });

        const auto f_it = firstNonDeleted(f_id_it, f_end);
        if (f_it != f_end)
            return (*f_it).first.getId();

        const auto r_end = _messages.rend();
        const auto r_it = firstNonDeleted(std::make_reverse_iterator(f_id_it), r_end);
        if (r_it != r_end)
            return (*r_it).first.getId();

        return -1;
    }

    bool MessagesModel::containsMessageId(const MessagesMap& _messages, qint64 _id)
    {
        CHECK_THREAD
        return std::any_of(_messages.begin(), _messages.end(), [_id](const auto& x) { return x.first.getId() == _id; });
    }

    bool MessagesModel::containsMessageId(const Data::MessageBuddies& _messages, qint64 _id)
    {
        CHECK_THREAD
        return std::any_of(_messages.begin(), _messages.end(), [_id](const auto& x) { return x->Id_ == _id; });
    }

    qint64 MessagesModel::getMessageIdByPrevId(const Data::MessageBuddies& _messages, qint64 _prevId)
    {
        CHECK_THREAD
        const auto end = _messages.end();
        const auto it = std::find_if(_messages.begin(), end, [_prevId](const auto& x) { return x->Prev_ == _prevId; });
        return it != end ? (*it)->Id_ : -1;
    }

    void MessagesModel::messagesDeleted(const QString& _aimId, const QVector<int64_t>& _deletedIds)
    {
        CHECK_THREAD
        assert(!_aimId.isEmpty());
        assert(!_deletedIds.isEmpty());
        auto& dialog = getContactDialog(_aimId);
        auto &dialogMessages = dialog.getMessages();

        QVector<Logic::MessageKey> messageKeys;
        messageKeys.reserve(_deletedIds.size());
        for (const auto id : _deletedIds)
        {
            assert(id > 0);

            MessageKey newKey;
            newKey.setId(id);


            // find the deleted index record

            auto msgsRecIter = dialogMessages.find(newKey);
            if (msgsRecIter == dialogMessages.end())
            {
                continue;
            }

            msgsRecIter->second.setDeleted(true);

            auto key = msgsRecIter->first;

            messageKeys.push_back(key);
        }

        if (!messageKeys.empty())
        {
            emitDeleted(messageKeys, _aimId);
        }

        updateMessagesMarginsAndAvatars(_aimId);

        updateDateItems(_aimId);

        if (const auto newKey = dialog.getNewKey())
            updateNew(_aimId, newKey->getId());
    }

    void MessagesModel::messagesDeletedUpTo(const QString& _aimId, int64_t _id)
    {
        CHECK_THREAD
        assert(!_aimId.isEmpty());

        auto &dialog = getContactDialog(_aimId);
        auto &dialogMessages = dialog.getMessages();

        QVector<Logic::MessageKey> messageKeys;

        auto iter = dialogMessages.cbegin();
        for (; iter != dialogMessages.cend(); ++iter)
        {
            const auto currentId = iter->first.getId();
            if (currentId > _id && _id != -1)
            {
                break;
            }

            const Logic::MessageKey key(currentId, -1, QString(), -1, 0, core::message_type::base, false, Logic::preview_type::none, Logic::control_type::ct_message);

            if (currentId <= dialog.getLastRequestedMessage())
                dialog.setLastRequestedMessage(-1);

            messageKeys.push_back(key);
        }

        emitDeleted(messageKeys, _aimId);

        dialogMessages.erase(dialogMessages.cbegin(), iter);

        updateDateItems(_aimId);

        if (dialogMessages.empty())
            dialog.deleteLastKey();
        else
            dialog.setLastKey(dialogMessages.cbegin()->first);
    }

    void MessagesModel::setRecvLastMsg(const QString& _aimId, bool _value)
    {
        CHECK_THREAD
        getContactDialog(_aimId).setRecvLastMessage(_value);
    }

    void MessagesModel::messagesModified(const QString& _aimId, const Data::MessageBuddies& _modifications)
    {
        CHECK_THREAD
        assert(!_aimId.isEmpty());
        assert(!_modifications.isEmpty());

        QVector<Logic::MessageKey> messageKeys;

        for (const auto &modification : _modifications)
        {
            if (!modification->HasText() &&
                !modification->IsChatEvent())
            {
                assert(!"unexpected modification");
                continue;
            }

            const auto key = applyMessageModification(_aimId, *modification);
            if (!key.hasId())
            {
                continue;
            }

            messageKeys.push_back(key);

            __INFO(
                "delete_history",
                "sending update request to the history page\n"
                "    message-id=<" << key.getId() << ">");
        }

        updateMessagesMarginsAndAvatars(_aimId);

        emitUpdated(messageKeys, _aimId, MODIFIED);
    }

    void MessagesModel::addMentionMe(const QString& _contact, Data::MessageBuddySptr _mention)
    {
        CHECK_THREAD
        auto iter_contact = mentionsMe_.find(_contact);
        if (iter_contact != mentionsMe_.end())
        {
            const qint64 messageId = _mention->Id_;


            const auto& mentions = iter_contact->second;

            const auto found = std::any_of(mentions.begin(), mentions.end(), [messageId](const Data::MessageBuddySptr& _mention){
                return (_mention->Id_ == messageId); });

            if (found)
                return;
        }

        mentionsMe_[_contact].push_back(_mention);
    }

    void MessagesModel::addMentionMe2Pending(const QString& _contact, Data::MessageBuddySptr _mention)
    {
        CHECK_THREAD
        auto iter_contact = pendingMentionsMe_.find(_contact);
        if (iter_contact != pendingMentionsMe_.end())
        {
            const qint64 messageId = _mention->Id_;

            const auto& mentions = iter_contact->second;

            const auto found = std::any_of(mentions.begin(), mentions.end(), [messageId](const Data::MessageBuddySptr& _mention) {
                return (_mention->Id_ == messageId); });

            if (found)
                return;
        }

        pendingMentionsMe_[_contact].push_back(_mention);
    }

    void MessagesModel::removeMentionMe(const QString& _contact, Data::MessageBuddySptr _mention)
    {
        CHECK_THREAD
        auto iter_contact = mentionsMe_.find(_contact);
        if (iter_contact != mentionsMe_.end())
        {
            auto& mentions = iter_contact->second;

            const qint64 messageId = _mention->Id_;

            mentions.erase(std::remove_if(
                mentions.begin(), mentions.end(), [messageId](const Data::MessageBuddySptr& _mention)
                {
                    return (_mention->Id_ == messageId);
                }), mentions.end());

            if (mentions.empty())
                mentionsMe_.erase(iter_contact);
        }
    }

    int MessagesModel::getMentionsCount(const QString& _contact) const
    {
        CHECK_THREAD
        auto iter_contact = mentionsMe_.find(_contact);
        if (iter_contact == mentionsMe_.end())
            return 0;

        return iter_contact->second.size();
    }

    Data::MessageBuddySptr MessagesModel::getNextUnreadMention(const QString& _contact) const
    {
        CHECK_THREAD
        auto iter_contact = mentionsMe_.find(_contact);
        if (iter_contact == mentionsMe_.end())
            return nullptr;

        const auto& mentions = iter_contact->second;

        if (mentions.empty())
            return nullptr;

        return mentions.front();
    }

    void MessagesModel::mentionMe(const QString _contact, Data::MessageBuddySptr _mention)
    {
        CHECK_THREAD
        auto dlgState = Logic::getRecentsModel()->getDlgState(_contact);
        if (dlgState.YoursLastRead_ >= _mention->Id_)
            return;

        addMentionMe(_contact, _mention);
    }

    void MessagesModel::onMessageItemRead(const QString& _contact, const qint64 _messageId, const bool _visible)
    {
        CHECK_THREAD
        auto iter_contact = pendingMentionsMe_.find(_contact);
        if (iter_contact != pendingMentionsMe_.end())
        {
            auto& mentions = iter_contact->second;

            auto iter_mention = std::find_if(mentions.begin(), mentions.end(), [_messageId](const Data::MessageBuddySptr& _mention)->bool {
                    return (_mention->Id_ == _messageId); });

            if (iter_mention != mentions.end())
            {
                if (_visible)
                    return;

                addMentionMe(_contact, *iter_mention);

                mentions.erase(iter_mention);

                if (mentions.empty())
                    pendingMentionsMe_.erase(iter_contact);

                return;
            }
        }

        if (_visible)
        {
            iter_contact = mentionsMe_.find(_contact);
            if (iter_contact == mentionsMe_.end())
                return;

            auto& mentions = iter_contact->second;

            auto iter_mention = std::find_if(mentions.begin(), mentions.end(), [_messageId](const Data::MessageBuddySptr& _mention)->bool {
                return (_mention->Id_ == _messageId); });

            if (iter_mention != mentions.end())
            {
                addMentionMe2Pending(_contact, *iter_mention);

                mentions.erase(iter_mention);
                if (mentions.empty())
                    mentionsMe_.erase(iter_contact);

                QTimer::singleShot(500, this, [this, _contact, _messageId]()
                {
                    auto iter_contact = pendingMentionsMe_.find(_contact);
                    if (iter_contact != pendingMentionsMe_.end())
                    {
                        auto& mentions = iter_contact->second;

                        auto iter_mention = std::find_if(mentions.begin(), mentions.end(), [_messageId](const Data::MessageBuddySptr& _mention){
                            return (_mention->Id_ == _messageId); });

                        if (iter_mention != mentions.end())
                        {
                            mentions.erase(iter_mention);

                            if (mentions.empty())
                                pendingMentionsMe_.erase(iter_contact);

                            emit mentionRead(_contact, _messageId);
                        }
                    }
                });
            }
        }
    }

    MessagesModel::PendingsCount MessagesModel::processPendingMessage(InOut Data::MessageBuddies& _msgs, const QString& _aimId, const Ui::MessagesBuddiesOpt _state)
    {
        CHECK_THREAD
        assert(_state > Ui::MessagesBuddiesOpt::Min);
        assert(_state < Ui::MessagesBuddiesOpt::Max);

        PendingsCount result;
        if (_msgs.isEmpty())
            return result;

        QVector<MessageKey> updatedValues;

        auto& dialog = getContactDialog(_aimId);
        auto& pendingMessages = dialog.getPendingMessages();
        auto& dialogMessages = dialog.getMessages();

        for (auto iter = _msgs.begin(); iter != _msgs.end();)
        {
            auto msg = *iter;

            const MessageKey key = msg->ToKey();
            if (msg->IsPending())
            {
                msg->SetHasAvatar(!msg->IsOutgoing());

                Message newMessage(_aimId);
                newMessage.setKey(key);
                newMessage.setChatSender(msg->GetChatSender());
                newMessage.setChatFriendly(msg->ChatFriendly_);
                newMessage.setFileSharing(msg->GetFileSharing());
                newMessage.setSticker(msg->GetSticker());
                newMessage.setChatEvent(msg->GetChatEvent());
                newMessage.setVoipEvent(msg->GetVoipEvent());
                newMessage.setDeleted(msg->IsDeleted());
                newMessage.setBuddy(msg);

                iter = _msgs.erase(iter);

                pendingMessages.emplace(newMessage.getKey(), newMessage);
                dialogMessages.emplace(newMessage.getKey(), newMessage);

                ++result.added;

                updatedValues << newMessage.getKey();
            }
            else if (key.isOutgoing())
            {
                auto isKeyEqual = [&key](const std::pair<MessageKey, Message>& _pair) { return key == _pair.first; };
                auto exist_pending = std::find_if(pendingMessages.begin(), pendingMessages.end(), isKeyEqual);

                if (exist_pending != pendingMessages.end())
                {
                    assert(key.hasId());
                    assert(!key.getInternalId().isEmpty());
                    emit messageIdFetched(_aimId, key);

                    auto exist_message = std::find_if(dialogMessages.begin(), dialogMessages.end(), isKeyEqual);

                    if (exist_message != dialogMessages.end())
                    {
                        dialogMessages.erase(exist_message);
                    }

                    ++result.removed;
                    pendingMessages.erase(exist_pending);
                }

                ++iter;
            }
            else
            {
                ++iter;
            }
        }

        if (_state != Ui::MessagesBuddiesOpt::Requested && !updatedValues.isEmpty())
            emitUpdated(updatedValues, _aimId, BASE);

        return result;
    }

    Data::MessageBuddy MessagesModel::item(const Message& _message)
    {
        CHECK_THREAD
        Data::MessageBuddy result;

        result.Id_ = _message.getKey().getId();
        result.Prev_ = _message.getKey().getPrev();
        result.InternalId_ = _message.getKey().getInternalId();

        auto& dialog = getContactDialog(_message.getAimId());

        std::shared_ptr<Data::MessageBuddy> buddy;

        const auto &dialogMessages = dialog.getMessages();
        auto iterMsg = dialogMessages.find(_message.getKey());

        if (iterMsg != dialogMessages.end())
        {
            buddy = iterMsg->second.getBuddy();
        }
        else
        {
            const auto &pendingMessages = dialog.getPendingMessages();
            auto itPending = pendingMessages.find(_message.getKey());
            if (itPending != pendingMessages.end())
                buddy = itPending->second.getBuddy();
        }

        if (buddy)
        {
            result.SetType(_message.getKey().getType());
            result.AimId_ = _message.getAimId();
            result.SetOutgoing(buddy->IsOutgoing());
            result.Chat_ = buddy->Chat_;
            result.SetChatSender(buddy->GetChatSender());
            result.ChatFriendly_ = buddy->ChatFriendly_;
            result.SetHasAvatar(buddy->HasAvatar());
            result.SetIndentBefore(buddy->GetIndentBefore());

            result.FillFrom(*buddy);

            result.SetTime(buddy->GetTime());

            result.SetFileSharing(_message.getFileSharing());
            result.SetSticker(_message.getSticker());
            result.SetChatEvent(_message.getChatEvent());
            result.SetVoipEvent(_message.getVoipEvent());
            result.Quotes_ = _message.getBuddy()->Quotes_;
            result.Mentions_ = _message.getBuddy()->Mentions_;

            if (_message.getKey().getId() == -1)
            {
                result.Id_ = -1;
            }
        }
        else
        {
            result.Id_ = -1;
        }

        return result;
    }

    void MessagesModel::removeDateItemIfOutdated(const QString& _aimId, const Data::MessageBuddy& _msg)
    {
        CHECK_THREAD
        assert(_msg.HasId());

        const auto msgKey = _msg.ToKey();

        const auto &date = _msg.GetDate();

        auto& dialog = getContactDialog(_aimId);

        auto& dateItems = dialog.getDatesMap();

        auto dateItemIter = dateItems.find(date);
        if (dateItemIter == dateItems.end())
        {
            return;
        }

        auto &indexRecord = dateItemIter->second;

        const auto isOutdated = (msgKey < indexRecord.getKey());
        if (!isOutdated)
        {
            return;
        }

        dialog.getMessages().erase(indexRecord.getKey());

        emitDeleted({ indexRecord.getKey() }, _aimId);

        dateItems.erase(dateItemIter);
    }

    void MessagesModel::emitUpdated(const QVector<Logic::MessageKey>& _list, const QString& _aimId, unsigned _mode)
    {
        CHECK_THREAD
        if (_list.isEmpty())
            return;

        const bool containsChatEvent = std::any_of(_list.begin(), _list.end(), [](const auto& key) { return key.isChatEvent(); });
        if (containsChatEvent && Logic::getContactListModel()->isChat(_aimId))
            emit chatEvent(_aimId);

        if (_list.size() <= moreCount())
        {
            emit updated(_list, _aimId, _mode);
            return;
        }

        int i = 0;
        QVector<Logic::MessageKey> updatedList;
        for (const auto& iter : _list)
        {
            if (++i < moreCount())
            {
                updatedList.push_back(iter);
                continue;
            }

            updatedList.push_back(iter);

            i = 0;
            emit updated(updatedList, _aimId, _mode);
            updatedList.clear();
        }

        if (!updatedList.isEmpty())
            emit updated(updatedList, _aimId, _mode);
    }

    void MessagesModel::emitDeleted(const QVector<Logic::MessageKey>& _list, const QString& _aimId)
    {
        CHECK_THREAD
        emit deleted(_list, _aimId);
    }

    void MessagesModel::createFileSharingWidget(Ui::MessageItem& _messageItem, const Data::MessageBuddy& _messageBuddy) const
    {
        CHECK_THREAD
        auto parent = &_messageItem;

        std::unique_ptr<HistoryControl::MessageContentWidget> item;

        const auto previewsEnabled = Ui::get_gui_settings()->get_value<bool>(settings_show_video_and_images, true);


        item = std::make_unique<HistoryControl::FileSharingWidget>(
            parent,
            _messageBuddy.IsOutgoing(),
            _messageBuddy.AimId_,
            _messageBuddy.GetFileSharing(),
            previewsEnabled
        );

        item->setFixedWidth(itemWidth_);

        connect(
            item.get(),
            &HistoryControl::MessageContentWidget::removeMe,
            [_messageBuddy]
            {
                emit GetMessagesModel()->deleted({ _messageBuddy.ToKey() }, _messageBuddy.AimId_);
            });

        _messageItem.setContentWidget(item.release());
    }

    MessageKey MessagesModel::applyMessageModification(const QString& _aimId, const Data::MessageBuddy& _modification)
    {
        CHECK_THREAD
        assert(_modification.AimId_ == _aimId);

        auto &dialogMessages = getContactDialog(_aimId).getMessages();

        auto key = _modification.ToKey();
        assert(key.hasId());

        // find index

        const auto existingIndexIter = findIndexRecord(dialogMessages, key);
        const auto isIndexMissing = (existingIndexIter == dialogMessages.end());
        if (isIndexMissing)
        {
            __INFO(
                "delete_history",
                "modification patch skipped\n"
                "    reason=<no-index>\n"
                "    message-id=<" << key.getId() << ">");

            return MessageKey();
        }

        // apply modification to the index record

        auto existingIndex = existingIndexIter->second;
        assert(!existingIndex.isDeleted());

        existingIndex.applyModification(_modification);
        existingIndex.getBuddy()->ApplyModification(_modification);
        key = existingIndex.getKey();

        auto insertionIter = existingIndexIter;
        ++insertionIter;

        dialogMessages.erase(existingIndexIter);

        dialogMessages.emplace_hint(insertionIter, std::make_pair(key, existingIndex));

        return key;
    }

    bool MessagesModel::hasItemsInBetween(const QString& _aimId, const Message& _l, const Message& _r) const
    {
        CHECK_THREAD
        assert(!_aimId.isEmpty());

        const ContactDialog* dialog = getContactDialogConst(_aimId);
        if (!dialog)
            return false;

        return dialog->hasItemsInBetween(_l.getKey(), _r.getKey());
    }

    void MessagesModel::updateDateItems(const QString& _aimId)
    {
        CHECK_THREAD
        assert(!_aimId.isEmpty());

        auto &dateRecords = getContactDialog(_aimId).getDatesMap();

        QVector<MessageKey> toRemove;
        toRemove.reserve(dateRecords.size());

        for(
            auto dateRecordsIter = dateRecords.cbegin();
            dateRecordsIter != dateRecords.cend();
        )
        {
            const auto &dateIndexRecord = dateRecordsIter->second;

            Message nextDateIndexRecord(_aimId);

            auto nextDateRecordsIter = dateRecordsIter;
            ++nextDateRecordsIter;

            const auto isLastDateRecord = (nextDateRecordsIter == dateRecords.cend());
            if (isLastDateRecord)
            {
                nextDateIndexRecord.setKey(MessageKey::MAX);
            }
            else
            {
                nextDateIndexRecord = nextDateRecordsIter->second;
            }

            const auto hasItemsBetweenDates = hasItemsInBetween(_aimId, dateIndexRecord, nextDateIndexRecord);
            if (!hasItemsBetweenDates)
            {
                toRemove << dateIndexRecord.getKey();
                dateRecords.erase(dateRecordsIter);
            }

            dateRecordsIter = nextDateRecordsIter;
        }

        emitDeleted(toRemove, _aimId);
    }

    void MessagesModel::updateMessagesMarginsAndAvatars(const QString& _aimId)
    {
        CHECK_THREAD
        assert(!_aimId.isEmpty());

        const auto isMultichat = Logic::getContactListModel()->isChat(_aimId);

        const auto &dialogMessages = getContactDialog(_aimId).getMessages();

        auto messagesIter = dialogMessages.crbegin();

        for(;;)
        {
            for (;;)
            {
                const auto isFirstElementReached = (messagesIter == dialogMessages.crend());
                if (isFirstElementReached)
                {
                    return;
                }

                if (!messagesIter->second.getBuddy()->IsDeleted())
                {
                    break;
                }

                ++messagesIter;
            }

            auto &message = *messagesIter->second.getBuddy();

            const auto &messageKey = messagesIter->first;

            for (;;)
            {
                ++messagesIter;

                const auto isFirstElementReached = (messagesIter == dialogMessages.crend());
                if (isFirstElementReached)
                {
                    if (message.GetIndentBefore())
                    {
                        message.SetIndentBefore(false);
                        emit indentChanged(messageKey, false);
                    }

                    const auto hasAvatar = (message.IsVoipEvent() ? !message.IsOutgoingVoip() : !message.IsOutgoing());
                    if (hasAvatar != message.HasAvatar())
                    {
                        message.SetHasAvatar(hasAvatar);
                        emit hasAvatarChanged(messageKey, hasAvatar);
                    }

                    return;
                }

                if (!messagesIter->second.getBuddy()->IsDeleted())
                {
                    break;
                }
            }

            const auto &prevMessage = *messagesIter->second.getBuddy();

            const auto oldMessageIndent = message.GetIndentBefore();
            const auto newMessageIndent = message.GetIndentWith(prevMessage);

            if (newMessageIndent != oldMessageIndent)
            {
                message.SetIndentBefore(newMessageIndent);
                emit indentChanged(messageKey, newMessageIndent);
            }

            auto hasAvatar = (message.IsVoipEvent() ? !message.IsOutgoingVoip() : !message.IsOutgoing());
            if (hasAvatar)
            {
                hasAvatar = message.hasAvatarWith(prevMessage, isMultichat);
            }

            if (hasAvatar != message.HasAvatar())
            {
                message.SetHasAvatar(hasAvatar);
                emit hasAvatarChanged(messageKey, hasAvatar);
            }
        }
    }

    MessagesMapIter MessagesModel::findIndexRecord(MessagesMap& _indexRecords, const Logic::MessageKey& _key) const
    {
        CHECK_THREAD
        assert(!_key.isEmpty());

        return std::find_if(_indexRecords.begin(), _indexRecords.end(), [&_key](const std::pair<MessageKey, Message>& _pair)
        {
            return (_pair.first == _key);
        });
    }

    Logic::MessageKey MessagesModel::findFirstKeyAfter(const QString& _aimId, const Logic::MessageKey& _key) const
    {
        CHECK_THREAD
        assert(!_aimId.isEmpty());
        assert(!_key.isEmpty());

        const ContactDialog* dialog = getContactDialogConst(_aimId);
        if (!dialog)
        {
            return Logic::MessageKey();
        }

        return dialog->findFirstKeyAfter(_key);
    }

    Logic::MessageKey MessagesModel::getKey(const QString& _aimId, qint64 _id) const
    {
        CHECK_THREAD
        assert(!_aimId.isEmpty());
        const ContactDialog* dialog = getContactDialogConst(_aimId);
        if (!dialog)
            return Logic::MessageKey();
        return dialog->getKey(_id);
    }

    void MessagesModel::requestLastNewMessages(const QString& _aimId, const QVector<qint64>& _ids)
    {
        CHECK_THREAD
        if (_ids.size() <= Data::PRELOAD_MESSAGES_COUNT)
        {
            requestMessages(_aimId, _ids.first(), RequestDirection::ToNew, Prefetch::No, JumpToBottom::No, FirstRequest::No, Reason::ServerUpdate);
        }
        else
        {
            const auto id = *std::next(_ids.rbegin(), Data::PRELOAD_MESSAGES_COUNT - 1);
            requestMessages(_aimId, id, RequestDirection::ToNew, Prefetch::No, JumpToBottom::No, FirstRequest::No, Reason::ServerUpdate);
        }
    }

    void MessagesModel::requestMessages(const QString& _aimId, qint64 _messageId, RequestDirection _direction, Prefetch _prefetch, JumpToBottom _jump_to_bottom, FirstRequest _firstRequest, Reason _reason)
    {
        CHECK_THREAD
        //qDebug("request message : %lld %i %i", _messageId, _toOlder, _needPrefetch);
        assert(!_aimId.isEmpty());
        assert(_direction != RequestDirection::Bidirection || _messageId > 0);

        if (!requestedContact_.contains(_aimId))
            requestedContact_.append(_aimId);

        auto& dialog = getContactDialog(_aimId);
        auto& dialogMessages = dialog.getMessages();

        qint64 lastId = dialogMessages.empty() ? -1 : dialogMessages.cbegin()->second.getBuddy()->Id_;
        qint64 lastPrev = dialogMessages.empty() ? -1 : dialogMessages.cbegin()->second.getBuddy()->Prev_;

        if (_jump_to_bottom == JumpToBottom::Yes)
        {
            auto dlgState = Logic::getRecentsModel()->getDlgState(_aimId);
            if (dlgState.AimId_ != _aimId)
                dlgState = Logic::getUnknownsModel()->getDlgState(_aimId);
            if (dlgState.LastMsgId_ > 0 && containsMessageId(dialogMessages, dlgState.LastMsgId_))
            {
                QVector<MessageKey> updatedValues;
                updatedValues.reserve(int(dialogMessages.size()));
                for (const auto& x : dialogMessages)
                    updatedValues.push_back(x.first);
                emitUpdated(updatedValues, _aimId, HOLE);
                updateLastSeen(_aimId);
                return;
            }

            lastId = -1;
            _direction = RequestDirection::ToOlder;
        }
        else
        {
            if (_direction == RequestDirection::ToNew)
            {
                lastId = dialogMessages.empty() ? -1 : dialogMessages.crbegin()->second.getBuddy()->Id_;
            }
            else if (_reason == Reason::Plain && !dialog.isLastRequestedMessageEmpty() && dialog.getLastRequestedMessage() == lastId && dialogMessages.size() > moreCount() && _direction != RequestDirection::Bidirection)
            {
                return;
            }
        }

        const auto requestedId = _messageId == -1 ? lastId : _messageId;

        if (requestedId != -1 && lastPrev == -1 && _direction == RequestDirection::ToOlder && _reason == Reason::Plain)
            return;

        const auto countEarly = (_direction == RequestDirection::ToOlder || _direction == RequestDirection::Bidirection) ? Data::PRELOAD_MESSAGES_COUNT : 0;
        const auto countLater = (_direction == RequestDirection::ToNew || _direction == RequestDirection::Bidirection) ? Data::PRELOAD_MESSAGES_COUNT : 0;

        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("contact", _aimId);
        collection.set_value_as_int64("from", requestedId);
        collection.set_value_as_int64("count_early", countEarly);
        collection.set_value_as_int64("count_later", countLater);
        collection.set_value_as_bool("need_prefetch", _prefetch == Prefetch::Yes);
        collection.set_value_as_int64("last_read_msg", -1);
        collection.set_value_as_bool("is_first_request", _firstRequest == FirstRequest::Yes);

        qDebug() << qsl("request from %1. count_early: %2; count_later: %3; firstRequest: %4; isNew: %5").arg(requestedId).arg(countEarly).arg(countLater).arg(_firstRequest == FirstRequest::Yes).arg(_reason == Reason::ServerUpdate);

        auto seq = Ui::GetDispatcher()->post_message_to_core(qsl("archive/messages/get"), collection.get());
        sequences_ << seq;

        if (_jump_to_bottom == JumpToBottom::Yes)
        {
            seqAndJumpBottom_.insert(seq, _messageId);
        }
        else
        {
            seqAndToOlder_.insert(seq, _messageId);
        }

        if (_reason == Reason::ServerUpdate)
            seqAndNewMessageRequest.insert(seq, { _messageId , _direction });
        else if (_reason == Reason::HoleOnTheEdge)
            seqHoleOnTheEdge.insert(seq);

        if (_direction == RequestDirection::ToOlder)
            dialog.setLastRequestedMessage(lastId);
    }

    Ui::HistoryControlPageItem* MessagesModel::makePageItem(const Data::MessageBuddy& _msg, QWidget* _parent) const
    {
        CHECK_THREAD
        if (_msg.IsEmpty())
            return nullptr;

        const auto isServiceMessage = (!_msg.IsBase() && !_msg.IsFileSharing() && !_msg.IsSticker() && !_msg.IsChatEvent() && !_msg.IsVoipEvent());
        if (isServiceMessage)
        {
            auto serviceMessageItem = std::make_unique<Ui::ServiceMessageItem>(_parent);
            serviceMessageItem->setDate(_msg.GetDate());
            serviceMessageItem->setWidth(itemWidth_);
            serviceMessageItem->setContact(_msg.AimId_);
            serviceMessageItem->updateStyle();
            serviceMessageItem->setDeleted(_msg.IsDeleted());

            return serviceMessageItem.release();
        }

        if (_msg.IsChatEvent())
        {
            auto item = std::make_unique<Ui::ChatEventItem>(_parent, _msg.GetChatEvent(), _msg.Id_);
            item->setContact(_msg.AimId_);
            item->setHasAvatar(_msg.HasAvatar());
            item->setFixedWidth(itemWidth_);
            item->setDeleted(_msg.IsDeleted());
            return item.release();
        }

        if (_msg.IsVoipEvent())
        {
            const auto &voipEvent = _msg.GetVoipEvent();

            auto item = std::make_unique<Ui::VoipEventItem>(_parent, voipEvent);
            item->setTopMargin(_msg.GetIndentBefore());
            item->setFixedWidth(itemWidth_);
            item->setHasAvatar(_msg.HasAvatar());
            item->setId(_msg.Id_);
            item->setDeleted(_msg.IsDeleted());
            return item.release();
        }

        if (_msg.IsDeleted())
        {
            auto deletedItem = std::make_unique<Ui::DeletedMessageItem>(_parent);
            deletedItem->setDeleted(true);
            return deletedItem.release();
        }

        const auto sender =
            (_msg.Chat_ && _msg.HasChatSender()) ?
                NormalizeAimId(_msg.GetChatSender()) :
                _msg.AimId_;


        const bool previewsEnabled = Ui::get_gui_settings()->get_value<bool>(settings_show_video_and_images, true);
        const bool isSitePreview = ((previewsEnabled && (_msg.GetPreviewableLinkType() == preview_type::site)));
        const bool is_not_auth = (!_msg.Chat_ && Logic::getContactListModel()->isNotAuth(_msg.AimId_));

        if (isSitePreview || !_msg.Quotes_.isEmpty() || _msg.IsSticker() || _msg.ContainsPttAudio())
        {
            QString senderFriendly;

            if (_msg.IsOutgoing())
            {
                senderFriendly = Ui::MyInfo()->friendlyName();
            }
            else if (_msg.Chat_)
            {
                senderFriendly = GetChatFriendly(_msg.GetChatSender(), _msg.ChatFriendly_);
            }
            else
            {
                senderFriendly = Logic::getContactListModel()->getDisplayName(_msg.AimId_);
            }

            auto item =
                Ui::ComplexMessage::ComplexMessageItemBuilder::makeComplexItem(
                    _parent,
                    _msg.Id_,
                    _msg.GetDate(),
                    _msg.Prev_,
                    _msg.GetText().trimmed(),
                    _msg.AimId_,
                    sender,
                    senderFriendly,
                    _msg.Quotes_,
                    _msg.Mentions_,
                    _msg.GetSticker(),
                    _msg.IsOutgoing(),
                    is_not_auth);

            item->setContact(_msg.AimId_);
            item->setTime(_msg.GetTime());
            item->setHasAvatar(_msg.HasAvatar());
            item->setTopMargin(_msg.GetIndentBefore());
            item->setDeliveredToServer(_msg.IsDeliveredToServer());

            if (_msg.Chat_ && !senderFriendly.isEmpty())
            {
                item->setMchatSender(senderFriendly);
            }

            item->setFixedWidth(itemWidth_);

            return item.release();
        }

        auto messageItem = std::make_unique<Ui::MessageItem>(_parent);
        messageItem->setContact(_msg.AimId_);
        messageItem->setId(_msg.Id_, _msg.AimId_);
        messageItem->setSender(sender);
        messageItem->setHasAvatar(_msg.HasAvatar());
        if (_msg.HasAvatar())
        {
            messageItem->loadAvatar(Utils::scale_bitmap(Ui::MessageStyle::getAvatarSize()));
        }

        messageItem->setTopMargin(_msg.GetIndentBefore());
        messageItem->setOutgoing(_msg.IsOutgoing(), _msg.IsDeliveredToServer(), _msg.Chat_, true);
        messageItem->setMchatSender(GetChatFriendly(_msg.GetChatSender(), _msg.ChatFriendly_));
        messageItem->setMchatSenderAimId(_msg.HasChatSender() ? _msg.GetChatSender(): _msg.AimId_);
        messageItem->setTime(_msg.GetTime());
        messageItem->setDate(_msg.GetDate());
        messageItem->setDeleted(_msg.IsDeleted());
        messageItem->setNotAuth(is_not_auth);
        messageItem->setMentions(_msg.Mentions_);

        if (_msg.IsFileSharing())
        {
            createFileSharingWidget(*messageItem, _msg);
        }
        else
        {
            messageItem->setMessage(_msg.GetText());
        }

        return messageItem.release();
    }


    Ui::HistoryControlPageItem* MessagesModel::fillItemById(const QString& _aimId, const MessageKey& _key,  QWidget* _parent)
    {
        CHECK_THREAD
        assert(!_aimId.isEmpty());

        if (_key.getControlType() == control_type::ct_new_messages)
        {
            return createNew(_aimId, _key, _parent);
        }

        if (failedUploads_.removeAll(_key.getInternalId()) > 0)
            return nullptr;

        auto& dialog = getContactDialog(_aimId);

        auto &pendingMessages = dialog.getPendingMessages();

        auto current_pending = pendingMessages.crbegin();

        while (current_pending != pendingMessages.crend())
        {
            if (current_pending->first == _key)
            {
                break;
            }

            ++current_pending;
        }


        if (current_pending != pendingMessages.crend())
        {
            // merge message widgets
            auto result = item(current_pending->second);

            return makePageItem(result, _parent);
        }

        auto &dialogMessages = dialog.getMessages();

        auto haveIncoming = false;

        auto currentMessage = dialogMessages.crbegin();

        while (currentMessage != dialogMessages.crend())
        {
            haveIncoming |= !currentMessage->first.isOutgoing();

            if (currentMessage->first == _key)
            {
                break;
            }

            ++currentMessage;
        }

        if (currentMessage == dialogMessages.crend())
        {
            return nullptr;
        }

        // merge message widgets
        auto result = item(currentMessage->second);

        return makePageItem(result, _parent);
    }

    void MessagesModel::setFirstMessage(const QString& aimId, qint64 msgId)
    {
        CHECK_THREAD
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("contact", aimId);
        collection.set_value_as_int64("message", msgId);
        Ui::GetDispatcher()->post_message_to_core(qsl("dialogs/set_first_message"), collection.get());
    }

    MessagesMapIter MessagesModel::previousMessage(MessagesMap& _map, MessagesMapIter _iter) const
    {
        CHECK_THREAD
        MessagesMapIter result = _map.end();

        while (_iter != _map.begin())
        {
            --_iter;

            if (_iter->first.isDate() || _iter->second.isDeleted())
            {
                continue;
            }

            result = _iter;

            break;
        }

        return result;
    }

    MessagesMapIter MessagesModel::previousMessageWithId(MessagesMap& _map, MessagesMapIter _iter, IncludeDeleted _mode)
    {
        CHECK_THREAD
        MessagesMapIter result = _map.end();

        while (_iter != _map.begin())
        {
            --_iter;

            if (_iter->first.isDate() || !_iter->first.hasId())
                continue;

            if (_mode == IncludeDeleted::No && _iter->second.isDeleted())
                continue;

            result = _iter;

            break;
        }

        return result;
    }

    MessagesMapIter MessagesModel::firstMessageWithId(MessagesMap& _map, MessagesMapIter _iter)
    {
        CHECK_THREAD
        MessagesMapIter result = _map.end();

        while (_iter != _map.end())
        {
            if (_iter->first.isDate() || _iter->second.isDeleted() || !_iter->first.hasId())
            {
                ++_iter;
                continue;
            }

            result = _iter;

            break;
        }

        return result;
    }

    qint64 MessagesModel::lastMessageId(MessagesMap& _map)
    {
        CHECK_THREAD
        const auto end = _map.end();
        const auto it = previousMessageWithId(_map, end);
        if (it != end)
            return it->first.getId();
        return -1;
    }

    MessagesMapIter MessagesModel::nextMessage(MessagesMap& _map, MessagesMapIter _iter) const
    {
        CHECK_THREAD
        auto iter = _iter;

        while (++iter != _map.end())
        {
            if (iter->first.isDate() || iter->second.isDeleted())
            {
                continue;
            }

            break;
        }

        return iter;
    }

    void MessagesModel::contactChanged(const QString& _contact, qint64 _messageId, const Data::DlgState& _dlgState, qint64 quote_id, bool _isFirstRequest, Logic::scroll_mode_type _scrollMode)
    {
        CHECK_THREAD
        if (_contact.isEmpty())
            return;

        Ui::GetDispatcher()->contactSwitched(_contact);

        const ContactDialog* dialog = getContactDialogConst(_contact);

        if (dialog && !dialog->isLastRequestedMessageEmpty() && _messageId == -1)
            return;

        auto& d = getContactDialog(_contact);
        d.setInitMessage(_messageId);
        d.setFirstUnreadMessage(-1);
        d.setMessageCountAfter(_dlgState.UnreadCount_);
        d.enableJumpToMessage(_messageId != -1);
        d.setScrollMode(_scrollMode);
        d.enablePostponeUpdate(false);

        const auto firstRequest = _isFirstRequest ? FirstRequest::Yes : FirstRequest::No;
        const auto needPrefetch = _isFirstRequest ? Prefetch::Yes : Prefetch::No;
        if (_messageId != -1)
            requestMessages(_contact, _messageId, RequestDirection::Bidirection, needPrefetch, JumpToBottom::No, firstRequest, Reason::Plain);
        else
            requestMessages(_contact, _messageId, RequestDirection::ToOlder, needPrefetch, JumpToBottom::No, firstRequest, Reason::Plain);
    }

    void MessagesModel::setItemWidth(int width)
    {
        CHECK_THREAD
        itemWidth_ = width;
    }

    bool MessagesModel::getRecvLastMsg(const QString& _aimId) const
    {
        CHECK_THREAD
        assert(!_aimId.isEmpty());

        auto dialog = getContactDialogConst(_aimId);
        if (!dialog)
            return false;

        return dialog->getRecvLastMessage();
    }

    void MessagesModel::scrollToBottom(const QString& _aimId)
    {
        CHECK_THREAD
        requestMessages(_aimId, -1 /* _messageId */, RequestDirection::ToOlder, Prefetch::Yes, JumpToBottom::Yes, FirstRequest::No, Reason::Plain);
    }

    std::map<MessageKey, Ui::HistoryControlPageItem*> MessagesModel::tail(const QString& aimId, QWidget* parent, bool _is_search, int64_t _mess_id, bool _is_jump_to_bottom)
    {
        CHECK_THREAD
        std::map<MessageKey, Ui::HistoryControlPageItem*> result;

        auto& dialog = getContactDialog(aimId);
        const auto& dialogMessages = dialog.getMessages();
        const auto& pendingMessages = dialog.getPendingMessages();

        if (!_is_jump_to_bottom && dialogMessages.empty() && pendingMessages.empty())
            return result;

        int i = 0;
        MessageKey key;

        auto margin = 0;
        for (const auto& it : boost::adaptors::reverse(pendingMessages))
        {
            assert(!it.second.isDeleted());

            key = it.first;

            const auto buddy = item(it.second);

            result[key] = makePageItem(buddy, parent);
        }
        bool foundMessId = false;
        const bool needToJump = _mess_id != -1;
        auto maxSize = moreCount();
        for (const auto& it : boost::adaptors::reverse(dialogMessages))
        {
            key = it.first;
            const auto buddy = item(it.second);

            result.emplace(key, makePageItem(buddy, parent));

            ++i;
            if (needToJump)
            {
                if (!foundMessId && it.first.getId() == _mess_id)
                {
                    foundMessId = true;
                    i = 0;
                    if (result.size() > maxSize)
                        maxSize /= 2;
                }
                if (foundMessId && i == maxSize)
                    break;
            }
            else if (i == maxSize)
            {
                break;
            }
        }

        dialog.setLastKey(key);
        dialog.setFirstKey(key);

        if (dialog.getLastKey().isEmpty()
            || dialog.getLastKey().isPending()
            || (dialog.getLastKey().getPrev() != -1 && dialogMessages.begin()->first.getId() == dialog.getLastKey().getId())
            || _is_jump_to_bottom
            )
        {
            requestMessages(aimId, -1 /* _messageId */, RequestDirection::ToOlder, Prefetch::Yes, _is_jump_to_bottom ? JumpToBottom::Yes : JumpToBottom::No, FirstRequest::No, Reason::Plain);
        }

        if (result.empty())
            subscribed_ << aimId;

        return result;
    }

    std::map<MessageKey, Ui::HistoryControlPageItem*> MessagesModel::more(const QString& aimId, QWidget* parent, bool _isMoveToBottomIfNeed)
    {
        CHECK_THREAD
        auto& dialog = getContactDialog(aimId);
        auto& dialogMessages = dialog.getMessages();
        auto& pendingMessages = dialog.getPendingMessages();

        if (dialog.getLastKey().isEmpty())
            return tail(aimId, parent, false /* _is_search */, -1 /* _mess_id */);

        std::map<MessageKey, Ui::HistoryControlPageItem*> result;
        if (dialogMessages.empty())
            return result;

        auto i = 0;
        MessageKey key = dialog.getLastKey();
        MessageKey firstKey = dialog.getFirstKey();

        auto iterPending = pendingMessages.crbegin();

        while (iterPending != pendingMessages.crend())
        {
            assert(!iterPending->second.isDeleted());

            if (!(iterPending->first < key))
            {
                ++iterPending;
                continue;
            }

            key = iterPending->first;
            Data::MessageBuddy buddy = item(iterPending->second);

            result[key] = makePageItem(buddy, parent);

            if (++i == moreCount())
            {
                break;
            }

            ++iterPending;
        }

        if (i < moreCount())
        {
            if (_isMoveToBottomIfNeed)
            {
                auto iterIndex = dialogMessages.rbegin();
                auto iterEnd = dialogMessages.rend();

                while (iterIndex != iterEnd)
                {
                    if (!(iterIndex->first < key))
                    {
                        ++iterIndex;
                        continue;
                    }

                    Data::MessageBuddy buddy = item(iterIndex->second);
                    result[iterIndex->first] = makePageItem(buddy, parent);

                    if (iterIndex->first < key)
                    {
                        key = iterIndex->first;
                    }

                    if (++i == moreCount())
                    {
                        break;
                    }

                    ++iterIndex;
                }
            }
            else
            {
                auto iterIndex = dialogMessages.begin();
                auto iterEnd = dialogMessages.end();

                while (iterIndex != iterEnd)
                {
                    if (!(firstKey < iterIndex->first))
                    {
                        ++iterIndex;
                        continue;
                    }

                    Data::MessageBuddy buddy = item(iterIndex->second);
                    result[iterIndex->first] = makePageItem(buddy, parent);

                    if (!(iterIndex->first < firstKey))
                    {
                        firstKey = iterIndex->first;
                    }

                    if (++i == moreCount())
                    {
                        break;
                    }

                    ++iterIndex;
                }
            }
        }

        dialog.setLastKey(key);
        dialog.setFirstKey(firstKey);

        if (
            dialog.getLastKey().isEmpty() ||
            dialog.getLastKey().isPending() ||
            (
                (dialog.getLastKey().getPrev() != -1) &&
                dialogMessages.begin()->first.getId() == dialog.getLastKey().getId()
            )
        )
        {
            if (_isMoveToBottomIfNeed) {
                requestMessages(aimId, -1 /* _messageId */, RequestDirection::ToOlder, Prefetch::Yes, JumpToBottom::No, FirstRequest::No, Reason::Plain);
                if (result.empty())
                    subscribed_ << aimId;
            }
        }

        if (!dialog.getRecvLastMessage())
        {
            if (!_isMoveToBottomIfNeed) {
                requestMessages(aimId, -1 /* _messageId */, RequestDirection::ToNew, Prefetch::Yes, JumpToBottom::No, FirstRequest::No, Reason::Plain);
                if (result.empty())
                    subscribed_ << aimId;
            }
        }

        if (!result.empty())
            Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::history_preload);

        //qDebug("more : %i %lld %lld", result.size(), key.getId(), firstKey.getId());
        return result;
    }

    Ui::HistoryControlPageItem* MessagesModel::getById(const QString& aimId, const MessageKey& key, QWidget* parent)
    {
        CHECK_THREAD
        return fillItemById(aimId, key, parent);
    }

    int32_t MessagesModel::preloadCount() const
    {
        return Data::PRELOAD_MESSAGES_COUNT;
    }

    int32_t MessagesModel::moreCount() const
    {
        return Data::MORE_MESSAGES_COUNT;
    }

    void MessagesModel::setLastKey(const MessageKey& key, const QString& aimId)
    {
        CHECK_THREAD
        assert(!key.isEmpty());
        assert(!aimId.isEmpty());

        __TRACE(
            "history_control",
            "setting a last key for the dialog\n"
            "	contact=<" << aimId << ">"
            "	key=<" << key.toLogStringShort() << ">");

        auto& dialog = getContactDialog(aimId);
        auto& dialogMessages = dialog.getMessages();

        dialog.setLastKey(key);

        auto iter = dialogMessages.begin();
        while(iter != dialogMessages.end())
        {
            if (iter->first < key)
            {
                if (iter->second.isDate())
                {
                    dialog.removeDateItem(iter->second.getDate());
                }

                if (iter->first.getId() <= dialog.getLastRequestedMessage())
                    dialog.setLastRequestedMessage(-1);

                iter = dialogMessages.erase(iter);
            }
            else
            {
                break;
            }
        }

        /*
        if ((int32_t)dialogMessages.size() >= preloadCount())
        {
            setFirstMessage(aimId, dialogMessages.begin()->first.getId());
        }
        */
    }

    void MessagesModel::removeDialog(const QString& _aimId)
    {
        CHECK_THREAD
        dialogs_.remove(_aimId);

        requestedContact_.removeAll(_aimId);
    }

    Ui::ServiceMessageItem* MessagesModel::createNew(const QString& _aimId, const MessageKey& /*_key*/, QWidget* _parent) const
    {
        CHECK_THREAD
        assert(_parent);

        std::unique_ptr<Ui::ServiceMessageItem> newPlate = std::make_unique<Ui::ServiceMessageItem>(_parent);

        newPlate->setWidth(itemWidth_);
        newPlate->setNew();
        newPlate->setContact(_aimId);
        newPlate->updateStyle();

        return newPlate.release();
    }

    void MessagesModel::hideNew(const QString& _aimId)
    {
        CHECK_THREAD
        auto& dialog = getContactDialog(_aimId);
        auto& dialogMessages = dialog.getMessages();

        if (!dialog.getNewKey())
            return;

        const auto isMultichat = Logic::getContactListModel()->isChat(_aimId);

        const Logic::MessageKey& key = *dialog.getNewKey();

        MessagesModel::emitDeleted({ key }, _aimId);

        auto iter_message_after_new = dialogMessages.rend();

        for (auto iterMessage = dialogMessages.rbegin(); iterMessage != dialogMessages.rend(); ++iterMessage)
        {
            if (iterMessage->first.getId() <= key.getId())
                break;

            if (!iterMessage->first.isOutgoing() && !iterMessage->second.isDeleted() && iterMessage->first.getControlType() == control_type::ct_message)
            {
                iter_message_after_new = iterMessage;
                continue;
            }

            iter_message_after_new = dialogMessages.rend();
        }

        if (iter_message_after_new != dialogMessages.rend())
        {
            auto iter_prev_message = iter_message_after_new;
            ++iter_prev_message;

            bool hasAvatar = false;
            if (iter_prev_message == dialogMessages.rend() || iter_message_after_new->second.getBuddy()->hasAvatarWith(*iter_prev_message->second.getBuddy(), isMultichat))
                hasAvatar = true;

            if (!hasAvatar)
                emit hasAvatarChanged(iter_message_after_new->first, false);
        }

        dialog.resetNewKey();
    }

    void MessagesModel::updateNew(const QString& _aimId, const qint64 _newId, const bool _hide)
    {
        CHECK_THREAD
        hideNew(_aimId);

        auto& dialog = getContactDialog(_aimId);

        if (_hide || _newId <= 0)
            return;

        auto& dialogMessages = dialog.getMessages();

        bool isShow = false;

        const auto end = dialogMessages.crend();
        auto iter_prev_message = end;
        for (auto iterMessage = dialogMessages.crbegin(); iterMessage != end; ++iterMessage)
        {
            if (iterMessage->first.getId() <= _newId)
                break;

            if (!iterMessage->first.isOutgoing() && !iterMessage->second.isDeleted() && iterMessage->first.getControlType() == control_type::ct_message)
            {
                isShow = true;
                iter_prev_message = iterMessage;

                continue;
            }

            iter_prev_message = end;
        }

        if (!isShow)
            return;

        dialog.setNewKey(MessageKey(_newId, control_type::ct_new_messages));

        emitUpdated({ *dialog.getNewKey() }, _aimId, NEW_PLATE);

        if (iter_prev_message != end)
        {
            emit hasAvatarChanged(iter_prev_message->first, true);
        }
    }

    QString MessagesModel::formatRecentsText(const Data::MessageBuddy &buddy) const
    {
        CHECK_THREAD
        if (buddy.IsServiceMessage())
        {
            return QString();
        }

        if (buddy.IsChatEvent())
        {
            auto item = std::make_unique<Ui::ChatEventItem>(buddy.GetChatEvent(), buddy.Id_);
            item->setContact(buddy.AimId_);
            return item->formatRecentsText();
        }

        if (buddy.IsVoipEvent())
        {
            auto item = std::make_unique<Ui::VoipEventItem>(buddy.GetVoipEvent());
            return item->formatRecentsText();
        }

        if (buddy.ContainsPreviewableSiteLink() || !buddy.Quotes_.isEmpty() || buddy.IsSticker())
        {
            auto item =
                Ui::ComplexMessage::ComplexMessageItemBuilder::makeComplexItem(
                    nullptr,
                    0,
                    QDate::currentDate(),
                    0,
                    buddy.GetText(),
                    buddy.AimId_,
                    buddy.AimId_,
                    buddy.AimId_,
                    buddy.Quotes_,
                    buddy.Mentions_,
                    buddy.GetSticker(),
                    false,
                    false);

            return item->formatRecentsText();
        }

        auto messageItem = std::make_unique<Ui::MessageItem>();
        messageItem->setContact(buddy.AimId_);
        if (buddy.IsFileSharing())
        {
            if (buddy.ContainsPttAudio())
            {
                return QT_TRANSLATE_NOOP("contact_list", "Voice message");
            }

            if (buddy.ContainsGif())
            {
                return QT_TRANSLATE_NOOP("contact_list", "GIF");
            }

            if (buddy.ContainsImage())
            {
                return QT_TRANSLATE_NOOP("contact_list", "Photo");
            }

            if (buddy.ContainsVideo())
            {
                return QT_TRANSLATE_NOOP("contact_list", "Video");
            }

            auto item = new HistoryControl::FileSharingWidget(buddy.GetFileSharing(), buddy.AimId_);
            messageItem->setContentWidget(item);
        }
        else
        {
            messageItem->setMentions(buddy.Mentions_);
            messageItem->setMessage(buddy.GetText());
        }

        return messageItem->formatRecentsText();
    }

    int64_t MessagesModel::getLastMessageId(const QString &aimId) const
    {
        CHECK_THREAD
        assert(!aimId.isEmpty());

        auto dialog = getContactDialogConst(aimId);
        if (!dialog)
            return -1;

        return dialog->getLastMessageId();
    }

    qint64 MessagesModel::normalizeNewMessagesId(const QString& _aimid, qint64 _id)
    {
        CHECK_THREAD
        if (_id == -1)
            return _id;

        auto& dialog = getContactDialog(_aimid);
        auto& dialogMessages = dialog.getMessages();

        qint64 newId = -1;

        auto iter = dialogMessages.crbegin();

        while (iter != dialogMessages.crend() && iter->first.getId() >= _id)
        {
            if (iter->first.isOutgoing())
            {
                newId = iter->first.getId();
                break;
            }

            newId = iter->first.getId();
            ++iter;
        }

        return newId == -1 ? _id : newId;
    }

    bool MessagesModel::hasPending(const QString& _aimId) const
    {
        CHECK_THREAD
        const ContactDialog* dialog = getContactDialogConst(_aimId);
        return dialog && dialog->hasPending();
    }

    void MessagesModel::eraseHistory(const QString& _aimid)
    {
        CHECK_THREAD
        const auto lastMessageId = getLastMessageId(_aimid);

        if (lastMessageId > 0)
        {
            Ui::GetDispatcher()->deleteMessagesFrom(_aimid, lastMessageId);

            Utils::InterConnector::instance().setSidebarVisible(false);
        }

        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::history_delete);
    }

    void MessagesModel::emitQuote(int64_t quote_id)
    {
        CHECK_THREAD
        emit quote(quote_id);
    }

    SendersVector Logic::MessagesModel::getSenders(const QString& _aimid, bool _includeSelf) const
    {
        CHECK_THREAD
        auto dialog = getContactDialogConst(_aimid);
        if (dialog)
            return dialog->getSenders(_includeSelf);

        return SendersVector();
    }

    ContactDialog& MessagesModel::getContactDialog(const QString& _aimid)
    {
        CHECK_THREAD
        auto iter = dialogs_.find(_aimid);
        if (iter == dialogs_.end())
        {
            iter = dialogs_.insert(_aimid, std::make_shared<ContactDialog>());
        }

        return *iter.value();
    }

    const ContactDialog* MessagesModel::getContactDialogConst(const QString& _aimid) const
    {
        CHECK_THREAD
        const auto iter = dialogs_.find(_aimid);
        if (iter == dialogs_.end())
            return nullptr;

        return iter.value().get();
    }

    MessagesModel* GetMessagesModel()
    {
        CHECK_THREAD
        if (!g_messages_model)
            g_messages_model = std::make_unique<MessagesModel>(nullptr);

        return g_messages_model.get();
    }

    void ResetMessagesModel()
    {
        CHECK_THREAD
        if (g_messages_model)
            g_messages_model.reset();
    }
}

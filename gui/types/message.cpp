#include "stdafx.h"

#include "message.h"

#include "../utils/gui_coll_helper.h"
#include "../../corelib/enumerations.h"
#include "../utils/log/log.h"
#include "../utils/UrlParser.h"

#include "../main_window/history_control/FileSharingInfo.h"
#include "../main_window/history_control/StickerInfo.h"
#include "../main_window/history_control/ChatEventInfo.h"
#include "../main_window/history_control/VoipEventInfo.h"
#include "../main_window/history_control/MessagesModel.h"

#include "../cache/avatars/AvatarStorage.h"
#include "../main_window/contact_list/ContactListModel.h"
#include "../my_info.h"

namespace
{
    void unserializeMessages(
        core::iarray* msgArray,
        const QString &aimId,
        const QString &myAimid,
        const qint64 theirs_last_delivered,
        const qint64 theirs_last_read,
        Out Data::MessageBuddies &messages);

    bool containsSitePreviewUri(const QString &text, Out QStringRef &uri);

    bool containsPttAudio(const QString& text, Out int& duration);

    int decodeSymbols(const QStringRef& str)
    {
        int result = 0;

        for (QChar ch : str)
        {
            char c = ch.toLatin1();
            int value = 0;
            if (c >= '0' && c <= '9')
                value = c - 48;
            else if (c >= 'a' && c <= 'z')
                value = c - 87;
            else
                value = c - 29;

            result += value;
        }

        return result;
    }
}

namespace Data
{
    MessageBuddy::MessageBuddy()
        : Id_(-1)
        , Prev_(-1)
        , PendingId_(-1)
        , Time_(0)
        , Chat_(false)
        , Unread_(false)
        , Filled_(false)
        , LastId_(-1)
        , Deleted_(false)
        , DeliveredToClient_(false)
        , HasAvatar_(false)
        , IndentBefore_(false)
        , Outgoing_(false)
        , Type_(core::message_type::base)
    {
    }

    void MessageBuddy::ApplyModification(const MessageBuddy &modification)
    {
        assert(modification.Id_ == Id_);

        EraseEventData();

        if (modification.IsBase())
        {
            SetText(modification.GetText());

            Type_ = core::message_type::base;

            return;
        }

        if (modification.IsChatEvent())
        {
            const auto &chatEventInfo = *modification.GetChatEvent();
            const auto &eventText = chatEventInfo.formatEventText();
            assert(!eventText.isEmpty());

            SetText(eventText);
            SetChatEvent(modification.GetChatEvent());

            Type_ = core::message_type::chat_event;
            //Type_ = core::message_type::base;

            return;
        }

        assert(!"unexpected modification type");
    }

    bool MessageBuddy::IsEmpty() const
    {
        return ((Id_ == -1) && InternalId_.isEmpty());
    }

    bool MessageBuddy::CheckInvariant() const
    {
        if (Outgoing_)
        {
            if (!HasId() && InternalId_.isEmpty())
            {
                return false;
            }
        }
        else
        {
            if (!InternalId_.isEmpty())
            {
                return false;
            }
        }

        return true;
    }

    Logic::preview_type MessageBuddy::GetPreviewableLinkType() const
    {
        assert(Type_ > core::message_type::min);
        assert(Type_ < core::message_type::max);

        if (FileSharing_ || Sticker_ || ChatEvent_ || VoipEvent_)
        {
            return Logic::preview_type::none;
        }

        QStringRef uri;
        if (containsSitePreviewUri(Text_, Out uri))
        {
            return Logic::preview_type::site;
        }

        return Logic::preview_type::none;
    }

    bool MessageBuddy::ContainsAnyPreviewableLink() const
    {
        const auto previewLinkType = GetPreviewableLinkType();
        assert(previewLinkType > Logic::preview_type::min);
        assert(previewLinkType < Logic::preview_type::max);

        return (previewLinkType != Logic::preview_type::none);
    }

    bool MessageBuddy::ContainsPreviewableSiteLink() const
    {
        const auto previewLinkType = GetPreviewableLinkType();
        assert(previewLinkType > Logic::preview_type::min);
        assert(previewLinkType < Logic::preview_type::max);

        return (previewLinkType == Logic::preview_type::site);
    }

    bool MessageBuddy::ContainsPttAudio() const
    {
        assert(Type_ > core::message_type::min);
        assert(Type_ < core::message_type::max);

        auto pttDuration = 0;

        if (containsPttAudio(Text_, Out pttDuration))
        {
            return true;
        }

        if (!FileSharing_ || FileSharing_->IsOutgoing())
        {
            return false;
        }

        return containsPttAudio(FileSharing_->GetUri(), Out pttDuration);
    }

    bool MessageBuddy::ContainsGif() const
    {
        assert(Type_ > core::message_type::min);
        assert(Type_ < core::message_type::max);

        if (FileSharing_)
        {
            const auto isGif = FileSharing_->getContentType() == core::file_sharing_content_type::gif;
            return isGif;
        }

        return false;
    }

    bool MessageBuddy::ContainsImage() const
    {
        assert(Type_ > core::message_type::min);
        assert(Type_ < core::message_type::max);

        if (FileSharing_)
        {
            const auto isImage = FileSharing_->getContentType() == core::file_sharing_content_type::image;
            return isImage;
        }

        return false;
    }

    bool MessageBuddy::ContainsVideo() const
    {
        assert(Type_ > core::message_type::min);
        assert(Type_ < core::message_type::max);

        if (FileSharing_)
        {
            const auto isVideo = (FileSharing_->getContentType() == core::file_sharing_content_type::video);
            return isVideo;
        }

        return false;
    }

    bool MessageBuddy::ContainsMentions() const
    {
        return !Mentions_.empty();
    }

    bool MessageBuddy::GetIndentWith(const MessageBuddy& _buddy) const
    {
        if (_buddy.IsServiceMessage())
        {
            return false;
        }

        if (_buddy.IsChatEvent())
        {
            return true;
        }

        if (!isSameDirection(_buddy))
        {
            return true;
        }

        if (!_buddy.IsOutgoing() && !IsOutgoing() && ChatSender_ != _buddy.ChatSender_)
        {
            return true;
        }

        return false;
    }

    bool MessageBuddy::isSameDirection(const MessageBuddy& _prevBuddy) const
    {
        if (IsVoipEvent() && _prevBuddy.IsVoipEvent())
        {
            return (IsOutgoingVoip() == _prevBuddy.IsOutgoingVoip());
        }
        else if (IsVoipEvent() && !_prevBuddy.IsVoipEvent())
        {
            return (IsOutgoingVoip() == _prevBuddy.IsOutgoing());
        }
        else if (!IsVoipEvent() && _prevBuddy.IsVoipEvent())
        {
            return (IsOutgoing() == _prevBuddy.IsOutgoingVoip());
        }

        return (IsOutgoing() == _prevBuddy.IsOutgoing());
    }

    bool MessageBuddy::hasAvatarWith(const MessageBuddy& _prevBuddy, const bool _isMultichat) const
    {
        if (_isMultichat)
        {
            if (IsChatEvent() == _prevBuddy.IsChatEvent())
                return (GetChatSender() != _prevBuddy.GetChatSender());
            return true;
        }

        if (!isSameDirection(_prevBuddy))
        {
            return true;
        }

        if (_prevBuddy.IsChatEvent())
        {
            return true;
        }

        if (_prevBuddy.IsServiceMessage())
        {
            return true;
        }

        return false;
    }

    bool MessageBuddy::IsBase() const
    {
        assert(Type_ > core::message_type::min);
        assert(Type_ < core::message_type::max);

        return (Type_ == core::message_type::base);
    }

    bool MessageBuddy::IsChatEvent() const
    {
        assert(Type_ > core::message_type::min);
        assert(Type_ < core::message_type::max);

        return (Type_ == core::message_type::chat_event);
    }

    bool MessageBuddy::IsDeleted() const
    {
        return Deleted_;
    }

    bool MessageBuddy::IsDeliveredToClient() const
    {
        return DeliveredToClient_;
    }

    bool MessageBuddy::IsDeliveredToServer() const
    {
        return HasId();
    }

    bool MessageBuddy::IsFileSharing() const
    {
        assert(Type_ > core::message_type::min);
        assert(Type_ < core::message_type::max);

        return (Type_ == core::message_type::file_sharing);
    }

    bool MessageBuddy::IsOutgoing() const
    {
        return Outgoing_;
    }

    bool MessageBuddy::IsOutgoingVoip() const
    {
        if (!VoipEvent_)
        {
            return IsOutgoing();
        }

        return !VoipEvent_->isIncomingCall();
    }

    bool MessageBuddy::IsSticker() const
    {
        return (Type_ == core::message_type::sticker);
    }

    bool MessageBuddy::IsPending() const
    {
        return Id_ == -1 && !InternalId_.isEmpty();
    }

    bool MessageBuddy::IsServiceMessage() const
    {
        return (!IsBase() && !IsFileSharing() && !IsSticker() && !IsChatEvent() && !IsVoipEvent());
    }


    bool MessageBuddy::IsVoipEvent() const
    {
        return (VoipEvent_ != nullptr);
    }

    const HistoryControl::ChatEventInfoSptr& MessageBuddy::GetChatEvent() const
    {
        return ChatEvent_;
    }

    const QString& MessageBuddy::GetChatSender() const
    {
        return ChatSender_;
    }

    const QDate& MessageBuddy::GetDate() const
    {
        assert(Date_.isValid());

        return Date_;
    }

    const HistoryControl::FileSharingInfoSptr& MessageBuddy::GetFileSharing() const
    {
        return FileSharing_;
    }

    QStringRef MessageBuddy::GetFirstSiteLinkFromText() const
    {
        QStringRef result;
        containsSitePreviewUri(GetText(), Out result);
        return result;
    }

    int MessageBuddy::GetPttDuration() const
    {
        int duration;
        containsPttAudio(GetText(), duration);
        return duration;
    }

    bool MessageBuddy::GetIndentBefore() const
    {
        return IndentBefore_;
    }

    const HistoryControl::StickerInfoSptr& MessageBuddy::GetSticker() const
    {
        return Sticker_;
    }

    const QString& MessageBuddy::GetText() const
    {
        return Text_;
    }

    qint32 MessageBuddy::GetTime() const
    {
        return Time_;
    }

    qint64 MessageBuddy::GetLastId() const
    {
        assert(LastId_ >= -1);

        return LastId_;
    }

    core::message_type MessageBuddy::GetType() const
    {
        assert(Type_ > core::message_type::min);
        assert(Type_ < core::message_type::max);

        return Type_;
    }

    const HistoryControl::VoipEventInfoSptr& MessageBuddy::GetVoipEvent() const
    {
        return VoipEvent_;
    }

    bool MessageBuddy::HasAvatar() const
    {
        return HasAvatar_;
    }

    bool MessageBuddy::HasChatSender() const
    {
        return !ChatSender_.isEmpty();
    }

    bool MessageBuddy::HasId() const
    {
        return (Id_ != -1);
    }

    bool MessageBuddy::HasText() const
    {
        return !Text_.isEmpty();
    }

    void MessageBuddy::FillFrom(const MessageBuddy &buddy)
    {
        Text_ = buddy.GetText();

        SetLastId(buddy.Id_);
        Unread_ = buddy.Unread_;
        DeliveredToClient_ = buddy.DeliveredToClient_;
        Date_ = buddy.Date_;
        Deleted_ = buddy.Deleted_;

        SetFileSharing(buddy.GetFileSharing());
        SetSticker(buddy.GetSticker());
        SetChatEvent(buddy.GetChatEvent());
        SetVoipEvent(buddy.GetVoipEvent());

        Quotes_ = buddy.Quotes_;
    }

    void MessageBuddy::EraseEventData()
    {
        FileSharing_.reset();
        Sticker_.reset();
        ChatEvent_.reset();
        VoipEvent_.reset();
    }

    void MessageBuddy::SetChatEvent(const HistoryControl::ChatEventInfoSptr& chatEvent)
    {
        assert(!chatEvent || (!Sticker_ && !FileSharing_ && !VoipEvent_));

        ChatEvent_ = chatEvent;
    }

    void MessageBuddy::SetChatSender(const QString& chatSender)
    {
        ChatSender_ = chatSender;
    }

    void MessageBuddy::SetDate(const QDate &date)
    {
        assert(date.isValid());

        Date_ = date;
    }

    void MessageBuddy::SetDeleted(const bool isDeleted)
    {
        Deleted_ = isDeleted;
    }

    void MessageBuddy::SetFileSharing(const HistoryControl::FileSharingInfoSptr& fileSharing)
    {
        assert(!fileSharing || (!Sticker_ && !ChatEvent_ && !VoipEvent_));

        FileSharing_ = fileSharing;
    }

    void MessageBuddy::SetHasAvatar(const bool hasAvatar)
    {
        HasAvatar_ = hasAvatar;
    }

    void MessageBuddy::SetIndentBefore(const bool indentBefore)
    {
        IndentBefore_ = indentBefore;
    }

    void MessageBuddy::SetLastId(const qint64 lastId)
    {
        assert(lastId >= -1);

        LastId_ = lastId;
    }

    void MessageBuddy::SetText(const QString &text)
    {
        Text_ = text;
    }

    void MessageBuddy::SetTime(const qint32 time)
    {
        Time_ = time;
    }

    void MessageBuddy::SetType(const core::message_type type)
    {
        assert(type > core::message_type::min);
        assert(type < core::message_type::max);

        Type_ = type;
    }

    void MessageBuddy::SetVoipEvent(const HistoryControl::VoipEventInfoSptr &voip)
    {
        assert(!voip || (!Sticker_ && !ChatEvent_ && !FileSharing_));

        VoipEvent_ = voip;
    }

    Logic::MessageKey MessageBuddy::ToKey() const
    {
        return Logic::MessageKey(Id_, Prev_, InternalId_, PendingId_, Time_, Type_, IsOutgoing(), GetPreviewableLinkType(), Logic::control_type::ct_message);
    }

    void MessageBuddy::SetOutgoing(const bool isOutgoing)
    {
        Outgoing_ = isOutgoing;

        if (!HasId() && Outgoing_)
        {
            assert(!InternalId_.isEmpty());
        }
    }

    void MessageBuddy::SetSticker(const HistoryControl::StickerInfoSptr &sticker)
    {
        assert(!sticker || (!FileSharing_ && !ChatEvent_));

        Sticker_ = sticker;
    }

    const QString& DlgState::GetText() const
    {
        return Text_;
    }

    bool DlgState::HasLastMsgId() const
    {
        assert(LastMsgId_ >= -1);

        return (LastMsgId_ > 0);
    }

    bool DlgState::HasText() const
    {
        return !Text_.isEmpty();
    }

    void DlgState::SetText(QString text)
    {
        Text_ = std::move(text);
    }

    MessagesResult UnserializeMessageBuddies(core::coll_helper* helper, const QString &myAimid)
    {
        assert(!myAimid.isEmpty());

        bool havePending = false;
        QString aimId = QString::fromUtf8(helper->get_value_as_string("contact"));
        MessageBuddies messages;
        MessageBuddies modifications;
        MessageBuddies introMessages;
        int64_t lastMsgId = -1;
        const auto resultExist = helper->is_value_exist("result");
        if (!resultExist || helper->get_value_as_bool("result"))
        {
            const auto theirs_last_delivered = helper->get_value_as_int64("theirs_last_delivered", -1);
            const auto theirs_last_read = helper->get_value_as_int64("theirs_last_read", -1);

            core::iarray* msgArray = helper->get_value_as_array("messages");
            unserializeMessages(msgArray, aimId, myAimid, theirs_last_delivered, theirs_last_read, Out messages);

            if (helper->is_value_exist("pending_messages"))
            {
                havePending = true;
                msgArray = helper->get_value_as_array("pending_messages");
                unserializeMessages(msgArray, aimId, myAimid, theirs_last_delivered, theirs_last_read, Out messages);
            }

            if (helper->is_value_exist("intro_messages"))
            {
                auto introArray = helper->get_value_as_array("intro_messages");
                unserializeMessages(introArray, aimId, myAimid, theirs_last_delivered, theirs_last_read, Out introMessages);
            }

            if (helper->is_value_exist("modified"))
            {
                auto modificationsArray = helper->get_value_as_array("modified");
                unserializeMessages(modificationsArray, aimId, myAimid, theirs_last_delivered, theirs_last_read, Out modifications);
            }

            if (helper->is_value_exist("last_msg_in_index"))
                lastMsgId = helper->get_value_as_int64("last_msg_in_index");
        }

        return { std::move(aimId), std::move(messages), std::move(introMessages), std::move(modifications), lastMsgId, havePending };
    }

    Data::MessageBuddySptr unserializeMessage(
        core::coll_helper &msgColl,
        const QString &aimId,
        const QString &myAimid,
        const qint64 theirs_last_delivered,
        const qint64 theirs_last_read)
    {
        auto message = std::make_shared<Data::MessageBuddy>();

        message->Id_ = msgColl.get_value_as_int64("id");
        message->InternalId_ = QString::fromUtf8(msgColl.get_value_as_string("internal_id"));
        message->Prev_ = msgColl.get_value_as_int64("prev_id");
        message->AimId_ = aimId;
        message->SetOutgoing(msgColl.get<bool>("outgoing"));
        message->SetDeleted(msgColl.get<bool>("deleted"));

        if (message->IsOutgoing() && (message->Id_ != -1))
            message->Unread_ = (message->Id_ > theirs_last_read);

        if (message->Id_ == -1 && !message->InternalId_.isEmpty())
        {
            int pendingPos = message->InternalId_.lastIndexOf(ql1c('-'));
            const auto pendingId = message->InternalId_.rightRef(message->InternalId_.length() - pendingPos - 1);
            message->PendingId_ = pendingId.toInt();
        }

        const auto timestamp = msgColl.get<int32_t>("time");

        message->SetTime(timestamp);
        if (msgColl->is_value_exist("text"))
            message->SetText(QString::fromUtf8(msgColl.get_value_as_string("text")));
        message->SetDate(QDateTime::fromTime_t(message->GetTime()).date());

        __TRACE(
            "delivery",
            "unserialized message\n" <<
            "	id=					<" << message->Id_ << ">\n" <<
            "	last_delivered=		<" << theirs_last_delivered << ">\n" <<
            "	outgoing=<" << logutils::yn(message->IsOutgoing()) << ">\n" <<
            "	notification_key=<" << message->InternalId_ << ">\n" <<
            "	delivered_to_client=<" << logutils::yn(message->IsDeliveredToClient()) << ">\n" <<
            "	delivered_to_server=<" << logutils::yn(message->Id_ != -1) << ">");

        if (msgColl.is_value_exist("chat"))
        {
            core::coll_helper chat(msgColl.get_value_as_collection("chat"), false);
            if (!chat->empty())
            {
                message->Chat_ = true;
                const QString sender = QString::fromUtf8(chat.get_value_as_string("sender"));
                message->SetChatSender(sender);
                message->ChatFriendly_ = chat.get_value_as_string("friendly");
                if (message->ChatFriendly_.isEmpty() && sender != myAimid)
                    message->ChatFriendly_ = sender;
            }
        }

        if (msgColl.is_value_exist("file_sharing"))
        {
            core::coll_helper file_sharing(msgColl.get_value_as_collection("file_sharing"), false);

            message->SetType(core::message_type::file_sharing);
            message->SetFileSharing(std::make_shared<HistoryControl::FileSharingInfo>(file_sharing));
        }

        if (msgColl.is_value_exist("sticker"))
        {
            core::coll_helper sticker(msgColl.get_value_as_collection("sticker"), false);

            message->SetType(core::message_type::sticker);
            message->SetSticker(HistoryControl::StickerInfo::Make(sticker));
        }

        if (msgColl.is_value_exist("voip"))
        {
            core::coll_helper voip(msgColl.get_value_as_collection("voip"), false);

            message->SetType(core::message_type::voip_event);
            message->SetVoipEvent(
                HistoryControl::VoipEventInfo::Make(voip, timestamp)
            );
        }

        if (msgColl.is_value_exist("chat_event"))
        {
            assert(!message->IsChatEvent());

            core::coll_helper chat_event(msgColl.get_value_as_collection("chat_event"), false);

            message->SetType(core::message_type::chat_event);

            message->SetChatEvent(
                HistoryControl::ChatEventInfo::Make(
                    chat_event,
                    message->IsOutgoing(),
                    myAimid
                )
            );
        }

        if (msgColl->is_value_exist("quotes"))
        {
            core::iarray* quotes = msgColl.get_value_as_array("quotes");
            const auto size = quotes->size();
            message->Quotes_.reserve(size);
            for (auto i = 0; i < size; ++i)
            {
                Data::Quote q;
                q.unserialize(quotes->get_at(i)->get_as_collection());
                message->Quotes_.push_back(std::move(q));
            }
        }

        if (msgColl->is_value_exist("mentions"))
        {
            core::iarray* ment = msgColl.get_value_as_array("mentions");
            for (auto i = 0; i < ment->size(); ++i)
            {
                const auto coll = ment->get_at(i)->get_as_collection();
                core::coll_helper ment_helper(coll, false);
                auto currentAimId = QString::fromUtf8(ment_helper.get_value_as_string("sn"));

                QString fr;
                if (currentAimId == Ui::MyInfo()->aimId())
                    fr = Ui::MyInfo()->friendlyName();
                else if (Logic::getContactListModel()->contains(currentAimId))
                    fr = Logic::getContactListModel()->getDisplayName(currentAimId);
                else if (ment_helper->is_value_exist("friendly"))
                    fr = QString::fromUtf8(ment_helper.get_value_as_string("friendly"));
                else
                    fr = currentAimId;

                if (!currentAimId.isEmpty() && !fr.isEmpty())
                    message->Mentions_.emplace(std::move(currentAimId), std::move(fr));
            }
        }

        return message;
    }

    ServerMessagesIds UnserializeServerMessagesIds(const core::coll_helper& helper)
    {
        auto aimId = QString::fromUtf8(helper.get_value_as_string("contact"));
        QVector<qint64> ids;
        if (helper.is_value_exist("result") && !helper.get_value_as_bool("result"))
        {
            return {};
        }
        core::iarray* msgArray = helper.get_value_as_array("ids");
        if (msgArray)
        {
            const int32_t size = msgArray->size();
            ids.reserve(size);
            for (int32_t i = 0; i < size; ++i)
                ids.push_back(msgArray->get_at(i)->get_as_int64());
        }
        QVector<int64_t> deletedIds;
        if (helper.is_value_exist("deleted"))
        {
            const auto deletedIdsArray = helper.get_value_as_array("deleted");
            assert(!deletedIdsArray->empty());
            deletedIds.reserve(deletedIdsArray->size());
            for (auto i = 0; i < deletedIdsArray->size(); ++i)
                deletedIds.push_back(deletedIdsArray->get_at(i)->get_as_int64());
        }
        MessageBuddies modifications;
        if (helper.is_value_exist("modified"))
        {
            auto modificationsArray = helper.get_value_as_array("modified");
            const auto myAimid = helper.get<QString>("my_aimid");
            const auto theirs_last_delivered = helper.get_value_as_int64("theirs_last_delivered", -1);
            const auto theirs_last_read = helper.get_value_as_int64("theirs_last_read", -1);
            unserializeMessages(modificationsArray, aimId, myAimid, theirs_last_delivered, theirs_last_read, Out modifications);
        }
        return { std::move(aimId), std::move(ids), std::move(deletedIds), std::move(modifications) };
    }

    void SerializeDlgState(core::coll_helper* helper, const DlgState& state)
    {
        helper->set_value_as_string("contact", state.AimId_.toStdString());
        helper->set_value_as_int64("unreads", state.UnreadCount_);
        helper->set_value_as_int64("last_msg_id", state.LastMsgId_);
        helper->set_value_as_int64("yours_last_read", state.YoursLastRead_);
        helper->set_value_as_int64("theirs_last_read", state.TheirsLastRead_);
        helper->set_value_as_int64("theirs_last_delivered", state.TheirsLastDelivered_);
        helper->set_value_as_string("last_message_friendly", state.LastMessageFriendly_.toStdString());
    }

    DlgState UnserializeDlgState(core::coll_helper* helper, const QString &myAimId, bool _from_search)
    {
        DlgState state;
        state.AimId_ = helper->get<QString>("contact");
        state.UnreadCount_ = helper->get<int64_t>("unreads");
        state.LastMsgId_ = helper->get<int64_t>("last_msg_id");
        state.YoursLastRead_ = helper->get<int64_t>("yours_last_read");
        state.TheirsLastRead_ = helper->get<int64_t>("theirs_last_read");
        state.TheirsLastDelivered_ = helper->get<int64_t>("theirs_last_delivered");
        state.Visible_ = helper->get<bool>("visible");
        state.LastMessageFriendly_ = helper->get<QString>("last_message_friendly");
        state.Friendly_ = helper->get<QString>("friendly");
        state.Chat_ = helper->get<bool>("is_chat");
        state.Official_ = helper->get<bool>("official");
        state.unreadMentionsCount_ = helper->get<int32_t>("unread_mention_count");

	if (helper->is_value_exist("term"))
            state.SearchTerm_ = helper->get<QString>("term");

        if (helper->is_value_exist("favorite_time"))
        {
            state.FavoriteTime_ = helper->get<int64_t>("favorite_time");
        }
        if (helper->is_value_exist("is_contact"))
        {
            state.IsContact_ = helper->get<bool>("is_contact");
        }

        if (helper->is_value_exist("message"))
        {
            core::coll_helper value(helper->get_value_as_collection("message"), false);

            const auto messageBuddy = Data::unserializeMessage(value, state.AimId_, myAimId, state.TheirsLastDelivered_, state.TheirsLastRead_);

            state.hasMentionMe_ = (messageBuddy->Mentions_.find(myAimId) != messageBuddy->Mentions_.end());

            const auto serializeMessage = helper->get<bool>("serialize_message");
            if (serializeMessage)
            {
                auto text = messageBuddy->GetText();

                if (!_from_search || state.IsContact_)
                {
                    text = Logic::GetMessagesModel()->formatRecentsText(*messageBuddy);
                }

                state.SetText(std::move(text));

                if (!state.IsContact_)
                    state.SearchedMsgId_ = messageBuddy->Id_;

                state.IsLastMessageDelivered = messageBuddy->HasId();
            }

            state.senderAimId_ = messageBuddy->GetChatSender();
            state.senderNick_ = messageBuddy->ChatFriendly_;
            state.Time_ = value.get<int32_t>("time");
            state.Outgoing_ = value.get<bool>("outgoing");

            if (messageBuddy->GetChatEvent() && messageBuddy->GetChatEvent()->eventType() == core::chat_event_type::avatar_modified)
            {
                static int32_t previousUpdateTime = 0;
                if ((state.Time_ - previousUpdateTime) > 1)
                {
                    previousUpdateTime = state.Time_;
                    Logic::GetAvatarStorage()->updateAvatar(state.AimId_);
                }
            }
        }
        return state;
    }

    void Quote::serialize(core::icollection* _collection) const
    {
        Ui::gui_coll_helper coll(_collection, false);
        coll.set_value_as_qstring("text", text_);
        coll.set_value_as_qstring("sender", senderId_);
        coll.set_value_as_qstring("chatId", chatId_);
        coll.set_value_as_qstring("senderFriendly", senderFriendly_);
        coll.set_value_as_int("time", time_);
        coll.set_value_as_int64("msg", msgId_);
        coll.set_value_as_bool("forward", isForward_);
        coll.set_value_as_int("setId", setId_);
        coll.set_value_as_int("stickerId", stickerId_);
        coll.set_value_as_qstring("stamp", chatStamp_);
        coll.set_value_as_qstring("chatName", chatName_);
    }

    void Quote::unserialize(core::icollection* _collection)
    {
        Ui::gui_coll_helper coll(_collection, false);
        if (coll->is_value_exist("text"))
            text_ = QString::fromUtf8(coll.get_value_as_string("text"));

        if (coll->is_value_exist("sender"))
            senderId_ = QString::fromUtf8(coll.get_value_as_string("sender"));

        if (coll->is_value_exist("chatId"))
            chatId_ = QString::fromUtf8(coll.get_value_as_string("chatId"));

        if (coll->is_value_exist("time"))
            time_ = coll.get_value_as_int("time");

        if (coll->is_value_exist("msg"))
            msgId_ = coll.get_value_as_int64("msg");

        if (coll->is_value_exist("senderFriendly"))
            senderFriendly_ = QString::fromUtf8(coll.get_value_as_string("senderFriendly"));

        if (coll->is_value_exist("forward"))
            isForward_ = coll.get_value_as_bool("forward");

        if (coll->is_value_exist("setId"))
            setId_ = coll.get_value_as_int("setId");

        if (coll->is_value_exist("stickerId"))
            stickerId_ = coll.get_value_as_int("stickerId");

        if (coll->is_value_exist("stamp"))
            chatStamp_ = QString::fromUtf8(coll.get_value_as_string("stamp"));

        if (coll->is_value_exist("chatName"))
            chatName_ = QString::fromUtf8(coll.get_value_as_string("chatName"));

        if (senderId_ == Ui::MyInfo()->aimId())
            senderFriendly_ = Ui::MyInfo()->friendlyName();
        else if (Logic::getContactListModel()->contains(senderId_))
            senderFriendly_ = Logic::getContactListModel()->getDisplayName(senderId_);
    }
}

namespace
{
    void unserializeMessages(
        core::iarray* msgArray,
        const QString &aimId,
        const QString &myAimid,
        const qint64 theirs_last_delivered,
        const qint64 theirs_last_read,
        Out Data::MessageBuddies &messages)
    {
        assert(!aimId.isEmpty());
        assert(!myAimid.isEmpty());
        assert(msgArray);

        __TRACE(
            "delivery",
            "unserializing messages collection\n" <<
            "	size=<" << msgArray->size() << ">\n" <<
            "	last_delivered=<" << theirs_last_delivered << ">");

        const auto size = msgArray->size();
        messages.reserve(messages.size() + size);
        for (int32_t i = 0; i < size; ++i)
        {
            core::coll_helper value(
                msgArray->get_at(i)->get_as_collection(),
                false
            );

            auto message = Data::unserializeMessage(
                value, aimId, myAimid, theirs_last_delivered, theirs_last_read
            );

            const auto isInvisibleVoipEvent = (message->IsVoipEvent() && !message->GetVoipEvent()->isVisible());
            const auto skipMessage = isInvisibleVoipEvent;
            if (!skipMessage)
                messages.push_back(std::move(message));
        }
    }

    bool containsSitePreviewUri(const QString &text, Out QStringRef &uri)
    {
        Out uri = QStringRef();

        static const QRegularExpression space(
            ql1s("\\s|\\n|\\r"),
            QRegularExpression::UseUnicodePropertiesOption | QRegularExpression::OptimizeOnFirstUsageOption
        );

        const auto parts = text.splitRef(space, QString::SkipEmptyParts);

        for (const auto &part : parts)
        {
            Utils::UrlParser parser;

            parser.process(part);

            if (parser.hasUrl())
            {
                uri = part;
                return true;
            }
        }

        return false;
    }

    bool containsPttAudio(const QString& text, Out int& duration)
    {
        Out duration = 0;

        const auto parts = text.splitRef(ql1c(' '), QString::SkipEmptyParts);
        for (const auto &part : parts)
        {
            if (!part.startsWith(ql1s("www."), Qt::CaseInsensitive) &&
                !part.startsWith(ql1s("http://"), Qt::CaseInsensitive) &&
                !part.startsWith(ql1s("https://"), Qt::CaseInsensitive))
            {
                continue;
            }

            auto index = part.lastIndexOf(ql1c('/'));
            if (index < 0)
            {
                continue;
            }

            ++index;

            auto isEos = (index >= part.length());
            if (isEos)
            {
                continue;
            }

            if (part.at(index) != ql1c('I') && part.at(index) != ql1c('J'))
            {
                continue;
            }

            ++index;

            isEos = (index >= part.length());
            if (isEos)
            {
                continue;
            }

            const QStringRef durationStr = part.mid(index, 4);
            duration = decodeSymbols(durationStr);

            return true;
        }

        return false;
    }
}

#pragma once

#include "../../corelib/core_face.h"

namespace core
{
	class coll_helper;

	enum class message_type;
}

namespace HistoryControl
{
	typedef std::shared_ptr<class FileSharingInfo> FileSharingInfoSptr;

	typedef std::shared_ptr<class StickerInfo> StickerInfoSptr;

	typedef std::shared_ptr<class ChatEventInfo> ChatEventInfoSptr;

    typedef std::shared_ptr<class VoipEventInfo> VoipEventInfoSptr;
}

namespace Logic
{
	class MessageKey;

    enum class preview_type;
}

namespace Data
{
    static const auto PRELOAD_MESSAGES_COUNT = 30;
    static const auto MORE_MESSAGES_COUNT = 30;

    struct Quote;

    typedef std::map<QString, QString, StringComparator> MentionMap;

	class MessageBuddy
	{
	public:
		MessageBuddy();

        void ApplyModification(const MessageBuddy &modification);

        bool IsEmpty() const;

		bool CheckInvariant() const;

        Logic::preview_type GetPreviewableLinkType() const;

        bool ContainsAnyPreviewableLink() const;

        bool ContainsPreviewableSiteLink() const;

        bool ContainsPttAudio() const;

        bool ContainsGif() const;

        bool ContainsImage() const;

        bool ContainsVideo() const;

        bool ContainsMentions() const;

        bool GetIndentWith(const MessageBuddy &buddy) const;

        bool hasAvatarWith(const MessageBuddy& _prevBuddy, const bool _isMultichat) const;

        bool isSameDirection(const MessageBuddy& _prevBuddy) const;

		bool IsBase() const;

		bool IsChatEvent() const;

        bool IsDeleted() const;

		bool IsDeliveredToClient() const;

		bool IsDeliveredToServer() const;

		bool IsFileSharing() const;

		bool IsOutgoing() const;

        bool IsOutgoingVoip() const;

		bool IsSticker() const;

		bool IsPending() const;

        bool IsServiceMessage() const;

        bool IsVoipEvent() const;

		const HistoryControl::ChatEventInfoSptr& GetChatEvent() const;

        const QString& GetChatSender() const;

		const QDate& GetDate() const;

		const HistoryControl::FileSharingInfoSptr& GetFileSharing() const;

        QStringRef GetFirstSiteLinkFromText() const;

        int GetPttDuration() const;

        bool GetIndentBefore() const;

		const HistoryControl::StickerInfoSptr& GetSticker() const;

		const QString& GetText() const;

		qint32 GetTime() const;

		qint64 GetLastId() const;

        core::message_type GetType() const;

        const HistoryControl::VoipEventInfoSptr& GetVoipEvent() const;

        bool HasAvatar() const;

        bool HasChatSender() const;

		bool HasId() const;

		bool HasText() const;

		void FillFrom(const MessageBuddy &buddy);

        void EraseEventData();

		void SetChatEvent(const HistoryControl::ChatEventInfoSptr& chatEvent);

        void SetChatSender(const QString& chatSender);

		void SetDate(const QDate &date);

        void SetDeleted(const bool isDeleted);

		void SetFileSharing(const HistoryControl::FileSharingInfoSptr& fileSharing);

        void SetHasAvatar(const bool hasAvatar);

        void SetIndentBefore(const bool indentBefore);

		void SetLastId(const qint64 lastId);

		void SetOutgoing(const bool isOutgoing);

		void SetSticker(const HistoryControl::StickerInfoSptr &sticker);

		void SetText(const QString &text);

		void SetTime(const qint32 time);

        void SetType(const core::message_type type);

        void SetVoipEvent(const HistoryControl::VoipEventInfoSptr &voipEvent);

		Logic::MessageKey ToKey() const;

		QString AimId_;
		QString InternalId_;
		qint64 Id_;
		qint64 Prev_;
        int PendingId_;
        qint32 Time_;
        QVector<Quote> Quotes_;
        MentionMap Mentions_;

		bool Chat_;
		QString ChatFriendly_;

		//filled by model
		bool Unread_;
		bool Filled_;

	private:
		qint64 LastId_;

		QString Text_;

        QString ChatSender_;

		QDate Date_;

        bool Deleted_;

		bool DeliveredToClient_;

        bool HasAvatar_;

        bool IndentBefore_;

		bool Outgoing_;

        core::message_type Type_;

		HistoryControl::FileSharingInfoSptr FileSharing_;

		HistoryControl::StickerInfoSptr Sticker_;

		HistoryControl::ChatEventInfoSptr ChatEvent_;

        HistoryControl::VoipEventInfoSptr VoipEvent_;

	};

	class DlgState
	{
	public:
		DlgState()
			: UnreadCount_(0)
			, LastMsgId_(-1)
			, YoursLastRead_(-1)
			, TheirsLastRead_(-1)
			, TheirsLastDelivered_(-1)
            , FavoriteTime_(-1)
			, Time_(-1)
			, Outgoing_(false)
			, Chat_(false)
			, Visible_(true)
            , Official_(false)
            , IsContact_(false)
            , IsLastMessageDelivered(true)
            , IsFromSearch_(false)
            , RequestId_(-1)
            , SearchedMsgId_(-1)
            , SearchPriority_(-1)
            , isFromGlobalSearch_(false)
            , hasMentionMe_(false)
            , unreadMentionsCount_(0)
		{
		}

		bool operator==(const DlgState& other) const
		{
			return AimId_ == other.AimId_;
		}

		const QString& GetText() const;

        bool HasLastMsgId() const;

        bool HasText() const;

		void SetText(QString text);

		QString AimId_;
		qint64 UnreadCount_;
		qint64 LastMsgId_;
		qint64 YoursLastRead_;
		qint64 TheirsLastRead_;
		qint64 TheirsLastDelivered_;
        qint64 FavoriteTime_;
		qint32 Time_;
		bool Outgoing_;
		bool Chat_;
		bool Visible_;
        bool Official_;
        bool IsContact_;
        bool IsLastMessageDelivered;
        QString senderAimId_;
		QString LastMessageFriendly_;
        QString senderNick_;
        QString Friendly_;
        bool IsFromSearch_;
        qint64 RequestId_;
        QString SearchTerm_;
        qint64 SearchedMsgId_;
        int SearchPriority_;
        QString MailId_;
        QString header_;
        bool isFromGlobalSearch_;

        bool hasMentionMe_;
        int unreadMentionsCount_;

	private:
		QString Text_;

	};

    struct Quote
    {
        enum Type
        {
            text = 1,
            link = 0,
            file_sharing = 2,
            quote = 3,
            other = 100
        };

        QString text_;
        QString senderId_;
        QString chatId_;
        QString senderFriendly_;
        QString chatStamp_;
        QString chatName_;
        qint32 time_;
        qint64 msgId_;
        int32_t setId_;
        int32_t stickerId_;
        bool isForward_;
        MentionMap mentions_;

        int id_;

        //gui only values
        bool isFirstQuote_;
        bool isLastQuote_;

        Quote::Type type_;

        Quote()
            : time_(-1)
            , msgId_(-1)
            , setId_(-1)
            , stickerId_(-1)
            , isForward_(false)
            , id_(-1)
            , isFirstQuote_(false)
            , isLastQuote_(false)
            , type_(Quote::Type::text)
        {
        }

        bool isEmpty() const { return text_.isEmpty() && !isSticker(); }
        bool isSticker() const { return setId_ != -1 && stickerId_ != -1; }

        void serialize(core::icollection* _collection) const;
        void unserialize(core::icollection* _collection);
    };

	typedef std::shared_ptr<MessageBuddy> MessageBuddySptr;
	typedef QVector<MessageBuddySptr> MessageBuddies;
	typedef std::shared_ptr<MessageBuddies> MessageBuddiesSptr;

    struct MessagesResult
    {
        QString aimId;
        MessageBuddies messages;
        MessageBuddies introMessages;
        MessageBuddies modifications;
        int64_t lastMsgId;
        bool havePending;
    };

    MessagesResult UnserializeMessageBuddies(core::coll_helper* helper, const QString &myAimid);

    Data::MessageBuddySptr unserializeMessage(
        core::coll_helper &msgColl,
        const QString &aimId,
        const QString &myAimid,
        const qint64 theirs_last_delivered,
        const qint64 theirs_last_read);

    struct ServerMessagesIds
    {
        QString AimId_;
        QVector<qint64> AllIds_;
        QVector<int64_t> DeletedIds_;
        Data::MessageBuddies UpdatedMessages_;
    };
    ServerMessagesIds UnserializeServerMessagesIds(const core::coll_helper& helper);

	void SerializeDlgState(core::coll_helper* helper, const DlgState& state);
	DlgState UnserializeDlgState(core::coll_helper* helper, const QString &myAimId, bool _from_search);
}

Q_DECLARE_METATYPE(Data::MessageBuddy);
Q_DECLARE_METATYPE(Data::DlgState);

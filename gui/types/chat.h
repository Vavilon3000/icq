#pragma once

class QPixmap;

namespace core
{
	class coll_helper;
}

namespace Data
{
	class ChatMemberInfo
	{
	public:

        bool operator==(const ChatMemberInfo& other) const
        {
            return AimId_ == other.AimId_;
        }

		QString AimId_;
		QString Role_;
		QString FirstName_;
		QString LastName_;
		QString NickName_;

        QString getFriendly() const;

		bool Friend_ = false;
		bool NoAvatar_ = false;
	};

	class ChatInfo
	{
    public:
        QString AimId_;
        QString Name_;
        QString Location_;
        QString About_;
        QString YourRole_;
        QString Owner_;
        QString MembersVersion_;
        QString InfoVersion_;
        QString Stamp_;
        QString Creator_;
        QString DefaultRole_;

        int32_t CreateTime_ = -1;
        int32_t MembersCount_ = - 1;
        int32_t FriendsCount = -1;
        int32_t BlockedCount_ = -1;
        int32_t PendingCount_ = -1;

        bool YouBlocked_ = false;
        bool YouPending_ = false;
        bool Public_ = false;
        bool ApprovedJoin_ = false;
        bool AgeRestriction_ = false;
        bool Live_ = false;
        bool Controlled_ = false;
        bool YouMember_ = false;

        QVector<ChatMemberInfo> Members_;
	};

    QVector<ChatMemberInfo> UnserializeChatMembers(core::coll_helper* helper);
    ChatInfo UnserializeChatInfo(core::coll_helper* helper);

    struct ChatResult
    {
        QVector<ChatInfo> chats;
        QString newTag;
        bool restart = false;
        bool finished = false;
    };

    ChatResult UnserializeChatHome(core::coll_helper* helper);
}

Q_DECLARE_METATYPE(Data::ChatMemberInfo*);
Q_DECLARE_METATYPE(Data::ChatInfo);
Q_DECLARE_METATYPE(QVector<Data::ChatInfo>);
Q_DECLARE_METATYPE(QVector<Data::ChatMemberInfo>);
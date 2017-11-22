#include "stdafx.h"
#include "chat.h"

#include "../../corelib/collection_helper.h"


namespace Data
{
    QString ChatMemberInfo::getFriendly() const
    {
        if (!NickName_.isEmpty())
            return NickName_;

        QString friendly;
        if (!FirstName_.isEmpty())
            friendly += FirstName_;

        if (!LastName_.isEmpty())
        {
            if (!friendly.isEmpty())
                friendly += ql1c(' ');

            friendly += LastName_;
        }

        if (!friendly.isEmpty())
            return friendly;

        return AimId_;
    }

    QVector<ChatMemberInfo> UnserializeChatMembers(core::coll_helper* helper)
    {
        core::iarray* membersArray = helper->get_value_as_array("members");
        const auto size = membersArray->size();
        QVector<ChatMemberInfo> members;
        members.reserve(size);
        for (int i = 0; i < size; ++i)
        {
            ChatMemberInfo member;
            core::coll_helper value(membersArray->get_at(i)->get_as_collection(), false);
            member.AimId_ = QString::fromUtf8(value.get_value_as_string("aimid"));
            if (value.is_value_exist("role"))
                member.Role_ = value.get_value_as_string("role");
            member.FirstName_ = QString::fromUtf8(value.get_value_as_string("first_name")).trimmed();
            member.LastName_ = QString::fromUtf8(value.get_value_as_string("last_name")).trimmed();
            member.NickName_ = QString::fromUtf8(value.get_value_as_string("nick_name")).trimmed();
            if (value.is_value_exist("friend"))
                member.Friend_ = value.get_value_as_bool("friend");
            if (value.is_value_exist("no_avatar"))
                member.NoAvatar_ = value.get_value_as_bool("no_avatar");
            members.push_back(std::move(member));
        }
        return members;
    }

    ChatInfo UnserializeChatInfo(core::coll_helper* helper)
	{
        ChatInfo info;
		info.AimId_ = QString::fromUtf8(helper->get_value_as_string("aimid"));
		info.Name_ = QString::fromUtf8(helper->get_value_as_string("name")).trimmed();
        info.Location_ = QString::fromUtf8(helper->get_value_as_string("location"));
        info.Stamp_ = QString::fromUtf8(helper->get_value_as_string("stamp"));
		info.About_ = QString::fromUtf8(helper->get_value_as_string("about"));
		info.YourRole_ = QString::fromUtf8(helper->get_value_as_string("your_role"));
		info.Owner_ = QString::fromUtf8(helper->get_value_as_string("owner"));
        info.Creator_ = QString::fromUtf8(helper->get_value_as_string("creator"));
        info.DefaultRole_ = QString::fromUtf8(helper->get_value_as_string("default_role"));
		info.MembersVersion_ = QString::fromUtf8(helper->get_value_as_string("members_version"));
		info.InfoVersion_ = QString::fromUtf8(helper->get_value_as_string("info_version"));
		info.CreateTime_ =  helper->get_value_as_int("create_time");
		info.MembersCount_ =  helper->get_value_as_int("members_count");
		info.FriendsCount =  helper->get_value_as_int("friend_count");
		info.BlockedCount_ =  helper->get_value_as_int("blocked_count");
        info.PendingCount_ =  helper->get_value_as_int("pending_count");
		info.YouBlocked_ = helper->get_value_as_bool("you_blocked");
        info.YouPending_ = helper->get_value_as_bool("you_pending");
        info.YouMember_ = helper->get_value_as_bool("you_member");
		info.Public_ = helper->get_value_as_bool("public");
		info.Live_ = helper->get_value_as_bool("live");
		info.Controlled_ = helper->get_value_as_bool("controlled");
        info.ApprovedJoin_ = helper->get_value_as_bool("joinModeration");
        info.AgeRestriction_ = helper->get_value_as_bool("age_restriction");
        info.Members_ = UnserializeChatMembers(helper);
        return info;
	}

    ChatResult UnserializeChatHome(core::coll_helper* helper)
    {
        ChatResult result;
        result.newTag = QString::fromUtf8(helper->get_value_as_string("new_tag"));
        result.restart = helper->get_value_as_bool("need_restart");
        result.finished = helper->get_value_as_bool("finished");
        core::iarray* chatsArray = helper->get_value_as_array("chats");
        const auto size = chatsArray->size();
        QVector<ChatInfo> chats;
        chats.reserve(size);
        for (int i = 0; i < chatsArray->size(); ++i)
        {
            core::coll_helper value(chatsArray->get_at(i)->get_as_collection(), false);
            chats.append(UnserializeChatInfo(&value));
        }
        result.chats = std::move(chats);
        return result;
    }
}
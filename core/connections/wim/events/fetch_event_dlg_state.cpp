#include "stdafx.h"

#include "fetch_event_dlg_state.h"
#include "../wim_history.h"
#include "../wim_im.h"
#include "../wim_packet.h"
#include "../../../archive/archive_index.h"
#include "../../../log/log.h"

using namespace core;
using namespace wim;

fetch_event_dlg_state::fetch_event_dlg_state()
    : tail_messages_(std::make_shared<archive::history_block>())
    , intro_messages_(std::make_shared<archive::history_block>())
{
}


fetch_event_dlg_state::~fetch_event_dlg_state()
{
}

template <typename T>
int64_t get_older_msg_id(const T& value)
{
    const auto iter_older_msgid = value.FindMember("olderMsgId");
    if (iter_older_msgid != value.MemberEnd() && iter_older_msgid->value.IsInt64())
        return iter_older_msgid->value.GetInt64();
    return -1;
}

int32_t fetch_event_dlg_state::parse(const rapidjson::Value& _node_event_data)
{
    const auto iter_sn = _node_event_data.FindMember("sn");
    const auto iter_last_msg_id = _node_event_data.FindMember("lastMsgId");

    if (
        iter_sn == _node_event_data.MemberEnd() ||
        iter_last_msg_id == _node_event_data.MemberEnd() ||
        !iter_sn->value.IsString() ||
        !iter_last_msg_id->value.IsInt64())
    {
        __TRACE(
            "delivery",
            "%1%",
            "failed to parse incoming dlg state event");

        return wpie_error_parse_response;
    }

    aimid_ = rapidjson_get_string(iter_sn->value);

    state_.set_last_msgid(iter_last_msg_id->value.GetInt64());

    const auto iter_unread_count = _node_event_data.FindMember("unreadCnt");
    if (iter_unread_count != _node_event_data.MemberEnd() && iter_unread_count->value.IsUint())
        state_.set_unread_count(iter_unread_count->value.GetUint());

    const auto iter_mme_count = _node_event_data.FindMember("unreadMentionMeCount");
    if (iter_mme_count != _node_event_data.MemberEnd() && iter_mme_count->value.IsUint())
        state_.set_unread_mentions_count(iter_mme_count->value.GetInt());

    const auto iter_yours = _node_event_data.FindMember("yours");
    if (iter_yours != _node_event_data.MemberEnd() && iter_yours->value.IsObject())
    {
        auto iter_last_read = iter_yours->value.FindMember("lastRead");
        if (iter_last_read != iter_yours->value.MemberEnd() && iter_last_read->value.IsInt64())
            state_.set_yours_last_read(iter_last_read->value.GetInt64());
    }

    const auto iter_theirs = _node_event_data.FindMember("theirs");
    if (iter_theirs != _node_event_data.MemberEnd() && iter_theirs->value.IsObject())
    {
        const auto iter_last_read = iter_theirs->value.FindMember("lastRead");
        if (iter_last_read != iter_theirs->value.MemberEnd() && iter_last_read->value.IsInt64())
            state_.set_theirs_last_read(iter_last_read->value.GetInt64());

        const auto iter_last_delivered = iter_theirs->value.FindMember("lastDelivered");
        if (iter_last_delivered != iter_theirs->value.MemberEnd() && iter_last_delivered->value.IsInt64())
            state_.set_theirs_last_delivered(iter_last_delivered->value.GetInt64());
    }

    const auto iter_patch_version = _node_event_data.FindMember("patchVersion");
    if (iter_patch_version != _node_event_data.MemberEnd())
    {
        std::string patch_version = rapidjson_get_string(iter_patch_version->value);
        assert(!patch_version.empty());

        state_.set_dlg_state_patch_version(std::move(patch_version));
    }

    const auto iter_del_up_to = _node_event_data.FindMember("delUpto");
    if ((iter_del_up_to != _node_event_data.MemberEnd()) &&
        iter_del_up_to->value.IsInt64())
    {
        state_.set_del_up_to(iter_del_up_to->value.GetInt64());
    }

    const core::archive::persons_map persons = parse_persons(_node_event_data);
    const auto iter_tail = _node_event_data.FindMember("tail");
    if (iter_tail != _node_event_data.MemberEnd() && iter_tail->value.IsObject())
    {
        const int64_t older_msg_id = get_older_msg_id(iter_tail->value);

        if (!parse_history_messages_json(iter_tail->value, older_msg_id, aimid_, *tail_messages_, persons, message_order::reverse))
        {
            __TRACE(
                "delivery",
                "%1%",
                "failed to parse incoming tail dlg state messages");

            return wpie_error_parse_response;
        }
    }

    const auto iter_intro = _node_event_data.FindMember("intro");
    if (iter_intro != _node_event_data.MemberEnd() && iter_intro->value.IsObject())
    {
        const int64_t older_msg_id = get_older_msg_id(iter_intro->value);

        if (!parse_history_messages_json(iter_intro->value, older_msg_id, aimid_, *intro_messages_, persons, message_order::direct))
        {
            __TRACE(
                "delivery",
                "%1%",
                "failed to parse incoming intro dlg state messages");

            return wpie_error_parse_response;
        }
    }

    auto iter_person = persons.find(aimid_);
    if (iter_person != persons.end())
    {
        state_.set_friendly(iter_person->second.friendly_);
        state_.set_official(iter_person->second.official_);
    }

    if (::build::is_debug())
    {
        __INFO(
            "delete_history",
            "incoming dlg state event\n"
            "    contact=<%1%>\n"
            "    dlg-patch=<%2%>\n"
            "    del-up-to=<%3%>",
            aimid_ % state_.get_dlg_state_patch_version() % state_.get_del_up_to()
            );

        __TRACE(
            "delivery",
            "parsed incoming tail dlg state event\n"
            "	size=<%1%>\n"
            "	last_msgid=<%2%>",
            tail_messages_->size() %
            state_.get_last_msgid());

        __TRACE(
            "delivery",
            "parsed incoming intro dlg state event\n"
            "	size=<%1%>\n"
            "	last_msgid=<%2%>",
            intro_messages_->size() %
            state_.get_last_msgid());

        for (const auto &message : *tail_messages_)
        {
            __TRACE(
                "delivery",
                "parsed incoming tail dlg state message\n"
                "	id=<%1%>",
                message->get_msgid());
        }

        for (const auto &message : *intro_messages_)
        {
            __TRACE(
                "delivery",
                "parsed incoming intro dlg state message\n"
                "	id=<%1%>",
                message->get_msgid());
        }
    }

    return 0;
}

void fetch_event_dlg_state::on_im(std::shared_ptr<core::wim::im> _im, std::shared_ptr<auto_callback> _on_complete)
{
    _im->on_event_dlg_state(this, _on_complete);
}

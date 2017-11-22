#include "stdafx.h"

#include "wim_contactlist_cache.h"
#include "wim_im.h"
#include "../../core.h"

#include "../../../corelib/core_face.h"
#include "../../../corelib/collection_helper.h"
#include "../../tools/system.h"

using namespace core;
using namespace wim;

namespace
{
    bool Contains(const std::string& first, const std::string& second)
    {
        return first.find(second) != std::string::npos;
    }
}

void cl_presence::serialize(icollection* _coll)
{
    coll_helper cl(_coll, false);
    cl.set_value_as_string("state", state_);
    cl.set_value_as_string("userType", usertype_);
    cl.set_value_as_string("statusMsg", status_msg_);
    cl.set_value_as_string("otherNumber", other_number_);
    cl.set_value_as_string("smsNumber", sms_number_);
    cl.set_value_as_string("friendly", friendly_);
    cl.set_value_as_string("abContactName", ab_contact_name_);
    cl.set_value_as_bool("is_chat", is_chat_);
    cl.set_value_as_bool("mute", muted_);
    cl.set_value_as_bool("official", official_);
    cl.set_value_as_int("lastseen", lastseen_);
    cl.set_value_as_int("outgoingCount", outgoing_msg_count_);
    cl.set_value_as_bool("livechat", is_live_chat_);
    cl.set_value_as_string("iconId", icon_id_);
    cl.set_value_as_string("bigIconId", big_icon_id_);
    cl.set_value_as_string("largeIconId", large_icon_id_);
}


void cl_presence::serialize(rapidjson::Value& _node, rapidjson_allocator& _a)
{
    _node.AddMember("state",  state_, _a);
    _node.AddMember("userType",  usertype_, _a);
    _node.AddMember("statusMsg",  status_msg_, _a);
    _node.AddMember("otherNumber",  other_number_, _a);
    _node.AddMember("smsNumber", sms_number_, _a);
    _node.AddMember("friendly",  friendly_, _a);
    _node.AddMember("abContactName",  ab_contact_name_, _a);
    _node.AddMember("lastseen",  lastseen_, _a);
    _node.AddMember("outgoingCount", outgoing_msg_count_, _a);
    _node.AddMember("mute",  muted_, _a);
    _node.AddMember("livechat", is_live_chat_ ? 1 : 0, _a);
    _node.AddMember("official", official_ ? 1 : 0, _a);
    _node.AddMember("iconId",  icon_id_, _a);
    _node.AddMember("bigIconId",  big_icon_id_, _a);
    _node.AddMember("largeIconId",  large_icon_id_, _a);

    if (!capabilities_.empty())
    {
        rapidjson::Value node_capabilities(rapidjson::Type::kArrayType);
        node_capabilities.Reserve(capabilities_.size(), _a);
        for (const auto& x : capabilities_)
        {
            rapidjson::Value capa;
            capa.SetString(x, _a);

            node_capabilities.PushBack(std::move(capa), _a);
        }

        _node.AddMember("capabilities", std::move(node_capabilities), _a);
    }
}

void cl_presence::unserialize(const rapidjson::Value& _node)
{
    search_cache_.clear();

    const auto end = _node.MemberEnd();

    const auto iter_state = _node.FindMember("state");
    const auto iter_user_type = _node.FindMember("userType");
    const auto iter_capabilities = _node.FindMember("capabilities");
    const auto iter_statusMsg = _node.FindMember("statusMsg");
    const auto iter_otherNumber = _node.FindMember("otherNumber");
    const auto iter_friendly = _node.FindMember("friendly");
    const auto iter_ab_contact_name = _node.FindMember("abContactName");
    const auto iter_last_seen = _node.FindMember("lastseen");
    const auto iter_outgoing_count = _node.FindMember("outgoingCount");
    const auto iter_mute = _node.FindMember("mute");
    const auto iter_livechat = _node.FindMember("livechat");
    const auto iter_official = _node.FindMember("official");
    const auto iter_iconId = _node.FindMember("iconId");
    const auto iter_bigIconId = _node.FindMember("bigIconId");
    const auto iter_largeIconId = _node.FindMember("largeIconId");
    const auto iter_sms_number = _node.FindMember("smsNumber");

    if (iter_state != end && iter_state->value.IsString())
        state_ = rapidjson_get_string(iter_state->value);

    if (iter_user_type != end && iter_user_type->value.IsString())
        usertype_ = rapidjson_get_string(iter_user_type->value);

    if (iter_statusMsg != end && iter_statusMsg->value.IsString())
        status_msg_ = rapidjson_get_string(iter_statusMsg->value);

    if (iter_otherNumber != end && iter_otherNumber->value.IsString())
        other_number_ = rapidjson_get_string(iter_otherNumber->value);

    if (iter_sms_number != end && iter_sms_number->value.IsString())
        sms_number_ = rapidjson_get_string(iter_sms_number->value);

    if (iter_friendly != end && iter_friendly->value.IsString())
        friendly_ = rapidjson_get_string(iter_friendly->value);

    if (iter_ab_contact_name != end && iter_ab_contact_name->value.IsString())
        ab_contact_name_ = rapidjson_get_string(iter_ab_contact_name->value);

    if (iter_last_seen != end && iter_last_seen->value.IsUint())
        lastseen_ = iter_last_seen->value.GetUint();

    if (iter_outgoing_count != end && iter_outgoing_count->value.IsUint())
        outgoing_msg_count_ = iter_outgoing_count->value.GetUint();

    if (iter_capabilities != end && iter_capabilities->value.IsArray())
    {
        for (auto iter = iter_capabilities->value.Begin(), iter_end = iter_capabilities->value.End(); iter != iter_end; ++iter)
            capabilities_.insert(rapidjson_get_string(*iter));
    }

    if (iter_mute != end)
    {
        if (iter_mute->value.IsUint())
            muted_ = iter_mute->value != 0;
        else if (iter_mute->value.IsBool())
            muted_ = iter_mute->value.GetBool();
    }


    if (iter_livechat != end)
        is_live_chat_ = iter_livechat->value != 0;

    if (iter_official != end)
    {
        if (iter_official->value.IsUint())
            official_ = iter_official->value.GetUint() == 1;
    }

    if (iter_iconId != end && iter_iconId->value.IsString())
        icon_id_ = rapidjson_get_string(iter_iconId->value);

    if (iter_bigIconId != end && iter_bigIconId->value.IsString())
        big_icon_id_ = rapidjson_get_string(iter_bigIconId->value);

    if (iter_largeIconId != end && iter_largeIconId->value.IsString())
        large_icon_id_ = rapidjson_get_string(iter_largeIconId->value);
}


void contactlist::update_cl(const contactlist& _cl)
{
    groups_ = _cl.groups_;

    std::map<std::string, int32_t> out_counts;
    for (const auto& contact: contacts_index_)
        out_counts[contact.first] = contact.second->presence_->outgoing_msg_count_;

    contacts_index_ = _cl.contacts_index_;

    if (!out_counts.empty())
    {
        for (auto& contact : contacts_index_)
        {
            const auto it = out_counts.find(contact.first);
            if (it != out_counts.end())
                contact.second->presence_->outgoing_msg_count_ = it->second;
        }
    }

    set_changed_status(contactlist::changed_status::full);
    set_need_update_cache(true);
}

void contactlist::update_ignorelist(const ignorelist_cache& _ignorelist)
{
    ignorelist_ = _ignorelist;

    set_changed_status(contactlist::changed_status::full);
    set_need_update_cache(true);
}

void contactlist::set_changed_status(changed_status _status) noexcept
{
    if (changed_status_ == changed_status::full && _status == changed_status::presence)
    {
        // don't drop status
        return;
    }
    changed_status_ = _status;
}

contactlist::changed_status contactlist::get_changed_status() const noexcept
{
    return changed_status_;
}


void contactlist::update_presence(const std::string& _aimid, const std::shared_ptr<cl_presence>& _presence)
{
    auto contact_presence = get_presence(_aimid);
    if (!contact_presence)
        return;

    if (_presence->friendly_ != contact_presence->friendly_ ||
        _presence->ab_contact_name_ != contact_presence->ab_contact_name_)
    {
        contact_presence->search_cache_.clear();
    }

    contact_presence->state_ = _presence->state_;
    contact_presence->usertype_ = _presence->usertype_;
    contact_presence->status_msg_ = _presence->status_msg_;
    contact_presence->other_number_ = _presence->other_number_;
    contact_presence->sms_number_ = _presence->sms_number_;
    contact_presence->capabilities_ = _presence->capabilities_;
    contact_presence->friendly_ = _presence->friendly_;
    contact_presence->lastseen_ = _presence->lastseen_;
    contact_presence->is_chat_ = _presence->is_chat_;
    contact_presence->muted_ = _presence->muted_;
    contact_presence->is_live_chat_ = _presence->is_live_chat_;
    contact_presence->official_ = _presence->official_;
    contact_presence->icon_id_ = _presence->icon_id_;
    contact_presence->big_icon_id_ = _presence->big_icon_id_;
    // no need to update outgoing_msg_count_ here
    {
        const auto large_icon_id = contact_presence->large_icon_id_;
        contact_presence->large_icon_id_ = _presence->large_icon_id_;
        need_update_avatar_ = (large_icon_id != contact_presence->large_icon_id_);
    }

    set_changed_status(contactlist::changed_status::presence);
}

void contactlist::serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const
{
    rapidjson::Value node_groups(rapidjson::Type::kArrayType);
    node_groups.Reserve(groups_.size(), _a);
    for (const auto& group : groups_)
    {
        rapidjson::Value node_group(rapidjson::Type::kObjectType);

        node_group.AddMember("name",  group->name_, _a);
        node_group.AddMember("id", group->id_, _a);

        rapidjson::Value node_buddies(rapidjson::Type::kArrayType);
        node_buddies.Reserve(group->buddies_.size(),_a);
        for (const auto& buddy : group->buddies_)
        {
            rapidjson::Value node_buddy(rapidjson::Type::kObjectType);

            node_buddy.AddMember("aimId",  buddy->aimid_, _a);

            buddy->presence_->serialize(node_buddy, _a);

            node_buddies.PushBack(std::move(node_buddy), _a);
        }

        node_group.AddMember("buddies", std::move(node_buddies), _a);
        node_groups.PushBack(std::move(node_group), _a);
    }

    _node.AddMember("groups", std::move(node_groups), _a);

    rapidjson::Value node_ignorelist(rapidjson::Type::kArrayType);
    node_ignorelist.Reserve(ignorelist_.size(), _a);
    for (const auto& _aimid : ignorelist_)
    {
        rapidjson::Value node_aimid(rapidjson::Type::kObjectType);

        node_aimid.AddMember("aimId", _aimid, _a);

        node_ignorelist.PushBack(std::move(node_aimid), _a);
    }

    _node.AddMember("ignorelist", std::move(node_ignorelist), _a);
}

void core::wim::contactlist::serialize(icollection* _coll, const std::string& type) const
{
    coll_helper cl(_coll, false);

    ifptr<iarray> groups_array(_coll->create_array());
    groups_array->reserve((int32_t)groups_.size());

    for (const auto& group : groups_)
    {
        coll_helper group_coll(_coll->create_collection(), true);
        group_coll.set_value_as_string("group_name", group->name_);
        group_coll.set_value_as_int("group_id", group->id_);
        group_coll.set_value_as_bool("added", group->added_);
        group_coll.set_value_as_bool("removed", group->removed_);

        ifptr<iarray> contacts_array(_coll->create_array());
        contacts_array->reserve((int32_t)group->buddies_.size());

        for (const auto& buddy : group->buddies_)
        {
            if (is_ignored(buddy->aimid_))
                continue;

            coll_helper contact_coll(_coll->create_collection(), true);
            contact_coll.set_value_as_string("aimId", buddy->aimid_);

            buddy->presence_->serialize(contact_coll.get());

            ifptr<ivalue> val_contact(_coll->create_value());
            val_contact->set_as_collection(contact_coll.get());
            contacts_array->push_back(val_contact.get());
        }

        ifptr<ivalue> val_group(_coll->create_value());
        val_group->set_as_collection(group_coll.get());
        groups_array->push_back(val_group.get());

        group_coll.set_value_as_array("contacts", contacts_array.get());
    }

    if (!type.empty())
        cl.set_value_as_string("type", type);

    cl.set_value_as_array("groups", groups_array.get());
}

std::vector<std::string> core::wim::contactlist::search(const std::vector<std::vector<std::string>>& search_patterns, int32_t fixed_patterns_count)
{
    std::vector<std::string> result;

    std::string base_word;
    for (const auto& symbol : search_patterns)
        base_word.append(symbol[0]);

    std::map< std::string, std::shared_ptr<cl_buddy> > result_cache;
    std::map< std::string, std::shared_ptr<cl_buddy> >::const_iterator iter = tmp_cache_.begin();

    bool check_first = search_patterns.size() >= 3;
    auto cur = 0u;
    for (const auto& p : search_patterns)
    {
        if (p[0] != " ")
            ++cur;
        else
            cur = 0;

        if (cur > 1)
        {
            check_first = false;
            break;
        }
    }

    while (g_core->is_valid_search() && iter != tmp_cache_.end())
    {
        if (is_ignored(iter->first) || iter->second->presence_->usertype_ == "sms")
        {
            ++iter;
            continue;
        }

        if (iter->second->presence_->search_cache_.is_empty())
        {
            iter->second->presence_->search_cache_.aimid_ = tools::system::to_upper(iter->second->aimid_);
            iter->second->presence_->search_cache_.friendly_ = tools::system::to_upper(iter->second->presence_->friendly_);
            iter->second->presence_->search_cache_.ab_ = tools::system::to_upper(iter->second->presence_->ab_contact_name_);
            iter->second->presence_->search_cache_.sms_number_ = iter->second->presence_->sms_number_;
            iter->second->presence_->search_cache_.friendly_words_ = tools::get_words(iter->second->presence_->search_cache_.friendly_);
            iter->second->presence_->search_cache_.ab_words_ = tools::get_words(iter->second->presence_->search_cache_.ab_);
        }

        const auto& aimId = iter->second->presence_->search_cache_.aimid_;
        const auto& friendly = iter->second->presence_->search_cache_.friendly_;
        const auto& ab = iter->second->presence_->search_cache_.ab_;
        const auto& number = iter->second->presence_->search_cache_.sms_number_;
        const auto& friendly_words = iter->second->presence_->search_cache_.friendly_words_;
        const auto& ab_words = iter->second->presence_->search_cache_.ab_words_;

        auto check = [this, &result_cache, &result](std::map< std::string, std::shared_ptr<cl_buddy> >::const_iterator iter,
                                                       const std::vector<std::vector<std::string>>& search_patterns,
                                                       const std::string& word,
                                                       int32_t fixed_patterns_count)
        {
            if (word.empty())
                return false;

            int32_t priority = -1;
            if (tools::contains(search_patterns, word, fixed_patterns_count, priority))
            {
                result_cache.insert(std::make_pair(iter->first, iter->second));
                if (search_priority_.find(iter->first) != search_priority_.end())
                {
                    search_priority_[iter->first] = std::min(search_priority_[iter->first], priority);
                    return true;
                }
                else
                {
                    search_priority_.insert(std::make_pair(iter->first, priority));
                    result.push_back(iter->first);
                    return true;
                }
            }

            return false;
        };

        auto check_multi = [this, &result_cache, &result](std::map< std::string, std::shared_ptr<cl_buddy> >::const_iterator iter,
            std::vector<std::vector<std::string>> search_patterns,
            const std::vector<std::string> word,
            int32_t fixed_patterns_count)
        {
            if (word.empty())
                return false;

            int32_t priority = -1;

            auto w = word.begin();
            while (w != word.end())
            {
                std::vector<std::vector<std::string>> pat;
                std::vector<std::vector<std::string>>::iterator is = search_patterns.begin();
                while (is != search_patterns.end())
                {
                    if (!is->empty() && (*is)[0] == " ")
                    {
                        is = search_patterns.erase(is);
                        break;
                    }

                    pat.push_back(*is);
                    is = search_patterns.erase(is);
                }

                if (!tools::contains(pat, *w, fixed_patterns_count, priority, true))
                    return false;

                ++w;
            }

            result_cache.insert(std::make_pair(iter->first, iter->second));
            if (search_priority_.find(iter->first) != search_priority_.end())
            {
                search_priority_[iter->first] = std::min(search_priority_[iter->first], 0);
            }
            else
            {
                search_priority_.insert(std::make_pair(iter->first, 0));
                result.push_back(iter->first);
            }

            return true;
        };

        auto check_first_chars = [this, &result_cache, &result](std::map< std::string, std::shared_ptr<cl_buddy> >::const_iterator iter,
            std::vector<std::vector<std::string>> search_patterns,
            const std::vector<std::string> word,
            int32_t fixed_patterns_count)
        {
            if (word.empty())
                return false;

            auto w = word.rbegin();
            while (w != word.rend())
            {
                std::vector<std::vector<std::string>> pat;
                std::vector<std::vector<std::string>>::iterator is = search_patterns.begin();
                while (is != search_patterns.end())
                {
                    if (!is->empty() && (*is)[0] == " ")
                    {
                        is = search_patterns.erase(is);
                        break;
                    }

                    pat.push_back(*is);
                    is = search_patterns.erase(is);
                }


                if (pat.empty())
                    return false;

                auto ch = core::tools::from_utf8(*w)[0];
                bool found = false;
                for (const auto& s : pat[0])
                {
                    if (core::tools::from_utf8(s)[0] == ch)
                    {
                        found = true;
                        break;
                    }
                }

                if (!found)
                    return false;

                ++w;
            }

            result_cache.insert(std::make_pair(iter->first, iter->second));
            if (search_priority_.find(iter->first) != search_priority_.end())
            {
                search_priority_[iter->first] = std::min(search_priority_[iter->first], 0);
            }
            else
            {
                search_priority_.insert(std::make_pair(iter->first, 0));
                result.push_back(iter->first);
            }

            return true;
        };

        check(iter, search_patterns, friendly, fixed_patterns_count);
        check(iter, search_patterns, ab, fixed_patterns_count);
        check(iter, search_patterns, aimId, fixed_patterns_count);
        check(iter, search_patterns, number, fixed_patterns_count);

        check_multi(iter, search_patterns, friendly_words, fixed_patterns_count);
        if (check_first)
            check_first_chars(iter, search_patterns, friendly_words, fixed_patterns_count);
        check_multi(iter, search_patterns, ab_words, fixed_patterns_count);
        if (check_first)
            check_first_chars(iter, search_patterns, ab_words, fixed_patterns_count);

        ++iter;
    }


    if (g_core->is_valid_search())
    {
        for (auto _iter : result_cache)
        {
            search_cache_.insert(_iter);
        }

        if (g_core->end_search() == 0)
            last_search_patterns_ = base_word;

        return result;
    }

    last_search_patterns_.clear();
    g_core->end_search();
    return std::vector<std::string>();
}

std::vector<std::string> core::wim::contactlist::search(const std::string& search_pattern, bool first, int32_t search_priority, int32_t fixed_patterns_count)
{
    std::vector<std::string> result;
    if (first)
        search_priority_.clear();

    std::string base_word = search_pattern;

    if (first)
    {
        if (last_search_patterns_.empty() || base_word.find(last_search_patterns_.c_str()) == std::string::npos || last_search_patterns_[last_search_patterns_.length() - 1] == ' ' || need_update_search_cache_)
        {
            tmp_cache_ = contacts_index_;
            set_need_update_cache(false);
            search_cache_.clear();
        }
        else
        {
            tmp_cache_ = search_cache_;
        }
    }

    std::map< std::string, std::shared_ptr<cl_buddy> > result_cache;
    std::map< std::string, std::shared_ptr<cl_buddy> >::const_iterator iter = tmp_cache_.begin();

    auto patterns = core::tools::get_words(search_pattern);
    bool check_first = patterns.size() >= 2;
    for (const auto& p : patterns)
    {
        if (p.size() != 1)
        {
            check_first = false;
            break;
        }
    }

    while (g_core->is_valid_search() && iter != tmp_cache_.end() && !search_pattern.empty())
    {
        if (is_ignored(iter->first) || iter->second->presence_->usertype_ == "sms")
        {
            ++iter;
            continue;
        }

        if (iter->second->presence_->search_cache_.is_empty())
        {
            iter->second->presence_->search_cache_.aimid_ = tools::system::to_upper(iter->second->aimid_);
            iter->second->presence_->search_cache_.friendly_ = tools::system::to_upper(iter->second->presence_->friendly_);
            iter->second->presence_->search_cache_.ab_ = tools::system::to_upper(iter->second->presence_->ab_contact_name_);
            iter->second->presence_->search_cache_.sms_number_ = iter->second->presence_->sms_number_;
            iter->second->presence_->search_cache_.friendly_words_ = tools::get_words(iter->second->presence_->search_cache_.friendly_);
            iter->second->presence_->search_cache_.ab_words_ = tools::get_words(iter->second->presence_->search_cache_.ab_);
        }

        const auto& aimId = iter->second->presence_->search_cache_.aimid_;
        const auto& friendly = iter->second->presence_->search_cache_.friendly_;
        const auto& ab = iter->second->presence_->search_cache_.ab_;
        const auto& number = iter->second->presence_->search_cache_.sms_number_;
        const auto& friendly_words = iter->second->presence_->search_cache_.friendly_words_;
        const auto& ab_words = iter->second->presence_->search_cache_.ab_words_;

        auto check = [this, &result_cache, &result, search_priority](std::map< std::string, std::shared_ptr<cl_buddy> >::const_iterator iter,
            const std::string& search_pattern,
            const std::string& word,
            int32_t /*fixed_patterns_count*/)
        {
            auto pos = word.find(search_pattern);
            if (pos != std::string::npos)
            {
                int32_t priority = (word.length() == search_pattern.length() || pos == 0 || word[pos - 1] == ' ') ? 0 : 1;
                result_cache.insert(std::make_pair(iter->first, iter->second));
                if (search_priority_.find(iter->first) != search_priority_.end())
                {
                    search_priority_[iter->first] = std::min(search_priority_[iter->first], priority);
                    return true;
                }
                else
                {
                    search_priority_.insert(std::make_pair(iter->first, priority));
                    result.push_back(iter->first);
                    return true;
                }
            }

            return false;
        };

        check(iter, search_pattern, friendly, fixed_patterns_count);
        check(iter, search_pattern, ab, fixed_patterns_count);
        check(iter, search_pattern, aimId, fixed_patterns_count);
        check(iter, search_pattern, number, fixed_patterns_count);

        auto check_multi = [this, &result_cache, &result, search_priority](std::map< std::string, std::shared_ptr<cl_buddy> >::const_iterator iter,
            const std::vector<std::string>& search_pattern,
            const std::vector<std::string>& word,
            int32_t fixed_patterns_count)
        {
            for (auto iter_search = search_pattern.begin(), iter_word = word.begin(); iter_search != search_pattern.end() && iter_word != word.end();)
            {
                if (iter_word->find(*iter_search) != 0)
                    break;

                if (++iter_word == word.end())
                {
                    result_cache.insert(std::make_pair(iter->first, iter->second));
                    if (search_priority_.find(iter->first) != search_priority_.end())
                    {
                        search_priority_[iter->first] = std::min(search_priority_[iter->first], 0);
                        return true;
                    }
                    else
                    {
                        search_priority_.insert(std::make_pair(iter->first, 0));
                        result.push_back(iter->first);
                        return true;
                    }
                }

                ++iter_search;
            }

            return false;
        };

        auto check_first_chars = [this, &result_cache, &result, search_priority](std::map< std::string, std::shared_ptr<cl_buddy> >::const_iterator iter,
            const std::vector<std::string>& search_pattern,
            const std::vector<std::string>& word,
            int32_t fixed_patterns_count)
        {
            auto found = true;
            auto iter_search = search_pattern.rbegin();
            auto iter_word = word.begin();
            while (iter_search != search_pattern.rend() && iter_word != word.end())
            {
                auto f = core::tools::from_utf8(*iter_word);
                auto s = core::tools::from_utf8(*iter_search);
                if (f.length() > 0 && s.length() > 0 && f[0] != s[0])
                {
                    found = false;
                    break;
                }

                ++iter_word;
                ++iter_search;
            }

            if (found)
            {
                result_cache.insert(std::make_pair(iter->first, iter->second));
                if (search_priority_.find(iter->first) != search_priority_.end())
                {
                    search_priority_[iter->first] = std::min(search_priority_[iter->first], 0);
                    return true;
                }
                else
                {
                    search_priority_.insert(std::make_pair(iter->first, 0));
                    result.push_back(iter->first);
                    return true;
                }
            }

            return false;
        };

        if (friendly_words.size() == patterns.size())
        {
            check_multi(iter, patterns, friendly_words, fixed_patterns_count);
            if (check_first)
                check_first_chars(iter, patterns, friendly_words, fixed_patterns_count);
        }

        if (ab_words.size() == patterns.size())
        {
            check_multi(iter, patterns, ab_words, fixed_patterns_count);
            if (check_first)
                check_first_chars(iter, patterns, ab_words, fixed_patterns_count);
        }

        ++iter;
    }

    if (g_core->is_valid_search())
    {
        if (first)
            search_cache_.clear();

        for (const auto& _iter : result_cache)
        {
            search_cache_.insert(_iter);
        }

        return result;
    }

    last_search_patterns_.clear();
    return std::vector<std::string>();
}

void core::wim::contactlist::serialize_search(icollection* _coll) const
{
    coll_helper cl(_coll, false);

    ifptr<iarray> contacts_array(_coll->create_array());
    contacts_array->reserve((int32_t)search_cache_.size());

    for (const auto& buddy : search_cache_)
    {
        coll_helper contact_coll(_coll->create_collection(), true);
        contact_coll.set_value_as_string("aimId", buddy.first);
        ifptr<ivalue> val_contact(_coll->create_value());
        val_contact->set_as_collection(contact_coll.get());
        contacts_array->push_back(val_contact.get());
    }
    cl.set_value_as_array("contacts", contacts_array.get());
}

void contactlist::serialize_ignorelist(icollection* _coll) const
{
    coll_helper cl(_coll, false);

    ifptr<iarray> ignore_array(cl->create_array());
    ignore_array->reserve(int32_t(ignorelist_.size()));
    for (const auto& x : ignorelist_)
    {
        coll_helper coll_chatter(cl->create_collection(), true);
        ifptr<ivalue> val(cl->create_value());
        val->set_as_string(x.c_str(), (int32_t) x.length());
        ignore_array->push_back(val.get());
    }

    cl.set_value_as_array("aimids", ignore_array.get());
}

void contactlist::serialize_contact(const std::string& _aimid, icollection* _coll) const
{
    coll_helper coll(_coll, false);

    ifptr<iarray> groups_array(_coll->create_array());

    for (const auto& _group : groups_)
    {
        for (const auto& _buddy : _group->buddies_)
        {
            if (_buddy->aimid_ == _aimid)
            {
                coll_helper coll_group(_coll->create_collection(), true);
                coll_group.set_value_as_string("group_name", _group->name_);
                coll_group.set_value_as_int("group_id", _group->id_);
                coll_group.set_value_as_bool("added", _group->added_);
                coll_group.set_value_as_bool("removed", _group->removed_);

                ifptr<iarray> contacts_array(_coll->create_array());

                coll_helper coll_contact(coll->create_collection(), true);
                coll_contact.set_value_as_string("aimId", _buddy->aimid_);

                _buddy->presence_->serialize(coll_contact.get());

                ifptr<ivalue> val_contact(_coll->create_value());
                val_contact->set_as_collection(coll_contact.get());
                contacts_array->push_back(val_contact.get());

                ifptr<ivalue> val_group(_coll->create_value());
                val_group->set_as_collection(coll_group.get());
                groups_array->push_back(val_group.get());

                coll_group.set_value_as_array("contacts", contacts_array.get());

                coll.set_value_as_array("groups", groups_array.get());

                return;
            }
        }
    }
}

std::string contactlist::get_contact_friendly_name(const std::string& contact_login) const
{
    auto presence = get_presence(contact_login);
    if (presence)
        return presence->friendly_;
    return std::string();
}

int32_t contactlist::unserialize(const rapidjson::Value& _node)
{
    static long buddy_id = 0;

    static const std::string chat_domain = "@chat.agent";

    const auto node_end = _node.MemberEnd();
    const auto iter_groups = _node.FindMember("groups");
    if (iter_groups == node_end || !iter_groups->value.IsArray())
        return 0;

    for (auto iter_grp = iter_groups->value.Begin(), grp_end = iter_groups->value.End(); iter_grp != grp_end; ++iter_grp)
    {
        auto group = std::make_shared<core::wim::cl_group>();

        const auto iter_grp_end = iter_grp->MemberEnd();
        const auto iter_group_name = iter_grp->FindMember("name");
        if (iter_group_name != iter_grp_end && iter_group_name->value.IsString())
        {
            group->name_ = rapidjson_get_string(iter_group_name->value);
        }
        else
        {
            assert(false);
        }

        const auto iter_group_id = iter_grp->FindMember("id");
        if (iter_group_id == iter_grp_end || !iter_group_id->value.IsUint())
        {
            assert(false);
            continue;
        }

        const auto iter_buddies = iter_grp->FindMember("buddies");
        if (iter_buddies != iter_grp_end && iter_buddies->value.IsArray())
        {
            for (auto iter_bd = iter_buddies->value.Begin(), bd_end = iter_buddies->value.End(); iter_bd != bd_end; ++iter_bd)
            {
                auto buddy = std::make_shared<wim::cl_buddy>();

                buddy->id_ = (uint32_t)++buddy_id;

                const auto iter_bd_end = iter_bd->MemberEnd();
                const auto iter_aimid = iter_bd->FindMember("aimId");
                if (iter_aimid == iter_bd_end || !iter_aimid->value.IsString())
                {
                    assert(false);
                    continue;
                }

                buddy->aimid_ = rapidjson_get_string(iter_aimid->value);
                buddy->presence_->unserialize(*iter_bd);

                if (buddy->aimid_.length() > chat_domain.length() && buddy->aimid_.substr(buddy->aimid_.length() - chat_domain.length(), chat_domain.length()) == chat_domain)
                    buddy->presence_->is_chat_ = true;

                contacts_index_[buddy->aimid_] = buddy;

                group->buddies_.push_back(std::move(buddy));
            }
        }

        group->id_ = iter_group_id->value.GetUint();
        groups_.push_back(std::move(group));
    }

    const auto iter_ignorelist = _node.FindMember("ignorelist");
    if (iter_ignorelist == node_end || !iter_ignorelist->value.IsArray())
        return 0;

    for (auto iter_ignore = iter_ignorelist->value.Begin(); iter_ignore != iter_ignorelist->value.End(); ++iter_ignore)
    {
        auto iter_aimid = iter_ignore->FindMember("aimId");

        if (iter_aimid != iter_ignore->MemberEnd() || iter_aimid->value.IsString())
        {
            ignorelist_.emplace(rapidjson_get_string(iter_aimid->value));
        }
    }

    return 0;
}

int32_t contactlist::unserialize_from_diff(const rapidjson::Value& _node)
{
    static long buddy_id = 0;

    static const std::string chat_domain = "@chat.agent";

    for (auto iter_grp = _node.Begin(), grp_end = _node.End(); iter_grp != grp_end; ++iter_grp)
    {
        auto group = std::make_shared<core::wim::cl_group>();

        const auto iter_grp_end = iter_grp->MemberEnd();
        const auto iter_group_name = iter_grp->FindMember("name");
        if (iter_group_name != iter_grp_end && iter_group_name->value.IsString())
        {
            group->name_ = rapidjson_get_string(iter_group_name->value);
        }
        else
        {
            assert(false);
        }

        const auto iter_group_id = iter_grp->FindMember("id");
        if (iter_group_id == iter_grp_end || !iter_group_id->value.IsUint())
        {
            assert(false);
            continue;
        }

        group->id_ = iter_group_id->value.GetUint();

        const auto iter_group_added = iter_grp->FindMember("added");
        if (iter_group_added != iter_grp_end)
        {
            group->added_ = true;
        }

        const auto iter_group_removed = iter_grp->FindMember("removed");
        if (iter_group_removed != iter_grp_end)
        {
            group->removed_ = true;
        }
        const auto iter_buddies = iter_grp->FindMember("buddies");
        if (iter_buddies != iter_grp_end && iter_buddies->value.IsArray())
        {
            for (auto iter_bd = iter_buddies->value.Begin(), bd_end = iter_buddies->value.End(); iter_bd != bd_end; ++iter_bd)
            {
                auto buddy = std::make_shared<wim::cl_buddy>();

                buddy->id_ = (uint32_t)++buddy_id;

                const auto iter_aimid = iter_bd->FindMember("aimId");
                if (iter_aimid == iter_bd->MemberEnd() || !iter_aimid->value.IsString())
                {
                    assert(false);
                    continue;
                }

                buddy->aimid_ = rapidjson_get_string(iter_aimid->value);
                buddy->presence_->unserialize(*iter_bd);

                if (buddy->aimid_.length() > chat_domain.length() && buddy->aimid_.substr(buddy->aimid_.length() - chat_domain.length(), chat_domain.length()) == chat_domain)
                    buddy->presence_->is_chat_ = true;

                contacts_index_[buddy->aimid_] = buddy;

                group->buddies_.push_back(std::move(buddy));
            }
        }

        groups_.push_back(std::move(group));
    }

    return 0;
}

namespace
{
    struct int_count
    {
        explicit int_count() : count{ 0 } {}
        int count;

        int_count& operator++()
        {
            ++count;
            return *this;
        }
    };
}

void contactlist::merge_from_diff(const std::string& _type, const std::shared_ptr<contactlist>& _diff, const std::shared_ptr<std::list<std::string>>& removedContacts)
{
    changed_status status = changed_status::none;
    if (_type == "created")
    {
        status = changed_status::full;
        for (const auto& diff_group : _diff->groups_)
        {
            const auto diff_group_id = diff_group->id_;
            if (diff_group->added_)
            {
                auto group = std::make_shared<cl_group>();
                group->id_ = diff_group_id;
                group->name_ = diff_group->name_;
                groups_.push_back(std::move(group));
            }

            for (const auto& group : groups_)
            {
                if (group->id_ != diff_group_id)
                    continue;

                for (const auto& diff_buddy : diff_group->buddies_)
                {
                    group->buddies_.push_back(diff_buddy);
                    contacts_index_[diff_buddy->aimid_] = diff_buddy;
                }
            }
        }
    }
    else if (_type == "updated")
    {
        status = changed_status::presence;
        for (const auto& diff_group : _diff->groups_)
        {
            for (const auto& buddie : diff_group->buddies_)
                update_presence(buddie->aimid_, buddie->presence_);
        }
    }
    else if (_type == "deleted")
    {
        status = changed_status::full;
        std::map<std::string, int_count> contacts;

        for (const auto& diff_group : _diff->groups_)
        {
            const auto diff_group_id = diff_group->id_;
            for (auto group_iter = groups_.begin(); group_iter != groups_.end();)
            {
                if ((*group_iter)->id_ != diff_group_id)
                {
                    for (const auto& diff_buddy : diff_group->buddies_)
                    {
                        const auto& aimId = diff_buddy->aimid_;
                        const auto& group_buddies = (*group_iter)->buddies_;
                        const auto contains = std::any_of(group_buddies.begin(), group_buddies.end(), [&aimId](const std::shared_ptr<cl_buddy>& value) { return value->aimid_ == aimId; });
                        if (contains)
                            ++contacts[aimId];
                    }

                    ++group_iter;
                    continue;
                }

                for (const auto& diff_buddy : diff_group->buddies_)
                {
                    const auto& aimId = diff_buddy->aimid_;
                    auto& group_buddies = (*group_iter)->buddies_;
                    const auto end = group_buddies.end();
                    group_buddies.erase(std::remove_if(group_buddies.begin(), end,
                        [&aimId, &contacts](const std::shared_ptr<cl_buddy>& value)
                        {
                            if (value->aimid_ == aimId)
                            {
                                ++contacts[aimId];
                                return true;
                            }
                            return false;
                        }
                    ), end);
                }

                if (diff_group->removed_)
                    group_iter = groups_.erase(group_iter);
                else
                    ++group_iter;
            }
        }

        for (const auto& c : contacts)
        {
            if (c.second.count == 1)
            {
                contacts_index_.erase(c.first);
                removedContacts->push_back(c.first);
            }
        }
    }
    else
    {
        return;
    }
    set_changed_status(status);
    set_need_update_cache(true);
}

int32_t contactlist::get_contacts_count() const
{
    return contacts_index_.size();
}

int32_t contactlist::get_phone_contacts_count() const
{
    return std::count_if(contacts_index_.begin(), contacts_index_.end(), [](const std::pair<std::string, std::shared_ptr<cl_buddy>>& contact)
    { return contact.second->aimid_.find('+') != std::string::npos; });
}

int32_t contactlist::get_groupchat_contacts_count() const
{
    return std::count_if(contacts_index_.begin(), contacts_index_.end(), [](const std::pair<std::string, std::shared_ptr<cl_buddy>>& contact)
    { return contact.second->aimid_.find("@chat.agent") != std::string::npos; });
}

void contactlist::add_to_ignorelist(const std::string& _aimid)
{
    const auto res = ignorelist_.emplace(_aimid);
    if (res.second)
        set_changed_status(changed_status::full);
}

void contactlist::remove_from_ignorelist(const std::string& _aimid)
{
    const auto iter = ignorelist_.find(_aimid);
    if (iter != ignorelist_.end())
    {
        ignorelist_.erase(iter);
        set_changed_status(changed_status::full);
    }
}

std::shared_ptr<cl_presence> core::wim::contactlist::get_presence(const std::string & _aimid) const
{
    auto it = contacts_index_.find(_aimid);
    if (it != contacts_index_.end() && it->second->presence_)
        return it->second->presence_;

    return nullptr;
}

void contactlist::set_outgoing_msg_count(const std::string& _contact, int32_t _count)
{
    if (_count != -1)
    {
        auto presence = get_presence(_contact);
        if (presence)
        {
            presence->outgoing_msg_count_ = _count;
            set_changed_status(changed_status::presence);
        }
    }
}

std::shared_ptr<cl_group> contactlist::get_first_group() const
{
    if (groups_.empty())
    {
        return nullptr;
    }

    return (*groups_.begin());
}

bool contactlist::is_ignored(const std::string& _aimid) const
{
    return (ignorelist_.find(_aimid) != ignorelist_.end());
}

void contactlist::set_need_update_cache(bool _need_update_search_cache)
{
    need_update_search_cache_ = _need_update_search_cache;
}

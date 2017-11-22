#include "stdafx.h"
#include "search_contacts_response.h"

using namespace core;
using namespace wim;

search_contacts_response::search_contacts_response(): finish_(false)
{
    //
}

bool search_contacts_response::unserialize_contacts(const rapidjson::Value& _node)
{
    auto contacts_node = _node.FindMember("data");
    if (contacts_node != _node.MemberEnd())
    {
        if (!contacts_node->value.IsArray())
            return false;

        contacts_data_.clear();
        for (auto node = contacts_node->value.Begin(), nend = contacts_node->value.End(); node != nend; ++node)
        {
            contact_chunk c;
            { auto i = node->FindMember("sn"); if (i != node->MemberEnd() && i->value.IsString()) c.aimid_ = rapidjson_get_string(i->value); }
            { auto i = node->FindMember("stamp"); if (i != node->MemberEnd() && i->value.IsString()) c.stamp_ = rapidjson_get_string(i->value); }
            { auto i = node->FindMember("type"); if (i != node->MemberEnd() && i->value.IsString()) c.type_ = rapidjson_get_string(i->value); }
            { auto i = node->FindMember("score"); if (i != node->MemberEnd() && i->value.IsInt()) c.score_ = i->value.GetInt(); }
            {
                auto a = node->FindMember("anketa");
                if (a != node->MemberEnd() && a->value.IsObject())
                {
                    const auto& sub_node = a->value;
                    { auto i = sub_node.FindMember("firstName"); if (i != sub_node.MemberEnd() && i->value.IsString()) c.first_name_ = rapidjson_get_string(i->value); }
                    { auto i = sub_node.FindMember("lastName"); if (i != sub_node.MemberEnd() && i->value.IsString()) c.last_name_ = rapidjson_get_string(i->value); }
                    { auto i = sub_node.FindMember("nickname"); if (i != sub_node.MemberEnd() && i->value.IsString()) c.nick_name_ = rapidjson_get_string(i->value); }
                    { auto i = sub_node.FindMember("city"); if (i != sub_node.MemberEnd() && i->value.IsString()) c.city_ = rapidjson_get_string(i->value); }
                    { auto i = sub_node.FindMember("state"); if (i != sub_node.MemberEnd() && i->value.IsString()) c.state_ = rapidjson_get_string(i->value); }
                    { auto i = sub_node.FindMember("country"); if (i != sub_node.MemberEnd() && i->value.IsString()) c.country_ = rapidjson_get_string(i->value); }
                    { auto i = sub_node.FindMember("gender"); if (i != sub_node.MemberEnd() && i->value.IsString()) c.gender_ = rapidjson_get_string(i->value); }
                    {
                        auto b = sub_node.FindMember("birthDate");
                        if (b != sub_node.MemberEnd() && b->value.IsObject())
                        {
                            const auto& sub_sub_node = b->value;
                            { auto i = sub_sub_node.FindMember("year"); if (i != sub_sub_node.MemberEnd() && i->value.IsInt()) c.birthdate_.year_ = i->value.GetInt(); }
                            { auto i = sub_sub_node.FindMember("month"); if (i != sub_sub_node.MemberEnd() && i->value.IsInt()) c.birthdate_.month_ = i->value.GetInt(); }
                            { auto i = sub_sub_node.FindMember("day"); if (i != sub_sub_node.MemberEnd() && i->value.IsInt()) c.birthdate_.day_ = i->value.GetInt(); }
                        }
                    }
                    { auto i = sub_node.FindMember("aboutMeCut"); if (i != sub_node.MemberEnd() && i->value.IsString()) c.about_ = rapidjson_get_string(i->value); }
                }
                //{ auto i = node->FindMember("mutualCount"); if (i != node->MemberEnd() && i->value.IsInt()) c.score_ = i->value.GetInt(); }//TODO fix name when server side is done
            }
            contacts_data_.emplace_back(std::move(c));
        }
    }
    return true;
}

bool search_contacts_response::unserialize_chats(const rapidjson::Value& _node)
{
    auto chats_node = _node.FindMember("chats");
    if (chats_node != _node.MemberEnd())
    {
        if (!chats_node->value.IsArray())
            return false;

        chats_data_.clear();
        for (auto node = chats_node->value.Begin(), nend = chats_node->value.End(); node != nend; ++node)
        {
            chat_info c;
            c.unserialize(*node);
            chats_data_.emplace_back(std::move(c));
        }
    }
    return true;
}


void search_contacts_response::serialize_contacts(core::coll_helper _root_coll) const
{
    if (contacts_data_.empty())
        return;

    ifptr<iarray> array(_root_coll->create_array());
    array->reserve((int32_t)contacts_data_.size());
    for (const auto& c : contacts_data_)
    {
        coll_helper coll(_root_coll->create_collection(), true);

        coll.set_value_as_string("aimid", c.aimid_);
        coll.set_value_as_string("stamp", c.stamp_);
        coll.set_value_as_string("type", c.type_);
        if (c.score_ >= 0)
        {
            coll.set_value_as_int("score", c.score_);
        }
        coll.set_value_as_string("first_name", c.first_name_);
        coll.set_value_as_string("last_name", c.last_name_);
        coll.set_value_as_string("nick_name", c.nick_name_);
        coll.set_value_as_string("city", c.city_);
        coll.set_value_as_string("state", c.state_);
        coll.set_value_as_string("country", c.country_);
        coll.set_value_as_string("gender", c.gender_);
        if (c.birthdate_.year_ >= 0 && c.birthdate_.month_ >= 0 && c.birthdate_.day_ >= 0)
        {
            coll_helper b(_root_coll->create_collection(), true);
            b.set_value_as_int("year", c.birthdate_.year_);
            b.set_value_as_int("month", c.birthdate_.month_);
            b.set_value_as_int("day", c.birthdate_.day_);
            coll.set_value_as_collection("birthdate", b.get());
        }
        coll.set_value_as_string("about", c.about_);
        coll.set_value_as_int("mutual_count", c.mutual_friend_count_);

        ifptr<ivalue> val(_root_coll->create_value());
        val->set_as_collection(coll.get());
        array->push_back(val.get());
    }

    _root_coll.set_value_as_array("data", array.get());
}

void core::wim::search_contacts_response::serialize_chats(core::coll_helper _root_coll) const
{
    if (chats_data_.empty())
        return;

    ifptr<iarray> array(_root_coll->create_array());
    array->reserve((int32_t)chats_data_.size());

    for (const auto& c : chats_data_)
    {
        coll_helper coll(_root_coll->create_collection(), true);
        c.serialize(coll);

        ifptr<ivalue> val(_root_coll->create_value());
        val->set_as_collection(coll.get());
        array->push_back(val.get());
    }

    _root_coll.set_value_as_array("chats", array.get());
}

bool search_contacts_response::unserialize(const rapidjson::Value& _node)
{
    { auto i = _node.FindMember("finish"); if (i != _node.MemberEnd() && i->value.IsBool()) finish_ = i->value.GetBool(); }
    { auto i = _node.FindMember("newTag"); if (i != _node.MemberEnd() && i->value.IsString()) next_tag_ = rapidjson_get_string(i->value); }

    return unserialize_contacts(_node) && unserialize_chats(_node);
}

void search_contacts_response::serialize(core::coll_helper _root_coll) const
{
    serialize_contacts(_root_coll);
    serialize_chats(_root_coll);

    _root_coll.set_value_as_bool("finish", finish_);
    _root_coll.set_value_as_string("next_tag", next_tag_);
}

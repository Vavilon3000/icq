#include "stdafx.h"
#include "contact_profile.h"

namespace core
{
    namespace profile
    {
        bool address::unserialize(const rapidjson::Value& _node)
        {
            const auto end = _node.MemberEnd();
            const auto iter_city = _node.FindMember("city");
            if (iter_city != end && iter_city->value.IsString())
                city_ = rapidjson_get_string(iter_city->value);

            const auto iter_state = _node.FindMember("state");
            if (iter_state != end && iter_state->value.IsString())
                state_ = rapidjson_get_string(iter_state->value);

            const auto iter_country = _node.FindMember("country");
            if (iter_country != end && iter_country->value.IsString())
                country_ = rapidjson_get_string(iter_country->value);

            return true;
        }

        void address::serialize(coll_helper _coll) const
        {
            _coll.set_value_as_string("city", city_);
            _coll.set_value_as_string("state", state_);
            _coll.set_value_as_string("country", country_);
        }

        bool phone::unserialize(const rapidjson::Value &_node)
        {
            const auto end = _node.MemberEnd();
            const auto iter_phone = _node.FindMember("number");
            if (iter_phone != end && iter_phone->value.IsString())
                phone_ = rapidjson_get_string(iter_phone->value);

            const auto iter_type = _node.FindMember("type");
            if (iter_type != end && iter_type->value.IsString())
                type_ = rapidjson_get_string(iter_type->value);

            return true;
        }

        void phone::serialize(coll_helper _coll) const
        {
            _coll.set_value_as_string("phone", phone_);
            _coll.set_value_as_string("type", type_);
        }

        bool info::unserialize(const rapidjson::Value& _root)
        {
            const auto root_end = _root.MemberEnd();
            const auto iter_phones = _root.FindMember("abPhones");
            if (iter_phones != root_end && iter_phones->value.IsArray())
            {
                for (auto iter_phone = iter_phones->value.Begin(), iter_phone_end = iter_phones->value.End(); iter_phone != iter_phone_end; ++iter_phone)
                {
                    phone p;
                    if (p.unserialize(*iter_phone))
                        phones_.push_back(std::move(p));
                }
            }

            const auto iter_profile = _root.FindMember("profile");
            if (iter_profile == root_end || !iter_profile->value.IsObject())
                return false;

            const rapidjson::Value& _node = iter_profile->value;

            const auto node_end = _node.MemberEnd();
            const auto iter_first_name = _node.FindMember("firstName");
            if (iter_first_name != node_end && iter_first_name->value.IsString())
                first_name_ = rapidjson_get_string(iter_first_name->value);

            const auto iter_last_name = _node.FindMember("lastName");
            if (iter_last_name != node_end && iter_last_name->value.IsString())
                last_name_ = rapidjson_get_string(iter_last_name->value);

            const auto iter_friendly_name = _node.FindMember("friendlyName");
            if (iter_friendly_name != node_end && iter_friendly_name->value.IsString())
                friendly_ = rapidjson_get_string(iter_friendly_name->value);

            const auto iter_displayid = _node.FindMember("displayId");
            if (iter_displayid != node_end && iter_displayid->value.IsString())
                displayid_ = rapidjson_get_string(iter_displayid->value);

            const auto iter_gender = _node.FindMember("gender");
            if (iter_gender != node_end && iter_gender->value.IsString())
            {
                const auto sex = rapidjson_get_string(iter_gender->value);
                if (sex == "male")
                    gender_ = gender::male;
                else if (sex == "female")
                    gender_ = gender::female;
                else
                    gender_ = gender::unknown;
            }

            const auto iter_relationship = _node.FindMember("relationshipStatus");
            if (iter_relationship != node_end && iter_relationship->value.IsString())
                relationship_ = rapidjson_get_string(iter_relationship->value);

            const auto iter_birthdate = _node.FindMember("birthDate");
            if (iter_birthdate != node_end && iter_birthdate->value.IsInt64())
                birthdate_ = iter_birthdate->value.GetInt64();

            const auto iter_home_address = _node.FindMember("homeAddress");
            if (iter_home_address != node_end && iter_home_address->value.IsArray())
            {
                if (iter_home_address->value.Size() > 0)
                    home_address_.unserialize(*iter_home_address->value.Begin());
            }

            const auto iter_about = _node.FindMember("aboutMe");
            if (iter_about != node_end && iter_about->value.IsString())
                about_ = rapidjson_get_string(iter_about->value);

            return true;
        }

        void info::serialize(coll_helper _coll) const
        {
            _coll.set_value_as_string("aimid", aimid_);
            _coll.set_value_as_string("firstname", first_name_);
            _coll.set_value_as_string("lastname", last_name_);
            _coll.set_value_as_string("friendly", friendly_);
            _coll.set_value_as_string("displayid", displayid_);
            _coll.set_value_as_string("relationship", relationship_);
            _coll.set_value_as_string("about", about_);
            _coll.set_value_as_int64("birthdate", birthdate_);
            std::string sgender;
            if (gender_ == gender::male)
                sgender = "male";
            else if (gender_ == gender::female)
                sgender = "female";
            _coll.set_value_as_string("gender", sgender);

            coll_helper coll_address(_coll->create_collection(), true);
            home_address_.serialize(coll_address);
            _coll.set_value_as_collection("homeaddress", coll_address.get());

            ifptr<iarray> phones(_coll->create_array());
            phones->reserve((int32_t)phones_.size());
            for (const auto& phone : phones_)
            {
                coll_helper coll_phone(_coll->create_collection(), true);
                phone.serialize(coll_phone);

                ifptr<ivalue> val(_coll->create_value());
                val->set_as_collection(coll_phone.get());

                phones->push_back(val.get());
            }
            _coll.set_value_as_array("phones", phones.get());
        }
    }
}


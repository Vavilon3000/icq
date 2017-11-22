#pragma once

#include "../../../corelib/collection_helper.h"
#include "chat_info.h"

namespace core
{
    namespace wim
    {
        class search_contacts_response
        {
            bool finish_;
            std::string next_tag_;

            struct contact_chunk
            {
                std::string aimid_;
                std::string stamp_;
                std::string type_;
                int32_t score_;

                std::string first_name_;
                std::string last_name_;
                std::string nick_name_;
                std::string city_;
                std::string state_;
                std::string country_;
                std::string gender_;
                struct birthdate
                {
                    int32_t year_;
                    int32_t month_;
                    int32_t day_;
                    birthdate(): year_(-1), month_(-1), day_(-1) {}
                }
                birthdate_;
                std::string about_;
                int32_t mutual_friend_count_;

                contact_chunk(): score_(-1), mutual_friend_count_(0) {}
            };
            std::vector<contact_chunk> contacts_data_;
            std::vector<chat_info> chats_data_;

            bool unserialize_contacts(const rapidjson::Value& _node);
            bool unserialize_chats(const rapidjson::Value& _node);

            void serialize_contacts(core::coll_helper _root_coll) const;
            void serialize_chats(core::coll_helper _root_coll) const;

        public:
            search_contacts_response();

            bool unserialize(const rapidjson::Value& _node);
            void serialize(core::coll_helper _root_coll) const;
        };

    }
}
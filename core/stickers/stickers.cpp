#include "stdafx.h"

#include "stickers.h"

#include "../../../corelib/enumerations.h"

#include "../tools/system.h"

#include "../async_task.h"
#include "../../common.shared/loader_errors.h"

namespace core
{
    namespace stickers
    {
        sticker_size string_size_2_size(const std::string& _size)
        {
            if (_size == "small")
            {
                return sticker_size::small;
            }
            else if (_size == "medium")
            {
                return sticker_size::medium;
            }
            else if (_size == "large" || _size == "xlarge" || _size == "xxlarge")
            {
                return sticker_size::large;
            }
            else
            {
                assert(!"unknown type");
                return sticker_size::large;
            }
        }


        const std::wstring stickers_meta_file_name = L"meta.js";

        //////////////////////////////////////////////////////////////////////////
        // class sticker_params
        //////////////////////////////////////////////////////////////////////////
        sticker_params::sticker_params(sticker_size _size, int32_t _width, int32_t _height)
            :	size_(_size), width_(_width), height_(_height)
        {

        }

        sticker_size sticker_params::get_size() const
        {
            return size_;
        }





        //////////////////////////////////////////////////////////////////////////
        // class sticker
        //////////////////////////////////////////////////////////////////////////
        sticker::sticker()
            : id_(0)
        {
        }


        bool sticker::unserialize(const rapidjson::Value& _node)
        {
            auto iter_id = _node.FindMember("id");
            if (iter_id == _node.MemberEnd() || !iter_id->value.IsInt())
                return false;

            id_ = iter_id->value.GetInt();

            auto iter_size = _node.FindMember("size");
            if (iter_size == _node.MemberEnd() || !iter_size->value.IsObject())
                return false;

            for (auto iter_size_type = iter_size->value.MemberBegin(); iter_size_type != iter_size->value.MemberEnd(); ++iter_size_type)
            {
                if (!iter_size_type->value.IsObject())
                {
                    assert(false);
                    continue;
                }

                auto iter_w = iter_size_type->value.FindMember("w");
                auto iter_h = iter_size_type->value.FindMember("h");
                if (iter_w == iter_size_type->value.MemberEnd() || !iter_w->value.IsInt() ||
                    iter_h == iter_size_type->value.MemberEnd() || !iter_h->value.IsInt())
                {
                    assert(false);
                    continue;
                }

                sticker_size sz = string_size_2_size(rapidjson_get_string(iter_size_type->name));

                sizes_.emplace(sz, stickers::sticker_params(sz, iter_w->value.GetInt(), iter_h->value.GetInt()));

            }

            auto iter_small = iter_size->value.FindMember("small");
            if (iter_small == iter_size->value.MemberEnd() || !iter_size->value.IsObject())
                return false;

            auto iter_medium = iter_size->value.FindMember("medium");
            if (iter_medium == iter_size->value.MemberEnd() || !iter_size->value.IsObject())
                return false;

            auto iter_large = iter_size->value.FindMember("large");
            if (iter_large == iter_size->value.MemberEnd() || !iter_size->value.IsObject())
                return false;

            return true;
        }

        int32_t sticker::get_id() const
        {
            return id_;
        }

        const size_map& sticker::get_sizes() const
        {
            return sizes_;
        }

        //////////////////////////////////////////////////////////////////////////
        // class set_icon
        //////////////////////////////////////////////////////////////////////////
        set_icon::set_icon()
            : size_(set_icon_size::invalid)
        {

        }

        set_icon::set_icon(set_icon_size _size, std::string _url)
            :	size_(_size),
            url_(std::move(_url))
        {

        }

        set_icon_size set_icon::get_size() const
        {
            return size_;
        }

        std::string set_icon::get_url() const
        {
            return url_;
        }



        //////////////////////////////////////////////////////////////////////////
        // class set
        //////////////////////////////////////////////////////////////////////////
        set::set()
            : id_(-1),
            show_(false),
            purchased_(false),
            user_(false)
        {

        }

        int32_t set::get_id() const
        {
            return id_;
        }

        void set::set_id(int32_t _id)
        {
            id_ = _id;
        }

        const std::string& set::get_name() const
        {
            return name_;
        }

        void set::set_name(const std::string& _name)
        {
            name_ = _name;
        }

        bool set::is_show() const
        {
            return show_;
        }

        void set::set_show(bool _show)
        {
            show_ = _show;
        }

        bool set::is_purchased() const
        {
            return purchased_;
        }

        void set::set_purchased(const bool _purchased)
        {
            purchased_ = _purchased;
        }

        bool set::is_user() const
        {
            return user_;
        }

        void set::set_user(const bool _user)
        {
            user_ = _user;
        }

        const std::string& set::get_store_id() const
        {
            return store_id_;
        }

        void set::set_store_id(const std::string& _store_id)
        {
            store_id_ = _store_id;
        }

        void set::set_description(const std::string& _description)
        {
            description_ = _description;
        }

        const std::string& set::get_description() const
        {
            return description_;
        }

        void set::put_icon(const set_icon& _icon)
        {
            icons_[_icon.get_size()] = _icon;
        }

        set_icon set::get_icon(stickers::set_icon_size _size) const
        {
            auto iter = icons_.find(_size);
            if (iter == icons_.end())
            {
                assert(!"invalid icon size");
                return set_icon();
            }

            return iter->second;
        }

        const icons_map& set::get_icons() const
        {
            return icons_;
        }

        const stickers_list& set::get_stickers() const
        {
            return stickers_;
        }

        bool set::unserialize(const rapidjson::Value& _node)
        {
            auto iter_id = _node.FindMember("id");
            if (iter_id == _node.MemberEnd() || !iter_id->value.IsInt())
                return false;

            set_id(iter_id->value.GetInt());

            auto iter_name = _node.FindMember("name");
            if (iter_name != _node.MemberEnd() && iter_name->value.IsString())
                set_name(rapidjson_get_string(iter_name->value));

            auto iter_show = _node.FindMember("is_enabled");
            if (iter_show != _node.MemberEnd() && iter_show->value.IsBool())
                set_show(iter_show->value.GetBool());

            auto iter_purchased = _node.FindMember("purchased");
            if (iter_purchased != _node.MemberEnd() && iter_purchased->value.IsBool())
                set_purchased(iter_purchased->value.GetBool());

            auto iter_usersticker = _node.FindMember("usersticker");
            if (iter_usersticker != _node.MemberEnd() && iter_usersticker->value.IsBool())
                set_user(iter_usersticker->value.GetBool());

            auto iter_store_id = _node.FindMember("store_id");
            if (iter_store_id != _node.MemberEnd() && iter_store_id->value.IsString())
                set_store_id(rapidjson_get_string(iter_store_id->value));

            auto iter_description = _node.FindMember("description");
            if (iter_description != _node.MemberEnd() && iter_description->value.IsString())
                set_description(rapidjson_get_string(iter_description->value));

            auto iter_icons = _node.FindMember("contentlist_sticker_picker_icon");
            if (iter_icons != _node.MemberEnd() && iter_icons->value.IsObject())
            {
                auto iter_icon_20 = iter_icons->value.FindMember("xsmall");
                if (iter_icon_20 != iter_icons->value.MemberEnd() && iter_icon_20->value.IsString())
                    put_icon(set_icon(set_icon_size::_20, rapidjson_get_string(iter_icon_20->value)));

                auto iter_icon_32 = iter_icons->value.FindMember("small");
                if (iter_icon_32 != iter_icons->value.MemberEnd() && iter_icon_32->value.IsString())
                    put_icon(set_icon(set_icon_size::_32, rapidjson_get_string(iter_icon_32->value)));

                auto iter_icon_48 = iter_icons->value.FindMember("medium");
                if (iter_icon_48 != iter_icons->value.MemberEnd() && iter_icon_48->value.IsString())
                    put_icon(set_icon(set_icon_size::_48, rapidjson_get_string(iter_icon_48->value)));

                auto iter_icon_64 = iter_icons->value.FindMember("large");
                if (iter_icon_64 != iter_icons->value.MemberEnd() && iter_icon_64->value.IsString())
                    put_icon(set_icon(set_icon_size::_64, rapidjson_get_string(iter_icon_64->value)));
            }

            auto iter_content = _node.FindMember("content");
            if (iter_content != _node.MemberEnd() && iter_content->value.IsArray())
            {
                for (auto iter_sticker = iter_content->value.Begin(); iter_sticker != iter_content->value.End(); ++iter_sticker)
                {
                    auto new_sticker = std::make_shared<stickers::sticker>();

                    if (!new_sticker->unserialize(*iter_sticker))
                    {
                        continue;
                    }

                    stickers_.push_back(std::move(new_sticker));
                }
            }

            return true;
        }


        //////////////////////////////////////////////////////////////////////////
        // download_task
        //////////////////////////////////////////////////////////////////////////
        download_task::download_task(
            const std::string& _source_url,
            const std::wstring& _dest_file,
            int32_t _set_id = -1,
            int32_t _sticker_id = -1,
            sticker_size _size = sticker_size::min)
            :
        source_url_(_source_url),
            dest_file_(_dest_file),
            set_id_(_set_id),
            sticker_id_(_sticker_id),
            size_(_size)

        {

        }

        const std::string& download_task::get_source_url() const
        {
            return source_url_;
        }

        const std::wstring& download_task::get_dest_file() const
        {
            return dest_file_;
        }

        int32_t download_task::get_set_id() const
        {
            return set_id_;
        }

        int32_t download_task::get_sticker_id() const
        {
            return sticker_id_;
        }

        sticker_size download_task::get_size() const
        {
            return size_;
        }

        std::wstring g_stickers_path;

        //////////////////////////////////////////////////////////////////////////
        // class cache
        //////////////////////////////////////////////////////////////////////////
        cache::cache(const std::wstring& _stickers_path)
        {
            g_stickers_path = _stickers_path;
        }

        cache::~cache()
        {
        }

        bool cache::parse(core::tools::binary_stream& _data, bool _insitu, bool& _up_todate)
        {
            _up_todate = false;

            rapidjson::Document doc;
            const auto& parse_result = (_insitu ? doc.ParseInsitu(_data.read(_data.available())) : doc.Parse(_data.read(_data.available())));
            if (parse_result.HasParseError())
                return false;

            auto iter_status = doc.FindMember("status");
            if (iter_status == doc.MemberEnd())
                return false;

            if (iter_status->value.IsInt())
            {
                auto status_code_ = iter_status->value.GetInt();
                if (status_code_ != 200)
                {
                    assert(false);
                    return false;
                }
            }
            else if (iter_status->value.IsString())
            {
                std::string status = rapidjson_get_string(iter_status->value);

                if (status == "UP_TO_DATE")
                {
                    _up_todate = true;
                    return true;
                }
                else
                {
                    return false;
                }
            }
            else
            {
                return false;
            }


            auto iter_md5 = doc.FindMember("md5");
            if (iter_md5 == doc.MemberEnd() || !iter_md5->value.IsString())
                return false;

            md5_ = rapidjson_get_string(iter_md5->value);

            auto iter_stickers = doc.FindMember("stickers");
            if (iter_stickers == doc.MemberEnd() || !iter_stickers->value.IsObject())
                return false;

            auto iter_sets = iter_stickers->value.FindMember("sets");
            if (iter_sets == iter_stickers->value.MemberEnd() || !iter_sets->value.IsArray())
                return false;

            sets_.clear();

            for (auto iter_set = iter_sets->value.Begin(); iter_set != iter_sets->value.End(); iter_set++)
            {
                auto new_set = std::make_shared<stickers::set>();

                if (!new_set->unserialize(*iter_set))
                {
                    assert(!"error reading stickers set");
                    continue;
                }

                sets_.push_back(new_set);
            }

            return true;
        }

        bool cache::parse_store(core::tools::binary_stream& _data)
        {
            rapidjson::Document doc;
            const auto& parse_result = doc.ParseInsitu(_data.read(_data.available()));
            if (parse_result.HasParseError())
                return false;

            auto iter_status = doc.FindMember("status");
            if (iter_status == doc.MemberEnd())
                return false;

            if (!iter_status->value.IsInt())
                return false;

            auto status_code_ = iter_status->value.GetInt();
            if (status_code_ != 200)
            {
                assert(false);
                return false;
            }

            auto iter_data = doc.FindMember("data");
            if (iter_data == doc.MemberEnd() || !iter_data->value.IsObject())
                return false;

            auto iter_sets = iter_data->value.FindMember("top");
            if (iter_sets == iter_data->value.MemberEnd() || !iter_sets->value.IsArray())
                return false;

            store_.clear();

            for (auto iter_set = iter_sets->value.Begin(); iter_set != iter_sets->value.End(); iter_set++)
            {
                auto new_set = std::make_shared<stickers::set>();

                if (!new_set->unserialize(*iter_set))
                {
                    assert(!"error reading stickers set");
                    continue;
                }

                store_.push_back(new_set);
            }

            return true;
        }

        bool cache::is_meta_icons_exist()
        {
            for (auto iter = sets_.cbegin(); iter != sets_.cend(); ++iter)
            {
                if (!(*iter)->is_show())
                    continue;

                auto icons = (*iter)->get_icons();

                for (auto iter_icon = icons.cbegin(); iter_icon != icons.cend(); iter_icon++)
                {
                    std::wstring icon_file = get_set_icon_path(*(*iter), iter_icon->second);
                    if (!core::tools::system::is_exist(icon_file))
                        return false;
                }
            }

            return true;
        }

        std::wstring cache::get_meta_file_name() const
        {
            return g_stickers_path + L"/" + stickers_meta_file_name;
        }

        std::wstring cache::get_set_icon_path(const set& _set, const set_icon& _icon)
        {
            std::wstringstream ss_out;
            ss_out << g_stickers_path << L"/" << _set.get_id() << L"/icons/" << L"_icon_" << _icon.get_size() << L".png";

            return ss_out.str();
        }

        std::wstring cache::get_set_big_icon_path(const int32_t _set_id)
        {

            std::wstringstream ss_out;
            ss_out << g_stickers_path << L"/" << _set_id << L"/icons/" << L"_icon_xset.png";

            return ss_out.str();
        }

        std::wstring cache::get_sticker_path(int32_t _set_id, int32_t _sticker_id, sticker_size _size)
        {
            std::wstringstream ss_out;

            ss_out << g_stickers_path << L"/" << _set_id << L"/" << _sticker_id << L"/" << _size << L".png";

            return ss_out.str();
        }

        std::wstring cache::get_sticker_path(const set& _set, const sticker& _sticker, sticker_size _size)
        {
            return get_sticker_path(_set.get_id(), _sticker.get_id(), _size);
        }

        std::string cache::make_sticker_url(const int32_t _set_id, const int32_t _sticker_id, const core::sticker_size _size) const
        {
            std::stringstream ss_size;

            ss_size << _size;

            return make_sticker_url(_set_id, _sticker_id, ss_size.str());
        }

        std::string cache::make_sticker_url(const int32_t _set_id, const int32_t _sticker_id, const std::string& _size) const
        {
            std::stringstream ss_url;

            ss_url << "https://c.icq.com/store/stickers/" << _set_id << "/" << _sticker_id << "/" << _size << ".png";

            return ss_url.str();
        }

        std::string cache::make_big_icon_url(const int32_t _set_id)
        {
            std::stringstream ss_url;

            ss_url << "https://c.icq.com/store/stickers/" << _set_id << "/icon/large.png";

            return ss_url.str();
        }

        void cache::make_download_tasks(const std::string& _size)
        {
            for (auto iter = sets_.cbegin(); iter != sets_.cend(); ++iter)
            {
                if (!(*iter)->is_show() || !(*iter)->is_purchased())
                    continue;

                auto icons = (*iter)->get_icons();

                for (auto iter_icon = icons.cbegin(); iter_icon != icons.cend(); iter_icon++)
                {
                    std::wstring icon_file = get_set_icon_path(*(*iter), iter_icon->second);
                    if (!core::tools::system::is_exist(icon_file))
                        meta_tasks_.push_back(download_task(iter_icon->second.get_url(), icon_file));
                }

                auto map_stickers = (*iter)->get_stickers();

                int32_t set_id = (*iter)->get_id();

                for (auto iter_sticker = map_stickers.cbegin(); iter_sticker != map_stickers.cend(); ++iter_sticker)
                {
                    int32_t sticker_id = (*iter_sticker)->get_id();

                    const auto sticker_url = make_sticker_url(set_id, sticker_id, _size);

                    std::wstring file_name = get_sticker_path(*(*iter), *(*iter_sticker), string_size_2_size(_size));

                    if (!core::tools::system::is_exist(file_name))
                    {
                        if (has_gui_request(set_id, sticker_id))
                        {
                            stickers_tasks_.emplace_front(sticker_url, file_name, set_id, sticker_id, string_size_2_size(_size));
                        }
                        else
                        {
                            stickers_tasks_.emplace_back(sticker_url, file_name, set_id, sticker_id, string_size_2_size(_size));
                        }
                    }
                }
            }
        }

        void cache::serialize_meta_set_sync(const stickers::set& _set, coll_helper _coll_set, const std::string& _size)
        {
            _coll_set.set_value_as_int("id", _set.get_id());
            _coll_set.set_value_as_string("name", _set.get_name());
            _coll_set.set_value_as_string("store_id", _set.get_store_id());
            _coll_set.set_value_as_bool("purchased", _set.is_purchased());
            _coll_set.set_value_as_bool("usersticker", _set.is_user());
            _coll_set.set_value_as_string("description", _set.get_description());

            set_icon_size icon_size = set_icon_size::invalid;

            if (!_size.empty())
            {
                if (_size == "small")
                    icon_size = set_icon_size::_32;
                else if (_size == "medium")
                    icon_size = set_icon_size::_48;
                else if (_size == "large")
                    icon_size = set_icon_size::_64;

                std::wstring file_name = get_set_icon_path(_set, _set.get_icon(icon_size));

                core::tools::binary_stream bs_icon;

                if (bs_icon.load_from_file(file_name))
                {
                    ifptr<istream> icon(_coll_set->create_stream(), true);

                    uint32_t file_size = bs_icon.available();

                    icon->write((uint8_t*)bs_icon.read(file_size), file_size);

                    _coll_set.set_value_as_stream("icon", icon.get());
                }
            }

            // serialize stickers
            ifptr<iarray> stickers_array(_coll_set->create_array(), true);

            for (auto iter_sticker = _set.get_stickers().cbegin(); iter_sticker != _set.get_stickers().cend(); ++iter_sticker)
            {
                coll_helper coll_sticker(_coll_set->create_collection(), true);

                ifptr<ivalue> val_sticker(_coll_set->create_value(), true);

                val_sticker->set_as_collection(coll_sticker.get());

                stickers_array->push_back(val_sticker.get());

                coll_sticker.set_value_as_int("id", (*iter_sticker)->get_id());
            }

            _coll_set.set_value_as_array("stickers", stickers_array.get());
        }

        void cache::serialize_meta_sync(coll_helper _coll, const std::string& _size)
        {
            if (sets_.empty())
                return;

            ifptr<iarray> sets_array(_coll->create_array(), true);

            for (const auto& _set : sets_)
            {
                if (!_set->is_show())
                    continue;

                // serailize sets
                coll_helper coll_set(_coll->create_collection(), true);

                ifptr<ivalue> val_set(_coll->create_value(), true);

                val_set->set_as_collection(coll_set.get());

                sets_array->push_back(val_set.get());

                serialize_meta_set_sync(*_set, coll_set, _size);
            }

            _coll.set_value_as_array("sets", sets_array.get());
        }

        void cache::serialize_store_sync(coll_helper _coll)
        {
            if (store_.empty())
                return;

            ifptr<iarray> sets_array(_coll->create_array(), true);

            for (const auto& _set : store_)
            {
                // serailize sets
                coll_helper coll_set(_coll->create_collection(), true);

                ifptr<ivalue> val_set(_coll->create_value(), true);

                val_set->set_as_collection(coll_set.get());

                sets_array->push_back(val_set.get());

                serialize_meta_set_sync(*_set, coll_set, std::string());
            }

            _coll.set_value_as_array("sets", sets_array.get());
        }

        std::string cache::get_md5() const
        {
            return md5_;
        }


        bool cache::get_next_meta_task(download_task& _task)
        {
            if (meta_tasks_.empty())
                return false;

            _task = meta_tasks_.front();

            return true;
        }

        bool cache::get_next_sticker_task(download_task& _task)
        {
            if (stickers_tasks_.empty())
                return false;

            _task = stickers_tasks_.front();

            return true;
        }

        void cache::get_sticker(int64_t _seq, int32_t _set_id, int32_t _sticker_id, const sticker_size _size, tools::binary_stream& _data)
        {
            gui_requests_[_set_id][_sticker_id].push_back(_seq);

            for (auto iter = stickers_tasks_.begin(); iter != stickers_tasks_.end(); ++iter)
            {
                if (iter->get_set_id() == _set_id && iter->get_sticker_id() == _sticker_id && iter->get_size() == _size)
                {
                    download_task task = *iter;
                    stickers_tasks_.erase(iter);
                    stickers_tasks_.push_front(task);

                    return;
                }
            }

            if (!_data.load_from_file(get_sticker_path(_set_id, _sticker_id, _size)))
            {
                const auto sticker_url = make_sticker_url(_set_id, _sticker_id, _size);

                const auto file_name = get_sticker_path(_set_id, _sticker_id, _size);

                stickers_tasks_.emplace_front(sticker_url, file_name, _set_id, _sticker_id, _size);

                return;
            }
        }

        void cache::get_set_icon_big(const int64_t _seq, const int32_t _set_id, tools::binary_stream& _data)
        {
            gui_requests_[_set_id][-1].push_back(_seq);

            for (auto iter = stickers_tasks_.begin(); iter != stickers_tasks_.end(); ++iter)
            {
                if (iter->get_set_id() == _set_id && iter->get_sticker_id() == -1)
                {
                    download_task task = *iter;
                    stickers_tasks_.erase(iter);
                    stickers_tasks_.push_front(task);

                    return;
                }
            }


            if (!_data.load_from_file(get_set_big_icon_path(_set_id)))
            {
                const auto sticker_url = make_big_icon_url(_set_id);

                const auto file_name = get_set_big_icon_path(_set_id);

                stickers_tasks_.emplace_front(sticker_url, file_name, _set_id, -1);

                return;
            }
        }

        requests_list cache::get_sticker_gui_requests(int32_t _set_id, int32_t _sticker_id) const
        {
            auto iter_set = gui_requests_.find(_set_id);
            if (iter_set != gui_requests_.end())
            {
                auto iter_sticker = iter_set->second.find(_sticker_id);
                if (iter_sticker != iter_set->second.end())
                {
                    return iter_sticker->second;
                }
            }

            return requests_list();
        }

        void cache::clear_sticker_gui_requests(int32_t _set_id, int32_t _sticker_id)
        {
            auto iter_set = gui_requests_.find(_set_id);
            if (iter_set != gui_requests_.end())
            {
                auto iter_sticker = iter_set->second.find(_sticker_id);
                if (iter_sticker != iter_set->second.end())
                {
                    iter_sticker->second.clear();
                }
            }
        }

        bool cache::has_gui_request(int32_t _set_id, int32_t _sticker_id)
        {
            auto iter_set = gui_requests_.find(_set_id);
            if (iter_set != gui_requests_.end())
            {
                auto iter_sticker = iter_set->second.find(_sticker_id);
                if (iter_sticker != iter_set->second.end())
                {
                    return !iter_sticker->second.empty();
                }
            }

            return false;
        }

        bool cache::sticker_loaded(const download_task& _task, /*out*/ requests_list& _requests)
        {
            for (auto iter = stickers_tasks_.begin(); iter != stickers_tasks_.end(); ++iter)
            {
                if (_task.get_source_url() == iter->get_source_url())
                {
                    _requests = get_sticker_gui_requests(_task.get_set_id(), _task.get_sticker_id());

                    clear_sticker_gui_requests(_task.get_set_id(), _task.get_sticker_id());

                    stickers_tasks_.erase(iter);

                    return true;
                }
            }
            return false;
        }

        bool cache::meta_loaded(const download_task& _task)
        {
            for (auto iter = meta_tasks_.begin(); iter != meta_tasks_.end(); ++iter)
            {
                if (_task.get_source_url() == iter->get_source_url())
                {
                    meta_tasks_.erase(iter);
                    return true;
                }
            }
            return false;
        }







        //////////////////////////////////////////////////////////////////////////
        // stickers::face
        //////////////////////////////////////////////////////////////////////////
        face::face(const std::wstring& _stickers_path)
            :	cache_(std::make_shared<cache>(_stickers_path)),
                thread_(std::make_shared<async_executer>()),
                meta_requested_(false),
                up_to_date_(false),
                download_meta_in_progress_(false),
                download_meta_error_(false),
                download_stickers_in_progess_(false),
                download_stickers_error_(false),
                flag_meta_need_reload_(false)
        {
        }

        std::shared_ptr<result_handler<const parse_result&>> face::parse(std::shared_ptr<core::tools::binary_stream> _data, bool _insitu)
        {
            auto handler = std::make_shared<result_handler<const parse_result&>>();
            auto stickers_cache = cache_;
            auto up_to_date = std::make_shared<bool>();

            thread_->run_async_function([stickers_cache, _data, _insitu, up_to_date]()->int32_t
            {
                return (stickers_cache->parse(*_data, _insitu, *up_to_date) ? 0 : -1);

            })->on_result_ = [handler, up_to_date](int32_t _error)
            {
                if (handler->on_result_)
                    handler->on_result_(parse_result((_error == 0), *up_to_date));
            };

            return handler;
        }

        std::shared_ptr<result_handler<const bool>> face::parse_store(std::shared_ptr<core::tools::binary_stream> _data)
        {
            auto handler = std::make_shared<result_handler<const bool>>();
            auto stickers_cache = cache_;
            auto up_to_date = std::make_shared<bool>();

            thread_->run_async_function([stickers_cache, _data]()->int32_t
            {
                return (stickers_cache->parse_store(*_data) ? 0 : -1);

            })->on_result_ = [handler, up_to_date](int32_t _error)
            {
                if (handler->on_result_)
                    handler->on_result_((_error == 0));
            };

            return handler;
        }

        std::shared_ptr<result_handler<tools::binary_stream&>> face::get_sticker(int64_t _seq, int32_t _set_id, int32_t _sticker_id, const core::sticker_size _size)
        {
            assert(_size > core::sticker_size::min);
            assert(_size < core::sticker_size::max);

            auto handler = std::make_shared<result_handler<tools::binary_stream&>>();
            auto stickers_cache = cache_;
            auto sticker_data = std::make_shared<tools::binary_stream>();

            thread_->run_async_function([stickers_cache, sticker_data, _set_id, _sticker_id, _size, _seq]
            {
                stickers_cache->get_sticker(_seq, _set_id, _sticker_id, _size, *sticker_data);

                return 0;

            })->on_result_ = [handler, sticker_data](int32_t _error)
            {
                if (handler->on_result_)
                    handler->on_result_(*sticker_data);
            };

            return handler;
        }

        std::shared_ptr<result_handler<tools::binary_stream&>> face::get_set_icon_big(const int64_t _seq, const int32_t _set_id)
        {
            auto handler = std::make_shared<result_handler<tools::binary_stream&>>();
            auto stickers_cache = cache_;
            auto icon_data = std::make_shared<tools::binary_stream>();

            thread_->run_async_function([stickers_cache, icon_data, _set_id, _seq]
            {
                stickers_cache->get_set_icon_big(_seq, _set_id, *icon_data);

                return 0;

            })->on_result_ = [handler, icon_data](int32_t _error)
            {
                if (handler->on_result_)
                    handler->on_result_(*icon_data);
            };

            return handler;
        }

        std::shared_ptr<result_handler<coll_helper>> face::serialize_meta(coll_helper _coll, const std::string& _size)
        {
            auto handler = std::make_shared<result_handler<coll_helper>>();
            auto stickers_cache = cache_;
            std::string size = _size;

            thread_->run_async_function([stickers_cache, _coll, size]()->int32_t
            {
                stickers_cache->serialize_meta_sync(_coll, size);

                return 0;

            })->on_result_ = [handler, _coll](int32_t _error)
            {
                if (handler->on_result_)
                    handler->on_result_(_coll);
            };

            return handler;
        }

        std::shared_ptr<result_handler<coll_helper>> face::serialize_store(coll_helper _coll)
        {
            auto handler = std::make_shared<result_handler<coll_helper>>();
            auto stickers_cache = cache_;

            thread_->run_async_function([stickers_cache, _coll]()->int32_t
            {
                stickers_cache->serialize_store_sync(_coll);

                return 0;

            })->on_result_ = [handler, _coll](int32_t _error)
            {
                if (handler->on_result_)
                    handler->on_result_(_coll);
            };

            return handler;
        }

        std::shared_ptr<result_handler<bool>> face::make_download_tasks(const std::string& _size)
        {
            auto handler = std::make_shared<result_handler<bool>>();
            auto stickers_cache = cache_;

            thread_->run_async_function([stickers_cache, _size]()->int32_t
            {
                stickers_cache->make_download_tasks(_size);
                return 0;

            })->on_result_ = [handler](int32_t _error)
            {
                if (handler->on_result_)
                    handler->on_result_(true);
            };

            return handler;
        }

        std::shared_ptr<result_handler<const load_result&>> face::load_meta_from_local()
        {
            auto handler = std::make_shared<result_handler<const load_result&>>();
            auto stickers_cache = cache_;
            auto md5 = std::make_shared<std::string>();

            thread_->run_async_function([stickers_cache, md5]()->int32_t
            {
                core::tools::binary_stream bs;
                if (!bs.load_from_file(stickers_cache->get_meta_file_name()))
                    return -1;

                bs.write(0);

                bool up_todate = false;

                if (!stickers_cache->parse(bs, true, up_todate))
                    return -1;

                if (!stickers_cache->is_meta_icons_exist())
                    return -1;

                *md5 = stickers_cache->get_md5();

                return 0;

            })->on_result_ = [handler, md5](int32_t _error)
            {
                if (handler->on_result_)
                    handler->on_result_(load_result((_error == 0), *md5));
            };

            return handler;
        }

        std::shared_ptr<result_handler<bool>> face::save(std::shared_ptr<core::tools::binary_stream> _data)
        {
            auto handler = std::make_shared<result_handler<bool>>();
            auto stickers_cache = cache_;

            thread_->run_async_function([stickers_cache, _data]()->int32_t
            {
                return (_data->save_2_file(stickers_cache->get_meta_file_name()) ? 0 : -1);

            })->on_result_ = [handler](int32_t _error)
            {
                if (handler->on_result_)
                    handler->on_result_(_error == 0);
            };

            return handler;
        }

        std::shared_ptr<result_handler<bool, const download_task&>> face::get_next_meta_task()
        {
            auto handler = std::make_shared<result_handler<bool, const download_task&>>();
            auto stickers_cache = cache_;
            auto task = std::make_shared<download_task>("", L"");

            thread_->run_async_function([stickers_cache, task]()->int32_t
            {
                if (!stickers_cache->get_next_meta_task(*task))
                    return -1;

                return 0;

            })->on_result_ = [handler, task](int32_t _error)
            {
                if (handler->on_result_)
                    handler->on_result_(_error == 0, *task);
            };

            return handler;
        }

        std::shared_ptr<result_handler<bool, const download_task&>> face::get_next_sticker_task()
        {
            auto handler = std::make_shared<result_handler<bool, const download_task&>>();
            auto stickers_cache = cache_;
            auto task = std::make_shared<download_task>("", L"");

            thread_->run_async_function([stickers_cache, task]()->int32_t
            {
                if (!stickers_cache->get_next_sticker_task(*task))
                    return -1;

                return 0;

            })->on_result_ = [handler, task](int32_t _error)
            {
                if (handler->on_result_)
                    handler->on_result_(_error == 0, *task);
            };

            return handler;
        }

        std::shared_ptr<result_handler<bool, const std::list<int64_t>&>> face::on_sticker_loaded(const download_task& _task)
        {
            auto handler = std::make_shared<result_handler<bool, const std::list<int64_t>&>>();
            auto stickers_cache = cache_;
            auto requests = std::make_shared<std::list<int64_t>>();

            thread_->run_async_function([stickers_cache, _task, requests]()->int32_t
            {
                return (stickers_cache->sticker_loaded(_task, *requests) ? 0 : -1);

            })->on_result_ = [handler, requests](int32_t _error)
            {
                if (handler->on_result_)
                    handler->on_result_((_error == 0), *requests);
            };

            return handler;
        }

        std::shared_ptr<result_handler<bool>> face::on_metadata_loaded(const download_task& _task)
        {
            auto handler = std::make_shared<result_handler<bool>>();
            auto stickers_cache = cache_;

            thread_->run_async_function([stickers_cache, _task]()->int32_t
            {
                return (stickers_cache->meta_loaded(_task) ? 0 : -1);

            })->on_result_ = [handler](int32_t _error)
            {
                if (handler->on_result_)
                    handler->on_result_(_error == 0);
            };

            return handler;
        }

        std::shared_ptr<result_handler<const std::string&>> face::get_md5()
        {
            auto handler = std::make_shared<result_handler<const std::string&>>();
            auto stickers_cache = cache_;
            auto md5 = std::make_shared<std::string>();

            thread_->run_async_function([stickers_cache, md5]()->int32_t
            {
                *md5 = stickers_cache->get_md5();

                return 0;

            })->on_result_ = [handler, md5](int32_t _error)
            {
                if (handler->on_result_)
                    handler->on_result_(*md5);
            };

            return handler;
        }

        void face::set_gui_request_params(const gui_request_params& _params)
        {
            gui_request_params_ = _params;
        }

        const gui_request_params& face::get_gui_request_params()
        {
            return gui_request_params_;
        }

        void face::set_up_to_date(bool _up_to_date)
        {
            up_to_date_ = _up_to_date;
        }

        bool face::is_up_to_date() const
        {
            return up_to_date_;
        }

        void face::set_download_meta_error(const bool _is_error)
        {
            download_meta_error_ = _is_error;
        }

        bool face::is_download_meta_error() const
        {
            return download_meta_error_;
        }

        bool face::is_download_meta_in_progress()
        {
            return download_meta_in_progress_;
        }

        void face::set_download_meta_in_progress(const bool _in_progress)
        {
            download_meta_in_progress_ = _in_progress;
        }

        void face::set_flag_meta_need_reload(const bool _need_reload)
        {
            flag_meta_need_reload_ = _need_reload;
        }

        bool face::is_flag_meta_need_reload() const
        {
            return flag_meta_need_reload_;
        }

        void face::set_download_stickers_error(const bool _is_error)
        {
            download_stickers_error_ = _is_error;
        }

        bool face::is_download_stickers_error() const
        {
            return download_stickers_error_;
        }

        bool face::is_download_stickers_in_progress() const
        {
            return download_stickers_in_progess_;
        }

        void face::set_download_stickers_in_progress(const bool _in_progress)
        {
            download_stickers_in_progess_ = _in_progress;
        }

    }
}



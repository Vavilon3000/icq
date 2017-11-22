#include "stdafx.h"

#include "../log/log.h"

#include "contact_archive.h"
#include "messages_data.h"
#include "archive_index.h"
#include "history_message.h"
#include "image_cache.h"
#include "mentions_me.h"

using namespace core;
using namespace archive;

contact_archive::contact_archive(const std::wstring& _archive_path, const std::string& _contact_id)
    : path_(_archive_path)
    , index_(std::make_unique<archive_index>(_archive_path + L'/' + index_filename(), _contact_id))
    , data_(std::make_unique<messages_data>(_archive_path + L'/' + db_filename()))
    , state_(std::make_unique<archive_state>(_archive_path + L'/' + dlg_state_filename(), _contact_id))
    , images_(std::make_unique<image_cache>(_archive_path + L'/' + image_cache_filename()))
    , mentions_(std::make_unique<mentions_me>(_archive_path + L'/' + mentions_filename()))
    , local_loaded_(false)
{
}


contact_archive::~contact_archive()
{
    images_->cancel_build();
    if (image_cache_thread_.joinable())
        image_cache_thread_.join();
}

void contact_archive::get_images(int64_t _from, int64_t _count, image_list& _images) const
{
    images_->get_images(_from, _count, _images);
}

bool contact_archive::repair_images() const
{
    return images_->build(*this);
}

void contact_archive::get_messages(int64_t _from, int64_t _count_early, int64_t _count_later, history_block& _messages, get_message_policy policy) const
{
    _messages.clear();

    headers_list headers;
    while (true)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        index_->serialize_from(_from, _count_early, _count_later, headers);
        if (headers.empty())
            return;

        _from = headers.begin()->get_id();

        if (policy == get_message_policy::skip_patches_and_deleted)
        {
            headers.remove_if([](const message_header& h) { return h.is_patch() || h.is_deleted(); });
            if (headers.empty())
                continue;
        }

        data_->get_messages(headers, _messages);
        return;
    }
}

void contact_archive::get_messages_index(int64_t _from, int64_t _count_early, int64_t _count_later, headers_list& _headers) const
{
    index_->serialize_from(_from, _count_early, _count_later, _headers);
}

bool contact_archive::get_history_file(const std::wstring& _file_name, core::tools::binary_stream& _data
    , const std::shared_ptr<int64_t>& _offset, const std::shared_ptr<int64_t>& _remaining_size, int64_t& _cur_index, const std::shared_ptr<int64_t>& _mode)
{
    return messages_data::get_history_archive(_file_name, _data, _offset, _remaining_size, _cur_index, _mode);
}

bool contact_archive::get_messages_buddies(const std::shared_ptr<archive::msgids_list>& _ids, const std::shared_ptr<history_block>& _messages) const
{
    headers_list _headers;

    for (auto id : *_ids)
    {
        message_header msg_header;

        if (!index_->get_header(id, Out msg_header))
        {
            assert(!"message header not found");
            continue;
        }

        if (msg_header.is_patch())
        {
            continue;
        }

        _headers.emplace_back(std::move(msg_header));
    }

    std::lock_guard<std::mutex> lock(mutex_);

    data_->get_messages(_headers, *_messages);

    return true;
}

const dlg_state& contact_archive::get_dlg_state() const
{
    return state_->get_state();
}

void contact_archive::set_dlg_state(const dlg_state& _state, Out dlg_state_changes& _changes)
{
    state_->set_state(_state, Out _changes);
}

void contact_archive::clear_dlg_state()
{
    state_->clear_state();
}

void contact_archive::insert_history_block(
    history_block_sptr _data,
    Out headers_list& _inserted_messages,
    Out dlg_state& _updated_state,
    Out dlg_state_changes& _state_changes
)
{
    Out _updated_state = state_->get_state();

    archive::history_block insert_data;
    insert_data.reserve(_data->size());

    const auto last_msgid = state_->get_state().get_last_msgid();
    auto last_message_updated = false;

    for (const auto &message : *_data)
    {
        assert(message);
        assert(message->has_msgid());

        message_header existing_header;
        if (index_->get_header(message->get_msgid(), Out existing_header))
        {
            const auto is_patch_operation = (message->is_patch() != existing_header.is_patch());

            const auto& quotes = message->get_quotes();
            const bool quote_with_chat_name = !quotes.empty() && !quotes.front().get_chat_name().empty();

            const bool is_prev_id_changed = message->get_prev_msgid() != existing_header.get_prev_msgid();

            const auto skip_duplicate = !is_patch_operation && !quote_with_chat_name && !is_prev_id_changed;
            if (skip_duplicate)
            {
                // message is already in the db, no need in quote header replacement,
                // and it's not a patch thus just skip it,
                // and prev_ids are not changed, no need to fix linked list
                continue;
            }
        }

        const auto is_message_obsolete = (
            message->has_msgid() &&
            (message->get_msgid() <= _updated_state.get_del_up_to())
        );
        if (is_message_obsolete)
        {
            assert(message->is_patch());

            __INFO(
                "delete_history",
                "skipped obsolete message\n"
                "    id=<%1%>\n"
                "    is_patch=<%2%>\n"
                "    del_up_to=<%3%>",
                message->get_msgid() % logutils::yn(message->is_patch()) % _updated_state.get_del_up_to()
            );

            continue;
        }

        const auto is_prev_id_obsolete = (
            message->has_prev_msgid() &&
            (message->get_prev_msgid() <= _updated_state.get_del_up_to())
        );
        if (is_prev_id_obsolete)
        {
            __INFO(
                "delete_history",
                "fixed message prev_msgid\n"
                "    id=<%1%>\n"
                "    prev_id=<%2%>\n"
                "    del_up_to=<%3%>",
                message->get_msgid() % message->get_prev_msgid() % _updated_state.get_del_up_to()
            );

            message->set_prev_msgid(-1);
        }

        last_message_updated = (
            last_message_updated ||
            (message->get_msgid() == last_msgid)
        );

        insert_data.push_back(message);
    }

    if (insert_data.empty())
    {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);

        if (!data_->update(insert_data))
        {
            assert(!"update data error");
            return;
        }

        if (!images_->update(insert_data))
        {
            assert(!"update images error");
            return;
        }

        if (!index_->update(insert_data, _inserted_messages))
        {
            assert(!"update index error");
            return;
        }
    }
}

int32_t contact_archive::load_from_local()
{
    if (local_loaded_)
        return 0;

    local_loaded_ = true;

    if (!index_->load_from_local())
    {
        if (index_->get_last_error() != archive::error::file_not_exist)
        {
            assert(!"index file crash, need repair");
            index_->save_all();
        }
    }

    image_cache_thread_ = std::thread(&image_cache::load_from_local, images_.get(), std::cref(*this));

    return 0;
}

bool contact_archive::get_next_hole(int64_t _from, archive_hole& _hole, int64_t _depth) const
{
    return index_->get_next_hole(_from, _hole, _depth);
}

int64_t contact_archive::validate_hole_request(const archive_hole& _hole, const int32_t _count) const
{
    return index_->validate_hole_request(_hole, _count);
}

bool contact_archive::need_optimize() const
{
    return index_->need_optimize();
}

void contact_archive::optimize()
{
    if (index_->need_optimize())
    {
        std::lock_guard<std::mutex> lock(mutex_);

        index_->optimize();
        images_->synchronize(*index_);
    }
}

void contact_archive::delete_messages_up_to(const int64_t _up_to)
{
    assert(_up_to > -1);

    auto dlg_state = state_->get_state();

    const auto &last_message = dlg_state.get_last_message();
    const auto is_last_message_obsolete = (last_message.has_msgid() && (_up_to >= last_message.get_msgid()));

    const auto is_last_msgid_obsolete = (dlg_state.has_last_msgid() && (_up_to >= dlg_state.get_last_msgid()));

    if (is_last_message_obsolete || is_last_msgid_obsolete)
    {
        dlg_state.clear_last_message();
        dlg_state.clear_last_msgid();

        dlg_state_changes changes;
        state_->set_state(dlg_state, Out changes);
    }

    std::lock_guard<std::mutex> lock(mutex_);

    index_->delete_up_to(_up_to);
    images_->synchronize(*index_);
}

void contact_archive::add_mention(const std::shared_ptr<archive::history_message>& _message)
{
    mentions_->add(_message);
}

std::wstring archive::db_filename()
{
    return L"_db2";
}

std::wstring archive::index_filename()
{
    return L"_idx2";
}

std::wstring archive::dlg_state_filename()
{
    return L"_ste2";
}

std::wstring archive::image_cache_filename()
{
    return L"_img3";
}

std::wstring archive::cache_filename()
{
    return L"cache2";
}

std::wstring archive::mentions_filename()
{
    return L"_mentions";
}


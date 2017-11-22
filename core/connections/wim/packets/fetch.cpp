#include "stdafx.h"
#include "fetch.h"

#include <sstream>

#include "../../../http_request.h"
#include "../events/fetch_event.h"

#include "../events/fetch_event_buddy_list.h"
#include "../events/fetch_event_presence.h"
#include "../events/fetch_event_dlg_state.h"
#include "../events/fetch_event_hidden_chat.h"
#include "../events/fetch_event_diff.h"
#include "../events/fetch_event_my_info.h"
#include "../events/fetch_event_user_added_to_buddy_list.h"
#include "../events/fetch_event_typing.h"
#include "../events/fetch_event_permit.h"
#include "../events/fetch_event_imstate.h"
#include "../events/fetch_event_notification.h"
#include "../events/fetch_event_appsdata.h"
#include "../events/fetch_event_mention_me.h"

#include "../events/webrtc.h"

#include "../../../log/log.h"


using namespace core;
using namespace wim;

const auto default_fetch_timeout = std::chrono::milliseconds(500);

fetch::fetch(
    wim_packet_params _params,
    const std::string& _fetch_url,
    const time_t _timeout,
    const timepoint _fetch_time,
    std::function<bool(const int32_t)> _wait_function)
    :
    wim_packet(std::move(_params)),
    fetch_url_(_fetch_url),
    timeout_(_timeout),
    wait_function_(_wait_function),
    fetch_time_(_fetch_time),
    relogin_(relogin::none),
    next_fetch_time_(std::chrono::system_clock::now()),
    ts_(0),
    time_offset_(0),
    execute_time_(0),
    request_time_(0)
{
}


fetch::~fetch()
{
}

int32_t fetch::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    std::stringstream ss_url;

    ss_url << core::tools::trim_right<std::string>(fetch_url_, "/");
    if (ss_url.str().rfind('?') == std::string::npos)
        ss_url << '?';
    else
        ss_url << '&';

    static int32_t request_id = 0;
    ss_url << "f=json" << "&r=" << ++request_id << "&timeout=" << timeout_ << "&peek=" << '0';

    _request->set_url(ss_url.str());
    _request->set_timeout((uint32_t)timeout_ + 5000);
    _request->set_keep_alive();
    _request->set_priority(top_priority);

    if (!params_.full_log_)
    {
        log_replace_functor f;
        f.add_marker("aimsid");
        f.add_json_marker("text");
        f.add_json_marker("message");
        _request->set_replace_log_function(f);
    }

    return 0;
}

int32_t fetch::execute_request(std::shared_ptr<core::http_request_simple> _request)
{
    auto current_time = std::chrono::system_clock::now();

    if (fetch_time_ > current_time)
    {
        auto wait_timeout = (fetch_time_ - current_time)/std::chrono::milliseconds(1);
        if (wait_function_(wait_timeout))
            return wpie_error_request_canceled;
    }

    next_fetch_time_ = std::chrono::system_clock::now() + default_fetch_timeout;

    execute_time_ = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

    if (!_request->get(request_time_))
        return wpie_network_error;

    http_code_ = (uint32_t)_request->get_response_code();

    if (http_code_ != 200)
    {
        if (http_code_ >= 500 && http_code_ < 600)
            return wpie_error_resend;

        return wpie_http_error;
    }

    return 0;
}

void fetch::on_session_ended(const rapidjson::Value &_data)
{
    relogin_ = relogin::relogin_with_error;
    auto iter_end_code = _data.FindMember("endCode");
    if (iter_end_code != _data.MemberEnd() && iter_end_code->value.IsInt())
    {
        auto end_code = iter_end_code->value.GetInt();
        switch (end_code)
        {
            case 26:            // "endCode":26, "offReason":"User Initiated Bump"
            case 142:           // "endCode":142, "offReason":"Killed Sessions"
                relogin_ = relogin::relogin_without_error;
                break;
        }
    }
}

relogin fetch::need_relogin() const
{
    return relogin_;
}


std::shared_ptr<core::wim::fetch_event> fetch::push_event(std::shared_ptr<core::wim::fetch_event> _event)
{
    events_.push_back(_event);

    return _event;
}

std::shared_ptr<core::wim::fetch_event> fetch::pop_event()
{
    if (events_.empty())
    {
        return nullptr;
    }


    auto evt = events_.front();

    events_.pop_front();

    return evt;
}


int32_t fetch::parse_response_data(const rapidjson::Value& _data)
{
    try
    {
        bool have_webrtc_event = false;

        const auto iter_events = _data.FindMember("events");

        if (iter_events != _data.MemberEnd() && iter_events->value.IsArray())
        {
            for (auto iter_evt = iter_events->value.Begin(); iter_evt != iter_events->value.End(); ++iter_evt)
            {
                auto iter_type = iter_evt->FindMember("type");
                auto iter_event_data = iter_evt->FindMember("eventData");

                if (iter_type != iter_evt->MemberEnd() && iter_event_data != iter_evt->MemberEnd() &&
                    iter_type->value.IsString())
                {
                    const std::string event_type = rapidjson_get_string(iter_type->value);

                    if (event_type == "buddylist")
                        push_event(std::make_shared<fetch_event_buddy_list>())->parse(iter_event_data->value);
                    else if (event_type == "presence")
                        push_event(std::make_shared<fetch_event_presence>())->parse(iter_event_data->value);
                    else if (event_type == "histDlgState")
                        push_event(std::make_shared<fetch_event_dlg_state>())->parse(iter_event_data->value);
                    else if (event_type == "webrtcMsg")
                        have_webrtc_event = true;
                    else if (event_type == "hiddenChat")
                        push_event(std::make_shared<fetch_event_hidden_chat>())->parse(iter_event_data->value);
                    else if (event_type == "diff")
                        push_event(std::make_shared<fetch_event_diff>())->parse(iter_event_data->value);
                    else if (event_type == "myInfo")
                        push_event(std::make_shared<fetch_event_my_info>())->parse(iter_event_data->value);
                    else if (event_type == "userAddedToBuddyList")
                        push_event(std::make_shared<fetch_event_user_added_to_buddy_list>())->parse(iter_event_data->value);
                    else if (event_type == "typing")
                        push_event(std::make_shared<fetch_event_typing>())->parse(iter_event_data->value);
                    else if (event_type == "sessionEnded")
                        on_session_ended(iter_event_data->value);
                    else if (event_type == "permitDeny")
                        push_event(std::make_shared<fetch_event_permit>())->parse(iter_event_data->value);
                    else if (event_type == "imState")
                        push_event(std::make_shared<fetch_event_imstate>())->parse(iter_event_data->value);
                    else if (event_type == "notification")
                        push_event(std::make_shared<fetch_event_notification>())->parse(iter_event_data->value);
                    else if (event_type == "apps")
                        push_event(std::make_shared<fetch_event_appsdata>())->parse(iter_event_data->value);
                    else if (event_type == "mentionMeMessage")
                        push_event(std::make_shared<fetch_event_mention_me>())->parse(iter_event_data->value);
                }
            }
        }

        if (relogin_ == relogin::none)
        {
            auto iter_next_fetch_url = _data.FindMember("fetchBaseURL");
            if (iter_next_fetch_url == _data.MemberEnd() || !iter_next_fetch_url->value.IsString())
                return wpie_http_parse_response;

            next_fetch_url_ = rapidjson_get_string(iter_next_fetch_url->value);
            next_fetch_time_ = std::chrono::system_clock::now();

            auto iter_time_to_next_fetch = _data.FindMember("timeToNextFetch");
            if (iter_time_to_next_fetch != _data.MemberEnd() && iter_time_to_next_fetch->value.IsUint())
            {
                time_t fetch_timeout = iter_time_to_next_fetch->value.GetUint();
                next_fetch_time_ += std::chrono::milliseconds(fetch_timeout);
            }

            auto iter_ts = _data.FindMember("ts");
            if (iter_ts == _data.MemberEnd() || !iter_ts->value.IsUint())
                return wpie_http_parse_response;

            ts_ = iter_ts->value.GetUint();
            auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
            auto diff = now - execute_time_ - std::round(request_time_);
            time_offset_ = now - ts_ - diff;
        }

        if (have_webrtc_event) {
            auto we = std::make_shared<webrtc_event>();
            if (!!we) {
                // sorry... the simplest way
                we->parse(response_str());
                push_event(we);
            } else {
                assert(false);
            }
        }
    }
    catch (const std::exception&)
    {
        return wpie_http_parse_response;
    }

    return 0;
}

int32_t fetch::on_response_error_code()
{
    auto code = get_status_code();

    if (code >= 500 && code < 600)
        return wpie_error_resend;

    switch (code)
    {
    case 401:
    case 460:
    default:
        return wpie_error_need_relogin;
    }
}

#ifndef __FETCH_H__
#define __FETCH_H__

#pragma once


#include "../wim_packet.h"
#include "../auth_parameters.h"

namespace core
{
    namespace wim
    {
        class fetch_event;

        enum class relogin
        {
            none,
            relogin_with_error,
            relogin_without_error,
        };

        class fetch : public wim_packet
        {
            std::string fetch_url_;
            time_t timeout_;
            std::function<bool(int32_t)> wait_function_;
            timepoint fetch_time_;
            relogin relogin_;

            virtual int32_t init_request(std::shared_ptr<core::http_request_simple> request) override;
            virtual int32_t parse_response_data(const rapidjson::Value& _data) override;
            virtual int32_t on_response_error_code() override;
            virtual int32_t execute_request(std::shared_ptr<core::http_request_simple> request) override;

            void on_session_ended(const rapidjson::Value& _data);

            std::list< std::shared_ptr<core::wim::fetch_event> > events_;

            std::string next_fetch_url_;
            timepoint next_fetch_time_;
            time_t ts_;
            time_t time_offset_;
            time_t execute_time_;
            double request_time_;

        public:

            const std::string& get_next_fetch_url() const { return next_fetch_url_; }
            timepoint get_next_fetch_time() const { return next_fetch_time_; }
            time_t get_ts() const { return ts_; }
            time_t get_time_offset() const { return time_offset_; }
            relogin need_relogin() const;

            std::shared_ptr<core::wim::fetch_event> push_event(std::shared_ptr<core::wim::fetch_event> _event);
            std::shared_ptr<core::wim::fetch_event> pop_event();

            fetch(
                wim_packet_params params,
                const std::string& fetch_url,
                const time_t timeout,
                const timepoint _fetch_time,
                std::function<bool(const int32_t)> wait_function);
            virtual ~fetch();
        };

    }
}


#endif//__FETCH_H__

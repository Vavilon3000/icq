#ifndef __GET_STICKERS_STORE_H_
#define __GET_STICKERS_STORE_H_

#pragma once

#include "../wim_packet.h"

namespace core
{
    namespace tools
    {
        class http_request_simple;
    }
}

namespace core
{
    namespace tools
    {
        class binary_stream;
    }

    namespace wim
    {
        class get_stickers_store_packet : public wim_packet
        {
            std::shared_ptr<core::tools::binary_stream> response_;

            virtual int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;
            virtual int32_t parse_response(std::shared_ptr<core::tools::binary_stream> _response) override;

        public:

            get_stickers_store_packet(wim_packet_params _params);
            virtual ~get_stickers_store_packet();

            std::shared_ptr<core::tools::binary_stream> get_response();

        };
    }
}

#endif //__GET_STICKERS_STORE_H_
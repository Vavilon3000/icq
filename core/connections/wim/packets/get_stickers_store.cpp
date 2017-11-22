#include "stdafx.h"
#include "get_stickers_store.h"
#include "../../../http_request.h"
#include "../../../tools/hmac_sha_base64.h"
#include "../../../core.h"
#include "../../../tools/system.h"
#include "../../../utils.h"

using namespace core;
using namespace wim;

get_stickers_store_packet::get_stickers_store_packet(wim_packet_params _params)
    : wim_packet(std::move(_params))
{
}

get_stickers_store_packet::~get_stickers_store_packet()
{
}

int32_t get_stickers_store_packet::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    std::map<std::string, std::string> params;

    const time_t ts = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) - params_.time_offset_;

    const std::string host = "https://store.icq.com/store/showcase";

    params["a"] = escape_symbols(params_.a_token_);
    params["f"] = "json";
    params["k"] = params_.dev_id_;
    params["ts"] = std::to_string((int64_t) ts);
    params["r"] = core::tools::system::generate_guid();

    params["client"] = (build::is_icq() ? "icq" : "agent");

    params["lang"] = g_core->get_locale();

    params["platform"] = utils::get_protocol_platform_string();

    const auto sha256 = escape_symbols(get_url_sign(host, params, params_, false));
    params["sig_sha256"] = sha256;

    std::stringstream ss_url;
    ss_url << host << '?' << format_get_params(params);

    _request->set_url(ss_url.str());
    _request->set_keep_alive();
    _request->set_priority(high_priority);

    return 0;
}

int32_t get_stickers_store_packet::parse_response(std::shared_ptr<core::tools::binary_stream> _response)
{
    if (!_response->available())
        return wpie_http_empty_response;

    response_ = _response;

    return 0;
}

std::shared_ptr<core::tools::binary_stream> get_stickers_store_packet::get_response()
{
    return response_;
}

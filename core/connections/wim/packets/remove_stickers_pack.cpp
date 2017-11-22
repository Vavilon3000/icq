#include "stdafx.h"
#include "remove_stickers_pack.h"
#include "../../../http_request.h"
#include "../../../tools/hmac_sha_base64.h"
#include "../../../core.h"
#include "../../../tools/system.h"
#include "../../../utils.h"

using namespace core;
using namespace wim;

remove_stickers_pack_packet::remove_stickers_pack_packet(wim_packet_params _params, const int32_t _pack_id, const std::string& _store_id)
    :   wim_packet(std::move(_params)),
        pack_id_(_pack_id),
        store_id_(_store_id)
{
}


remove_stickers_pack_packet::~remove_stickers_pack_packet()
{
}

int32_t remove_stickers_pack_packet::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    std::map<std::string, std::string> params;

    const time_t ts = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) - params_.time_offset_;

    const std::string host = "https://store.icq.com";

    params["a"] = escape_symbols(params_.a_token_);
    params["f"] = "json";
    params["k"] = params_.dev_id_;
    params["platform"] = utils::get_protocol_platform_string();

    params["product_id"] = std::to_string(pack_id_);

    params["ts"] = std::to_string((int64_t) ts);
    params["r"] = core::tools::system::generate_guid();

    params["client"] = (build::is_icq() ? "icq" : "agent");

    params["lang"] = g_core->get_locale();

    std::stringstream ss_url;
    ss_url << host << "/store/deletepurchase";

    const auto sha256 = escape_symbols(get_url_sign(ss_url.str(), params, params_, false));
    params["sig_sha256"] = sha256;

    std::stringstream ss_url_signed;
    ss_url_signed << ss_url.str() << '?' << format_get_params(params);

    _request->set_url(ss_url_signed.str());
    _request->set_keep_alive();
    _request->set_priority(high_priority);

    return 0;
}

int32_t remove_stickers_pack_packet::execute_request(std::shared_ptr<core::http_request_simple> _request)
{
    bool res = _request->post();
    if (!res)
        return wpie_network_error;

    http_code_ = (uint32_t)_request->get_response_code();

    if (http_code_ != 200)
        return wpie_http_error;

    return 0;
}


int32_t remove_stickers_pack_packet::parse_response(std::shared_ptr<core::tools::binary_stream> _response)
{
    if (!_response->available())
        return wpie_http_empty_response;

    response_ = _response;

    return 0;
}

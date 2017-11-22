#include "stdafx.h"
#include "set_dlg_state.h"

#include "../../../http_request.h"

using namespace core;
using namespace wim;


set_dlg_state::set_dlg_state(wim_packet_params _params, set_dlg_state_params _dlg_params)
    :	robusto_packet(std::move(_params)),
    params_(std::move(_dlg_params))
{
}


set_dlg_state::~set_dlg_state()
{

}

int32_t set_dlg_state::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    _request->set_url(c_robusto_host);
    _request->set_keep_alive();
    _request->set_priority(highest_priority);

    rapidjson::Document doc(rapidjson::Type::kObjectType);

    auto& a = doc.GetAllocator();

    std::string req_id = get_req_id();

    doc.AddMember("method", "setDlgStateWim", a);
    doc.AddMember("reqId", req_id, a);
    doc.AddMember("authToken", robusto_params_.robusto_token_, a);
    doc.AddMember("clientId", robusto_params_.robusto_client_id_, a);

    rapidjson::Value node_params(rapidjson::Type::kObjectType);

    node_params.AddMember("sn", params_.aimid_, a);
    if (!params_.last_delivered_wim_.empty())
        node_params.AddMember("lastDeliveredWim", params_.last_delivered_wim_, a);
    if (!params_.last_read_wim_.empty())
        node_params.AddMember("lastReadWim", params_.last_read_wim_, a);
    if (params_.invisible_)
        node_params.AddMember("invisible", params_.invisible_, a);
    if (params_.last_read_ > 0)
        node_params.AddMember("lastRead", params_.last_read_, a);
    if (params_.last_delivered_ > 0)
        node_params.AddMember("lastDelivered", params_.last_delivered_, a);

    doc.AddMember("params", std::move(node_params), a);

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    _request->push_post_parameter(buffer.GetString(), std::string());

    if (!robusto_packet::params_.full_log_)
    {
        log_replace_functor f;
        f.add_json_marker("authToken");
        _request->set_replace_log_function(f);
    }

    return 0;
}

#include "stdafx.h"
#include "resolve_pending.h"

#include "../../../http_request.h"
#include "../../../tools/system.h"

using namespace core;
using namespace wim;

resolve_pending::resolve_pending(
    wim_packet_params _params,
    const std::string& _aimid,
    const std::vector<std::string>& _contacts,
    bool _approve)
    : robusto_packet(std::move(_params))
    , aimid_(_aimid)
    , contacts_(_contacts)
    , approve_(_approve)
{
}

resolve_pending::~resolve_pending()
{
}

int32_t resolve_pending::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    _request->set_url(c_robusto_host);
    _request->set_keep_alive();

    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();
    doc.AddMember("method", "chatResolvePending", a);

    doc.AddMember("reqId", get_req_id(), a);
    doc.AddMember("authToken", robusto_params_.robusto_token_, a);
    doc.AddMember("clientId", robusto_params_.robusto_client_id_, a);

    rapidjson::Value node_params(rapidjson::Type::kObjectType);
    node_params.AddMember("sn", aimid_, a);

    rapidjson::Value node_members(rapidjson::Type::kArrayType);
    node_members.Reserve(contacts_.size(), a);
    for (const auto& iter : contacts_)
    {
        rapidjson::Value node_member(rapidjson::Type::kObjectType);
        node_member.AddMember("sn", iter, a);
        node_member.AddMember("approve", approve_, a);
        node_members.PushBack(std::move(node_member), a);
    }

    node_params.AddMember("members", std::move(node_members), a);
    doc.AddMember("params", std::move(node_params), a);

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    _request->push_post_parameter(buffer.GetString(), std::string());

    if (!params_.full_log_)
    {
        log_replace_functor f;
        f.add_json_marker("authToken");
        _request->set_replace_log_function(f);
    }

    return 0;
}

int32_t resolve_pending::parse_response_data(const rapidjson::Value& _data)
{
    return 0;
}

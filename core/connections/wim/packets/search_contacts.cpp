#include "stdafx.h"
#include "search_contacts.h"

#include "../../../http_request.h"

using namespace core;
using namespace wim;

search_contacts::search_contacts(wim_packet_params _packet_params, const std::string& keyword, const std::string& phonenumber, const std::string& tag):
    robusto_packet(std::move(_packet_params))
{
    params_.keyword_ = keyword;
    params_.phonenumber_ = phonenumber;
    params_.tag_ = tag;
}

search_contacts::~search_contacts()
{
}

int32_t search_contacts::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    if (!params_.keyword_.length() && !params_.tag_.length() && !params_.phonenumber_.length())
        return -1;

    _request->set_url(c_robusto_host);
    _request->set_keep_alive();

    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    doc.AddMember("method", "search", a);
    doc.AddMember("reqId", get_req_id(), a);
    doc.AddMember("authToken", robusto_params_.robusto_token_, a);
    doc.AddMember("clientId", robusto_params_.robusto_client_id_, a);

    rapidjson::Value node_params(rapidjson::Type::kObjectType);
    // possible only tag OR keyword OR phone. not all together.
    if (!params_.tag_.empty())
    {
        node_params.AddMember("tag", params_.tag_, a);
    }
    else
    {
        node_params.AddMember("keyword", params_.keyword_, a);
    }
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

int32_t search_contacts::parse_results(const rapidjson::Value& node)
{
    if (!response_.unserialize(node))
        return wpie_http_parse_response;
    return 0;
}

int32_t search_contacts::on_response_error_code()
{
    return robusto_packet::on_response_error_code();
}

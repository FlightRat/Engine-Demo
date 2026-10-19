#include "Protocol.h"
#include <rapidjson/schema.h>
#include <fstream>
#include <sstream>
#include <set>

namespace ENGINE_EDITOR::Automation {
Request::Request(std::string method, const Value& p, std::string correlationId)
    : correlationId(std::move(correlationId)), method(std::move(method)),
      deadline(std::chrono::steady_clock::now() + std::chrono::seconds(5)) {
    params.CopyFrom(p, params.GetAllocator());
}
bool Request::IsWrite() const {
    return method == "create_scene" || method == "create_object" || method == "set_transform" ||
        method == "set_light" || method == "apply_operations" || method == "undo_operation";
}
std::string Request::Fingerprint() const {
    Json d;
    d.SetObject();
    auto& a = d.GetAllocator();
    d.AddMember("method", Text(method, a), a);
    d.AddMember("params", Canonical(params, a), a);
    return Stringify(d);
}
Contract::Contract(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) throw std::runtime_error("Cannot open MCP tool contract: " + path);
    std::ostringstream data;
    data << file.rdbuf();
    m_Document = Parse(data.str());
    if (!m_Document.IsObject() || !m_Document.HasMember("tools") || !m_Document["tools"].IsArray())
        throw std::runtime_error("Invalid MCP tool contract");
}
void Contract::Validate(const std::string& method, const Value& params) const {
    if (!params.IsObject() || !Finite(params) || !UniqueKeys(params))
        throw Error("invalid_arguments", "Expected unique fields and finite float-compatible values");
    const Value* tool = nullptr;
    for (const auto& t : Tools().GetArray()) if (Str(t, "name") == method) { tool = &t; break; }
    if (!tool) throw Error("unknown_method", "Unknown tool: " + method);
    rapidjson::SchemaDocument schema((*tool)["inputSchema"]);
    rapidjson::SchemaValidator validator(schema);
    if (!params.Accept(validator)) {
        rapidjson::StringBuffer path;
        validator.GetInvalidDocumentPointer().StringifyUriFragment(path);
        throw Error("invalid_arguments", std::string("Invalid arguments at ") + path.GetString());
    }
    if (method == "apply_operations") {
        std::set<std::string> refs;
        for (const auto& action : params["operations"].GetArray()) {
            const auto& p = action["params"];
            if (p.HasMember("target_ref") && !refs.contains(Str(p, "target_ref")))
                throw Error("invalid_arguments", "target_ref must reference an earlier creation");
            if (p.HasMember("client_ref") && !refs.insert(Str(p, "client_ref")).second)
                throw Error("invalid_arguments", "Duplicate client_ref");
        }
    }
}
Json Failure(const std::string& requestId, const Error& error) {
    Json d(rapidjson::kObjectType);
    auto& a = d.GetAllocator();
    d.AddMember("request_id", Text(requestId, a), a);
    d.AddMember("status", "failed", a);
    Value e(rapidjson::kObjectType);
    e.AddMember("code", Text(error.code, a), a);
    e.AddMember("message", Text(error.what(), a), a);
    Value details(rapidjson::kObjectType);
    if (error.actionIndex) details.AddMember("failed_action_index", uint64_t(*error.actionIndex), a);
    e.AddMember("details", std::move(details), a);
    e.AddMember("retryable", error.retryable, a);
    d.AddMember("error", std::move(e), a);
    return d;
}
}

#pragma once
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>

namespace ENGINE_EDITOR::Automation {
using Json = rapidjson::Document;
using Value = rapidjson::Value;
using Allocator = Json::AllocatorType;

inline Value Text(const std::string& s, Allocator& a) {
    return Value(s.data(), static_cast<rapidjson::SizeType>(s.size()), a);
}
inline Value Copy(const Value& v, Allocator& a) { return Value(v, a); }
inline void Set(Value& o, const char* key, Value v, Allocator& a) {
    if (o.HasMember(key)) o[key] = std::move(v);
    else o.AddMember(Text(key, a), std::move(v), a);
}
inline std::string Stringify(const Value& v) {
    rapidjson::StringBuffer b;
    rapidjson::Writer<rapidjson::StringBuffer> w(b);
    if (!v.Accept(w)) throw std::runtime_error("JSON contains an invalid number");
    return {b.GetString(), b.GetSize()};
}
inline Json Parse(const std::string& text) {
    Json d;
    d.Parse<rapidjson::kParseValidateEncodingFlag>(text.data(), text.size());
    if (d.HasParseError()) throw std::runtime_error("Invalid UTF-8 JSON");
    return d;
}
inline std::string Str(const Value& o, const char* key, const std::string& fallback = {}) {
    if (!o.IsObject() || !o.HasMember(key) || !o[key].IsString()) return fallback;
    return {o[key].GetString(), o[key].GetStringLength()};
}
inline bool Finite(const Value& v) {
    if (v.IsNumber()) return std::isfinite(v.GetDouble()) && std::abs(v.GetDouble()) <= 3.4028234e38 &&
        (v.GetDouble() == 0 || static_cast<float>(v.GetDouble()) != 0);
    if (v.IsArray()) for (const auto& x : v.GetArray()) { if (!Finite(x)) return false; }
    if (v.IsObject()) for (auto i = v.MemberBegin(); i != v.MemberEnd(); ++i) { if (!Finite(i->value)) return false; }
    return true;
}
inline bool UniqueKeys(const Value& v) {
    if (v.IsObject()) {
        std::vector<std::string> keys;
        for (auto i = v.MemberBegin(); i != v.MemberEnd(); ++i) {
            std::string k(i->name.GetString(), i->name.GetStringLength());
            if (std::find(keys.begin(), keys.end(), k) != keys.end() || !UniqueKeys(i->value)) return false;
            keys.push_back(std::move(k));
        }
    } else if (v.IsArray()) for (const auto& x : v.GetArray()) if (!UniqueKeys(x)) return false;
    return true;
}
// Sort object members and normalize integral JSON numbers for retry fingerprints.
inline Value Canonical(const Value& v, Allocator& a) {
    if (v.IsObject()) {
        Value out(rapidjson::kObjectType);
        std::vector<std::string> keys;
        for (auto i = v.MemberBegin(); i != v.MemberEnd(); ++i) keys.emplace_back(i->name.GetString(), i->name.GetStringLength());
        std::sort(keys.begin(), keys.end());
        for (const auto& key : keys) out.AddMember(Text(key, a), Canonical(v[key.c_str()], a), a);
        return out;
    }
    if (v.IsArray()) {
        Value out(rapidjson::kArrayType);
        for (const auto& x : v.GetArray()) out.PushBack(Canonical(x, a), a);
        return out;
    }
    if (v.IsNumber()) {
        const double n = v.GetDouble();
        if (n == 0) return Value(0);
        if (n >= -9007199254740991.0 && n <= 9007199254740991.0 && std::floor(n) == n)
            return Value(static_cast<int64_t>(n));
    }
    return Copy(v, a);
}
}

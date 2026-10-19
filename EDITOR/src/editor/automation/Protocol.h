#pragma once
#include "Json.h"
#include <chrono>
#include <memory>
#include <optional>

namespace ENGINE_EDITOR::Automation {
inline constexpr unsigned ProtocolVersion = 1;
inline constexpr size_t MaxMessageBytes = 1024 * 1024;
inline constexpr size_t MaxBatchSize = 32;
inline constexpr size_t DefaultLedgerCapacity = 4096;
inline constexpr size_t DefaultQueueCapacity = 128;

struct Error : std::runtime_error {
    std::string code;
    bool retryable;
    std::optional<size_t> actionIndex;
    Error(std::string code, std::string message, bool retryable = false, std::optional<size_t> actionIndex = {})
        : std::runtime_error(std::move(message)), code(std::move(code)), retryable(retryable), actionIndex(actionIndex) {}
};
struct Request {
    std::string correlationId;
    std::string method;
    Json params;
    std::chrono::steady_clock::time_point deadline;
    Request(std::string method, const Value& params, std::string correlationId = {});
    bool IsWrite() const;
    std::string Id() const { return Str(params, "request_id"); }
    std::string Fingerprint() const;
};
// Owns the same schemas published by the MCP server, never engine state.
class Contract {
    Json m_Document;
public:
    explicit Contract(const std::string& path);
    void Validate(const std::string& method, const Value& params) const;
    const Value& Tools() const { return m_Document["tools"]; }
};
Json Failure(const std::string& requestId, const Error& error);
}

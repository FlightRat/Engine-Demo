#pragma once
#include "OperationTransaction.h"

namespace ENGINE_EDITOR::Automation {
// Admission is read-only. Commit uses pre-reserved slots and cannot evict on failure.
class OperationHistory {
    size_t maxEntries, maxBytes;
    std::vector<OperationRecord> undo, redo;
    uint64_t serial = 0;
public:
    explicit OperationHistory(size_t entries = 64, size_t bytes = 256 * 1024 * 1024)
        : maxEntries(entries), maxBytes(bytes) {
        if (!entries || !bytes) throw std::invalid_argument("History limits must be positive");
        undo.reserve(entries + 1); redo.reserve(entries + 1);
    }
    void Clear() { undo.clear(); redo.clear(); ++serial; }
    void Admit(const OperationRecord& record) const {
        if (record.Bytes() > maxBytes) throw Error("history_capacity_exceeded", "Operation exceeds the history byte budget");
    }
    size_t Bytes() const {
        size_t bytes = 0;
        for (const auto& r : undo) bytes += r.Bytes();
        for (const auto& r : redo) bytes += r.Bytes();
        return bytes;
    }
    void Commit(OperationRecord record) {
        Admit(record);
        redo.clear(); undo.push_back(std::move(record));
        while (undo.size() > maxEntries || Bytes() > maxBytes) undo.erase(undo.begin());
        ++serial;
    }
    OperationRecord* Top(bool forward = false) {
        auto& stack = forward ? redo : undo;
        return stack.empty() ? nullptr : &stack.back();
    }
    void Traversed(bool forward) {
        auto& from = forward ? redo : undo;
        auto& to = forward ? undo : redo;
        to.push_back(std::move(from.back())); from.pop_back(); ++serial;
    }
    const auto& Undo() const { return undo; }
    const auto& Redo() const { return redo; }
    size_t EntryLimit() const { return maxEntries; }
    size_t ByteLimit() const { return maxBytes; }
    uint64_t Serial() const { return serial; }
};
}

#pragma once
#include "CommandQueue.h"
#include <memory>
namespace ENGINE_EDITOR::Automation {
std::wstring PipeName(const std::string& instance);
class NamedPipeServer {
    struct Impl;
    std::unique_ptr<Impl> m_Impl;
public:
    NamedPipeServer(CommandQueue&, Contract&);
    ~NamedPipeServer();
    void Start(const std::string& instance);
    void Stop();
};
}

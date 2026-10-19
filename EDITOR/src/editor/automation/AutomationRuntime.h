#pragma once
#include <memory>
#include <string>
namespace ENGINE_EDITOR::Automation {
class AutomationRuntime {
    struct Impl;
    std::unique_ptr<Impl> m_Impl;
public:
    explicit AutomationRuntime(const std::string& instance);
    ~AutomationRuntime();
    void Update();
    void AfterUI();
};
}

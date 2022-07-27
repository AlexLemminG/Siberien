#include "System.h"
#include "Common.h"
#include <dear-imgui/imgui.h>

class OnScreenLogSystem : public System<OnScreenLogSystem>, public LogHandler {
    virtual bool Init() override {
        logHandlers.push_back(this);
        return true;
    }

    virtual void Term() override {
        logHandlers.erase(std::find(logHandlers.begin(), logHandlers.end(), this));
    }

    virtual void Update() override {
        ImGui::SetNextWindowBgAlpha(0.1f);
        ImGui::Begin("Logs");
        for (auto& l : lastLogs) {
            ImVec4 color;
            switch (l.priority)
            {
            case Log::Priority::ERROR:
                color = ImVec4(1.f, 0.2f, 0.2f, 1.f);
                break;
            case Log::Priority::WARNING:
                color = ImVec4(1.f, 1.f, 0.2f, 1.f);
                break;
            case Log::Priority::CRITICAL:
                color = ImVec4(1.f, 0.f, 0.f, 1.f);
                break;
            default:
                color = ImVec4(1.f, 1.f, 1.f, 1.f);
            }
            ImGui::TextColored(color, l.log.c_str());
        }
        ImGui::SetScrollHereY(1.f);
        ImGui::End();
    }

    virtual PriorityInfo GetPriorityInfo() const override {
        return PriorityInfo(PriorityInfo::ASSET_DATABASE - 1);  // TODO more custom
    }

    struct Log {
        std::string log;
        enum class Priority {
            LOG,
            ERROR,
            WARNING,
            CRITICAL
        };
        Priority priority;
    };
    std::vector<Log> lastLogs;

    virtual void Log(const std::string& str) override {
        lastLogs.push_back({ str, Log::Priority::LOG });
    }
    virtual void LogWarning(const std::string& str) override {
        lastLogs.push_back({ str, Log::Priority::WARNING });
    }
    virtual void LogError(const std::string& str) override {
        lastLogs.push_back({ str, Log::Priority::ERROR });
    }
    virtual void LogCritical(const std::string& str) override {
        lastLogs.push_back({ str, Log::Priority::CRITICAL });
    }
};

REGISTER_SYSTEM(OnScreenLogSystem);

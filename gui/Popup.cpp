#include "Popup.h"
#include <imgui.h>

bool Popup::showMessage(const string& title, const std::vector<AppError>& errors, const string& buttonLabel)
{
    bool closed = false;

    ImGui::SetNextWindowPos(
        ImGui::GetMainViewport()->GetCenter(),
        ImGuiCond_Appearing,
        ImVec2(0.5f, 0.5f)
    );

    if (ImGui::BeginPopupModal(title.c_str(), NULL,
                               ImGuiWindowFlags_AlwaysAutoResize))
    {
        for (const auto& msg : errors)
        {
            ImGui::TextWrapped("• %s", msg.getMessage().c_str());
        }

        ImGui::Spacing();

        if (ImGui::Button(buttonLabel.c_str(), ImVec2(120, 0)))
        {
            closed = true;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    return closed;
}

bool Popup::showFatalMessage(
    const std::string& title,
    const std::vector<AppError>& errors,
    const std::string& buttonLabel
)
{
    bool shouldClose = false;

    ImGui::SetNextWindowPos(
        ImGui::GetMainViewport()->GetCenter(),
        ImGuiCond_Appearing,
        ImVec2(0.5f, 0.5f)
    );

    if (ImGui::BeginPopupModal(
            title.c_str(),
            nullptr,
            ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::TextColored(ImVec4(1,0,0,1), "A fatal system error occurred.");
        ImGui::Separator();
        ImGui::Spacing();

        for (const auto& err : errors)
        {
            ImGui::TextWrapped("• %s", err.getMessage().c_str());
        }

        ImGui::Spacing();

        if (ImGui::Button(buttonLabel.c_str(), ImVec2(150, 0)))
        {
            shouldClose = true;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    return shouldClose;
}

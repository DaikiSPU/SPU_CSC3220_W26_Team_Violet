#include "ToggleButton.h"
#include "imgui.h"
#include "imgui_internal.h"

// OrderState ToggleButton::toggleSwitch(const char* widget_id, OrderState& state)
// {
//     ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
//     ImDrawList* draw_list = ImGui::GetWindowDrawList();

//     float toggle_height = ImGui::GetFrameHeight();
//     float toggle_width = toggle_height * 2.4f;
//     float knob_radius = toggle_height * 0.5f;

//     // Invisible button for click handling
//     ImGui::InvisibleButton(widget_id, ImVec2(toggle_width, toggle_height));

//     if (ImGui::IsItemClicked())
//     {
//         switch (state)
//         {
//             case OrderState::BUY:  state = OrderState::SELL; break;
//             case OrderState::SELL: state = OrderState::BUY; break;
//         }
//     }

//     // Determine position and color based on state
//     float position_factor = 0.0f;
//     ImVec4 bg_color;

//     switch (state)
//     {
//         case OrderState::BUY:
//             position_factor = 0.5f;
//             bg_color = ImVec4(0.1f, 0.7f, 0.2f, 1.0f); // Green
//             break;

//         case OrderState::SELL:
//             position_factor = 1.0f;
//             bg_color = ImVec4(0.8f, 0.2f, 0.2f, 1.0f); // Red
//             break;
//     }

//     ImU32 background_color = ImGui::GetColorU32(bg_color);

//     // Draw background
//     draw_list->AddRectFilled(
//         cursor_pos,
//         ImVec2(cursor_pos.x + toggle_width, cursor_pos.y + toggle_height),
//         background_color,
//         knob_radius
//     );

//     // Compute knob position
//     ImVec2 knob_center = ImVec2(
//         cursor_pos.x + knob_radius +
//         position_factor * (toggle_width - knob_radius * 2.0f),
//         cursor_pos.y + knob_radius
//     );

//     // Draw knob
//     draw_list->AddCircleFilled(
//         knob_center,
//         knob_radius - 1.5f,
//         IM_COL32(255, 255, 255, 255)
//     );

//     return state;
// }

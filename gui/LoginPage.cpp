#include "LoginPage.h"
#include <cstdio>

PageType LoginPage::draw()
{
    PageType nextPage = PageType::None;

    // Get current screen size
    ImVec2 screenSize = ImGui::GetIO().DisplaySize;

    // Make this window fullscreen
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(screenSize);

    // Begin fullscreen background window
    ImGui::Begin("LoginPage",
        nullptr,
        ImGuiWindowFlags_NoDecoration |   // Remove title bar
        ImGuiWindowFlags_NoMove |         // Disable moving
        ImGuiWindowFlags_NoResize);       // Disable resizing

    // Compute center position
    ImVec2 cardPos(
        (screenSize.x - FORM_WIDTH) * 0.5f,
        (screenSize.y - FORM_HEIGHT) * 0.5f
    );

    // Move cursor to center position
    ImGui::SetCursorPos(cardPos);

    // Begin card container
    ImGui::BeginChild("LoginCard", ImVec2(FORM_WIDTH, FORM_HEIGHT), true);

    float formWidth  = ImGui::GetContentRegionAvail().x;
    float formHeight = ImGui::GetContentRegionAvail().y;

    float totalHeight =
        ImGui::GetTextLineHeight() + // username label
        ImGui::GetFrameHeight() + // username input field
        SMALL_SPACING + // spacing (ImGui::Dummy(ImVec2(0, 5));)
        ImGui::GetTextLineHeight() + // password label
        ImGui::GetFrameHeight() + // password input field
        BIG_SPACING + // spacing (ImGui::Dummy(ImVec2(0, 15));)
        ELEMENT_HEIGHT; // login button

    float centerX = (formWidth - ELEMENT_WIDTH) / 2;
    float centerY = (formHeight - totalHeight) / 2;

    // Put UIs at the center.
    ImGui::SetCursorPos(ImVec2(centerX, centerY));
    // Begin group to make the UIs put together.
    ImGui::BeginGroup();

    /* ======USERNAME====== */
    // Label for username
    ImGui::Text("Username");

    // Sets elemet width for input field.
    ImGui::PushItemWidth(ELEMENT_WIDTH);
    // Username input field
    ImGui::InputText("##username", username, IM_ARRAYSIZE(username));

    // Add vertical space
    ImGui::Dummy(ImVec2(0, SMALL_SPACING));
    /*============*/

    /* ======PASSWORD====== */
    // Label for password
    ImGui::Text("Password"); 
    // Password input field
    ImGui::InputText("##password", password, IM_ARRAYSIZE(password),
                     ImGuiInputTextFlags_Password); 

    // Sets it back to default.
    ImGui::PopItemWidth();

    // Add vertical space
    ImGui::Dummy(ImVec2(0, BIG_SPACING));
    /*============*/

    /*======LOGIN BUTTON======*/
    // Create login button
    if (ImGui::Button("Login", ImVec2(ELEMENT_WIDTH, ELEMENT_HEIGHT))) // -1 makes button full width
    {
        errorManager.clear();
        Result<string> dbResult = backend.getHashPassoword(username);
        if (!dbResult.isSuccess())
        {
            errorManager.addError(dbResult);
        }
        auto storedHash = dbResult.value;
        Result<bool> verifyResult = auth.verifyPassword(username, password, strlen(password), storedHash.c_str());
        if (verifyResult.value) {
            printf("Login successful for user: %s\n", username);

            Result<int> userIdResult = backend.setUserInfo(username);
            if (!userIdResult.isSuccess())
            {
                errorManager.addError(userIdResult);
            }

            printf("userId: %d. Move onto Main Page.\n", userIdResult.value);

            nextPage = PageType::Dashboard;
            
            // Delete static username and password because we don't need to use them anymore.
            memset(username, 0, sizeof(username));
            memset(password, 0, sizeof(password));
        }
        else 
        {
            errorManager.addError(verifyResult);
            printf("Login failed\n");
        }

        if (errorManager.hasError())
        {
            ImGui::OpenPopup("Login Failed");
        }
    }
    /*============*/

    ImGui::EndGroup();

    /*======POPUP======*/
    if (Popup::showMessage("Login Failed", errorManager.getErrors(), "OK"))
    {
        errorManager.clear();
    }
    /*============*/

    // End card container
    ImGui::EndChild();
    // End fullscreen window
    ImGui::End();

    return nextPage;
}
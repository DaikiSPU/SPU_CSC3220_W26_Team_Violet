#include "CreateAccountPage.h"
#include <cstdio>

PageType CreateAccountPage::draw()
{
    PageType nextPage = PageType::None;
    // Get current screen size
    ImVec2 screenSize = ImGui::GetIO().DisplaySize;

    // Make this window fullscreen
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(screenSize);

    // Begin fullscreen background window
    ImGui::Begin("CreateAccountPage",
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
    ImGui::BeginChild("CreateAccountCard", ImVec2(FORM_WIDTH, FORM_HEIGHT), true);

    float formWidth  = ImGui::GetContentRegionAvail().x;
    float formHeight = ImGui::GetContentRegionAvail().y;

    float totalHeight =
        ImGui::GetTextLineHeight() + // username label
        ImGui::GetFrameHeight() + // username input field
        SMALL_SPACING + // spacing (ImGui::Dummy(ImVec2(0, 5));)
        ImGui::GetTextLineHeight() + // password label
        ImGui::GetFrameHeight() + // password input field
        SMALL_SPACING + // spacing (ImGui::Dummy(ImVec2(0, 5));)
        ImGui::GetTextLineHeight() + // password label
        ImGui::GetFrameHeight() + // password input field
        BIG_SPACING + // spacing (ImGui::Dummy(ImVec2(0, 15));)
        ELEMENT_HEIGHT; // creation button

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

    // Add vertical space
    ImGui::Dummy(ImVec2(0, SMALL_SPACING));
    /*============*/

    /* ======PIN====== */
    // Label for pin
    ImGui::Text("Pin"); 
    // Password input field
    ImGui::InputText("##Pin", pin, IM_ARRAYSIZE(pin),
                     ImGuiInputTextFlags_Password); 

    // Sets it back to default.
    ImGui::PopItemWidth();

    // Add vertical space
    ImGui::Dummy(ImVec2(0, BIG_SPACING));
    /*============*/

    /* ======Creation BUTTON====== */
    // Create CreateAccount button
    if (ImGui::Button("Create", ImVec2(ELEMENT_WIDTH, ELEMENT_HEIGHT))) // -1 makes button full width
    {
        string hashedPassword = auth.createHashPassword(password, strlen(password));
        string hashedPin = auth.createHashPin(pin, strlen(pin));
        printf("%s\n", username);
        printf("%s\n", password);
        printf("%s\n", pin);
        printf("%s\n", hashedPin.c_str());
        printf("%s\n", hashedPassword.c_str());

        auto result = backend.registerAccount(username, hashedPassword, hashedPin);

        if (result.isSuccess())
        {
            nextPage = PageType::Login;
        }
        else
        {
            errorManager.addError(result);
        }
        if (errorManager.hasError())
        {
            ImGui::OpenPopup("Account creation failed");
        }
    }
    /*============*/

    ImGui::EndGroup();

    /*======POPUP======*/
    if (Popup::showMessage("Account creation failed", errorManager.getErrors(), "OK"))
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
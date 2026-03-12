#include "MainGui.h"
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <cstdio>

#include "Application.h"

MainGui::MainGui() {
    // Initializes GLFW (window and input system).
    glfwInit();
    
    // Requests OpenGL 4.1 Core Profile context (required for macOS).
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    // Creates a window and OpenGL context.
    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "ImGui Demo", nullptr, nullptr);
    if (!window) {
        printf("window creation failed");
        glfwTerminate();
        return;
    }

    // Makes the OpenGL context of this window current.
    glfwMakeContextCurrent(window);

    // Enables vertical synchronization (VSync).
    glfwSwapInterval(1);

    // Ensures ImGui version matches compiled version.
    IMGUI_CHECKVERSION();

    // Creates ImGui context (internal state storage).
    ImGui::CreateContext();

    // Changes font and font size.
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->AddFontFromFileTTF("assets/fonts/times.ttf", 22.0f);

    // To enable docker
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // Hides window menu
    ImGui::GetStyle().WindowMenuButtonPosition = ImGuiDir_None;

    // Applies dark theme styling.
    ImGui::StyleColorsDark();

    // Initializes ImGui GLFW backend (handles input).
    ImGui_ImplGlfw_InitForOpenGL(window, true);

    // Initializes ImGui OpenGL3 backend (handles rendering).
    ImGui_ImplOpenGL3_Init(GLSL_VERSION);
}

void MainGui::run(PageManager& pageManager, Application& app) {
    // Main render loop (runs until window is closed).
    while (!glfwWindowShouldClose(window))
    {
        // Polls input and window events.
        glfwPollEvents();

        // Starts a new ImGui frame.
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        Page* page = pageManager.getCurrentPage();
        if (page)
        {
            if (!page->getStopApp()) 
            {
                app.update();
            }
            if (page->getCloseApp()) 
            {
                printf("gui stop\n");
                stop();
            }
            // Draws a page.
            pageManager.draw();
        }

        // Finalizes ImGui draw data for this frame.
        ImGui::Render();

        int display_w, display_h;

        // Retrieves actual framebuffer pixel size (important for Retina).
        glfwGetFramebufferSize(window, &display_w, &display_h);

        // Sets OpenGL viewport.
        glViewport(0, 0, display_w, display_h);

        // Clears the color buffer (prepares background for new frame).
        glClear(GL_COLOR_BUFFER_BIT);
        
        // Renders ImGui draw data using OpenGL.
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Swaps front and back buffers (displays rendered frame).
        glfwSwapBuffers(window);
    }

    // Shuts down ImGui OpenGL backend.
    ImGui_ImplOpenGL3_Shutdown();

    // Shuts down ImGui GLFW backend.
    ImGui_ImplGlfw_Shutdown();

    // Destroys ImGui context.
    ImGui::DestroyContext();

    // Destroys the GLFW window.
    glfwDestroyWindow(window);

    // Terminates GLFW and releases resources.
    glfwTerminate();
}

void MainGui::stop()
{
    printf("stop maingui \n");
    glfwSetWindowShouldClose(window, true);
}

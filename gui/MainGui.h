#pragma once

#include "PageManager.h"

class Application;

#define GL_SILENCE_DEPRECATION
struct GLFWwindow;

class MainGui {
    public:
        MainGui();
        ~MainGui() = default;

        void run(PageManager& pageManager, Application& app);
        void stop();
    protected:
    private:
        // window height
        int WINDOW_HEIGHT = 600;
        // window width
        int WINDOW_WIDTH = 800;

        // window
        GLFWwindow* window = nullptr;
        // Specifies GLSL version used by ImGui's shader.
        const char* GLSL_VERSION = "#version 410";
};
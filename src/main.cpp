#include <stdio.h>
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include "gui.h"

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

static ImFont* g_ui_bold_font = NULL;

ImFont* GetUiBoldFont()
{
    return g_ui_bold_font;
}

static void configure_ui_scale(ImGuiIO& io)
{
    constexpr float ui_scale = 1.35f;
    constexpr float font_size = 24.0f;
    const char* regular_font_paths[] = {
#ifdef _WIN32
        "C:/Windows/Fonts/segoeui.ttf",
#elif defined(__APPLE__)
        "/System/Library/Fonts/SFNS.ttf",
#else
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
#endif
        NULL
    };
    const char* bold_font_paths[] = {
#ifdef _WIN32
        "C:/Windows/Fonts/segoeuib.ttf",
#elif defined(__APPLE__)
        "/System/Library/Fonts/SFNS-Bold.ttf",
#else
        "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
#endif
        NULL
    };

    ImFont* regular_font = NULL;
    for (int i = 0; regular_font_paths[i] != NULL && regular_font == NULL; ++i) {
        regular_font = io.Fonts->AddFontFromFileTTF(regular_font_paths[i], font_size);
    }

    if (regular_font == NULL) {
        ImFontConfig fallback_config;
        fallback_config.SizePixels = font_size;
        regular_font = io.Fonts->AddFontDefault(&fallback_config);
    }

    for (int i = 0; bold_font_paths[i] != NULL && g_ui_bold_font == NULL; ++i) {
        g_ui_bold_font = io.Fonts->AddFontFromFileTTF(bold_font_paths[i], font_size);
    }

    if (g_ui_bold_font == NULL) {
        g_ui_bold_font = regular_font;
    }

    io.FontDefault = regular_font;
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(ui_scale);
    style.ItemSpacing.y += 2.0f;
    style.FramePadding.y += 1.0f;
}

int main(int, char**)
{
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Grow Castle Progress Tracker", NULL, NULL);
    if (window == NULL)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    configure_ui_scale(io);
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ShowApplication();

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

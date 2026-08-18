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

static void configure_ui_style()
{
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowPadding = ImVec2(18.0f, 16.0f);
    style.FramePadding = ImVec2(10.0f, 7.0f);
    style.ItemSpacing = ImVec2(10.0f, 8.0f);
    style.ItemInnerSpacing = ImVec2(8.0f, 6.0f);
    style.CellPadding = ImVec2(8.0f, 6.0f);
    style.ScrollbarSize = 16.0f;
    style.GrabMinSize = 12.0f;
    style.WindowRounding = 6.0f;
    style.ChildRounding = 5.0f;
    style.FrameRounding = 4.0f;
    style.PopupRounding = 5.0f;
    style.TabRounding = 4.0f;
    style.WindowBorderSize = 1.0f;
    style.ChildBorderSize = 1.0f;
    style.FrameBorderSize = 1.0f;
    style.PopupBorderSize = 1.0f;

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.055f, 0.070f, 0.105f, 1.0f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.070f, 0.090f, 0.135f, 1.0f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.075f, 0.095f, 0.145f, 0.98f);
    colors[ImGuiCol_Border] = ImVec4(0.18f, 0.34f, 0.52f, 0.65f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.105f, 0.145f, 0.205f, 1.0f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.14f, 0.24f, 0.36f, 1.0f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.16f, 0.30f, 0.46f, 1.0f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.045f, 0.070f, 0.115f, 1.0f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.075f, 0.145f, 0.235f, 1.0f);
    colors[ImGuiCol_Tab] = ImVec4(0.075f, 0.145f, 0.235f, 1.0f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.12f, 0.30f, 0.50f, 1.0f);
    colors[ImGuiCol_TabSelected] = ImVec4(0.10f, 0.24f, 0.40f, 1.0f);
    colors[ImGuiCol_TabSelectedOverline] = ImVec4(0.30f, 0.65f, 1.0f, 1.0f);
    colors[ImGuiCol_Button] = ImVec4(0.10f, 0.27f, 0.46f, 1.0f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.14f, 0.38f, 0.62f, 1.0f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.08f, 0.20f, 0.34f, 1.0f);
    colors[ImGuiCol_Header] = ImVec4(0.10f, 0.24f, 0.40f, 1.0f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.14f, 0.34f, 0.55f, 1.0f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.17f, 0.40f, 0.64f, 1.0f);
    colors[ImGuiCol_TableHeaderBg] = ImVec4(0.09f, 0.20f, 0.34f, 1.0f);
    colors[ImGuiCol_TableBorderStrong] = ImVec4(0.20f, 0.38f, 0.58f, 0.85f);
    colors[ImGuiCol_TableBorderLight] = ImVec4(0.14f, 0.25f, 0.38f, 0.65f);
    colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.08f, 0.115f, 0.17f, 0.55f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.35f, 0.72f, 1.0f, 1.0f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.25f, 0.58f, 0.88f, 1.0f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.40f, 0.72f, 1.0f, 1.0f);
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
    configure_ui_style();

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

#define NOMINMAX // Prevent Windows.h from defining min/max macros

#include <iostream>
#include <vector>
#include <string>
#include <algorithm> // for std::min/std::max

// Loader must be included before GLFW/ImGui internals usually, or ImGui backend handles it
// #include <GL/gl3w.h> 
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "portable-file-dialogs.h"

#include "FileScanner.h"

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

// Simple Log Helper
struct AppLog {
    ImGuiTextBuffer     Buf;
    ImGuiTextFilter     Filter;
    ImVector<int>       LineOffsets; // Index to lines offset. We maintain this with AddLog() calls.
    bool                AutoScroll;  // Keep scrolling if already at the bottom.

    AppLog() {
        AutoScroll = true;
        Clear();
    }

    void Clear() {
        Buf.clear();
        LineOffsets.clear();
        LineOffsets.push_back(0);
    }

    void AddLog(const char* fmt, ...) IM_FMTARGS(2) {
        int old_size = Buf.size();
        va_list args;
        va_start(args, fmt);
        Buf.appendfv(fmt, args);
        va_end(args);
        for (int new_size = Buf.size(); old_size < new_size; old_size++)
            if (Buf[old_size] == '\n')
                LineOffsets.push_back(old_size + 1);
    }

    void Draw(const char* title, bool* p_open = NULL) {
        if (!ImGui::Begin(title, p_open)) {
            ImGui::End();
            return;
        }

        // Options menu
        if (ImGui::BeginPopup("Options")) {
            ImGui::Checkbox("Auto-scroll", &AutoScroll);
            ImGui::EndPopup();
        }

        // Main window
        if (ImGui::Button("Options"))
            ImGui::OpenPopup("Options");
        ImGui::SameLine();
        bool clear = ImGui::Button("Clear");
        ImGui::SameLine();
        bool copy = ImGui::Button("Copy");
        ImGui::SameLine();
        Filter.Draw("Filter", -100.0f);

        if (clear) Clear();
        if (copy) ImGui::LogToClipboard();

        ImGui::Separator();
        ImGui::BeginChild("scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

        if (copy) ImGui::LogToClipboard();

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
        const char* buf = Buf.begin();
        const char* buf_end = Buf.end();
        if (Filter.IsActive()) {
            for (int line_no = 0; line_no < LineOffsets.Size; line_no++) {
                const char* line_start = buf + LineOffsets[line_no];
                const char* line_end = (line_no + 1 < LineOffsets.Size) ? (buf + LineOffsets[line_no + 1] - 1) : buf_end;
                if (Filter.PassFilter(line_start, line_end))
                    ImGui::TextUnformatted(line_start, line_end);
            }
        }
        else {
            ImGui::TextUnformatted(buf, buf_end);
        }
        ImGui::PopStyleVar();

        if (AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
            ImGui::SetScrollHereY(1.0f);

        ImGui::EndChild();
        ImGui::End();
    }
};

int main(int, char**)
{
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    // Decide GL+GLSL versions
#if defined(__APPLE__)
    // GL 3.3 Core + GLSL 150
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
    // GL 3.3 + GLSL 330
    const char* glsl_version = "#version 330";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

    // Create window with graphics context
    GLFWwindow* window = glfwCreateWindow(1280, 720, "FileNamesManager", NULL, NULL);
    if (window == NULL)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Application State
    FileScanner scanner;
    AppLog my_log;
    my_log.AddLog("Welcome to FileNamesManager!\n");
    
    // Rename State
    bool show_rename_popup = false;
    char rename_suffix_buffer[256] = "";
    
    // Filter & Scan State
    char filter_buffer[256] = "";
    bool apply_filter_to_all_folders = false; // "Apply Filter To All" (Recursive)
    
    int last_selected_index = -1; // For Shift+Click range selection

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Root Window covering whole viewport
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        // ImGui::SetNextWindowViewport(viewport->ID); // Docking branch only
        
        ImGuiWindowFlags window_flags = 0;
        // ImGuiWindowFlags_NoDocking is docking branch only. ImGuiWindowFlags_NoMenuBar does not exist (it is default).
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::Begin("MainDockSpace", nullptr, window_flags);
        ImGui::PopStyleVar(2);
        
        // Define layout using child windows or tables to put log at bottom
        float log_height = 150.0f;
        float main_area_height = ImGui::GetContentRegionAvail().y - log_height - 10.0f;

        // --- Top Bar ---
        if (ImGui::Button("Select Folder")) {
            auto selection = pfd::select_folder("Select Directory", "").result();
            if (!selection.empty()) {
                scanner.ScanDirectory(selection, apply_filter_to_all_folders);
                my_log.AddLog("Scanned directory: %s (Recursive: %s)\n", selection.c_str(), apply_filter_to_all_folders ? "Yes" : "No");
                last_selected_index = -1;
            }
        }
        ImGui::SameLine();
        ImGui::Text("Path: %s", scanner.GetCurrentPath().c_str());

        ImGui::Separator();

        // --- Control Panel ---
        // Filter
        ImGui::Text("Filter:");
        ImGui::SameLine();
        if (ImGui::InputText("##filter", filter_buffer, IM_ARRAYSIZE(filter_buffer))) {
            scanner.ApplyFilter(filter_buffer);
        }

        // Apply Filter To All (Recursive Scan)
        ImGui::SameLine();
        if (ImGui::Checkbox("Apply Filter To All", &apply_filter_to_all_folders)) {
             // Optional: Immediate rescan if a path is already selected?
             // For now, just toggles the state for next scan or manual refresh.
             // Let's allow manual rescan if path exists
             if (!scanner.GetCurrentPath().empty()) {
                 scanner.ScanDirectory(scanner.GetCurrentPath(), apply_filter_to_all_folders);
                 scanner.ApplyFilter(filter_buffer); // Re-apply filter
                 my_log.AddLog("Re-scanned with Recursive: %s\n", apply_filter_to_all_folders ? "Yes" : "No");
                 last_selected_index = -1;
             }
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Apply's fillter to all folders incloding sub folders");
        }

        ImGui::SameLine();
        if (ImGui::Button("Select All")) {
            for (auto& f : scanner.GetFilesModifiable()) {
                if (!f.is_filtered) f.is_selected = true;
            }
            my_log.AddLog("Selected all visible files.\n");
        }
        ImGui::SameLine();
        if (ImGui::Button("Deselect All")) {
            for (auto& f : scanner.GetFilesModifiable()) {
                f.is_selected = false;
            }
            my_log.AddLog("Deselected all files.\n");
            last_selected_index = -1;
        }
        
        // Count selected
        int selected_count = 0;
        for(const auto& f : scanner.GetFiles()) if(f.is_selected) selected_count++;

        ImGui::SameLine();
        ImGui::Text("| %d Selected", selected_count);

        // Actions
        ImGui::SameLine();
        ImGui::BeginDisabled(selected_count == 0);
        
        if (ImGui::Button("Delete")) {
             // Confirmation? For now just action
             int deleted = scanner.ExecuteDelete();
             if (deleted > 0)
                my_log.AddLog("Deleted %d files.\n", deleted);
             last_selected_index = -1;
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Rename...")) {
            show_rename_popup = true;
            ImGui::OpenPopup("Rename Files");
        }
        ImGui::EndDisabled();

        // Rename Modal
        if (ImGui::BeginPopupModal("Rename Files", &show_rename_popup, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Rename %d selected files.", selected_count);
            ImGui::InputText("Suffix to append", rename_suffix_buffer, IM_ARRAYSIZE(rename_suffix_buffer));
            
            if (ImGui::Button("Execute Rename", ImVec2(120, 0))) {
                int renamed = scanner.ExecuteRename(rename_suffix_buffer);
                if (renamed > 0)
                    my_log.AddLog("Renamed %d files with suffix '%s'.\n", renamed, rename_suffix_buffer);
                else
                    my_log.AddLog("Rename failed or no files modified.\n");
                
                ImGui::CloseCurrentPopup();
                show_rename_popup = false;
                last_selected_index = -1;
            }
            ImGui::SetItemDefaultFocus();
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0))) { 
                ImGui::CloseCurrentPopup(); 
                show_rename_popup = false;
            }
            ImGui::EndPopup();
        }

        // --- Main Table ---
        ImGui::BeginChild("FileArea", ImVec2(0, main_area_height), true);
        static ImGuiTableFlags flags = ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable;
        
        if (ImGui::BeginTable("FileTable", 4, flags)) {
            ImGui::TableSetupScrollFreeze(0, 1); // Make top row always visible
            ImGui::TableSetupColumn("Select", ImGuiTableColumnFlags_WidthFixed, 50.0f);
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_None);
            ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 100.0f);
            ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 100.0f);
            ImGui::TableHeadersRow();

            auto& files = scanner.GetFilesModifiable();
            // Need index for range selection, so use standard for loop
            for (int i = 0; i < (int)files.size(); i++) {
                auto& file = files[i];
                if (file.is_filtered) continue;

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                
                // Custom behavior for Shift+Click
                // We use a custom Checkbox-like behavior or just intercept the click
                bool clicked = ImGui::Checkbox(("##" + std::to_string(i)).c_str(), &file.is_selected);
                
                if (clicked) {
                     if (ImGui::GetIO().KeyShift && last_selected_index != -1) {
                         // Range select
                         int start = (std::min)(last_selected_index, i);
                         int end = (std::max)(last_selected_index, i);
                         bool new_state = file.is_selected; // The state we just toggled to
                         
                         for (int k = start; k <= end; k++) {
                             if (!files[k].is_filtered) {
                                  files[k].is_selected = new_state;
                             }
                         }
                     }
                     last_selected_index = i;
                }

                ImGui::TableNextColumn();
                ImGui::TextUnformatted(file.name.c_str());
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", file.path.c_str()); // Show full path on hover
                
                ImGui::TableNextColumn();
                if (file.is_directory)
                    ImGui::Text("-");
                else
                    ImGui::Text("%zu B", file.size);
                
                ImGui::TableNextColumn();
                ImGui::Text(file.is_directory ? "Folder" : "File");
            }
            ImGui::EndTable();
        }
        ImGui::EndChild();

        // --- Log Window ---
        my_log.Draw("Log Output", NULL);

        ImGui::End();

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

#ifdef _WIN32
#include <windows.h>
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    return main(__argc, __argv);
}
#endif

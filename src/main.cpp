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

    void Draw() {
        // Options menu
        if (ImGui::BeginPopup("Options")) {
            ImGui::Checkbox("Auto-scroll", &AutoScroll);
            ImGui::EndPopup();
        }

        // Controls
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
    }
};

void SetupStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    style.WindowRounding = 0.0f;
    style.ChildRounding = 0.0f;
    style.FrameRounding = 3.0f;
    style.GrabRounding = 3.0f;
    style.PopupRounding = 3.0f;
    style.ScrollbarRounding = 3.0f;
    style.TabRounding = 3.0f;
    
    // Charcoal Black & Dark Gray
    colors[ImGuiCol_Text]                   = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    colors[ImGuiCol_TextDisabled]           = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    colors[ImGuiCol_WindowBg]               = ImVec4(0.12f, 0.12f, 0.12f, 1.00f); // Charcoal
    colors[ImGuiCol_ChildBg]                = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    colors[ImGuiCol_PopupBg]                = ImVec4(0.15f, 0.15f, 0.15f, 0.98f);
    colors[ImGuiCol_Border]                 = ImVec4(0.25f, 0.25f, 0.25f, 1.00f); 
    colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]                = ImVec4(0.18f, 0.18f, 0.19f, 1.00f); 
    colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.25f, 0.25f, 0.26f, 1.00f);
    colors[ImGuiCol_FrameBgActive]          = ImVec4(0.30f, 0.30f, 0.31f, 1.00f);
    colors[ImGuiCol_TitleBg]                = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
    colors[ImGuiCol_TitleBgActive]          = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_MenuBarBg]              = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    
    // Muted Blue Accents
    colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.10f, 0.10f, 0.10f, 0.60f);
    colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
    
    colors[ImGuiCol_CheckMark]              = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_SliderGrab]             = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    
    colors[ImGuiCol_Button]                 = ImVec4(0.20f, 0.25f, 0.30f, 1.00f); // Default button (dark blue-grey)
    colors[ImGuiCol_ButtonHovered]          = ImVec4(0.28f, 0.35f, 0.42f, 1.00f);
    colors[ImGuiCol_ButtonActive]           = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
    
    colors[ImGuiCol_Header]                 = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
    colors[ImGuiCol_HeaderHovered]          = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_HeaderActive]           = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    
    colors[ImGuiCol_Separator]              = colors[ImGuiCol_Border];
    colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
    colors[ImGuiCol_SeparatorActive]        = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
    
    colors[ImGuiCol_ResizeGrip]             = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
    colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    
    colors[ImGuiCol_PlotLines]              = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram]          = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    colors[ImGuiCol_DragDropTarget]         = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavHighlight]           = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
}

int main(int, char**)
{
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    // Decide GL+GLSL versions
#if defined(__APPLE__)
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); 
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); 
#else
    const char* glsl_version = "#version 330";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif

    GLFWwindow* window = glfwCreateWindow(1280, 800, "FileNamesManager", NULL, NULL);
    if (window == NULL)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); 

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    SetupStyle(); // Apply custom style

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // App State
    FileScanner scanner;
    AppLog my_log;
    my_log.AddLog("Welcome to FileNamesManager!\n");
    
    bool show_rename_popup = false;
    char rename_suffix_buffer[256] = "";
    char filter_buffer[256] = "";
    bool is_recursive_mode = false;
    int last_selected_index = -1;

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        
        ImGuiWindowFlags window_flags = 0;
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 10.0f)); 
        ImGui::Begin("MainDockSpace", nullptr, window_flags);
        ImGui::PopStyleVar(3);
        
        // Count selected
        int selected_count = 0;
        for(const auto& f : scanner.GetFiles()) if(f.is_selected) selected_count++;

        // --- 1. Top Toolbar ---
        // Blue "Select Folder" Button
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.26f, 0.59f, 0.98f, 1.00f)); 
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.35f, 0.65f, 1.00f, 1.00f));
        if (ImGui::Button("Select Folder", ImVec2(120, 30))) {
            auto selection = pfd::select_folder("Select Directory", "").result();
            if (!selection.empty()) {
                scanner.ScanDirectory(selection, is_recursive_mode);
                my_log.AddLog("Scanned directory: %s\n", selection.c_str());
                last_selected_index = -1;
            }
        }
        ImGui::PopStyleColor(2);

        ImGui::SameLine();
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Path:");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s", scanner.GetCurrentPath().empty() ? "[No Folder Selected]" : scanner.GetCurrentPath().c_str());

        // Right-aligned Selected Count
        float count_width = 150.0f;
        ImGui::SameLine(ImGui::GetContentRegionAvail().x - count_width);
        ImGui::Text("%d Selected Files", selected_count);

        ImGui::Dummy(ImVec2(0, 5)); // Spacer

        // --- 2. Filter Bar ---
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Filter:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(400);
        if (ImGui::InputText("##filter", filter_buffer, IM_ARRAYSIZE(filter_buffer))) {
            scanner.ApplyFilter(filter_buffer);
        }

       ImGui::SameLine();
        if (ImGui::Checkbox("Recursive Scan", &is_recursive_mode)) {
            std::string current_path = scanner.GetCurrentPath();
            if (!current_path.empty()) {
                scanner.ScanDirectory(current_path, is_recursive_mode);
                scanner.ApplyFilter(filter_buffer);
                
                my_log.AddLog("[System] Recursive scan toggled: %s\n", is_recursive_mode ? "ENABLED" : "DISABLED");
                last_selected_index = -1;
            }
        }
        if (ImGui::IsItemHovered()) 
            ImGui::SetTooltip("Apply filter to all subdirectories within the current path.");

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

        ImGui::Dummy(ImVec2(0, 5)); // Spacer

        // --- 3. Main File Table ---
        float log_height = 180.0f;
        float actions_height = 50.0f;
        float table_height = ImGui::GetContentRegionAvail().y - log_height - actions_height - 10.0f;
        
        ImGui::BeginChild("FileArea", ImVec2(0, table_height), true);
        
        // Handle Shortcuts (Must be done before Table to catch events, or inside if focused, but Window focus is safe)
        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows)) {
            if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_A)) {
                for (auto& f : scanner.GetFilesModifiable()) {
                    if (!f.is_filtered) f.is_selected = true;
                }
                my_log.AddLog("Selected all visible files (Ctrl+A).\n");
            }
            if (ImGui::IsKeyPressed(ImGuiKey_Delete) && selected_count > 0) {
                 int deleted = scanner.ExecuteDelete();
                 if (deleted > 0) my_log.AddLog("Deleted %d files (Del).\n", deleted);
                 last_selected_index = -1;
            }
            if (ImGui::IsKeyPressed(ImGuiKey_F2) && selected_count > 0) {
                show_rename_popup = true;
                ImGui::OpenPopup("Rename Files");
            }
        }

        static ImGuiTableFlags table_flags = ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable;
        
        if (ImGui::BeginTable("FileTable", 4, table_flags)) {
            ImGui::TableSetupScrollFreeze(0, 1);
            // Select column is now just an indicator or redundant if whole row is selectable. 
            // Let's keep a small checkbox for visual clarity but allow row click.
            ImGui::TableSetupColumn("Select", ImGuiTableColumnFlags_WidthFixed, 30.0f);
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_None);
            ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 100.0f);
            ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 100.0f);
            ImGui::TableHeadersRow();

            auto& files = scanner.GetFilesModifiable();
            for (int i = 0; i < (int)files.size(); i++) {
                auto& file = files[i];
                if (file.is_filtered) continue;

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                
                // Selectable Row Logic
                // We want the whole row to be interactable. ImGui::Selectable spans width.
                // But we are in a table column. To make the WHOLE row selectable, we usually use Selectable in the first column 
                // with SpanAllColumns flag.
                
                bool is_selected = file.is_selected;
                ImGuiSelectableFlags selectable_flags = ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap;
                
                // Checkbox purely for visual or individual toggle without affecting selection logic of row?
                // Actually, if we use Selectable, we replace the checkbox interaction usually.
                // Let's draw the checkbox manually or just use the Selectable state.
                // We will use the Selectable for the interaction.
                
                // Interaction
                if (ImGui::Selectable(("##" + std::to_string(i)).c_str(), is_selected, selectable_flags)) {
                    if (ImGui::GetIO().KeyShift) {
                        // Shift+Click: Range Select
                        if (last_selected_index != -1) {
                             int start = (std::min)(last_selected_index, i);
                             int end = (std::max)(last_selected_index, i);
                             
                             // Deselect all others if Ctrl not held? Explorer usually keeps previous state if just Shift
                             // Standard Shift-Click logic: Select from Anchor to Current.
                             // If Ctrl is NOT held, we explicitly set the range and clear others? 
                             // Let's stick to simple "Add Range" or "Set Range".
                             
                             if (!ImGui::GetIO().KeyCtrl) {
                                 // Clear others if Ctrl is not held
                                 for (auto& f : files) f.is_selected = false;
                             }

                             for (int k = start; k <= end; k++) {
                                 if (!files[k].is_filtered) files[k].is_selected = true;
                             }
                        } else {
                            // No anchor, just select this one
                            file.is_selected = true;
                            last_selected_index = i;
                        }
                    } 
                    else if (ImGui::GetIO().KeyCtrl) {
                        // Ctrl+Click: Toggle
                        file.is_selected = !file.is_selected;
                        last_selected_index = i;
                    } 
                    else {
                        // Regular Click: Select Single, Clear Others
                        for (auto& f : files) f.is_selected = false;
                        file.is_selected = true;
                        last_selected_index = i;
                    }
                }
                
                // Visual Checkbox (Non-interactive mostly, reflects state)
                ImGui::SameLine();
                // Store cursor pos to draw over the Selectable? Or just draw simple text/icon
                // Using standard checkbox but pass a dummy bool or the real one (careful with ID)
                // Actually, Selectable handles the click. Just render a box state.
                ImGui::Text(file.is_selected ? "[X]" : "[ ]"); 

                ImGui::TableNextColumn();
                ImGui::TextUnformatted(file.name.c_str());
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", file.path.c_str());
                
                ImGui::TableNextColumn();
                if (file.is_directory) ImGui::Text("-");
                else ImGui::Text("%zu B", file.size);
                
                ImGui::TableNextColumn();
                ImGui::Text(file.is_directory ? "Folder" : "File");
            }
            ImGui::EndTable();
        }
        ImGui::EndChild();

        // --- 4. Actions Section ---
        ImGui::Dummy(ImVec2(0, 5));
        ImGui::Text("Actions:");
        ImGui::SameLine();
        
        ImGui::BeginDisabled(selected_count == 0);
        if (ImGui::Button("Delete Selected", ImVec2(150, 30))) {
             int deleted = scanner.ExecuteDelete();
             if (deleted > 0) my_log.AddLog("Deleted %d files.\n", deleted);
             last_selected_index = -1;
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Rename Selected", ImVec2(150, 30))) {
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
                if (renamed > 0) my_log.AddLog("Renamed %d files.\n", renamed);
                else my_log.AddLog("Rename failed.\n");
                
                ImGui::CloseCurrentPopup();
                show_rename_popup = false;
                last_selected_index = -1;
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0))) { 
                ImGui::CloseCurrentPopup(); 
                show_rename_popup = false;
            }
            ImGui::EndPopup();
        }

        ImGui::Dummy(ImVec2(0, 5));

        // --- 5. Log Panel ---
        ImGui::Dummy(ImVec2(0, 5));
        ImGui::Separator();
        ImGui::Text("Log Output:");
        
        // Use remaining space for log
        ImGui::BeginChild("LogPanel", ImVec2(0, 0), true);
        my_log.Draw();
        ImGui::EndChild();

        ImGui::End();

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.12f, 0.12f, 0.12f, 1.00f); // Match window bg
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

#ifdef _WIN32
#include <windows.h>
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    return main(__argc, __argv);
}
#endif

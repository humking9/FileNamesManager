# Project Plan - FileNamesManager

## Goals
- Develop a cross-platform C++ application using Dear ImGui, GLFW, and OpenGL 3.3.
- Target Windows 7+ (no .NET/VCRT deps), Linux, and macOS.
- Implement file scanning, filtering, and bulk actions (Delete/Rename).
- Ensure static linking for a standalone executable.

## TODO List & Status

### Project Setup
- [ ] Initialize CMake project structure
- [ ] Configure static linking settings (/MT for MSVC)
- [ ] Setup dependency management (GLFW, ImGui)

### Core Logic
- [ ] Implement `FileScanner` class structure
- [ ] Implement `ScanDirectory` using `std::filesystem`
- [ ] Implement `ApplyFilter` logic
- [ ] Implement `ExecuteAction` (Delete/Rename)

### GUI Development
- [ ] Select Folder Dialog integration
- [ ] Main File Table (ImGui Table API)
- [ ] Filter Modal/Panel
- [ ] Action execution triggers

## Current Status
- **Phase**: Code Complete
- **Next Step**: Install CMake and build manually.

# FileNamesManager

A cross-platform C++ application for file management using **Dear ImGui**, **GLFW**, and **OpenGL 3**.

## Features

- **File Scanning**: Recursively scan directories and view file details (Name, Size, Type).
- **Filtering**: Real-time filtering of files by name.
- **Selection**:
  - Individual checkboxes.
  - **Select All / Deselect All** buttons.
- **Actions**:
  - **Rename**: Batch rename selected files with a suffix.
  - **Delete**: Bulk delete selected files.
- **Logging**: Integrated log window to track operations and status.

## Technologies

- **C++17**
- **CMake**: Build system.
- **Dear ImGui**: Immediate Mode GUI.
- **GLFW**: Windowing and Input.
- **Portable File Dialogs**: Native file selection dialogs.

## Building

### Requirements
- CMake (3.14+)
- C++ Compiler (MSVC, GCC, or Clang)

### Instructions

1.  **Clone the repository:**
    ```bash
    git clone https://github.com/humking9/FileNamesManager.git
    cd FileNamesManager
    ```

2.  **Configure:**
    ```bash
    cmake -S . -B build
    ```

3.  **Build:**
    ```bash
    cmake --build build --config Release
    ```

4.  **Run:**
    - Windows: `build\Release\FileNamesManager.exe`
    - Linux/macOS: `./build/FileNamesManager`

## License

This project is built for educational/demonstration purposes.

#pragma once
#include <string>
#include <vector>
#include <filesystem>

enum class ActionType {
    Delete,
    Rename
};

struct FileEntry {
    std::string name;
    std::string path;
    uintmax_t size;
    bool is_directory;
    bool is_selected = false;
    bool is_filtered = false; // true if hidden by filter
};

class FileScanner {
public:
    void ScanDirectory(const std::string& path, bool recursive = false);
    void ApplyFilter(const std::string& pattern);
    
    // Returns number of successes
    int ExecuteDelete();
    
    // Simple rename: appends suffix to selected files
    int ExecuteRename(const std::string& suffix);

    const std::vector<FileEntry>& GetFiles() const { return m_files; }
    std::vector<FileEntry>& GetFilesModifiable() { return m_files; }
    const std::string& GetCurrentPath() const { return m_current_path; }

private:
    std::vector<FileEntry> m_files;
    std::string m_current_path;
};

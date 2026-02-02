#include "FileScanner.h"
#include <algorithm>
#include <iostream>
#include <system_error>

namespace fs = std::filesystem;

void FileScanner::ScanDirectory(const std::string& path, bool recursive) {
    m_files.clear();
    m_current_path = path;
    
    std::error_code ec;
    if (!fs::exists(path, ec) || !fs::is_directory(path, ec)) {
        return;
    }

    try {
        auto add_entry = [&](const fs::directory_entry& entry) {
            FileEntry file;
            file.path = entry.path().string();
            // Store relative path or full path? Using full path effectively.
            // For display, we might want just filename, but if recursive, maybe relative path to root?
            // For now, keeping simple filename for display, but user might want to see structure.
            // Let's stick to filename for now, maybe tooltip for full path?
            // Actually, if recursive, duplicates of filenames are possible.
            file.name = entry.path().filename().string();
            
            std::error_code status_ec;
            file.is_directory = entry.is_directory(status_ec);
            if (status_ec) file.is_directory = false;

            if (file.is_directory) {
                file.size = 0;
            } else {
                file.size = entry.file_size(status_ec);
                if (status_ec) file.size = 0;
            }
            
            file.is_selected = false;
            file.is_filtered = false;
            m_files.push_back(file);
        };

        if (recursive) {
            for (const auto& entry : fs::recursive_directory_iterator(path, fs::directory_options::skip_permission_denied, ec)) {
                if (ec) continue;
                add_entry(entry);
            }
        } else {
            for (const auto& entry : fs::directory_iterator(path, fs::directory_options::skip_permission_denied, ec)) {
                if (ec) continue;
                add_entry(entry);
            }
        }

    } catch (const fs::filesystem_error& e) {
        std::cerr << "Filesystem error during scan: " << e.what() << std::endl;
    }
}

void FileScanner::ApplyFilter(const std::string& pattern) {
    for (auto& file : m_files) {
        if (pattern.empty()) {
            file.is_filtered = false;
        } else {
            // Case-insensitive search could be done here, keeping it simple for now
            if (file.name.find(pattern) == std::string::npos) {
                file.is_filtered = true;
            } else {
                file.is_filtered = false;
            }
        }
    }
}

int FileScanner::ExecuteDelete() {
    int count = 0;
    for (auto it = m_files.begin(); it != m_files.end(); ) {
        if (it->is_selected && !it->is_filtered) {
            std::error_code ec;
            std::uintmax_t result = fs::remove_all(it->path, ec);
            if (!ec && result > 0) {
                it = m_files.erase(it);
                count++;
            } else {
                ++it;
            }
        } else {
            ++it;
        }
    }
    return count;
}

int FileScanner::ExecuteRename(const std::string& suffix) {
    int count = 0;
    if (suffix.empty()) return 0;
    
    for (auto& file : m_files) {
        if (file.is_selected && !file.is_filtered) {
            fs::path old_path(file.path);
            std::string new_name = old_path.stem().string() + suffix + old_path.extension().string();
            fs::path new_path = old_path.parent_path() / new_name;
            
            std::error_code ec;
            fs::rename(old_path, new_path, ec);
            if (!ec) {
                file.path = new_path.string();
                file.name = new_name;
                count++;
            }
        }
    }
    return count;
}

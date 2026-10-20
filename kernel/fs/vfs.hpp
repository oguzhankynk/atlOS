/**
 * @file vfs.hpp
 * @author Oguzhan KAYNAK
 * @brief Fs::Vfs: one tree of paths over every mounted file system.
 *
 *     /                 the mount points, as directories
 *     /disk0/...        the root of the file system mounted as "disk0"
 */

#pragma once

#include <stdint.h>

#include <fs/file_system.hpp>

namespace Fs {

class Vfs {
public:
    static constexpr uint32_t maxMounts = 8;
    static constexpr uint32_t maxMountNameLength = 15;

    Vfs() = default;
    Vfs(const Vfs&) = delete;
    Vfs& operator=(const Vfs&) = delete;

    /**
     * @brief Make fileSystem reachable as /name. false if the name is empty,
     * too long, contains '/', is taken, or there is no free slot.
     */
    bool mount(const char* name, FileSystem& fileSystem);

    /// The file system mounted as name, nullptr if none.
    FileSystem* find(const char* name);

    uint32_t mountCount() const { return m_count; }

    // Absolute paths ("/disk0/DOCS/README.TXT"), see FileSystem
    bool stat(const char* path, FileInfo& info);
    int64_t read(const char* path, uint64_t offset, void* buffer, uint64_t size);
    bool listDirectory(const char* path, DirectoryCallback callback, void* arg);

private:
    // "/disk0/a/b" -> the disk0 file system and "a/b"; nullptr for "/" or an unknown mount
    FileSystem* resolve(const char* path, const char*& rest);
    static bool isRoot(const char* path);

    struct Mount {
        char name[maxMountNameLength + 1];
        FileSystem* fileSystem;
    };

    Mount m_mounts[maxMounts] {};
    uint32_t m_count {};
};

} // namespace Fs

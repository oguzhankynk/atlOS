/**
 * @file file_system.hpp
 * @author Oguzhan KAYNAK
 * @brief Fs::FileSystem: the interface every file system implements; the VFS
 * only talks to this.
 *
 * Paths inside a file system are "/" separated and relative to its root:
 * "DOCS/README.TXT" (a leading "/" is allowed, "" or "/" is the root).
 */

#pragma once

#include <stdint.h>

namespace Fs {

/// "NAME.EXT" (8.3) is the longest name for now
static constexpr uint32_t maxNameLength = 12;

struct FileInfo {
    char name[maxNameLength + 1];
    bool isDirectory;
    uint64_t size; ///< bytes, 0 for directories
};

/// Called for every entry of a directory.
using DirectoryCallback = void (*)(const FileInfo& entry, void* arg);

class FileSystem {
public:
    /// Find path; false if it does not exist (or on a disk error).
    virtual bool stat(const char* path, FileInfo& info) = 0;

    /**
     * @brief Read up to size bytes of the file at path, starting at offset.
     * @return The number of bytes read (0 at or after the end of the file),
     * -1 if path is not a file or on a disk error or a broken cluster chain.
     */
    virtual int64_t read(const char* path, uint64_t offset, void* buffer, uint64_t size) = 0;

    /// Call callback for each entry of the directory at path ("." and ".." excluded).
    virtual bool listDirectory(const char* path, DirectoryCallback callback, void* arg) = 0;

    /// e.g. "fat16"
    virtual const char* typeName() const = 0;

protected:
    // Never deleted through the interface
    ~FileSystem() = default;
};

} // namespace Fs

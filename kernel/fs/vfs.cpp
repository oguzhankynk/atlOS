/**
 * @file vfs.cpp
 * @author Oguzhan KAYNAK
 * @brief Fs::Vfs: one tree of paths over every mounted file system.
 */

#include <fs/vfs.hpp>

#include <string.h>

namespace Fs {

bool Vfs::mount(const char* name, FileSystem& fileSystem)
{
    size_t length = strlen(name);
    if (!length || length > maxMountNameLength || strchr(name, '/') || find(name) || m_count == maxMounts)
        return false;

    Mount& mount = m_mounts[m_count++];
    strncpy(mount.name, name, maxMountNameLength); // length checked above, the rest is zero
    mount.fileSystem = &fileSystem;
    return true;
}

FileSystem* Vfs::find(const char* name)
{
    for (uint32_t i = 0; i < m_count; i++) {
        if (!strcmp(m_mounts[i].name, name))
            return m_mounts[i].fileSystem;
    }
    return nullptr;
}

bool Vfs::isRoot(const char* path)
{
    while (*path == '/')
        path++;
    return !*path;
}

FileSystem* Vfs::resolve(const char* path, const char*& rest)
{
    while (*path == '/')
        path++;

    size_t length = 0;
    while (path[length] && path[length] != '/')
        length++;
    if (!length || length > maxMountNameLength)
        return nullptr;

    char name[maxMountNameLength + 1];
    memcpy(name, path, length);
    name[length] = '\0';

    rest = path + length;
    return find(name);
}

bool Vfs::stat(const char* path, FileInfo& info)
{
    if (isRoot(path)) {
        strcpy(info.name, "/");
        info.isDirectory = true;
        info.size = 0;
        return true;
    }

    const char* rest;
    FileSystem* fileSystem = resolve(path, rest);
    return fileSystem && fileSystem->stat(rest, info);
}

int64_t Vfs::read(const char* path, uint64_t offset, void* buffer, uint64_t size)
{
    const char* rest;
    FileSystem* fileSystem = resolve(path, rest);
    if (!fileSystem)
        return -1;
    return fileSystem->read(rest, offset, buffer, size);
}

bool Vfs::listDirectory(const char* path, DirectoryCallback callback, void* arg)
{
    // The root holds no files, only the mount points
    if (isRoot(path)) {
        for (uint32_t i = 0; i < m_count; i++) {
            FileInfo info {};
            strncpy(info.name, m_mounts[i].name, maxNameLength);
            info.isDirectory = true;
            callback(info, arg);
        }
        return true;
    }

    const char* rest;
    FileSystem* fileSystem = resolve(path, rest);
    return fileSystem && fileSystem->listDirectory(rest, callback, arg);
}

// The root and the mount points themselves can not be changed: an empty
// rest of the path ("/disk0") is refused by the file system (no parent)
bool Vfs::create(const char* path)
{
    const char* rest;
    FileSystem* fileSystem = resolve(path, rest);
    return fileSystem && fileSystem->create(rest);
}

int64_t Vfs::write(const char* path, uint64_t offset, const void* buffer, uint64_t size)
{
    const char* rest;
    FileSystem* fileSystem = resolve(path, rest);
    if (!fileSystem)
        return -1;
    return fileSystem->write(rest, offset, buffer, size);
}

bool Vfs::makeDirectory(const char* path)
{
    const char* rest;
    FileSystem* fileSystem = resolve(path, rest);
    return fileSystem && fileSystem->makeDirectory(rest);
}

bool Vfs::remove(const char* path)
{
    const char* rest;
    FileSystem* fileSystem = resolve(path, rest);
    return fileSystem && fileSystem->remove(rest);
}

bool Vfs::removeDirectory(const char* path)
{
    const char* rest;
    FileSystem* fileSystem = resolve(path, rest);
    return fileSystem && fileSystem->removeDirectory(rest);
}

} // namespace Fs

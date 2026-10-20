/**
 * @file fat16.cpp
 * @author Oguzhan KAYNAK
 * @brief Fs::Fat16: a read only FAT16 file system with 8.3 names.
 */

#include <fs/fat16.hpp>

#include <string.h>

namespace Fs {

using Drivers::BlockDevice;

// The fields of the boot sector the driver needs (the BIOS Parameter Block)
struct BootSector {
    uint8_t jump[3];
    uint8_t oemName[8];
    uint16_t bytesPerSector;
    uint8_t sectorsPerCluster;
    uint16_t reservedSectors; // including the boot sector
    uint8_t fatCount;
    uint16_t rootEntryCount;
    uint16_t totalSectors16; // 0: see totalSectors32
    uint8_t media;
    uint16_t sectorsPerFat;
    uint16_t sectorsPerTrack;
    uint16_t headCount;
    uint32_t hiddenSectors;
    uint32_t totalSectors32;
    uint8_t rest[474];
    uint8_t signature[2]; // 0x55 0xAA
} __attribute__((packed));

static_assert(sizeof(BootSector) == BlockDevice::sectorSize, "the boot sector is one sector");
static_assert(sizeof(Fat16::DirectoryEntry) == 32, "directory entries are 32 bytes");

static constexpr uint8_t attributeVolumeLabel = 0x08;
static constexpr uint8_t attributeDirectory = 0x10;
static constexpr uint8_t attributeLongName = 0x0F; // read only + hidden + system + volume: an LFN part

static constexpr uint8_t entryEnd = 0x00; // first name byte: no entries after this one
static constexpr uint8_t entryDeleted = 0xE5;

static constexpr uint16_t chainBad = 0xFFF7;
static constexpr uint16_t chainEnd = 0xFFF8; // 0xFFF8..0xFFFF

// The FAT type is decided by the number of clusters, nothing else (Microsoft's FAT spec)
static constexpr uint32_t minFat16Clusters = 4085;
static constexpr uint32_t maxFat16Clusters = 65524;

static constexpr uint32_t entriesPerSector = BlockDevice::sectorSize / sizeof(Fat16::DirectoryEntry);
static constexpr uint32_t fatEntriesPerSector = BlockDevice::sectorSize / 2;

static char toUpper(char c)
{
    return (c >= 'a' && c <= 'z') ? static_cast<char>(c - 'a' + 'A') : c;
}

Fat16::Fat16(Drivers::BlockDevice& volume)
    : m_volume(volume)
{
}

bool Fat16::mount()
{
    BootSector boot;
    if (!m_volume.readSectors(0, 1, &boot))
        return false;

    if (boot.signature[0] != 0x55 || boot.signature[1] != 0xAA)
        return false;

    // Only 512 byte sectors, as the block devices use
    uint32_t spc = boot.sectorsPerCluster;
    if (boot.bytesPerSector != BlockDevice::sectorSize || !spc || (spc & (spc - 1)) || !boot.reservedSectors
        || !boot.fatCount || !boot.rootEntryCount || !boot.sectorsPerFat)
        return false;

    uint64_t totalSectors = boot.totalSectors16 ? boot.totalSectors16 : boot.totalSectors32;
    if (totalSectors > m_volume.sectorCount())
        return false;

    m_sectorsPerCluster = spc;
    m_firstFatSector = boot.reservedSectors;
    m_firstRootSector = m_firstFatSector + boot.fatCount * boot.sectorsPerFat;
    m_rootSectors = (boot.rootEntryCount * sizeof(DirectoryEntry) + BlockDevice::sectorSize - 1) / BlockDevice::sectorSize;
    m_firstDataSector = m_firstRootSector + m_rootSectors;
    if (m_firstDataSector >= totalSectors)
        return false;

    m_clusterCount = (totalSectors - m_firstDataSector) / spc;
    if (m_clusterCount < minFat16Clusters || m_clusterCount > maxFat16Clusters)
        return false;

    // The FAT must have an entry for every cluster (plus the two reserved ones)
    if ((uint64_t)boot.sectorsPerFat * fatEntriesPerSector < m_clusterCount + 2)
        return false;

    m_fatSector = UINT32_MAX;
    m_mounted = true;
    return true;
}

uint64_t Fat16::clusterLba(uint16_t cluster) const
{
    // Data clusters are numbered from 2
    return m_firstDataSector + (uint64_t)(cluster - 2) * m_sectorsPerCluster;
}

Fat16::ChainStep Fat16::nextCluster(uint16_t cluster, uint16_t& next)
{
    // Two bytes per cluster: the FAT sector is cluster / 256
    uint32_t sector = cluster / fatEntriesPerSector;
    if (sector != m_fatSector) {
        if (!m_volume.readSectors(m_firstFatSector + sector, 1, m_fat))
            return ChainStep::Broken;
        m_fatSector = sector;
    }

    uint16_t value = m_fat[cluster % fatEntriesPerSector];
    if (value >= chainEnd)
        return ChainStep::End;
    if (value == chainBad || !isDataCluster(value))
        return ChainStep::Broken; // a free (0), reserved or bad cluster in a chain
    next = value;
    return ChainStep::Next;
}

bool Fat16::toShortName(const char* component, uint32_t length, uint8_t shortName[11])
{
    memset(shortName, ' ', 11);

    // "." and ".." exist as entries in subdirectories
    if ((length == 1 || length == 2) && component[0] == '.' && component[length - 1] == '.') {
        memcpy(shortName, component, length);
        return true;
    }

    uint32_t i = 0;
    uint32_t out = 0;
    for (; i < length && component[i] != '.'; i++) {
        if (out == 8)
            return false;
        shortName[out++] = toUpper(component[i]);
    }
    if (!out)
        return false;

    if (i < length) {
        i++; // the dot
        out = 8;
        for (; i < length; i++) {
            if (out == 11 || component[i] == '.')
                return false;
            shortName[out++] = toUpper(component[i]);
        }
    }
    return true;
}

void Fat16::fromShortName(const uint8_t shortName[11], char name[maxNameLength + 1])
{
    uint32_t out = 0;
    for (int i = 0; i < 8 && shortName[i] != ' '; i++)
        name[out++] = static_cast<char>(shortName[i]);
    // 0x05 stands for a real 0xE5 first character (0xE5 marks deleted entries)
    if (out && static_cast<uint8_t>(name[0]) == 0x05)
        name[0] = static_cast<char>(0xE5);

    if (shortName[8] != ' ') {
        name[out++] = '.';
        for (int i = 8; i < 11 && shortName[i] != ' '; i++)
            name[out++] = static_cast<char>(shortName[i]);
    }
    name[out] = '\0';
}

bool Fat16::visitSector(uint64_t lba, EntryVisitor visit, void* arg, bool& done)
{
    // Not m_sector: a visitor (or a listDirectory callback) may read the volume
    DirectoryEntry entries[entriesPerSector];
    if (!m_volume.readSectors(lba, 1, entries))
        return false;

    for (const DirectoryEntry& entry : entries) {

        if (entry.name[0] == entryEnd) {
            done = true;
            return true;
        }
        if (entry.name[0] == entryDeleted || entry.attributes == attributeLongName
            || (entry.attributes & attributeVolumeLabel))
            continue;

        if (visit(entry, arg)) {
            done = true;
            return true;
        }
    }
    return true;
}

bool Fat16::forEachEntry(const Node& directory, EntryVisitor visit, void* arg)
{
    bool done = false;

    // The root directory has a fixed place and size
    if (directory.isRoot) {
        for (uint32_t s = 0; s < m_rootSectors && !done; s++) {
            if (!visitSector(m_firstRootSector + s, visit, arg, done))
                return false;
        }
        return true;
    }

    // A subdirectory is a cluster chain like a file. A chain longer than the
    // number of clusters loops: stop there instead of walking forever
    uint16_t cluster = directory.firstCluster;
    for (uint32_t steps = 0; steps < m_clusterCount; steps++) {
        if (!isDataCluster(cluster))
            return false;

        for (uint32_t s = 0; s < m_sectorsPerCluster && !done; s++) {
            if (!visitSector(clusterLba(cluster) + s, visit, arg, done))
                return false;
        }
        if (done)
            return true;

        switch (nextCluster(cluster, cluster)) {
        case ChainStep::Next:
            break;
        case ChainStep::End:
            return true;
        case ChainStep::Broken:
            return false;
        }
    }
    return false;
}

namespace {
    struct FindArg {
        const uint8_t* shortName;
        Fat16::DirectoryEntry found;
        bool isFound;
    };

    struct ListArg {
        DirectoryCallback callback;
        void* arg;
    };
} // namespace

bool Fat16::lookup(const char* path, Node& node)
{
    if (!m_mounted)
        return false;

    node = { true, true, 0, 0, {} };

    while (*path) {
        while (*path == '/')
            path++;
        if (!*path)
            break;

        uint32_t length = 0;
        while (path[length] && path[length] != '/')
            length++;

        uint8_t shortName[11];
        if (!node.isDirectory || !toShortName(path, length, shortName))
            return false;

        FindArg find {};
        find.shortName = shortName;
        auto visit = [](const DirectoryEntry& entry, void* arg) {
            auto* find = static_cast<FindArg*>(arg);
            if (memcmp(entry.name, find->shortName, 11) != 0)
                return false;
            find->found = entry;
            find->isFound = true;
            return true;
        };
        if (!forEachEntry(node, visit, &find) || !find.isFound)
            return false;

        node.isDirectory = find.found.attributes & attributeDirectory;
        node.firstCluster = find.found.firstCluster;
        node.size = node.isDirectory ? 0 : find.found.size;
        memcpy(node.shortName, find.found.name, 11);
        // ".." of a directory in the root points at cluster 0: the root
        node.isRoot = node.isDirectory && !node.firstCluster;

        path += length;
    }
    return true;
}

bool Fat16::stat(const char* path, FileInfo& info)
{
    Node node;
    if (!lookup(path, node))
        return false;

    if (node.isRoot)
        strcpy(info.name, "/");
    else
        fromShortName(node.shortName, info.name);
    info.isDirectory = node.isDirectory;
    info.size = node.size;
    return true;
}

int64_t Fat16::read(const char* path, uint64_t offset, void* buffer, uint64_t size)
{
    Node node;
    if (!lookup(path, node) || node.isDirectory)
        return -1;

    if (offset >= node.size)
        return 0;
    if (size > node.size - offset)
        size = node.size - offset;

    uint16_t cluster = node.firstCluster;
    uint64_t clusterBytes = clusterSize();

    // Follow the chain to the cluster that holds offset
    for (uint64_t skip = offset / clusterBytes; skip; skip--) {
        if (nextCluster(cluster, cluster) != ChainStep::Next)
            return -1;
    }

    auto* out = static_cast<uint8_t*>(buffer);
    uint64_t position = offset % clusterBytes; // inside the current cluster
    uint64_t done = 0;

    while (done < size) {
        if (!isDataCluster(cluster))
            return -1;

        uint64_t sector = position / BlockDevice::sectorSize;
        uint64_t inSector = position % BlockDevice::sectorSize;
        if (!m_volume.readSectors(clusterLba(cluster) + sector, 1, m_sector))
            return -1;

        uint64_t chunk = BlockDevice::sectorSize - inSector;
        if (chunk > size - done)
            chunk = size - done;
        memcpy(out + done, m_sector + inSector, chunk);
        done += chunk;
        position += chunk;

        // The file size says more data follows: the chain must go on
        if (position == clusterBytes && done < size) {
            position = 0;
            if (nextCluster(cluster, cluster) != ChainStep::Next)
                return -1;
        }
    }
    return static_cast<int64_t>(done);
}

bool Fat16::listDirectory(const char* path, DirectoryCallback callback, void* arg)
{
    Node node;
    if (!lookup(path, node) || !node.isDirectory)
        return false;

    ListArg list { callback, arg };
    auto visit = [](const DirectoryEntry& entry, void* arg) {
        if (entry.name[0] == '.')
            return false; // "." and ".."

        FileInfo info;
        fromShortName(entry.name, info.name);
        info.isDirectory = entry.attributes & attributeDirectory;
        info.size = info.isDirectory ? 0 : entry.size;

        auto* list = static_cast<ListArg*>(arg);
        list->callback(info, list->arg);
        return false;
    };
    return forEachEntry(node, visit, &list);
}

} // namespace Fs

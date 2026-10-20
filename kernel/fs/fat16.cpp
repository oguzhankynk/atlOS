/**
 * @file fat16.cpp
 * @author Oguzhan KAYNAK
 * @brief Fs::Fat16: a FAT16 file system with 8.3 names.
 *
 * Writes go straight to the volume (no cache) in whatever order the code
 * needs them; a crash in the middle can leave lost clusters or an entry that
 * points at a cluster not yet linked. See README.md, "Write order".
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
static constexpr uint8_t attributeArchive = 0x20; // set on new files: "changed since the last backup"
static constexpr uint8_t attributeLongName = 0x0F; // read only + hidden + system + volume: an LFN part

static constexpr uint8_t entryEnd = 0x00; // first name byte: no entries after this one
static constexpr uint8_t entryDeleted = 0xE5;

static constexpr uint16_t clusterFree = 0x0000;
static constexpr uint16_t chainBad = 0xFFF7;
static constexpr uint16_t chainEnd = 0xFFF8; // 0xFFF8..0xFFFF
static constexpr uint16_t chainEndMark = 0xFFFF; // what the driver writes

// The FAT type is decided by the number of clusters, nothing else (Microsoft's FAT spec)
static constexpr uint32_t minFat16Clusters = 4085;
static constexpr uint32_t maxFat16Clusters = 65524;

static constexpr uint32_t entriesPerSector = BlockDevice::sectorSize / sizeof(Fat16::DirectoryEntry);
static constexpr uint32_t fatEntriesPerSector = BlockDevice::sectorSize / 2;

// No clock yet: every write is stamped 1980-01-01 00:00, the FAT epoch
// (date = (year - 1980) << 9 | month << 5 | day, time = hours << 11 | minutes << 5 | seconds / 2)
static constexpr uint16_t fixedDate = (0 << 9) | (1 << 5) | 1;
static constexpr uint16_t fixedTime = 0;

static char toUpper(char c)
{
    return (c >= 'a' && c <= 'z') ? static_cast<char>(c - 'a' + 'A') : c;
}

static bool isDotName(const uint8_t shortName[11])
{
    return shortName[0] == '.';
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
    m_fatCount = boot.fatCount;
    m_sectorsPerFat = boot.sectorsPerFat;
    m_firstFatSector = boot.reservedSectors;
    m_firstRootSector = m_firstFatSector + m_fatCount * m_sectorsPerFat;
    m_rootSectors = (boot.rootEntryCount * sizeof(DirectoryEntry) + BlockDevice::sectorSize - 1) / BlockDevice::sectorSize;
    m_firstDataSector = m_firstRootSector + m_rootSectors;
    if (m_firstDataSector >= totalSectors)
        return false;

    m_clusterCount = (totalSectors - m_firstDataSector) / spc;
    if (m_clusterCount < minFat16Clusters || m_clusterCount > maxFat16Clusters)
        return false;

    // The FAT must have an entry for every cluster (plus the two reserved ones)
    if ((uint64_t)m_sectorsPerFat * fatEntriesPerSector < m_clusterCount + 2)
        return false;

    m_fatSector = UINT32_MAX;
    m_nextFree = 2;
    m_mounted = true;
    return true;
}

uint64_t Fat16::clusterLba(uint16_t cluster) const
{
    // Data clusters are numbered from 2
    return m_firstDataSector + (uint64_t)(cluster - 2) * m_sectorsPerCluster;
}

// ------------------------------------------------------------
// The FAT
// ------------------------------------------------------------

bool Fat16::loadFatSector(uint32_t sector)
{
    if (sector == m_fatSector)
        return true;
    if (!m_volume.readSectors(m_firstFatSector + sector, 1, m_fat))
        return false;
    m_fatSector = sector;
    return true;
}

bool Fat16::fatValue(uint16_t cluster, uint16_t& value)
{
    // Two bytes per cluster: the FAT sector is cluster / 256
    if (!loadFatSector(cluster / fatEntriesPerSector))
        return false;
    value = m_fat[cluster % fatEntriesPerSector];
    return true;
}

bool Fat16::setFat(uint16_t cluster, uint16_t value)
{
    uint32_t sector = cluster / fatEntriesPerSector;
    if (!loadFatSector(sector))
        return false;
    m_fat[cluster % fatEntriesPerSector] = value;

    // Every copy of the FAT gets the change (the copies exist for recovery tools)
    for (uint32_t copy = 0; copy < m_fatCount; copy++) {
        if (!m_volume.writeSectors(m_firstFatSector + copy * m_sectorsPerFat + sector, 1, m_fat))
            return false;
    }
    return true;
}

Fat16::ChainStep Fat16::nextCluster(uint16_t cluster, uint16_t& next)
{
    uint16_t value;
    if (!fatValue(cluster, value))
        return ChainStep::Broken;
    if (value >= chainEnd)
        return ChainStep::End;
    if (value == chainBad || !isDataCluster(value))
        return ChainStep::Broken; // a free (0), reserved or bad cluster in a chain
    next = value;
    return ChainStep::Next;
}

uint32_t Fat16::freeClusterCount()
{
    uint32_t count = 0;
    for (uint32_t cluster = 2; cluster < m_clusterCount + 2; cluster++) {
        uint16_t value;
        if (fatValue(cluster, value) && value == clusterFree)
            count++;
    }
    return count;
}

uint16_t Fat16::allocateCluster(uint16_t previous)
{
    // First fit, starting after the last allocation: one pass over the FAT
    uint32_t cluster = m_nextFree;
    for (uint32_t i = 0; i < m_clusterCount; i++, cluster++) {
        if (!isDataCluster(cluster))
            cluster = 2;

        uint16_t value;
        if (!fatValue(cluster, value))
            return 0;
        if (value != clusterFree)
            continue;

        if (!setFat(cluster, chainEndMark))
            return 0;
        if (previous && !setFat(previous, cluster))
            return 0;
        m_nextFree = cluster + 1;
        return cluster;
    }
    return 0; // the volume is full
}

bool Fat16::zeroCluster(uint16_t cluster)
{
    memset(m_sector, 0, sizeof(m_sector));
    for (uint32_t s = 0; s < m_sectorsPerCluster; s++) {
        if (!m_volume.writeSectors(clusterLba(cluster) + s, 1, m_sector))
            return false;
    }
    return true;
}

bool Fat16::freeChain(uint16_t first)
{
    uint16_t cluster = first;
    for (uint32_t steps = 0; steps < m_clusterCount && isDataCluster(cluster); steps++) {
        uint16_t next;
        if (!fatValue(cluster, next) || !setFat(cluster, clusterFree))
            return false;
        if (cluster < m_nextFree)
            m_nextFree = cluster;
        cluster = next;
    }
    return true;
}

// ------------------------------------------------------------
// Names
// ------------------------------------------------------------

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

// ------------------------------------------------------------
// Directories
// ------------------------------------------------------------

bool Fat16::forEachSlot(const Node& directory, SlotVisitor visit, void* arg, bool& stopped, uint16_t& lastCluster)
{
    stopped = false;
    lastCluster = 0;

    // Not m_sector: a visitor (or a listDirectory callback) may use the volume
    auto visitSector = [&](uint64_t lba) {
        DirectoryEntry entries[entriesPerSector];
        if (!m_volume.readSectors(lba, 1, entries))
            return false;
        for (uint32_t i = 0; i < entriesPerSector && !stopped; i++)
            stopped = visit(entries[i], Slot { lba, i }, arg);
        return true;
    };

    // The root directory has a fixed place and size
    if (directory.isRoot) {
        for (uint32_t s = 0; s < m_rootSectors && !stopped; s++) {
            if (!visitSector(m_firstRootSector + s))
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
        lastCluster = cluster;

        for (uint32_t s = 0; s < m_sectorsPerCluster && !stopped; s++) {
            if (!visitSector(clusterLba(cluster) + s))
                return false;
        }
        if (stopped)
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
        uint64_t lba;
        uint32_t index;
    };

    struct ListArg {
        DirectoryCallback callback;
        void* arg;
    };

    struct FreeSlotArg {
        uint64_t lba;
        uint32_t index;
    };
} // namespace

bool Fat16::forEachEntry(const Node& directory, SlotVisitor visit, void* arg)
{
    // Only the entries in use: stop at the end mark, skip deleted entries,
    // long name parts and the volume label
    struct Filter {
        SlotVisitor visit;
        void* arg;
    };
    Filter filter { visit, arg };
    auto filtered = [](const DirectoryEntry& entry, const Slot& slot, void* arg) {
        auto* filter = static_cast<Filter*>(arg);
        if (entry.name[0] == entryEnd)
            return true;
        if (entry.name[0] == entryDeleted || entry.attributes == attributeLongName
            || (entry.attributes & attributeVolumeLabel))
            return false;
        return filter->visit(entry, slot, filter->arg);
    };

    bool stopped;
    uint16_t lastCluster;
    return forEachSlot(directory, filtered, &filter, stopped, lastCluster);
}

bool Fat16::lookup(const char* path, Node& node)
{
    if (!m_mounted)
        return false;

    node = {};
    node.isRoot = true;
    node.isDirectory = true;

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
        auto visit = [](const DirectoryEntry& entry, const Slot& slot, void* arg) {
            auto* find = static_cast<FindArg*>(arg);
            if (memcmp(entry.name, find->shortName, 11) != 0)
                return false;
            find->found = entry;
            find->isFound = true;
            find->lba = slot.lba;
            find->index = slot.index;
            return true;
        };
        // "." and ".." are not in the root directory: the root is its own parent
        if (node.isRoot && isDotName(shortName)) {
            path += length;
            continue;
        }
        if (!forEachEntry(node, visit, &find) || !find.isFound)
            return false;

        node.isDirectory = find.found.attributes & attributeDirectory;
        node.firstCluster = find.found.firstCluster;
        node.size = node.isDirectory ? 0 : find.found.size;
        memcpy(node.shortName, find.found.name, 11);
        node.slot = { find.lba, find.index };
        // ".." of a directory in the root points at cluster 0: the root
        node.isRoot = node.isDirectory && !node.firstCluster;

        path += length;
    }
    return true;
}

// The directory that would hold path's last component, and that component
bool Fat16::lookupParent(const char* path, Node& parent, uint8_t shortName[11])
{
    while (*path == '/')
        path++;

    size_t length = strlen(path);
    while (length && path[length - 1] == '/')
        length--;
    if (!length)
        return false; // the root has no parent

    size_t nameStart = length;
    while (nameStart && path[nameStart - 1] != '/')
        nameStart--;

    // "." and ".." are never created or removed by name
    if (!toShortName(path + nameStart, length - nameStart, shortName) || isDotName(shortName))
        return false;

    char parentPath[256];
    if (nameStart >= sizeof(parentPath))
        return false;
    memcpy(parentPath, path, nameStart);
    parentPath[nameStart] = '\0';

    return lookup(parentPath, parent) && parent.isDirectory;
}

bool Fat16::readEntry(const Slot& slot, DirectoryEntry& entry)
{
    DirectoryEntry entries[entriesPerSector];
    if (!m_volume.readSectors(slot.lba, 1, entries))
        return false;
    entry = entries[slot.index];
    return true;
}

bool Fat16::writeEntry(const Slot& slot, const DirectoryEntry& entry)
{
    // A sector holds 16 entries: read, change one, write back
    DirectoryEntry entries[entriesPerSector];
    if (!m_volume.readSectors(slot.lba, 1, entries))
        return false;
    entries[slot.index] = entry;
    return m_volume.writeSectors(slot.lba, 1, entries);
}

bool Fat16::findFreeSlot(const Node& directory, Slot& slot)
{
    // The first deleted entry or the end mark can take a new entry
    FreeSlotArg free {};
    auto visit = [](const DirectoryEntry& entry, const Slot& slot, void* arg) {
        if (entry.name[0] != entryEnd && entry.name[0] != entryDeleted)
            return false;
        *static_cast<FreeSlotArg*>(arg) = { slot.lba, slot.index };
        return true;
    };

    bool stopped;
    uint16_t lastCluster;
    if (!forEachSlot(directory, visit, &free, stopped, lastCluster))
        return false;
    if (stopped) {
        slot = { free.lba, free.index };
        return true;
    }

    // Full. The root directory can not grow; a subdirectory gets a cluster
    if (directory.isRoot)
        return false;
    uint16_t cluster = allocateCluster(lastCluster);
    if (!cluster || !zeroCluster(cluster))
        return false;
    slot = { clusterLba(cluster), 0 };
    return true;
}

bool Fat16::isEmptyDirectory(const Node& directory)
{
    bool hasEntries = false;
    auto visit = [](const DirectoryEntry& entry, const Slot&, void* arg) {
        if (isDotName(entry.name))
            return false;
        *static_cast<bool*>(arg) = true;
        return true;
    };
    return forEachEntry(directory, visit, &hasEntries) && !hasEntries;
}

bool Fat16::addEntry(const char* path, uint8_t attributes, uint16_t firstCluster, Node* parentOut)
{
    Node parent;
    uint8_t shortName[11];
    if (!lookupParent(path, parent, shortName))
        return false;

    // The name must be free
    Node existing;
    if (lookup(path, existing))
        return false;

    Slot slot;
    if (!findFreeSlot(parent, slot))
        return false;

    DirectoryEntry entry {};
    memcpy(entry.name, shortName, 11);
    entry.attributes = attributes;
    entry.createDate = fixedDate;
    entry.createTime = fixedTime;
    entry.accessDate = fixedDate;
    entry.writeDate = fixedDate;
    entry.writeTime = fixedTime;
    entry.firstCluster = firstCluster;
    if (!writeEntry(slot, entry))
        return false;

    if (parentOut)
        *parentOut = parent;
    return true;
}

// ------------------------------------------------------------
// The FileSystem interface
// ------------------------------------------------------------

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
    auto visit = [](const DirectoryEntry& entry, const Slot&, void* arg) {
        if (isDotName(entry.name))
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

bool Fat16::create(const char* path)
{
    // An empty file has no clusters: first cluster 0, size 0
    return m_mounted && addEntry(path, attributeArchive, 0);
}

int64_t Fat16::write(const char* path, uint64_t offset, const void* buffer, uint64_t size)
{
    Node node;
    if (!lookup(path, node) || node.isDirectory)
        return -1;

    // No holes: writing starts inside the file or right at its end
    if (offset > node.size || offset + size > UINT32_MAX)
        return -1;
    if (!size)
        return 0;

    uint64_t clusterBytes = clusterSize();
    uint64_t end = offset + size;
    uint64_t needed = (end + clusterBytes - 1) / clusterBytes;

    // Walk the chain to its last cluster, then add clusters until the data fits
    uint16_t first = node.firstCluster;
    uint16_t last = first;
    uint64_t have = first ? 1 : 0;
    while (have && have < needed) {
        uint16_t next;
        ChainStep step = nextCluster(last, next);
        if (step == ChainStep::Broken)
            return -1;
        if (step == ChainStep::End)
            break;
        last = next;
        have++;
    }

    bool full = false;
    while (have < needed) {
        uint16_t cluster = allocateCluster(last);
        if (!cluster) {
            full = true;
            break;
        }
        if (!first)
            first = cluster;
        last = cluster;
        have++;
    }

    // What fits: with a full volume, the clusters there are
    uint64_t room = have * clusterBytes;
    if (end > room)
        end = room;

    // Write sector by sector; a partly written sector is read first
    auto* in = static_cast<const uint8_t*>(buffer);
    uint16_t cluster = first;
    for (uint64_t skip = offset / clusterBytes; skip; skip--) {
        if (nextCluster(cluster, cluster) != ChainStep::Next)
            return -1;
    }

    uint64_t position = offset;
    while (position < end) {
        uint64_t inCluster = position % clusterBytes;
        uint64_t lba = clusterLba(cluster) + inCluster / BlockDevice::sectorSize;
        uint64_t inSector = position % BlockDevice::sectorSize;
        uint64_t chunk = BlockDevice::sectorSize - inSector;
        if (chunk > end - position)
            chunk = end - position;

        if (chunk != BlockDevice::sectorSize && !m_volume.readSectors(lba, 1, m_sector))
            return -1;
        memcpy(m_sector + inSector, in + (position - offset), chunk);
        if (!m_volume.writeSectors(lba, 1, m_sector))
            return -1;

        position += chunk;
        if (position < end && !(position % clusterBytes)) {
            if (nextCluster(cluster, cluster) != ChainStep::Next)
                return -1;
        }
    }

    // The directory entry: first cluster (a new file got one), size, time
    DirectoryEntry entry;
    if (!readEntry(node.slot, entry))
        return -1;
    entry.firstCluster = first;
    if (end > entry.size)
        entry.size = (uint32_t)end;
    entry.writeDate = fixedDate;
    entry.writeTime = fixedTime;
    entry.attributes |= attributeArchive;
    if (!writeEntry(node.slot, entry))
        return -1;

    if (full && end == offset)
        return -1; // nothing fit
    return (int64_t)(end - offset);
}

bool Fat16::makeDirectory(const char* path)
{
    if (!m_mounted)
        return false;

    // The name must be free before a cluster is spent on it
    Node existing;
    if (lookup(path, existing))
        return false;

    uint16_t cluster = allocateCluster(0);
    if (!cluster || !zeroCluster(cluster))
        return false;

    Node parent;
    if (!addEntry(path, attributeDirectory, cluster, &parent)) {
        freeChain(cluster);
        return false;
    }

    // Every subdirectory starts with "." (itself) and ".." (its parent; 0 = the root)
    DirectoryEntry dot {};
    memcpy(dot.name, ".          ", 11);
    dot.attributes = attributeDirectory;
    dot.writeDate = dot.createDate = dot.accessDate = fixedDate;
    dot.firstCluster = cluster;
    DirectoryEntry dotDot = dot;
    memcpy(dotDot.name, "..         ", 11);
    dotDot.firstCluster = parent.isRoot ? 0 : parent.firstCluster;

    return writeEntry({ clusterLba(cluster), 0 }, dot) && writeEntry({ clusterLba(cluster), 1 }, dotDot);
}

bool Fat16::remove(const char* path)
{
    Node node;
    if (!lookup(path, node) || node.isRoot || node.isDirectory)
        return false;

    DirectoryEntry entry;
    if (!readEntry(node.slot, entry))
        return false;
    entry.name[0] = entryDeleted;
    return writeEntry(node.slot, entry) && freeChain(node.firstCluster);
}

bool Fat16::removeDirectory(const char* path)
{
    Node node;
    uint8_t shortName[11];
    Node parent;
    // lookupParent refuses "." and "..": they name another directory's entry
    if (!lookupParent(path, parent, shortName) || !lookup(path, node) || node.isRoot || !node.isDirectory)
        return false;
    if (!isEmptyDirectory(node))
        return false;

    DirectoryEntry entry;
    if (!readEntry(node.slot, entry))
        return false;
    entry.name[0] = entryDeleted;
    return writeEntry(node.slot, entry) && freeChain(node.firstCluster);
}

} // namespace Fs

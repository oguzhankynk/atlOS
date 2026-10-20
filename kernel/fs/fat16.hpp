/**
 * @file fat16.hpp
 * @author Oguzhan KAYNAK
 * @brief Fs::Fat16: a FAT16 file system with 8.3 names.
 *
 * Layout of a FAT16 volume (sizes from the boot sector):
 *
 *     | boot sector + reserved | FAT 1 | FAT 2 | root directory | data: cluster 2, 3, ... |
 *
 * A file's directory entry holds its first cluster; FAT[cluster] holds the
 * next one, until an end of chain mark (0xFFF8..0xFFFF). See README.md.
 */

#pragma once

#include <stdint.h>

#include <drivers/disk/block_device.hpp>
#include <fs/file_system.hpp>

namespace Fs {

class Fat16 final : public FileSystem {
public:
    /// The volume (usually a partition) stays owned by the caller.
    explicit Fat16(Drivers::BlockDevice& volume);

    Fat16(const Fat16&) = delete;
    Fat16& operator=(const Fat16&) = delete;

    /// Read and check the boot sector. false if the volume is not FAT16.
    bool mount();

    bool isMounted() const { return m_mounted; }

    bool stat(const char* path, FileInfo& info) override;
    int64_t read(const char* path, uint64_t offset, void* buffer, uint64_t size) override;
    bool listDirectory(const char* path, DirectoryCallback callback, void* arg) override;
    bool create(const char* path) override;
    int64_t write(const char* path, uint64_t offset, const void* buffer, uint64_t size) override;
    bool makeDirectory(const char* path) override;
    bool remove(const char* path) override;
    bool removeDirectory(const char* path) override;
    const char* typeName() const override { return "fat16"; }

    uint32_t clusterCount() const { return m_clusterCount; }
    uint32_t clusterSize() const { return m_sectorsPerCluster * Drivers::BlockDevice::sectorSize; }

    /// Count the free clusters (scans the whole FAT).
    uint32_t freeClusterCount();

    /// A 32 byte directory entry as it is on disk.
    struct DirectoryEntry {
        uint8_t name[11]; // 8 + 3 characters, space padded, no dot
        uint8_t attributes;
        uint8_t reserved;
        uint8_t createTimeTenths;
        uint16_t createTime;
        uint16_t createDate;
        uint16_t accessDate;
        uint16_t firstClusterHigh; // FAT32 only
        uint16_t writeTime;
        uint16_t writeDate;
        uint16_t firstCluster;
        uint32_t size;
    } __attribute__((packed));

    /// Turn a path component into the 11 byte on disk form ("readme.txt" -> "README  TXT").
    static bool toShortName(const char* component, uint32_t length, uint8_t shortName[11]);

    /// Turn an on disk name into "NAME.EXT".
    static void fromShortName(const uint8_t shortName[11], char name[maxNameLength + 1]);

private:
    // Where a directory entry is on the volume
    struct Slot {
        uint64_t lba;
        uint32_t index; // entry in that sector
    };

    // What a path resolves to
    struct Node {
        bool isRoot;
        bool isDirectory;
        uint16_t firstCluster;
        uint32_t size;
        uint8_t shortName[11];
        Slot slot; // its directory entry (not for the root)
    };

    enum class ChainStep {
        Next,
        End,
        Broken,
    };

    // Return true from a visitor to stop the walk
    using SlotVisitor = bool (*)(const DirectoryEntry& entry, const Slot& slot, void* arg);

    bool lookup(const char* path, Node& node);
    bool lookupParent(const char* path, Node& parent, uint8_t shortName[11]);
    bool forEachSlot(const Node& directory, SlotVisitor visit, void* arg, bool& stopped, uint16_t& lastCluster);
    bool forEachEntry(const Node& directory, SlotVisitor visit, void* arg);
    bool isEmptyDirectory(const Node& directory);

    bool readEntry(const Slot& slot, DirectoryEntry& entry);
    bool writeEntry(const Slot& slot, const DirectoryEntry& entry);
    bool findFreeSlot(const Node& directory, Slot& slot);
    bool addEntry(const char* path, uint8_t attributes, uint16_t firstCluster, Node* parentOut = nullptr);

    bool loadFatSector(uint32_t sector);
    ChainStep nextCluster(uint16_t cluster, uint16_t& next);
    bool fatValue(uint16_t cluster, uint16_t& value);
    bool setFat(uint16_t cluster, uint16_t value);
    uint16_t allocateCluster(uint16_t previous);
    bool zeroCluster(uint16_t cluster);
    bool freeChain(uint16_t first);

    bool isDataCluster(uint32_t cluster) const { return cluster >= 2 && cluster < m_clusterCount + 2; }
    uint64_t clusterLba(uint16_t cluster) const;

    Drivers::BlockDevice& m_volume;
    bool m_mounted {};

    uint32_t m_sectorsPerCluster {};
    uint32_t m_fatCount {};
    uint32_t m_sectorsPerFat {};
    uint32_t m_firstFatSector {};
    uint32_t m_firstRootSector {};
    uint32_t m_rootSectors {};
    uint32_t m_firstDataSector {};
    uint32_t m_clusterCount {};
    uint16_t m_nextFree { 2 }; // where the search for a free cluster starts

    // One sector of the FAT, the one used last (written through to every copy)
    uint16_t m_fat[Drivers::BlockDevice::sectorSize / 2] {};
    uint32_t m_fatSector { UINT32_MAX };
    uint8_t m_sector[Drivers::BlockDevice::sectorSize] {}; // scratch for data sectors
};

} // namespace Fs

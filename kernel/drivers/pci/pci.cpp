/**
 * @file pci.cpp
 * @author Oguzhan KAYNAK
 * @brief The PCI scan and the name tables.
 */

#include <drivers/pci/pci.hpp>

namespace Drivers {

static bool readDevice(PciConfigSpace& config, PciAddress address, PciDevice& device)
{
    device.vendorId = config.read16(address, PciVendorId);
    if (device.vendorId == pciNoVendor)
        return false;
    device.address = address;
    device.deviceId = config.read16(address, PciDeviceId);
    device.classCode = config.read8(address, PciClass);
    device.subclass = config.read8(address, PciSubclass);
    device.progIf = config.read8(address, PciProgIf);
    device.revision = config.read8(address, PciRevision);
    device.headerType = config.read8(address, PciHeaderType) & ~pciMultiFunction;
    return true;
}

uint32_t scanPci(PciConfigSpace& config, PciDevice* devices, uint32_t capacity)
{
    // 256 x 32 x 8 tries: slower than following the bridges, but it can not miss a bus
    uint32_t count = 0;
    for (uint32_t bus = 0; bus < 256; bus++) {
        for (uint8_t device = 0; device < 32; device++) {
            PciAddress first { (uint8_t)bus, device, 0 };
            if (config.read16(first, PciVendorId) == pciNoVendor)
                continue;

            uint8_t functions = (config.read8(first, PciHeaderType) & pciMultiFunction) ? 8 : 1;
            for (uint8_t function = 0; function < functions; function++) {
                if (count == capacity)
                    return count;
                if (readDevice(config, { (uint8_t)bus, device, function }, devices[count]))
                    count++;
            }
        }
    }
    return count;
}

// ------------------------------------------------------------
// Names
// ------------------------------------------------------------

struct ClassName {
    uint8_t classCode;
    uint8_t subclass;
    const char* name;
};

// The common subclasses; the others get the name of their class
static constexpr ClassName subclassNames[] = {
    { 0x01, 0x00, "SCSI controller" },
    { 0x01, 0x01, "IDE controller" },
    { 0x01, 0x05, "ATA controller" },
    { 0x01, 0x06, "SATA controller" },
    { 0x01, 0x08, "NVMe controller" },
    { 0x02, 0x00, "Ethernet controller" },
    { 0x03, 0x00, "VGA controller" },
    { 0x03, 0x02, "3D controller" },
    { 0x04, 0x01, "Audio device" },
    { 0x04, 0x03, "Audio device" },
    { 0x06, 0x00, "Host bridge" },
    { 0x06, 0x01, "ISA bridge" },
    { 0x06, 0x04, "PCI bridge" },
    { 0x07, 0x00, "Serial controller" },
    { 0x0C, 0x03, "USB controller" },
    { 0x0C, 0x05, "SMBus controller" },
};

static constexpr const char* classNames[] = {
    "Unclassified device", // 0x00
    "Storage controller", // 0x01
    "Network controller", // 0x02
    "Display controller", // 0x03
    "Multimedia device", // 0x04
    "Memory controller", // 0x05
    "Bridge", // 0x06
    "Communication controller", // 0x07
    "System peripheral", // 0x08
    "Input device", // 0x09
    "Docking station", // 0x0A
    "Processor", // 0x0B
    "Serial bus controller", // 0x0C
    "Wireless controller", // 0x0D
    "Intelligent controller", // 0x0E
    "Satellite controller", // 0x0F
    "Encryption controller", // 0x10
    "Signal processing controller", // 0x11
    "Processing accelerator", // 0x12
};

const char* pciClassName(uint8_t classCode, uint8_t subclass)
{
    for (const ClassName& entry : subclassNames) {
        if (entry.classCode == classCode && entry.subclass == subclass)
            return entry.name;
    }
    if (classCode < sizeof(classNames) / sizeof(classNames[0]))
        return classNames[classCode];
    return "Unknown device";
}

struct VendorName {
    uint16_t id;
    const char* name;
};

static constexpr VendorName vendorNames[] = {
    { 0x8086, "Intel" },
    { 0x1022, "AMD" },
    { 0x1002, "AMD" }, // the former ATI
    { 0x10DE, "NVIDIA" },
    { 0x10EC, "Realtek" },
    { 0x14E4, "Broadcom" },
    { 0x144D, "Samsung" },
    { 0x1B4B, "Marvell" },
    { 0x168C, "Qualcomm Atheros" },
    { 0x1106, "VIA" },
    { 0x15AD, "VMware" },
    { 0x80EE, "VirtualBox" },
    { 0x1AF4, "Red Hat (virtio)" },
    { 0x1B36, "Red Hat" },
    { 0x1234, "Bochs" },
};

const char* pciVendorName(uint16_t vendorId)
{
    for (const VendorName& entry : vendorNames) {
        if (entry.id == vendorId)
            return entry.name;
    }
    return nullptr;
}

const char* pciCapabilityName(uint8_t id)
{
    switch (id) {
    case 0x01:
        return "Power Management";
    case 0x05:
        return "MSI";
    case 0x09:
        return "Vendor Specific";
    case 0x0D:
        return "Bridge Subsystem";
    case pciCapabilityExpress:
        return "PCI Express";
    case 0x11:
        return "MSI-X";
    case 0x12:
        return "SATA";
    case 0x13:
        return "Advanced Features";
    default:
        return nullptr;
    }
}

} // namespace Drivers

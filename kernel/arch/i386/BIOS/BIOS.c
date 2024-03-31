#include <stdio.h>
#include <string.h>

#include <kernel/public/BIOS/BIOS.h>
#include <kernel/private/BIOS/BIOS_P.hpp>

void BIOSInit()
{
    BIOS bios;
    //bios.printEntryPoint();
    //bios.printBIOSInformation();
    bios.printSystemInformation();
}



/**
 * @file probe.hpp
 * @author Oguzhan KAYNAK
 * @brief Reading memory that may not be there: a fault (page fault, #GP for
 * a non canonical address) returns false instead of panicking, through the
 * exception table (Fault::fixup), like readMsr.
 */

#pragma once

#include <stdint.h>

/// @brief Read the 8 bytes at address into value. false if the read faults.
extern "C" bool readQword(const void* address, uint64_t* value);

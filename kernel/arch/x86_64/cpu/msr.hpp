/**
 * @file msr.hpp
 * @author Oguzhan KAYNAK
 * @brief Model specific registers (MSRs), read and written without the risk
 * of a #GP: an MSR the CPU does not have makes rdmsr/wrmsr fault, and the
 * exception table (Fault::fixup) turns that fault into a "false" here.
 */

#pragma once

#include <stdint.h>

/// EFER: SCE (syscall), LME/LMA (long mode enabled/active), NXE (no-execute)
static constexpr uint32_t msrEfer = 0xC0000080;

/// @brief Read MSR msr into value. false if the CPU does not have it.
extern "C" bool readMsr(uint32_t msr, uint64_t* value);

/// @brief Write value to MSR msr. false if the CPU refuses (no such MSR, a reserved bit set).
extern "C" bool writeMsr(uint32_t msr, uint64_t value);

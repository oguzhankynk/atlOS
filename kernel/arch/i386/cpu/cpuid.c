/**
 * @file cpuid.c
 * @author okaynak ()
 * @brief This file contains functions for interacting with the CPUID instruction to retrieve processor information.
 * @date 2024-03-31
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#include <kernel/public/cpuid.h>

void cpuid(unsigned int value, unsigned int *eax, unsigned int *ebx, unsigned int *ecx, unsigned int *edx)
{
  unsigned int eaxres, ebxres, ecxres, edxres;

  __asm__ volatile("cpuid" \
			: "=b"(ebxres), \
			"=c"(ecxres), \
			"=d"(edxres), \
      "=a"(eaxres) \
			: "a"(value));
      
  *eax = eaxres;
  *ebx = ebxres;
  *ecx = ecxres;
  *edx = edxres;    
}
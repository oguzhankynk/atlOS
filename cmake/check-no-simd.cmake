# Fails if the kernel image uses MMX/SSE/AVX registers.
# Usage: cmake -DOBJDUMP=<objdump> -DIMAGE=<atlOS.kernel> -P check-no-simd.cmake

execute_process(
    COMMAND ${OBJDUMP} -d --no-show-raw-insn ${IMAGE}
    OUTPUT_VARIABLE disassembly
    RESULT_VARIABLE result
)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "objdump failed on ${IMAGE}")
endif()

string(REGEX MATCHALL "[^\n]*%(x|y|z)mm[0-9]+[^\n]*|[^\n]*%mm[0-7][^\n]*" hits "${disassembly}")
list(LENGTH hits count)

if(count GREATER 0)
    list(SUBLIST hits 0 5 first)
    string(REPLACE ";" "\n" first "${first}")
    message(FATAL_ERROR
        "${IMAGE} contains ${count} SIMD instructions, but SIMD is not enabled "
        "(see ATLOS_NO_SIMD_FLAGS in CMakeLists.txt). First ones:\n${first}")
endif()

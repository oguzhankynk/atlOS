#pragma once

#include <core/panic.hpp>
#define assert(condition) kernelPanicAssert(condition)

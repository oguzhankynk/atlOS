/**
 * @file stdint.h
 * @author okaynak ()
 * @brief 
 * @date 2024-03-31
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#ifndef BA22560F_9F55_4018_A630_CB062C833243
#define BA22560F_9F55_4018_A630_CB062C833243

/**
 * @brief Signed 8-bit integer type.
 */
typedef signed char int8_t;

/**
 * @brief Signed 16-bit integer type.
 */
typedef short int int16_t;

/**
 * @brief Signed 32-bit integer type.
 */
typedef int int32_t;

/**
 * @brief Signed 64-bit integer type.
 */
typedef long long int int64_t;

/**
 * @brief Unsigned 8-bit integer type.
 */
typedef unsigned char uint8_t;

/**
 * @brief Unsigned 16-bit integer type.
 */
typedef unsigned short int uint16_t;

/**
 * @brief Unsigned 32-bit integer type.
 */
typedef unsigned int uint32_t;

/**
 * @brief Unsigned 64-bit integer type.
 */
typedef unsigned long long int uint64_t;

/**
 * @brief Minimum value of int8_t.
 */
#define INT8_MIN    (-128)

/**
 * @brief Minimum value of int16_t.
 */
#define INT16_MIN   (-32768)

/**
 * @brief Minimum value of int32_t.
 */
#define INT32_MIN   (-2147483648)

/**
 * @brief Minimum value of int64_t.
 */
#define INT64_MIN   (-9223372036854775807LL - 1)

/**
 * @brief Minimum value of uint8_t.
 */
#define UINT8_MIN   (0)

/**
 * @brief Minimum value of uint16_t.
 */
#define UINT16_MIN  (0)

/**
 * @brief Minimum value of uint32_t.
 */
#define UINT32_MIN  (0)

/**
 * @brief Minimum value of uint64_t.
 */
#define UINT64_MIN  (0)

/**
 * @brief Maximum value of int8_t.
 */
#define INT8_MAX    (127)

/**
 * @brief Maximum value of int16_t.
 */
#define INT16_MAX   (32767)

/**
 * @brief Maximum value of int32_t.
 */
#define INT32_MAX   (2147483647)

/**
 * @brief Maximum value of int64_t.
 */
#define INT64_MAX   (9223372036854775807LL)

/**
 * @brief Maximum value of uint8_t.
 */
#define UINT8_MAX   (255)

/**
 * @brief Maximum value of uint16_t.
 */
#define UINT16_MAX  (65535)

/**
 * @brief Maximum value of uint32_t.
 */
#define UINT32_MAX  (4294967295U)

/**
 * @brief Maximum value of uint64_t.
 */
#define UINT64_MAX  (18446744073709551615ULL)

/**
 * @brief Greatest-width signed integer type.
 */
typedef int64_t intmax_t;

/**
 * @brief Greatest-width unsigned integer type.
 */
typedef uint64_t uintmax_t;

/**
 * @brief Integer type capable of holding a pointer.
 */
typedef uint64_t uintptr_t;


#endif /* BA22560F_9F55_4018_A630_CB062C833243 */

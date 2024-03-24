#ifndef BA22560F_9F55_4018_A630_CB062C833243
#define BA22560F_9F55_4018_A630_CB062C833243

/* Signed integer types */
typedef signed char int8_t;
typedef short int int16_t;
typedef int int32_t;
typedef long long int int64_t;

/* Unsigned integer types */
typedef unsigned char uint8_t;
typedef unsigned short int uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long int uint64_t;

/* Minimum value macros */
#define INT8_MIN    (-128)
#define INT16_MIN   (-32768)
#define INT32_MIN   (-2147483648)
#define INT64_MIN   (-9223372036854775807LL - 1)

#define UINT8_MIN   (0)
#define UINT16_MIN  (0)
#define UINT32_MIN  (0)
#define UINT64_MIN  (0)

/* Maximum value macros */
#define INT8_MAX    (127)
#define INT16_MAX   (32767)
#define INT32_MAX   (2147483647)
#define INT64_MAX   (9223372036854775807LL)

#define UINT8_MAX   (255)
#define UINT16_MAX  (65535)
#define UINT32_MAX  (4294967295U)
#define UINT64_MAX  (18446744073709551615ULL)

/* Greatest-width integer types */
typedef int64_t intmax_t;
typedef uint64_t uintmax_t;

/* Pointer-width integer type */
typedef uint64_t uintptr_t;


#endif /* BA22560F_9F55_4018_A630_CB062C833243 */

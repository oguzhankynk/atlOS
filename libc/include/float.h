/**
 * @file float.h
 * @author Oguzhan KAYNAK
 * @brief Header file defining limits and characteristics of floating-point types
 *
 * Copyright © 2024 Oguzhan KAYNAK
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software
 * without  restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef _FLOAT_H
#define _FLOAT_H

#define FLT_RADIX 2 // Radix of exponent representation
#define FLT_ROUNDS 1 // Rounding mode
#define FLT_DIG 6 // Decimal digits of precision
#define FLT_EPSILON 1.19209290e-07F // Smallest such that 1.0+FLT_EPSILON != 1.0
#define FLT_MANT_DIG 24 // Number of bits in mantissa
#define FLT_MAX 3.40282347e+38F // Maximum representable finite float
#define FLT_MAX_10_EXP 38 // Maximum decimal exponent
#define FLT_MAX_EXP 128 // Maximum binary exponent
#define FLT_MIN 1.17549435e-38F // Minimum positive normalized float
#define FLT_MIN_10_EXP (-37) // Minimum decimal exponent
#define FLT_MIN_EXP (-125) // Minimum binary exponent

#define DBL_DIG 15 // Decimal digits of precision
#define DBL_EPSILON 2.2204460492503131e-16 // Smallest such that 1.0+DBL_EPSILON != 1.0
#define DBL_MANT_DIG 53 // Number of bits in mantissa
#define DBL_MAX 1.7976931348623157e+308 // Maximum representable finite double
#define DBL_MAX_10_EXP 308 // Maximum decimal exponent
#define DBL_MAX_EXP 1024 // Maximum binary exponent
#define DBL_MIN 2.2250738585072014e-308 // Minimum positive normalized double
#define DBL_MIN_10_EXP (-307) // Minimum decimal exponent
#define DBL_MIN_EXP (-1021) // Minimum binary exponent

#endif /* _FLOAT_H */

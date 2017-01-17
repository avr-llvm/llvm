/* Copyright (c) 2002,2007-2009 Michael Stumpf

   Portions of documentation Copyright (c) 1990 - 1994
   The Regents of the University of California.

   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:

   * Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.

   * Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in
     the documentation and/or other materials provided with the
     distribution.

   * Neither the name of the copyright holders nor the names of
     contributors may be used to endorse or promote products derived
     from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE. */

/* $Id$ */

/*
   math.h - mathematical functions

   Author : Michael Stumpf
            Michael.Stumpf@t-online.de

   __ATTR_CONST__ added by marekm@linux.org.pl for functions
   that "do not examine any values except their arguments, and have
   no effects except the return value", for better optimization by gcc.
 */

#ifndef __MATH_H
#define __MATH_H

/** \file */
/** \defgroup avr_math <math.h>: Mathematics
    \code #include <math.h> \endcode

    This header file declares basic mathematics constants and
    functions.

    \par Notes:
    - In order to access the functions declared herein, it is usually
      also required to additionally link against the library \c libm.a.
      See also the related \ref faq_libm "FAQ entry".
    - Math functions do not raise exceptions and do not change the
      \c errno variable. Therefore the majority of them are declared
      with const attribute, for better optimization by GCC.	*/


/** \ingroup avr_math	*/
/*@{*/

/** The constant \a e.	*/
#define M_E		2.7182818284590452354

/** The logarithm of the \a e to base 2. */
#define M_LOG2E		1.4426950408889634074	/* log_2 e */

/** The logarithm of the \a e to base 10. */
#define M_LOG10E	0.43429448190325182765	/* log_10 e */

/** The natural logarithm of the 2.	*/
#define M_LN2		0.69314718055994530942	/* log_e 2 */

/** The natural logarithm of the 10.	*/
#define M_LN10		2.30258509299404568402	/* log_e 10 */

/** The constant \a pi.	*/
#define M_PI		3.14159265358979323846	/* pi */

/** The constant \a pi/2.	*/
#define M_PI_2		1.57079632679489661923	/* pi/2 */

/** The constant \a pi/4.	*/
#define M_PI_4		0.78539816339744830962	/* pi/4 */

/** The constant \a 1/pi.	*/
#define M_1_PI		0.31830988618379067154	/* 1/pi */

/** The constant \a 2/pi.	*/
#define M_2_PI		0.63661977236758134308	/* 2/pi */

/** The constant \a 2/sqrt(pi).	*/
#define M_2_SQRTPI	1.12837916709551257390	/* 2/sqrt(pi) */

/** The square root of 2.	*/
#define M_SQRT2		1.41421356237309504880	/* sqrt(2) */

/** The constant \a 1/sqrt(2).	*/
#define M_SQRT1_2	0.70710678118654752440	/* 1/sqrt(2) */

/** NAN constant.	*/
#define NAN	__builtin_nan("")

/** INFINITY constant.	*/
#define INFINITY	__builtin_inf()


#ifndef __ATTR_CONST__
# define __ATTR_CONST__ __attribute__((__const__))
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
    The cos() function returns the cosine of \a __x, measured in radians.
 */
extern double cos(double __x) __ATTR_CONST__;
#define cosf	cos		/**< The alias for cos().	*/

/**
    The sin() function returns the sine of \a __x, measured in radians.
 */
extern double sin(double __x) __ATTR_CONST__;
#define sinf	sin		/**< The alias for sin().	*/

/**
    The tan() function returns the tangent of \a __x, measured in radians.
 */
extern double tan(double __x) __ATTR_CONST__;
#define tanf	tan		/**< The alias for tan().	*/

/**
    The fabs() function computes the absolute value of a floating-point
    number \a __x.
 */
extern double fabs(double __x) __ATTR_CONST__;
#define fabsf	fabs		/**< The alias for fabs().	*/

/**
    The function fmod() returns the floating-point remainder of <em>__x /
    __y</em>.
 */
extern double fmod(double __x, double __y) __ATTR_CONST__;
#define fmodf	fmod		/**< The alias for fmod().	*/

/**
    The modf() function breaks the argument \a __x into integral and
    fractional parts, each of which has the same sign as the argument. 
    It stores the integral part as a double in the object pointed to by
    \a __iptr.

    The modf() function returns the signed fractional part of \a __x.

    \note This implementation skips writing by zero pointer.  However,
    the GCC 4.3 can replace this function with inline code that does not
    permit to use NULL address for the avoiding of storing.
 */
extern double modf(double __x, double *__iptr);

/** The alias for modf().
 */
extern float modff (float __x, float *__iptr);

/**
    The sqrt() function returns the non-negative square root of \a __x.
 */
extern double sqrt(double __x) __ATTR_CONST__;
#define sqrtf	sqrt		/**< The alias for sqrt().	*/

/**
    The cbrt() function returns the cube root of \a __x.
 */
extern double cbrt(double __x) __ATTR_CONST__;
#define cbrtf	cbrt		/**< The alias for cbrt().	*/

/**
    The hypot() function returns <em>sqrt(__x*__x + __y*__y)</em>. This
    is the length of the hypotenuse of a right triangle with sides of
    length \a __x and \a __y, or the  distance of the point (\a __x, \a
    __y) from the origin. Using this function  instead of the direct
    formula is wise, since the error is much smaller. No underflow with
    small \a __x and \a __y. No overflow if result is in range.
 */
extern double hypot (double __x, double __y) __ATTR_CONST__;
#define hypotf	hypot		/**< The alias for hypot().	*/

/**
    The function square() returns <em>__x * __x</em>.

    \note This function does not belong to the C standard definition.
 */
extern double square(double __x) __ATTR_CONST__;
#define squaref	square		/**< The alias for square().	*/

/**
    The floor() function returns the largest integral value less than or
    equal to \a __x, expressed as a floating-point number.
 */
extern double floor(double __x) __ATTR_CONST__;
#define floorf	floor		/**< The alias for floor().	*/

/**
    The ceil() function returns the smallest integral value greater than
    or equal to \a __x, expressed as a floating-point number.
 */
extern double ceil(double __x) __ATTR_CONST__;
#define ceilf	ceil		/**< The alias for ceil().	*/

/**
    The frexp() function breaks a floating-point number into a normalized
    fraction and an integral power of 2.  It stores the integer in the \c
    int object pointed to by \a __pexp.

    If \a __x is a normal float point number, the frexp() function
    returns the value \c v, such that \c v has a magnitude in the
    interval [1/2, 1) or zero, and \a __x equals \c v times 2 raised to
    the power \a __pexp. If \a __x is zero, both parts of the result are
    zero. If \a __x is not a finite number, the frexp() returns \a __x as
    is and stores 0 by \a __pexp.

    \note  This implementation permits a zero pointer as a directive to
    skip a storing the exponent.
 */
extern double frexp(double __x, int *__pexp);
#define frexpf	frexp		/**< The alias for frexp().	*/

/**
    The ldexp() function multiplies a floating-point number by an integral
    power of 2. It returns the value of \a __x times 2 raised to the power
    \a __exp.
 */
extern double ldexp(double __x, int __exp) __ATTR_CONST__;
#define ldexpf	ldexp		/**< The alias for ldexp().	*/

/**
    The exp() function returns the exponential value of \a __x.
 */
extern double exp(double __x) __ATTR_CONST__;
#define expf	exp		/**< The alias for exp().	*/

/**
    The cosh() function returns the hyperbolic cosine of \a __x.
 */
extern double cosh(double __x) __ATTR_CONST__;
#define coshf	cosh		/**< The alias for cosh().	*/

/**
    The sinh() function returns the hyperbolic sine of \a __x.
 */
extern double sinh(double __x) __ATTR_CONST__;
#define sinhf	sinh		/**< The alias for sinh().	*/

/**
    The tanh() function returns the hyperbolic tangent of \a __x.
 */
extern double tanh(double __x) __ATTR_CONST__;
#define tanhf	tanh		/**< The alias for tanh().	*/

/**
    The acos() function computes the principal value of the arc cosine of
    \a __x.  The returned value is in the range [0, pi] radians. A domain
    error occurs for arguments not in the range [-1, +1].
 */
extern double acos(double __x) __ATTR_CONST__;
#define acosf	acos		/**< The alias for acos().	*/

/**
    The asin() function computes the principal value of the arc sine of
    \a __x.  The returned value is in the range [-pi/2, pi/2] radians. A
    domain error occurs for arguments not in the range [-1, +1].
 */
extern double asin(double __x) __ATTR_CONST__;
#define asinf	asin		/**< The alias for asin().	*/

/**
    The atan() function computes the principal value of the arc tangent
    of \a __x.  The returned value is in the range [-pi/2, pi/2] radians.
 */
extern double atan(double __x) __ATTR_CONST__;
#define atanf	atan		/**< The alias for atan().	*/

/**
    The atan2() function computes the principal value of the arc tangent
    of <em>__y / __x</em>, using the signs of both arguments to determine
    the quadrant of the return value.  The returned value is in the range
    [-pi, +pi] radians.
 */
extern double atan2(double __y, double __x) __ATTR_CONST__;
#define atan2f	atan2		/**< The alias for atan2().	*/

/**
    The log() function returns the natural logarithm of argument \a __x.
 */
extern double log(double __x) __ATTR_CONST__;
#define logf	log		/**< The alias for log().	*/

/**
    The log10() function returns the logarithm of argument \a __x to base 10.
 */
extern double log10(double __x) __ATTR_CONST__;
#define log10f	log10		/**< The alias for log10().	*/

/**
    The function pow() returns the value of \a __x to the exponent \a __y.
 */
extern double pow(double __x, double __y) __ATTR_CONST__;
#define powf	pow		/**< The alias for pow().	*/

/**
    The function isnan() returns 1 if the argument \a __x represents a
    "not-a-number" (NaN) object, otherwise 0.
 */
extern int isnan(double __x) __ATTR_CONST__;
#define	isnanf	isnan		/**< The alias for isnan().	*/

/**
    The function isinf() returns 1 if the argument \a __x is positive
    infinity, -1 if \a __x is negative infinity, and 0 otherwise.

    \note The GCC 4.3 can replace this function with inline code that
    returns the 1 value for both infinities (gcc bug #35509).
 */
extern int isinf(double __x) __ATTR_CONST__;
#define isinff	isinf		/**< The alias for isinf().	*/

/**
    The isfinite() function returns a nonzero value if \a __x is finite:
    not plus or minus infinity, and not NaN.
 */
__ATTR_CONST__ static inline int isfinite (double __x)
{
    unsigned char __exp;
    __asm__ (
	"mov	%0, %C1		\n\t"
	"lsl	%0		\n\t"
	"mov	%0, %D1		\n\t"
	"rol	%0		"
	: "=r" (__exp)
	: "r" (__x)	);
    return __exp != 0xff;
}
#define isfinitef isfinite	/**< The alias for isfinite().	*/

/**
    The copysign() function returns \a __x but with the sign of \a __y.
    They work even if \a __x or \a __y are NaN or zero.
*/
__ATTR_CONST__ static inline double copysign (double __x, double __y)
{
    __asm__ (
	"bst	%D2, 7	\n\t"
	"bld	%D0, 7	"
	: "=r" (__x)
	: "0" (__x), "r" (__y) );
    return __x;
}
#define copysignf copysign	/**< The alias for copysign().	*/

/**
    The signbit() function returns a nonzero value if the value of \a __x
    has its sign bit set.  This is not the same as `\a __x < 0.0',
    because IEEE 754 floating point allows zero to be signed. The
    comparison `-0.0 < 0.0' is false, but `signbit (-0.0)' will return a
    nonzero value.
 */
extern int signbit (double __x) __ATTR_CONST__;
#define signbitf signbit	/**< The alias for signbit().	*/

/**
    The fdim() function returns <em>max(__x - __y, 0)</em>. If \a __x or
    \a __y or both are NaN, NaN is returned.
 */
extern double fdim (double __x, double __y) __ATTR_CONST__;
#define fdimf	fdim		/**< The alias for fdim().	*/

/**
    The fma() function performs floating-point multiply-add. This is the
    operation <em>(__x * __y) + __z</em>, but the intermediate result is
    not rounded to the destination type.  This can sometimes improve the
    precision of a calculation.
 */
extern double fma (double __x, double __y, double __z) __ATTR_CONST__;
#define fmaf	fma		/**< The alias for fma().	*/

/**
    The fmax() function returns the greater of the two values \a __x and
    \a __y. If an argument is NaN, the other argument is returned. If
    both arguments are NaN, NaN is returned.
 */
extern double fmax (double __x, double __y) __ATTR_CONST__;
#define fmaxf	fmax		/**< The alias for fmax().	*/

/**
    The fmin() function returns the lesser of the two values \a __x and
    \a __y. If an argument is NaN, the other argument is returned. If
    both arguments are NaN, NaN is returned.
 */
extern double fmin (double __x, double __y) __ATTR_CONST__;
#define fminf	fmin		/**< The alias for fmin().	*/

/**
    The trunc() function rounds \a __x to the nearest integer not larger
    in absolute value.
 */
extern double trunc (double __x) __ATTR_CONST__;
#define truncf	trunc		/**< The alias for trunc().	*/

/**
    The round() function rounds \a __x to the nearest integer, but rounds
    halfway cases away from zero (instead of to the nearest even integer).
    Overflow is impossible.

    \return The rounded value. If \a __x is an integral or infinite, \a
    __x itself is returned. If \a __x is \c NaN, then \c NaN is returned.
 */
extern double round (double __x) __ATTR_CONST__;
#define roundf	round		/**< The alias for round().	*/

/**
    The lround() function rounds \a __x to the nearest integer, but rounds
    halfway cases away from zero (instead of to the nearest even integer).
    This function is similar to round() function, but it differs in type of
    return value and in that an overflow is possible.

    \return The rounded long integer value. If \a __x is not a finite number
    or an overflow was, this realization returns the \c LONG_MIN value
    (0x80000000).
 */
extern long lround (double __x) __ATTR_CONST__;
#define lroundf	lround		/**< The alias for lround().	*/

/**
    The lrint() function rounds \a __x to the nearest integer, rounding the
    halfway cases to the even integer direction. (That is both 1.5 and 2.5
    values are rounded to 2). This function is similar to rint() function,
    but it differs in type of return value and in that an overflow is
    possible.

    \return The rounded long integer value. If \a __x is not a finite
    number or an overflow was, this realization returns the \c LONG_MIN
    value (0x80000000).
 */
extern long lrint (double __x) __ATTR_CONST__;
#define lrintf	lrint		/**< The alias for lrint().	*/

#ifdef __cplusplus
}
#endif

/*@}*/
#endif /* !__MATH_H */

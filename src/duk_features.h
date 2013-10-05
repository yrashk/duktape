/*
 *  Determine platform features, select feature selection defines
 *  (e.g. _XOPEN_SOURCE), include system headers, and define DUK_USE_XXX
 *  defines which are (only) checked in Duktape internal code for
 *  activated features.  Duktape feature selection is based on DUK_PROFILE,
 *  other user supplied defines, and automatic feature detection.
 *
 *  This file is included by duk_internal.h before anything else is
 *  included.  Feature selection defines (e.g. _XOPEN_SOURCE) are defined
 *  here before any system headers are included (which is a requirement for
 *  system headers to work correctly).  This file is responsible for including
 *  all system headers and contains all platform dependent cruft in general.
 *
 *  The general order of handling:
 *    - Compiler feature detection (require no includes)
 *    - Intermediate platform detection (-> easier platform defines)
 *    - Platform detection, system includes, byte order detection, etc
 *    - ANSI C wrappers (e.g. DUK_MEMCMP), wrappers for constants, etc
 *    - Duktape profile handling, DUK_USE_xxx constants are set
 *    - Duktape Date provider settings
 *    - Final sanity checks
 *
 *  DUK_F_XXX are internal feature detection macros which should not
 *  be used outside this header.
 *
 *  Useful resources:
 *
 *    http://sourceforge.net/p/predef/wiki/Home/
 *    http://sourceforge.net/p/predef/wiki/Architectures/
 *    http://stackoverflow.com/questions/5919996/how-to-detect-reliably-mac-os-x-ios-linux-windows-in-c-preprocessor
 *    http://en.wikipedia.org/wiki/C_data_types#Fixed-width_integer_types
 *
 *  FIXME: at the moment there is no direct way of configuring
 *  or overriding individual settings.
 */

#ifndef DUK_FEATURES_H_INCLUDED
#define DUK_FEATURES_H_INCLUDED

/* FIXME: platform detection and all includes and defines in one big
 * if-else ladder (now e.g. datetime providers is a separate ladder).
 */

/*
 *  Compiler features
 */

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)
#define  DUK_F_C99
#else
#undef   DUK_F_C99
#endif

/*
 *  Provides the duk_rdtsc() inline function (if available)
 *
 *  See: http://www.mcs.anl.gov/~kazutomo/rdtsc.html
 */

/* XXX: more accurate detection of what gcc versions work; more inline
 * asm versions for other compilers.
 */
#if defined(__GNUC__) && defined(__i386__)
static __inline__ unsigned long long duk_rdtsc(void) {
	unsigned long long int x;
	__asm__ volatile (".byte 0x0f, 0x31" : "=A" (x));
	return x;
}
#define  DUK_RDTSC_AVAILABLE 1
#elif defined(__GNUC__) && defined(__x86_64__)
static __inline__ unsigned long long duk_rdtsc(void) {
	unsigned hi, lo;
	__asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
	return ((unsigned long long) lo) | (((unsigned long long) hi) << 32);
}
#define  DUK_RDTSC_AVAILABLE 1
#else
/* not available */
#undef  DUK_RDTSC_AVAILABLE
#endif

/*
 *  Intermediate platform detection
 *
 *  Provide easier defines for platforms.
 */

#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD) || \
    defined(__bsdi__) || defined(__DragonFly__)
#define  DUK_F_BSD
#endif

/* FIXME: Atari ST (TOS) with PureC is an exotic platform test; it now
 * fails because 'int' is 16 bit.
 */
#if defined(__TOS__)
#define  DUK_F_TOS
#endif

/* FIXME: This is now incorrect; is there a define for AmigaOS?  Assume
 * VBCC implies AmigaOS for now.
 */
#if defined(__VBCC__)
#define  DUK_F_AMIGAOS
#endif

/*
 *  Platform detection and system includes
 *
 *  Feature selection (e.g. _XOPEN_SOURCE) must happen before any system
 *  headers are included.
 *
 *  Can trigger standard byte order detection (later in this file) or
 *  specify byte order explicitly on more exotic platforms.
 */

#if defined(__linux)
#define  _POSIX_C_SOURCE  200809L
#define  _GNU_SOURCE      /* e.g. getdate_r */
#define  _XOPEN_SOURCE    /* e.g. strptime */
#endif

#if defined(__APPLE__)
/* Apple OSX */
#define  DUK_F_STD_BYTEORDER_DETECT
#include <architecture/byte_order.h>
#include <limits.h>
#include <sys/param.h>
#elif defined(DUK_F_BSD)
/* BSD */
#define  DUK_F_STD_BYTEORDER_DETECT
#include <sys/endian.h>
#include <limits.h>
#include <sys/param.h>
#elif defined(DUK_F_TOS)
/* Atari ST */
#define  DUK_USE_DOUBLE_BE
#include <limits.h>
#elif defined(DUK_F_AMIGAOS)
/* Amiga OS on M68k */
/* FIXME: check for M68k */
#define  DUK_USE_DOUBLE_BE
#include <limits.h>
#else
/* Linux and hopefully others */
#define  DUK_F_STD_BYTEORDER_DETECT
#include <endian.h>
#include <limits.h>
#include <sys/param.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>  /* varargs */
#include <setjmp.h>
#include <stddef.h>  /* e.g. ptrdiff_t */

#ifdef DUK_F_TOS
/*FIXME*/
#else
#include <stdint.h>
#endif

#include <math.h>

/*
 *  Sanity check types and define bit types such as duk_u32
 */

/* FIXME: Is there a reason not to rely on C99 types only, and only fall
 * back to guessing if C99 types are not available?
 */

/* FIXME: How to do reasonable automatic detection on older compilers,
 * and how to allow user override?
 */

#ifdef INT_MAX
#if INT_MAX < 2147483647
#error INT_MAX too small, expected int to be 32 bits at least
#endif
#else
#error INT_MAX not defined
#endif

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L) && \
    !(defined(DUK_F_AMIGAOS) && defined(__VBCC__)) /* vbcc + AmigaOS has C99 but no inttypes.h */
/* C99 */
#include <inttypes.h>
typedef uint8_t duk_u8;
typedef int8_t duk_i8;
typedef uint16_t duk_u16;
typedef int16_t duk_i16;
typedef uint32_t duk_u32;
typedef int32_t duk_i32;
#else
/* FIXME: need actual detection here */
typedef unsigned char duk_u8;
typedef signed char duk_i8;
typedef unsigned short duk_u16;
typedef signed short duk_i16;
typedef unsigned int duk_u32;
typedef signed int duk_i32;
#endif

/*
 *  Support for unaligned accesses
 */

/* FIXME: currently just a hack for ARM, what would be a good way to detect? */
#if defined(__arm__) || defined(__thumb__) || defined(_ARM) || defined(_M_ARM)
#undef   DUK_USE_UNALIGNED_ACCESSES_POSSIBLE
#else
#define  DUK_USE_UNALIGNED_ACCESSES_POSSIBLE
#endif

/*
 *  Byte order and double memory layout detection
 *
 *  This needs to be done before choosing a default profile, as it affects
 *  profile selection.
 */

/* FIXME: Not very good detection right now, expect to find __BYTE_ORDER
 * and __FLOAT_WORD_ORDER or resort to GCC/ARM specifics.  Improve the
 * detection code and perhaps allow some compiler define to override the
 * detection for unhandled cases.
 */

#if defined(DUK_F_STD_BYTEORDER_DETECT)
/* determine endianness variant: little-endian (LE), big-endian (BE), or "middle-endian" (ME) i.e. ARM */
#if (defined(__BYTE_ORDER) && defined(__LITTLE_ENDIAN) && (__BYTE_ORDER == __LITTLE_ENDIAN)) || \
    (defined(__LITTLE_ENDIAN__))
#if defined(__FLOAT_WORD_ORDER) && defined(__LITTLE_ENDIAN) && (__FLOAT_WORD_ORDER == __LITTLE_ENDIAN) || \
    (defined(__GNUC__) && !defined(__arm__))
#define DUK_USE_DOUBLE_LE
#elif (defined(__FLOAT_WORD_ORDER) && defined(__BIG_ENDIAN) && (__FLOAT_WORD_ORDER == __BIG_ENDIAN)) || \
      (defined(__GNUC__) && defined(__arm__))
#define DUK_USE_DOUBLE_ME
#else
#error unsupported: byte order is little endian but cannot determine IEEE double word order
#endif
#elif (defined(__BYTE_ORDER) && defined(__BIG_ENDIAN) && (__BYTE_ORDER == __BIG_ENDIAN)) || \
      (defined(__BIG_ENDIAN__))
#if (defined(__FLOAT_WORD_ORDER) && defined(__BIG_ENDIAN) && (__FLOAT_WORD_ORDER == __BIG_ENDIAN)) || \
    (defined(__GNUC__) && !defined(__arm__))
#define DUK_USE_DOUBLE_BE
#else
#error unsupported: byte order is big endian but cannot determine IEEE double word order
#endif
#else
#error unsupported: cannot determine byte order
#endif
#endif  /* DUK_F_STD_BYTEORDER_DETECT */

#if !defined(DUK_USE_DOUBLE_LE) && !defined(DUK_USE_DOUBLE_ME) && !defined(DUK_USE_DOUBLE_BE)
#error unsupported: cannot determine IEEE double byte order variant
#endif

/*
 *  Check whether or not a packed duk_tval representation is possible
 */

/* best effort viability checks, not particularly accurate */
#if (defined(__WORDSIZE) && (__WORDSIZE == 32)) && \
    (defined(UINT_MAX) && (UINT_MAX == 4294967295))
#define DUK_USE_PACKED_TVAL_POSSIBLE
#else
#undef  DUK_USE_PACKED_TVAL_POSSIBLE
#endif

/*
 *  Detection of double constants and math related functions.  Availability
 *  of constants and math functions is a significant porting concern.
 *
 *  INFINITY/HUGE_VAL is problematic on GCC-3.3: it causes an overflow warning
 *  and there is no pragma in GCC-3.3 to disable it.  Using __builtin_inf()
 *  avoids this problem for some reason.
 */

#define  DUK_DOUBLE_2TO32     4294967296.0
#define  DUK_DOUBLE_2TO31     2147483648.0

#undef  DUK_USE_COMPUTED_INFINITY
#if defined(__GNUC__) && defined(__GNUC_MINOR__) && \
    (((__GNUC__ == 4) && (__GNUC_MINOR__ < 6)) || (__GNUC__ < 4))
/* GCC older than 4.6: avoid overflow warnings related to using INFINITY */
#define  DUK_DOUBLE_INFINITY  (__builtin_inf())
#elif defined(INFINITY)
#define  DUK_DOUBLE_INFINITY  ((double) INFINITY)
#elif !defined(__VBCC__)
#define  DUK_DOUBLE_INFINITY  (1.0 / 0.0)
#else
/* In VBCC (1.0 / 0.0) results in a warning and 0.0 instead of infinity.
 * Use a computed infinity(initialized when a heap is created at the
 * latest).
 */
extern double duk_computed_infinity;
#define  DUK_USE_COMPUTED_INFINITY
#define  DUK_DOUBLE_INFINITY  duk_computed_infinity
#endif

#undef  DUK_USE_COMPUTED_NAN
#if defined(NAN)
#define  DUK_DOUBLE_NAN       NAN
#elif !defined(__VBCC__)
#define  DUK_DOUBLE_NAN       (0.0 / 0.0)
#define  NAN                  DUK_DOUBLE_NAN  /*FIXME*/
#else
/* In VBCC (0.0 / 0.0) results in a warning and 0.0 instead of NaN.
 * Use a computed NaN (initialized when a heap is created at the
 * latest).
 */
extern double duk_computed_nan;
#define  DUK_USE_COMPUTED_NAN
#define  DUK_DOUBLE_NAN       duk_computed_nan
#define  NAN                  DUK_DOUBLE_NAN  /*FIXME*/
#endif

/* Many platforms are missing fpclassify() and friends, so use replacements
 * if necessary.  The replacement constants (FP_NAN etc) can be anything but
 * match Linux constants now.
 */
#undef  DUK_USE_REPL_FPCLASSIFY
#undef  DUK_USE_REPL_SIGNBIT
#undef  DUK_USE_REPL_ISFINITE
#undef  DUK_USE_REPL_ISNAN
#if !(defined(FP_NAN) && defined(FP_INFINITE) && defined(FP_ZERO) && \
      defined(FP_SUBNORMAL) && defined(FP_NORMAL)) || \
    (defined(DUK_F_AMIGAOS) && defined(__VBCC__))
#define  DUK_USE_REPL_FPCLASSIFY
#define  DUK_USE_REPL_SIGNBIT
#define  DUK_USE_REPL_ISFINITE
#define  DUK_USE_REPL_ISNAN
#define  DUK_FPCLASSIFY       duk_repl_fpclassify
#define  DUK_SIGNBIT          duk_repl_signbit
#define  DUK_ISFINITE         duk_repl_isfinite
#define  DUK_ISNAN            duk_repl_isnan
#define  DUK_FP_NAN           0
#define  DUK_FP_INFINITE      1
#define  DUK_FP_ZERO          2
#define  DUK_FP_SUBNORMAL     3
#define  DUK_FP_NORMAL        4
#else
#define  DUK_FPCLASSIFY       fpclassify
#define  DUK_SIGNBIT          signbit
#define  DUK_ISFINITE         isfinite
#define  DUK_ISNAN            isnan
#define  DUK_FP_NAN           FP_NAN
#define  DUK_FP_INFINITE      FP_INFINITE
#define  DUK_FP_ZERO          FP_ZERO
#define  DUK_FP_SUBNORMAL     FP_SUBNORMAL
#define  DUK_FP_NORMAL        FP_NORMAL
#endif

/* Some math functions are C99 only.  This is also an issue with some
 * embedded environments using uclibc where uclibc has been configured
 * not to provide some functions.  For now, use replacements whenever
 * using uclibc.
 */
#if defined(DUK_F_C99) && \
    !defined(__UCLIBC__) /* uclibc may be missing these */ && \
    !(defined(DUK_F_AMIGAOS) && defined(__VBCC__)) /* vbcc + AmigaOS may be missing these */
#define  DUK_USE_MATH_FMIN
#define  DUK_USE_MATH_FMAX
#define  DUK_USE_MATH_ROUND
#else
#undef  DUK_USE_MATH_FMIN
#undef  DUK_USE_MATH_FMAX
#undef  DUK_USE_MATH_ROUND
#endif

/*
 *  ANSI C string/memory function wrapper defines to allow easier workarounds.
 *
 *  For instance, some platforms don't support zero-size memcpy correctly,
 *  some arcane uclibc versions have a buggy memcpy (but working memmove)
 *  and so on.  Such broken platforms can be dealt with here.
 */

/* Old uclibcs have a broken memcpy so use memmove instead (this is overly
 * wide now on purpose):
 * http://lists.uclibc.org/pipermail/uclibc-cvs/2008-October/025511.html
 */
#if defined(__UCLIBC__)
#define  DUK_MEMCPY       memmove
#else
#define  DUK_MEMCPY       memcpy
#endif

#define  DUK_MEMMOVE      memmove
#define  DUK_MEMCMP       memcmp
#define  DUK_MEMSET       memset
#define  DUK_STRCMP       strcmp
#define  DUK_STRNCMP      strncmp
#define  DUK_SPRINTF      sprintf
#define  DUK_SNPRINTF     snprintf
#define  DUK_VSPRINTF     vsprintf
#define  DUK_VSNPRINTF    vsnprintf

/*
 *  Macro hackery to convert e.g. __LINE__ to a string without formatting,
 *  see: http://stackoverflow.com/questions/240353/convert-a-preprocessor-token-to-a-string
 */

#define  DUK_F_STRINGIFY_HELPER(x)  #x
#define  DUK_MACRO_STRINGIFY(x)  DUK_F_STRINGIFY_HELPER(x)

/*
 *  Macro for suppressing warnings for potentially unreferenced variables.
 *  The variables can be actually unreferenced or unreferenced in some
 *  specific cases only; for instance, if a variable is only debug printed,
 *  it is unreferenced when debug printing is disabled.
 *
 *  (Introduced here because it's potentially compiler specific.)
 */

#define  DUK_UNREF(x)  do { \
		(void) (x); \
	} while (0)

/* 
 *  Profile processing
 *
 *  DUK_PROFILE values:
 *    0      custom
 *    100    FULL
 *    101    FULL_DEBUG
 *    200    MINIMAL
 *    201    MINIMAL_DEBUG
 *    300    TINY
 *    301    TINY_DEBUG
 *    400    PORTABLE        [tagged types]
 *    401    PORTABLE_DEBUG  [tagged types]
 *    500    TORTURE         [tagged types + torture]
 *    501    TORTURE_DEBUG   [tagged types + torture]
 */

#if !defined(DUK_PROFILE)
#if defined(DUK_USE_PACKED_TVAL_POSSIBLE)
#define  DUK_PROFILE  100
#else
#define  DUK_PROFILE  400
#endif
#endif

#if (DUK_PROFILE > 0)

/* start with the settings for the FULL profile */

#define  DUK_USE_SELF_TEST_TVAL
#define  DUK_USE_PACKED_TVAL
#undef   DUK_USE_FULL_TVAL
#define  DUK_USE_REFERENCE_COUNTING
#define  DUK_USE_DOUBLE_LINKED_HEAP
#define  DUK_USE_MARK_AND_SWEEP
#define  DUK_USE_AUGMENT_ERRORS
#define  DUK_USE_TRACEBACKS
#undef   DUK_USE_GC_TORTURE
#undef   DUK_USE_DEBUG
#undef   DUK_USE_DDEBUG
#undef   DUK_USE_DDDEBUG
#undef   DUK_USE_DPRINT_RDTSC                       /* feature determination below */
#define  DUK_USE_VERBOSE_ERRORS
#undef   DUK_USE_ASSERTIONS
#undef   DUK_USE_VARIADIC_MACROS                    /* feature determination below */
#define  DUK_USE_PROVIDE_DEFAULT_ALLOC_FUNCTIONS
#undef   DUK_USE_EXPLICIT_NULL_INIT
#define  DUK_USE_REGEXP_SUPPORT
#define  DUK_USE_STRICT_UTF8_SOURCE
#define  DUK_USE_OCTAL_SUPPORT
#define  DUK_USE_SOURCE_NONBMP
#define  DUK_USE_DPRINT_COLORS
#define  DUK_USE_BROWSER_LIKE
#define  DUK_USE_SECTION_B

/* unaligned accesses */
#ifdef DUK_USE_UNALIGNED_ACCESSES_POSSIBLE
#define  DUK_USE_HASHBYTES_UNALIGNED_U32_ACCESS
#define  DUK_USE_HOBJECT_UNALIGNED_LAYOUT
#else
#undef   DUK_USE_HASHBYTES_UNALIGNED_U32_ACCESS
#undef   DUK_USE_HOBJECT_UNALIGNED_LAYOUT
#endif

/* profile specific modifications */

#if (DUK_PROFILE == 100)
/* FULL */
#elif (DUK_PROFILE == 101)
/* FULL_DEBUG */
#define  DUK_USE_DEBUG
#undef   DUK_USE_DDEBUG
#undef   DUK_USE_DDDEBUG
#define  DUK_USE_ASSERTIONS
#elif (DUK_PROFILE == 200)
/* MINIMAL */
#undef   DUK_USE_VERBOSE_ERRORS
#elif (DUK_PROFILE == 201)
/* MINIMAL_DEBUG */
#define  DUK_USE_DEBUG
#undef   DUK_USE_DDEBUG
#undef   DUK_USE_DDDEBUG
#define  DUK_USE_ASSERTIONS
#elif (DUK_PROFILE == 300)
/* TINY */
#undef   DUK_USE_SELF_TEST_TVAL
#undef   DUK_USE_REFERENCE_COUNTING
#undef   DUK_USE_DOUBLE_LINKED_HEAP
#define  DUK_USE_MARK_AND_SWEEP
#undef   DUK_USE_AUGMENT_ERRORS
#undef   DUK_USE_TRACEBACKS
#undef   DUK_USE_VERBOSE_ERRORS
#elif (DUK_PROFILE == 301)
/* TINY_DEBUG */
#undef   DUK_USE_SELF_TEST_TVAL
#undef   DUK_USE_REFERENCE_COUNTING
#undef   DUK_USE_DOUBLE_LINKED_HEAP
#define  DUK_USE_MARK_AND_SWEEP
#undef   DUK_USE_AUGMENT_ERRORS
#undef   DUK_USE_TRACEBACKS
#define  DUK_USE_DEBUG
#undef   DUK_USE_DDEBUG
#undef   DUK_USE_DDDEBUG
#define  DUK_USE_ASSERTIONS
#undef   DUK_USE_VERBOSE_ERRORS
#elif (DUK_PROFILE == 400)
#undef   DUK_USE_PACKED_TVAL
#undef   DUK_USE_FULL_TVAL
#define  DUK_USE_EXPLICIT_NULL_INIT
#elif (DUK_PROFILE == 401)
#undef   DUK_USE_PACKED_TVAL
#undef   DUK_USE_FULL_TVAL
#undef   DUK_USE_GC_TORTURE
#define  DUK_USE_DEBUG
#undef   DUK_USE_DDEBUG
#undef   DUK_USE_DDDEBUG
#define  DUK_USE_ASSERTIONS
#define  DUK_USE_EXPLICIT_NULL_INIT
#elif (DUK_PROFILE == 500)
#undef   DUK_USE_PACKED_TVAL
#undef   DUK_USE_FULL_TVAL
#define  DUK_USE_GC_TORTURE
#elif (DUK_PROFILE == 501)
#undef   DUK_USE_PACKED_TVAL
#undef   DUK_USE_FULL_TVAL
#define  DUK_USE_GC_TORTURE
#define  DUK_USE_DEBUG
#undef   DUK_USE_DDEBUG
#undef   DUK_USE_DDDEBUG
#undef   DUK_USE_ASSERTIONS
#else
#error unknown DUK_PROFILE
#endif

/* FIXME: how to handle constants like these? */
#if defined(DUK_USE_TRACEBACKS) && !defined(DUK_OPT_TRACEBACK_DEPTH)
#define  DUK_OPT_TRACEBACK_DEPTH  10
#endif

/*
 *  Dynamically detected features
 */

#if defined(DUK_RDTSC_AVAILABLE) && defined(DUK_OPT_DPRINT_RDTSC)
#define  DUK_USE_DPRINT_RDTSC
#else
#undef  DUK_USE_DPRINT_RDTSC
#endif

#ifdef DUK_F_C99
#define  DUK_USE_VARIADIC_MACROS
#else
#undef  DUK_USE_VARIADIC_MACROS
#endif

/* zero-size array at end of struct (char buf[0]) instead of C99 version (char buf[]) */
#ifdef DUK_F_C99
#undef  DUK_USE_STRUCT_HACK
#else
#define  DUK_USE_STRUCT_HACK  /* non-portable */
#endif

/* FIXME: GCC pragma inside a function fails in some earlier GCC versions (e.g. gcc 4.5).
 * This is very approximate but allows clean builds for development right now.
 */
/* http://gcc.gnu.org/onlinedocs/cpp/Common-Predefined-Macros.html */
#if defined(__GNUC__) && defined(__GNUC_MINOR__) && (__GNUC__ == 4) && (__GNUC_MINOR__ >= 6)
#define  DUK_USE_GCC_PRAGMAS
#else
#undef  DUK_USE_GCC_PRAGMAS
#endif

/*
 *  Date built-in platform primitive selection
 *
 *  This is a direct platform dependency which is difficult to eliminate.
 *  Select provider through defines, and then include necessary system
 *  headers so that duk_builtin_date.c compiles.
 *
 *  FIXME: add a way to provide custom functions to provide the critical
 *  primitives; this would be convenient when porting to unknown platforms
 *  (rather than muck with Duktape internals).
 */

/* NOW = getting current time (required)
 * TZO = getting local time offset (required)
 * PRS = parse datetime (optional)
 * FMT = format datetime (optional)
 */

#if defined(_WIN64)
/* Windows 64-bit */
#error WIN64 not supported
#elif defined(_WIN32) || defined(WIN32)
/* Windows 32-bit */
#error WIN32 not supported
#elif defined(__APPLE__)
/* Mac OSX, iPhone, Darwin */
#define  DUK_USE_DATE_NOW_GETTIMEOFDAY
#define  DUK_USE_DATE_TZO_GMTIME
#define  DUK_USE_DATE_PRS_STRPTIME
#define  DUK_USE_DATE_FMT_STRFTIME
#elif defined(__linux)
/* Linux (__unix also defined) */
#define  DUK_USE_DATE_NOW_GETTIMEOFDAY
#define  DUK_USE_DATE_TZO_GMTIME
#define  DUK_USE_DATE_PRS_STRPTIME
#define  DUK_USE_DATE_FMT_STRFTIME
#elif defined(__unix)
/* Other Unix */
#define  DUK_USE_DATE_NOW_GETTIMEOFDAY
#define  DUK_USE_DATE_TZO_GMTIME
#define  DUK_USE_DATE_PRS_STRPTIME
#define  DUK_USE_DATE_FMT_STRFTIME
#elif defined(__posix)
/* POSIX */
#define  DUK_USE_DATE_NOW_GETTIMEOFDAY
#define  DUK_USE_DATE_TZO_GMTIME
#define  DUK_USE_DATE_PRS_STRPTIME
#define  DUK_USE_DATE_FMT_STRFTIME
#elif defined(DUK_F_TOS)
/* Atari ST */
#define  DUK_USE_DATE_NOW_TIME
#define  DUK_USE_DATE_TZO_GMTIME
/* no parsing (not an error) */
#define  DUK_USE_DATE_FMT_STRFTIME
#elif defined(DUK_F_AMIGAOS)
/* Amiga OS */
#define  DUK_USE_DATE_NOW_TIME
#define  DUK_USE_DATE_TZO_GMTIME
/* no parsing (not an error) */
#define  DUK_USE_DATE_FMT_STRFTIME
#else
#error platform not supported
#endif

#if defined(DUK_USE_DATE_NOW_GETTIMEOFDAY)
#include <sys/time.h>
#endif

#if defined(DUK_USE_DATE_TZO_GMTIME) || \
    defined(DUK_USE_DATE_PRS_STRPTIME) || \
    defined(DUK_USE_DATE_FMT_STRFTIME)
/* just a sanity check */
#if defined(__linux) && !defined(_XOPEN_SOURCE)
#error expected _XOPEN_SOURCE to be defined here
#endif
#include <time.h>
#endif

#else  /* DUK_PROFILE > 0 */

/*
 *  All DUK_USE_ defines must be defined manually, no compiler
 *  or platform feature detection.
 */

#endif  /* DUK_PROFILE > 0 */

/* FIXME: An alternative approach to customization would be to include
 * some user define file at this point.  The user file could then modify
 * the base settings.  Something like:
 * #ifdef DUK_CUSTOM_HEADER
 * #include "duk_custom.h"
 * #endif
 */

/*
 *  Sanity checks on defines
 */

#if defined(DUK_DDEBUG) && !defined(DUK_DEBUG)
#error DUK_DEBUG and DUK_DDEBUG should not be defined (obsolete)
#endif

#if defined(DUK_USE_DDEBUG) && !defined(DUK_USE_DEBUG)
#error DUK_USE_DDEBUG defined without DUK_USE_DEBUG
#endif

#if defined(DUK_USE_DDDEBUG) && !defined(DUK_USE_DEBUG)
#error DUK_USE_DDDEBUG defined without DUK_USE_DEBUG
#endif

#if defined(DUK_USE_DDDEBUG) && !defined(DUK_USE_DDEBUG)
#error DUK_USE_DDDEBUG defined without DUK_USE_DDEBUG
#endif

#if defined(DUK_USE_REFERENCE_COUNTING) && !defined(DUK_USE_DOUBLE_LINKED_HEAP)
#error DUK_USE_REFERENCE_COUNTING defined without DUK_USE_DOUBLE_LINKED_HEAP
#endif

#if defined(DUK_USE_GC_TORTURE) && !defined(DUK_USE_MARK_AND_SWEEP)
#error DUK_USE_GC_TORTURE defined without DUK_USE_MARK_AND_SWEEP
#endif

#endif  /* DUK_FEATURES_H_INCLUDED */


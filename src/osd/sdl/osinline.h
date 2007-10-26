//============================================================
//
//	osinline.h - GNU C inline functions
//
//============================================================

#ifndef __OSINLINE__
#define __OSINLINE__

#include "osd_cpu.h"

//============================================================
//	INLINE FUNCTIONS
//============================================================

//#define YieldProcessor() sched_yield()
//#define YieldProcessor() do {} while (0)

#ifndef YieldProcessor

#if defined(__i386__) || defined(__x86_64__)

INLINE void osd_yield_processor(void)
{
	__asm__ __volatile__ ( " rep ; nop ;" );
}

#elif defined(__ppc__) || defined (__PPC__) || defined(__ppc64__) || defined(__PPC64__)

INLINE void osd_yield_processor(void)
{
	__asm__ __volatile__ ( " nop \n" );
}

#endif

#else
#define osd_yield_processor() YieldProcessor()
#endif

#ifdef __GNUC__

#if defined(__i386__) || defined(__x86_64__)


//============================================================
//  osd_compare_exchange32
//============================================================
#ifndef SDLMAME_WIN32
ATTR_UNUSED
INLINE INT32 _osd_compare_exchange32(INT32 volatile *ptr, INT32 compare, INT32 exchange)
{
	register INT32 ret;
	__asm__ __volatile__ (
		" lock ; cmpxchg %[exchange], %[ptr] ;"
		: [ptr]      "+m" (*ptr)
		, [ret]      "=a" (ret)
		: [compare]  "1"  (compare)
		, [exchange] "q"  (exchange)
		: "%cc"
	);
	return ret;
}
#define osd_compare_exchange32 _osd_compare_exchange32


//============================================================
//  osd_exchange32
//============================================================

ATTR_UNUSED
INLINE INT32 _osd_exchange32(INT32 volatile *ptr, INT32 exchange)
{
	register INT32 ret;
	__asm__ __volatile__ (
		" lock ; xchg %[exchange], %[ptr] ;"
		: [ptr]      "+m" (*ptr)
		, [ret]      "=r" (ret)
		: [exchange] "1"  (exchange)
	);
	return ret;
}
#define osd_exchange32 _osd_exchange32
#endif


//============================================================
//  osd_sync_add
//============================================================

ATTR_UNUSED
INLINE INT32 _osd_sync_add(INT32 volatile *ptr, INT32 delta)
{
	register INT32 ret;
	__asm __volatile__ (
		" mov           %[delta],%[ret] ;"
		" lock ; xadd   %[ret],%[ptr]   ;"
		" add           %[delta],%[ret] ;"
		: [ptr]   "+m"  (*ptr)
		, [ret]   "=&r" (ret)
		: [delta] "rmi" (delta)
		: "%cc"
	);
	return ret;
}
#define osd_sync_add _osd_sync_add


//============================================================
//  osd_interlocked_increment
//============================================================

ATTR_UNUSED
INLINE INT32 _osd_interlocked_increment(INT32 volatile *ptr)
{
	register INT32 ret;
	__asm__ __volatile__(
		" mov           $1,%[ret]     ;"
		" lock ; xadd   %[ret],%[ptr] ;"
		" inc           %[ret]        ;"
		: [ptr] "+m"  (*ptr)
		, [ret] "=&r" (ret)
		: 
		: "%cc"
	);
	return ret;
}
#define osd_interlocked_increment _osd_interlocked_increment


//============================================================
//  osd_interlocked_decrement
//============================================================

ATTR_UNUSED
INLINE INT32 _osd_interlocked_decrement(INT32 volatile *ptr)
{
	register INT32 ret;
	__asm__ __volatile__(
		" mov           $-1,%[ret]    ;"
		" lock ; xadd   %[ret],%[ptr] ;"
		" dec           %[ret]        ;"
		: [ptr] "+m"  (*ptr)
		, [ret] "=&r" (ret)
		: 
		: "%cc"
	);
	return ret;
}
#define osd_interlocked_decrement _osd_interlocked_decrement


#if defined(__x86_64__)

//============================================================
//  osd_compare_exchange64
//============================================================

ATTR_UNUSED
INLINE INT64 _osd_compare_exchange64(INT64 volatile *ptr, INT64 compare, INT64 exchange)
{
	register INT64 ret;
	__asm__ __volatile__ (
		" lock ; cmpxchg %[exchange], %[ptr] ;"
		: [ptr]      "+m" (*ptr)
		, [ret]      "=a" (ret)
		: [compare]  "1"  (compare)
		, [exchange] "q"  (exchange)
		: "%cc"
	);
	return ret;
}
#define osd_compare_exchange64 _osd_compare_exchange64


//============================================================
//  osd_exchange64
//============================================================

ATTR_UNUSED
INLINE INT64 _osd_exchange64(INT64 volatile *ptr, INT64 exchange)
{
	register INT64 ret;
	__asm__ __volatile__ (
		" lock ; xchg %[exchange], %[ptr] ;"
		: [ptr]      "+m" (*ptr)
		, [ret]      "=r" (ret)
		: [exchange] "1"  (exchange)
	);
	return ret;
}
#define osd_exchange64 _osd_exchange64

#endif /* __x86_64__ */


#elif defined(__ppc__) || defined(__PPC__) || defined(__ppc64__) || defined(__PPC64__)


//============================================================
//  osd_compare_exchange32
//============================================================

ATTR_UNUSED
INLINE INT32 _osd_compare_exchange32(INT32 volatile *ptr, INT32 compare, INT32 exchange)
{
	register INT32 ret;
	__asm__ __volatile__ (
		"1: lwarx  %[ret], 0, %[ptr]      \n"
		"   cmpw   %[compare], %[ret]     \n"
		"   bne    2f                     \n"
		"   sync                          \n"
		"   stwcx. %[exchange], 0, %[ptr] \n"
		"   bne- 1b                       \n"
		"2:                                 "
		: [ret]      "=&r" (ret)
		: [ptr]      "r"   (ptr)
		, [exchange] "r"   (exchange)
		, [compare]  "r"   (compare)
		: "cr0"
	);
	return ret;
}
#define osd_compare_exchange32 _osd_compare_exchange32


//============================================================
//  osd_exchange32
//============================================================

ATTR_UNUSED
INLINE INT32 _osd_exchange32(INT32 volatile *ptr, INT32 exchange)
{
	register INT32 ret;
	__asm__ __volatile__ (
		"1: lwarx  %[ret], 0, %[ptr]      \n"
		"   sync                          \n"
		"   stwcx. %[exchange], 0, %[ptr] \n"
		"   bne-   1b                     \n"
		: [ret]      "=&r" (ret)
		: [ptr]      "r"   (ptr)
		, [exchange] "r"   (exchange)
		: "cr0"
	);
	return ret;
}
#define osd_exchange32 _osd_exchange32


//============================================================
//  osd_sync_add
//============================================================

ATTR_UNUSED
INLINE INT32 _osd_sync_add(INT32 volatile *ptr, INT32 delta)
{
	register INT32 ret;
	__asm __volatile__ (
		"1: lwarx  %[ret], 0, %[ptr]        \n"
		"   add    %[ret], %[ret], %[delta] \n"
		"   sync                            \n"
		"   stwcx. %[ret], 0, %[ptr]        \n"
		"   bne- 1b                         \n"
		: [ret]   "=&b" (ret)
		: [ptr]   "r"   (ptr)
		, [delta] "r"   (delta)
		: "cr0"
	);
	return ret;
}
#define osd_sync_add _osd_sync_add


//============================================================
//  osd_interlocked_increment
//============================================================

ATTR_UNUSED
INLINE INT32 _osd_interlocked_increment(INT32 volatile *ptr)
{
	register INT32 ret;
	__asm__ __volatile__(
		"1: lwarx  %[ret], 0, %[ptr] \n"
		"   addi   %[ret], %[ret], 1 \n"
		"   sync                     \n"
		"   stwcx. %[ret], 0, %[ptr] \n"
		"   bne-   1b                \n"
		: [ret] "=&b" (ret)
		: [ptr] "r"   (ptr)
		: "cr0"
	);
	return ret;
}
#define osd_interlocked_increment _osd_interlocked_increment


//============================================================
//  osd_interlocked_decrement
//============================================================

ATTR_UNUSED
INLINE INT32 _osd_interlocked_decrement(INT32 volatile *ptr)
{
	register INT32 ret;
	__asm__ __volatile__(
		"1: lwarx  %[ret], 0, %[ptr]  \n"
		"   addi   %[ret], %[ret], -1 \n"
		"   sync                      \n"
		"   stwcx. %[ret], 0, %[ptr]  \n"
		"   bne-   1b                 \n"
		: [ret] "=&b" (ret)
		: [ptr] "r"   (ptr)
		: "cr0"
	);
	return ret;
}
#define osd_interlocked_decrement _osd_interlocked_decrement


#if defined(__ppc64__) || defined(__PPC64__)

//============================================================
//  osd_compare_exchange64
//============================================================

ATTR_UNUSED
INLINE INT64 _osd_compare_exchange64(INT64 volatile *ptr, INT64 compare, INT64 exchange)
{
	register INT64 ret;
	__asm__ __volatile__ (
		"1: ldarx  %[ret], 0, %[ptr]      \n"
		"   cmpd   %[compare], %[ret]     \n"
		"   bne    2f                     \n"
		"   stdcx. %[exchange], 0, %[ptr] \n"
		"   bne--  1b                     \n"
		"2:                                 "
		: [ret]      "=&r" (ret)
		: [ptr]      "r"   (ptr)
		, [exchange] "r"   (exchange)
		, [compare]  "r"   (compare)
		: "cr0"
	);
	return ret;
}
#define osd_compare_exchange64 _osd_compare_exchange64


//============================================================
//  osd_exchange64
//============================================================

ATTR_UNUSED
INLINE INT64 _osd_exchange64(INT64 volatile *ptr, INT64 exchange)
{
	register INT64 ret;
	__asm__ __volatile__ (
		"1: ldarx  %[ret], 0, %[ptr]      \n"
		"   stdcx. %[exchange], 0, %[ptr] \n"
		"   bne--  1b                     \n"
		: [ret]      "=&r" (ret)
		: [ptr]      "r"   (ptr)
		, [exchange] "r"   (exchange)
		: "cr0"
	);
	return ret;
}
#define osd_exchange64 _osd_exchange64

#endif /* __ppc64__ || __PPC64__ */


#endif

#endif /* __GNUC__ */


#endif /* __OSINLINE__ */

//============================================================
//
//  osinline.h - Win32 inline functions
//
//  Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
//  Visit http://mamedev.org for licensing and usage restrictions.
//
//============================================================

#ifndef __OSINLINE__
#define __OSINLINE__

#include "osd_cpu.h"


//============================================================
//  INLINE MATH FUNCTIONS
//============================================================

#if defined(_MSC_VER) && defined(_M_IX86) && !defined(PTR64)

// Microsoft Visual C, x86, 32-bit
#define vec_mult _vec_mult
INLINE int _vec_mult(int x, int y)
{
    int result;

    __asm {
        mov eax, x
        imul y
        mov result, edx
    }

    return result;
}



c:\Program Files (x86)\Microsoft Visual Studio 8\VC\include

#elif defined(__GNUC__) && !defined(PTR64)

// GCC, x86, 32-bit
#define vec_mult _vec_mult
INLINE int _vec_mult(int x, int y)
{
	int result;
	__asm__ (
			"movl  %1    , %0    ; "
			"imull %2            ; "    /* do the multiply */
			"movl  %%edx , %%eax ; "
			:  "=&a" (result)           /* the result has to go in eax */
			:  "mr" (x),                /* x and y can be regs or mem */
			   "mr" (y)
			:  "%edx", "%cc"            /* clobbers edx and flags */
		);
	return result;
}

#else

// other
#define vec_mult _vec_mult
INLINE int _vec_mult(int x, int y)
{
	return (int)(((INT64)x * (INT64)y) >> 32);
}

#endif // defined(_MSC_VER) && defined(_M_IX86) && !defined(PTR64)



//============================================================
//  INLINE SYNCHRONIZATION FUNCTIONS
//============================================================

#if defined(_MSC_VER)

// Microsoft Visual C
#include <intrin.h>

#pragma intrinsic(_InterlockedCompareExchange)
#pragma intrinsic(_InterlockedCompareExchange64)
#pragma intrinsic(_InterlockedExchangeAdd)

#define osd_compare_exchange32 _osd_compare_exchange32
INLINE INT32 _osd_compare_exchange32(INT32 volatile *ptr, INT32 compare, INT32 exchange)
{
	return _InterlockedCompareExchange(ptr, exchange, compare);
}

#ifdef PTR64
#define osd_compare_exchange64 _osd_compare_exchange64
INLINE INT64 _osd_compare_exchange64(INT64 volatile *ptr, INT64 compare, INT64 exchange)
{
	return _InterlockedCompareExchange64(ptr, exchange, compare);
}
#endif

#define osd_sync_add _osd_sync_add
INLINE INT32 osd_sync_add(INT32 volatile *ptr, INT32 delta)
{
	return _InterlockedExchangeAdd(ptr, delta) + delta;
}

#elif defined(__GNUC__) && !defined(PTR64)

// need to copy this from the SDL implementation

#endif



#endif /* __OSINLINE__ */

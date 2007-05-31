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
//  INLINE FUNCTIONS
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

#endif /* __OSINLINE__ */

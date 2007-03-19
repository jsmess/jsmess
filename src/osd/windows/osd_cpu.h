//============================================================
//
//  osd_cpu.h - Win32 CPU-specific data types
//
//  Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
//  Visit http://mamedev.org for licensing and usage restrictions.
//
//============================================================

/*******************************************************************************
*                                                                              *
*   Define size independent data types and operations.                         *
*                                                                              *
*   The following types must be supported by all platforms:                    *
*                                                                              *
*   UINT8  - Unsigned 8-bit Integer     INT8  - Signed 8-bit integer           *
*   UINT16 - Unsigned 16-bit Integer    INT16 - Signed 16-bit integer          *
*   UINT32 - Unsigned 32-bit Integer    INT32 - Signed 32-bit integer          *
*   UINT64 - Unsigned 64-bit Integer    INT64 - Signed 64-bit integer          *
*                                                                              *
*                                                                              *
*   The macro names for the artithmatic operations are composed as follows:    *
*                                                                              *
*   XXX_R_A_B, where XXX - 3 letter operation code (ADD, SUB, etc.)            *
*                    R   - The type of the result                              *
*                    A   - The type of operand 1                               *
*                    B   - The type of operand 2 (if binary operation)         *
*                                                                              *
*                    Each type is one of: U8,8,U16,16,U32,32,U64,64            *
*                                                                              *
*******************************************************************************/
#pragma once

#ifndef OSD_CPU_H
#define OSD_CPU_H

/* Combine two 32-bit integers into a 64-bit integer */
#define COMBINE_64_32_32(A,B)     ((((UINT64)(A))<<32) | (UINT32)(B))
#define COMBINE_U64_U32_U32(A,B)  COMBINE_64_32_32(A,B)

/* Return upper 32 bits of a 64-bit integer */
#define HI32_32_64(A)		  (((UINT64)(A)) >> 32)
#define HI32_U32_U64(A)		  HI32_32_64(A)

/* Return lower 32 bits of a 64-bit integer */
#define LO32_32_64(A)		  ((A) & 0xffffffff)
#define LO32_U32_U64(A)		  LO32_32_64(A)

#define DIV_64_64_32(A,B)	  ((A)/(B))
#define DIV_U64_U64_U32(A,B)  ((A)/(UINT32)(B))

#define MOD_32_64_32(A,B)	  ((A)%(B))
#define MOD_U32_U64_U32(A,B)  ((A)%(UINT32)(B))

#define MUL_64_32_32(A,B)	  ((A)*(INT64)(B))
#define MUL_U64_U32_U32(A,B)  ((A)*(UINT64)(UINT32)(B))


/***************************************************************************

    Inline math helpers

***************************************************************************/

#ifdef __GNUC__

INLINE UINT32 _count_leading_zeros(UINT32 value)
{
	UINT32 result;

	__asm__ (
		"bsrl %1,%0;"
		"xorl $31,%0;"
		: "=r,r" (result)		/* result can be in any register */
		: "r,m" (value)			/* 'value' can be register or memory */
		: "%cc"					/* clobbers condition codes */
	);

	return result;
}
#define count_leading_zeros _count_leading_zeros

INLINE UINT32 _count_leading_ones(UINT32 value)
{
	UINT32 result;

	__asm__ (
		"notl %1;"
		"bsrl %1,%0;"
		"notl %1;"
		"xorl $31,%0;"
		: "=r,r" (result)		/* result can be in any register */
		: "r,m" (value)			/* 'value' can be register or memory */
		: "%cc"					/* clobbers condition codes */
	);

	return result;
}
#define count_leading_ones _count_leading_ones

INLINE INT32 _fixed_mul_shift(INT32 val1, INT32 val2, UINT8 shift)
{
	INT32 result;

	__asm__ (
		"imull %2;"
		"shrdl %3,%%edx,%0;"
      : "=a,a,a,a" (result)		/* result ends up in eax */
      : "0,0,0,0" (val1),		/* 'val1' should also be in eax on entry */
        "r,m,r,m" (val2),		/* 'val2' can be memory or register */
        "I,I,c,c" (shift)		/* 'shift' must be constant in 0-31 range or in CL */
      : "%edx", "%cc"			/* clobbers EDX and condition codes */
	);

	return result;
}
#define fixed_mul_shift _fixed_mul_shift

#endif


#endif	/* defined OSD_CPU_H */

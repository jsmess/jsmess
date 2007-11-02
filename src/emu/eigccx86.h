/***************************************************************************

    eigccx86.h

    x86 (32 and 64-bit) inline implementations for GCC compilers. This
    code is automatically included if appropriate by eminline.h.

    Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#ifndef __EIGCCX86__
#define __EIGCCX86__


/***************************************************************************
    INLINE MATH FUNCTIONS
***************************************************************************/

/*-------------------------------------------------
    mul_32x32 - perform a signed 32 bit x 32 bit
    multiply and return the full 64 bit result
-------------------------------------------------*/

#ifndef PTR64
#define mul_32x32 _mul_32x32
INLINE INT64 _mul_32x32(INT32 a, INT32 b)
{
	INT64 result;

	__asm__ (
		"imull %2;"
      : "=A,A" (result)			/* result in edx:eax */
      : "0,0" (a),				/* 'a' should also be in eax on entry */
        "r,m" (b)				/* 'b' can be memory or register */
	);

	return result;
}
#endif


/*-------------------------------------------------
    mulu_32x32 - perform an unsigned 32 bit x
    32 bit multiply and return the full 64 bit
    result
-------------------------------------------------*/

#ifndef PTR64
#define mulu_32x32 _mulu_32x32
INLINE UINT64 _mulu_32x32(UINT32 a, UINT32 b)
{
	UINT64 result;

	__asm__ (
		"mull  %2;"
      : "=A,A" (result)			/* result in edx:eax */
      : "0,0" (a),				/* 'a' should also be in eax on entry */
        "r,m" (b)				/* 'b' can be memory or register */
	);

	return result;
}
#endif


/*-------------------------------------------------
    mul_32x32_hi - perform a signed 32 bit x 32 bit
    multiply and return the upper 32 bits of the
    result
-------------------------------------------------*/

#ifndef PTR64
#define mul_32x32_hi _mul_32x32_hi
INLINE INT32 _mul_32x32_hi(INT32 a, INT32 b)
{
	INT32 result;

	__asm__ (
		"imull %2;"
		"movl  %%edx,%0;"
      : "=a,a" (result)			/* result ends up in eax */
      : "0,0" (a),				/* 'a' should also be in eax on entry */
        "r,m" (b)				/* 'b' can be memory or register */
      : "%edx", "%cc"			/* clobbers EDX and condition codes */
	);

	return result;
}
#endif


/*-------------------------------------------------
    mulu_32x32_hi - perform an unsigned 32 bit x
    32 bit multiply and return the upper 32 bits
    of the result
-------------------------------------------------*/

#ifndef PTR64
#define mulu_32x32_hi _mulu_32x32_hi
INLINE UINT32 _mulu_32x32_hi(UINT32 a, UINT32 b)
{
	UINT32 result;

	__asm__ (
		"mull  %2;"
		"movl  %%edx,%0;"
      : "=a,a" (result)			/* result ends up in eax */
      : "0,0" (a),				/* 'a' should also be in eax on entry */
        "r,m" (b)				/* 'b' can be memory or register */
      : "%edx", "%cc"			/* clobbers EDX and condition codes */
	);

	return result;
}
#endif


/*-------------------------------------------------
    mul_32x32_shift - perform a signed 32 bit x
    32 bit multiply and shift the result by the
    given number of bits before truncating the
    result to 32 bits
-------------------------------------------------*/

#ifndef PTR64
#define mul_32x32_shift _mul_32x32_shift
INLINE INT32 _mul_32x32_shift(INT32 a, INT32 b, UINT8 shift)
{
	INT32 result;

	__asm__ (
		"imull %2;"
		"shrdl %3,%%edx,%0;"
      : "=a,a,a,a" (result)		/* result ends up in eax */
      : "0,0,0,0" (a),			/* 'a' should also be in eax on entry */
        "r,m,r,m" (b),			/* 'b' can be memory or register */
        "I,I,c,c" (shift)		/* 'shift' must be constant in 0-31 range or in CL */
      : "%edx", "%cc"			/* clobbers EDX and condition codes */
	);

	return result;
}
#endif


/*-------------------------------------------------
    mulu_32x32_shift - perform an unsigned 32 bit x
    32 bit multiply and shift the result by the
    given number of bits before truncating the
    result to 32 bits
-------------------------------------------------*/

#ifndef PTR64
#define mulu_32x32_shift _mulu_32x32_shift
INLINE UINT32 _mulu_32x32_shift(UINT32 a, UINT32 b, UINT8 shift)
{
	UINT32 result;

	__asm__ (
		"imull %2;"
		"shrdl %3,%%edx,%0;"
      : "=a,a,a,a" (result)		/* result ends up in eax */
      : "0,0,0,0" (a),			/* 'a' should also be in eax on entry */
        "r,m,r,m" (b),			/* 'b' can be memory or register */
        "I,I,c,c" (shift)		/* 'shift' must be constant in 0-31 range or in CL */
      : "%edx", "%cc"			/* clobbers EDX and condition codes */
	);

	return result;
}
#endif


/*-------------------------------------------------
    div_64x32 - perform a signed 64 bit x 32 bit
    divide and return the 32 bit quotient
-------------------------------------------------*/

/* TBD */


/*-------------------------------------------------
    divu_64x32 - perform an unsigned 64 bit x 32 bit
    divide and return the 32 bit quotient
-------------------------------------------------*/

/* TBD */


/*-------------------------------------------------
    div_32x32_shift - perform a signed divide of
    two 32 bit values, shifting the first before
    division, and returning the 32 bit quotient
-------------------------------------------------*/

#ifndef PTR64
/* TBD - I get an error if this is enabled: error: 'asm' operand requires impossible reload */
#if 0
#define div_32x32_shift _div_32x32_shift
INLINE INT32 _div_32x32_shift(INT32 a, INT32 b, UINT8 shift)
{
	INT32 result;

	__asm__ (
	    "cdq;"
	    "shldl %3,%0,%%edx;"
	    "shll  %3,%0;"
		"idivl %2;"
      : "=a,a,a,a" (result)		/* result ends up in eax */
      : "0,0,0,0" (a),			/* 'a' should also be in eax on entry */
        "r,m,r,m" (b),			/* 'b' can be memory or register */
        "I,I,c,c" (shift)		/* 'shift' must be constant in 0-31 range or in CL */
      : "%edx", "%cc"			/* clobbers EDX and condition codes */
	);

	return result;
}
#endif
#endif


/*-------------------------------------------------
    divu_32x32_shift - perform an unsigned divide of
    two 32 bit values, shifting the first before
    division, and returning the 32 bit quotient
-------------------------------------------------*/

#ifndef PTR64
#define divu_32x32_shift _divu_32x32_shift
INLINE UINT32 _divu_32x32_shift(UINT32 a, UINT32 b, UINT8 shift)
{
	INT32 result;

	__asm__ (
	    "xorl  %%edx,%%edx;"
	    "shldl %3,%0,%%edx;"
	    "shll  %3,%0;"
		"divl  %2;"
      : "=a,a,a,a" (result)		/* result ends up in eax */
      : "0,0,0,0" (a),			/* 'a' should also be in eax on entry */
        "r,m,r,m" (b),			/* 'b' can be memory or register */
        "I,I,c,c" (shift)		/* 'shift' must be constant in 0-31 range or in CL */
      : "%edx", "%cc"			/* clobbers EDX and condition codes */
	);

	return result;
}
#endif


/*-------------------------------------------------
    mod_64x32 - perform a signed 64 bit x 32 bit
    divide and return the 32 bit remainder
-------------------------------------------------*/

/* TBD */


/*-------------------------------------------------
    modu_64x32 - perform an unsigned 64 bit x 32 bit
    divide and return the 32 bit remainder
-------------------------------------------------*/

/* TBD */


/*-------------------------------------------------
    recip_approx - compute an approximate floating
    point reciprocal
-------------------------------------------------*/

/* TBD */



/***************************************************************************
    INLINE BIT MANIPULATION FUNCTIONS
***************************************************************************/

/*-------------------------------------------------
    count_leading_zeros - return the number of
    leading zero bits in a 32-bit value
-------------------------------------------------*/

#define count_leading_zeros _count_leading_zeros
INLINE UINT8 _count_leading_zeros(UINT32 value)
{
	UINT32 result;

	__asm__ (
		"bsrl  %1,%0;"
		"jnz   1f;"
		"movl  $63,%0;"
	  "1:xorl  $31,%0;"
	  : "=r,r" (result)			/* result can be in any register */
	  : "r,m" (value)			/* 'value' can be register or memory */
	  : "%cc"					/* clobbers condition codes */
	);

	return result;
}


/*-------------------------------------------------
    count_leading_ones - return the number of
    leading one bits in a 32-bit value
-------------------------------------------------*/

#define count_leading_ones _count_leading_ones
INLINE UINT8 _count_leading_ones(UINT32 value)
{
	UINT32 result;

	__asm__ (
		"movl  %1,%0;"
		"notl  %0;"
		"bsrl  %0,%0;"
		"jnz   1f;"
		"movl  $63,%0;"
	  "1:xorl  $31,%0;"
	  : "=r,r" (result)			/* result can be in any register */
	  : "r,m" (value)			/* 'value' can be register or memory */
	  : "%cc"					/* clobbers condition codes */
	);

	return result;
}



/***************************************************************************
    INLINE SYNCHRONIZATION FUNCTIONS
***************************************************************************/

/*-------------------------------------------------
    compare_exchange32 - compare the 'compare'
    value against the memory at 'ptr'; if equal,
    swap in the 'exchange' value. Regardless,
    return the previous value at 'ptr'.
-------------------------------------------------*/

#define compare_exchange32 _compare_exchange32
INLINE INT32 _compare_exchange32(INT32 volatile *ptr, INT32 compare, INT32 exchange)
{
	register INT32 result;

	__asm__ __volatile__ (
		" lock ; cmpxchg %[exchange], %[ptr] ;"
	  : [ptr]      "+m" (*ptr)
	  , [result]   "=a" (result)
	  : [compare]  "1"  (compare)
	  , [exchange] "q"  (exchange)
	  : "%cc"
	);

	return result;
}


/*-------------------------------------------------
    compare_exchange64 - compare the 'compare'
    value against the memory at 'ptr'; if equal,
    swap in the 'exchange' value. Regardless,
    return the previous value at 'ptr'.
-------------------------------------------------*/

#ifdef PTR64
#define compare_exchange64 _compare_exchange64
INLINE INT64 _compare_exchange64(INT64 volatile *ptr, INT64 compare, INT64 exchange)
{
	register INT64 result;

	__asm__ __volatile__ (
		" lock ; cmpxchg %[exchange], %[ptr] ;"
	  : [ptr]      "+m" (*ptr)
	  , [result]   "=a" (result)
	  : [compare]  "1"  (compare)
	  , [exchange] "q"  (exchange)
	  : "%cc"
	);

	return result;
}
#endif


/*-------------------------------------------------
    atomic_exchange32 - atomically exchange the
    exchange value with the memory at 'ptr',
    returning the original value.
-------------------------------------------------*/

#define atomic_exchange32 _atomic_exchange32
INLINE INT32 _atomic_exchange32(INT32 volatile *ptr, INT32 exchange)
{
	register INT32 result;

	__asm__ __volatile__ (
		" lock ; xchg %[exchange], %[ptr] ;"
		: [ptr]      "+m" (*ptr)
		, [result]   "=r" (result)
		: [exchange] "1"  (exchange)
	);

	return result;
}


/*-------------------------------------------------
    atomic_add32 - atomically add the delta value
    to the memory at 'ptr', returning the final
    result.
-------------------------------------------------*/

#define atomic_add32 _atomic_add32
INLINE INT32 _atomic_add32(INT32 volatile *ptr, INT32 delta)
{
	register INT32 result;

	__asm __volatile__ (
		" mov           %[delta],%[result] ;"
		" lock ; xadd   %[result],%[ptr]   ;"
		" add           %[delta],%[result] ;"
	  : [ptr]    "+m"  (*ptr)
	  , [result] "=&r" (result)
	  : [delta]  "rmi" (delta)
	  : "%cc"
	);

	return result;
}


#endif /* __EIGCCX86__ */

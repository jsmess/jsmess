/*
 * 74181
 *
 * 4-Bit Arithmetic Logic Unit
 *
 */

#if !defined( TTL74181_H )
#define TTL74181_H ( 1 )

#define TTL74181_MAX_CHIPS ( 2 )

#define TTL74181_INPUT_A0 ( 0 )
#define TTL74181_INPUT_A1 ( 1 )
#define TTL74181_INPUT_A2 ( 2 )
#define TTL74181_INPUT_A3 ( 3 )
#define TTL74181_INPUT_B0 ( 4 )
#define TTL74181_INPUT_B1 ( 5 )
#define TTL74181_INPUT_B2 ( 6 )
#define TTL74181_INPUT_B3 ( 7 )
#define TTL74181_INPUT_S0 ( 8 )
#define TTL74181_INPUT_S1 ( 9 )
#define TTL74181_INPUT_S2 ( 10 )
#define TTL74181_INPUT_S3 ( 11 )
#define TTL74181_INPUT_C ( 12 )
#define TTL74181_INPUT_M ( 13 )
#define TTL74181_INPUT_TOTAL ( 14 )

#define TTL74181_OUTPUT_F0 ( 0 )
#define TTL74181_OUTPUT_F1 ( 1 )
#define TTL74181_OUTPUT_F2 ( 2 )
#define TTL74181_OUTPUT_F3 ( 3 )
#define TTL74181_OUTPUT_AEQB ( 4 )
#define TTL74181_OUTPUT_P ( 5 )
#define TTL74181_OUTPUT_G ( 6 )
#define TTL74181_OUTPUT_CN4 ( 7 )
#define TTL74181_OUTPUT_TOTAL ( 8 )

extern void TTL74181_init( int chip );
extern void TTL74181_write( int chip, int startline, int lines, int data );
extern int TTL74181_read( int chip, int startline, int lines );

#endif

#ifndef __GAMECOM_H
#define __GAMECOM_H

#include "mame.h"
#include "sound/custom.h"

extern MACHINE_RESET( gamecom );
extern DEVICE_INIT( gamecom_cart );
extern DEVICE_LOAD( gamecom_cart );

extern WRITE8_HANDLER( gamecom_internal_w );
extern READ8_HANDLER( gamecom_internal_r );

extern WRITE8_HANDLER( gamecom_vram_w );
extern READ8_HANDLER( gamecom_vram_r );

extern void gamecom_handle_dma( int cycles );
extern void gamecom_update_timers( int cycles );

extern INTERRUPT_GEN( gamecom_scanline );

/* SM8521 register addresses */
#define SM8521_R0	0x00
#define SM8521_R1	0x01
#define SM8521_R2	0x02
#define SM8521_R3	0x03
#define SM8521_R4	0x04
#define SM8521_R5	0x05
#define SM8521_R6	0x06
#define SM8521_R7	0x07
#define SM8521_R8	0x08
#define SM8521_R9	0x09
#define SM8521_R10	0x0A
#define SM8521_R11	0x0B
#define SM8521_R12	0x0C
#define SM8521_R13	0x0D
#define SM8521_R14	0x0E
#define SM8521_R15	0x0F
#define SM8521_IE0	0x10
#define SM8521_IE1	0x11
#define SM8521_IR0	0x12
#define SM8521_IR1	0x13
#define SM8521_P0	0x14
#define SM8521_P1	0x15
#define SM8521_P2	0x16
#define SM8521_P3	0x17
#define SM8521_18	0x18 /* reserved */
#define SM8521_SYS	0x19
#define SM8521_CKC	0x1A
#define SM8521_1B	0x1B /* reserved */
#define SM8521_SPH	0x1C
#define SM8521_SPL	0x1D
#define SM8521_PS0	0x1E
#define SM8521_PS1	0x1F
#define SM8521_P0C	0x20
#define SM8521_P1C	0x21
#define SM8521_P2C	0x22
#define SM8521_P3C	0x23
#define SM8521_MMU0	0x24
#define SM8521_MMU1	0x25
#define SM8521_MMU2	0x26
#define SM8521_MMU3	0x27
#define SM8521_MMU4	0x28
#define SM8521_29	0x29 /* reserved */
#define SM8521_2A	0x2A /* reserved */
#define SM8521_URTT	0x2B
#define SM8521_URTR	0x2C
#define SM8521_URTS	0x2D
#define SM8521_URTC	0x2E
#define SM8521_2F	0x2F /* reserved */
#define SM8521_LCDC	0x30
#define SM8521_LCH	0x31
#define SM8521_LCV	0x32
#define SM8521_33	0x33 /* reserved */
#define SM8521_DMC	0x34
#define SM8521_DMX1	0x35
#define SM8521_DMY1	0x36
#define SM8521_DMDX	0x37
#define SM8521_DMDY	0x38
#define SM8521_DMX2	0x39
#define SM8521_DMY2	0x3A
#define SM8521_DMPL	0x3B
#define SM8521_DMBR	0x3C
#define SM8521_DMVP	0x3D
#define SM8521_3E	0x3E /* reserved */
#define SM8521_3F	0x3F /* reserved */
#define SM8521_SGC	0x40
#define SM8521_41	0x41 /* reserved */
#define SM8521_SG0L	0x42
#define SM8521_43	0x43 /* reserved */
#define SM8521_SG1L	0x44
#define SM8521_45	0x45 /* reserved */
#define SM8521_SG0TH	0x46
#define SM8521_SG0TL	0x47
#define SM8521_SG1TH	0x48
#define SM8521_SG1TL	0x49
#define SM8521_SG2L	0x4A
#define SM8521_4B	0x4B /* reserved */
#define SM8521_SG2TH	0x4C
#define SM8521_SG2TL	0x4D
#define SM8521_SGDA	0x4E
#define SM8521_4F	0x4F /* reserved */
#define SM8521_TM0C	0x50
#define SM8521_TM0D	0x51
#define SM8521_TM1C	0x52
#define SM8521_TM1D	0x53
#define SM8521_CLKT	0x54
#define SM8521_55	0x55 /* reserved */
#define SM8521_56	0x56 /* reserved */
#define SM8521_57	0x57 /* reserved */
#define SM8521_58	0x58 /* reserved */
#define SM8521_59	0x59 /* reserved */
#define SM8521_5A	0x5A /* reserved */
#define SM8521_5B	0x5B /* reserved */
#define SM8521_5C	0x5C /* reserved */
#define SM8521_5D	0x5D /* reserved */
#define SM8521_WDT	0x5E
#define SM8521_WDTC	0x5F
#define SM8521_SG0W0	0x60
#define SM8521_SG0W1	0x61
#define SM8521_SG0W2	0x62
#define SM8521_SG0W3	0x63
#define SM8521_SG0W4	0x64
#define SM8521_SG0W5	0x65
#define SM8521_SG0W6	0x66
#define SM8521_SG0W7	0x67
#define SM8521_SG0W8	0x68
#define SM8521_SG0W9	0x69
#define SM8521_SG0W10	0x6A
#define SM8521_SG0W11	0x6B
#define SM8521_SG0W12	0x6C
#define SM8521_SG0W13	0x6D
#define SM8521_SG0W14	0x6E
#define SM8521_SG0W15	0x6F
#define SM8521_SG1W0	0x70
#define SM8521_SG1W1	0x71
#define SM8521_SG1W2	0x72
#define SM8521_SG1W3	0x73
#define SM8521_SG1W4	0x74
#define SM8521_SG1W5	0x75
#define SM8521_SG1W6	0x76
#define SM8521_SG1W7	0x77
#define SM8521_SG1W8	0x78
#define SM8521_SG1W9	0x79
#define SM8521_SG1W10	0x7A
#define SM8521_SG1W11	0x7B
#define SM8521_SG1W12	0x7C
#define SM8521_SG1W13	0x7D
#define SM8521_SG1W14	0x7E
#define SM8521_SG1W15	0x7F

/* machine/gamecom.c */
extern UINT8 internal_registers[];
extern UINT8 gamecom_vram[];

#endif

/*
	For more information, please see:
	- http://cgfm2.emuviews.com/txt/msvdp.txt
	- http://www.smspower.org/forums/viewtopic.php?p=44198

A scanline contains the following sections:
  - horizontal sync     1  ED
  - left blanking       2  ED-EE
  - color burst        14  EE-EF
  - left blanking       8  F5-F9
  - left border        13  F9-FF
  - active display    256  00-7F
  - right border       15  80-87
  - right blanking      8  87-8B
  - horizontal sync    25  8B-97


NTSC frame timing
                       256x192         256x224        256x240 (doesn't work on real hardware)
  - vertical blanking   3  D5-D7        3  E5-E7       3  EE-F0
  - top blanking       13  D8-E4       13  E8-F4      13  F1-FD
  - top border         27  E5-FF       11  F5-FF       3  FD-FF
  - active display    192  00-BF      224  00-DF     240  00-EF
  - bottom border      24  C0-D7        8  E0-E7       0  F0-F0
  - bottom blanking     3  D8-DA        3  E8-EA       3  F0-F2


PAL frame timing
                       256x192         256x224        256x240
  - vertical blanking   3  BA-BC        3  CA-CC       3  D2-D4
  - top blanking       13  BD-C9       13  CD-D9      13  D5-E1
  - top border         54  CA-FF       38  DA-FF      30  E2-FF
  - active display    192  00-BF      224  00-DF     240  00-EF
  - bottom border      48  C0-EF       32  E0-FF      24  F0-07
  - bottom blanking     3  F0-F2        3  00-02       3  08-0A

*/

#include "driver.h"
#include "includes/sms.h"
#include "video/generic.h"
#include "cpu/z80/z80.h"

#define CRAM_SIZE		MAX_CRAM_SIZE
#define PRIORITY_BIT		0x1000

UINT8 reg[NUM_OF_REGISTER];
UINT8 *VRAM = NULL;
UINT8 *CRAM = NULL;
int CRAMMask;
UINT8 *lineCollisionBuffer;
UINT8 reg9copy;

int addr;
int code;
int pending;
int latch;
int buffer;
int statusReg;
int ggSmsMode;

int isCRAMDirty;

int currentLine;
int lineCountDownCounter;
int irqState;			/* The status of the IRQ line, as seen by the VDP */
int y_pixels;			/* 192, 224, 240 */
int start_blanking;		/* when is the transition from bottom border area to blanking area */
int start_top_border;		/* when is the transition from blanking area to top border area */
int max_y_pixels;		/* full range of y counter */
int vdp_mode;			/* current mode of the VDP: 0,1,2,3,4 */
int bborder_192_y_pixels, bborder_224_y_pixels, bborder_240_y_pixels;

mame_bitmap *prevBitMap;

/* lineBuffer will be used to hold 5 lines of line data. Line #0 is the regular blitting area.
   Lines #1-#4 will be used as a kind of cache to be used for vertical scaling in the gamegear
   sms compatibility mode.
 */
int *lineBuffer = NULL;
int currentPalette[32];
int prevBitMapSaved;

UINT8 *vcnt_lookup;
UINT8 *vcnt_192_lookup, *vcnt_224_lookup, *vcnt_240_lookup;

/* NTSC 192 lines precalculated return values from the V counter */
static UINT8 vcnt_ntsc_192[NTSC_Y_PIXELS] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
	0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
	0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
	0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
	0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
	0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
	0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
	0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,
	0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
	0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
	0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
	0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA,
																0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
	0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
	0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF
};

/* NTSC 224 lines precalculated return values from the V counter */
static UINT8 vcnt_ntsc_224[NTSC_Y_PIXELS] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
	0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
	0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
	0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
	0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
	0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
	0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
	0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,
	0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
	0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
	0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
	0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
	0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA,
																0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
	0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF
};

/* NTSC 240 lines precalculated return values from the V counter */
static UINT8 vcnt_ntsc_240[NTSC_Y_PIXELS] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
	0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
	0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
	0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
	0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
	0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
	0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
	0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,
	0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
	0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
	0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
	0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
	0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
	0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF,
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05
};

/* PAL 192 lines precalculated return values from the V counter */
static UINT8 vcnt_pal_192[PAL_Y_PIXELS] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
	0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
	0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
	0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
	0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
	0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
	0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
	0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,
	0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
	0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
	0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
	0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
	0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
	0xF0, 0xF1, 0xF2,																						0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
	0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
	0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
	0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
	0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF
};

/* PAL 224 lines precalculated return values from the V counter */
static UINT8 vcnt_pal_224[PAL_Y_PIXELS] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
	0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
	0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
	0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
	0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
	0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
	0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
	0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,
	0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
	0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
	0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
	0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
	0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
	0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF,
	0x00, 0x01, 0x02,																						0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
	0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
	0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
	0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF
};

/* PAL 240 lines precalculated return values from the V counter */
static UINT8 vcnt_pal_240[PAL_Y_PIXELS] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
	0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
	0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
	0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
	0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
	0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
	0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
	0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,
	0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
	0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
	0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
	0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
	0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
	0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF,
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A,
							0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
	0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
	0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF
};

static void set_display_settings( void ) {
	int M1, M2, M3, M4;
	M1 = reg[0x01] & 0x10;
	M2 = reg[0x00] & 0x02;
	M3 = reg[0x01] & 0x08;
	M4 = reg[0x00] & 0x04;
	y_pixels = 192;
	if ( M4 ) {
		/* mode 4 */
		vdp_mode = 4;
		if ( M2 ) {
			/* Is it 224-line display */
			if ( M1 && ! M3 ) {
				y_pixels = 224;
			} else if ( ! M1 && M3 ) {
				/* 240-line display */
				y_pixels = 240;
			}
		}
	} else {
		/* original TMS9918 mode */
		if ( ! M1 && ! M2 && ! M3 ) {
			vdp_mode = 0;
		} else
//		if ( M1 && ! M2 && ! M3 ) {
//			vdp_mode = 1;
//		} else
		if ( ! M1 && M2 && ! M3 ) {
			vdp_mode = 2;
//		} else
//		if ( ! M1 && ! M2 && M3 ) {
//			vdp_mode = 3;
		} else {
			logerror( "Unknown video mode detected (M1=%c, M2=%c, M3=%c, M4=%c)\n", M1 ? '1' : '0', M2 ? '1' : '0', M3 ? '1' : '0', M4 ? '1' : '0');
		}
	}
	switch( y_pixels ) {
	case 192:
		vcnt_lookup = vcnt_192_lookup;
		start_blanking = y_pixels + bborder_192_y_pixels;
		break;
	case 224:
		vcnt_lookup = vcnt_224_lookup;
		start_blanking = y_pixels + bborder_224_y_pixels;
		break;
	case 240:
		vcnt_lookup = vcnt_240_lookup;
		start_blanking = y_pixels + bborder_240_y_pixels;
		break;
	}
	start_top_border = start_blanking + 19;
	if ( IS_GAMEGEAR ) {
		if ( vdp_mode == 4 && y_pixels != 192 ) {
			video_screen_set_visarea( 0, 6*8, 26*8 - 1, 5*8, 23*8 - 1 );
		} else {
			video_screen_set_visarea( 0, 6*8, 26*8 - 1, 3*8, 21*8 - 1 );
		}
	} else {
		video_screen_set_visarea( 0, LBORDER_X_PIXELS, LBORDER_X_PIXELS + 255, TBORDER_Y_PIXELS, TBORDER_Y_PIXELS + y_pixels - 1 );
	}
	isCRAMDirty = 1;
}

 READ8_HANDLER(sms_vdp_curline_r) {
	return vcnt_lookup[currentLine];
}

void sms_set_ggsmsmode( int mode ) {
	ggSmsMode = mode;
}

int sms_video_init( int max_lines, int bborder_192, int bborder_224, int bborder_240, UINT8 *vcnt_192, UINT8 *vcnt_224, UINT8 *vcnt_240 ) {

	max_y_pixels = max_lines;
	bborder_192_y_pixels = bborder_192;
	bborder_224_y_pixels = bborder_224;
	bborder_240_y_pixels = bborder_240;
	vcnt_192_lookup = vcnt_192;
	vcnt_224_lookup = vcnt_224;
	vcnt_240_lookup = vcnt_240;

	/* Allocate video RAM
	   In theory the driver could have a REGION_GFX1 and/or REGION_GFX2 memory region
	   of it's own. So this code could potentially cause a clash.
	*/
	if ( VRAM == NULL ) {
		VRAM = new_memory_region( Machine, REGION_GFX1, VRAM_SIZE, ROM_REQUIRED );
	}
	if ( CRAM == NULL ) {
		CRAM = new_memory_region( Machine, REGION_GFX2, CRAM_SIZE, ROM_REQUIRED );
	}
	if ( lineBuffer == NULL ) {
		lineBuffer = auto_malloc( 256 * 5 * sizeof(int) );
		memset( lineBuffer, 0, 256 * 5 * sizeof(int) );
	}

	/* Clear RAM */
	memset(reg, 0, NUM_OF_REGISTER);
	isCRAMDirty = 1;
	memset(VRAM, 0, VRAM_SIZE);
	memset(CRAM, 0, CRAM_SIZE);
	reg[0x02] = 0x0E;			/* power up default */

	CRAMMask = ( IS_GAMEGEAR && ! ggSmsMode ) ? ( GG_CRAM_SIZE - 1 ) : ( SMS_CRAM_SIZE - 1 );

	/* Initialize VDP state variables */
	addr = code = pending = latch = buffer = statusReg = \
	currentLine = lineCountDownCounter = irqState = 0;

	lineCollisionBuffer = auto_malloc(MAX_X_PIXELS);

	/* Make temp bitmap for rendering */
	tmpbitmap = auto_bitmap_alloc(Machine->screen[0].width, Machine->screen[0].height, BITMAP_FORMAT_INDEXED32);

	prevBitMapSaved = 0;
	prevBitMap = auto_bitmap_alloc(Machine->screen[0].width, Machine->screen[0].height, BITMAP_FORMAT_INDEXED32);

	set_display_settings();

	return (0);
}

VIDEO_START(sms_pal) {
	return sms_video_init( PAL_Y_PIXELS, PAL_192_BBORDER_Y_PIXELS, PAL_224_BBORDER_Y_PIXELS, PAL_240_BBORDER_Y_PIXELS, vcnt_pal_192, vcnt_pal_224, vcnt_pal_240 );
}

VIDEO_START(sms_ntsc) {
	return sms_video_init( NTSC_Y_PIXELS, NTSC_192_BBORDER_Y_PIXELS, NTSC_224_BBORDER_Y_PIXELS, 1, vcnt_ntsc_192, vcnt_ntsc_224, vcnt_ntsc_240 );
}

INTERRUPT_GEN(sms) {
	/* Bump scanline counter */
	currentLine = (currentLine + 1) % max_y_pixels;

	if (currentLine <= y_pixels) {
		/* We start a new frame, so reset line count down counter */
		if (currentLine == 0x00) {
			lineCountDownCounter = reg[0x0A];
			reg9copy = reg[0x09];
		}

		if (lineCountDownCounter == 0x00) {
			lineCountDownCounter = reg[0x0A];
			statusReg |= STATUS_HINT;
		} else {
			lineCountDownCounter -= 1;
		}

		if ((statusReg & STATUS_HINT) && (reg[0x00] & 0x10)) {
			irqState = 1;
			cpunum_set_input_line(0, 0, ASSERT_LINE);
		}

		if ( currentLine == 112 ) {
			check_pause_button();
		}
	} else {
		if ( currentLine == y_pixels + 1 ) {
			statusReg |= STATUS_VINT;
		}

		lineCountDownCounter = reg[0x0A];

		if ((statusReg & STATUS_VINT) && (reg[0x01] & 0x20)) {
			irqState = 1;
			cpunum_set_input_line(0, 0, ASSERT_LINE);
		}
	}

	if (!video_skip_this_frame()) {
#ifdef LOG_CURLINE
		logerror("l %04x, pc: %04x\n", currentLine, activecpu_get_pc());
#endif
		if (currentLine < max_y_pixels) {
			sms_update_palette();
			sms_refresh_line(tmpbitmap, currentLine);
		}
	}
}

 READ8_HANDLER(sms_vdp_data_r) {
	int temp;

	/* SMS 2 & GG behaviour. Seems like the latched data is passed straight through */
	/* to the address register when in the middle of doing a command.               */
	/* Cosmic Spacehead needs this, among others                                    */
	if ( pending ) {
		addr = ( addr & 0xff00 ) | latch;
		/* Clear pending write flag */
		pending = 0;
	}

	/* Return read buffer contents */
	temp = buffer;

	/* Load read buffer */
	buffer = VRAM[(addr & 0x3FFF)];

	/* Bump internal address register */
	addr += 1;
	return (temp);
}

 READ8_HANDLER(sms_vdp_ctrl_r) {
	int temp = statusReg;

	/* Clear pending write flag */
	pending = 0;

	statusReg &= ~( STATUS_VINT | STATUS_SPROVR | STATUS_SPRCOL | STATUS_HINT );

	if (irqState == 1) {
		irqState = 0;
		cpunum_set_input_line(0, 0, CLEAR_LINE);
	}

	return (temp);
}

WRITE8_HANDLER(sms_vdp_data_w) {
	/* SMS 2 & GG behaviour. Seems like the latched data is passed straight through */
	/* to the address register when in the middle of doing a command.               */
        /* Cosmic Spacehead needs this, among others                                    */
	if ( pending ) {
		addr = ( addr & 0xff00 ) | latch;
		/* Clear pending write flag */
		pending = 0;
	}

	switch(code) {
		case 0x00:
		case 0x01:
		case 0x02: {
			int address = (addr & 0x3FFF);

			VRAM[address] = data;
		}
		break;
		case 0x03: {
			int address = addr & CRAMMask;

			if (data != CRAM[address]) {
				CRAM[address] = data;
				isCRAMDirty = 1;
			}
		}
		break;
	}

	//buffer = data;
	addr += 1;
}

WRITE8_HANDLER(sms_vdp_ctrl_w) {
	int regNum;

	if (pending == 0) {
		latch = data;
		pending = 1;
	} else {
		/* Clear pending write flag */
		pending = 0;

		code = (data >> 6) & 0x03;
		switch( code ) {
		case 0:		/* VRAM reading mode */
			addr = ( data << 8 ) | latch;
			buffer = VRAM[ addr & 0x3FFF ];
			addr += 1;
			break;
		case 1:		/* VRAM writing mode */
			addr = ( data << 8 ) | latch;
			break;
		case 2:		/* VDP register write */
			regNum = data & 0x0F;
			reg[regNum] = latch;
			if (regNum == 0 && latch & 0x02) {
				logerror("overscan enabled.\n");
			}
			if ( regNum == 0 || regNum == 1 ) {
				set_display_settings();
			}
			code = 0;
			break;
		case 3:		/* CRAM writing mode */
			addr = ( data << 8 ) | latch;
			break;
		}
	}
}

void sms_refresh_line_mode4(int *lineBuffer, int line) {
	int tileColumn;
	int xScroll, yScroll, xScrollStartColumn;
	int spriteIndex;
	int pixelX, pixelPlotX, prioritySelected[256];
	int spriteX, spriteY, spriteLine, spriteTileSelected, spriteHeight, spriteZoom;
	int spriteBuffer[8], spriteBufferCount, spriteBufferIndex;
	int bitPlane0, bitPlane1, bitPlane2, bitPlane3;
	UINT16 *nameTable = (UINT16 *) &(VRAM[(((reg[0x02] & 0x0E) << 10) & 0x3800) + ((((line + reg9copy) % 224) >> 3) << 6)]);
	UINT8 *spriteTable = (UINT8 *) &(VRAM[(reg[0x05] << 7) & 0x3F00]);

	if ( y_pixels != 192 ) {
		nameTable = (UINT16 *) &(VRAM[(((reg[0x02] & 0x0C) << 10) | 0x0700) + ((((line + reg9copy) % 256) >> 3 ) << 6)]);
	}

	/* if top 2 rows of screen not affected by horizontal scrolling, then xScroll = 0 */
	/* else xScroll = reg[0x08] (SMS Only)                                            */
	if ( IS_GAMEGEAR ) {
		xScroll = 0x0100 - reg[0x08];
	} else {
		xScroll = (((reg[0x00] & 0x40) && (line < 16)) ? 0 : 0x0100 - reg[0x08]);
	}

	xScrollStartColumn = (xScroll >> 3);							 /* x starting column tile */

	/* Draw background layer */
	for (tileColumn = 0; tileColumn < 33; tileColumn++) {
		UINT16 tileData;
		int tileSelected, palletteSelected, horizSelected, vertSelected, prioritySelect;
		int tileLine;

		/* Rightmost 8 columns for SMS (or 2 columns for GG) not affected by */
		/* vertical scrolling when bit 7 of reg[0x00] is set */
		if ( IS_GAMEGEAR ) {
			yScroll = (((reg[0x00] & 0x80) && (tileColumn > 29)) ? 0 : (reg9copy) % 224);
		} else {
			yScroll = (((reg[0x00] & 0x80) && (tileColumn > 23)) ? 0 : (reg9copy) % 224);
		}

		#ifndef LSB_FIRST
		tileData = ((nameTable[((tileColumn + xScrollStartColumn) & 0x1F)] & 0xFF) << 8) | (nameTable[((tileColumn + xScrollStartColumn) & 0x1F)] >> 8);
		#else
		tileData = nameTable[((tileColumn + xScrollStartColumn) & 0x1F)];
		#endif

		tileSelected = (tileData & 0x01FF);
		prioritySelect = tileData & PRIORITY_BIT;
		palletteSelected = (tileData >> 11) & 0x01;
		vertSelected = (tileData >> 10) & 0x01;
		horizSelected = (tileData >> 9) & 0x01;

		tileLine = line - ((0x07 - (yScroll & 0x07)) + 1);
		if (vertSelected) {
			tileLine = 0x07 - tileLine;
		}
		bitPlane0 = VRAM[((tileSelected << 5) + ((tileLine & 0x07) << 2)) + 0x00];
		bitPlane1 = VRAM[((tileSelected << 5) + ((tileLine & 0x07) << 2)) + 0x01];
		bitPlane2 = VRAM[((tileSelected << 5) + ((tileLine & 0x07) << 2)) + 0x02];
		bitPlane3 = VRAM[((tileSelected << 5) + ((tileLine & 0x07) << 2)) + 0x03];

		for (pixelX = 0; pixelX < 8 ; pixelX++) {
			UINT8 penBit0, penBit1, penBit2, penBit3;
			UINT8 penSelected;

			penBit0 = (bitPlane0 >> (7 - pixelX)) & 0x01;
			penBit1 = (bitPlane1 >> (7 - pixelX)) & 0x01;
			penBit2 = (bitPlane2 >> (7 - pixelX)) & 0x01;
			penBit3 = (bitPlane3 >> (7 - pixelX)) & 0x01;

			penSelected = (penBit3 << 3 | penBit2 << 2 | penBit1 << 1 | penBit0);
			if (palletteSelected) {
				penSelected |= 0x10;
			}

			if (!horizSelected) {
				pixelPlotX = pixelX;
			} else {
				pixelPlotX = 7 - pixelX;
			}
			pixelPlotX = (0 - (xScroll & 0x07) + (tileColumn << 3) + pixelPlotX);
			if (pixelPlotX >= 0 && pixelPlotX < 256) {
//				logerror("%x %x\n", pixelPlotX + pixelOffsetX, pixelPlotY);
				lineBuffer[pixelPlotX] = currentPalette[penSelected];
				prioritySelected[pixelPlotX] = prioritySelect | ( penSelected & 0x0F );
			}
		}
	}

	/* Draw sprite layer */
	spriteHeight = (reg[0x01] & 0x02 ? 16 : 8);
	spriteZoom = 1;
	if (reg[0x01] & 0x01) {
		/* sprite doubling */
		spriteZoom = 2;
	}
	spriteBufferCount = 0;
	for (spriteIndex = 0; (spriteIndex < 64) && (spriteTable[spriteIndex] != 0xD0 || y_pixels != 192) && (spriteBufferCount < 9); spriteIndex++) {
		spriteY = spriteTable[spriteIndex] + 1; /* sprite y position starts at line 1 */
		if (spriteY > 240) {
			spriteY -= 256; /* wrap from top if y position is > 240 */
		}
		if ((line >= spriteY) && (line < (spriteY + spriteHeight * spriteZoom))) {
			if (spriteBufferCount < 8) {
				spriteBuffer[spriteBufferCount] = spriteIndex;
			} else {
				/* Too many sprites per line */
				statusReg |= STATUS_SPROVR;
			}
			spriteBufferCount++;
		}
	}
	if ( spriteBufferCount > 8 ) {
		spriteBufferCount = 8;
	}
	memset(lineCollisionBuffer, 0, MAX_X_PIXELS);
	spriteBufferCount--;
	for (spriteBufferIndex = spriteBufferCount; spriteBufferIndex >= 0; spriteBufferIndex--) {
		spriteIndex = spriteBuffer[spriteBufferIndex];
		spriteY = spriteTable[spriteIndex] + 1; /* sprite y position starts at line 1 */
		if (spriteY > 240) {
			spriteY -= 256; /* wrap from top if y position is > 240 */
		}

		spriteX = spriteTable[0x80 + (spriteIndex << 1)];
		if (reg[0x00] & 0x08) {
			spriteX -= 0x08;	 /* sprite shift */
		}
		spriteTileSelected = spriteTable[0x81 + (spriteIndex << 1)];
		if (reg[0x06] & 0x04) {
			spriteTileSelected += 256; /* pattern table select */
		}
		if (reg[0x01] & 0x02) {
			spriteTileSelected &= 0x01FE; /* force even index */
		}
		spriteLine = ( line - spriteY ) / spriteZoom;

		if (spriteLine > 0x07) {
			spriteTileSelected += 1;
		}

		bitPlane0 = VRAM[((spriteTileSelected << 5) + ((spriteLine & 0x07) << 2)) + 0x00];
		bitPlane1 = VRAM[((spriteTileSelected << 5) + ((spriteLine & 0x07) << 2)) + 0x01];
		bitPlane2 = VRAM[((spriteTileSelected << 5) + ((spriteLine & 0x07) << 2)) + 0x02];
		bitPlane3 = VRAM[((spriteTileSelected << 5) + ((spriteLine & 0x07) << 2)) + 0x03];

		for (pixelX = 0; pixelX < 8 ; pixelX++) {
			UINT8 penBit0, penBit1, penBit2, penBit3;
			int penSelected;

			penBit0 = (bitPlane0 >> (7 - pixelX)) & 0x01;
			penBit1 = (bitPlane1 >> (7 - pixelX)) & 0x01;
			penBit2 = (bitPlane2 >> (7 - pixelX)) & 0x01;
			penBit3 = (bitPlane3 >> (7 - pixelX)) & 0x01;

			penSelected = (penBit3 << 3 | penBit2 << 2 | penBit1 << 1 | penBit0) | 0x10;

			if ( penSelected == 0x10 ) {	/* Transparent palette so skip draw */
				continue;
			}

			if (reg[0x01] & 0x01) {
				/* sprite doubling is enabled */
				pixelPlotX = spriteX + (pixelX << 1);

				/* check to prevent going outside of active display area */
				if ( pixelPlotX > 256 ) {
					continue;
				}

				if ( ! ( prioritySelected[pixelPlotX] & PRIORITY_BIT ) ) {
					lineBuffer[pixelPlotX] = currentPalette[penSelected];
					lineBuffer[pixelPlotX+1] = currentPalette[penSelected];
				} else {
					if ( prioritySelected[pixelPlotX] == PRIORITY_BIT ) {
						lineBuffer[pixelPlotX] = currentPalette[penSelected];
					}
					if ( prioritySelected[pixelPlotX + 1] == PRIORITY_BIT ) {
						lineBuffer[pixelPlotX+1] = currentPalette[penSelected];
					}
				}
				if (lineCollisionBuffer[pixelPlotX] != 1) {
					lineCollisionBuffer[pixelPlotX] = 1;
				} else {
					/* sprite collision occurred */
					statusReg |= STATUS_SPRCOL;
				}
				if (lineCollisionBuffer[pixelPlotX + 1] != 1) {
					lineCollisionBuffer[pixelPlotX + 1] = 1;
				} else {
					/* sprite collision occurred */
					statusReg |= STATUS_SPRCOL;
				}
			} else {
				pixelPlotX = spriteX + pixelX;

				/* check to prevent going outside of active display area */
				if ( pixelPlotX > 256 ) {
					continue;
				}

				if ( ! ( prioritySelected[pixelPlotX] & PRIORITY_BIT ) ) {
					lineBuffer[pixelPlotX] = currentPalette[penSelected];
				} else {
					if ( prioritySelected[pixelPlotX] == PRIORITY_BIT ) {
						lineBuffer[pixelPlotX] = currentPalette[penSelected];
					}
				}
				if (lineCollisionBuffer[pixelPlotX] != 1) {
					lineCollisionBuffer[pixelPlotX] = 1;
				} else {
					/* sprite collision occurred */
					statusReg |= STATUS_SPRCOL;
				}
			}
		}
	}

	/* Fill column 0 with overscan color from reg[0x07]	 (SMS Only) */
	if ( ! IS_GAMEGEAR ) {
		if (reg[0x00] & 0x20) {
			lineBuffer[0] = currentPalette[BACKDROP_COLOR];
			lineBuffer[1] = currentPalette[BACKDROP_COLOR];
			lineBuffer[2] = currentPalette[BACKDROP_COLOR];
			lineBuffer[3] = currentPalette[BACKDROP_COLOR];
			lineBuffer[4] = currentPalette[BACKDROP_COLOR];
			lineBuffer[5] = currentPalette[BACKDROP_COLOR];
			lineBuffer[6] = currentPalette[BACKDROP_COLOR];
			lineBuffer[7] = currentPalette[BACKDROP_COLOR];
		}
	}
}

void sms_refresh_line_mode2(int *lineBuffer, int line) {
	int tileColumn;
	int pixelX, pixelPlotX;
	int spriteHeight, spriteBufferCount, spriteIndex, spriteY, spriteBuffer[4], spriteBufferIndex;
	UINT8 *nameTable, *colorTable, *patternTable, *spriteTable, *spritePatternTable;
	int patternMask, colorMask, patternOffset;

	/* Draw background layer */
	nameTable = VRAM + ( ( reg[0x02] & 0x0F ) << 10 ) + ( ( line >> 3 ) * 32 );
	colorTable = VRAM + ( ( reg[0x03] & 0x80 ) << 6 );
	colorMask = ( ( reg[0x03] & 0x7F ) << 3 ) | 0x07;
	patternTable = VRAM + ( ( reg[0x04] & 0x04 ) << 11 );
	patternMask = ( ( reg[0x04] & 0x03 ) << 8 ) | 0xFF;
	patternOffset = ( line & 0xC0 ) << 2;
	for ( tileColumn = 0; tileColumn < 32; tileColumn++ ) {
		UINT8 pattern;
		UINT8 colors;
		pattern = patternTable[ ( ( ( patternOffset + nameTable[tileColumn] ) & patternMask ) * 8 ) + ( line & 0x07 ) ];
		colors = colorTable[ ( ( ( patternOffset + nameTable[tileColumn] ) & colorMask ) * 8 ) + ( line & 0x07 ) ];
		for ( pixelX = 0; pixelX < 8; pixelX++ ) {
			UINT8 penSelected;
			if ( pattern & ( 1 << ( 7 - pixelX ) ) ) {
				penSelected = colors >> 4;
			} else {
				penSelected = colors & 0x0F;
			} 
			if ( ! penSelected ) {
				penSelected = BACKDROP_COLOR;
			}
			pixelPlotX = ( tileColumn << 3 ) + pixelX;
			if ( IS_GAMEGEAR ) {
				penSelected |= 0x10;
			}
			lineBuffer[pixelPlotX] = currentPalette[penSelected];
		}
	}

	/* Draw sprite layer */
	spriteTable = VRAM + ( ( reg[0x05] & 0x7F ) << 7 );
	spritePatternTable = VRAM + ( ( reg[0x06] & 0x07 ) << 11 );
	spriteHeight = ( reg[0x01] & 0x03 ? 16 : 8 ); /* check if either MAG or SI is set */
	spriteBufferCount = 0;
	for ( spriteIndex = 0; (spriteIndex < 32*4 ) && ( spriteTable[spriteIndex] != 0xD0 ) && ( spriteBufferCount < 5); spriteIndex+= 4 ) {
		spriteY = spriteTable[spriteIndex] + 1;
		if ( spriteY > 240 ) {
			spriteY -= 256;
		}
		if ( ( line >= spriteY ) && ( line < ( spriteY + spriteHeight ) ) ) {
			if ( spriteBufferCount < 5 ) {
				spriteBuffer[spriteBufferCount] = spriteIndex;
			} else {
				/* Too many sprites per line */
				statusReg |= STATUS_SPROVR;
			}
			spriteBufferCount++;
		}
	}
	if ( spriteBufferCount > 4 ) {
		spriteBufferCount = 4;
	}
	memset( lineCollisionBuffer, 0, MAX_X_PIXELS );
	spriteBufferCount--;
	for ( spriteBufferIndex = spriteBufferCount; spriteBufferIndex >= 0; spriteBufferIndex-- ) {
		UINT8 penSelected;
		int spriteLine, pixelX, spriteX, spriteTileSelected;
		UINT8 pattern;
		spriteIndex = spriteBuffer[ spriteBufferIndex ];
		spriteY = spriteTable[ spriteIndex ] + 1;
		if ( spriteY > 240 ) {
			spriteY -= 256;
		}
		spriteX = spriteTable[ spriteIndex + 1 ];
		penSelected = spriteTable[ spriteIndex + 3 ] & 0x0F;
		if ( IS_GAMEGEAR ) {
			penSelected |= 0x10;
		}
		if ( spriteTable[ spriteIndex + 3 ] & 0x80 ) {
			spriteX -= 32;
		}
		spriteTileSelected = spriteTable[ spriteIndex + 2 ];
		spriteLine = line - spriteY;
		if ( reg[0x01] & 0x01 ) {
			spriteLine >>= 1;
		}
		if ( reg[0x01] & 0x02 ) {
			spriteTileSelected &= 0xFC;
			if ( spriteLine > 0x07 ) {
				spriteTileSelected += 1;
				spriteLine -= 8;
			}
		}
		pattern = spritePatternTable[ spriteTileSelected * 8 + spriteLine ];
		for ( pixelX = 0; pixelX < 8; pixelX++ ) {
			if ( reg[0x01] & 0x01 ) {
				pixelPlotX = spriteX + pixelX * 2;
				if ( pixelPlotX < 0 || pixelPlotX > 255 ) {
					continue;
				}
				if ( penSelected && ( pattern & ( 1 << ( 7 - pixelX ) ) ) ) {
					lineBuffer[pixelPlotX] = currentPalette[penSelected];
					if ( lineCollisionBuffer[pixelPlotX] != 1 ) {
						lineCollisionBuffer[pixelPlotX] = 1;
					} else {
						statusReg |= STATUS_SPRCOL;
					}
					lineBuffer[pixelPlotX+1] = currentPalette[penSelected];
					if ( lineCollisionBuffer[pixelPlotX + 1] != 1 ) {
						lineCollisionBuffer[pixelPlotX + 1] = 1;
					} else {
						statusReg |= STATUS_SPRCOL;
					}
				}
			} else {
				pixelPlotX = spriteX + pixelX;
				if ( pixelPlotX < 0 || pixelPlotX > 255 ) {
					continue;
				}
				if ( penSelected && ( pattern & ( 1 << ( 7 - pixelX ) ) ) ) {
					lineBuffer[pixelPlotX] = currentPalette[penSelected];
					if ( lineCollisionBuffer[pixelPlotX] != 1 ) {
						lineCollisionBuffer[pixelPlotX] = 1;
					} else {
						statusReg |= STATUS_SPRCOL;
					}
				}
			}
		}
		if ( reg[0x01] & 0x02 ) {
			spriteTileSelected += 2;
			pattern = spritePatternTable[ spriteTileSelected * 8 + spriteLine ];
			spriteX += 8;
			for ( pixelX = 0; pixelX < 8; pixelX++ ) {
				pixelPlotX = spriteX + pixelX;
				if ( pixelPlotX < 0 || pixelPlotX > 255 ) {
					continue;
				}
				if ( penSelected && ( pattern & ( 1 << ( 7 - pixelX ) ) ) ) {
					lineBuffer[pixelPlotX] = currentPalette[penSelected];
					if ( lineCollisionBuffer[pixelPlotX] != 1 ) {
						lineCollisionBuffer[pixelPlotX] = 1;
					} else {
						statusReg |= STATUS_SPRCOL;
					}
				}
			}
		}
	}
}

void sms_refresh_line_mode0(int *lineBuffer, int line) {
	int tileColumn;
	int pixelX, pixelPlotX; 
	int spriteHeight, spriteBufferCount, spriteIndex, spriteBuffer[4], spriteBufferIndex;
	UINT8 *nameTable, *colorTable, *patternTable, *spriteTable, *spritePatternTable;

	/* Draw background layer */
	nameTable = VRAM + ( ( reg[0x02] & 0x0F ) << 10 ) + ( ( line >> 3 ) * 32 );
	colorTable = VRAM + ( ( reg[0x03] << 6 ) & ( VRAM_SIZE - 1 ) );
	patternTable = VRAM + ( ( reg[0x04] << 11 ) & ( VRAM_SIZE - 1 ) ); 
	for ( tileColumn = 0; tileColumn < 32; tileColumn++ ) {
		UINT8 pattern;
		UINT8 colors;
		pattern = patternTable[ ( nameTable[tileColumn] * 8 ) + ( line & 0x07 ) ];
		colors = colorTable[ nameTable[tileColumn] >> 3 ];
		for ( pixelX = 0; pixelX < 8; pixelX++ ) {
			int penSelected;
			if ( pattern & ( 1 << ( 7 - pixelX ) ) ) {
				penSelected = colors >> 4;
			} else {
				penSelected = colors & 0x0F;
			}
			if ( IS_GAMEGEAR ) {
				penSelected |= 0x10;
			}
			pixelPlotX = ( tileColumn << 3 ) + pixelX;
			lineBuffer[pixelPlotX] = currentPalette[penSelected];
		}
	}

	/* Draw sprite layer */
	spriteTable = VRAM + ( ( reg[0x05] & 0x7F ) << 7 );
	spritePatternTable = VRAM + ( ( reg[0x06] & 0x07 ) << 11 );
	spriteHeight = 8;
	if ( reg[0x01] & 0x02 )				/* Check if SI is set */
		spriteHeight = spriteHeight * 2;
	if ( reg[0x01] & 0x01 )				/* Check if MAG is set */
		spriteHeight = spriteHeight * 2;
	spriteBufferCount = 0;
	for ( spriteIndex = 0; (spriteIndex < 32*4 ) && ( spriteTable[spriteIndex] != 0xD0 ) && ( spriteBufferCount < 5); spriteIndex+= 4 ) {
		int spriteY = spriteTable[spriteIndex] + 1;
		if ( spriteY > 240 ) {
			spriteY -= 256;
		}
		if ( ( line >= spriteY ) && ( line < ( spriteY + spriteHeight ) ) ) {
			if ( spriteBufferCount < 5 ) {
				spriteBuffer[spriteBufferCount] = spriteIndex;
			} else {
				/* Too many sprites per line */
				statusReg |= STATUS_SPROVR;
			}
			spriteBufferCount++;
		}
	}
	if ( spriteBufferCount > 4 ) {
		spriteBufferCount = 4;
	}
	memset( lineCollisionBuffer, 0, MAX_X_PIXELS );
	spriteBufferCount--;
	for ( spriteBufferIndex = spriteBufferCount; spriteBufferIndex >= 0; spriteBufferIndex-- ) {
		int penSelected;
		int spriteLine, pixelX, spriteX, spriteTileSelected;
		int spriteY;
		UINT8 pattern;
		spriteIndex = spriteBuffer[ spriteBufferIndex ];
		spriteY = spriteTable[ spriteIndex ] + 1;
		if ( spriteY > 240 ) {
			spriteY -= 256;
		}
		spriteX = spriteTable[ spriteIndex + 1 ];
		penSelected = spriteTable[ spriteIndex + 3 ] & 0x0F;
		if ( IS_GAMEGEAR ) {
			penSelected |= 0x10;
		}
		if ( spriteTable[ spriteIndex + 3 ] & 0x80 ) {
			spriteX -= 32;
		}
		spriteTileSelected = spriteTable[ spriteIndex + 2 ];
		spriteLine = line - spriteY;
		if ( reg[0x01] & 0x01 ) {
			spriteLine >>= 1;
		}
		if ( reg[0x01] & 0x02 ) {
			spriteTileSelected &= 0xFC;
			if ( spriteLine > 0x07 ) {
				spriteTileSelected += 1;
				spriteLine -= 8;
			}
		}
		pattern = spritePatternTable[ spriteTileSelected * 8 + spriteLine ];

		for ( pixelX = 0; pixelX < 8; pixelX++ ) {
			if ( reg[0x01] & 0x01 ) {
				pixelPlotX = spriteX + pixelX * 2;
				if ( pixelPlotX < 0 || pixelPlotX > 255 ) {
					continue;
				}
				if ( penSelected && ( pattern & ( 1 << ( 7 - pixelX ) ) ) ) {
					lineBuffer[pixelPlotX] = currentPalette[penSelected];
					if ( lineCollisionBuffer[pixelPlotX] != 1 ) {
						lineCollisionBuffer[pixelPlotX] = 1;
					} else {
						statusReg |= STATUS_SPRCOL;
					}
					lineBuffer[pixelPlotX+1] = currentPalette[penSelected];
					if ( lineCollisionBuffer[pixelPlotX + 1] != 1 ) {
						lineCollisionBuffer[pixelPlotX + 1] = 1;
					} else {
						statusReg |= STATUS_SPRCOL;
					}
				}
			} else {
				pixelPlotX = spriteX + pixelX;
				if ( pixelPlotX < 0 || pixelPlotX > 255 ) {
					continue;
				}
				if ( penSelected && ( pattern & ( 1 << ( 7 - pixelX ) ) ) ) {
					lineBuffer[pixelPlotX] = currentPalette[penSelected];
					if ( lineCollisionBuffer[pixelPlotX] != 1 ) {
						lineCollisionBuffer[pixelPlotX] = 1;
					} else {
						statusReg |= STATUS_SPRCOL;
					}
				}
			}
		}

		if ( reg[0x01] & 0x02 ) {
			spriteTileSelected += 2;
			pattern = spritePatternTable[ spriteTileSelected * 8 + spriteLine ];
			spriteX += ( reg[0x01] & 0x01 ? 16 : 8 );
			for ( pixelX = 0; pixelX < 8; pixelX++ ) {
				if ( reg[0x01] & 0x01 ) {
					pixelPlotX = spriteX + pixelX * 2;
					if ( pixelPlotX < 0 || pixelPlotX > 255 ) {
						continue;
					}
					if ( penSelected && ( pattern & ( 1 << ( 7 - pixelX ) ) ) ) {
						lineBuffer[pixelPlotX] = currentPalette[penSelected];
						if ( lineCollisionBuffer[pixelPlotX] != 1 ) {
							lineCollisionBuffer[pixelPlotX] = 1;
						} else {
							statusReg |= STATUS_SPRCOL;
						}
						lineBuffer[pixelPlotX+1] = currentPalette[penSelected];
						if ( lineCollisionBuffer[pixelPlotX + 1] != 1 ) {
							lineCollisionBuffer[pixelPlotX + 1] = 1;
						} else {
							statusReg |= STATUS_SPRCOL;
						}
					}
				} else {
					pixelPlotX = spriteX + pixelX;
					if ( pixelPlotX < 0 || pixelPlotX > 255 ) {
						continue;
					}
					if ( penSelected && ( pattern & ( 1 << ( 7 - pixelX ) ) ) ) {
						lineBuffer[pixelPlotX] = currentPalette[penSelected];
						if ( lineCollisionBuffer[pixelPlotX] != 1 ) {
							lineCollisionBuffer[pixelPlotX] = 1;
						} else {
							statusReg |= STATUS_SPRCOL;
						}
					}
				}
			}
		}
	}
}

void sms_refresh_line( mame_bitmap *bitmap, int line ) {
	int pixelPlotY = line;
	int pixelOffsetX = 0;
	int x;
	int *blitLineBuffer = lineBuffer;

	if ( ! IS_GAMEGEAR ) {
		pixelPlotY += TBORDER_Y_PIXELS;
		pixelOffsetX = LBORDER_X_PIXELS;
	}

	/* Check if display is disabled */
	if ( ! ( reg[0x01] & 0x40 ) ) {
		rectangle rec;
		/* set whole line to backdrop color */
		rec.min_x = 0;
		rec.max_x = LBORDER_X_PIXELS + 255 + RBORDER_X_PIXELS;
		rec.min_y = rec.max_y = pixelPlotY;
		fillbitmap( bitmap, Machine->pens[currentPalette[BACKDROP_COLOR]], &rec );
		return;
	}

	/* Only SMS has border */
	if ( ! IS_GAMEGEAR ) {
		rectangle rec;
		if ( line >= y_pixels && line < start_blanking ) {
			/* Bottom border; draw no more than 11 lines */
			if ( line - y_pixels < 11 ) {
				rec.min_x = 0;
				rec.max_x = LBORDER_X_PIXELS + 255 + RBORDER_X_PIXELS;
				rec.min_y = rec.max_y = pixelPlotY;
				fillbitmap( bitmap, Machine->pens[currentPalette[BACKDROP_COLOR]], &rec );
			}
			return;
		}
		if ( line >= start_blanking && line < start_top_border ) {
			return;
		}
		if ( line >= start_top_border && line < max_y_pixels ) {
			/* Top border; draw no more than 11 lines */
			if ( line - start_top_border < 11 ) {
				rec.min_x = 0;
				rec.max_x = LBORDER_X_PIXELS + 255 + RBORDER_X_PIXELS;
				rec.min_y = rec.max_y = 10 - ( line - start_top_border );
				fillbitmap( bitmap, Machine->pens[currentPalette[BACKDROP_COLOR]], &rec );
				return;
			}
		}
		/* Draw left border */
		rec.min_y = rec.max_y = pixelPlotY;
		rec.min_x = 0;
		rec.max_x = LBORDER_X_PIXELS - 1;
		fillbitmap( bitmap, Machine->pens[currentPalette[BACKDROP_COLOR]], &rec );
		/* Draw right border */
		rec.min_y = rec.max_y = pixelPlotY;
		rec.min_x = LBORDER_X_PIXELS + 256;
		rec.max_x = rec.min_x + RBORDER_X_PIXELS - 1;
		fillbitmap( bitmap, Machine->pens[currentPalette[BACKDROP_COLOR]], &rec );
	} else {
		if ( line >= y_pixels ) {
			return;
		}
	}

	switch( vdp_mode ) {
	case 0:
		sms_refresh_line_mode0( blitLineBuffer, line );
		break;
	case 2:
		sms_refresh_line_mode2( blitLineBuffer, line );
		break;
	case 4:
	default:
		sms_refresh_line_mode4( blitLineBuffer, line );
		break;
	}

	if ( IS_GAMEGEAR && ggSmsMode ) {
		int *combineLineBuffer = lineBuffer + ( ( line & 0x03 ) + 1 ) * 256;
		int plotX = 48;

		/* Do horizontal scaling */
		for( x = 8; x < 248; ) {
			int	combined;

			/* Take red and green from first pixel, and blue from second pixel */
			combined = ( blitLineBuffer[x] & 0x00FF ) | ( blitLineBuffer[x+1] & 0x0F00 );
			combineLineBuffer[plotX] = combined;

			/* Take red from second pixel, and green and blue from third pixel */
			combined = ( blitLineBuffer[x+1] & 0x000F ) | ( blitLineBuffer[x+2] & 0x0FF0 );
			combineLineBuffer[plotX+1] = combined;
			x += 3;
			plotX += 2;
		}

		/* Do vertical scaling for a screen with 192 lines
		   GG lines 0-7 get set to the backdrop color
		   GG lines 136-143 get set to the backdrop color
		   The remaining 128 lines (8-135) get generated from the 192 SMS lines.
		   We will calculate the gamegear lines as follows:
		   GG_8  =                             5/6 * SMS_0 + 1/6 * SMS_1
		   GG_9  = 1/3 * SMS_0 + 1/3 * SMS_1 + 1/3 * SMS_2
		   GG_10 = 1/6 * SMS_1 + 1/3 * SMS_2 + 1/3 * SMS_3 + 1/6 * SMS_4
		   GG_11 = 1/3 * SMS_3 + 1/3 * SMS_4 + 1/3 * SMS_5
		   GG_12 = 1/6 * SMS_4 + 1/3 * SMS_5 + 1/3 * SMS_6 + 1/6 * SMS_7
		   GG_13 = 1/3 * SMS_6 + 1/3 * SMS_7 + 1/3 * SMS_8
		   GG_14 = 1/6 * SMS_7 + 1/3 * SMS_8 + 1/3 * SMS_9 + 1/6 * SMS_10
		   .....
		   GG_134 = 1/6 * SMS_187 + 1/3 * SMS_188 + 1/3 * SMS_189 + 1/6 * SMS_190
		   GG_135 = 1/3 * SMS_189 + 1/3 * SMS_190 + 1/3 * SMS_191
		*/
		if ( y_pixels == 192 ) {
			int ggLine;
			int	*line1, *line2, *line3, *line4;

			/* Check if we can draw the top border */
			if ( line == 0 ) {
				rectangle rec;
				rec.min_y = 24;
				rec.max_y = 31;
				rec.min_x = 48;
				rec.max_x = 48 + 160 - 1;
				fillbitmap( bitmap, Machine->pens[currentPalette[BACKDROP_COLOR]], &rec );
			}

			/* Check if we can draw the bottom border */
			if ( line == y_pixels-1 ) {
				rectangle rec;
				rec.min_y = 24+136;
				rec.max_y = 24+143;
				rec.min_x = 48;
				rec.max_x = 48 + 160 - 1;
				fillbitmap( bitmap, Machine->pens[currentPalette[BACKDROP_COLOR]], &rec );
			}
			/* Make sure we have enough data to draw a GG line */
			if ( ( line % 3 ) == 0 ) {
				return;
			}
			ggLine = 32 + ( line / 3 ) * 2;
			/* Advance a GG line if we're on SMS line 2, 5, 8, 11, etc */
			if ( ( line % 3 ) == 2 ) {
				ggLine++;
			}
			pixelPlotY = ggLine;

			/* Setup our source lines */
			line1 = lineBuffer + ( ( ( line - 3 ) & 0x03 ) + 1 ) * 256;
			line2 = lineBuffer + ( ( ( line - 2 ) & 0x03 ) + 1 ) * 256;
			line3 = lineBuffer + ( ( ( line - 1 ) & 0x03 ) + 1 ) * 256;
			line4 = lineBuffer + ( ( ( line - 0 ) & 0x03 ) + 1 ) * 256;

			if ( ( line % 3 ) == 1 ) {
				if ( line == 1 ) {
					for( x = 0+48; x < 160+48; x++ ) {
						rgb_t   c3 = Machine->pens[line3[x]];
						rgb_t   c4 = Machine->pens[line4[x]];
						plot_pixel( bitmap, pixelOffsetX + x, pixelPlotY,
							MAKE_RGB( ( ( RGB_RED(c3) * 5 )/6 + RGB_RED(c4)/6 ),
								( ( RGB_GREEN(c3) * 5 )/6 + RGB_GREEN(c4)/6 ),
								( ( RGB_BLUE(c3) * 5 )/6 + RGB_BLUE(c4)/6 ) )
						);
					}
				} else {
					for( x = 0+48; x < 160+48; x++ ) {
						rgb_t	c1 = Machine->pens[line1[x]];
						rgb_t	c2 = Machine->pens[line2[x]];
						rgb_t	c3 = Machine->pens[line3[x]];
						rgb_t	c4 = Machine->pens[line4[x]];
						plot_pixel( bitmap, pixelOffsetX + x, pixelPlotY,
							MAKE_RGB( ( RGB_RED(c1)/6 + RGB_RED(c2)/3 + RGB_RED(c3)/3 + RGB_RED(c4)/6 ),
								( RGB_GREEN(c1)/6 + RGB_GREEN(c2)/3 + RGB_GREEN(c3)/3 + RGB_GREEN(c4)/6 ),
								( RGB_BLUE(c1)/6 + RGB_BLUE(c2)/3 + RGB_BLUE(c3)/3 + RGB_BLUE(c4)/6 ) )
						);
					}
				}
			} else {
				for( x = 0+48; x < 160+48; x++ ) {
					rgb_t	c2 = Machine->pens[line2[x]];
					rgb_t	c3 = Machine->pens[line3[x]];
					rgb_t	c4 = Machine->pens[line4[x]];
					plot_pixel( bitmap, pixelOffsetX + x, pixelPlotY,
						MAKE_RGB( ( RGB_RED(c2)/3 + RGB_RED(c3)/3 + RGB_RED(c4)/3 ),
							( RGB_GREEN(c2)/3 + RGB_GREEN(c3)/3 + RGB_GREEN(c4)/3 ),
							( RGB_BLUE(c2)/3 + RGB_BLUE(c3)/3 + RGB_BLUE(c4)/3 ) )
					);
				}
			}
			return;
		}

		/* Do vertical scaling for a screen with 224 lines
		   Lines 0-2 and 221-223 have no effect on the output on the GG screen.
		   We will calculate the gamegear lines as follows:
		   GG_0 = 1/6 * SMS_3 + 1/3 * SMS_4 + 1/3 * SMS_5 + 1/6 * SMS_6
		   GG_1 = 1/6 * SMS_4 + 1/3 * SMS_5 + 1/3 * SMS_6 + 1/6 * SMS_7
		   GG_2 = 1/6 * SMS_6 + 1/3 * SMS_7 + 1/3 * SMS_8 + 1/6 * SMS_9
		   GG_3 = 1/6 * SMS_7 + 1/3 * SMS_8 + 1/3 * SMS_9 + 1/6 * SMS_10
		   GG_4 = 1/6 * SMS_9 + 1/3 * SMS_10 + 1/3 * SMS_11 + 1/6 * SMS_12
		   .....
		   GG_142 = 1/6 * SMS_216 + 1/3 * SMS_217 + 1/3 * SMS_218 + 1/6 * SMS_219
		   GG_143 = 1/6 * SMS_217 + 1/3 * SMS_218 + 1/3 * SMS_219 + 1/6 * SMS_220
		*/
		if ( y_pixels == 224 ) {
			int	ggLine;
			int	*line1, *line2, *line3, *line4;

			/* First make sure there's enough data to draw anything */
			/* We need one more line of data if we're on line 8, 11, 14, 17, etc */
			if ( line < 6 || line > 220 || ( ( line - 8 ) % 3 == 0 ) ) {
				return;
			}
			ggLine = ( ( line - 6 ) / 3 ) * 2;
			/* If we're on SMS line 7, 10, 13, etc we're on an odd GG line */
			if ( line % 3 ) {
				ggLine++;
			}
			/* Calculate the line we will be drawing on */
			pixelPlotY = 24 + ggLine;

			/* Setup our source lines */
			line1 = lineBuffer + ( ( ( line - 3 ) & 0x03 ) + 1 ) * 256;
			line2 = lineBuffer + ( ( ( line - 2 ) & 0x03 ) + 1 ) * 256;
			line3 = lineBuffer + ( ( ( line - 1 ) & 0x03 ) + 1 ) * 256;
			line4 = lineBuffer + ( ( ( line - 0 ) & 0x03 ) + 1 ) * 256;

			for( x = 0+48; x < 160+48; x++ ) {
				rgb_t	c1 = Machine->pens[line1[x]];
				rgb_t	c2 = Machine->pens[line2[x]];
				rgb_t	c3 = Machine->pens[line3[x]];
				rgb_t	c4 = Machine->pens[line4[x]];
				plot_pixel( bitmap, pixelOffsetX + x, pixelPlotY,
					MAKE_RGB( ( RGB_RED(c1)/6 + RGB_RED(c2)/3 + RGB_RED(c3)/3 + RGB_RED(c4)/6 ),
						( RGB_GREEN(c1)/6 + RGB_GREEN(c2)/3 + RGB_GREEN(c3)/3 + RGB_GREEN(c4)/6 ),
						( RGB_BLUE(c1)/6 + RGB_BLUE(c2)/3 + RGB_BLUE(c3)/3 + RGB_BLUE(c4)/6 ) )
				);
			}
			return;
		}
		blitLineBuffer = combineLineBuffer;
	}

	for( x = 0; x < 256; x++ ) {
		plot_pixel( bitmap, pixelOffsetX + x, pixelPlotY, Machine->pens[blitLineBuffer[x]] );
	}
}

void sms_update_palette(void) {
	int i;

	/* Exit if palette is has no changes */
	if (isCRAMDirty == 0) {
		return;
	}
	isCRAMDirty = 0;

	if ( vdp_mode != 4 && ! IS_GAMEGEAR ) {
		for( i = 0; i < 16; i++ ) {
			currentPalette[i] = 64+i;
		}
		return;
	}

	if ( IS_GAMEGEAR ) {
		if ( ggSmsMode ) {
			for ( i = 0; i < 32; i++ ) {
				currentPalette[i] = ( ( CRAM[i] & 0x30 ) << 6 ) | ( ( CRAM[i] & 0x0C ) << 4 ) | ( ( CRAM[i] & 0x03 ) << 2 );
			}
		} else {
			for ( i = 0; i < 32; i++ ) {
				currentPalette[i] = ( ( CRAM[i*2+1] << 8 ) | CRAM[i*2] ) & 0x0FFF;
			}
		}
	} else {
		for ( i = 0; i < 32; i++ ) {
			currentPalette[i] = CRAM[i] & 0x3F;
		}
	}
}

VIDEO_UPDATE(sms) {
	int x, y;

	if (prevBitMapSaved) {
	for (y = 0; y < Machine->screen[0].height; y++) {
		for (x = 0; x < Machine->screen[0].width; x++) {
			plot_pixel(bitmap, x, y, (read_pixel(tmpbitmap, x, y) + read_pixel(prevBitMap, x, y)) >> 2);
			logerror("%x %x %x\n", read_pixel(tmpbitmap, x, y), read_pixel(prevBitMap, x, y), (read_pixel(tmpbitmap, x, y) + read_pixel(prevBitMap, x, y)) >> 2);
		}
	}
	} else {
		copybitmap(bitmap, tmpbitmap, 0, 0, 0, 0, &Machine->screen[0].visarea, TRANSPARENCY_NONE, 0);
	}
	if (!prevBitMapSaved) {
		copybitmap(prevBitMap, tmpbitmap, 0, 0, 0, 0, &Machine->screen[0].visarea, TRANSPARENCY_NONE, 0);
	//prevBitMapSaved = 1;
	}
	return 0;
}



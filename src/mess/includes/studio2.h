#ifndef __STUDIO2__
#define __STUDIO2__

#include "cpu/cdp1802/cdp1802.h"

#define SCREEN_TAG	"screen"
#define CDP1802_TAG "cdp1802"
#define CDP1861_TAG "cdp1861"
#define CDP1864_TAG "cdp1864"

#define ST2_BLOCK_SIZE 256

typedef struct _studio2_state studio2_state;
struct _studio2_state
{
	/* cpu state */
	cdp1802_control_mode cdp1802_mode;

	/* keyboard state */
	UINT8 keylatch;

	/* video state */
	int cdp1861_efx;
	int cdp1864_efx;
	UINT8 *color_ram;
	UINT8 color;

	/* devices */
	const device_config *cdp1861;
	const device_config *cdp1864;
};

typedef struct _st2_header st2_header;
struct _st2_header
{
	UINT8 header[4];			/* "RCA2" in ASCII code */
	UINT8 blocks;				/* Total number of 256 byte blocks in file (including this one) */
	UINT8 format;				/* Format Code (this is format number 1) */
	UINT8 video;				/* If non-zero uses a special video driver, and programs cannot assume that it uses the standard Studio 2 one (top of screen at $0900+RB.0). A value of '1' here indicates the RAM is used normally, but scrolling is not (e.g. the top of the page is always at $900) */
	UINT8 reserved0;
	UINT8 author[2];			/* 2 byte ASCII code indicating the identity of the program coder */
	UINT8 dumper[2];			/* 2 byte ASCII code indicating the identity of the ROM Source */
	UINT8 reserved1[4];
	UINT8 catalogue[10];		/* RCA Catalogue Code as ASCIIZ string. If a homebrew ROM, may contain any identifying code you wish */
	UINT8 reserved2[6];
	UINT8 title[32];			/* Cartridge Program Title as ASCIIZ string */
	UINT8 page[64];				/* Contain the page addresses for each 256 byte block. The first byte at 64, contains the target address of the data at offset 256, the second byte contains the target address of the data at offset 512, and so on. Unused block bytes should be filled with $00 (an invalid page address). So, if byte 64 contains $1C, the ROM is paged into memory from $1C00-$1CFF */
	UINT8 reserved3[128];
};

#endif

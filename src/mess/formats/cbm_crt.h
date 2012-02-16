/*********************************************************************

    formats/cbm_crt.h

    Commodore VIC-20/C64 cartridge images

*********************************************************************/

#pragma once

#ifndef __CBM_CRT__
#define __CBM_CRT__

#include "emu.h"



//**************************************************************************
//  MACROS/CONSTANTS
//**************************************************************************

#define CRT_HEADER_LENGTH	0x40
#define CRT_CHIP_LENGTH		0x10

#define UNSUPPORTED 		"standard"


// VIC-20 cartridge types
enum
{
	CRT_VIC20_STANDARD = 1,
	CRT_VIC20_MEGACART,
	CRT_VIC20_FINAL_EXPANSION,
	CRT_VIC20_FP,
	_CRT_VIC20_COUNT
};


// slot names for the VIC-20 cartridge types
const char *CRT_VIC20_SLOT_NAMES[_CRT_VIC20_COUNT] = 
{
	UNSUPPORTED,
	"standard",
	UNSUPPORTED,
	UNSUPPORTED,
	UNSUPPORTED
};


// C64 cartridge types
enum
{
	CRT_C64_STANDARD = 0,
	CRT_C64_ACTION_REPLAY,
	CRT_C64_KCS_POWER,
	CRT_C64_FINAL_III,
	CRT_C64_SIMONS_BASIC,
	CRT_C64_OCEAN,
	CRT_C64_EXPERT,
	CRT_C64_FUNPLAY,
	CRT_C64_SUPER_GAMES,
	CRT_C64_ATOMIC_POWER,
	CRT_C64_EPYX_FASTLOAD,
	CRT_C64_WESTERMANN,
	CRT_C64_REX,
	CRT_C64_FINAL_I,
	CRT_C64_MAGIC_FORMEL,
	CRT_C64_GS,
	CRT_C64_WARPSPEED,
	CRT_C64_DINAMIC,
	CRT_C64_ZAXXON,
	CRT_C64_MAGIC_DESK,
	CRT_C64_SUPER_SNAPSHOT_V5,
	CRT_C64_COMAL80,
	CRT_C64_STRUCTURED_BASIC,
	CRT_C64_ROSS,
	CRT_C64_DELA_EP64,
	CRT_C64_DELA_EP7x8,
	CRT_C64_DELA_EP256,
	CRT_C64_REX_EP256,
	CRT_C64_MIKRO_ASSEMBLER,
	CRT_C64_FINAL_PLUS,
	CRT_C64_ACTION_REPLAY4,
	CRT_C64_STARDOS,
	CRT_C64_EASYFLASH,
	CRT_C64_EASYFLASH_XBANK,
	CRT_C64_CAPTURE,
	CRT_C64_ACTION_REPLAY3,
	CRT_C64_RETRO_REPLAY,
	CRT_C64_MMC64,
	CRT_C64_MMC_REPLAY,
	CRT_C64_IDE64,
	CRT_C64_SUPER_SNAPSHOT,
	CRT_C64_IEEE488,
	CRT_C64_GAME_KILLER,
	CRT_C64_P64,
	_CRT_C64_COUNT
};


// slot names for the C64 cartridge types
const char *CRT_C64_SLOT_NAMES[_CRT_C64_COUNT] = 
{
	"standard",
	UNSUPPORTED,
	UNSUPPORTED,
	UNSUPPORTED,
	"simons_basic",
	"ocean",
	UNSUPPORTED,
	"fun_play",
	"super_games",
	UNSUPPORTED,
	"epyxfastload",
	"westermann",
	"rex",
	UNSUPPORTED,
	"magic_formel",
	"system3",
	"warp_speed",
	"dinamic",
	"zaxxon",
	"magic desk",
	UNSUPPORTED,
	"comal80",
	"struct_basic",
	"ross",
	"ep64",
	"ep7x8",
	"dela_ep256",
	"rex_ep256",
	"mikroasm",
	UNSUPPORTED,
	UNSUPPORTED,
	"stardos",
	UNSUPPORTED,
	UNSUPPORTED,
	UNSUPPORTED,
	UNSUPPORTED,
	UNSUPPORTED,
	UNSUPPORTED,
	UNSUPPORTED,
	UNSUPPORTED,
	UNSUPPORTED,
	"ieee488",
	UNSUPPORTED,
	UNSUPPORTED
};



//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

typedef struct _cbm_crt_header cbm_crt_header;
struct _cbm_crt_header
{
	UINT8 signature[16];
	UINT8 header_length[4];
	UINT8 version[2];
	UINT8 hardware[2];
	UINT8 exrom;
	UINT8 game;
	UINT8 reserved[6];
	UINT8 name[32];
};


typedef struct _cbm_crt_chip cbm_crt_chip;
struct _cbm_crt_chip
{
	UINT8 signature[4];
	UINT8 packet_length[4];
	UINT8 chip_type[2];
	UINT8 bank[2];
	UINT8 start_address[2];
	UINT8 image_size[2];
};


#endif

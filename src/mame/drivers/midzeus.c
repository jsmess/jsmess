/*************************************************************************

    Driver for Midway Zeus games

    driver by Aaron Giles

    Games supported:
        * Invasion
        * Mortal Kombat 4
        * Cruis'n Exotica
        * The Grid

    Known bugs:
        * not done yet

**************************************************************************/

#include "driver.h"
#include "cpu/tms34010/tms34010.h"
#include "cpu/adsp2100/adsp2100.h"
#include "machine/midwayic.h"
#include "audio/dcs.h"


static UINT32 *ram_base;
static UINT8 cmos_protected;

static void *timer[2];

static UINT32 *tms32031_control;
static UINT32 *graphics_ram;


//------------------------------

#define WAVERAM_WIDTH	512
#define WAVERAM_HEIGHT	4096

static UINT32 *waveram;


VIDEO_START( midzeus )
{
	int i;

	waveram = auto_malloc(2 * WAVERAM_WIDTH * WAVERAM_HEIGHT * sizeof(*waveram));

	for (i = 0; i < 32768; i++)
		palette_set_color(machine, i, pal5bit(i >> 10), pal5bit(i >> 5), pal5bit(i >> 0));
	return 0;
}

VIDEO_UPDATE( midzeus )
{
	static int yoffs = 0;
	int p = code_pressed(KEYCODE_0) ? 0 : 1;
	int x, y;
	if (code_pressed(KEYCODE_DOWN) && code_pressed(KEYCODE_LCONTROL)) yoffs += 8;
	if (code_pressed(KEYCODE_UP) && code_pressed(KEYCODE_LCONTROL)) yoffs -= 8;
	for (y = cliprect->min_y; y <= cliprect->max_y; y++)
	{
		UINT32 *src = &waveram[(p * WAVERAM_HEIGHT + ((y + yoffs) % WAVERAM_HEIGHT)) * WAVERAM_WIDTH + 0];
		UINT16 *dest = (UINT16 *)bitmap->base + y * bitmap->rowpixels;
		for (x = cliprect->min_x; x <= cliprect->max_x; x++)
			dest[x] = src[x] & 0x7fff;
	}
	return 0;
}

/*
    offset B7 = control....

        8000 = page 1 enable
        4000 = page 0 enable

        0080 = 2nd pixel enable (maybe Z buff?)
        0040 = 1st pixel enable (maybe Z buff?)
        0020 = 2nd pixel enable
        0010 = 1st pixel enable


    writes 80a2 to write an odd pixel at startup
    writes 80f6 to write the middle pixels
    writes 8052 to write an odd pixel at the end

    writes 8050 before writing an odd pixel
    writes 80a0 before writing an even pixel

    writes 44f1 before reading from page 0 of wave ram
    writes 84f1 before reading from page 1 of wave ram

    writes 42f0 before writing to page 0 of wave ram
    writes 82f0 before writing to page 1 of wave ram


Initialization:
01BFF4:graphics_w(080) = 0020F023
01BFF1:graphics_w(081) = 00000020
01C03E:graphics_w(080) = 00000000
01C03E:graphics_w(081) = 00008200
01C03E:graphics_w(08F) = 00000206
01C03E:graphics_w(08E) = 00000000
01C03E:graphics_w(08B) = 00000000
01C03E:graphics_w(08A) = 0000FAFA
01C03E:graphics_w(08D) = 00000206
01C03E:graphics_w(08C) = 00000000
01C03E:graphics_w(089) = 00000000
01C03E:graphics_w(088) = 0000FAFA
01C03E:graphics_w(080) = 00000001
01C03E:graphics_w(0CF) = 000000C8
01C03E:graphics_w(0CE) = 00007620
01C03E:graphics_w(0CD) = 00000000
01C03E:graphics_w(0CC) = 00000000
01C03E:graphics_w(0CB) = 00000116
01C03E:graphics_w(0CA) = 00000107
01C03E:graphics_w(0C9) = 00000103
01C03E:graphics_w(0C8) = 000000FF
01C03E:graphics_w(0C7) = 00000211
01C03E:graphics_w(0C6) = 0000007F
01C03E:graphics_w(0C5) = 0000000C
01C03E:graphics_w(0C4) = 00000042
01C03E:graphics_w(0C3) = 00000015
01C03E:graphics_w(0C2) = 0000E591
01C03E:graphics_w(0C1) = 0000801F
01C03E:graphics_w(0C0) = 00002500
01C03E:graphics_w(086) = 00008003
01C03E:graphics_w(080) = 00000C05
01C03E:graphics_w(085) = 00000000
01C03E:graphics_w(084) = 00000000
01C03E:graphics_w(083) = 00001F1F
01C03E:graphics_w(082) = 00002011
01C03E:graphics_w(069) = 00000003
01C03E:graphics_w(069) = 00000000
01C03E:graphics_w(08D) = 00000227
01C03E:graphics_w(08C) = 0000BFFF
01C03E:graphics_w(08D) = 00000226
01C03E:graphics_w(08C) = 00000000
01C03E:graphics_w(08D) = 00000227
01C03E:graphics_w(08C) = 0000BFFF
01C03E:graphics_w(08D) = 00000226
01C03E:graphics_w(08C) = 00000000
01C03E:graphics_w(08D) = 00000227
01C03E:graphics_w(08C) = 0000F020
01C03E:graphics_w(08D) = 00000226
01C03E:graphics_w(08C) = 00000000
01C03E:graphics_w(08D) = 00000227
01C03E:graphics_w(08C) = 0000DFFF
01C03E:graphics_w(08D) = 00000226
01C03E:graphics_w(08C) = 00000000
01C03E:graphics_w(08D) = 00000227
01C03E:graphics_w(08C) = 0000DFFF
01C03E:graphics_w(08D) = 00000226
01C03E:graphics_w(08C) = 00000000
01C03E:graphics_w(08F) = 00000207
01C03E:graphics_w(08E) = 0000BFFF
01C03E:graphics_w(08F) = 00000206
01C03E:graphics_w(08E) = 00000000
01C03E:graphics_w(08F) = 00000207
01C03E:graphics_w(08E) = 0000F020
01C03E:graphics_w(08F) = 00000206
01C03E:graphics_w(08E) = 00000000
01C03E:graphics_w(08F) = 00000207
01C03E:graphics_w(08E) = 0000DFFF
01C03E:graphics_w(08F) = 00000206
01C03E:graphics_w(08E) = 00000000
01C03E:graphics_w(08F) = 00000207
01C03E:graphics_w(08E) = 0000DFFF
01C03E:graphics_w(08F) = 00000206
01C03E:graphics_w(08E) = 00000000
01C03E:graphics_w(08F) = 00000206
01C03E:graphics_w(08E) = 00000000
01C03E:graphics_w(08D) = 00000226
01C03E:graphics_w(08C) = 00000000
01C03E:graphics_w(08B) = 00000000
01C03E:graphics_w(08A) = 0000D254
01C03E:graphics_w(089) = 00000000
01C03E:graphics_w(088) = 0000D794

then:
01C2CE:graphics_w(0B7) = 00000000
01C2D0:graphics_w(0B6) = 00000001
01C2D1:graphics_w(0B0) = 00000000
01C2D2:graphics_w(0B1) = 00000000
01C2D3:graphics_w(0B2) = 00007FFF
01C2D4:graphics_w(0B3) = 00007FFF
01C2D8:graphics_w(0B5) = 000000FF  y
01C2DD:graphics_w(0B4) = 00000100  x/yhi
01C2EA:graphics_w(0B7) = FFFF80F6  page 1
01C2EB:graphics_w(0B0) = 00000000
01C2EC:graphics_r(0F6)
01C2F4:graphics_w(0B0) = 00000000 x 256

01C2F6:graphics_w(0B7) = FFFF8052
01C2CE:graphics_w(0B7) = 00000000
01C2D0:graphics_w(0B6) = 00000001
01C2D1:graphics_w(0B0) = 00000000
01C2D2:graphics_w(0B1) = 00000000
01C2D3:graphics_w(0B2) = 00007FFF
01C2D4:graphics_w(0B3) = 00007FFF
01C2D8:graphics_w(0B5) = 000000FF  y
01C2DD:graphics_w(0B4) = 00000000  x/yhi
01C2EA:graphics_w(0B7) = FFFF80F6
01C2EB:graphics_w(0B0) = 00000000
01C2EC:graphics_r(0F6)
01C2F4:graphics_w(0B0) = 00000000 x 256

etc. to clear all scanlines on page 1

then:
01C2F6:graphics_w(0B7) = FFFF8052
01BFF4:graphics_w(080) = 00200001
01C1B6:graphics_w(0CF) = 000000C9
01BFF4:graphics_w(080) = 00200C27
01C1BC:graphics_w(0CF) = 000000C8
01BFF4:graphics_w(080) = 0020F023
01BFF1:graphics_w(081) = 00000020
01BFF1:graphics_w(081) = 00000022
01BFF4:graphics_w(080) = 0022F023

01C0DB:graphics_w(200) = 00000E00
01C0DB:graphics_w(202) = 00000025
01C0DB:graphics_w(204) = 00000E01
01C0DB:graphics_w(206) = 00000025
01C0DB:graphics_w(208) = 00000E02
01C0DB:graphics_w(20A) = 00000025
01C0DB:graphics_w(20C) = 00000E03
01C0DB:graphics_w(20E) = 00000025
01C0DB:graphics_w(210) = 00000E04
01C0DB:graphics_w(212) = 00000025
01C0DB:graphics_w(214) = 00000E05
01C0DB:graphics_w(216) = 00000025
01C0DB:graphics_w(218) = 00000E06
01C0DB:graphics_w(21A) = 00000025
01C0DB:graphics_w(21C) = 00000E07
01C0DB:graphics_w(21E) = 00000025
01C0DB:graphics_w(220) = 00000E08
01C0DB:graphics_w(222) = 00000025
01C0DB:graphics_w(224) = 00000E09
01C0DB:graphics_w(226) = 00000025
01C0DB:graphics_w(228) = 00000E0A
01C0DB:graphics_w(22A) = 00000025
01C0DB:graphics_w(22C) = 00000E0B
01C0DB:graphics_w(22E) = 00000025
01C0DB:graphics_w(230) = 00000E0C
01C0DB:graphics_w(232) = 00000025
01C0DB:graphics_w(234) = 00000E0D
01C0DB:graphics_w(236) = 00000025
01C0DB:graphics_w(238) = 00000E0E
01C0DB:graphics_w(23A) = 00000025
01C0DB:graphics_w(23C) = 00000E0F
01C0DB:graphics_w(23E) = 00000025
01C0DB:graphics_w(240) = 00000E10
01C0DB:graphics_w(242) = 00000025
01C0DB:graphics_w(244) = 00000E11
01C0DB:graphics_w(246) = 00000025
01C0DB:graphics_w(248) = 00000E12
01C0DB:graphics_w(24A) = 00000025
01C0DB:graphics_w(24C) = 00000E13
01C0DB:graphics_w(24E) = 00000025
01C0DB:graphics_w(250) = 00000E14
01C0DB:graphics_w(252) = 00000025
01C0DB:graphics_w(254) = 00000E15
01C0DB:graphics_w(256) = 00000025
01C0DB:graphics_w(258) = 00000E16
01C0DB:graphics_w(25A) = 00000025
01C0DB:graphics_w(25C) = 00000E17
01C0DB:graphics_w(25E) = 00000025
01C0DB:graphics_w(260) = 00000E18
01C0DB:graphics_w(262) = 00000025
01C0DB:graphics_w(264) = 00000E19
01C0DB:graphics_w(266) = 00000025
01C0DB:graphics_w(268) = 00000E1A
01C0DB:graphics_w(26A) = 00000025
01C0DB:graphics_w(26C) = 00000E1B
01C0DB:graphics_w(26E) = 00000025
01C0DB:graphics_w(270) = 00000E1C
01C0DB:graphics_w(272) = 00000025
01C0DB:graphics_w(274) = 00000E1D
01C0DB:graphics_w(276) = 00000025
01C0DB:graphics_w(278) = 00000E1E
01C0DB:graphics_w(27A) = 00000025
01C0DB:graphics_w(27C) = 00000E1F
01C0DB:graphics_w(27E) = 00000025
01C0DB:graphics_w(280) = 00000E20
01C0DB:graphics_w(282) = 00000025
01C0DB:graphics_w(284) = 00000E21
01C0DB:graphics_w(286) = 00000025
01C0DB:graphics_w(288) = 00000E22
01C0DB:graphics_w(28A) = 00000025
01C0DB:graphics_w(28C) = 00000E23
01C0DB:graphics_w(28E) = 00000025
01C0DB:graphics_w(290) = 00000E24
01C0DB:graphics_w(292) = 00000025
01C0DB:graphics_w(294) = 00000E25
01C0DB:graphics_w(296) = 00000025
01C0DB:graphics_w(298) = 00000E26
01C0DB:graphics_w(29A) = 00000025
01C0DB:graphics_w(29C) = 00000E27
01C0DB:graphics_w(29E) = 00000025
01C0DB:graphics_w(2A0) = 00000E28
01C0DB:graphics_w(2A2) = 00000025
01C0DB:graphics_w(2A4) = 00000E29
01C0DB:graphics_w(2A6) = 00000025
01C0DB:graphics_w(2A8) = 00000E2A
01C0DB:graphics_w(2AA) = 00000025
01C0DB:graphics_w(2AC) = 00000E2B
01C0DB:graphics_w(2AE) = 00000025
01C0DB:graphics_w(2B0) = 00000E2C
01C0DB:graphics_w(2B2) = 00000025
01C0DB:graphics_w(2B4) = 00000E2D
01C0DB:graphics_w(2B6) = 00000025
01C0DB:graphics_w(2B8) = 00000E2E
01C0DB:graphics_w(2BA) = 00000025
01C0DB:graphics_w(2BC) = 00000E2F
01C0DB:graphics_w(2BE) = 00000025
01C0DB:graphics_w(2C0) = 00000E30
01C0DB:graphics_w(2C2) = 00000025
01C0DB:graphics_w(2C4) = 00000E31
01C0DB:graphics_w(2C6) = 00000025
01C0DB:graphics_w(2C8) = 00000E32
01C0DB:graphics_w(2CA) = 00000025
01C0DB:graphics_w(2CC) = 00000E33
01C0DB:graphics_w(2CE) = 00000025
01C0DB:graphics_w(2D0) = 00000E34
01C0DB:graphics_w(2D2) = 00000025
01C0DB:graphics_w(2D4) = 00000E35
01C0DB:graphics_w(2D6) = 00000025
01C0DB:graphics_w(2D8) = 00000E36
01C0DB:graphics_w(2DA) = 00000025
01C0DB:graphics_w(2DC) = 00000E37
01C0DB:graphics_w(2DE) = 00000025
01C0DB:graphics_w(2E0) = 00000E38
01C0DB:graphics_w(2E2) = 00000025
01C0DB:graphics_w(2E4) = 00000E39
01C0DB:graphics_w(2E6) = 00000025
01C0DB:graphics_w(2E8) = 00000E3A
01C0DB:graphics_w(2EA) = 00000025
01C0DB:graphics_w(2EC) = 00000E3B
01C0DB:graphics_w(2EE) = 00000025
01C0DB:graphics_w(2F0) = 00000E3C
01C0DB:graphics_w(2F2) = 00000025
01C0DB:graphics_w(2F4) = 00000E3D
01C0DB:graphics_w(2F6) = 00000025
01C0DB:graphics_w(2F8) = 00000E3E
01C0DB:graphics_w(2FA) = 00000025
01C0DB:graphics_w(2FC) = 00000E3F
01C0DB:graphics_w(2FE) = 00000025
01C0DB:graphics_w(300) = 00000E40
01C0DB:graphics_w(302) = 00000025
01C0DB:graphics_w(304) = 00000E41
01C0DB:graphics_w(306) = 00000025
01C0DB:graphics_w(308) = 00000E42
01C0DB:graphics_w(30A) = 00000025
01C0DB:graphics_w(30C) = 00000E43
01C0DB:graphics_w(30E) = 00000025
01C0DB:graphics_w(310) = 00000E44
01C0DB:graphics_w(312) = 00000025
01C0DB:graphics_w(314) = 00000E45
01C0DB:graphics_w(316) = 00000025
01C0DB:graphics_w(318) = 00000E46
01C0DB:graphics_w(31A) = 00000025
01C0DB:graphics_w(31C) = 00000E47
01C0DB:graphics_w(31E) = 00000025
01C0DB:graphics_w(320) = 00000E48
01C0DB:graphics_w(322) = 00000025
01C0DB:graphics_w(324) = 00000E49
01C0DB:graphics_w(326) = 00000025
01C0DB:graphics_w(328) = 00000E4A
01C0DB:graphics_w(32A) = 00000025
01C0DB:graphics_w(32C) = 00000E4B
01C0DB:graphics_w(32E) = 00000025
01C0DB:graphics_w(330) = 00000E4C
01C0DB:graphics_w(332) = 00000025
01C0DB:graphics_w(334) = 00000E4D
01C0DB:graphics_w(336) = 00000025
01C0DB:graphics_w(338) = 00000E4E
01C0DB:graphics_w(33A) = 00000025
01C0DB:graphics_w(33C) = 00000E4F
01C0DB:graphics_w(33E) = 00000025
01C0DB:graphics_w(340) = 00000E50
01C0DB:graphics_w(342) = 00000025
01C0DB:graphics_w(344) = 00000E51
01C0DB:graphics_w(346) = 00000025
01C0DB:graphics_w(348) = 00000E52
01C0DB:graphics_w(34A) = 00000025
01C0DB:graphics_w(34C) = 00000E53
01C0DB:graphics_w(34E) = 00000025
01C0DB:graphics_w(350) = 00000E54
01C0DB:graphics_w(352) = 00000025
01C0DB:graphics_w(354) = 00000E55
01C0DB:graphics_w(356) = 00000025
01C0DB:graphics_w(358) = 00000E56
01C0DB:graphics_w(35A) = 00000025
01C0DB:graphics_w(35C) = 00000E57
01C0DB:graphics_w(35E) = 00000025
01C0DB:graphics_w(360) = 00000E58
01C0DB:graphics_w(362) = 00000025
01C0DB:graphics_w(364) = 00000E59
01C0DB:graphics_w(366) = 00000025
01C0DB:graphics_w(368) = 00000E5A
01C0DB:graphics_w(36A) = 00000025
01C0DB:graphics_w(36C) = 00000E5B
01C0DB:graphics_w(36E) = 00000025
01C0DB:graphics_w(370) = 00000E5C
01C0DB:graphics_w(372) = 00000025
01C0DB:graphics_w(374) = 00000E5D
01C0DB:graphics_w(376) = 00000025
01C0DB:graphics_w(378) = 00000E5E
01C0DB:graphics_w(37A) = 00000025
01C0DB:graphics_w(37C) = 00000E5F
01C0DB:graphics_w(37E) = 00000025
01C0DB:graphics_w(380) = 00000E60
01C0DB:graphics_w(382) = 00000025
01C0DB:graphics_w(384) = 00000E61
01C0DB:graphics_w(386) = 00000025
01C0DB:graphics_w(388) = 00000E62
01C0DB:graphics_w(38A) = 00000025
01C0DB:graphics_w(38C) = 00000E63
01C0DB:graphics_w(38E) = 00000025
01C0DB:graphics_w(390) = 00000E64
01C0DB:graphics_w(392) = 00000025
01C0DB:graphics_w(394) = 00000E65
01C0DB:graphics_w(396) = 00000025
01C0DB:graphics_w(398) = 00000E66
01C0DB:graphics_w(39A) = 00000025
01C0DB:graphics_w(39C) = 00000E67
01C0DB:graphics_w(39E) = 00000025
01C0DB:graphics_w(3A0) = 00000E68
01C0DB:graphics_w(3A2) = 00000025
01C0DB:graphics_w(3A4) = 00000E69
01C0DB:graphics_w(3A6) = 00000025
01C0DB:graphics_w(3A8) = 00000E6A
01C0DB:graphics_w(3AA) = 00000025
01C0DB:graphics_w(3AC) = 00000E6B
01C0DB:graphics_w(3AE) = 00000025
01C0DB:graphics_w(3B0) = 00000E6C
01C0DB:graphics_w(3B2) = 00000025
01C0DB:graphics_w(3B4) = 00000E6D
01C0DB:graphics_w(3B6) = 00000025
01C0DB:graphics_w(3B8) = 00000E6E
01C0DB:graphics_w(3BA) = 00000025
01C0DB:graphics_w(3BC) = 00000E6F
01C0DB:graphics_w(3BE) = 00000025
01C0DB:graphics_w(3C0) = 00000E70
01C0DB:graphics_w(3C2) = 00000025
01C0DB:graphics_w(3C4) = 00000E71
01C0DB:graphics_w(3C6) = 00000025
01C0DB:graphics_w(3C8) = 00000E72
01C0DB:graphics_w(3CA) = 00000025
01C0DB:graphics_w(3CC) = 00000E73
01C0DB:graphics_w(3CE) = 00000025
01C0DB:graphics_w(3D0) = 00000E74
01C0DB:graphics_w(3D2) = 00000025
01C0DB:graphics_w(3D4) = 00000E75
01C0DB:graphics_w(3D6) = 00000025
01C0DB:graphics_w(3D8) = 00000E76
01C0DB:graphics_w(3DA) = 00000025
01C0DB:graphics_w(3DC) = 00000E77
01C0DB:graphics_w(3DE) = 00000025
01C0DB:graphics_w(3E0) = 00000E78
01C0DB:graphics_w(3E2) = 00000025
01C0DB:graphics_w(3E4) = 00000E79
01C0DB:graphics_w(3E6) = 00000025
01C0DB:graphics_w(3E8) = 00000E7A
01C0DB:graphics_w(3EA) = 00000025
01C0DB:graphics_w(3EC) = 00000E7B
01C0DB:graphics_w(3EE) = 00000025
01C0DB:graphics_w(3F0) = 00000E7C
01C0DB:graphics_w(3F2) = 00000025
01C0DB:graphics_w(3F4) = 00000E7D
01C0DB:graphics_w(3F6) = 00000025
01C0DB:graphics_w(3F8) = 00000E7E
01C0DB:graphics_w(3FA) = 00000025
01C0DB:graphics_w(3FC) = 00000E7F
01C0DB:graphics_w(3FE) = 00000025

01BFB6:graphics_w(05A) = 00000000
01BFB8:graphics_w(058) = 3F000000
01BFB9:graphics_r(0F6)
01BFBE:graphics_w(0E0) = 00000000
01BFF4:graphics_w(080) = 0022F02B
01BFC2:graphics_w(068) = 00030000
01BFC4:graphics_w(068) = 00000000
01C052:graphics_w(08A) = 8FFFD254
01C054:graphics_w(088) = 87FFD794
01BFF4:graphics_w(080) = 0022FC2B

(other stuff)

loop over each scanline:
    A2DD19:graphics_w(0B7) = 00000000
    A2DD24:graphics_w(0B6) = 82F00001
    A2DD30:graphics_r(0F6)
    A2DD34:graphics_w(0B4) = 01FF0000
    A2DD37:graphics_w(0B0) = AAAAAAAA
    A2DD39:graphics_w(0B2) = AAAAAAAA
    A2DD30:graphics_r(0F6)
    A2DD34:graphics_w(0B4) = 01FF0001
    A2DD37:graphics_w(0B0) = AAAAAAAA
    A2DD39:graphics_w(0B2) = AAAAAAAA
    A2DD30:graphics_r(0F6)
    ...
    A2DD34:graphics_w(0B4) = 01FF01FE
    A2DD37:graphics_w(0B0) = AAAAAAAA
    A2DD39:graphics_w(0B2) = AAAAAAAA
    A2DD30:graphics_r(0F6)
    A2DD34:graphics_w(0B4) = 01FF01FF
    A2DD37:graphics_w(0B0) = AAAAAAAA
    A2DD39:graphics_w(0B2) = AAAAAAAA
    A2DD45:graphics_w(0B6) = 00000000
    A2DD58:graphics_w(0B6) = 00000000
    A2DD6E:graphics_w(0B6) = 84F10001
    A2DD6F:graphics_w(0B4) = 01FF0000
    A2DD71:graphics_r(0F6)
    A2DD76:graphics_w(0B6) = 00000000
    A2DD77:graphics_r(0B0)
    A2DD79:graphics_r(0B2)
    A2DD6E:graphics_w(0B6) = 84F10001
    A2DD6F:graphics_w(0B4) = 01FF0001
    A2DD71:graphics_r(0F6)
    A2DD76:graphics_w(0B6) = 00000000
    A2DD77:graphics_r(0B0)
    A2DD79:graphics_r(0B2)
    ...
    A2DD6E:graphics_w(0B6) = 84F10001
    A2DD6F:graphics_w(0B4) = 01FF01FE
    A2DD71:graphics_r(0F6)
    A2DD76:graphics_w(0B6) = 00000000
    A2DD77:graphics_r(0B0)
    A2DD79:graphics_r(0B2)
    A2DD6E:graphics_w(0B6) = 84F10001
    A2DD6F:graphics_w(0B4) = 01FF01FF
    A2DD71:graphics_r(0F6)
    A2DD76:graphics_w(0B6) = 00000000
    A2DD77:graphics_r(0B0)
    A2DD79:graphics_r(0B2)
    A2DD82:graphics_w(0B6) = 00000000
    A2DD19:graphics_w(0B7) = 00000000
    A2DD24:graphics_w(0B6) = 82F00001
    A2DD30:graphics_r(0F6)

repeat with 55555555....

repeat with ffffffff....

repeat with 00000000....

A2DD82:graphics_w(0B6) = 00000000
A2DCAF:graphics_w(0B6) = 00000000
A2DCB3:graphics_w(0B2) = 7FFF7FFF
A2DCB7:graphics_w(0B0) = 00000000
A2DCC2:graphics_w(0B4) = 007F0100
A2DCCD:graphics_w(0B6) = 80F20001
A2DD89:graphics_w(0B0) = 00000000
A2DD8A:graphics_r(0F6)            \  ... x256
A2DD89:graphics_w(0B0) = 00000000 /

A2DCAF:graphics_w(0B6) = 00000000
A2DCB3:graphics_w(0B2) = 7FFF7FFF
A2DCB7:graphics_w(0B0) = 00000000
A2DCC2:graphics_w(0B4) = 007F0000
A2DCCD:graphics_w(0B6) = 80F20001
A2DD89:graphics_w(0B0) = 00000000
A2DD8A:graphics_r(0F6)            \  ... x256
A2DD89:graphics_w(0B0) = 00000000 /

drawing:
A2DCEB:graphics_w(0B6) = 00000000
A2DCEF:graphics_w(0B2) = 7FFF7FFF
A2DCF3:graphics_w(0B0) = 001F001F
A2DCFE:graphics_w(0B4) = 00040104
A2DD02:graphics_w(0B7) = 80A00000
A2DD89:graphics_w(0B0) = 001F001F
A2DD8A:graphics_r(0F6)
A2DCEB:graphics_w(0B6) = 00000000
A2DCEF:graphics_w(0B2) = 7FFF7FFF
A2DCF3:graphics_w(0B0) = 001F001F
A2DCFE:graphics_w(0B4) = 00040105
A2DD07:graphics_w(0B7) = 80500000
A2DD89:graphics_w(0B0) = 001F001F
A2DD8A:graphics_r(0F6)
A2DCEB:graphics_w(0B6) = 00000000
A2DCEF:graphics_w(0B2) = 7FFF7FFF
A2DCF3:graphics_w(0B0) = 001F001F
A2DCFE:graphics_w(0B4) = 00040105
A2DD02:graphics_w(0B7) = 80A00000
A2DD89:graphics_w(0B0) = 001F001F
A2DD8A:graphics_r(0F6)
A2DCEB:graphics_w(0B6) = 00000000
A2DCEF:graphics_w(0B2) = 7FFF7FFF
A2DCF3:graphics_w(0B0) = 001F001F
A2DCFE:graphics_w(0B4) = 00040106
A2DD07:graphics_w(0B7) = 80500000
A2DD89:graphics_w(0B0) = 001F001F
A2DD8A:graphics_r(0F6)


Invasion:

011729:graphics_w(0E0) = 1A000000
011729:graphics_w(0E0) = 187E0000
011729:graphics_w(0E0) = 0000FFFF
011729:graphics_w(0E0) = 187C0000
011729:graphics_w(0E0) = FFFFFFE0
011729:graphics_w(0E0) = 187A0000
011729:graphics_w(0E0) = 00800000
011729:graphics_w(0E0) = 18780000
011729:graphics_w(0E0) = 000000C7
011729:graphics_w(0E0) = 18760000
011729:graphics_w(0E0) = F8800000
011729:graphics_w(0E0) = 176F0032
011729:graphics_w(0E0) = 176E0031
011729:graphics_w(0E0) = 176D003B
011729:graphics_w(0E0) = 176C002B
011729:graphics_w(0E0) = 184E0000
011729:graphics_w(0E0) = 00808080
011729:graphics_w(0E0) = 184C0000
011729:graphics_w(0E0) = 00808080
011729:graphics_w(0E0) = 184A0000
011729:graphics_w(0E0) = 00FF018E
011729:graphics_w(0E0) = 18480000
011729:graphics_w(0E0) = 00FF018E
011729:graphics_w(0E0) = 176940C4
011729:graphics_w(0E0) = 17681370
011729:graphics_w(0E0) = 1707FFFF
011729:graphics_w(0E0) = 17060030
011729:graphics_w(0E0) = 17010000
011729:graphics_w(0E0) = 2D007FFF
011729:graphics_w(0E0) = 2E000800
011729:graphics_w(0E0) = 00000000
011729:graphics_w(0E0) = 1757500A
011729:graphics_w(0E0) = 175600FF
011729:graphics_w(0E0) = 17524321
011729:graphics_w(0E0) = 0100C0B0
011729:graphics_w(0E0) = 3F001059

011729:graphics_w(0E0) = 0100C0B0
011729:graphics_w(0E0) = 3F001059

011841:graphics_w(0E0) = 1A000000
011841:graphics_w(0E0) = 1A000000

00D09A:graphics_w(080) = FB22FC3F
00D01E:graphics_w(076) = F8800000
00D020:graphics_w(0CC) = 00800000
00D034:graphics_w(084) = 00000000
00D0A7:graphics_r(0F2) = 00000000
00D0C9:graphics_w(068) = 00000100
00D0CA:graphics_w(000) = 00000000
00D0CC:graphics_w(05C) = 00000000
00D0CE:graphics_w(006) = FFFF0030
00D0D0:graphics_w(002) = 000C0000
00D0D2:graphics_w(004) = 00000E01
00D0D4:graphics_w(04C) = 00808080
00D0D5:graphics_w(04E) = 00808080
00D0D6:graphics_w(008) = 00000000
00D0D7:graphics_w(00C) = 00FF018F
00D0DD:graphics_w(00A) = 0000018F
00D0E3:graphics_w(00E) = 00FF0000
00D0E5:graphics_w(01E) = FFFFFFFF
00D0E6:graphics_w(01C) = FFFFFFFF
00D0E7:graphics_w(01A) = FFFFFFFF
00D0E8:graphics_w(018) = FFFFFFFF
00D0EA:graphics_w(046) = 00000000
00D0EB:graphics_w(044) = 00000000
00D0EC:graphics_w(042) = 00000000
00D0ED:graphics_w(040) = 00000000
00D0EE:graphics_w(026) = 00000000
00D0EF:graphics_w(024) = 00000000
00D0F0:graphics_w(022) = 00000000
00D0F1:graphics_w(020) = 00000000
00D09A:graphics_w(080) = FB22FCFF
00D0F6:graphics_w(060) = 00000001
00D09A:graphics_w(080) = FB22FC3F
011729:graphics_w(0E0) = 1757500A
011729:graphics_w(0E0) = 175600FF
00D09A:graphics_w(080) = 0222FC3F

*/

INLINE UINT32 *graphics_pixaddr(void)
{
	int p = (graphics_ram[0xb7] >> 15) & 1;
	int x = (graphics_ram[0xb4] & 0xff) << 1;
	int y = ((graphics_ram[0xb4] >> 8) & 0x01) | ((graphics_ram[0xb5] << 1) & 0xffe);
	return &waveram[(p * WAVERAM_HEIGHT + (y % WAVERAM_HEIGHT)) * WAVERAM_WIDTH + x];
}


READ32_HANDLER( graphics_r )
{
	UINT32 result;
	int logit = 1;

	if (offset >= 0xb0 && offset <= 0xb7)
		logit = graphics_ram[0xb4] < 8;

	result = graphics_ram[offset & ~1] | (graphics_ram[offset | 1] << 16);

	switch (offset)
	{
		case 0x00:
			// crusnexo wants bit 0x20 to be non-zero
			result = 0x20;
			break;
		case 0x51:
			// crusnexo expects a reflection of the data at 0x08 here (b425)
			result = graphics_ram[0x08];
			break;
		case 0xf4:
			result = 6;
			logit = 0;
			break;
		case 0xf6:		// status -- they wait for this & 9 == 0
			result = 0;
			logit = 0;
			break;
	}

	if (logit || code_pressed(KEYCODE_F11))
		logerror("%06X:graphics_r(%03X) = %08X\n", activecpu_get_pc(), offset, result);
	return result;
}


/*
    Writes to even addresses: reg[offs] = data & 0xffff; reg[offs+1] = data >> 16;
    Writes to odd addresses:  reg[offs] = data & 0xffff;

    Control in $B7.
    Enable in $B6.
    Y/2 in $B5.
    X | ((Y & 1) << 8) in $B4.
    Depth data in $B2/$B3.
    Pixel data in $B0/$B1.

    Control = $80F6 when doing initial screen clear.
       Writes 256 consecutive values to $B0, must mean autoincrement
       Uses latched value in $B2 (maybe also in $B0)

    Control = $82F0 when doing video RAM test write phase.
       Writes 512 values across, but updates address before each one.

    Control = $84F1 when doing video RAM test read phase.
       Reads 512 values across, but updates address before each one.

    Control = $80F2 when doing screen clear.
       Writes 256 consecutive values to $B0, must mean autoincrement.
       Uses latched value in $B2 (maybe also in $B0)

    Control = $80A0/$8050 when plotting pixels.

    Control = $42F0 when doing wave RAM test write phase.
       Writes 512 values across, but updates address before each one.

    Control = $44F1 when doing wave RAM test read phase.
       Reads 512 values across, but updates address before each one.


    Low bits = operation?
      $0 = write single
      $1 = read single
      $2 = write autoinc
*/

WRITE32_HANDLER( graphics_w )
{
	int logit = 1;

	if (offset >= 0xb0 && offset <= 0xb7)
		logit = graphics_ram[0xb4] < 8;

	if (!(offset & 1))
		graphics_ram[offset | 0] = data & 0xffff;
	graphics_ram[offset | 1] = data >> 16;

	switch (offset)
	{
		case 0xb0:
		case 0xb2:
		{
			UINT32 *dest = graphics_pixaddr();
			if (code_pressed(KEYCODE_F11)) logerror("B7=%04X x=%3d y=%3d B0=%04X%04X B2=%04X%04X\n",
					graphics_ram[0xb7],
					(graphics_ram[0xb4] & 0xff) << 1,
					((graphics_ram[0xb4] >> 8) & 0x01) | ((graphics_ram[0xb5] << 1) & 0xffe),
					graphics_ram[0xb0],graphics_ram[0xb1],
					graphics_ram[0xb2],graphics_ram[0xb3]);

			if (graphics_ram[0xb7] != 0)
			{
				if (graphics_ram[0xb7] & 0x0050)
					dest[0] = (graphics_ram[0xb2] << 16) | graphics_ram[0xb0];
				if (graphics_ram[0xb7] & 0x00a0)
					dest[1] = (graphics_ram[0xb3] << 16) | graphics_ram[0xb1];
				switch (graphics_ram[0xb7] & 3)
				{
					case 0:
					case 1:
						break;
					case 2:
					case 3:
						graphics_ram[0xb4]++;
						break;
				}
{
	static int count = 0;
	popmessage("WaveRAM writes = %X", ++count);
}
			}
			break;
		}

		case 0xb4:
		{
			if (graphics_ram[0xb7] & 1)
			{
				UINT32 *src = graphics_pixaddr();
				graphics_ram[0xb0] = src[0] & 0xffff;
				graphics_ram[0xb1] = src[1] & 0xffff;
				graphics_ram[0xb2] = src[0] >> 16;
				graphics_ram[0xb3] = src[1] >> 16;
			}
			break;
		}
	}

	if (logit || code_pressed(KEYCODE_F11))
		logerror("%06X:graphics_w(%03X) = %08X\n", activecpu_get_pc(), offset, data);
}



/*************************************
 *
 *  Machine init
 *
 *************************************/

static MACHINE_RESET( midzeus )
{
	memcpy(ram_base, memory_region(REGION_USER1), 0x20000*4);

	*(UINT32 *)ram_base *= 2;

	timer[0] = timer_alloc(NULL);
	timer[1] = timer_alloc(NULL);
}



/*************************************
 *
 *  CMOS access
 *
 *************************************/

static WRITE32_HANDLER( cmos_protect_w )
{
	cmos_protected = 0;
}


static WRITE32_HANDLER( cmos_w )
{
	if (!cmos_protected)
		COMBINE_DATA(generic_nvram32 + offset);
}


static READ32_HANDLER( cmos_r )
{
	return generic_nvram32[offset];
}



/*************************************
 *
 *  TMS32031 I/O accesses
 *
 *************************************/

static READ32_HANDLER( tms32031_control_r )
{
	/* watch for accesses to the timers */
	if (offset == 0x24 || offset == 0x34)
	{
		/* timer is clocked at 100ns */
		int which = (offset >> 4) & 1;
		INT32 result = timer_timeelapsed(timer[which]) * 10000000.;
		return result;
	}

	/* log anything else except the memory control register */
	if (offset != 0x64)
		logerror("%06X:tms32031_control_r(%02X)\n", activecpu_get_pc(), offset);

	return tms32031_control[offset];
}


static WRITE32_HANDLER( tms32031_control_w )
{
	COMBINE_DATA(&tms32031_control[offset]);

	/* ignore changes to the memory control register */
	if (offset == 0x64)
		;

	/* watch for accesses to the timers */
	else if (offset == 0x20 || offset == 0x30)
	{
		int which = (offset >> 4) & 1;
		if (data & 0x40)
			timer_adjust(timer[which], TIME_NEVER, 0, TIME_NEVER);
	}
	else
		logerror("%06X:tms32031_control_w(%02X) = %08X\n", activecpu_get_pc(), offset, data);
}



/*************************************
 *
 *  Memory maps
 *
 *************************************/


// read 8d0003, check bit 1, skip some stuff if 0
// write junk to 9e0000

static UINT32 *unknown_8d0000;
static READ32_HANDLER( unknown_8d0000_r )
{
	return unknown_8d0000[offset];
}
static WRITE32_HANDLER( unknown_8d0000_w )
{
	logerror("%06X:write to %06X = %08X\n", activecpu_get_pc(), 0x8d0000 + offset, data);
	COMBINE_DATA(&unknown_8d0000[offset]);
}


static ADDRESS_MAP_START( main_map, ADDRESS_SPACE_PROGRAM, 32 )
	ADDRESS_MAP_FLAGS( AMEF_UNMAP(1) )
	AM_RANGE(0x000000, 0x03ffff) AM_RAM AM_BASE(&ram_base)
	AM_RANGE(0x400000, 0x41ffff) AM_RAM
	AM_RANGE(0x808000, 0x80807f) AM_READWRITE(tms32031_control_r, tms32031_control_w) AM_BASE(&tms32031_control)
	AM_RANGE(0x87fe00, 0x87ffff) AM_RAM
	AM_RANGE(0x880000, 0x8803ff) AM_READWRITE(graphics_r, graphics_w) AM_BASE(&graphics_ram)
	AM_RANGE(0x8d0000, 0x8d0003) AM_READWRITE(unknown_8d0000_r, unknown_8d0000_w) AM_BASE(&unknown_8d0000)
	AM_RANGE(0x990000, 0x99000f) AM_READWRITE(midway_ioasic_r, midway_ioasic_w)
//9d0000 -- page select?
	AM_RANGE(0x9e0000, 0x9e0000) AM_WRITENOP		// watchdog?
	AM_RANGE(0x9f0000, 0x9f7fff) AM_READWRITE(cmos_r, cmos_w) AM_BASE(&generic_nvram32) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0x9f8000, 0x9f8000) AM_WRITE(cmos_protect_w)
	AM_RANGE(0xa00000, 0xffffff) AM_ROM AM_REGION(REGION_USER1, 0)
ADDRESS_MAP_END



/*************************************
 *
 *  Input ports
 *
 *************************************/

INPUT_PORTS_START( mk4 )
	PORT_START	    /* DS1 */
 	PORT_DIPNAME( 0x0001, 0x0001, "Fatalities" )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
 	PORT_DIPNAME( 0x0002, 0x0002, "Blood" )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
 	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unknown )) /* Manual states that switches 3-7 are Unused */
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
 	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ))
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0008, DEF_STR( On ))
 	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unknown ))
 	PORT_DIPSETTING(      0x0010, DEF_STR( Off ))
 	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ))
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ))
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ))
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0100, 0x0100, "Coinage Source" )
	PORT_DIPSETTING(      0x0100, "Dipswitch" )
	PORT_DIPSETTING(      0x0000, "CMOS" )
	PORT_DIPNAME( 0x3e00, 0x3e00, DEF_STR( Coinage ))
	PORT_DIPSETTING(      0x3e00, "USA-1" )
	PORT_DIPSETTING(      0x3c00, "USA-2" )
	PORT_DIPSETTING(      0x3a00, "USA-3" )
	PORT_DIPSETTING(      0x3800, "USA-4" )
	PORT_DIPSETTING(      0x3400, "USA-9" )
	PORT_DIPSETTING(      0x3200, "USA-11" )
	PORT_DIPSETTING(      0x3600, "USA-ECA" )
	PORT_DIPSETTING(      0x2e00, "German-1" )
	PORT_DIPSETTING(      0x2c00, "German-2" )
	PORT_DIPSETTING(      0x2a00, "German-3" )
	PORT_DIPSETTING(      0x2800, "German-4" )
	PORT_DIPSETTING(      0x2600, "German-ECA" )
	PORT_DIPSETTING(      0x1e00, "French-1" )
	PORT_DIPSETTING(      0x1c00, "French-2" )
	PORT_DIPSETTING(      0x1a00, "French-3" )
	PORT_DIPSETTING(      0x1800, "French-4" )
	PORT_DIPSETTING(      0x1600, "French-ECA" )
	PORT_DIPSETTING(      0x3000, DEF_STR( Free_Play ))
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ))	/* Manual lists this dip as Unused */
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x8000, 0x8000, "Test Switch" )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_TILT ) /* Slam Switch */
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_SERVICE) PORT_NAME(DEF_STR( Service_Mode )) PORT_CODE(KEYCODE_F2)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_START4 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_VOLUME_DOWN )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_VOLUME_UP )
	PORT_BIT( 0x6000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_BILL1 )	/* Bill */

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    ) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  ) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  ) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    ) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  ) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  ) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_PLAYER(2)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_PLAYER(2)
	PORT_BIT( 0xff80, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END


INPUT_PORTS_START( invasn )
	PORT_START	    /* DS1 */
	PORT_DIPNAME( 0x0001, 0x0001, "Coinage Source" )
	PORT_DIPSETTING(      0x0001, "Dipswitch" )
	PORT_DIPSETTING(      0x0000, "CMOS" )
	PORT_DIPNAME( 0x003e, 0x003e, DEF_STR( Coinage ))
	PORT_DIPSETTING(      0x003e, "USA-1" )
	PORT_DIPSETTING(      0x003c, "USA-2" )
	PORT_DIPSETTING(      0x003a, "USA-3" )
	PORT_DIPSETTING(      0x0038, "USA-4" )
	PORT_DIPSETTING(      0x0034, "USA-9" )
	PORT_DIPSETTING(      0x0032, "USA-10" )
	PORT_DIPSETTING(      0x0036, "USA-ECA" )
	PORT_DIPSETTING(      0x002e, "German-1" )
	PORT_DIPSETTING(      0x002c, "German-2" )
	PORT_DIPSETTING(      0x002a, "German-3" )
	PORT_DIPSETTING(      0x0028, "German-4" )
	PORT_DIPSETTING(      0x0024, "German-5" )
	PORT_DIPSETTING(      0x0026, "German-ECA" )
	PORT_DIPSETTING(      0x001e, "French-1" )
	PORT_DIPSETTING(      0x001c, "French-2" )
	PORT_DIPSETTING(      0x001a, "French-3" )
	PORT_DIPSETTING(      0x0018, "French-4" )
	PORT_DIPSETTING(      0x0014, "French-11" )
	PORT_DIPSETTING(      0x0012, "French-12" )
	PORT_DIPSETTING(      0x0016, "French-ECA" )
	PORT_DIPSETTING(      0x0030, DEF_STR( Free_Play ))
	PORT_DIPNAME( 0x0040, 0x0040, "Flip Y" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0080, 0x0000, "Test Switch" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0100, 0x0100, "Mirrored Display" )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0200, 0x0200, "Show Blood" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0200, DEF_STR( On ))
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Unknown ))
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unknown ))
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ))
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ))
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ))
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unknown ))
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_TILT ) /* Slam Switch */
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_SERVICE) PORT_NAME(DEF_STR( Service_Mode )) PORT_CODE(KEYCODE_F2)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_START4 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_VOLUME_DOWN )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_VOLUME_UP )
	PORT_BIT( 0x6000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_BILL1 )	/* Bill */

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    ) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  ) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  ) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    ) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  ) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  ) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_PLAYER(2)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_PLAYER(2)
	PORT_BIT( 0xff80, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END


INPUT_PORTS_START( crusnexo )
	PORT_START	    /* DS1 */
 	PORT_DIPNAME( 0x0001, 0x0001, "Game Type" )	/* Manual states "*DIP 1, Switch 1 MUST be set */
 	PORT_DIPSETTING(      0x0001, "Dedicated" )	/*   to OFF position for proper operation" */
 	PORT_DIPSETTING(      0x0000, "Kit" )
 	PORT_DIPNAME( 0x0002, 0x0002, "Seat Motion" )	/* For dedicated Sit Down models with Motion Seat */
  	PORT_DIPSETTING(      0x0002, DEF_STR( Off ))
  	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
 	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Cabinet ))
 	PORT_DIPSETTING(      0x0004, "Stand Up" )
 	PORT_DIPSETTING(      0x0000, "Sit Down" )
 	PORT_DIPNAME( 0x0008, 0x0008, "Wheel Invert" )
  	PORT_DIPSETTING(      0x0000, DEF_STR( Off ))
  	PORT_DIPSETTING(      0x0008, DEF_STR( On ))
 	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unknown ))	/* Manual lists this dip as Unused */
 	PORT_DIPSETTING(      0x0010, DEF_STR( Off ))
  	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
 	PORT_DIPNAME( 0x0020, 0x0020, "Link" )
 	PORT_DIPSETTING(      0x0020, DEF_STR( Off ))
  	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
 	PORT_DIPNAME( 0x00c0, 0x00c0, "Linking I.D.")
 	PORT_DIPSETTING(      0x00c0, "Master #1" )
 	PORT_DIPSETTING(      0x0080, "Slave #2" )
 	PORT_DIPSETTING(      0x0040, "Slave #3" )
 	PORT_DIPSETTING(      0x0000, "Slave #4" )
 	PORT_DIPNAME( 0x1f00, 0x1f00, "Country Code" )
 	PORT_DIPSETTING(      0x1f00, DEF_STR( USA ) )
 	PORT_DIPSETTING(      0x1e00, "Germany" )
 	PORT_DIPSETTING(      0x1d00, "France" )
 	PORT_DIPSETTING(      0x1c00, "Canada" )
 	PORT_DIPSETTING(      0x1b00, "Switzerland" )
 	PORT_DIPSETTING(      0x1a00, "Italy" )
 	PORT_DIPSETTING(      0x1900, "UK" )
 	PORT_DIPSETTING(      0x1800, "Spain" )
 	PORT_DIPSETTING(      0x1700, "Austrilia" )
 	PORT_DIPSETTING(      0x1600, DEF_STR( Japan ) )
 	PORT_DIPSETTING(      0x1500, "Taiwan" )
 	PORT_DIPSETTING(      0x1400, "Austria" )
 	PORT_DIPSETTING(      0x1300, "Belgium" )
 	PORT_DIPSETTING(      0x0f00, "Sweden" )
 	PORT_DIPSETTING(      0x0e00, "Findland" )
	PORT_DIPSETTING(      0x0d00, "Netherlands" )
 	PORT_DIPSETTING(      0x0c00, "Norway" )
 	PORT_DIPSETTING(      0x0b00, "Denmark" )
 	PORT_DIPSETTING(      0x0a00, "Hungary" )
 	PORT_DIPSETTING(      0x0800, "General" )
 	PORT_DIPNAME( 0x6000, 0x6000, "Coin Mode" )
 	PORT_DIPSETTING(      0x6000, "Mode 1" ) /* USA1/GER1/FRA1/SPN1/AUSTRIA1/GEN1/CAN1/SWI1/ITL1/JPN1/TWN1/BLGN1/NTHRLND1/FNLD1/NRWY1/DNMK1/HUN1 */
 	PORT_DIPSETTING(      0x4000, "Mode 2" ) /* USA3/GER1/FRA1/SPN1/AUSTRIA1/GEN3/CAN2/SWI2/ITL2/JPN2/TWN2/BLGN2/NTHRLND2 */
 	PORT_DIPSETTING(      0x2000, "Mode 3" ) /* USA7/GER1/FRA1/SPN1/AUSTRIA1/GEN5/CAN3/SWI3/ITL3/JPN3/TWN3/BLGN3 */
 	PORT_DIPSETTING(      0x0000, "Mode 4" ) /* USA8/GER1/FRA1/SPN1/AUSTRIA1/GEN7 */
	PORT_DIPNAME( 0x8000, 0x8000, "Test Switch" )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_TILT ) /* Slam Switch */
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_SERVICE) PORT_NAME(DEF_STR( Service_Mode )) PORT_CODE(KEYCODE_F2)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_START4 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_VOLUME_DOWN )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_VOLUME_UP )
	PORT_BIT( 0x6000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_BILL1 )	/* Bill */

	PORT_START /* Listed "names" are via the manual's "JAMMA" pinout sheet" */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    ) PORT_PLAYER(1) PORT_8WAY /* Not Used */
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  ) PORT_PLAYER(1) PORT_8WAY /* Radio Switch */
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  ) PORT_PLAYER(1) PORT_8WAY /* Not Used */
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1) PORT_8WAY /* Not Used */
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1) /* View 2 */
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1) /* View 3 */
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1) /* View 1 */
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    ) PORT_PLAYER(2) PORT_8WAY /* Gear 1 */
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  ) PORT_PLAYER(2) PORT_8WAY /* Gear 2 */
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  ) PORT_PLAYER(2) PORT_8WAY /* Gear 3 */
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2) PORT_8WAY /* Gear 4 */
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2) /* Not Used */
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2) /* Not Used */
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2) /* Not Used */
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_PLAYER(2)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_PLAYER(2)
	PORT_BIT( 0xff80, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END


INPUT_PORTS_START( thegrid )
	PORT_START	    /* DS1 */
 	PORT_DIPNAME( 0x0001, 0x0001, "Show Blood" )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
 	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown )) /* Manual states that switches 2-7 are Unused */
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unknown ))
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
 	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ))
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0008, DEF_STR( On ))
 	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unknown ))
 	PORT_DIPSETTING(      0x0010, DEF_STR( Off ))
 	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ))
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ))
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ))
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0100, 0x0100, "Coinage Source" )
	PORT_DIPSETTING(      0x0100, "Dipswitch" )
	PORT_DIPSETTING(      0x0000, "CMOS" )
	PORT_DIPNAME( 0x3e00, 0x3e00, DEF_STR( Coinage ))
	PORT_DIPSETTING(      0x3e00, "USA-1" )
	PORT_DIPSETTING(      0x3800, "USA-2" )
	PORT_DIPSETTING(      0x3c00, "USA-10" )
	PORT_DIPSETTING(      0x3a00, "USA-14" )
	PORT_DIPSETTING(      0x3600, "USA-DC1" )
	PORT_DIPSETTING(      0x3000, "USA-DC2" )
	PORT_DIPSETTING(      0x3200, "USA-DC4" )
	PORT_DIPSETTING(      0x3400, "USA-DC5" )
	PORT_DIPSETTING(      0x2e00, "French-ECA1" )
	PORT_DIPSETTING(      0x2c00, "French-ECA2" )
	PORT_DIPSETTING(      0x2a00, "French-ECA3" )
	PORT_DIPSETTING(      0x2800, "French-ECA4" )
	PORT_DIPSETTING(      0x2600, "French-ECA5" )
	PORT_DIPSETTING(      0x2400, "French-ECA6" )
	PORT_DIPSETTING(      0x2200, "French-ECA7" )
	PORT_DIPSETTING(      0x2000, "French-ECA8" )
	PORT_DIPSETTING(      0x1e00, "German-1" )
	PORT_DIPSETTING(      0x1c00, "German-2" )
	PORT_DIPSETTING(      0x1a00, "German-3" )
	PORT_DIPSETTING(      0x1800, "German-4" )
	PORT_DIPSETTING(      0x1600, "German-5" )
	PORT_DIPSETTING(      0x1400, "German-ECA1" )
	PORT_DIPSETTING(      0x1200, "German-ECA2" )
	PORT_DIPSETTING(      0x1000, "German-ECA3" )
	PORT_DIPSETTING(      0x0800, "UK-4" )
	PORT_DIPSETTING(      0x0600, "UK-5" )
	PORT_DIPSETTING(      0x0e00, "UK-1 ECA" )
	PORT_DIPSETTING(      0x0c00, "UK-2 ECA" )
	PORT_DIPSETTING(      0x0a00, "UK-3 ECA" )
	PORT_DIPSETTING(      0x0400, "UK-6 ECA" )
	PORT_DIPSETTING(      0x0200, "UK-7 ECA" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ))
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ))	/* Manual states switches 7 & 8 are Unused */
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unknown ))
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_TILT ) /* Slam Switch */
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_SERVICE) PORT_NAME(DEF_STR( Service_Mode )) PORT_CODE(KEYCODE_F2)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_START4 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_VOLUME_DOWN )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_VOLUME_UP )
	PORT_BIT( 0x6000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_BILL1 )	/* Bill */

	PORT_START /* Listed "names" are via the manual's "JAMMA" pinout sheet" */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    ) PORT_PLAYER(1) PORT_8WAY /* Not Used */
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  ) PORT_PLAYER(1) PORT_8WAY /* Not Used */
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  ) PORT_PLAYER(1) PORT_8WAY /* Not Used */
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1) PORT_8WAY /* Not Used */
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1) /* Trigger */
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1) /* Fire */
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1) /* Action */
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    ) PORT_PLAYER(2) PORT_8WAY /* No Connection */
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  ) PORT_PLAYER(2) PORT_8WAY /* No Connection */
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  ) PORT_PLAYER(2) PORT_8WAY /* No Connection */
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2) PORT_8WAY /* No Connection */
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2) /* No Connection */
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2) /* No Connection */
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2) /* No Connection */
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_PLAYER(2)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_PLAYER(2)
	PORT_BIT( 0xff80, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END



/*************************************
 *
 *  Machine drivers
 *
 *************************************/

MACHINE_DRIVER_START( midzeus )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", TMS32031, 50000000)		// actually, TMS32032
	MDRV_CPU_PROGRAM_MAP(main_map,0)
	MDRV_CPU_VBLANK_INT(irq0_line_assert,1)

	MDRV_SCREEN_REFRESH_RATE(57)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_RESET(midzeus)
	MDRV_NVRAM_HANDLER(generic_1fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(512, 432)
	MDRV_SCREEN_VISIBLE_AREA(0, 511, 0, 399)
	MDRV_PALETTE_LENGTH(32768)

	MDRV_VIDEO_START(midzeus)
	MDRV_VIDEO_UPDATE(midzeus)

	/* sound hardware */
	MDRV_IMPORT_FROM(dcs2_audio_2104)
MACHINE_DRIVER_END



/*************************************
 *
 *  ROM definitions
 *
 *************************************/

ROM_START( mk4 )
	ROM_REGION16_LE( 0x1000000, REGION_SOUND1, ROMREGION_ERASEFF )	/* sound data */
	ROM_LOAD16_BYTE( "mk4_l2.2", 0x000000, 0x200000, CRC(4c01b493) SHA1(dcd6da9ee30d0f6645bfdb1762d926be0a26090a) )
	ROM_LOAD16_BYTE( "mk4_l2.3", 0x400000, 0x200000, CRC(8fbcf0ac) SHA1(c53704e72cfcba800c7af3a03267041f1e29a784) )
	ROM_LOAD16_BYTE( "mk4_l2.4", 0x800000, 0x200000, CRC(dee91696) SHA1(00a182a36a414744cd014fcfc53c2e1a66ab5189) )
	ROM_LOAD16_BYTE( "mk4_l2.5", 0xc00000, 0x200000, CRC(44d072be) SHA1(8a636c2801d799dfb84e69607ade76d2b49cf09f) )

	ROM_REGION32_LE( 0x1800000, REGION_USER1, 0 )
	ROM_LOAD32_WORD( "mk4_l3.10", 0x0000000, 0x200000, CRC(84efe5a9) SHA1(e2a9bf6fab971691017371a87ab87b1bf66f96d0) )
	ROM_LOAD32_WORD( "mk4_l3.11", 0x0000002, 0x200000, CRC(0c026ccb) SHA1(7531fe81ff8d8dd9ec3cd915acaf14cbe6bdc90a) )
	ROM_LOAD32_WORD( "mk4_l3.12", 0x0400000, 0x200000, CRC(7816c07f) SHA1(da94b4391e671f915c61b5eb9bece4acb3382e31) )
	ROM_LOAD32_WORD( "mk4_l3.13", 0x0400002, 0x200000, CRC(b3c237cd) SHA1(9e71e60cc92c17524f85f36543c174ca138104cd) )
	ROM_LOAD32_WORD( "mk4_l3.14", 0x0800000, 0x200000, CRC(fd33eb1a) SHA1(59d9d2e5251679d19cab031f51731c85f429ba18) )
	ROM_LOAD32_WORD( "mk4_l3.15", 0x0800002, 0x200000, CRC(b907518f) SHA1(cfb56538746895bdca779957fec6a872019b23c3) )
	ROM_LOAD32_WORD( "mk4_l3.16", 0x0c00000, 0x200000, CRC(24371d57) SHA1(c90134b17c23a182d391d1679bf457d251e641f7) )
	ROM_LOAD32_WORD( "mk4_l3.17", 0x0c00002, 0x200000, CRC(3a1a082c) SHA1(5f8e8ce760d8ebadd1240ef08f1382a37cf11d0b) )
ROM_END


ROM_START( invasn )
	ROM_REGION16_LE( 0x1000000, REGION_SOUND1, ROMREGION_ERASEFF )	/* sound data */
	ROM_LOAD16_BYTE( "invau2", 0x000000, 0x200000, CRC(59d2e1d6) SHA1(994a4311ac4841d4341449c0c7480952b6f3855d) )
	ROM_LOAD16_BYTE( "invau3", 0x400000, 0x200000, CRC(86b956ae) SHA1(f7fd4601a2ce3e7e9b67e7d77908bfa206ee7e62) )
	ROM_LOAD16_BYTE( "invau4", 0x800000, 0x200000, CRC(5ef1fab5) SHA1(987afa0672fa89b18cf20d28644848a9e5ee9b17) )
	ROM_LOAD16_BYTE( "invau5", 0xc00000, 0x200000, CRC(e42805c9) SHA1(e5b71eb1852809a649ac43a82168b3bdaf4b1526) )

	ROM_REGION32_LE( 0x1800000, REGION_USER1, 0 )
	ROM_LOAD32_WORD( "invau10", 0x0000000, 0x200000, CRC(b3ce958b) SHA1(ed51c167d85bc5f6155b8046ec056a4f4ad5cf9d) )
	ROM_LOAD32_WORD( "invau11", 0x0000002, 0x200000, CRC(0bd09359) SHA1(f40886bd2e5f5fbf506580e5baa2f733be200852) )
	ROM_LOAD32_WORD( "invau12", 0x0400000, 0x200000, CRC(ce1eb06a) SHA1(ff17690a0cbca6dcccccde70e2c5812ae03db5bb) )
	ROM_LOAD32_WORD( "invau13", 0x0400002, 0x200000, CRC(33fc6707) SHA1(11a39ad980ec320547319eca6ffa5aef3ab8b010) )
	ROM_LOAD32_WORD( "invau14", 0x0800000, 0x200000, CRC(760682a1) SHA1(ff91210225d4aa750115c6219d4c35c9521a3f0b) )
	ROM_LOAD32_WORD( "invau15", 0x0800002, 0x200000, CRC(90467d7a) SHA1(a143a3d3605e5626852e75937160ba6bcd891608) )
	ROM_LOAD32_WORD( "invau16", 0x0c00000, 0x200000, CRC(3ef1b28d) SHA1(6f9a071b8830194fea84daa62aadabae86977c5f) )
	ROM_LOAD32_WORD( "invau17", 0x0c00002, 0x200000, CRC(97aa677a) SHA1(4d21cc59e0ffd4985f89c97c71d605c3b404a8a3) )
	ROM_LOAD32_WORD( "invau18", 0x1000000, 0x200000, CRC(6930c656) SHA1(28054ff9a6c6f5764a371f8defe4c1f5730618f3) )
	ROM_LOAD32_WORD( "invau19", 0x1000002, 0x200000, CRC(89fa6ee5) SHA1(572565e1308142b0b062aa72315c68e928f2419c) )
ROM_END


ROM_START( crusnexo )
	ROM_REGION16_LE( 0xc00000, REGION_SOUND1, ROMREGION_ERASEFF )	/* sound data */
	ROM_LOAD( "exotica.u2", 0x000000, 0x200000, CRC(d2d54acf) SHA1(2b4d6fda30af807228bb281335939dfb6df9b530) )
	ROM_RELOAD(             0x200000, 0x200000 )
	ROM_LOAD( "exotica.u3", 0x400000, 0x400000, CRC(28a3a13d) SHA1(8d7d641b883df089adefdd144229afef79db9e8a) )
	ROM_LOAD( "exotica.u4", 0x800000, 0x400000, CRC(213f7fd8) SHA1(8528d524a62bc41a8e3b39f0dbeeba33c862ee27) )

	ROM_REGION32_LE( 0x3000000, REGION_USER1, 0 )
	ROM_LOAD32_WORD( "exotica.u10", 0x0000000, 0x200000, CRC(65450140) SHA1(cad41a2cad48426de01feb78d3f71f768e3fc872) )
	ROM_LOAD32_WORD( "exotica.u11", 0x0000002, 0x200000, CRC(e994891f) SHA1(bb088729b665864c7f3b79b97c3c86f9c8f68770) )
	ROM_LOAD32_WORD( "exotica.u12", 0x0400000, 0x200000, CRC(21f122b2) SHA1(5473401ec954bf9ab66a8283bd08d17c7960cd29) )
	ROM_LOAD32_WORD( "exotica.u13", 0x0400002, 0x200000, CRC(cf9d3609) SHA1(6376891f478185d26370466bef92f0c5304d58d3) )
	ROM_LOAD32_WORD( "exotica.u14", 0x0800000, 0x400000, CRC(84452fc2) SHA1(06d87263f83ef079e6c5fb9de620e0135040c858) )
	ROM_LOAD32_WORD( "exotica.u15", 0x0800002, 0x400000, CRC(b6aaebdb) SHA1(6ede6ea123be6a88d1ff38e90f059c9d1f822d6d) )
	ROM_LOAD32_WORD( "exotica.u16", 0x1000000, 0x400000, CRC(aac6d2a5) SHA1(6c336520269d593b46b82414d9352a3f16955cc3) )
	ROM_LOAD32_WORD( "exotica.u17", 0x1000002, 0x400000, CRC(71cf5404) SHA1(a6eed1a66fb4f4ddd749e4272a2cdb8e3e354029) )
	ROM_LOAD32_WORD( "exotica.u18", 0x1800000, 0x200000, CRC(60cf5caa) SHA1(629870a305802d632bd2681131d1ffc0086280d2) )
	ROM_LOAD32_WORD( "exotica.u19", 0x1800002, 0x200000, CRC(6b919a18) SHA1(20e40e195554146ed1d3fad54f7280823ae89d4b) )
	ROM_LOAD32_WORD( "exotica.u20", 0x1c00002, 0x200000, CRC(4855b68b) SHA1(1f6e557590b2621d0d5c782b95577f1be5cbc51d) )
	ROM_LOAD32_WORD( "exotica.u21", 0x1c00002, 0x200000, CRC(0011b9d6) SHA1(231d768c964a16b905857b0814d758fe93c2eefb) )
	ROM_LOAD32_WORD( "exotica.u22", 0x2000002, 0x400000, CRC(ad6dcda7) SHA1(5c9291753e1659f9adbe7e59fa2d0e030efae5bc) )
	ROM_LOAD32_WORD( "exotica.u23", 0x2000002, 0x400000, CRC(1f103a68) SHA1(3b3acc63a461677cd424e75e7211fa6f063a37ef) )
	ROM_LOAD32_WORD( "exotica.u24", 0x2800002, 0x400000, CRC(6312feef) SHA1(4113e4e5d39c99e8131d41a57c973df475b67d18) )
	ROM_LOAD32_WORD( "exotica.u25", 0x2800002, 0x400000, CRC(b8277b16) SHA1(1355e87affd78e195906aedc9aed9e230374e2bf) )
ROM_END


ROM_START( thegrid )
	ROM_REGION16_LE( 0xc00000, REGION_SOUND1, ROMREGION_ERASEFF )	/* sound data */
	ROM_LOAD( "the_grid.u2", 0x000000, 0x400000, CRC(e6a39ee9) SHA1(4ddc62f5d278ea9791205098fa5f018ab1e698b4) )
	ROM_LOAD( "the_grid.u3", 0x400000, 0x400000, CRC(40be7585) SHA1(e481081edffa07945412a6eab17b4d3e7b42cfd3) )
	ROM_LOAD( "the_grid.u4", 0x800000, 0x400000, CRC(7a15c203) SHA1(a0a49dd08bba92402640ed2d1fb4fee112c4ab5f) )

	ROM_REGION32_LE( 0x3000000, REGION_USER1, 0 )
	ROM_LOAD32_WORD( "the_grid.u10", 0x0000000, 0x100000, BAD_DUMP CRC(3219c028) SHA1(956cb358c4c52d4fa7e870796e92880fb2b94147) )
	ROM_LOAD32_WORD( "the_grid.u11", 0x0000002, 0x100000, BAD_DUMP CRC(540dd6f9) SHA1(02af52bdbe266cdf51294b31e1db71c1ee723d0d) )
	ROM_LOAD32_WORD( "the_grid.u12", 0x0200000, 0x100000, BAD_DUMP CRC(4e9f7031) SHA1(4df99e0edc5510067521c554227921d2185070c4) )
	ROM_LOAD32_WORD( "the_grid.u13", 0x0200002, 0x100000, BAD_DUMP CRC(268252ca) SHA1(80080e0244bb8ba2ff7dd513ea263b5f5dc54bb6) )
	ROM_LOAD32_WORD( "the_grid.u18", 0x0400000, 0x400000, CRC(3a3460be) SHA1(e719dae8a2e54584cb6a074ed42e35e3debef2f6) )
	ROM_LOAD32_WORD( "the_grid.u19", 0x0400002, 0x400000, CRC(af262d5b) SHA1(3eb3980fa81a360a70aa74e793b2bc3028f68cf2) )
	ROM_LOAD32_WORD( "the_grid.u20", 0x0c00002, 0x400000, CRC(e6ad1917) SHA1(acab25e1251fd07b374badebe79f6ec1772b3589) )
	ROM_LOAD32_WORD( "the_grid.u21", 0x0c00002, 0x400000, CRC(48c03f8e) SHA1(50790bdae9f2234ffb4914c2c5c16374e3508b47) )
	ROM_LOAD32_WORD( "the_grid.u22", 0x1400002, 0x400000, CRC(84c3a8b6) SHA1(de0dcf9daf7ada7a6952b9e29a29571b2aa9d0b2) )
	ROM_LOAD32_WORD( "the_grid.u23", 0x1400002, 0x400000, CRC(f48ef409) SHA1(79d74b4fe38b06a02ae0351d13d7f0a7ed0f0c87) )
ROM_END



/*************************************
 *
 *  Driver init
 *
 *************************************/

static DRIVER_INIT( mk4 )
{
	dcs2_init(0, 0);
	midway_ioasic_init(MIDWAY_IOASIC_STANDARD, 461/* or 474 */, 94, NULL);
	midway_ioasic_set_shuffle_state(1);
}


static READ32_HANDLER( gun_r )
{
	return ~0;
}
static DRIVER_INIT( invasn )
{
	dcs2_init(0, 0);
	midway_ioasic_init(MIDWAY_IOASIC_STANDARD, 461/* unknown */, 94, NULL);
	memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0x9c0000, 0x9c0000, 0, 0, gun_r);
}


static DRIVER_INIT( crusnexo )
{
	dcs2_init(0, 0);
	midway_ioasic_init(MIDWAY_IOASIC_STANDARD, 461/* unknown */, 94, NULL);
}


static DRIVER_INIT( thegrid )
{
	cpunum_set_input_line(0, INPUT_LINE_HALT, ASSERT_LINE);
	dcs2_init(0, 0);
	midway_ioasic_init(MIDWAY_IOASIC_STANDARD, 461/* unknown */, 94, NULL);
}



/*************************************
 *
 *  Game drivers
 *
 *************************************/

GAME( 1997, mk4,      0,     midzeus, mk4,      mk4,      ROT0, "Midway", "Mortal Kombat 4 (rev L3)", GAME_NOT_WORKING | GAME_NO_SOUND )
GAME( 1999, invasn,   0,     midzeus, invasn,   invasn,   ROT0, "Midway", "Invasion (Midway)", GAME_NOT_WORKING | GAME_NO_SOUND )
GAME( 1999, crusnexo, 0,     midzeus, crusnexo, crusnexo, ROT0, "Midway", "Cruis'n Exotica", GAME_NOT_WORKING | GAME_NO_SOUND )
GAME( 2001, thegrid,  0,     midzeus, thegrid,  thegrid,  ROT0, "Midway", "The Grid", GAME_NOT_WORKING | GAME_NO_SOUND )

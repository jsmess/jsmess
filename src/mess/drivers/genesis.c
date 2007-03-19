/***************************************************************************

Sega Genesis/MegaDrive Memory Map (preliminary)

Main CPU (68k)

$000000 - $3fffff : Cartridge ROM
$400000 - $9fffff : Reserved by Sega (unused?)
$a00000 - $a0ffff : shared RAM w/Z80
$a10000 - $a10fff : IO
$a11000 - $a11fff : Controls
	$a11000 : memory mode (16-bit)
	$a11100 : Z80 bus request (16)
	$a11200 : Z80 reset (16)
$c00000 - $dfffff : Video Display Processor (VDP)
	$c00000 : data (16-bit, mirrored)
	$c00004 : control (16, m)
	$c00008 : hv counter (16, m)
	$c00011 : psg 76489 (8)
$ff0000 - $ffffff : RAM

Interrupts:
	level 6 - vertical
	level 4 - horizontal
	level 2 - external

Sound CPU (Z80)

$0000 - $1fff : RAM
$2000 - $3fff : Reserved (RAM?)
$4001 - $4003 : ym2612 registers
$6000         : bank register
$7f11         : psg 76489
$8000 - $ffff : shared RAM w/68k


Gareth S. Long

In Memory Of Haruki Ikeda

***************************************************************************/

#include "driver.h"
#include "sound/2612intf.h"
#include "video/generic.h"
#include "includes/genesis.h"
#include "devices/cartslot.h"
#include "inputx.h"

int genesis_region;
int genesis_is_ntsc;

/* code taken directly from GoodGEN by Cowering */
static int genesis_isfunkySMD(unsigned char *buf,unsigned int len)
{
	/* aq quiz */
	if (!strncmp("UZ(-01  ", (const char *) &buf[0xf0], 8))
		return 1;

    /* Phelios USA redump */
	/* target earth */
	/* klax (namcot) */
	if (buf[0x2080] == ' ' && buf[0x0080] == 'S' && buf[0x2081] == 'E' && buf[0x0081] == 'G')
		return 1;

    /* jap baseball 94 */
	if (!strncmp("OL R-AEAL", (const char *) &buf[0xf0], 9))
		return 1;

    /* devilish Mahjonng Tower */
    if (!strncmp("optrEtranet", (const char *) &buf[0xf3], 11))
		return 1;

	/* golden axe 2 beta */
	if (buf[0x0100] == 0x3c && buf[0x0101] == 0 && buf[0x0102] == 0 && buf[0x0103] == 0x3c)
		return 1;

    /* omega race */
	if (!strncmp("OEARC   ", (const char *) &buf[0x90], 8))
		return 1;

    /* budokan beta */
	if ((len >= 0x6708+8) && !strncmp(" NTEBDKN", (const char *) &buf[0x6708], 8))
		return 1;

    /* cdx pro 1.8 bios */
	if (!strncmp("so fCXP", (const char *) &buf[0x2c0], 7))
		return 1;

    /* ishido (hacked) */
	if (!strncmp("sio-Wyo ", (const char *) &buf[0x0090], 8))
		return 1;

    /* onslaught */
	if (!strncmp("SS  CAL ", (const char *) &buf[0x0088], 8))
		return 1;

    /* tram terror pirate */
	if ((len >= 0x3648 + 8) && !strncmp("SG NEPIE", (const char *) &buf[0x3648], 8))
		return 1;

    /* breath of fire 3 chinese */
	if (buf[0x0007] == 0x1c && buf[0x0008] == 0x0a && buf[0x0009] == 0xb8 && buf[0x000a] == 0x0a)
		return 1;

    /*tetris pirate */
	if ((len >= 0x1cbe + 5) && !strncmp("@TTI>", (const char *) &buf[0x1cbe], 5))
		return 1;

	return 0;
}



/* code taken directly from GoodGEN by Cowering */
static int genesis_isSMD(unsigned char *buf,unsigned int len)
{
	if (buf[0x2080] == 'S' && buf[0x80] == 'E' && buf[0x2081] == 'G' && buf[0x81] == 'A')
		return 1;
	return genesis_isfunkySMD(buf,len);
}



static int genesis_isfunkyBIN(unsigned char *buf,unsigned int len)
{
	/* all the special cases for crappy headered roms */
	/* aq quiz */
	if (!strncmp("QUIZ (G-3051", (const char *) &buf[0x1e0], 12))
		return 1;

	/* phelios USA redump */
	/* target earth */
	/* klax namcot */
	if (buf[0x0104] == 'A' && buf[0x0101] == 'S' && buf[0x0102] == 'E' && buf[0x0103] == 'G')
		return 1;

    /* jap baseball 94 */
	if (!strncmp("WORLD PRO-B", (const char *) &buf[0x1e0], 11))
		return 1;

    /* devlish mahj tower */
	if (!strncmp("DEVILISH MAH", (const char *) &buf[0x120], 12))
		return 1;

    /* golden axe 2 beta */
	if ((len >= 0xe40a+4) && !strncmp("SEGA", (const char *) &buf[0xe40a], 4))
		return 1;

    /* omega race */
	if (!strncmp(" OMEGA RAC", (const char *) &buf[0x120], 10))
		return 1;

    /* budokan beta */
	if ((len >= 0x4e18+8) && !strncmp("BUDOKAN.", (const char *) &buf[0x4e18], 8))
		return 1;

    /* cdx 1.8 bios */
	if ((len >= 0x588+8) && !strncmp(" CDX PRO", (const char *) &buf[0x588], 8))
		return 1;

    /* ishido (hacked) */
	if (!strncmp("Ishido - ", (const char *) &buf[0x120], 9))
		return 1;

    /* onslaught */
	if (!strncmp("(C)ACLD 1991", (const char *) &buf[0x118], 12))
		return 1;

    /* trampoline terror pirate */
	if ((len >= 0x2c70+9) && !strncmp("DREAMWORK", (const char *) &buf[0x2c70], 9))
		return 1;

    /* breath of fire 3 chinese */
	if (buf[0x000f] == 0x1c && buf[0x0010] == 0x00 && buf[0x0011] == 0x0a && buf[0x0012] == 0x5c)
		return 1;

    /* tetris pirate */
	if ((len >= 0x397f+6) && !strncmp("TETRIS", (const char *) &buf[0x397f], 6))
		return 1;

    return 0;
}



static int genesis_isBIN(unsigned char *buf,unsigned int len)
{
	if (buf[0x0100] == 'S' && buf[0x0101] == 'E' && buf[0x0102] == 'G' && buf[0x0103] == 'A')
		return 1;
	return genesis_isfunkyBIN(buf,len);
}



static int genesis_verify_cart(unsigned char *temp,unsigned int len)
{
	int retval = IMAGE_VERIFY_FAIL;

	/* is this an SMD file..? */
	if (genesis_isSMD(&temp[0x200],len))
		retval = IMAGE_VERIFY_PASS;

	/* How about a BIN file..? */
	if ((retval == IMAGE_VERIFY_FAIL) && genesis_isBIN(&temp[0],len))
		retval = IMAGE_VERIFY_PASS;

	/* maybe a .md file? (rare) */
	if ((retval == IMAGE_VERIFY_FAIL) && (temp[0x080] == 'E') && (temp[0x081] == 'A') && (temp[0x082] == 'M' || temp[0x082] == 'G'))
		retval = IMAGE_VERIFY_PASS;

	if (retval == IMAGE_VERIFY_FAIL)
		logerror("Invalid Image!\n");

	return retval;
}

int device_load_genesis_cart(mess_image *image)
{
	unsigned char *tmpROMnew, *tmpROM;
	unsigned char *secondhalf;
	unsigned char *rawROM;
	int relocate;
	int length;
	int ptr, x;
	unsigned char *ROM;

	rawROM = memory_region(REGION_USER1);
    ROM = rawROM /*+ 512 */;

//	genesis_soundram = memory_region(REGION_CPU2);

    length = image_fread(image, rawROM + 0x2000, 0x400200);
	logerror("image length = 0x%x\n", length);

	if (length < 1024 + 512)
		goto bad;						/* smallest known rom is 1.7K */

	if (genesis_verify_cart(&rawROM[0x2000],(unsigned)length) == IMAGE_VERIFY_FAIL)
		goto bad;

	if (genesis_isSMD(&rawROM[0x2200],(unsigned)length))	/* is this a SMD file..? */
	{

		tmpROMnew = ROM;
		tmpROM = ROM + 0x2000 + 512;

		for (ptr = 0; ptr < (0x400000) / (8192); ptr += 2)
		{
			for (x = 0; x < 8192; x++)
			{
				*tmpROMnew++ = *(tmpROM + ((ptr + 1) * 8192) + x);
				*tmpROMnew++ = *(tmpROM + ((ptr + 0) * 8192) + x);
			}
		}

		relocate = 0;

	}
	else
	/* check if it's a MD file */
	if ((rawROM[0x2080] == 'E') &&
		(rawROM[0x2081] == 'A') &&
		(rawROM[0x2082] == 'M' || rawROM[0x2082] == 'G'))  /* is this a MD file..? */
	{

		tmpROMnew = malloc(length);
		secondhalf = &tmpROMnew[length >> 1];

		if (!tmpROMnew)
		{
			logerror("Memory allocation failed reading roms!\n");
			goto bad;
		}

		memcpy(tmpROMnew, ROM + 0x2000, length);

		for (ptr = 0; ptr < length; ptr += 2)
		{

			ROM[ptr] = secondhalf[ptr >> 1];
			ROM[ptr + 1] = tmpROMnew[ptr >> 1];
		}
		free(tmpROMnew);
		relocate = 0;

	}
	else
	/* BIN it is, then */
	{
		relocate = 0x2000;
	}

	ROM = memory_region(REGION_USER1);	/* 68000 ROM region */

	for (ptr = 0; ptr < 0x402000; ptr += 2)		/* mangle bytes for littleendian machines */
	{
#ifdef LSB_FIRST
		int temp = ROM[relocate + ptr];

		ROM[ptr] = ROM[relocate + ptr + 1];
		ROM[ptr + 1] = temp;
#else
		ROM[ptr] = ROM[relocate + ptr];
		ROM[ptr + 1] = ROM[relocate + ptr + 1];
#endif
	}

	return INIT_PASS;

bad:
	return INIT_FAIL;
}



/* Main Genesis 68k */

static ADDRESS_MAP_START( genesis_68000_mem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x3fffff) AM_ROM AM_BASE(&genesis_cartridge) // cartridge area
	AM_RANGE(0xa00000, 0xa07fff) AM_READWRITE(genesis_68000_z80_read, genesis_68000_z80_write) AM_MIRROR(0x8000) // z80 area
	      /* 0xa08000, 0xa0ffff mirrors above */
	AM_RANGE(0xa10000, 0xa1001f) AM_READWRITE(genesis_68000_io_r, genesis_68000_io_w)
	AM_RANGE(0xa11100, 0xa11101) AM_READWRITE(genesis_68000_z80_busreq_r, genesis_68000_z80_busreq_w)
	AM_RANGE(0xa11200, 0xa11201) AM_WRITE(genesis_68000_z80_reset_w)

	AM_RANGE(0xc00000, 0xdfffff) AM_READWRITE(genesis_68000_vdp_r, genesis_68000_vdp_w) /* 0x20 in size, masked in handler */

	AM_RANGE(0xe00000, 0xe0ffff) AM_RAMBANK(2) AM_MIRROR(0x1f0000)
	      /* 0xe10000, 0xffffff mirrors above */
ADDRESS_MAP_END



static ADDRESS_MAP_START( genesis_z80_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_RAM	// AM_MIRROR(0x2000)
	      /* 0x2000, 0x3fff mirrors above */
//	AM_RANGE(0x4000, 0x5fff) AM_READ(z80_ym2612_r)
	AM_RANGE(0x4000, 0x4000) AM_READ(YM2612_status_port_0_A_r)
	AM_RANGE(0x4000, 0x5fff) AM_WRITE(z80_ym2612_w)
//	AM_RANGE(0x4000, 0x4000) AM_WRITE(YM2612_control_port_0_A_w)
//	AM_RANGE(0x4001, 0x4001) AM_WRITE(YM2612_data_port_0_A_w)
//	AM_RANGE(0x4002, 0x4002) AM_WRITE(YM2612_control_port_0_B_w)
//	AM_RANGE(0x4003, 0x4003) AM_WRITE(YM2612_data_port_0_B_w)

	AM_RANGE(0x6000, 0x60ff) AM_WRITE(genesis_z80_bank_sel_w)

	AM_RANGE(0x7f00, 0x7fff) AM_WRITE(genesis_z80_vdp_w)
//	AM_RANGE(0x7f00, 0x7fff) AM_READ(genesis_z80_vdp_r)

	AM_RANGE(0x8000, 0xffff) AM_READ(genesis_banked_68k_r)

//	AM_RANGE(0x8000, 0xffff) AM_READ(MRA8_RAM)


ADDRESS_MAP_END



INPUT_PORTS_START( genesis )
	PORT_START	/* IN0 player 1 controller */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN) PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT) PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT) PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_NAME("P1 Button A")  PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2) PORT_NAME("P1 Button B")  PORT_PLAYER(1)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON3) PORT_NAME("P1 Button C")  PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START) PORT_NAME("P1 Start")  PORT_PLAYER(1)

	PORT_START	/* IN1 player 2 controller */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP) PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN) PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT) PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT) PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_NAME("P2 Button A")  PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2) PORT_NAME("P2 Button B")  PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON3) PORT_NAME("P2 Button C")  PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START) PORT_NAME("P2 Start")  PORT_PLAYER(2)
INPUT_PORTS_END



static struct YM2612interface ym3438_interface =
{
	NULL	/*	ym3438_interrupt */
};



static MACHINE_DRIVER_START( gen_ntsc )

	MDRV_CPU_ADD_TAG("main", M68000, 7670000)
	MDRV_CPU_PROGRAM_MAP(genesis_68000_mem, 0)
	MDRV_CPU_VBLANK_INT(genesis_interrupt,262) // use timers instead?

	MDRV_CPU_ADD(Z80, 3580000)
	MDRV_CPU_PROGRAM_MAP(genesis_z80_mem, 0)

	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(TIME_IN_USEC(0))

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_RGB32)
	MDRV_SCREEN_SIZE(128*8, 128*8)
	MDRV_SCREEN_VISIBLE_AREA(0*8, 40*8-1, 0*8, 28*8-1)
//	MDRV_SCREEN_VISIBLE_AREA(0*8, 128*8-1, 0*8, 128*8-1)

	MDRV_PALETTE_LENGTH(0x200)

	MDRV_INTERLEAVE(100)


	MDRV_MACHINE_RESET(genesis)

	MDRV_VIDEO_START(genesis)
	MDRV_VIDEO_UPDATE(genesis)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")
	MDRV_SOUND_ADD(YM2612, 7670000)
	MDRV_SOUND_CONFIG(ym3438_interface)		/* 8 MHz ?? */
	MDRV_SOUND_ROUTE(0, "left", 0.50)
	MDRV_SOUND_ROUTE(1, "right", 0.50)
	MDRV_SOUND_ADD(SN76496, 3580000)	/* 3.58 MHz */
	MDRV_SOUND_ROUTE(0, "left", 0.50)
	MDRV_SOUND_ADD(SN76496, 3580000)	/* 3.58 MHz */
	MDRV_SOUND_ROUTE(1, "right", 0.50)

MACHINE_DRIVER_END

static MACHINE_DRIVER_START( gen_pal )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(gen_ntsc)
//	MDRV_CPU_MODIFY("main")
//	MDRV_CPU_VBLANK_INT(genesis_interrupt,xxx) // use timers instead? more per frame?

	MDRV_SCREEN_REFRESH_RATE(50)

	/* video hardware */
	MDRV_SCREEN_VISIBLE_AREA(0*8, 40*8-1, 0*8, 30*8-1)
MACHINE_DRIVER_END

/* we don't use the bios rom (its not needed and only provides security on early models) */

ROM_START(gen_usa)
	ROM_REGION(0x415000, REGION_CPU1, 0)
	ROM_REGION(0x405000, REGION_USER1, 0)
	ROM_REGION( 0x10000, REGION_CPU2, 0)
ROM_END

ROM_START(gen_eur)
	ROM_REGION(0x415000, REGION_CPU1, 0)
	ROM_REGION(0x405000, REGION_USER1, 0)
	ROM_REGION( 0x10000, REGION_CPU2, 0)
ROM_END

ROM_START(gen_jpn)
	ROM_REGION(0x415000, REGION_CPU1, 0)
	ROM_REGION(0x405000, REGION_USER1, 0)
	ROM_REGION( 0x10000, REGION_CPU2, 0)
ROM_END

static void genesis_cartslot_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cartslot */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 1; break;
		case DEVINFO_INT_MUST_BE_LOADED:				info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_LOAD:							info->load = device_load_genesis_cart; break;
		case DEVINFO_PTR_PARTIAL_HASH:					info->partialhash = NULL;	/*genesis_partialhash*/ break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "smd,bin,md"); break;

		default:										cartslot_device_getinfo(devclass, state, info); break;
	}
}

SYSTEM_CONFIG_START(genesis)
	CONFIG_DEVICE(genesis_cartslot_getinfo)
SYSTEM_CONFIG_END

DRIVER_INIT(gen_usa)
{
	genesis_is_ntsc = 1;   // vdp status flag ...
	genesis_region = 0x80; // read via io

/*
    D7 : Console is 1= Export (USA, Europe, etc.) 0= Domestic (Japan)
    D6 : Video type is 1= PAL, 0= NTSC
*/
	init_genesis(machine);
}


DRIVER_INIT(gen_eur)
{
	genesis_is_ntsc = 0;   // vdp status flag ...
	genesis_region = 0xc0; // read via io

	init_genesis(machine);
}


DRIVER_INIT(gen_jpn)
{
	genesis_is_ntsc = 1;   // vdp status flag ...
	genesis_region = 0x00; // read via io

	init_genesis(machine);
}


/***************************************************************************

  Game driver(s)

***************************************************************************/

/*	  YEAR	NAME	  PARENT	COMPAT	MACHINE   INPUT 	INIT		CONFIG		COMPANY	FULLNAME */
CONS( 1988, gen_usa,  0,		0,		gen_ntsc,  genesis,	gen_usa,	genesis,	"Sega",   "Genesis (USA, NTSC)" , 0)
CONS( 1988, gen_eur,  gen_usa,	0,		gen_pal,   genesis,	gen_eur,	genesis,	"Sega",   "Megadrive (Europe, PAL)" , 0)
CONS( 1988, gen_jpn,  gen_usa,	0,		gen_ntsc,  genesis,	gen_jpn,	genesis,	"Sega",   "Megadrive (Japan, NTSC)" , 0)

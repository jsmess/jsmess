/******************************************************************
 *
 *  Fairchild Channel F driver
 *
 *  Juergen Buchmueller
 *  Frank Palazzolo
 *  Sean Riddle
 *
 *  Fredric "e5frog" Blaoholtz, added support large cartridges
 *    also spanning from $3000 to $FFFF. Added clones
 *
 ******************************************************************/

#include "emu.h"
#include "cpu/f8/f8.h"
#include "includes/channelf.h"
#include "devices/cartslot.h"


#ifndef VERBOSE
#define VERBOSE 0
#endif

#define LOG(x)	do { if (VERBOSE) logerror x; } while (0)

#define MASTER_CLOCK_PAL	2000000  /* PAL unit has a separate crystal at 4.000 MHz */
#define PAL_VBLANK_TIME     4623


/* The F8 has latches on its port pins
 * These mimic's their behavior
 * [0]=port0, [1]=port1, [2]=port4, [3]=port5
 *
 * Note: this should really be moved into f8.c,
 * but it's complicated by possible peripheral usage.
 *
 * If the read/write operation is going external to the F3850
 * (or F3853, etc.), then the latching applies.  However, relaying the
 * port read/writes from the F3850 to a peripheral like the F3853
 * should not be latched in this way. (See mk1 driver)
 *
 * The f8 cannot determine how its ports are mapped at runtime,
 * so it can't easily decide to latch or not.
 *
 * ...so it stays here for now.
 */

static UINT8 latch[6];	/* SKR - inc by 2 for 2102 ports */

static UINT8 port_read_with_latch(UINT8 ext, UINT8 latch_state)
{
	return (~ext | latch_state);
}

static READ8_HANDLER( channelf_port_0_r )
{
	return port_read_with_latch(input_port_read(space->machine, "PANEL"),latch[0]);
}

static READ8_HANDLER( channelf_port_1_r )
{
	UINT8 ext_value;

	if ((latch[0] & 0x40) == 0)
	{
		ext_value = input_port_read(space->machine, "RIGHT_C");
	}
	else
	{
		ext_value = 0xc0 | input_port_read(space->machine, "RIGHT_C");
	}
	return port_read_with_latch(ext_value,latch[1]);
}

static READ8_HANDLER( channelf_port_4_r )
{
	UINT8 ext_value;

	if ((latch[0] & 0x40) == 0)
	{
		ext_value = input_port_read(space->machine, "LEFT_C");
	}
	else
	{
		ext_value = 0xff;
	}
	return port_read_with_latch(ext_value,latch[2]);
}

static READ8_HANDLER( channelf_port_5_r )
{
	return port_read_with_latch(0xff,latch[3]);
}

static struct {	/* SKR - 2102 RAM chip on carts 10 and 18 I/O ports */
	UINT8 d;			/* data bit:inverted logic, but reading/writing cancel out */
	UINT8 r_w;			/* inverted logic: 0 means read, 1 means write */
	UINT8 a[10];		/* addr bits: inverted logic, but reading/writing cancel out */
	UINT16 addr;		/* calculated addr from addr bits */
	UINT8 ram[1024];	/* RAM array */
} r2102;

static  READ8_HANDLER( channelf_2102A_r )	/* SKR */
{
	UINT8 pdata;

	if(r2102.r_w==0) {
		r2102.addr=(r2102.a[0]&1)+((r2102.a[1]<<1)&2)+((r2102.a[2]<<2)&4)+((r2102.a[3]<<3)&8)+((r2102.a[4]<<4)&16)+((r2102.a[5]<<5)&32)+((r2102.a[6]<<6)&64)+((r2102.a[7]<<7)&128)+((r2102.a[8]<<8)&256)+((r2102.a[9]<<9)&512);
		r2102.d=r2102.ram[r2102.addr]&1;
		pdata=latch[4]&0x7f;
		pdata|=(r2102.d<<7);
		LOG(("rhA: addr=%d, d=%d, r_w=%d, ram[%d]=%d,  a[9]=%d, a[8]=%d, a[7]=%d, a[6]=%d, a[5]=%d, a[4]=%d, a[3]=%d, a[2]=%d, a[1]=%d, a[0]=%d\n",r2102.addr,r2102.d,r2102.r_w,r2102.addr,r2102.ram[r2102.addr],r2102.a[9],r2102.a[8],r2102.a[7],r2102.a[6],r2102.a[5],r2102.a[4],r2102.a[3],r2102.a[2],r2102.a[1],r2102.a[0]));
		return port_read_with_latch(0xff,pdata);
	} else
		LOG(("rhA: r_w=%d\n",r2102.r_w));
		return port_read_with_latch(0xff,latch[4]);
}

static  READ8_HANDLER( channelf_2102B_r )  /* SKR */
{
	LOG(("rhB\n"));
	return port_read_with_latch(0xff,latch[5]);
}

static WRITE8_HANDLER( channelf_port_0_w )
{
	int offs;

	latch[0] = data;

    if (data & 0x20)
	{
		offs = channelf_row_reg*128+channelf_col_reg;
		if (space->machine->generic.videoram.u8[offs] != channelf_val_reg)
			space->machine->generic.videoram.u8[offs] = channelf_val_reg;
	}
}

static WRITE8_HANDLER( channelf_port_1_w )
{
	latch[1] = data;
    channelf_val_reg = ((data ^ 0xff) >> 6) & 0x03;
}

static WRITE8_HANDLER( channelf_port_4_w )
{
    latch[2] = data;
    channelf_col_reg = (data | 0x80) ^ 0xff;
}

static WRITE8_HANDLER( channelf_port_5_w )
{
    latch[3] = data;
	channelf_sound_w(space->machine, (data>>6)&3);
    channelf_row_reg = (data | 0xc0) ^ 0xff;
}

static WRITE8_HANDLER( channelf_2102A_w )  /* SKR */
{
	latch[4]=data;
	r2102.a[2]=(data>>2)&1;
	r2102.a[3]=(data>>1)&1;
	r2102.r_w=data&1;
	r2102.addr=(r2102.a[0]&1)+((r2102.a[1]<<1)&2)+((r2102.a[2]<<2)&4)+((r2102.a[3]<<3)&8)+((r2102.a[4]<<4)&16)+((r2102.a[5]<<5)&32)+((r2102.a[6]<<6)&64)+((r2102.a[7]<<7)&128)+((r2102.a[8]<<8)&256)+((r2102.a[9]<<9)&512);
	r2102.d=(data>>3)&1;
	if(r2102.r_w==1)
		r2102.ram[r2102.addr]=r2102.d;
	LOG(("whA: data=%d, addr=%d, d=%d, r_w=%d, ram[%d]=%d\n",data,r2102.addr,r2102.d,r2102.r_w,r2102.addr,r2102.ram[r2102.addr]));
}

static WRITE8_HANDLER( channelf_2102B_w )  /* SKR */
{
	latch[5]=data;
	r2102.a[9]=(data>>7)&1;
	r2102.a[8]=(data>>6)&1;
	r2102.a[7]=(data>>5)&1;
	r2102.a[1]=(data>>4)&1;
	r2102.a[6]=(data>>3)&1;
	r2102.a[5]=(data>>2)&1;
	r2102.a[4]=(data>>1)&1;
	r2102.a[0]=data&1;
	LOG(("whB: data=%d, a[9]=%d,a[8]=%d,a[0]=%d\n",data,r2102.a[9],r2102.a[8],r2102.a[0]));
}

static ADDRESS_MAP_START( channelf_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_ROM
	AM_RANGE(0x0800, 0x27ff) AM_ROM /* Cartridge Data */
	AM_RANGE(0x2800, 0x2fff) AM_RAM /* Schach RAM */
	AM_RANGE(0x3000, 0xffff) AM_ROM /* Cartridge Data continued */
ADDRESS_MAP_END

static ADDRESS_MAP_START( channelf_io, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x00, 0x00) AM_READWRITE(channelf_port_0_r, channelf_port_0_w) /* Front panel switches */
	AM_RANGE(0x01, 0x01) AM_READWRITE(channelf_port_1_r, channelf_port_1_w) /* Right controller     */
	AM_RANGE(0x04, 0x04) AM_READWRITE(channelf_port_4_r, channelf_port_4_w) /* Left controller      */
	AM_RANGE(0x05, 0x05) AM_READWRITE(channelf_port_5_r, channelf_port_5_w)

	AM_RANGE(0x20, 0x20) AM_READWRITE(channelf_2102A_r, channelf_2102A_w) /* SKR 2102 control and addr for cart 18 */
	AM_RANGE(0x21, 0x21) AM_READWRITE(channelf_2102B_r, channelf_2102B_w) /* SKR 2102 addr */
	AM_RANGE(0x24, 0x24) AM_READWRITE(channelf_2102A_r, channelf_2102A_w) /* SKR 2102 control and addr for cart 10 */
	AM_RANGE(0x25, 0x25) AM_READWRITE(channelf_2102B_r, channelf_2102B_w) /* SKR 2102 addr */
ADDRESS_MAP_END



static INPUT_PORTS_START( channelf )
	PORT_START("PANEL") /* Front panel buttons */
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_START )	/* TIME  (1) */
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON5 )	/* HOLD  (2) */
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON6 )	/* MODE  (3) */
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON7 )	/* START (4) */
	PORT_BIT ( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("RIGHT_C") /* Right controller */
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT) PORT_PLAYER(1)
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT) PORT_PLAYER(1)
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN) PORT_PLAYER(1)
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP) PORT_PLAYER(1)
	PORT_BIT ( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON4) /* C-CLOCKWISE */ PORT_PLAYER(1)
	PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON3) /* CLOCKWISE   */ PORT_PLAYER(1)
	PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON2) /* PULL UP     */ PORT_PLAYER(1)
	PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1) /* PUSH DOWN   */ PORT_PLAYER(1)

	PORT_START("LEFT_C") /* Left controller */
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT) PORT_PLAYER(2)
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT) PORT_PLAYER(2)
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN) PORT_PLAYER(2)
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP) PORT_PLAYER(2)
	PORT_BIT ( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON4) /* C-CLOCKWISE */ PORT_PLAYER(2)
	PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON3) /* CLOCKWISE   */ PORT_PLAYER(2)
	PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON2) /* PULL UP     */ PORT_PLAYER(2)
	PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1) /* PUSH DOWN   */ PORT_PLAYER(2)
INPUT_PORTS_END



static DEVICE_IMAGE_LOAD( channelf_cart )
{
	UINT32 size;

	if (image.software_entry() == NULL)
	{
		size = image.length();

		if (size > 0xf800)
		{
			image.seterror(IMAGE_ERROR_UNSPECIFIED, "Unsupported cartridge size");
			return IMAGE_INIT_FAIL;
		}

		if (image.fread( memory_region(image.device().machine, "maincpu") + 0x0800, size) != size)
		{
			image.seterror(IMAGE_ERROR_UNSPECIFIED, "Unable to fully read from file");
			return IMAGE_INIT_FAIL;
		}

	}
	else
	{
		size = image.get_software_region_length("rom");
		memcpy(memory_region(image.device().machine, "maincpu") + 0x0800, image.get_software_region("rom"), size);
	}

	return IMAGE_INIT_PASS;
}

static MACHINE_DRIVER_START( channelf_cart )
	/* cartridge */
	MDRV_CARTSLOT_ADD("cart")
	MDRV_CARTSLOT_EXTENSION_LIST("bin,chf")
	MDRV_CARTSLOT_INTERFACE("channelf_cart")
	MDRV_CARTSLOT_LOAD(channelf_cart)

	/* Software lists */
	MDRV_SOFTWARE_LIST_ADD("cart_list","channelf")
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( channelf )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", F8, 3579545/2)        /* Colorburst/2 */
	MDRV_CPU_PROGRAM_MAP(channelf_map)
	MDRV_CPU_IO_MAP(channelf_io)
	MDRV_QUANTUM_TIME(HZ(60))

	/* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(128, 64)
	MDRV_SCREEN_VISIBLE_AREA(4, 112 - 7, 4, 64 - 3)
	MDRV_PALETTE_LENGTH(8)
	MDRV_PALETTE_INIT( channelf )

	MDRV_VIDEO_START( channelf )
	MDRV_VIDEO_UPDATE( channelf )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("custom", CHANNELF, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	MDRV_IMPORT_FROM( channelf_cart )
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( sabavdpl )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", F8, MASTER_CLOCK_PAL)        /* PAL speed */
	MDRV_CPU_PROGRAM_MAP(channelf_map)
	MDRV_CPU_IO_MAP(channelf_io)
	MDRV_QUANTUM_TIME(HZ(50))

	/* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(PAL_VBLANK_TIME)) /* approximate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(128, 64)
	MDRV_SCREEN_VISIBLE_AREA(4, 112 - 7, 4, 64 - 3)
	MDRV_PALETTE_LENGTH(8)
	MDRV_PALETTE_INIT( channelf )

	MDRV_VIDEO_START( channelf )
	MDRV_VIDEO_UPDATE( channelf )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("custom", CHANNELF, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	MDRV_IMPORT_FROM( channelf_cart )
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( channlf2 )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", F8, 3579545/2)        /* Colorburst / 2 */
	MDRV_CPU_PROGRAM_MAP(channelf_map)
	MDRV_CPU_IO_MAP(channelf_io)
	MDRV_QUANTUM_TIME(HZ(60))

	/* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(128, 64)
	MDRV_SCREEN_VISIBLE_AREA(4, 112 - 7, 4, 64 - 3)
	MDRV_PALETTE_LENGTH(8)
	MDRV_PALETTE_INIT( channelf )

	MDRV_VIDEO_START( channelf )
	MDRV_VIDEO_UPDATE( channelf )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("custom", CHANNELF, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	MDRV_IMPORT_FROM( channelf_cart )
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( sabavpl2 )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", F8, MASTER_CLOCK_PAL)        /* PAL speed */
	MDRV_CPU_PROGRAM_MAP(channelf_map)
	MDRV_CPU_IO_MAP(channelf_io)
	MDRV_QUANTUM_TIME(HZ(50))

	/* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(PAL_VBLANK_TIME)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(128, 64)
	MDRV_SCREEN_VISIBLE_AREA(4, 112 - 7, 4, 64 - 3)
	MDRV_PALETTE_LENGTH(8)
	MDRV_PALETTE_INIT( channelf )

	MDRV_VIDEO_START( channelf )
	MDRV_VIDEO_UPDATE( channelf )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("custom", CHANNELF, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	MDRV_IMPORT_FROM( channelf_cart )
MACHINE_DRIVER_END

ROM_START( channelf )
	ROM_REGION(0x10000,"maincpu",0)
	ROM_SYSTEM_BIOS( 0, "sl90025", "Luxor Video Entertainment System" )
	ROMX_LOAD("sl90025.rom",  0x0000, 0x0400, CRC(015c1e38) SHA1(759e2ed31fbde4a2d8daf8b9f3e0dffebc90dae2), ROM_BIOS(1))
	ROM_SYSTEM_BIOS( 1, "sl31253", "Channel F" )
	ROMX_LOAD("sl31253.rom",  0x0000, 0x0400, CRC(04694ed9) SHA1(81193965a374d77b99b4743d317824b53c3e3c78), ROM_BIOS(2))
	ROM_LOAD("sl31254.rom",   0x0400, 0x0400, CRC(9c047ba3) SHA1(8f70d1b74483ba3a37e86cf16c849d601a8c3d2c))
ROM_END

#define rom_sabavdpl rom_channelf
#define rom_luxorves rom_channelf
#define rom_channlf2 rom_channelf
#define rom_sabavdpl rom_channelf
#define rom_sabavpl2 rom_channelf
#define rom_luxorvec rom_channelf
#define rom_itttelma rom_channelf
#define rom_ingtelma rom_channelf

/***************************************************************************

  Game driver(s)

***************************************************************************/

/*    YEAR  NAME        PARENT  COMPAT  MACHINE     INPUT     INIT   COMPANY         FULLNAME        FLAGS */
CONS( 1976, channelf,  0,        0,    channelf,  channelf,   0,      "Fairchild",    "Channel F",                            0 )
CONS( 1977, sabavdpl,  channelf, 0,    sabavdpl,  channelf,   0,      "SABA",         "SABA Videoplay",                       0 )
CONS( 197?, luxorves,  channelf, 0,    sabavdpl,  channelf,   0,      "Luxor",        "Luxor Video Entertainment System",     0 )
CONS( 1978, channlf2,  0, channelf,    channlf2,  channelf,   0,      "Fairchild",    "Channel F II",                         0 )
CONS( 1978, sabavpl2,  channlf2, 0,    sabavpl2,  channelf,   0,      "SABA",         "SABA Videoplay 2",                     0 )
CONS( 197?, luxorvec,  channlf2, 0,    sabavpl2,  channelf,   0,      "Luxor",        "Luxor Video Entertainment Computer",   0 )
CONS( 197?, itttelma,  channlf2, 0,    sabavpl2,  channelf,   0,      "ITT",          "ITT Tele-Match Processor",             0 )
CONS( 1978, ingtelma,  channlf2, 0,    sabavpl2,  channelf,   0,      "Ingelen",      "Ingelen Tele-Match Processor",         0 )

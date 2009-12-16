/*
    Nintendo 64

    Driver by Ville Linde
    Reformatted to share hardware by R. Belmont
    Additional work by Ryan Holtz
    Porting from Mupen64 by Harmony
*/

#include "driver.h"
#include "cpu/rsp/rsp.h"
#include "cpu/mips/mips3.h"
#include "sound/dmadac.h"
#include "devices/cartslot.h"
#include "includes/n64.h"

/*
static READ32_HANDLER(rsp_dmem_r)
{
	return rsp_dmem[offset];
}

static WRITE32_HANDLER(rsp_dmem_w)
{
	COMBINE_DATA(&rsp_dmem[offset]);
}

static READ32_HANDLER(rsp_imem_r)
{
	return rsp_imem[offset];
}

static WRITE32_HANDLER(rsp_imem_w)
{
	COMBINE_DATA(&rsp_imem[offset]);
}
*/

static ADDRESS_MAP_START( n64_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x007fffff) AM_RAM	AM_BASE(&rdram)				// RDRAM
	AM_RANGE(0x03f00000, 0x03f00027) AM_READWRITE(n64_rdram_reg_r, n64_rdram_reg_w)
	AM_RANGE(0x04000000, 0x04000fff) AM_RAM AM_BASE(&rsp_dmem) AM_SHARE("dmem")	// RSP DMEM
	AM_RANGE(0x04001000, 0x04001fff) AM_RAM AM_BASE(&rsp_imem) AM_SHARE("imem")	// RSP IMEM
	AM_RANGE(0x04040000, 0x040fffff) AM_READWRITE(n64_sp_reg_r, n64_sp_reg_w)	// RSP
	AM_RANGE(0x04100000, 0x041fffff) AM_READWRITE(n64_dp_reg_r, n64_dp_reg_w)	// RDP
	AM_RANGE(0x04300000, 0x043fffff) AM_READWRITE(n64_mi_reg_r, n64_mi_reg_w)	// MIPS Interface
	AM_RANGE(0x04400000, 0x044fffff) AM_READWRITE(n64_vi_reg_r, n64_vi_reg_w)	// Video Interface
	AM_RANGE(0x04500000, 0x045fffff) AM_READWRITE(n64_ai_reg_r, n64_ai_reg_w)	// Audio Interface
	AM_RANGE(0x04600000, 0x046fffff) AM_READWRITE(n64_pi_reg_r, n64_pi_reg_w)	// Peripheral Interface
	AM_RANGE(0x04700000, 0x047fffff) AM_READWRITE(n64_ri_reg_r, n64_ri_reg_w)	// RDRAM Interface
	AM_RANGE(0x04800000, 0x048fffff) AM_READWRITE(n64_si_reg_r, n64_si_reg_w)	// Serial Interface
	//AM_RANGE(0x08000000, 0x08007fff) AM_RAM									// Cartridge SRAM
	AM_RANGE(0x10000000, 0x13ffffff) AM_ROM AM_REGION("user2", 0)	// Cartridge
	AM_RANGE(0x1fc00000, 0x1fc007bf) AM_ROM AM_REGION("user1", 0)	// PIF ROM
	AM_RANGE(0x1fc007c0, 0x1fc007ff) AM_READWRITE(n64_pif_ram_r, n64_pif_ram_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( rsp_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x00000fff) AM_RAM AM_SHARE("dmem")
	AM_RANGE(0x00001000, 0x00001fff) AM_RAM AM_SHARE("imem")
	AM_RANGE(0x04000000, 0x04000fff) AM_RAM AM_SHARE("dmem")
	AM_RANGE(0x04001000, 0x04001fff) AM_RAM AM_SHARE("imem")
ADDRESS_MAP_END

static INPUT_PORTS_START( n64 )
	PORT_START("P1")
		PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_BUTTON1 )        PORT_PLAYER(1) PORT_NAME("Button A")
		PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_BUTTON2 )        PORT_PLAYER(1) PORT_NAME("Button B")
		PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_BUTTON3 )        PORT_PLAYER(1) PORT_NAME("Button Z")
		PORT_BIT( 0x1000, IP_ACTIVE_HIGH, IPT_START )          PORT_PLAYER(1) PORT_NAME("Start")
		PORT_BIT( 0x0800, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP )    PORT_PLAYER(1) PORT_NAME("Joypad \xE2\x86\x91") /* Up */
		PORT_BIT( 0x0400, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN )  PORT_PLAYER(1) PORT_NAME("Joypad \xE2\x86\x93") /* Down */
		PORT_BIT( 0x0200, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )  PORT_PLAYER(1) PORT_NAME("Joypad \xE2\x86\x90") /* Left */
		PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1) PORT_NAME("Joypad \xE2\x86\x92") /* Right */
		PORT_BIT( 0x00c0, IP_ACTIVE_HIGH, IPT_UNUSED )
		PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_BUTTON4 )        PORT_PLAYER(1) PORT_NAME("Button L")
		PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_BUTTON5 )        PORT_PLAYER(1) PORT_NAME("Button R")
		PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_BUTTON6 )        PORT_PLAYER(1) PORT_NAME("Button C \xE2\x86\x91") /* Up */
		PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_BUTTON7 )        PORT_PLAYER(1) PORT_NAME("Button C \xE2\x86\x93") /* Down */
		PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_BUTTON8 )        PORT_PLAYER(1) PORT_NAME("Button C \xE2\x86\x90") /* Left */
		PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_BUTTON9 )        PORT_PLAYER(1) PORT_NAME("Button C \xE2\x86\x92") /* Right */

	PORT_START("P1_ANALOG_X")
		PORT_BIT( 0xff, 0x80, IPT_AD_STICK_X ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(30) PORT_KEYDELTA(30) PORT_PLAYER(1)

	PORT_START("P1_ANALOG_Y")
		PORT_BIT( 0xff, 0x80, IPT_AD_STICK_Y ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(30) PORT_KEYDELTA(30) PORT_PLAYER(1) PORT_REVERSE

	PORT_START("P2")
		PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_BUTTON1 )        PORT_PLAYER(2) PORT_NAME("Button A")
		PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_BUTTON2 )        PORT_PLAYER(2) PORT_NAME("Button B")
		PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_BUTTON3 )        PORT_PLAYER(2) PORT_NAME("Button Z")
		PORT_BIT( 0x1000, IP_ACTIVE_HIGH, IPT_START )          PORT_PLAYER(2) PORT_NAME("Start")
		PORT_BIT( 0x0800, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP )    PORT_PLAYER(2) PORT_NAME("Joypad \xE2\x86\x91") /* Up */
		PORT_BIT( 0x0400, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN )  PORT_PLAYER(2) PORT_NAME("Joypad \xE2\x86\x93") /* Down */
		PORT_BIT( 0x0200, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )  PORT_PLAYER(2) PORT_NAME("Joypad \xE2\x86\x90") /* Left */
		PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2) PORT_NAME("Joypad \xE2\x86\x92") /* Right */
		PORT_BIT( 0x00c0, IP_ACTIVE_HIGH, IPT_UNUSED )
		PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_BUTTON4 )        PORT_PLAYER(2) PORT_NAME("Button L")
		PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_BUTTON5 )        PORT_PLAYER(2) PORT_NAME("Button R")
		PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_BUTTON6 )        PORT_PLAYER(2) PORT_NAME("Button C \xE2\x86\x91") /* Up */
		PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_BUTTON7 )        PORT_PLAYER(2) PORT_NAME("Button C \xE2\x86\x93") /* Down */
		PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_BUTTON8 )        PORT_PLAYER(2) PORT_NAME("Button C \xE2\x86\x90") /* Left */
		PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_BUTTON9 )        PORT_PLAYER(2) PORT_NAME("Button C \xE2\x86\x92") /* Right */

	PORT_START("P2_ANALOG_X")
		PORT_BIT( 0xff, 0x80, IPT_AD_STICK_X ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(30) PORT_KEYDELTA(30) PORT_PLAYER(2)

	PORT_START("P2_ANALOG_Y")
		PORT_BIT( 0xff, 0x80, IPT_AD_STICK_Y ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(30) PORT_KEYDELTA(30) PORT_PLAYER(2) PORT_REVERSE

INPUT_PORTS_END

/* ?? */
static const mips3_config config =
{
	16384,				/* code cache size */
	8192,				/* data cache size */
	62500000			/* system clock */
};

static INTERRUPT_GEN( n64_vblank )
{
	signal_rcp_interrupt(device->machine, VI_INTERRUPT);
}

static DEVICE_IMAGE_LOAD(n64_cart)
{
	int i, length;
	UINT8 *cart = memory_region(image->machine, "user2");

	length = image_fread(image, cart, 0x4000000);

	if (cart[0] == 0x37 && cart[1] == 0x80)
	{
		for (i=0; i < length; i+=4)
		{
			UINT8 b1 = cart[i+0];
			UINT8 b2 = cart[i+1];
			UINT8 b3 = cart[i+2];
			UINT8 b4 = cart[i+3];
			cart[i+0] = b3;
			cart[i+1] = b4;
			cart[i+2] = b1;
			cart[i+3] = b2;
		}
	}
	else
	{
		for (i=0; i < length; i+=4)
		{
			UINT8 b1 = cart[i+0];
			UINT8 b2 = cart[i+1];
			UINT8 b3 = cart[i+2];
			UINT8 b4 = cart[i+3];
			cart[i+0] = b4;
			cart[i+1] = b3;
			cart[i+2] = b2;
			cart[i+3] = b1;
		}
	}

	logerror("cart length = %d\n", length);
	return INIT_PASS;
}

static MACHINE_DRIVER_START( n64 )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", VR4300BE, 93750000)
	MDRV_CPU_CONFIG(config)
	MDRV_CPU_PROGRAM_MAP(n64_map)
	MDRV_CPU_VBLANK_INT("screen", n64_vblank)

	MDRV_CPU_ADD("rsp", RSP, 62500000)
	MDRV_CPU_CONFIG(n64_rsp_config)
	MDRV_CPU_PROGRAM_MAP(rsp_map)

	MDRV_MACHINE_START( n64 )
	MDRV_MACHINE_RESET( n64 )
	MDRV_QUANTUM_TIME(HZ(600))

	/* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(0))
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_RGB32)
	MDRV_SCREEN_SIZE(640, 525)
	MDRV_SCREEN_VISIBLE_AREA(0, 639, 0, 239)
	MDRV_PALETTE_LENGTH(0x1000)

	MDRV_VIDEO_START(n64)
	MDRV_VIDEO_UPDATE(n64)

	MDRV_SPEAKER_STANDARD_STEREO("lspeaker", "rspeaker")

	MDRV_SOUND_ADD("dac1", DMADAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "lspeaker", 1.0)
	MDRV_SOUND_ADD("dac2", DMADAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "rspeaker", 1.0)

	/* cartridge */
	MDRV_CARTSLOT_ADD("cart")
	MDRV_CARTSLOT_EXTENSION_LIST("v64,z64,rom,n64,bin")
	MDRV_CARTSLOT_MANDATORY
	MDRV_CARTSLOT_LOAD(n64_cart)
MACHINE_DRIVER_END

ROM_START( n64 )
    ROM_REGION( 0x800000, "maincpu", ROMREGION_ERASEFF )      /* dummy region for R4300 */

    ROM_REGION32_BE( 0x800, "user1", 0 )
    ROM_LOAD( "pifdata.bin", 0x0000, 0x0800, CRC(5ec82be9) SHA1(9174eadc0f0ea2654c95fd941406ab46b9dc9bdd) )

    ROM_REGION32_BE( 0x4000000, "user2", ROMREGION_ERASEFF)

	ROM_REGION16_BE( 0x80, "normpoint", 0 )
    ROM_LOAD( "normpnt.rom", 0x00, 0x80, CRC(e7f2a005) SHA1(c27b4a364a24daeee6e99fd286753fd6216362b4) )

	ROM_REGION16_BE( 0x80, "normslope", 0 )
    ROM_LOAD( "normslp.rom", 0x00, 0x80, CRC(4f2ae525) SHA1(eab43f8cc52c8551d9cff6fced18ef80eaba6f05) )
ROM_END

CONS(1996, n64, 	0,		0,		n64, 	n64, 	0,	0,	"Nintendo", "Nintendo 64", GAME_NOT_WORKING | GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )

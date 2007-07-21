/* Sega MegaTech */
/*
todo: cleanup, fix so that everything works properly
      rewrite genesis emulation (so that it works perfectly with every genesis game)
      games are marked as NOT WORKING due to a) bad genesis emulation, b) incorrect behavior at time out
       or c) they're sms based games which aren't yet supported


About MegaTech:

Megatech games are identical to their Genesis/SMS equivlents, however the Megatech cartridges contain
a BIOS rom with the game instructions.  The last part number of the bios ROM is the cart/game ID code.

In Megatech games your coins buy you time to play the game, how you perform in the game does not
matter, you can die and start a new game providing you still have time, likewise you can be playing
well and run out of time if you fail to insert more coins.  This is the same method Nintendo used
with their Playchoice 10 system.

The BIOS screen is based around SMS hardware, with an additional Z80 and SMS VDP chip not present on
a standard Genesis.

SMS games run on Megatech in the Genesis's SMS compatability mode, where the Genesis Z80 becomes the
main CPU and the Genesis VDP acts in a mode mimicing the behavior of the SMS VDP.

Additions will only be made to this driver if proof that the dumped set are original roms with original
Sega part numbers is given..

A fairly significant number of Genesis games were available for this system.


Sega Mega Tech Cartridges (Readme by Guru)
-------------------------

These are cart-based games for use with Sega Mega Tech hardware. There are 6 known types of carts. All carts
are very simple, almost exactly the same as Mega Play carts. They contain just 2 or 3 ROMs.
PCB 171-6215A has locations for 2 ROMs and is dated 1991. PCB 171-6215A is also used in Mega Play!
PCB 171-5782 has locations for 2 ROMs and is dated 1989.
PCB 171-5869A has locations for 3 ROMs and is dated 1989.
PCB 171-5834 has locations for 3 ROMs and is dated 1989.
PCB 171-5783 has locations for 2 ROMs and is dated 1989.
PCB 171-5784 has locations for 2 ROMs and is dated 1989. It also contains a custom Sega IC 315-5235

                                                                         |------------------------------- ROMs -----------------------------|
                                                                         |                                                                  |
Game                    PCB #       Sticker on PCB    Sticker on cart      IC1                    IC2                    IC3
---------------------------------------------------------------------------------------------------------------------------------------------
Space Harrier II        171-5782    837-6963-02       610-0239-02          MPR-11934    (834200)  EPR-12368-02 (27256)   n/a
Out Run                 171-5783    837-6963-06       610-0239-06          MPR-11078    (Mask)    EPR-12368-06 (27256)   n/a
Alien Syndrome          171-5783    837-6963-07       610-0239-07          MPR-11194    (232011)  EPR-12368-07 (27256)   n/a
Afterburner             171-5784    837-6963-10       610-0239-10          315-5235     (custom)  MPR-11271-T  (834000)  EPR-12368-10 (27256)
Tetris                  171-5834    837-6963-22       610-0239-22          MPR-12356F   (831000)  MPR-12357F   (831000)  EPR-12368-22 (27256)
Ghouls & Ghosts         171-5869A   -                 610-0239-23          MPR-12605    (40 pins) MPR-12606    (40 pins) EPR-12368-23 (27256)
Super Hang On           171-5782    837-6963-24       610-0239-24          MPR-12640    (234000)  EPR-12368-24 (27256)   n/a
Forgotten Worlds        171-5782    837-6963-26       610-0239-26          MPR-12672-H  (Mask)    EPR-12368-26 (27256)   n/a
Super Real Basket Ball  171-5782    837-6963-32       610-0239-32          MPR-12904F   (838200A) EPR-12368-32 (27256)   n/a
Sonic Hedgehog 2        171-6215A   837-6963-62       610-0239-62          MPR-15000A-F (838200)  EPR-12368-62 (27256)   n/a
Mario Lemeux Hockey     171-5782    837-6963-59       610-0239-59          MPR-14376-H  (234000)  EPR-12368-59 (27256)   n/a


*/
#include "driver.h"
#include "genesis.h"
#include "rendlay.h"

/* Megatech BIOS specific */
static UINT32 bios_port_ctrl;
UINT32 bios_ctrl_inputs;

#define MASTER_CLOCK		53693100


INPUT_PORTS_START( megatech ) /* Genesis Input Ports */
	PORT_START


	PORT_START	/* Player 1 Controls - part 2 */
//  PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
//  PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
//  PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
//  PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
//  PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_BUTTON2 )
//  PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_BUTTON3 )
//  PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNUSED )
//  PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* Player 1 Controls - part 1 */
//  PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
//  PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_START1 )


	PORT_START	/* Player 2 Controls - part 2 */
//  PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
//  PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
//  PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
//  PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT  ) PORT_PLAYER(2)
//  PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
//  PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
//  PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNUSED )
//  PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* Player 2 Controls - part 1 */
//  PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
//  PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_START1 ) PORT_PLAYER(2)

	PORT_START	/* Temp - Fake dipswitch to turn on / off sms vdp display */
//  PORT_DIPNAME( 0x01, 0x01, "SMS VDP Display (fake)" )
//  PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
//  PORT_DIPSETTING(    0x01, DEF_STR( On ) )

	PORT_START
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE2 ) PORT_NAME("Select") PORT_CODE(KEYCODE_0)
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN  ) PORT_NAME("0x6800 bit 1") PORT_CODE(KEYCODE_Y)
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN ) PORT_NAME("0x6800 bit 2") PORT_CODE(KEYCODE_U)
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN ) PORT_NAME("0x6800 bit 3") PORT_CODE(KEYCODE_I)
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SPECIAL ) PORT_NAME("Door 1") PORT_CODE(KEYCODE_K)
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SPECIAL ) PORT_NAME("Door 2") PORT_CODE(KEYCODE_L)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) PORT_NAME("0x6800 bit 6") PORT_CODE(KEYCODE_O)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Test mode") PORT_CODE(KEYCODE_F2)

	PORT_START
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_COIN1 )  // a few coin inputs here
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 ) PORT_NAME("Service coin") PORT_CODE(KEYCODE_9)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE3 ) PORT_NAME("Enter") PORT_CODE(KEYCODE_MINUS)
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT) PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT) PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN ) PORT_NAME("port DD bit 4")
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN ) PORT_NAME("port DD bit 5")
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) PORT_NAME("port DD bit 6")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN ) PORT_NAME("port DD bit 7")

	PORT_START	 // up, down, left, right, button 2,3, 2P up, down.
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)

    PORT_START	/* DSW A */
	PORT_DIPNAME( 0x02, 0x02, "Coin slot 3" )
	PORT_DIPSETTING (   0x00, "Inhibit" )
	PORT_DIPSETTING (   0x02, "Accept" )
	PORT_DIPNAME( 0x01, 0x01, "Coin slot 4" )
	PORT_DIPSETTING (   0x00, "Inhibit" )
	PORT_DIPSETTING (   0x01, "Accept" )
	PORT_DIPNAME( 0x1c, 0x1c, "Coin slot 3/4 value" )
	PORT_DIPSETTING(    0x1c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x14, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_8C ) )
	PORT_DIPSETTING(    0x00, "1 Coin/10 credits" )
	PORT_DIPNAME( 0xe0, 0x60, "Coin slot 2 value" )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x00, "Inhibit" )

	PORT_START /* DSW B */
	PORT_DIPNAME( 0x0f, 0x01, "Coin Slot 1 value" )
	PORT_DIPSETTING(    0x00, "Inhibit" )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_8C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 1C_9C ) )
	PORT_DIPSETTING(    0x0a, "1 coin/10 credits" )
	PORT_DIPSETTING(    0x0b, "1 coin/11 credits" )
	PORT_DIPSETTING(    0x0c, "1 coin/12 credits" )
	PORT_DIPSETTING(    0x0d, "1 coin/13 credits" )
	PORT_DIPSETTING(    0x0e, "1 coin/14 credits" )
	PORT_DIPSETTING(    0x0f, "1 coin/15 credits" )
	PORT_DIPNAME( 0xf0, 0xa0, "Time per credit" )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x10, "7:30" )
	PORT_DIPSETTING(    0x20, "7:00" )
	PORT_DIPSETTING(    0x30, "6:30" )
	PORT_DIPSETTING(    0x40, "6:00" )
	PORT_DIPSETTING(    0x50, "5:30" )
	PORT_DIPSETTING(    0x60, "5:00" )
	PORT_DIPSETTING(    0x70, "4:30" )
	PORT_DIPSETTING(    0x80, "4:00" )
	PORT_DIPSETTING(    0x90, "3:30" )
	PORT_DIPSETTING(    0xa0, "3:00" )
	PORT_DIPSETTING(    0xb0, "2:30" )
	PORT_DIPSETTING(    0xc0, "2:00" )
	PORT_DIPSETTING(    0xd0, "1:30" )
	PORT_DIPSETTING(    0xe0, "1:00" )
	PORT_DIPSETTING(    0xf0, "0:30" )

	PORT_START	 // BIOS input ports extra
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1)
//  PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN  ) PORT_NAME("port DD bit 4") PORT_CODE(CODE_NONE)
//  PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN  ) PORT_NAME("port DD bit 5") PORT_CODE(CODE_NONE)
//  PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN  ) PORT_NAME("port DD bit 6") PORT_CODE(CODE_NONE)
//  PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN  ) PORT_NAME("port DD bit 7") PORT_CODE(CODE_NONE)
INPUT_PORTS_END

/* MEGATECH specific */

UINT8 mt_ram;

static READ8_HANDLER( megatech_instr_r )
{
	UINT8* instr = memory_region(REGION_USER1);

	if(mt_ram == 0)
		return instr[offset/2];
	else
		return 0xff;
}

static WRITE8_HANDLER( megatech_instr_w )
{
	mt_ram = data;
}


static READ8_HANDLER( bios_ctrl_r )
{
	if(offset == 0)
		return 0;
	if(offset == 2)
		return bios_ctrl[offset] & 0xfe;

	return bios_ctrl[offset];
}

static WRITE8_HANDLER( bios_ctrl_w )
{
	if(offset == 1)
	{
		bios_ctrl_inputs = data & 0x04;  // Genesis/SMS input ports disable bit
	}
	bios_ctrl[offset] = data;
}


static ADDRESS_MAP_START( megatech_bios_map, ADDRESS_SPACE_PROGRAM, 8 )
 	AM_RANGE(0x0000, 0x2fff) AM_ROM
 	AM_RANGE(0x3000, 0x5fff) AM_RAM
	AM_RANGE(0x6400, 0x6400) AM_READ(input_port_10_r)
	AM_RANGE(0x6401, 0x6401) AM_READ(input_port_11_r)
	AM_RANGE(0x6404, 0x6404) AM_WRITE(megatech_instr_w)
	AM_RANGE(0x6800, 0x6800) AM_READ(input_port_6_r)
	AM_RANGE(0x6801, 0x6801) AM_READ(input_port_7_r)
	AM_RANGE(0x6802, 0x6807) AM_READWRITE(bios_ctrl_r, bios_ctrl_w)
//  AM_RANGE(0x6805, 0x6805) AM_READ(input_port_8_r)
 	AM_RANGE(0x7000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0xffff) AM_READ(megatech_instr_r)
ADDRESS_MAP_END


/* basically from src/drivers/segasyse.c */
UINT8 segae_vdp_ctrl_r ( UINT8 chip );
UINT8 segae_vdp_data_r ( UINT8 chip );
void segae_vdp_ctrl_w ( UINT8 chip, UINT8 data );
void segae_vdp_data_w ( UINT8 chip, UINT8 data );

READ8_HANDLER (megatech_bios_port_be_bf_r)
{
	UINT8 temp = 0;

	switch (offset)
	{
		case 0: /* port 0xbe, VDP 1 DATA Read */
			temp = segae_vdp_data_r(0); break ;
		case 1: /* port 0xbf, VDP 1 CTRL Read */
			temp = segae_vdp_ctrl_r(0); break ;
	}
	return temp;
}

WRITE8_HANDLER (megatech_bios_port_be_bf_w)
{
	switch (offset)
	{
		case 0: /* port 0xbe, VDP 1 DATA Write */
			segae_vdp_data_w(0, data); break;
		case 1: /* port 0xbf, VDP 1 CTRL Write */
			segae_vdp_ctrl_w(0, data); break;
	}
}

static WRITE8_HANDLER (megatech_bios_port_ctrl_w)
{
	bios_port_ctrl = data;
}

static READ8_HANDLER (megatech_bios_port_dc_r)
{
	if(bios_port_ctrl == 0x55)
		return readinputport(12);
	else
		return readinputport(9);
}

static READ8_HANDLER (megatech_bios_port_dd_r)
{
	if(bios_port_ctrl == 0x55)
		return readinputport(12);
	else
		return readinputport(8);
}

static WRITE8_HANDLER (megatech_bios_port_7f_w)
{
//  popmessage("CPU #3: I/O port 0x7F write, data %02x",data);
}


static ADDRESS_MAP_START( megatech_bios_portmap, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x3f, 0x3f) AM_WRITE(megatech_bios_port_ctrl_w)
	AM_RANGE(0x7f, 0x7f) AM_WRITE(megatech_bios_port_7f_w)
	AM_RANGE(0xbe, 0xbf) AM_WRITE(megatech_bios_port_be_bf_w)			/* VDP */
	AM_RANGE(0xdc, 0xdc) AM_READ(megatech_bios_port_dc_r)  // player inputs
	AM_RANGE(0xdd, 0xdd) AM_READ(megatech_bios_port_dd_r)  // other player 2 inputs
	AM_RANGE(0xbe, 0xbf) AM_READ(megatech_bios_port_be_bf_r)			/* VDP */
ADDRESS_MAP_END



static UINT8 hintcount;			/* line interrupt counter, decreased each scanline */
extern UINT8 segae_vintpending;
extern UINT8 segae_hintpending;
extern UINT8 *segae_vdp_regs[];		/* pointer to vdp's registers */

// Interrupt handler - from drivers/segasyse.c
INTERRUPT_GEN (megatech_irq)
{
	int sline;
	sline = 261 - cpu_getiloops();

	if (sline ==0) {
		hintcount = segae_vdp_regs[0][10];
	}

	if (sline <= 192) {

//      if (sline != 192) segae_drawscanline(sline,1,1);

		if (sline == 192)
			segae_vintpending = 1;

		if (hintcount == 0) {
			hintcount = segae_vdp_regs[0][10];
			segae_hintpending = 1;

			if  ((segae_vdp_regs[0][0] & 0x10)) {
				cpunum_set_input_line(2, 0, HOLD_LINE);
				return;
			}

		} else {
			hintcount--;
		}
	}

	if (sline > 192) {
		hintcount = segae_vdp_regs[0][10];

		if ( (sline<0xe0) && (segae_vintpending) ) {
			cpunum_set_input_line(2, 0, HOLD_LINE);
		}
	}

}

static ADDRESS_MAP_START( genesis_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x3fffff) AM_ROM								/* Cartridge Program Rom */
	AM_RANGE(0xa10000, 0xa1001f) AM_READWRITE(genesis_io_r, genesis_io_w) AM_BASE(&genesis_io_ram) /* Genesis Input */
	AM_RANGE(0xa11000, 0xa11203) AM_WRITE(genesis_ctrl_w)
	AM_RANGE(0xa00000, 0xa0ffff) AM_READWRITE(genesis_68k_to_z80_r, megaplay_68k_to_z80_w)
	AM_RANGE(0xc00000, 0xc0001f) AM_READWRITE(genesis_vdp_r, genesis_vdp_w)				/* VDP Access */
	AM_RANGE(0xe00000, 0xe0ffff) AM_MIRROR(0x1f0000) AM_RAM AM_BASE(&generic_nvram16) AM_SIZE(&generic_nvram_size)
ADDRESS_MAP_END


static ADDRESS_MAP_START( genesis_z80_map, ADDRESS_SPACE_PROGRAM, 8 )
 	AM_RANGE(0x0000, 0x1fff) AM_MIRROR(0x2000) AM_RAM AM_BASE(&genesis_z80_ram)
	AM_RANGE(0x4000, 0x7fff) AM_READWRITE(genesis_z80_r, genesis_z80_w)
	AM_RANGE(0x8000, 0xffff) AM_READ(genesis_z80_bank_r)
ADDRESS_MAP_END


static MACHINE_DRIVER_START( genesis_base )
	/*basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M68000, MASTER_CLOCK / 7)
	MDRV_CPU_PROGRAM_MAP(genesis_map, 0)
	MDRV_CPU_VBLANK_INT(genesis_vblank_interrupt,1)

	MDRV_CPU_ADD_TAG("sound", Z80, MASTER_CLOCK / 15)
	MDRV_CPU_PROGRAM_MAP(genesis_z80_map, 0)
	MDRV_CPU_VBLANK_INT(irq0_line_hold, 1) /* from vdp at scanline 0xe0 */

	MDRV_SCREEN_REFRESH_RATE(60)

	MDRV_INTERLEAVE(100)

	MDRV_MACHINE_START(genesis)
	MDRV_MACHINE_RESET(genesis)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_HAS_SHADOWS | VIDEO_HAS_HIGHLIGHTS)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(342,262)
	MDRV_SCREEN_VISIBLE_AREA(0, 319, 0, 223)
	MDRV_PALETTE_LENGTH(64)

	MDRV_VIDEO_START(genesis)
	MDRV_VIDEO_UPDATE(genesis)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YM3438, MASTER_CLOCK/7)
	MDRV_SOUND_ROUTE(0, "mono", 0.50)
	MDRV_SOUND_ROUTE(1, "mono", 0.50)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( megatech )

	/* basic machine hardware */
	MDRV_IMPORT_FROM( genesis_base )

	MDRV_CPU_ADD_TAG("megatech_bios", Z80, MASTER_CLOCK / 15) /* ?? */
	MDRV_CPU_PROGRAM_MAP(megatech_bios_map, 0)
	MDRV_CPU_IO_MAP(megatech_bios_portmap,0)
	MDRV_CPU_VBLANK_INT(megatech_irq, 262)

	MDRV_MACHINE_START(genesis)
	MDRV_MACHINE_RESET(genesis)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_HAS_SHADOWS | VIDEO_HAS_HIGHLIGHTS)
	MDRV_PALETTE_LENGTH(64+32) /* +32 for megatech bios vdp part */

	MDRV_VIDEO_START(megatech)
	MDRV_VIDEO_UPDATE(megatech)
	MDRV_DEFAULT_LAYOUT(layout_dualhovu)

	MDRV_SCREEN_ADD("top", 0x000)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_SIZE(342,262)
	MDRV_SCREEN_VISIBLE_AREA(0, 256-1, 0, 192-1)

	MDRV_SCREEN_ADD("bottom", 0x000)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_REFRESH_RATE(30)
	MDRV_SCREEN_SIZE(342,262)
	MDRV_SCREEN_VISIBLE_AREA(0, 320-1, 0, 224-1)



	/* sound hardware */
	MDRV_SOUND_ADD(SN76496, MASTER_CLOCK/15)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	MDRV_SOUND_ADD(SN76496, MASTER_CLOCK/15)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_DRIVER_END


/* MegaTech Games - Genesis & sms! Games with a timer */

/* 12368-xx  xx is the game number? if so there are a _lot_ of carts, mt_beast is 01, mt_sonic is 52! */

ROM_START( megatech )
	ROM_REGION( 0x400000, REGION_CPU1, ROMREGION_ERASEFF )

	ROM_REGION( 0x8000, REGION_USER1, ROMREGION_ERASEFF ) /* Game Instructions */

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* Bios */
	ROM_LOAD( "epr12664.20", 0x000000, 0x8000, CRC(f71e9526) SHA1(1c7887541d02c41426992d17f8e3db9e03975953) )
ROM_END

ROM_START( mt_beast ) /* Altered Beast */
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "mp12538.ic1", 0x000000, 0x080000, CRC(3bea3dce) SHA1(ec72e4fde191dedeb3f148f132603ed3c23f0f86) )

	ROM_REGION( 0x8000, REGION_USER1, 0 ) /* Game Instructions */
	ROM_LOAD( "12368-01.ic2", 0x000000, 0x08000, CRC(40cb0088) SHA1(e1711532c29f395a35a1cb34d789015881b5a1ed) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* Bios */
	ROM_LOAD( "epr12664.20", 0x000000, 0x8000, CRC(f71e9526) SHA1(1c7887541d02c41426992d17f8e3db9e03975953) )
ROM_END

ROM_START( mt_astro ) /* Astro Warrior (Sms Game!) */
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	/* z80 code because this is sms based .... */
	ROM_LOAD( "ep13817.ic2", 0x000000, 0x020000, CRC(299cbb74) SHA1(901697a3535ad70190647f34ad5b30b695d54542) )

	ROM_REGION( 0x8000, REGION_USER1, 0 ) /* Game Instructions */
	ROM_LOAD( "12368-13.ic1", 0x000000, 0x08000,  CRC(4038cbd1) SHA1(696bc1efce45d9f0052b2cf0332a232687c8d6ab) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* Bios */
	ROM_LOAD( "epr12664.20", 0x000000, 0x8000, CRC(f71e9526) SHA1(1c7887541d02c41426992d17f8e3db9e03975953) )
ROM_END


ROM_START( mt_wcsoc ) /* World Cup Soccer */
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "mp12607b.ic1", 0x000000, 0x080000, CRC(bc591b30) SHA1(55e8577171c0933eee53af1dabd0f4c6462d5fc8) )

	ROM_REGION( 0x8000, REGION_USER1, 0 ) /* Game Instructions */
	ROM_LOAD( "12368-21.ic2", 0x000000, 0x08000, CRC(028ee46b) SHA1(cd8f81d66e5ae62107eb20e0ca5db4b66d4b2987) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* Bios */
	ROM_LOAD( "epr12664.20", 0x000000, 0x8000, CRC(f71e9526) SHA1(1c7887541d02c41426992d17f8e3db9e03975953) )
ROM_END

ROM_START( mt_gng ) /* Ghouls and Ghosts */
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "mp12605.ic1", 0x000000, 0x020000, CRC(1066C6AB) SHA1(C30E4442732BDB38C96D780542F8550A94D127B0) )
	ROM_LOAD16_WORD_SWAP( "mpr12606.ic2", 0x080000, 0x020000, CRC(D0BE7777) SHA1(A44B2A3D427F6973B5C1A3DCD8D1776366ACB9F7) )
	ROM_CONTINUE(0x020000,0x60000)

	ROM_REGION( 0x8000, REGION_USER1, 0 ) /* Game Instructions */
	ROM_LOAD( "12368-23.ic3", 0x000000, 0x08000, CRC(7ee58546) SHA1(ad5bb0934475eacdc5e354f67c96fe0d2512d33b) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* Bios */
	ROM_LOAD( "epr12664.20", 0x000000, 0x8000, CRC(f71e9526) SHA1(1c7887541d02c41426992d17f8e3db9e03975953) )
ROM_END

ROM_START( mt_shang ) /* Super HangOn */
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "mpr-12640.ic1", 0x000000, 0x080000, CRC(2fe2cf62) SHA1(4728bcc847deb38b16338cbd0154837cd4a07b7d) )

	ROM_REGION( 0x8000, REGION_USER1, 0 ) /* Game Instructions */
	ROM_LOAD( "epr-12368-24.ic2", 0x000000, 0x08000, CRC(6c2db7e3) SHA1(8de0a10ed9185c9e98f17784811a79d3ce8c4c03) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* Bios */
	ROM_LOAD( "epr12664.20", 0x000000, 0x8000, CRC(f71e9526) SHA1(1c7887541d02c41426992d17f8e3db9e03975953) )
ROM_END

ROM_START( mt_gaxe ) /* Golden Axe */
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "12806.ic1", 0x000000, 0x080000, CRC(43456820) SHA1(2f7f1fcd979969ac99426f11ab99999a5494a121) )

	ROM_REGION( 0x8000, REGION_USER1, 0 ) /* Game Instructions */
	ROM_LOAD( "12368-25.ic2", 0x000000, 0x08000, CRC(1f07ed28) SHA1(9d54192f4c6c1f8a51c38a835c1dd1e4e3e8279e) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* Bios */
	ROM_LOAD( "epr12664.20", 0x000000, 0x8000, CRC(f71e9526) SHA1(1c7887541d02c41426992d17f8e3db9e03975953) )
ROM_END

ROM_START( mt_smgp ) /* Super Monaco Grand Prix */
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "13250.ic1", 0x000000, 0x080000, CRC(189b885f) SHA1(31c06ffcb48b1604989a94e584261457de4f1f46) )

	ROM_REGION( 0x8000, REGION_USER1, 0 ) /* Game Instructions */
	ROM_LOAD( "12368-39.ic2", 0x000000, 0x08000, CRC(64b3ce25) SHA1(83a9f2432d146a712b037f96f261742f7dc810bb) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* Bios */
	ROM_LOAD( "epr12664.20", 0x000000, 0x8000, CRC(f71e9526) SHA1(1c7887541d02c41426992d17f8e3db9e03975953) )
ROM_END

ROM_START( mt_sonic ) /* Sonic */
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "mp13913.ic1", 0x000000, 0x080000, CRC(480b4b5c) SHA1(ab1dc1f738e3b2d0898a314b123fa71182bf572e) )

	ROM_REGION( 0x8000, REGION_USER1, 0 ) /* Game Instructions */
	ROM_LOAD( "12368-52.ic2", 0x0000, 0x8000,  CRC(6a69d20c) SHA1(e483b39ff6eca37dc192dc296d004049e220554a) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* Bios */
	ROM_LOAD( "epr12664.20", 0x000000, 0x8000, CRC(f71e9526) SHA1(1c7887541d02c41426992d17f8e3db9e03975953) )
ROM_END

ROM_START( mt_sonia ) /* Sonic  (alt)*/
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "mp13933.ic1", 0x000000, 0x080000, CRC(13775004) SHA1(5decfd35944a2d0e7b996b9a4a12b616a309fd5e) )

	ROM_REGION( 0x8000, REGION_USER1, 0 ) /* Game Instructions */
	ROM_LOAD( "12368-52.ic2", 0x0000, 0x8000,  CRC(6a69d20c) SHA1(e483b39ff6eca37dc192dc296d004049e220554a) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* Bios */
	ROM_LOAD( "epr12664.20", 0x000000, 0x8000, CRC(f71e9526) SHA1(1c7887541d02c41426992d17f8e3db9e03975953) )
ROM_END

ROM_START( mt_gaxe2 ) /* Golden Axe 2 */
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "mp14272.ic1", 0x000000, 0x080000, CRC(d4784cae) SHA1(b6c286027d06fd850016a2a1ee1f1aeea080c3bb) )

	ROM_REGION( 0x8000, REGION_USER1, 0 ) /* Game Instructions */
	ROM_LOAD( "12368-57.ic2", 0x000000, 0x08000, CRC(dc9b4433) SHA1(efd3a598569010cdc4bf38ecbf9ed1b4e14ffe36) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* Bios */
	ROM_LOAD( "epr12664.20", 0x000000, 0x8000, CRC(f71e9526) SHA1(1c7887541d02c41426992d17f8e3db9e03975953) )
ROM_END

ROM_START( mt_stf ) /* Sports Talk Football */
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "mp14356a-f.ic1", 0x000000, 0x100000, CRC(20cf32f6) SHA1(752314346a7a98b3808b3814609e024dc0a4108c) )

	ROM_REGION( 0x8000, REGION_USER1, 0 ) /* Game Instructions */
	ROM_LOAD( "ep12368-58.ic2", 0x000000, 0x08000, CRC(dce2708e) SHA1(fcebb1899ee11468f6bda705899f074e7de9d723) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* Bios */
	ROM_LOAD( "epr12664.20", 0x000000, 0x8000, CRC(f71e9526) SHA1(1c7887541d02c41426992d17f8e3db9e03975953) )
ROM_END

ROM_START( mt_fshrk ) /* Fire Shark */
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "mp14341.ic1", 0x000000, 0x080000, CRC(04d65ebc) SHA1(24338aecdc52b6f416548be722ca475c83dbae96) )
	/* alt version with these roms exists, but the content is the same */
	/* (6a221fd6) ep14706.ic1             mp14341.ic1  [even]     IDENTICAL */
	/* (09fa48af) ep14707.ic2             mp14341.ic1  [odd]      IDENTICAL */

	ROM_REGION( 0x8000, REGION_USER1, 0 ) /* Game Instructions */
	ROM_LOAD( "12368-53.ic2", 0x000000, 0x08000,  CRC(4fa61044) SHA1(7810deea221c10b0b2f5233443d81f4f1998ee58) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* Bios */
	ROM_LOAD( "epr12664.20", 0x000000, 0x8000, CRC(f71e9526) SHA1(1c7887541d02c41426992d17f8e3db9e03975953) )
ROM_END


ROM_START( mt_eswat ) /* E-Swat */
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "mp13192-h.ic1", 0x000000, 0x080000, CRC(82f458ef) SHA1(58444b783312def71ecffc4ad021b72a609685cb) )

	ROM_REGION( 0x8000, REGION_USER1, 0 ) /* Game Instructions */
	ROM_LOAD( "12368-38.ic2", 0x000000, 0x08000, CRC(43c5529b) SHA1(104f85adea6da1612c0aa96d553efcaa387d7aaf) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* Bios */
	ROM_LOAD( "epr12664.20", 0x000000, 0x8000, CRC(f71e9526) SHA1(1c7887541d02c41426992d17f8e3db9e03975953) )
ROM_END

ROM_START( mt_bbros ) /* Bonanza Bros */
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "mp13905a.ic1", 0x000000, 0x100000, CRC(68a88d60) SHA1(2f56e8a2b0999de4fa0d14a1527f4e1df0f9c7a2) )

	ROM_REGION( 0x8000, REGION_USER1, 0 ) /* Game Instructions */
	ROM_LOAD( "12368-49.ic2", 0x000000, 0x08000, CRC(c5101da2) SHA1(636f30043e2e9291e193ef9a2ead2e97a0bf7380) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* Bios */
	ROM_LOAD( "epr12664.20", 0x000000, 0x8000, CRC(f71e9526) SHA1(1c7887541d02c41426992d17f8e3db9e03975953) )
ROM_END

ROM_START( mt_soni2 ) /* Sonic 2 */
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "mp15000a-f.ic1", 0x000000, 0x100000, CRC(679ebb49) SHA1(557482064677702454562f753460993067ef9e16) )

	ROM_REGION( 0x8000, REGION_USER1, 0 ) /* Game Instructions */
	ROM_LOAD( "ep12368-62.ic2", 0x000000, 0x08000, CRC(14a8566f) SHA1(d1d14162144bf068ddd19e9736477ff98fb43f9e) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* Bios */
	ROM_LOAD( "epr12664.20", 0x000000, 0x8000, CRC(f71e9526) SHA1(1c7887541d02c41426992d17f8e3db9e03975953) )
ROM_END

ROM_START( mt_mlh ) /* Mario Lemieux Hockey */
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "mpr-14376-h.ic1", 0x000000, 0x80000, CRC(aa9be87e) SHA1(dceed94eaeb30e534f6953a4bc25ff37673b1e6b) )

	ROM_REGION( 0x8000, REGION_USER1, 0 ) /* Game Instructions */
	ROM_LOAD( "epr-12368-59.ic2", 0x000000, 0x08000, CRC(6d47b438) SHA1(0a145f6438e4e55c957ae559663c37662b685246) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* Bios */
	ROM_LOAD( "epr12664.20", 0x000000, 0x8000, CRC(f71e9526) SHA1(1c7887541d02c41426992d17f8e3db9e03975953) )
ROM_END

ROM_START( mt_kcham ) /* Kid Chameleon */
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "mp14557.ic1", 0x000000, 0x100000, CRC(e1a889a4) SHA1(a2768eacafc47d371e5276f0cce4b12b6041337a) )

	ROM_REGION( 0x8000, REGION_USER1, 0 ) /* Game Instructions */
	ROM_LOAD( "12368-60.ic2", 0x000000, 0x08000, CRC(a8e4af18) SHA1(dfa49f6ec4047718f33dba1180f6204dbaff884c) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* Bios */
	ROM_LOAD( "epr12664.20", 0x000000, 0x8000, CRC(f71e9526) SHA1(1c7887541d02c41426992d17f8e3db9e03975953) )
ROM_END

ROM_START( mt_lastb ) /* Last Battle */
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "mp12578f.ic1", 0x000000, 0x080000, CRC(531191a0) SHA1(f6bc26e975c01a3e10ab4033e4c5f494627a1e2f) )

	ROM_REGION( 0x8000, REGION_USER1, 0 ) /* Game Instructions */
	ROM_LOAD( "12368-20.ic2", 0x000000, 0x08000, CRC(e1a71c91) SHA1(c250da18660d8aea86eb2abace41ba46130dabc8) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* Bios */
	ROM_LOAD( "epr12664.20", 0x000000, 0x8000, CRC(f71e9526) SHA1(1c7887541d02c41426992d17f8e3db9e03975953) )
ROM_END

ROM_START( mt_mwalk ) /* Moon Walker */
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "mp13285a.ic1", 0x000000, 0x080000, CRC(189516e4) SHA1(2a79e07da2e831832b8d448cae87a833c85e67c9) )

	ROM_REGION( 0x8000, REGION_USER1, 0 ) /* Game Instructions */
	ROM_LOAD( "12368-40.ic2", 0x000000, 0x08000, CRC(0482378c) SHA1(734772f3ddb5ff82b76c3514d18a464b2bce8381) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* Bios */
	ROM_LOAD( "epr12664.20", 0x000000, 0x8000, CRC(f71e9526) SHA1(1c7887541d02c41426992d17f8e3db9e03975953) )
ROM_END

ROM_START( mt_crack ) /* Crackdown */
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "mp13578a-s.ic1", 0x000000, 0x080000, CRC(23f19893) SHA1(09aca793871e2246af4dc24925bc1eda8ff34446) )

	ROM_REGION( 0x8000, REGION_USER1, 0 ) /* Game Instructions */
	ROM_LOAD( "ep12368-41.ic2", 0x000000, 0x08000, CRC(3014acec) SHA1(07953e9ae5c23fc7e7d08993b215f4dfa88aa5d7) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* Bios */
	ROM_LOAD( "epr12664.20", 0x000000, 0x8000, CRC(f71e9526) SHA1(1c7887541d02c41426992d17f8e3db9e03975953) )
ROM_END

ROM_START( mt_mystd ) /* Mystic Defender */
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "mp12707.1", 0x000000, 0x080000, CRC(4f2c513d) SHA1(f9bb548b3688170fe18bb3f1b5b54182354143cf) )

	ROM_REGION( 0x8000, REGION_USER1, 0 ) /* Game Instructions */
	ROM_LOAD( "12368-27.ic2", 0x000000, 0x08000, CRC(caf46f78) SHA1(a9659e86a6a223646338cd8f29c346866e4406c7) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* Bios */
	ROM_LOAD( "epr12664.20", 0x000000, 0x8000, CRC(f71e9526) SHA1(1c7887541d02c41426992d17f8e3db9e03975953) )
ROM_END

ROM_START( mt_shar2 ) /* Space Harrier 2 */
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "mp11934.ic1", 0x000000, 0x080000, CRC(932daa09) SHA1(a2d7a76f3604c6227d43229908bfbd02b0ef5fd9) )

	ROM_REGION( 0x8000, REGION_USER1, 0 ) /* Game Instructions */
	ROM_LOAD( "12368-02.ic1", 0x000000, 0x08000, CRC(c129c66c) SHA1(e7c0c97db9df9eb04e2f9ff561b64305219b8f1f) ) // ic2?

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* Bios */
	ROM_LOAD( "epr12664.20", 0x000000, 0x8000, CRC(f71e9526) SHA1(1c7887541d02c41426992d17f8e3db9e03975953) )
ROM_END

ROM_START( mt_stbld ) /* Super Thunder Blade */
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "mp11996f.ic1", 0x000000, 0x080000,  CRC(9355c34e) SHA1(26ff91c2921408673c644b0b1c8931d98524bf63) )

	ROM_REGION( 0x8000, REGION_USER1, 0 ) /* Game Instructions */
	ROM_LOAD( "12368-03.ic2", 0x000000, 0x08000,  CRC(1ba4ac5d) SHA1(9bde57d70189d159ebdc537a9026001abfd0deae) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* Bios */
	ROM_LOAD( "epr12664.20", 0x000000, 0x8000, CRC(f71e9526) SHA1(1c7887541d02c41426992d17f8e3db9e03975953) )
ROM_END

ROM_START( mt_tetri ) /* Tetris */
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "mpr-12356f.ic1", 0x000001, 0x020000, CRC(1e71c1a0) SHA1(44b2312792e49d46d71e0417a7f022e5ffddbbfe) )
	ROM_LOAD16_BYTE( "mpr-12357f.ic2", 0x000000, 0x020000, CRC(d52ca49c) SHA1(a9159892eee2c0cf28ebfcfa99f81f80781851c6) )

	ROM_REGION( 0x8000, REGION_USER1, 0 ) /* Game Instructions */
	ROM_LOAD( "12368-22.ic3", 0x000000, 0x08000, CRC(1c1b6468) SHA1(568a38f4186167486e39ab4aa2c1ceffd0b81156) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* Bios */
	ROM_LOAD( "epr12664.20", 0x000000, 0x8000, CRC(f71e9526) SHA1(1c7887541d02c41426992d17f8e3db9e03975953) )
ROM_END

ROM_START( mt_tfor2 ) /* Thunder Force 2 */
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "mp12559.ic1", 0x000000, 0x080000, CRC(b093bee3) SHA1(0bf6194c3d228425f8cf1903ed70d8da1b027b6a) )

	ROM_REGION( 0x8000, REGION_USER1, 0 ) /* Game Instructions */
	ROM_LOAD( "12368-11.ic2", 0x000000, 0x08000, CRC(f4f27e8d) SHA1(ae1a2823deb416c53838115966f1833d5dac72d4) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* Bios */
	ROM_LOAD( "epr12664.20", 0x000000, 0x8000, CRC(f71e9526) SHA1(1c7887541d02c41426992d17f8e3db9e03975953) )
ROM_END

ROM_START( mt_tlbba ) /* Tommy Lasorda Baseball */
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "mp12706.ic1", 0x000000, 0x080000, CRC(8901214f) SHA1(f5ec166be1cf9b86623b9d7a78ec903b899da32a) )

	ROM_REGION( 0x8000, REGION_USER1, 0 ) /* Game Instructions */
	ROM_LOAD( "12368-35.ic2", 0x000000, 0x08000, CRC(67bbe482) SHA1(6fc283b22e68befabb44b2cc61a7f82a71d6f029) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* Bios */
	ROM_LOAD( "epr12664.20", 0x000000, 0x8000, CRC(f71e9526) SHA1(1c7887541d02c41426992d17f8e3db9e03975953) )
ROM_END

ROM_START( mt_cols ) /* Columns */
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "mp13193-t.ic1", 0x000000, 0x080000, CRC(8c770e2f) SHA1(02a3626025c511250a3f8fb3176eebccc646cda9) )

	ROM_REGION( 0x8000, REGION_USER1, 0 ) /* Game Instructions */
	ROM_LOAD( "ep12368-36.ic3", 0x000000, 0x08000,  CRC(a4b29bac) SHA1(c9be866ac96243897d09612fe17562e0481f66e3) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* Bios */
	ROM_LOAD( "epr12664.20", 0x000000, 0x8000, CRC(f71e9526) SHA1(1c7887541d02c41426992d17f8e3db9e03975953) )
ROM_END

ROM_START( mt_ggolf ) /* Great Golf (Bad Dump) */
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "mp11129f.ic1", 0x000000, 0x020000, BAD_DUMP CRC(942738ba) SHA1(e99d4e39c965fc123a39d75521a274687e917a57) ) // first 32kb is repeated 4 times, doesn't work in MEKA

	ROM_REGION( 0x8000, REGION_USER1, 0 ) /* Game Instructions */
	ROM_LOAD( "12368-04.ic2", 0x000000, 0x08000, CRC(62e5579b) SHA1(e1f531be5c40a1216d4192baeda9352384444410) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* Bios */
	ROM_LOAD( "epr12664.20", 0x000000, 0x8000, CRC(f71e9526) SHA1(1c7887541d02c41426992d17f8e3db9e03975953) )
ROM_END

ROM_START( mt_gsocr ) /* Great Soccer (SMS based) (Bad Dump) */
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "mp10747f.ic1", 0x000000, 0x020000, BAD_DUMP CRC(9cf53703) SHA1(c6b4d1de56bd5bf067ec7fc80449c07686d01337) ) // first 32kb is repeated 4 times, doesn't work in MEKA

	ROM_REGION( 0x8000, REGION_USER1, 0 ) /* Game Instructions */
	ROM_LOAD( "12368-05.ic2", 0x000000, 0x08000, CRC(bab91fcc) SHA1(a160c9d34b253e93ac54fdcef33f95f44d8fa90c) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* Bios */
	ROM_LOAD( "epr12664.20", 0x000000, 0x8000, CRC(f71e9526) SHA1(1c7887541d02c41426992d17f8e3db9e03975953) )
ROM_END

ROM_START( mt_asyn ) /* Alien Syndrome (SMS based) */
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "mpr-11194.ic1", 0x000000, 0x040000, CRC(4cc11df9) SHA1(5d786476b275de34efb95f576dd556cf4b335a83) )

	ROM_REGION( 0x8000, REGION_USER1, 0 ) /* Game Instructions */
	ROM_LOAD( "epr-12368-07.ic2", 0x000000, 0x08000, CRC(14f4a17b) SHA1(0fc010ac95762534892f1ae16986dbf1c25399d3) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* Bios */
	ROM_LOAD( "epr12664.20", 0x000000, 0x8000, CRC(f71e9526) SHA1(1c7887541d02c41426992d17f8e3db9e03975953) )
ROM_END

ROM_START( mt_parlg ) /* Parlour Games (SMS Based)  */
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "mp11404.ic1", 0x000000, 0x020000, CRC(E030E66C) SHA1(06664DAF208F07CB00B603B12ECCFC3F01213A17) )

	ROM_REGION( 0x8000, REGION_USER1, 0 ) /* Game Instructions */
	ROM_LOAD( "12368-29.ic2", 0x000000, 0x08000, CRC(534151e8) SHA1(219238d90c1d3ac07ff64c9a2098b490fff68f04) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* Bios */
	ROM_LOAD( "epr12664.20", 0x000000, 0x8000, CRC(f71e9526) SHA1(1c7887541d02c41426992d17f8e3db9e03975953) )
ROM_END

ROM_START( mt_shnbi ) /* Shinobi. */
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "mp11706.ic1", 0x000000, 0x040000, CRC(0C6FAC4E) SHA1(7C0778C055DC9C2B0AAE1D166DBDB4734E55B9D1) )

	ROM_REGION( 0x8000, REGION_USER1, 0 ) /* Game Instructions */
	ROM_LOAD( "12368-08.ic2", 0x000000, 0x08000, CRC(103A0459) SHA1(D803DDF7926B83785E8503C985B8C78E7CCB5DAC) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* Bios */
	ROM_LOAD( "epr12664.20", 0x000000, 0x8000, CRC(f71e9526) SHA1(1c7887541d02c41426992d17f8e3db9e03975953) )
ROM_END

ROM_START( mt_revsh ) /* The Revenge Of Shinobi. */
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "mp12675.ic1", 0x000000, 0x080000, CRC(672A1D4D) SHA1(5FD0AF14C8F2CF8CEAB1AE61A5A19276D861289A) )

	ROM_REGION( 0x8000, REGION_USER1, 0 ) /* Game Instructions */
	ROM_LOAD( "12368-28.ic2", 0x000000, 0x08000, CRC(0D30BEDE) SHA1(73A090D84B78A570E02FB54A33666DCADA52849B) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* Bios */
	ROM_LOAD( "epr12664.20", 0x000000, 0x8000, CRC(f71e9526) SHA1(1c7887541d02c41426992d17f8e3db9e03975953) )
ROM_END

ROM_START( mt_aftrb ) /* Afterburner. */
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "mp11271.ic1", 0x000000, 0x080000, CRC(1C951F8E) SHA1(51531DF038783C84640A0CAB93122E0B59E3B69A) )

	ROM_REGION( 0x8000, REGION_USER1, 0 ) /* Game Instructions */
	ROM_LOAD( "12368-10.ic2", 0x000000, 0x08000, CRC(2A7CB590) SHA1(2236963BDDC89CA9045B530259CC7B5CCF889EAF) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* Bios */
	ROM_LOAD( "epr12664.20", 0x000000, 0x8000, CRC(f71e9526) SHA1(1c7887541d02c41426992d17f8e3db9e03975953) )
ROM_END

ROM_START( mt_tgolf ) /* Arnold Palmer Tournament Golf */
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "mp12645f.ic1", 0x000000, 0x080000, CRC(c07ef8d2) SHA1(9d111fdc7bb92d52bfa048cd134aa488b4f475ef) )

	ROM_REGION( 0x8000, REGION_USER1, 0 ) /* Game Instructions */
	ROM_LOAD( "12368-31.ic2", 0x000000, 0x08000, CRC(30af7e4a) SHA1(baf91d527393dc90aba9371abcb1e690bcc83c7e) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* Bios */
	ROM_LOAD( "epr12664.20", 0x000000, 0x8000, CRC(f71e9526) SHA1(1c7887541d02c41426992d17f8e3db9e03975953) )
ROM_END

ROM_START( mt_astrm ) /* Alien Storm. */
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "mp13941.ic1", 0x000000, 0x080000, CRC(D71B3EE6) SHA1(05F272DAD243D132D517C303388248DC4C0482ED) )

	ROM_REGION( 0x8000, REGION_USER1, 0 ) /* Game Instructions */
	ROM_LOAD( "1236847.ic2", 0x000000, 0x08000, CRC(31FB683D) SHA1(E356DA020BBF817B97FB10C27F75CF5931EDF4FC) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* Bios */
	ROM_LOAD( "epr12664.20", 0x000000, 0x8000, CRC(f71e9526) SHA1(1c7887541d02c41426992d17f8e3db9e03975953) )
ROM_END

ROM_START( mt_arrow ) /* Arrow Flash */
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "mpr13396h.ic1", 0x000000, 0x080000, CRC(091226e3) SHA1(cb15c6277314f3c4a86b5ae5823f72811d5d269d) )

	ROM_REGION( 0x8000, REGION_USER1, 0 ) /* Game Instructions */
	ROM_LOAD( "epr12368-44.ic2", 0x000000, 0x08000, CRC(e653065d) SHA1(96b014fc4df8eb2188ac94ed0a778d974fe6dcad) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* Bios */
	ROM_LOAD( "epr12664.20", 0x000000, 0x8000, CRC(f71e9526) SHA1(1c7887541d02c41426992d17f8e3db9e03975953) )
ROM_END

ROM_START( mt_orun ) /* Outrun */
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "mpr-11078.ic1", 0x000000, 0x040000, CRC(5589d8d2) SHA1(4f9b61b24f0d9fee0448cdbbe8fc05411dbb1102) )

	ROM_REGION( 0x8000, REGION_USER1, 0 ) /* Game Instructions */
	ROM_LOAD( "epr-12368-06.ic2", 0x000000, 0x08000, CRC(c7c74429) SHA1(22ee261a653e10d66e0d6703c988bb7f236a7571) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* Bios */
	ROM_LOAD( "epr12664.20", 0x000000, 0x8000, CRC(f71e9526) SHA1(1c7887541d02c41426992d17f8e3db9e03975953) )
ROM_END

ROM_START( mt_srbb ) /* Super Real Basketball */
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "mpr-12904f.ic1", 0x000000, 0x080000, CRC(4346e11a) SHA1(c86725780027ef9783cb7884c8770cc030b0cd0d) )

	ROM_REGION( 0x8000, REGION_USER1, 0 ) /* Game Instructions */
	ROM_LOAD( "epr-12368-32.ic2", 0x000000, 0x08000, CRC(f70adcbe) SHA1(d4412a7cd59fe282a1c6619aa1051a2a2e00e1aa) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* Bios */
	ROM_LOAD( "epr12664.20", 0x000000, 0x8000, CRC(f71e9526) SHA1(1c7887541d02c41426992d17f8e3db9e03975953) )
ROM_END

ROM_START( mt_fwrld ) /* Forgotten Worlds */
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "mpr-12672-h.ic1", 0x000000, 0x080000, CRC(d0ee6434) SHA1(8b9a37c206c332ef23dc71f09ec40e1a92b1f83a) )

	ROM_REGION( 0x8000, REGION_USER1, 0 ) /* Game Instructions */
	ROM_LOAD( "epr-12368-26.ic2", 0x000000, 0x08000, CRC(4623b573) SHA1(29df4a5c5de66cd9cb7519e4f30000f7dddc2138) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* Bios */
	ROM_LOAD( "epr12664.20", 0x000000, 0x8000, CRC(f71e9526) SHA1(1c7887541d02c41426992d17f8e3db9e03975953) )
ROM_END


/* nn */ /* nn is part of the instruction rom name, should there be a game for each number? */
/* -- */ GAME( 1989, megatech, 0,        megatech, megatech, 0, ROT0, "Sega",                  "Mega-Tech BIOS", GAME_IS_BIOS_ROOT )
/* 01 */ GAME( 1988, mt_beast, megatech, megatech, megatech, 0, ROT0, "Sega",                  "Altered Beast (Mega-Tech)", GAME_NOT_WORKING )
/* 02 */ GAME( 1988, mt_shar2, megatech, megatech, megatech, 0, ROT0, "Sega",                  "Space Harrier II (Mega-Tech)", GAME_NOT_WORKING )
/* 03 */ GAME( 1988, mt_stbld, megatech, megatech, megatech, 0, ROT0, "Sega",                  "Super Thunder Blade (Mega-Tech)", GAME_NOT_WORKING )
/* 04 */ GAME( 19??, mt_ggolf, megatech, megatech, megatech, 0, ROT0, "Sega",                  "Great Golf (Mega-Tech)", GAME_NOT_WORKING ) /* sms! also bad */
/* 05 */ GAME( 19??, mt_gsocr, megatech, megatech, megatech, 0, ROT0, "Sega",                  "Great Soccer (Mega-Tech)", GAME_NOT_WORKING ) /* sms! also bad */
/* 06 */ GAME( 1989, mt_orun,  megatech, megatech, megatech, 0, ROT0, "Sega",                  "Out Run (Mega-Tech)", GAME_NOT_WORKING ) /* sms! */
/* 07 */ GAME( 19??, mt_asyn,  megatech, megatech, megatech, 0, ROT0, "Sega",                  "Alien Syndrome (Mega-Tech)", GAME_NOT_WORKING ) /* sms! */
/* 08 */ GAME( 19??, mt_shnbi, megatech, megatech, megatech, 0, ROT0, "Sega",                  "Shinobi (Mega-Tech)", GAME_NOT_WORKING) /* sms */
/* 09 */ // unknown
/* 10 */ GAME( 19??, mt_aftrb, megatech, megatech, megatech, 0, ROT0, "Sega",                  "After Burner (Mega-Tech)", GAME_NOT_WORKING) /* sms */
/* 11 */ GAME( 1989, mt_tfor2, megatech, megatech, megatech, 0, ROT0, "Tecno Soft / Sega",     "Thunder Force II MD (Mega-Tech)", GAME_NOT_WORKING )
/* 12 */ // unknown
/* 13 */ GAME( 19??, mt_astro, megatech, megatech, megatech, 0, ROT0, "Sega",                  "Astro Warrior (Mega-Tech)", GAME_NOT_WORKING ) /* sms! */
/* 14 */ // unknown
/* 15 */ // unknown
/* 16 */ // unknown
/* 17 */ // unknown
/* 18 */ // unknown
/* 19 */ // unknown
/* 20 */ GAME( 1989, mt_lastb, megatech, megatech, megatech, 0, ROT0, "Sega",                  "Last Battle (Mega-Tech)", GAME_NOT_WORKING )
/* 21 */ GAME( 1989, mt_wcsoc, megatech, megatech, megatech, 0, ROT0, "Sega",                  "World Championship Soccer (Mega-Tech)", GAME_NOT_WORKING )
/* 22 */ GAME( 19??, mt_tetri, megatech, megatech, megatech, 0, ROT0, "Sega",                  "Tetris (Mega-Tech)", GAME_NOT_WORKING )
/* 23 */ GAME( 1989, mt_gng,   megatech, megatech, megatech, 0, ROT0, "Capcom / Sega",         "Ghouls'n Ghosts (Mega-Tech)", GAME_NOT_WORKING )
/* 24 */ GAME( 1989, mt_shang, megatech, megatech, megatech, 0, ROT0, "Sega",                  "Super Hang-On (Mega-Tech)", GAME_NOT_WORKING )
/* 25 */ GAME( 1989, mt_gaxe,  megatech, megatech, megatech, 0, ROT0, "Sega",                  "Golden Axe (Mega-Tech)", GAME_NOT_WORKING )
/* 26 */ GAME( 1989, mt_fwrld, megatech, megatech, megatech, 0, ROT0, "Sega",                  "Forgotten Worlds (Mega-Tech)", GAME_NOT_WORKING )
/* 27 */ GAME( 1989, mt_mystd, megatech, megatech, megatech, 0, ROT0, "Sega",                  "Mystic Defender (Mega-Tech)", GAME_NOT_WORKING )
/* 28 */ GAME( 1989, mt_revsh, megatech, megatech, megatech, 0, ROT0, "Sega",                  "The Revenge of Shinobi (Mega-Tech)", GAME_NOT_WORKING )
/* 29 */ GAME( 19??, mt_parlg, megatech, megatech, megatech, 0, ROT0, "Sega",                  "Parlour Games (Mega-Tech)", GAME_NOT_WORKING ) /* sms! */
/* 30 */ // unknown
/* 31 */ GAME( 1989, mt_tgolf, megatech, megatech, megatech, 0, ROT0, "Sega",                  "Arnold Palmer Tournament Golf (Mega-Tech)", GAME_NOT_WORKING )
/* 32 */ GAME( 1989, mt_srbb,  megatech, megatech, megatech, 0, ROT0, "Sega",                  "Super Real Basketball (Mega-Tech)", GAME_NOT_WORKING )
/* 33 */ // unknown
/* 34 */ // unknown
/* 35 */ GAME( 1989, mt_tlbba, megatech, megatech, megatech, 0, ROT0, "Sega",                  "Tommy Lasorda Baseball (Mega-Tech)", GAME_NOT_WORKING )
/* 36 */ GAME( 1990, mt_cols,  megatech, megatech, megatech, 0, ROT0, "Sega",                  "Columns (Mega-Tech)", GAME_NOT_WORKING )
/* 37 */ // unknown
/* 38 */ GAME( 1990, mt_eswat, megatech, megatech, megatech, 0, ROT0, "Sega",                  "Cyber Police ESWAT: Enhanced Special Weapons and Tactics (Mega-Tech)", GAME_NOT_WORKING )
/* 39 */ GAME( 1990, mt_smgp,  megatech, megatech, megatech, 0, ROT0, "Sega",                  "Super Monaco GP (Mega-Tech)", GAME_NOT_WORKING )
/* 40 */ GAME( 1990, mt_mwalk, megatech, megatech, megatech, 0, ROT0, "Sega",                  "Moonwalker (Mega-Tech)", GAME_NOT_WORKING )
/* 41 */ GAME( 1990, mt_crack, megatech, megatech, megatech, 0, ROT0, "Sega",                  "Crack Down (Mega-Tech)", GAME_NOT_WORKING )
/* 42 */ // unknown
/* 43 */ // unknown
/* 44 */ GAME( 1990, mt_arrow, megatech, megatech, megatech, 0, ROT0, "Sega",                  "Arrow Flash (Mega-Tech)", GAME_NOT_WORKING )
/* 45 */ // unknown
/* 46 */ // unknown
/* 47 */ GAME( 1990, mt_astrm, megatech, megatech, megatech, 0, ROT0, "Sega",                  "Alien Storm (Mega-Tech", GAME_NOT_WORKING )
/* 48 */ // unknown
/* 49 */ GAME( 1991, mt_bbros, megatech, megatech, megatech, 0, ROT0, "Sega",                  "Bonanza Bros. (Mega-Tech)", GAME_NOT_WORKING )
/* 50 */ // unknown
/* 51 */ // unknown
/* 52 */ GAME( 1991, mt_sonic, megatech, megatech, megatech, 0, ROT0, "Sega",                  "Sonic The Hedgehog (Mega-Tech, set 1)", GAME_NOT_WORKING )
/*    */ GAME( 1991, mt_sonia, mt_sonic, megatech, megatech, 0, ROT0, "Sega",                  "Sonic The Hedgehog (Mega-Tech, set 2)", GAME_NOT_WORKING )
/* 53 */ GAME( 1990, mt_fshrk, megatech, megatech, megatech, 0, ROT0, "Toaplan / Sega",        "Fire Shark (Mega-Tech)", GAME_NOT_WORKING )
/* 54 */ // unknown
/* 55 */ // unknown
/* 56 */ // unknown
/* 57 */ GAME( 1991, mt_gaxe2, megatech, megatech, megatech, 0, ROT0, "Sega",                  "Golden Axe II (Mega-Tech)", GAME_NOT_WORKING )
/* 58 */ GAME( 1991, mt_stf,   megatech, megatech, megatech, 0, ROT0, "Sega",                  "Joe Montana II: Sports Talk Football (Mega-Tech)", GAME_NOT_WORKING )
/* 59 */ GAME( 1991, mt_mlh,   megatech, megatech, megatech, 0, ROT0, "Sega",                  "Mario Lemieux Hockey (Mega-Tech)", GAME_NOT_WORKING )
/* 60 */ GAME( 1992, mt_kcham, megatech, megatech, megatech, 0, ROT0, "Sega",                  "Kid Chameleon (Mega-Tech)", GAME_NOT_WORKING )
/* 61 */ // unknown
/* 62 */ GAME( 1992, mt_soni2, megatech, megatech, megatech, 0, ROT0, "Sega",                  "Sonic The Hedgehog 2 (Mega-Tech)", GAME_NOT_WORKING )
/*
    Known to Exist, but nn number currently not known:
    Wrestle War
*/
/* more? */

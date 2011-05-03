/***************************************************************************

    NeXT

    05/11/2009 Skeleton driver.

	TODO:
	- RTC check still fails

		000061D0: 61FF FFFF F24E                                      bsr     $5420; (2+)
		000061D6: 4640                                                not.w   D0
		000061D8: 6704                                                beq     $61de
		000061DA: B440                                                cmp.w   D0, D2
		000061DC: 6704                                                beq     $61e2 <---------------- D0 and D2 must be equal there
		000061DE: 70FF                                                moveq   #-$1, D0
		000061E0: 6002                                                bra     $61e4
		000061E2: 4280                                                clr.l   D0
		000061E4: 4CEE 0C04 FFF4                                      movem.l (-$c,A6), D2/A2-A3
		000061EA: 4E5E                                                unlk    A6
		000061EC: 4E75                                                rts

	DASM notes:
    - Jumping to 0x21ee means that the system POST failed
    - 0x258c: ROM test (check with 0x1a)
    - currently fails at 0xfee sub-routine (jumping it starts the MO disk check)

****************************************************************************/

#include "includes/next.h"

bool next_state::screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect)
{
	UINT32 count;
	int x,y,xi;

	count = 0;

	for(y=0;y<832;y++)
	{
		for(x=0;x<1152;x+=16)
		{
			for(xi=0;xi<16;xi++)
			{
				UINT8 pen = (m_vram[count] >> (30-(xi*2))) & 0x3;

				pen ^= 3;

				if((x+xi)>screen.machine().primary_screen->visible_area().max_x || (y)>screen.machine().primary_screen->visible_area().max_y)
					continue;

				*BITMAP_ADDR16(&bitmap, y, x+xi) = screen.machine().pens[pen];
			}

			count++;
		}
	}

    return 0;
}

/* Dummy catcher for any unknown r/w */
READ8_MEMBER( next_state::io_r )
{
	if(!space.debugger_access())
		printf("Read I/O register %08x\n",offset+0x02000000);

	return 0xff;
}

WRITE8_MEMBER( next_state::io_w )
{
	if(!space.debugger_access())
		printf("Write I/O register %08x %02x\n",offset+0x02000000,data);
}

/* map ROM at 0x01000000-0x0101ffff? */
READ32_MEMBER( next_state::rom_map_r )
{
	if(!space.debugger_access())
		printf("%08x ROM MAP?\n",cpu_get_pc(&space.device()));
	return 0x01000000;
}

READ32_MEMBER( next_state::scr2_r )
{
	if(!space.debugger_access())
		printf("%08x\n",cpu_get_pc(&space.device()));
	/*
    x--- ---- ---- ---- ---- ---- ---- ---- dsp reset
    -x-- ---- ---- ---- ---- ---- ---- ---- dsp block end
    --x- ---- ---- ---- ---- ---- ---- ---- dsp unpacked
    ---x ---- ---- ---- ---- ---- ---- ---- dsp mode b
    ---- x--- ---- ---- ---- ---- ---- ---- dsp mode a
    ---- -x-- ---- ---- ---- ---- ---- ---- remote int
    ---- ---x ---- ---- ---- ---- ---- ---- local int
    ---- ---- ---x ---- ---- ---- ---- ---- dram 256k
    ---- ---- ---- ---x ---- ---- ---- ---- dram 1m
    ---- ---- ---- ---- x--- ---- ---- ---- "timer on ipl7"
    ---- ---- ---- ---- -xxx ---- ---- ---- rom waitstates
    ---- ---- ---- ---- ---- x--- ---- ---- ROM 1M
    ---- ---- ---- ---- ---- -x-- ---- ---- MCS1850 rtdata
    ---- ---- ---- ---- ---- --x- ---- ---- MCS1850 rtclk
    ---- ---- ---- ---- ---- ---x ---- ---- MCS1850 rtce
    ---- ---- ---- ---- ---- ---- x--- ---- rom overlay
    ---- ---- ---- ---- ---- ---- -x-- ---- dsp ie
    ---- ---- ---- ---- ---- ---- --x- ---- mem en
    ---- ---- ---- ---- ---- ---- ---- ---x led
    */

	UINT32 data = m_scr2 & 0xfffffbff;

	data |= m_rtc->sdo_r() << 10;

	return data;
}

WRITE32_MEMBER( next_state::scr2_w )
{
	if(!space.debugger_access())
		printf("%08x %08x\n",cpu_get_pc(&space.device()),data);
	COMBINE_DATA(&m_scr2);

	m_rtc->ce_w(BIT(m_scr2, 8));
	m_rtc->sdi_w(BIT(m_scr2, 10));
	m_rtc->sck_w(BIT(m_scr2, 9));
}

READ32_MEMBER( next_state::scr1_r )
{
	/*
        xxxx ---- ---- ---- ---- ---- ---- ---- slot ID
        ---- ---- xxxx xxxx ---- ---- ---- ---- DMA type
        ---- ---- ---- ---- xxxx ---- ---- ---- cpu type
        ---- ---- ---- ---- ---- xxxx ---- ---- board revision
        ---- ---- ---- ---- ---- ---- -xx- ---- video mem speed
        ---- ---- ---- ---- ---- ---- ---x x--- mem speed
        ---- ---- ---- ---- ---- ---- ---- -xxx cpu speed
    */

	return 0x00012002; // TODO
}

READ32_MEMBER( next_state::irq_status_r )
{
	return m_irq_status;
}

WRITE32_MEMBER( next_state::irq_status_w )
{
	COMBINE_DATA(&m_irq_status);
}

READ32_MEMBER( next_state::irq_mask_r )
{
	return m_irq_mask;
}

WRITE32_MEMBER( next_state::irq_mask_w )
{
	COMBINE_DATA(&m_irq_mask);
}

READ32_MEMBER( next_state::modisk_r)
{
	return 0; // return 0 atm
}

READ32_MEMBER( next_state::network_r)
{
	return 0xffffffff;//machine().rand(); // return 0 atm
}

READ32_MEMBER( next_state::unk_r)
{
	return 0xffffffff;//machine().rand(); // return 0 atm
}

static ADDRESS_MAP_START( next_mem, AS_PROGRAM, 32, next_state )
	AM_RANGE(0x00000000, 0x0001ffff) AM_ROM AM_REGION("user1", 0)
	AM_RANGE(0x01000000, 0x0101ffff) AM_ROM AM_REGION("user1", 0)
	AM_RANGE(0x02006000, 0x02006003) AM_MIRROR(0x100000) AM_READ(unk_r)
	AM_RANGE(0x02007000, 0x02007003) AM_MIRROR(0x100000) AM_READWRITE(irq_status_r,irq_status_w)
	AM_RANGE(0x02007800, 0x02007803) AM_MIRROR(0x100000) AM_READWRITE(irq_mask_r,irq_mask_w)

	AM_RANGE(0x0200c000, 0x0200c003) AM_MIRROR(0x100000) AM_READ(scr1_r)
	AM_RANGE(0x0200c800, 0x0200c803) AM_MIRROR(0x100000) AM_READ(rom_map_r)
	AM_RANGE(0x0200d000, 0x0200d003) AM_MIRROR(0x100000) AM_READWRITE(scr2_r,scr2_w)
	AM_RANGE(0x0200e000, 0x0200e00f) AM_MIRROR(0x100000) AM_READ(unk_r) //keyboard

	AM_RANGE(0x02012004, 0x02012007) AM_MIRROR(0x100000) AM_READ(modisk_r)
	AM_RANGE(0x0201a000, 0x0201a003) AM_MIRROR(0x100000) AM_READ(network_r)

	AM_RANGE(0x02000000, 0x0201ffff) AM_MIRROR(0x100000) AM_READWRITE8(io_r,io_w,0xffffffff) //intentional fall-through

	AM_RANGE(0x04000000, 0x07ffffff) AM_RAM //work ram

	AM_RANGE(0x0b000000, 0x0b03ffff) AM_RAM AM_BASE(m_vram) //vram
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( next )
INPUT_PORTS_END

static PALETTE_INIT(next)
{
	/* TODO: simplify this*/
	palette_set_color(machine, 0,MAKE_RGB(0x00,0x00,0x00));
	palette_set_color(machine, 1,MAKE_RGB(0x55,0x55,0x55));
	palette_set_color(machine, 2,MAKE_RGB(0xaa,0xaa,0xaa));
	palette_set_color(machine, 3,MAKE_RGB(0xff,0xff,0xff));
}

static MCCS1850_INTERFACE( rtc_intf )
{
    DEVCB_NULL, // RTCINT
    DEVCB_NULL,
    DEVCB_NULL
};

static const floppy_config next_floppy_config =
{
    DEVCB_NULL,
    DEVCB_NULL,
    DEVCB_NULL,
    DEVCB_NULL,
    DEVCB_NULL,
    FLOPPY_STANDARD_3_5_DSED,
    FLOPPY_OPTIONS_NAME(default),
    "floppy_3_5"
};

static MACHINE_CONFIG_START( next, next_state )
    /* basic machine hardware */
    MCFG_CPU_ADD("maincpu",M68030, XTAL_25MHz)
    MCFG_CPU_PROGRAM_MAP(next_mem)

    /* video hardware */
    MCFG_SCREEN_ADD("screen", RASTER)
    MCFG_SCREEN_REFRESH_RATE(60)
    MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MCFG_SCREEN_SIZE(1120, 832)
    MCFG_SCREEN_VISIBLE_AREA(0, 1120-1, 0, 832-1)

    MCFG_PALETTE_LENGTH(4)
    MCFG_PALETTE_INIT(next)

	// devices
	MCFG_MCCS1850_ADD(MCCS1850_TAG, XTAL_32_768kHz, rtc_intf)
	MCFG_FLOPPY_DRIVE_ADD(FLOPPY_0, next_floppy_config)

	// software list
	MCFG_SOFTWARE_LIST_ADD("flop_list", "next")
MACHINE_CONFIG_END

static MACHINE_CONFIG_DERIVED( next040, next )
	MCFG_CPU_REPLACE("maincpu", M68040, XTAL_33MHz)
	MCFG_CPU_PROGRAM_MAP(next_mem)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( next ) // 68030
    ROM_REGION32_BE( 0x20000, "user1", ROMREGION_ERASEFF )
	ROM_SYSTEM_BIOS( 0, "v12", "v1.2" ) // version? word at 0xC: 5AD0
	ROMX_LOAD( "rev_1.2.bin",     0x0000, 0x10000, CRC(7070bd78) SHA1(e34418423da61545157e36b084e2068ad41c9e24), ROM_BIOS(1)) /* Label: "(C) 1990 NeXT, Inc. // All Rights Reserved. // Release 1.2 // 1142.02", underlabel exists but unknown */
	ROM_SYSTEM_BIOS( 1, "v10", "v1.0 v41" ) // version? word at 0xC: 3090
	ROMX_LOAD( "rev_1.0_v41.bin", 0x0000, 0x10000, CRC(54df32b9) SHA1(06e3ecf09ab67a571186efd870e6b44028612371), ROM_BIOS(2)) /* Label: "(C) 1989 NeXT, Inc. // All Rights Reserved. // Release 1.0 // 1142.00", underlabel: "MYF // 1.0.41 // 0D5C" */
	ROM_SYSTEM_BIOS( 2, "v10p", "v1.0 prototype?" ) // version? word at 0xC: 23D9
	ROMX_LOAD( "rev_1.0_proto.bin", 0x0000, 0x10000, CRC(F44974F9) SHA1(09EAF9F5D47E379CFA0E4DC377758A97D2869DDC), ROM_BIOS(3)) /* Label: "(C) 1989 NeXT, Inc. // All Rights Reserved. // Release 1.0 // 1142.00", no underlabel */
ROM_END

ROM_START( nextnt ) // 68040 non-turbo
	ROM_REGION32_BE( 0x20000, "user1", ROMREGION_ERASEFF )
	ROM_SYSTEM_BIOS( 0, "v25", "v2.5 v66" ) // version? word at 0xC: F302
	ROMX_LOAD( "rev_2.5_v66.bin", 0x0000, 0x20000, CRC(f47e0bfe) SHA1(b3534796abae238a0111299fc406a9349f7fee24), ROM_BIOS(1))
	ROM_SYSTEM_BIOS( 1, "v24", "v2.4 v65" ) // version? word at 0xC: A634
	ROMX_LOAD( "rev_2.4_v65.bin", 0x0000, 0x20000, CRC(74e9e541) SHA1(67d195351288e90818336c3a84d55e6a070960d2), ROM_BIOS(2))
	ROM_SYSTEM_BIOS( 2, "v21a", "v2.1a? v60?" ) // version? word at 0xC: 894C; NOT SURE about correct rom major and v versions of this one!
	ROMX_LOAD( "rev_2.1a_v60.bin", 0x0000, 0x20000, CRC(739D7C07) SHA1(48FFE54CF2038782A92A0850337C5C6213C98571), ROM_BIOS(3)) /* Label: "(C) 1990 NeXT Computer, Inc. // All Rights Reserved. // Release 2.1 // 2918.AB" */
	ROM_SYSTEM_BIOS( 3, "v21", "v2.1 v59" ) // version? word at 0xC: 72FE
	ROMX_LOAD( "rev_2.1_v59.bin", 0x0000, 0x20000, CRC(f20ef956) SHA1(09586c6de1ca73995f8c9b99870ee3cc9990933a), ROM_BIOS(4))
	ROM_SYSTEM_BIOS( 4, "v12", "v1.2 v58" ) // version? word at 0xC: 6372
	ROMX_LOAD( "rev_1.2_v58.bin", 0x0000, 0x20000, CRC(B815B6A4) SHA1(97D8B09D03616E1487E69D26609487486DB28090), ROM_BIOS(5)) /* Label: "V58 // (C) 1990 NeXT, Inc. // All Rights Reserved // Release 1.2 // 1142.02" */
ROM_END

ROM_START( nexttrb ) // 68040 turbo
	ROM_REGION32_BE( 0x20000, "user1", ROMREGION_ERASEFF )
	ROM_SYSTEM_BIOS( 0, "v33", "v3.3 v74" ) // version? word at 0xC: (12)3456
	ROMX_LOAD( "rev_3.3_v74.bin", 0x0000, 0x20000, CRC(fbc3a2cd) SHA1(a9bef655f26f97562de366e4a33bb462e764c929), ROM_BIOS(1))
	ROM_SYSTEM_BIOS( 1, "v32", "v3.2 v72" ) // version? word at 0xC: (01)2f31
	ROMX_LOAD( "rev_3.2_v72.bin", 0x0000, 0x20000, CRC(e750184f) SHA1(ccebf03ed090a79c36f761265ead6cd66fb04329), ROM_BIOS(2))
	ROM_SYSTEM_BIOS( 2, "v30", "v3.0 v70" ) // version? word at 0xC: (01)06e8
	ROMX_LOAD( "rev_3.0_v70.bin", 0x0000, 0x20000, CRC(37250453) SHA1(a7e42bd6a25c61903c8ca113d0b9a624325ee6cf), ROM_BIOS(3))
ROM_END

static DRIVER_INIT( next )
{
	//UINT32 *ROM = (UINT32 *)machine.region("user1")->base();

	//ROM[0x61dc/4] = (0x6604 << 16) | (ROM[0x61dc/4] & 0xffff); //hack until I understand ...

	//ROM[0x00b8/4] = (ROM[0x00b8/4] << 16) | (0x67ff & 0xffff); //patch ROM checksum
}

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY                 FULLNAME               FLAGS */
COMP( 1987, next,   0,          0,   next,      next,    next,   "Next Software Inc",   "NeXT Computer",		GAME_NOT_WORKING | GAME_NO_SOUND)
COMP( 1990, nextnt, next,       0,   next040,   next,    0,      "Next Software Inc",   "NeXT (Non Turbo)",		GAME_NOT_WORKING | GAME_NO_SOUND)
COMP( 1992, nexttrb,next,       0,   next040,   next,    0,      "Next Software Inc",   "NeXT (Turbo)",			GAME_NOT_WORKING | GAME_NO_SOUND)

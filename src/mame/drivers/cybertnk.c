/*******************************************************************************************

Cyber Tank HW (c) 1987 Coreland Technology

WIP driver by Angelo Salese

TODO:
-Understand the communication between Master & Slave CPUs.
-Fix the "USP" bug,which causes IC03 & IC04 to fail the self test.
 For now:
 Set a breakpoint at 1732
 Modify the USP register to 0
 Update: it reads from ROM 0x102 & 0x103,at PC 1a58 & 1a7c,it expects to read 0x61 & 0xe0.
 It seems to be a rom check at 0x20000 (odd & even bytes),but I don't know what rom is
 needed on it.
-Rom "SS5" is missing?Or it's a sound chip that's labelled SS5?
-Rom not yet hooked up?Or a left-over?
 cpu #1 (PC=000006D8): unmapped program memory word read from 00020000 & FF00
-Paletteram is likely to be buffered,the two banks are really similar.

============================================================================================
Cyber Tank
Coreland Technology, Inc 1987

---------------------
BA87015

   SS2               -
   SS4               SS3
   -                 -
   -                 SS1

        Y8950             Y8950

    2064
    SS5

    Z80B
    3.5795MHz

---------------------
BA87035

        68000-10      20MHz    68000-10
        SUBH  SUBL             P2A  P1A
        2064  2064             2064 2064


 C04  C03  C02  C01
 C08  C07  C06  C05
 C12  C11  C10  C09
 C16  C15  C14  C13                        2016
                                           2016
                                        W31003
               SW1 SW2 SW3

---------------------
BA87034

  T2   T1                               43256   43256
              IC19  IC20                43256   43256
              IC29  IC30
                                        43256   43256
                                        43256   43256

                                        43256   43256
  T3                                    43256   43256
  T4
                                        43256   43256
                                        43256   43256



                                                  W31004
                                                  W31004
----------------------
BA87033

     22.8MHz    IC2                       2016     W31001
                           IC15           2016     W31004
                                          ROAD_CHL
                                          ROAD_CHH


                        S01
      2064              S02
      2064              S03                    T6          T5
    W31002       2064   S04      W31004

                        S05
                        S06
      2064              S07                    2064        2064
      2064       2064   S08      W31004        2064        2064
    W31002
                        S09
      2064              S10
      2064              S11
    W31002       2064   S12      W31004
                                                VID1CONN  VID2CONN
********************************************************************************************
M68k Master irq table:
lev 1 : 0x64 : 0000 0870 - vblank
lev 2 : 0x68 : 0000 0caa - input device clear?
lev 3 : 0x6c : 0000 0caa - input device clear?
lev 4 : 0x70 : 0000 0caa - input device clear?
lev 5 : 0x74 : 0000 0caa - input device clear?
lev 6 : 0x78 : 0000 0caa - input device clear?
lev 7 : 0x7c : ffff ffff - illegal

M68k Slave irq table:
lev 1 : 0x64 : 0000 07e0 - input device clear?
lev 2 : 0x68 : 0000 07e0 - input device clear?
lev 3 : 0x6c : 0000 0764 - vblank?
lev 4 : 0x70 : 0000 07e0 - input device clear?
lev 5 : 0x74 : 0000 07e0 - input device clear?
lev 6 : 0x78 : 0000 07e0 - input device clear?
lev 7 : 0x7c : 0000 07e0 - input device clear?

*******************************************************************************************/

#include "driver.h"
#include "sound/3812intf.h"

static tilemap *tx_tilemap;
static UINT16 *tx_vram;
static UINT16 *slave_data;
static UINT16 *com_ram;
//static UINT16 mux_data;

#define MASTER_CPU (cpu_getactivecpu() == 0)
#define SLAVE_CPU (cpu_getactivecpu() == 1)

static void get_tx_tile_info(int tile_index)
{
	int code = tx_vram[tile_index];
	SET_TILE_INFO(
			0,
			code & 0x1fff,
			(code & 0xe000) >> 13,
			0)
}

VIDEO_START( cybertnk )
{
	tx_tilemap = tilemap_create(get_tx_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE,8,8,128,32);

	return 0;
}

VIDEO_UPDATE( cybertnk )
{
	tilemap_draw(bitmap,cliprect,tx_tilemap,0,0);
	return 0;
}

static WRITE16_HANDLER( tx_vram_w )
{
	int oldword = tx_vram[offset];
	int newword = oldword;
	COMBINE_DATA(&newword);

	if (oldword != newword)
	{
		tx_vram[offset] = newword;
		tilemap_mark_tile_dirty(tx_tilemap,offset);
	}
}

static UINT16 master_cmd;

static WRITE16_HANDLER( master_send )
{
	COMBINE_DATA(&master_cmd);
}

static READ16_HANDLER( slave_receive )
{
	popmessage("CPU #1 (PC=%06x) CMD = %04x",activecpu_get_pc(),master_cmd);
	return master_cmd;
}

static READ16_HANDLER( master_receive )
{
	popmessage("CPU #0 (PC=%06x) CMD = %04x",activecpu_get_pc(),slave_data[0x10/2]);
	return slave_data[0x10/2];
}

/*WRONG*/
static READ8_HANDLER( master_to_sound )
{
 	switch(offset & 1)
 	{
		case 0: return ((master_cmd & 0x00ff) >> 0);
		case 1: return ((master_cmd & 0xff00) >> 8);
	}

	return 0;//make the compiler happy...
}

/*I think this is the Network communication*/
static UINT8 master_tx,slave_tx;

static READ16_HANDLER( network_r )
{
	logerror("(PC=%06x) Read to input area [%04x]\n",activecpu_get_pc(),offset*2);

	switch(offset)
	{
		case 0x002/2:
			//mame_printf_debug("read at %06x %01x\n",activecpu_get_pc(),cpu_getactivecpu());
			if(MASTER_CPU)
				return slave_tx;//master RX
			else if(SLAVE_CPU)
				return master_tx;//slave RX
	}

	return com_ram[offset];
}

static WRITE16_HANDLER( network_w )
{
	COMBINE_DATA(&com_ram[offset]);

	switch(offset)
	{
		case 0x002/2:
			if(ACCESSING_LSB && MASTER_CPU)
				master_tx = ((data & 0xff) << 8); //master TX
			if(ACCESSING_MSB && SLAVE_CPU)
				slave_tx = ((data & 0xff00) >> 8); //slave TX
			break;
	}
}

static ADDRESS_MAP_START( master_mem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_ROM
	AM_RANGE(0x080000, 0x087fff) AM_RAM /*Work RAM*/
	AM_RANGE(0x0a0000, 0x0a0fff) AM_RAM
	AM_RANGE(0x0c0000, 0x0c3fff) AM_READWRITE(MRA16_RAM, tx_vram_w) AM_BASE(&tx_vram)
	AM_RANGE(0x0c4000, 0x0cffff) AM_RAM
	AM_RANGE(0x0e0008, 0x0e0009) AM_WRITE(master_send) /*Master->Slave*/
//  AM_RANGE(0x0e000a, 0x0e000b) AM_READ(master_receive_2)
	AM_RANGE(0x0e0010, 0x0e0011) AM_READ(master_receive) /*Slave->Master*/
	AM_RANGE(0x100000, 0x107fff) AM_READWRITE(MRA16_RAM, paletteram16_xBBBBBGGGGGRRRRR_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0x110000, 0x1101ff) AM_READWRITE(network_r,network_w) AM_BASE(&com_ram)
ADDRESS_MAP_END

static ADDRESS_MAP_START( slave_mem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x01ffff) AM_ROM
	AM_RANGE(0x07fff8, 0x07fff9) AM_READ(slave_receive) /*Master->Slave*/
	AM_RANGE(0x07fffa, 0x07fffb) AM_READ(slave_receive) /*Mirror?*/
	AM_RANGE(0x080000, 0x083fff) AM_RAM/*Work RAM*/
	AM_RANGE(0x0c0000, 0x0c3fff) AM_RAM
	AM_RANGE(0x100000, 0x100fff) AM_RAM AM_BASE(&slave_data)
	AM_RANGE(0x110000, 0x1101ff) AM_RAM
	AM_RANGE(0x140000, 0x1401ff) AM_READWRITE(network_r,network_w) AM_BASE(&com_ram)
ADDRESS_MAP_END

/*WRONG*/
static ADDRESS_MAP_START( sound_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff ) AM_ROM
	AM_RANGE(0x8000, 0x9fff ) AM_RAM
	AM_RANGE(0xa000, 0xa000 ) AM_READWRITE(Y8950_status_port_0_r,Y8950_control_port_0_w)
	AM_RANGE(0xa001, 0xa001 ) AM_WRITE(Y8950_write_port_0_w)
	AM_RANGE(0xa005, 0xa006 ) AM_READ(master_to_sound)
	AM_RANGE(0xc000, 0xc000 ) AM_READWRITE(Y8950_status_port_1_r,Y8950_control_port_1_w)
	AM_RANGE(0xc001, 0xc001 ) AM_WRITE(Y8950_write_port_1_w)
ADDRESS_MAP_END

INPUT_PORTS_START( cybertnk )
	/*Some inputs maps directly in ram,current implementation doesn't use it ATM*/
	PORT_START_TAG("HAND1")
	PORT_BIT( 0xff, 0x00, IPT_PEDAL ) PORT_SENSITIVITY(100) PORT_KEYDELTA(20)

	PORT_START_TAG("HAND2")
	PORT_BIT( 0xff, 0x00, IPT_PEDAL2 ) PORT_SENSITIVITY(100) PORT_KEYDELTA(20)

	PORT_START_TAG("HAND3")
	PORT_BIT( 0xff, 0x00, IPT_PEDAL3 ) PORT_SENSITIVITY(100) PORT_KEYDELTA(20)

	PORT_START_TAG("HAND4")
	PORT_BIT( 0xff, 0x7f, IPT_PADDLE ) PORT_SENSITIVITY(100) PORT_KEYDELTA(4)


	PORT_START_TAG("DSW1-2")
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START_TAG("DSW3")
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BIT( 	  0xff00, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

static const gfx_layout tile_8x8x4 =
{
	8,8,
	RGN_FRAC(1,4),
    4,
    { RGN_FRAC(0,4),RGN_FRAC(1,4),RGN_FRAC(2,4),RGN_FRAC(3,4) },
    { STEP8(0,1) },
    { STEP8(0,8) },
    8*8
};

static const gfx_layout tile_16x16x4 =
{
	16,16,
	RGN_FRAC(1,4),
    4,
    { RGN_FRAC(0,4),RGN_FRAC(1,4),RGN_FRAC(2,4),RGN_FRAC(3,4) },
    { STEP16(0,1) },
    { STEP16(0,16) },
    32*8
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tile_8x8x4,     0x1400, 16 }, /*Pal offset???*/
	{ REGION_GFX2, 0, &tile_8x8x4,     0,      0x400 },
	{ REGION_GFX2, 0, &tile_16x16x4,   0,      0x400 },
	{ REGION_GFX3, 0, &tile_8x8x4,     0,      0x400 },
	{ REGION_GFX3, 0, &tile_16x16x4,   0,      0x400 },
	{ -1 }	/* end of array */
};

static INTERRUPT_GEN( master_irq )
{
	switch(cpu_getiloops())
	{
		case 0: cpunum_set_input_line(0,1,HOLD_LINE); break;
		case 1: cpunum_set_input_line(0,3,HOLD_LINE); break;
	}
}

static INTERRUPT_GEN( slave_irq )
{
	switch(cpu_getiloops())
	{
		case 0: cpunum_set_input_line(0,3,HOLD_LINE); break;
		case 1: cpunum_set_input_line(0,1,HOLD_LINE); break;
	}
}

static struct Y8950interface y8950_interface = {
	0, /* TODO */
	REGION_SOUND1	/* memory region */
};

static MACHINE_DRIVER_START( cybertnk )
	MDRV_CPU_ADD(M68000,20000000/2)
	MDRV_CPU_PROGRAM_MAP(master_mem,0)
	MDRV_CPU_VBLANK_INT(master_irq,2)

	MDRV_CPU_ADD(M68000,20000000/2)
	MDRV_CPU_PROGRAM_MAP(slave_mem,0)
	MDRV_CPU_VBLANK_INT(slave_irq,2)

	MDRV_CPU_ADD(Z80,3579500)
	MDRV_CPU_PROGRAM_MAP(sound_mem,0)

	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_INTERLEAVE(100)//arbitrary value,needed to get the communication to work

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_SCREEN_VISIBLE_AREA(0*8, 64*8-1, 2*8, 32*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x4000)

	MDRV_VIDEO_START(cybertnk)
	MDRV_VIDEO_UPDATE(cybertnk)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(Y8950, 3579500)
	MDRV_SOUND_CONFIG(y8950_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 1.0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 1.0)

	MDRV_SOUND_ADD(Y8950, 3579500)
	MDRV_SOUND_CONFIG(y8950_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 1.0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 1.0)
MACHINE_DRIVER_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( cybertnk )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "p1a",   0x00000, 0x20000, CRC(de7ff83a) SHA1(64a34443b532db24ec2c86f0e288eaf12a2212af) )
	ROM_LOAD16_BYTE( "p2a",   0x00001, 0x20000, CRC(9b6afa26) SHA1(387a6eb6e5da9752869fcc6433cc7516a28d6d30) )

	ROM_REGION( 0x20000, REGION_CPU2, 0 )
	ROM_LOAD16_BYTE( "subl",   0x00000, 0x10000, CRC(3814a2eb) SHA1(252800b21f5cfada34ef5208cda33088daab132b) )
	ROM_LOAD16_BYTE( "subh",   0x00001, 0x10000, CRC(1af7ad58) SHA1(450c65289729d74cd4d17e11be16469246e61b7d) )

	ROM_REGION( 0x8000, REGION_CPU3, 0 )
	ROM_LOAD( "ss1",    0x0000, 0x8000, CRC(c3ba160b) SHA1(cfbfcad443ff83cd4e707f045a650417aca03d85) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )
	ROM_LOAD( "ss2",    0x00000, 0x20000, CRC(da4b8733) SHA1(177372a53fd49629d1cda83bdd324ee90fbcdbb5) )
	/*The following two are identical*/
	ROM_LOAD( "ss3",    0x20000, 0x20000, CRC(cecdea53) SHA1(7e6a6499cab4720f4b6d6d8988bb9dd5766511ab) )
	ROM_LOAD( "ss4",    0x40000, 0x20000, CRC(cecdea53) SHA1(7e6a6499cab4720f4b6d6d8988bb9dd5766511ab) )
	//ss5?

	ROM_REGION( 0x40000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "s09", 0x00000, 0x10000, CRC(69e6470c) SHA1(8e7db6988366cae714fff72449623a7977af1db1) )
	ROM_LOAD( "s10", 0x10000, 0x10000, CRC(77230f44) SHA1(b79fc841fa784d23855e4085310cee435c11348f) )
	ROM_LOAD( "s11", 0x20000, 0x10000, CRC(bfda980d) SHA1(1f975fdd2cfdc345eeb03fbc26fc1be1b2d7737e) )
	ROM_LOAD( "s12", 0x30000, 0x10000, CRC(8a11fcfa) SHA1(a406ac9cf841dd9d829cb83bfe8feb5128a3e77e) )

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "s01", 0x00000, 0x10000, CRC(6513452c) SHA1(95ed2da8f90e16c50716011577606a7dc93ba65e) )
	ROM_LOAD( "s02", 0x10000, 0x10000, CRC(3a270e3b) SHA1(97c8282d4d782c9d2fcfb5e5dabbe1ca88978f5c) )
	ROM_LOAD( "s03", 0x20000, 0x10000, CRC(584eff66) SHA1(308ec058693ce3ce34b058a8dbeedf342134311c) )
	ROM_LOAD( "s04", 0x30000, 0x10000, CRC(51ba5402) SHA1(c4522c4562ce0514bef3257e323bcc255b635544) )

	ROM_REGION( 0x40000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "s05", 0x00000, 0x10000, CRC(bddb6008) SHA1(bacb822bac4893eee0648a19ce449e5559d32b5e) )
	ROM_LOAD( "s06", 0x10000, 0x10000, CRC(d65b0fa5) SHA1(ce398a52ad408778fd910c42a9618194b862becf) )
	ROM_LOAD( "s07", 0x20000, 0x10000, CRC(70220567) SHA1(44b48ded8581a6d78b27a3af833f62413ff31c76) )
	ROM_LOAD( "s08", 0x30000, 0x10000, CRC(988c4fcb) SHA1(68d32be70605ad5415f2b6aeabbd92e269f0c9af) )

	/*The following ROM regions aren't checked yet*/
	ROM_REGION( 0x200000, REGION_USER1, 0 )
	ROM_LOAD( "c01" , 0x000000, 0x20000, CRC(f7021069) SHA1(67835750f39effd362ccaee381765afce8fa16b2) )
	ROM_LOAD( "c02" , 0x020000, 0x20000, CRC(665e193c) SHA1(12c116da0a2d4e881d8598727ff63299fb98c6d2) )
	ROM_LOAD( "c03" , 0x040000, 0x20000, CRC(f230d700) SHA1(60fdba4b0fe4df5507e999bed917da93c6cd9a9c) )
	ROM_LOAD( "c04" , 0x060000, 0x20000, CRC(999fd57d) SHA1(3e8b8dac595555419831784a27f95420e10b58bd) )
	ROM_LOAD( "c05" , 0x080000, 0x20000, CRC(9bafb49c) SHA1(6deddbaa44c8e11e0ac73a5330935a9a260b5d43) )
	ROM_LOAD( "c06" , 0x0a0000, 0x20000, CRC(e60de7a2) SHA1(9daa820eefddf079e3940341acc316b1f19ba7ed) )
	ROM_LOAD( "c07" , 0x0c0000, 0x20000, CRC(e7cf992a) SHA1(610b2c78a16d8a9d420b1513e2dcfa693f1d8b42) )
	ROM_LOAD( "c08" , 0x0e0000, 0x20000, CRC(ce0343b9) SHA1(ef511a04709c49250b32c5b47a6f5024af8acc5b) )
	ROM_LOAD( "c09" , 0x100000, 0x20000, CRC(63a443d1) SHA1(9c0fdca3f8e65dc984ec3c089b379c5a61066630) )
	ROM_LOAD( "c10" , 0x120000, 0x20000, CRC(01331635) SHA1(8af7fbe2609b6d96bcd63d884cf92095593130ff) )
	ROM_LOAD( "c11" , 0x140000, 0x20000, CRC(d46ccfa3) SHA1(c872bc5a25f0b574cb2f9d3b1dff36c3eff751b4) )
	ROM_LOAD( "c12" , 0x160000, 0x20000, CRC(c3c39c4a) SHA1(93f3572dd62ef7a92044345249efb0d9ec99bdf9) )
	ROM_LOAD( "c13" , 0x180000, 0x20000, CRC(0f366b92) SHA1(2361ac9b1309d5fbd1dec93ca5aecdf45deaeaed) )
	ROM_LOAD( "c14" , 0x1a0000, 0x20000, CRC(406d5a0d) SHA1(51e4e85d9c63ef687671fbb213b14d66930070ce) )
	ROM_LOAD( "c15" , 0x1c0000, 0x20000, CRC(ad681c70) SHA1(84c6589464103091b39f1ccdbfed10bf538452f3) )
	ROM_LOAD( "c16" , 0x1e0000, 0x20000, CRC(1f44dbb6) SHA1(ea1368d6367a2de6d5e6764f8ab705b182d6d276) )

	ROM_REGION( 0x200000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "road_chl" , 0x000000, 0x20000, CRC(862b109c) SHA1(9f81918362218ddc0a6bf0a5317c5150e514b699) )
	ROM_LOAD16_BYTE( "road_chh" , 0x000001, 0x20000, CRC(9dedc988) SHA1(10bae1be0e35320872d4994f7e882cd1de988c90) )

	ROM_REGION( 0x30000, REGION_USER3, 0 )
	ROM_LOAD( "t1",   0x00000, 0x08000, CRC(24890512) SHA1(2a6c9d39ca0c1c8316e85d9f565f6b3922d596b2) )
	ROM_LOAD( "t2",   0x08000, 0x08000, CRC(5a10480d) SHA1(f17598442091dae14abe3505957d94793f3ed886))
	ROM_LOAD( "t3",   0x10000, 0x08000, CRC(454af4dc) SHA1(e5b18a37715e50db2243432564f5a04fb39dea60) )
	ROM_LOAD( "t4",   0x18000, 0x08000, CRC(0e1ef6a9) SHA1(d230841bbee6d07bab05aa8d37ec2409fc6278bc) )
	/*The following two are identical*/
	ROM_LOAD( "t5",   0x20000, 0x08000, CRC(12eb51bc) SHA1(35708eb456207ebee498c70dd82340b364797c56) )
	ROM_LOAD( "t6",   0x28000, 0x08000, CRC(12eb51bc) SHA1(35708eb456207ebee498c70dd82340b364797c56) )

	ROM_REGION( 0x280, REGION_PROMS, 0 )
	ROM_LOAD( "ic2",  0x0000, 0x0100, CRC(aad2a447) SHA1(a12923027e3093bd6d358af44d35d2e8e588dd1a) )//road proms related?
	ROM_LOAD( "ic15", 0x0100, 0x0100, CRC(5f8c2c00) SHA1(50162503ac0ee9395377d7e45a84672a9493fb7d) )
	ROM_LOAD( "ic19", 0x0200, 0x0020, CRC(bd15cd71) SHA1(e0946d12eebd5db8707d965be157914d70f7472b) )//T1-T6 proms related?
	ROM_LOAD( "ic20", 0x0220, 0x0020, CRC(2f237563) SHA1(b0081c1cc6e357a6f10ab1ff357bd4e989ec7fb3) )
	ROM_LOAD( "ic29", 0x0240, 0x0020, CRC(95b32c0f) SHA1(5a19f441ced983bacbf3bc1aaee94ca768166447) )
	ROM_LOAD( "ic30", 0x0260, 0x0020, CRC(2bb6033f) SHA1(eb994108734d7d04f8e293eca21bb3051a63cfe9) )
ROM_END

GAME( 1990, cybertnk,  0,       cybertnk,  cybertnk,  0, ROT0, "Coreland", "Cyber Tank (v1.04)", GAME_NO_SOUND|GAME_NOT_WORKING )

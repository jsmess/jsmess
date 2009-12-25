/**************************************************************************
 *
 * gp2x.c - Game Park Holdings GP2X
 * Skeleton by R. Belmont
 *
 * CPU: MagicEyes MP2520F SoC
 * MP2520F consists of:
 *    ARM920T CPU core + MMU
 *    ARM940T slave CPU w/o MMU
 *    LCD controller
 *    DMA controller
 *    Interrupt controller
 *    NAND flash interface
 *    MMC/SD Card interface
 *    USB controller
 *    and more.
 *
 **************************************************************************/

#include "driver.h"
#include "video/generic.h"
#include "cpu/arm7/arm7.h"

static UINT32 *gp2x_ram;

static UINT16 gp2x_vidregs[0x200/2];
#if 0
static const char* gp2x_regnames[0x200] =
{
	"DPC Control",
	"PAD Control",
	"PAD Polarity Control 1",
	"PAD Polarity Control 2",
	"PAD Polarity Control 3",
	"PAD Enable Control 1",
	"PAD Enable Control 2",
	"PAD Enable Control 3",
	"",
	"",
	"",
	"Active width",
	"Active height",
	"HSYNC width",
	"HSYNC start",
	"HSYNC end",
	"VSYNC control", 	// 20
	"VSYNC end",
	"",
	"DE Control",
	"S Control",
	"FG Control",
	"LP Control",
	"CLKV Control",
	"CLKV Control 2",	// 30
	"POL Control",
	"CIS Sync Control",
	"",
	"",
	"Luma blank level",
	"Chroma blank level",
	"Luma pos sync",
	"Luma neg sync",	// 40
	"Chroma pos sync",
	"Chroma neg sync",
	"Interrupt control",
	"Clock control",
	"",
	"",
	"",
	"",			// 50
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",			// 60
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",			// 70
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"Overlay control",	// 80
	"Image effect",
	"Image control",
	"Scale factor A top",
	"Scale factor A bottom",
	"Scale factor A top 2 H",
	"Scale factor A top 2 L",
	"Scale factor A bottom 2 H",
	"Scale factor A bottom 2 L",	// 90
	"Width region A top",
	"Width region A bottom",
	"H Offset region A",
	"H Offset region A 2",
	"V Start region A",
	"V End region A top",
	"V End region A bottom",
	"YUV Source region A L",	// a0
	"YUV Source region A H",
};
#endif
static VIDEO_UPDATE( gp2x )
{
	// display enabled?
	if (gp2x_vidregs[0] & 1)
	{
		// only support RGB still image layer for now
		if (gp2x_vidregs[0x80/2] & 4)
		{
			int x, y;
			UINT16 *vram = (UINT16 *)&gp2x_ram[0x2100000/4];

/*          printf("RGB still image 1 enabled, bpp %d, size is %d %d %d %d\n",
                (gp2x_vidregs[(0xda/2)]>>9)&3,
                gp2x_vidregs[(0xe2/2)],
                gp2x_vidregs[(0xe4/2)],
                gp2x_vidregs[(0xe6/2)],
                gp2x_vidregs[(0xe8/2)]);*/


			for (y = 0; y < 240; y++)
			{
				UINT32 *scanline = BITMAP_ADDR32(bitmap, y, 0);

				for (x = 0; x < 320; x++)
				{
					UINT16 pixel = vram[(320*y)+x];

					*scanline++ = MAKE_ARGB(0xff, (pixel>>11)<<3, ((pixel>>5)&0x3f)<<2, (pixel&0x1f)<<3);
				}
			}
		}
	}

	return 0;
}

static READ32_HANDLER( gp2x_lcdc_r )
{
	return gp2x_vidregs[offset*2] | gp2x_vidregs[(offset*2)+1]<<16;
}

static WRITE32_HANDLER( gp2x_lcdc_w )
{
	if (mem_mask == 0xffff)
	{
		gp2x_vidregs[offset*2] = data;
//      printf("%x to video reg %x (%s)\n", data, offset*2, gp2x_regnames[offset*2]);
	}
	else if (mem_mask == 0xffff0000)
	{
		gp2x_vidregs[(offset*2)+1] = data>>16;
//      printf("%x to video reg %x (%s)\n", data>>16, (offset*2)+1, gp2x_regnames[(offset*2)+1]);
	}
	else
	{
		logerror("GP2X LCDC: ERROR: write %x to vid reg @ %x mask %08x\n", data, offset, mem_mask);
	}
}

static UINT32 nand_ptr = 0, nand_cmd, nand_subword_stage;
static UINT32 nand_stage = 0, nand_ptr_temp = 0;
static READ32_HANDLER(nand_r)
{
	UINT32 *ROM = (UINT32 *)memory_region(space->machine, "maincpu");
	UINT32 ret;

	if (offset == 0)
	{
		switch (nand_cmd)
		{
			case 0:
				if (mem_mask == 0xffffffff)
				{
					return ROM[nand_ptr++];
				}
				else if (mem_mask == 0x000000ff)  	// byte-wide reads?
				{
					switch (nand_subword_stage++)
					{
						case 0:
							return ROM[nand_ptr];
						case 1:
							return ROM[nand_ptr]>>8;
						case 2:
							return ROM[nand_ptr]>>16;
						case 3:
							ret = ROM[nand_ptr]>>24;
							nand_subword_stage = 0;
							nand_ptr++;
							return ret;
						default:
							logerror("Bad nand_subword_stage = %d\n", nand_subword_stage);
							break;
					}
				}
				break;

			case 0x50:	// read out-of-band data
				return 0xff;

			case 0x90:	// read ID
				switch (nand_stage++)
				{
					case 0:
						return 0xec;	// Samsung
					case 1:
						return 0x76;	// 64MB
				}
				break;

			default:
				logerror("NAND: read unk command %x (PC %x)\n", nand_cmd, cpu_get_pc(space->cpu));
				break;
		}
	}

//  printf("Read unknown nand offset %x\n", offset);

	return 0;
}

static WRITE32_HANDLER(nand_w)
{
	switch (offset)
	{
		case 4:	// command
			nand_cmd = data;
//          printf("NAND: command %x (PC %x0)\n", data, cpu_get_pc(space->cpu));
			nand_stage = 0;
			nand_subword_stage = 0;
			break;

		case 6:	// address
			if (nand_cmd == 0)
			{
				switch (nand_stage)
				{
					case 0:
						nand_ptr_temp &= ~0xff;
						nand_ptr_temp |= data;
						break;
					case 1:
						nand_ptr_temp &= ~0xff00;
						nand_ptr_temp |= data<<8;
						break;
					case 2:
						nand_ptr_temp &= ~0xff0000;
						nand_ptr_temp |= data<<16;
						break;
					case 3:
						nand_ptr_temp &= ~0xff000000;
						nand_ptr_temp |= data<<24;

//                      printf("NAND: ptr now %x, /2 %x, replacing %x\n", nand_ptr_temp, nand_ptr_temp/2, nand_ptr);
						nand_ptr = nand_ptr_temp/2;
						break;
				}
				nand_stage++;
			}
			break;

		default:
			logerror("nand_w: %x to unknown offset %x\n", data, offset);
			break;
	}
}

static READ32_HANDLER(tx_status_r)
{
	return 0x6;	// tx ready, tx empty
}

static WRITE32_HANDLER(tx_xmit_w)
{
	printf("%c", data&0xff);
}

static UINT32 timer = 0;
static READ32_HANDLER(timer_r)
{
	return timer++;
}

static READ32_HANDLER(nand_ctrl_r)
{
	return 0x8000<<16;		// timed out
}

static WRITE32_HANDLER(nand_ctrl_w)
{
//  printf("%08x to nand_ctrl_w\n", data);
}

static READ32_HANDLER(sdcard_r)
{
	return 0xffff<<16;	// at 3e146b0 - indicate timeout & CRC error
}

static ADDRESS_MAP_START( gp2x_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x00007fff) AM_ROM
	AM_RANGE(0x01000000, 0x04ffffff) AM_RAM	AM_BASE(&gp2x_ram) // 64 MB of RAM
	AM_RANGE(0x9c000000, 0x9c00001f) AM_READWRITE(nand_r, nand_w)
	AM_RANGE(0xc0000a00, 0xc0000a03) AM_READ(timer_r)
	AM_RANGE(0xc0001208, 0xc000120b) AM_READ(tx_status_r)
	AM_RANGE(0xc0001210, 0xc0001213) AM_WRITE(tx_xmit_w)
	AM_RANGE(0xc0001508, 0xc000150b) AM_READ(sdcard_r)
	AM_RANGE(0xc0002800, 0xc00029ff) AM_READWRITE(gp2x_lcdc_r, gp2x_lcdc_w)
	AM_RANGE(0xc0003a38, 0xc0003a3b) AM_READWRITE(nand_ctrl_r, nand_ctrl_w)
ADDRESS_MAP_END

static INPUT_PORTS_START( gp2x )
INPUT_PORTS_END

static MACHINE_DRIVER_START( gp2x )
	MDRV_CPU_ADD("maincpu", ARM9, 80000000)
	MDRV_CPU_PROGRAM_MAP(gp2x_map)

	MDRV_PALETTE_LENGTH(32768)

	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_RGB32)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_SIZE(320, 240)
	MDRV_SCREEN_VISIBLE_AREA(0, 319, 0, 239)

	MDRV_VIDEO_UPDATE(gp2x)

	MDRV_SPEAKER_STANDARD_STEREO("lspeaker", "rspeaker")
MACHINE_DRIVER_END

ROM_START(gp2x)
	ROM_REGION( 0x600000, "maincpu", 0 )	// contents of NAND flash
        ROM_LOAD( "gp2xboot.img", 0x000000, 0x05b264, CRC(9d82937e) SHA1(9655f04c11f78526b3b8a4613897070df3119ead) )
        ROM_LOAD( "gp2xkernel.img", 0x080000, 0x09a899, CRC(e272eac9) SHA1(9d814ad6c27d22812ba3f101a7358698d1b035eb) )
	ROM_LOAD( "gp2xsound.wav", 0x1a0000, 0x03a42c, CRC(b6ac0ade) SHA1(32362ebbcd09a3b15e164ec3fbd2a8f11159bcd7) )
ROM_END

ROM_START(gp2x3)
	ROM_REGION( 0x600000, "maincpu", ROMREGION_ERASEFF )	// contents of NAND flash
	ROM_LOAD( "gp2xboot.img", 0x000000, 0x05b264, CRC(ab1fc556) SHA1(2ce66fec325b6e8e29f540322aefb435d8e3baf0) )
	ROM_LOAD( "gp2xkernel.img", 0x080000, 0x09f047, CRC(6f8922ae) SHA1(0f44204757f9251c7d68560ff31a1ce72f16698e) )
	ROM_LOAD( "gp2xsound.wav", 0x1a0000, 0x03a42c, CRC(b6ac0ade) SHA1(32362ebbcd09a3b15e164ec3fbd2a8f11159bcd7) )
	ROM_LOAD( "gp2xyaffs.img", 0x300000, 0x2ec4d0, CRC(f81a4a57) SHA1(73bb84798ad3a4e281612a47abb2da4772cbe6cd) )
ROM_END

ROM_START(gp2x4)
	ROM_REGION( 0x600000, "maincpu", 0 )	// contents of NAND flash
        ROM_LOAD( "gp2xboot.img", 0x000000, 0x05b264, CRC(160af379) SHA1(0940fc975960fa52f32a9258a9480cc5cb1140e2) )
        ROM_LOAD( "gp2xkernel.img", 0x080000, 0x0a43a8, CRC(77b1cf9c) SHA1(7e759e4581399bfbee982e1b6c3b54a10b0e9c3d) )
        ROM_LOAD( "gp2xsound.wav", 0x1a0000, 0x03a42c, CRC(b6ac0ade) SHA1(32362ebbcd09a3b15e164ec3fbd2a8f11159bcd7) )
        ROM_LOAD( "gp2xyaffs.img", 0x300000, 0x2dfed0, CRC(e77efc53) SHA1(21477ff77aacb84005bc465a03066d71031a6098) )
ROM_END

CONS(2005, gp2x,  0,    0, gp2x, gp2x, 0, "Game Park Holdings", "GP2X 2.0", GAME_NOT_WORKING|GAME_NO_SOUND)
CONS(2005, gp2x3, gp2x, 0, gp2x, gp2x, 0, "Game Park Holdings", "GP2X 3.0", GAME_NOT_WORKING|GAME_NO_SOUND)
CONS(2005, gp2x4, gp2x, 0, gp2x, gp2x, 0, "Game Park Holdings", "GP2X 4.0", GAME_NOT_WORKING|GAME_NO_SOUND)



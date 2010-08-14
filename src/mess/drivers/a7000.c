/***************************************************************************

        Acorn Archimedes 7000/7000+

        30/07/2010 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/arm7/arm7.h"

static VIDEO_START( a7000 )
{
}

static VIDEO_UPDATE( a7000 )
{
    return 0;
}

/* TODO: some of these registers are actually ARM7500 specific */
static const char *const iomd_regnames[] =
{
	"I/O Control",						// 0x000 IOCR
	"Keyboard Data",					// 0x004 KBDDAT
	"Keyboard Control",					// 0x008 KBDCR
	"General Purpose I/O Lines",		// 0x00c IOLINES
	"IRQA Status",						// 0x010 IRQSTA
	"IRQA Request/clear",				// 0x014 IRQRQA
	"IRQA Mask",						// 0x018 IRQMSKA
	"Enter SUSPEND Mode",				// 0x01c SUSMODE
	"IRQB Status",						// 0x020 IRQSTB
	"IRQB Request/clear",				// 0x024 IRQRQB
	"IRQB Mask",						// 0x028 IRQMSKB
	"Enter STOP Mode",					// 0x02c STOPMODE
	"FIQ Status",						// 0x030 FIQST
	"FIQ Request/clear",				// 0x034 FIQRQ
	"FIQ Mask",							// 0x038 FIQMSK
	"Clock divider control",			// 0x03c CLKCTL
	"Timer 0 Low Bits",					// 0x040 T0LOW
	"Timer 0 High Bits",				// 0x044 T0HIGH
	"Timer 0 Go Command",				// 0x048 T0GO
	"Timer 0 Latch Command",			// 0x04c T0LATCH
	"Timer 1 Low Bits",					// 0x050 T1LOW
	"Timer 1 High Bits",				// 0x054 T1HIGH
	"Timer 1 Go Command",				// 0x058 T1GO
	"Timer 1 Latch Command",			// 0x05c T1LATCH
	"IRQC Status",						// 0x060 IRQSTC
	"IRQC Request/clear",				// 0x064 IRQRQC
	"IRQC Mask",						// 0x068 IRQMSKC
	"LCD and IIS Control Bits",			// 0x06c VIDIMUX
	"IRQD Status",						// 0x070 IRQSTD
	"IRQD Request/clear",				// 0x074 IRQRQD
	"IRQD Mask",						// 0x078 IRQMSKD
	"<RESERVED>",						// 0x07c
	"ROM Control Bank 0",				// 0x080 ROMCR0
	"ROM Control Bank 1",				// 0x084 ROMCR1
	"DRAM Control (IOMD)",				// 0x088 DRAMCR
	"VRAM and Refresh Control",			// 0x08c VREFCR
	"Flyback Line Size",				// 0x090 FSIZE
	"Chip ID no. Low Byte",				// 0x094 ID0
	"Chip ID no. High Byte",			// 0x098 ID1
	"Chip Version Number",				// 0x09c VERSION
	"Mouse X Position",					// 0x0a0 MOUSEX
	"Mouse Y Position",					// 0x0a4 MOUSEY
	"Mouse Data",						// 0x0a8 MSEDAT
	"Mouse Control",					// 0x0ac MSECR
	"<RESERVED>",						// 0x0b0
	"<RESERVED>",						// 0x0b4
	"<RESERVED>",						// 0x0b8
	"<RESERVED>",						// 0x0bc
	"DACK Timing Control",				// 0x0c0 DMATCR
	"I/O Timing Control",				// 0x0c4 IOTCR
	"Expansion Card Timing",			// 0x0c8 ECTCR
	"DMA External Control",				// 0x0cc DMAEXT (IOMD) / ASTCR (ARM7500)
	"DRAM Width Control",				// 0x0d0 DRAMWID
	"Force CAS/RAS Lines Low",			// 0x0d4 SELFREF
	"<RESERVED>",						// 0x0d8
	"<RESERVED>",						// 0x0dc
	"A to D IRQ Control",				// 0x0e0 ATODICR
	"A to D IRQ Status",				// 0x0e4 ATODCC
	"A to D IRQ Converter Control",		// 0x0e8 ATODICR
	"A to D IRQ Counter 1",				// 0x0ec ATODCNT1
	"A to D IRQ Counter 2",				// 0x0f0 ATODCNT2
	"A to D IRQ Counter 3",				// 0x0f4 ATODCNT3
	"A to D IRQ Counter 4",				// 0x0f8 ATODCNT4
	"<RESERVED>",						// 0x0fc
	"I/O DMA 0 CurA",					// 0x100 IO0CURA
	"I/O DMA 0 EndA",					// 0x104 IO0ENDA
	"I/O DMA 0 CurB",					// 0x108 IO0CURB
	"I/O DMA 0 EndB",					// 0x10c IO0ENDB
	"I/O DMA 0 Control",				// 0x110 IO0CR
	"I/O DMA 0 Status",					// 0x114 IO0ST
	"<RESERVED>",						// 0x118
	"<RESERVED>",						// 0x11c
	"I/O DMA 1 CurA",					// 0x120 IO1CURA
	"I/O DMA 1 EndA",					// 0x124 IO1ENDA
	"I/O DMA 1 CurB",					// 0x128 IO1CURB
	"I/O DMA 1 EndB",					// 0x12c IO1ENDB
	"I/O DMA 1 Control",				// 0x130 IO1CR
	"I/O DMA 1 Status",					// 0x134 IO1ST
	"<RESERVED>",						// 0x138
	"<RESERVED>",						// 0x13c
	"I/O DMA 2 CurA",					// 0x140 IO2CURA
	"I/O DMA 2 EndA",					// 0x144 IO2ENDA
	"I/O DMA 2 CurB",					// 0x148 IO2CURB
	"I/O DMA 2 EndB",					// 0x14c IO2ENDB
	"I/O DMA 2 Control",				// 0x150 IO2CR
	"I/O DMA 2 Status",					// 0x154 IO2ST
	"<RESERVED>",						// 0x158
	"<RESERVED>",						// 0x15c
	"I/O DMA 3 CurA",					// 0x160 IO3CURA
	"I/O DMA 3 EndA",					// 0x164 IO3ENDA
	"I/O DMA 3 CurB",					// 0x168 IO3CURB
	"I/O DMA 3 EndB",					// 0x16c IO3ENDB
	"I/O DMA 3 Control",				// 0x170 IO3CR
	"I/O DMA 3 Status",					// 0x174 IO3ST
	"<RESERVED>",						// 0x178
	"<RESERVED>",						// 0x17c
	"Sound DMA 0 CurA",					// 0x180 SD0CURA
	"Sound DMA 0 EndA",					// 0x184 SD0ENDA
	"Sound DMA 0 CurB",					// 0x188 SD0CURB
	"Sound DMA 0 EndB",					// 0x18c SD0ENDB
	"Sound DMA 0 Control",				// 0x190 SD0CR
	"Sound DMA 0 Status",				// 0x194 SD0ST
	"<RESERVED>",						// 0x198
	"<RESERVED>",						// 0x19c
	"Sound DMA 1 CurA",					// 0x1a0 SD1CURA
	"Sound DMA 1 EndA",					// 0x1a4 SD1ENDA
	"Sound DMA 1 CurB",					// 0x1a8 SD1CURB
	"Sound DMA 1 EndB",					// 0x1ac SD1ENDB
	"Sound DMA 1 Control",				// 0x1b0 SD1CR
	"Sound DMA 1 Status",				// 0x1b4 SD1ST
	"<RESERVED>",						// 0x1b8
	"<RESERVED>",						// 0x1bc
	"Cursor DMA Current",				// 0x1c0 CURSCUR
	"Cursor DMA Init",					// 0x1c4 CURSINIT
	"Duplex LCD Current B",				// 0x1c8 VIDCURB
	"<RESERVED>",						// 0x1cc
	"Video DMA Current",				// 0x1d0 VIDCUR
	"Video DMA End",					// 0x1d4 VIDEND
	"Video DMA Start",					// 0x1d8 VIDSTART
	"Video DMA Init",					// 0x1dc VIDINIT
	"Video DMA Control",				// 0x1e0 VIDCR
	"<RESERVED>",						// 0x1e4
	"Duplex LCD Init B",				// 0x1e8 VIDINITB
	"<RESERVED>",						// 0x1ec
	"DMA IRQ Status",					// 0x1f0 DMAST
	"DMA IRQ Request",					// 0x1f4 DMARQ
	"DMA IRQ Mask",						// 0x1f8 DMAMSK
	"<RESERVED>"						// 0x1fc
};

#define IOMD_IOCR 		0x000/4
#define IOMD_KBDDAT		0x004/4

#define IOMD_ID0 		0x094/4
#define IOMD_ID1 		0x098/4

static const char *const vidc20_regnames[] =
{
	"Video Palette", 					// 0
	"Video Palette Address", 			// 1
	"RESERVED", 						// 2
	"LCD offset", 						// 3
	"Border Colour",					// 4
	"Cursor Palette Logical Colour 1", 	// 5
	"Cursor Palette Logical Colour 2", 	// 6
	"Cursor Palette Logical Colour 3", 	// 7
	"Horizontal",						// 8
	"Vertical",							// 9
	"Stereo Image",						// A
	"Sound",							// B
	"External",							// C
	"Frequency Synthesis",				// D
	"Control",							// E
	"Data Control"						// F
};

static READ32_HANDLER( a7000_iomd_r )
{
	switch(offset)
	{
		case IOMD_ID0: return 0x000000e7; // IOMD ID low
		case IOMD_ID1: return 0x000000d4; // IOMD ID high
		default: 	logerror("IOMD: %s Register (%04x) read\n",iomd_regnames[offset & (0x1ff >> 2)],offset*4); break;
	}

	return 0;
}

static WRITE32_HANDLER( a7000_iomd_w )
{
	logerror("IOMD: %s Register (%04x) write = %08x\n",iomd_regnames[offset & (0x1ff >> 2)],offset*4,data);
}


static WRITE32_HANDLER( a7000_vidc20_w )
{
	int reg = data >> 28;
	int val = data & 0xffffff;

	switch(reg)
	{
		default: logerror("VIDC20: %s Register write = %08x\n",vidc20_regnames[reg],val);
	}
}

static ADDRESS_MAP_START( a7000_mem, ADDRESS_SPACE_PROGRAM, 32)
	AM_RANGE(0x00000000, 0x007fffff) AM_MIRROR(0x00800000) AM_ROM AM_REGION("user1", 0x0)
	AM_RANGE(0x01000000, 0x01ffffff) AM_NOP //expansion ROM
	AM_RANGE(0x02000000, 0x02ffffff) AM_RAM //VRAM
//	I/O 03000000 - 033fffff
	AM_RANGE(0x03000000, 0x030001ff) AM_MIRROR(0x00200000) AM_READWRITE(a7000_iomd_r,a7000_iomd_w) //IOMD Registers
//	AM_RANGE(0x03010000, 0x03011fff) //Super IO
//	AM_RANGE(0x03012000, 0x03029fff) //FDC
//	AM_RANGE(0x0302b000, 0x0302bfff) //Network podule
//	AM_RANGE(0x03040000, 0x0304ffff) //podule space 0,1,2,3
//	AM_RANGE(0x03070000, 0x0307ffff) //podule space 4,5,6,7
//	AM_RANGE(0x03310000, 0x03310003) //Mouse Buttons
	AM_RANGE(0x03400000, 0x037fffff) AM_WRITE(a7000_vidc20_w)
//	AM_RANGE(0x08000000, 0x08ffffff) AM_MIRROR(0x07000000) //EASI space
	AM_RANGE(0x10000000, 0x10ffffff) AM_MIRROR(0x03000000) AM_RAM //SIMM 0 bank 0
	AM_RANGE(0x14000000, 0x14ffffff) AM_MIRROR(0x03000000) AM_RAM //SIMM 0 bank 1
//	AM_RANGE(0x18000000, 0x18ffffff) AM_MIRROR(0x03000000) AM_RAM //SIMM 1 bank 0
//	AM_RANGE(0x1c000000, 0x1cffffff) AM_MIRROR(0x03000000) AM_RAM //SIMM 1 bank 1
ADDRESS_MAP_END


/* Input ports */
static INPUT_PORTS_START( a7000 )
INPUT_PORTS_END


static MACHINE_RESET(a7000)
{
}


static MACHINE_DRIVER_START( a7000 )

	/* Basic machine hardware */
	MDRV_CPU_ADD( "maincpu", ARM7, XTAL_32MHz )
	MDRV_CPU_PROGRAM_MAP( a7000_mem)

	MDRV_MACHINE_RESET( a7000 )


    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(a7000)
    MDRV_VIDEO_UPDATE(a7000)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( a7000p )
	MDRV_IMPORT_FROM( a7000 )

	MDRV_CPU_MODIFY("maincpu")
	MDRV_CPU_CLOCK(XTAL_48MHz)
MACHINE_DRIVER_END

ROM_START(a7000)
	ROM_REGION( 0x800000, "user1", ROMREGION_ERASEFF )
	ROM_LOAD( "rom1.bin", 0x000000, 0x100000, CRC(ff0e3d12) SHA1(fa489bebede3d13dc43cddec5b5c9b6829a28914) )
	ROM_LOAD( "rom2.bin", 0x100000, 0x100000, CRC(4ae4fd8b) SHA1(1b30d5905d5364dfa48bad69257b0ef8190e9bf6) )
	ROM_LOAD( "rom3.bin", 0x200000, 0x100000, CRC(3108fb2b) SHA1(865b01583f3fb5f4ed5e9201676db327cdeb40b3) )
	ROM_LOAD( "rom4.bin", 0x300000, 0x100000, CRC(55a51980) SHA1(a7191727edd5babf679ebbdea6585833a1fb34e6) )
ROM_END

ROM_START(a7000p)
	ROM_REGION( 0x800000, "user1", ROMREGION_ERASEFF )
	ROM_LOAD( "riscos-3.71.rom", 0x000000, 0x400000, CRC(211cf888) SHA1(c5fe0645e48894fb4b245abeefdc9a65d659af59))
ROM_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

/*    YEAR  NAME        PARENT  COMPAT  MACHINE     INPUT   INIT    COMPANY FULLNAME        FLAGS */
COMP( 1995, a7000,      0,      0,      a7000,      a7000,	0,      "Acorn",  "Archimedes A7000",   GAME_NOT_WORKING | GAME_NO_SOUND )
COMP( 1997, a7000p,     a7000,  0,      a7000,      a7000,	0,      "Acorn",  "Archimedes A7000+",  GAME_NOT_WORKING | GAME_NO_SOUND )

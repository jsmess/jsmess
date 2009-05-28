/***********************************************************************************************

	Fujitsu Micro 7 (FM-7)

	12/05/2009 Skeleton driver.
	
	Computers in this series:
	
                 | Release |    Main CPU    |  Sub CPU  |              RAM              |
    =====================================================================================
    FM-8         | 1981-05 | M68A09 @ 1MHz  |  M6809    |    64K (main) + 48K (VRAM)    |
    FM-7         | 1982-11 | M68B09 @ 2MHz  |  M68B09   |    64K (main) + 48K (VRAM)    |
    FM-NEW7      | 1984-05 | M68B09 @ 2MHz  |  M68B09   |    64K (main) + 48K (VRAM)    |
    FM-77        | 1984-05 | M68B09 @ 2MHz  |  M68B09E  |  64/256K (main) + 48K (VRAM)  |
    FM-77AV      | 1985-10 | M68B09E @ 2MHz |  M68B09   | 128/192K (main) + 96K (VRAM)  |
    FM-77AV20    | 1986-10 | M68B09E @ 2MHz |  M68B09   | 128/192K (main) + 96K (VRAM)  |
    FM-77AV40    | 1986-10 | M68B09E @ 2MHz |  M68B09   | 192/448K (main) + 144K (VRAM) |
    FM-77AV20EX  | 1987-11 | M68B09E @ 2MHz |  M68B09   | 128/192K (main) + 96K (VRAM)  |
    FM-77AV40EX  | 1987-11 | M68B09E @ 2MHz |  M68B09   | 192/448K (main) + 144K (VRAM) |
    FM-77AV40SX  | 1988-11 | M68B09E @ 2MHz |  M68B09   | 192/448K (main) + 144K (VRAM) |

	Note: FM-77AV dumps probably come from a FM-77AV40SX. Shall we confirm that both computers
	used the same BIOS components?

	memory map info from http://www.nausicaa.net/~lgreenf/fm7page.htm
	see also http://retropc.net/ryu/xm7/xm7.shtml

************************************************************************************************/

#include "driver.h"
#include "cpu/m6809/m6809.h"
#include "sound/ay8910.h"

// Interrupt flags
#define IRQ_FLAG_KEY      0x01
#define IRQ_FLAG_PRINTER  0x02
#define IRQ_FLAG_TIMER    0x04
#define IRQ_FLAG_UNKNOWN  0x08
// the following are not read in port 0xfd03
#define IRQ_FLAG_MFD      0x10
#define IRQ_FLAG_TXRDY    0x20
#define IRQ_FLAG_RXRDY    0x40
#define IRQ_FLAG_SYNDET   0x80

UINT8* shared_ram;
UINT8* fm7_video_ram;
static UINT8 irq_flags;  // active IRQ flags
static UINT8 irq_mask;  // IRQ mask 
emu_timer* fm7_timer;  // main timer, triggered every 2.0345ms?
static UINT8 sub_busy;
static UINT8 basic_rom_en;
static UINT8 fm7_attn_irq;

/*
 * I/O port 0xfd02
 * 
 * On read: returns cassette data (bit 7) and printer status (bits 0-5)
 * On write: sets IRQ masks 
 *   bit 0 - keypress
 *   bit 1 - printer
 *   bit 2 - timer
 *   bit 3 - not used
 *   bit 4 - MFD
 *   bit 5 - TXRDY
 *   bit 6 - RXRDY
 *   bit 7 - SYNDET
 * 
 */
WRITE8_HANDLER( fm7_irq_mask_w )
{
	irq_mask = data;
	logerror("IRQ mask set: 0x%02x\n",irq_mask);
}

/*
 * I/O port 0xfd03
 * 
 * On read: returns which IRQ is currently active (typically read by IRQ handler)
 *   bit 0 - keypress
 *   bit 1 - printer
 *   bit 2 - timer
 *   bit 3 - ???
 * On write: Buzzer/Speaker On/Off
 *   bit 0 - speaker on/off
 *   bit 1 - ??buzzer on/off
 *   bit 2 - ??buzzer on/off
 */
READ8_HANDLER( fm7_irq_cause_r )
{
	UINT8 ret = irq_flags;
	
	irq_flags = 0;  // clear flags
	logerror("IRQ flags read: 0x%02x\n",ret);
	return ret;
}

READ8_HANDLER( mainmem_r )
{
	UINT8* RAM = memory_region(space->machine,"maincpu");

	return RAM[offset];
}

WRITE8_HANDLER( mainmem_w )
{
	UINT8* RAM = memory_region(space->machine,"maincpu");

	RAM[offset] = data;
}

READ8_HANDLER( vector_r )
{
	UINT8* RAM = memory_region(space->machine,"maincpu");

	return RAM[0xfff0+offset];
}

WRITE8_HANDLER( vector_w )
{
	UINT8* RAM = memory_region(space->machine,"maincpu");
	
	RAM[0xfff0+offset] = data;
}

READ8_HANDLER( shared_r )
{
	return shared_ram[offset];
}

WRITE8_HANDLER( shared_w )
{
	shared_ram[offset] = data;
}

READ8_HANDLER( fm7_fd04_r )
{
	UINT8 ret = 0xff;
	
	if(fm7_attn_irq != 0)
	{
		ret &= ~0x01;
		fm7_attn_irq = 0;
	}
	return ret;
}
/*
 * Sub-CPU interface (port 0xfd05)
 * 
 * Read:
 *   bit 7: Sub-CPU busy
 *   bit 0: EXTDET (?)
 * Write:
 *   bit 7: Sub-CPU halt
 *   bit 6: Sub-CPU cancel IRQ
 */

READ8_HANDLER( fm7_subintf_r )
{
	return sub_busy & 0x80;
}

WRITE8_HANDLER( fm7_subintf_w )
{
	cputag_set_input_line(space->machine,"sub",INPUT_LINE_HALT,(data & 0x80) ? ASSERT_LINE : CLEAR_LINE);
	if(data & 0x40)
		cputag_set_input_line(space->machine,"sub",M6809_IRQ_LINE,ASSERT_LINE);
}

READ8_HANDLER( fm7_sub_busyflag_r )
{
	sub_busy = 0x00;
	return 0x00;
}

WRITE8_HANDLER( fm7_sub_busyflag_w )
{
	sub_busy = 0x80;
}

READ8_HANDLER( fm7_rom_en_r )
{
	UINT8* RAM = memory_region(space->machine,"maincpu");
	
	basic_rom_en = 1;
	memory_set_bankptr(space->machine,1,RAM+0x38000);
	logerror("BASIC ROM enabled\n");
	return 0x00;
}

WRITE8_HANDLER( fm7_rom_en_w )
{
	UINT8* RAM = memory_region(space->machine,"maincpu");
	
	basic_rom_en = 0;
	memory_set_bankptr(space->machine,1,RAM+0x8000);
	logerror("BASIC ROM disabled\n");
}

/*
 * Sub-CPU port 0xd402
 *   Read-only: Acknowledge Cancel IRQ
 */
READ8_HANDLER( fm7_cancel_ack )
{
	cputag_set_input_line(space->machine,"sub",M6809_IRQ_LINE,CLEAR_LINE);
	return 0x00;
}

/*
 * Reading from 0xd404 (sub-CPU) causes an "Attention" FIRQ on the main CPU 
 */
READ8_HANDLER( fm7_attn_irq_r )
{
	fm7_attn_irq = 1;
	cputag_set_input_line(space->machine,"maincpu",M6809_FIRQ_LINE,ASSERT_LINE);
	return 0xff;
}

static TIMER_CALLBACK( fm7_timer_irq )
{
	if(~irq_mask & IRQ_FLAG_TIMER)
	{
		irq_flags |= IRQ_FLAG_TIMER;
		cputag_set_input_line(machine,"maincpu",M6809_IRQ_LINE,ASSERT_LINE);
	}
}

static IRQ_CALLBACK(fm7_irq_ack)
{
	cputag_set_input_line(device->machine,"maincpu",irqline,CLEAR_LINE);
	return -1;
}

static IRQ_CALLBACK(fm7_sub_irq_ack)
{
	cputag_set_input_line(device->machine,"sub",irqline,CLEAR_LINE);
	return -1;
}

/*
   0000 - 7FFF: (RAM) BASIC working area, user's area
   8000 - FBFF: (ROM) F-BASIC ROM
   FC00 - FC7F: Shared RAM between main and sub CPU
   FD00 - FDFF: I/O space (6809 uses memory-mapped I/O)
   FE00 - FFEF: Boot rom
   FFF0 - FFFF: Interrupt vector table
*/
// The FM-7 has only 64kB RAM, so we'll worry about banking when we do the later models
static ADDRESS_MAP_START( fm7_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000,0x7fff) AM_READWRITE(mainmem_r,mainmem_w)
	AM_RANGE(0x8000,0xfbff) AM_RAMBANK(1) // also F-BASIC ROM, when enabled
	AM_RANGE(0xfc00,0xfc7f) AM_RAM
	AM_RANGE(0xfc80,0xfcff) AM_READWRITE(shared_r,shared_w) // shared RAM with sub-CPU
	// I/O space (FD00-FDFF)
	AM_RANGE(0xfd02,0xfd02) AM_WRITE(fm7_irq_mask_w)  // IRQ mask
	AM_RANGE(0xfd03,0xfd03) AM_READ(fm7_irq_cause_r)  // IRQ flags
	AM_RANGE(0xfd04,0xfd04) AM_READ(fm7_fd04_r)
	AM_RANGE(0xfd05,0xfd05) AM_READWRITE(fm7_subintf_r,fm7_subintf_w)
	AM_RANGE(0xfd0d,0xfd0d) AM_DEVWRITE("psg",ay8910_address_w)
	AM_RANGE(0xfd0e,0xfd0e) AM_DEVREADWRITE("psg",ay8910_r,ay8910_data_w)
	AM_RANGE(0xfd0f,0xfd0f) AM_READWRITE(fm7_rom_en_r,fm7_rom_en_w)
	// Boot ROM
	AM_RANGE(0xfe00,0xffef) AM_ROM AM_REGION("basic",0x0000)
	AM_RANGE(0xfff0,0xffff) AM_READWRITE(vector_r,vector_w) 
ADDRESS_MAP_END

/*
   0000 - 3FFF: Video RAM bank 0 (Blue plane)
   4000 - 7FFF: Video RAM bank 1 (Red plane)
   8000 - BFFF: Video RAM bank 2 (Green plane)
   C000 - C2FF: (RAM) working area
   C300 - C37F: Shared RAM between main and sub CPU
   C400 - FFDF: (ROM) Graphics command code
   FFF0 - FFFF: Interrupt vector table
*/

static ADDRESS_MAP_START( fm7_sub_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000,0xbfff) AM_RAM AM_BASE(&fm7_video_ram) // VRAM
	AM_RANGE(0xc000,0xcfff) AM_RAM // Console RAM
	AM_RANGE(0xd000,0xd37f) AM_RAM // Work RAM
	AM_RANGE(0xd380,0xd3ff) AM_READWRITE(shared_r,shared_w) // shared RAM
	// I/O space (D400-D7FF)
	AM_RANGE(0xd402,0xd402) AM_READ(fm7_cancel_ack)
	AM_RANGE(0xd404,0xd404) AM_READ(fm7_attn_irq_r)
	AM_RANGE(0xd40a,0xd40a) AM_READWRITE(fm7_sub_busyflag_r,fm7_sub_busyflag_w)
	AM_RANGE(0xd800,0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( fm77av_mem, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

static ADDRESS_MAP_START( fm77av_sub_mem, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000,0xbfff) AM_RAM AM_BASE(&fm7_video_ram) // VRAM
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( fm7 )
INPUT_PORTS_END

static DRIVER_INIT(fm7)
{
	shared_ram = auto_alloc_array(machine,UINT8,0x80);
	fm7_timer = timer_alloc(machine,fm7_timer_irq,NULL);
	cpu_set_irq_callback(cputag_get_cpu(machine,"maincpu"),fm7_irq_ack);
	cpu_set_irq_callback(cputag_get_cpu(machine,"sub"),fm7_sub_irq_ack);
}

static MACHINE_START(fm7)
{
	// The FM-7 has no initialisation ROM, and no other obvious
	// way to set the reset vector, so for now this will have to do.
	UINT8* RAM = memory_region(machine,"maincpu");
	
	RAM[0xfffe] = 0xfe;
	RAM[0xffff] = 0x00; 
	
	memset(shared_ram,0xff,0x80);
}

static MACHINE_RESET(fm7)
{
	UINT8* RAM = memory_region(machine,"maincpu");
	
	timer_adjust_periodic(fm7_timer,attotime_zero,0,ATTOTIME_IN_USEC(2034));
	irq_mask = 0xff;
	irq_flags = 0x00;
	fm7_attn_irq = 0;
	sub_busy = 0x80;  // busy at reset
	basic_rom_en = 1;  // enabled at reset
	memory_set_bankptr(machine,1,RAM+0x38000);
}

static VIDEO_START( fm7 )
{
}

static VIDEO_UPDATE( fm7 )
{
    UINT8 code_r,code_g,code_b;
    UINT8 col;
    int y, x, b;

    for (y = 0; y < 200; y++)
    {
	    for (x = 0; x < 80; x++)
	    {
            code_r = fm7_video_ram[0x4000 + y*80 + x];
            code_g = fm7_video_ram[0x8000 + y*80 + x];
            code_b = fm7_video_ram[0x0000 + y*80 + x];
            for (b = 0; b < 8; b++)
            {
                col = (((code_r >> b) & 0x01) ? 4 : 0) + (((code_g >> b) & 0x01) ? 2 : 0) + (((code_b >> b) & 0x01) ? 1 : 0);
                *BITMAP_ADDR16(bitmap, y,  x*8+(7-b)) =  col;
            }
        }
    }   
	return 0;
}

static const rgb_t fm7_palette[8] = {
	MAKE_RGB(0x00, 0x00, 0x00), // 0
	MAKE_RGB(0x00, 0x00, 0x80), // 1
	MAKE_RGB(0x00, 0x80, 0x00), // 2
	MAKE_RGB(0x00, 0x80, 0x80), // 3
	MAKE_RGB(0x80, 0x00, 0x00), // 4
	MAKE_RGB(0x80, 0x00, 0x80), // 5
	MAKE_RGB(0x80, 0x80, 0x00), // 6
	MAKE_RGB(0x80, 0x80, 0x80), // 7
};

PALETTE_INIT( fm7 )
{
	palette_set_colors(machine, 0, fm7_palette, ARRAY_LENGTH(fm7_palette));
}


static const ay8910_interface fm7_psg_intf =
{
	AY8910_LEGACY_OUTPUT,
	AY8910_DEFAULT_LOADS,
	DEVCB_NULL,	/* portA read */
	DEVCB_NULL,	/* portB read */
	DEVCB_NULL,					/* portA write */
	DEVCB_NULL					/* portB write */
};

static MACHINE_DRIVER_START( fm7 )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", M6809, XTAL_2MHz)
	MDRV_CPU_PROGRAM_MAP(fm7_mem)

	MDRV_CPU_ADD("sub", M6809, XTAL_2MHz)
	MDRV_CPU_PROGRAM_MAP(fm7_sub_mem)
	
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("psg", AY8910, 1000000)  // clock speed unknown
	MDRV_SOUND_CONFIG(fm7_psg_intf)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS,"mono",1.0)

	MDRV_MACHINE_START(fm7)
	MDRV_MACHINE_RESET(fm7)

	/* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(640, 200)
	MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 200-1)
	MDRV_PALETTE_LENGTH(8)
	MDRV_PALETTE_INIT(fm7)

	MDRV_VIDEO_START(fm7)
	MDRV_VIDEO_UPDATE(fm7)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( fm77av )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", M6809E, XTAL_2MHz)
	MDRV_CPU_PROGRAM_MAP(fm77av_mem)

	MDRV_CPU_ADD("sub", M6809, XTAL_2MHz)
	MDRV_CPU_PROGRAM_MAP(fm77av_sub_mem)

	MDRV_MACHINE_RESET(fm7)

	/* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(640, 200)
	MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 200-1)
	MDRV_PALETTE_LENGTH(8)
	MDRV_PALETTE_INIT(fm7)

	MDRV_VIDEO_START(fm7)
	MDRV_VIDEO_UPDATE(fm7)
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( fm7 )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD( "fbasic30.rom", 0x38000,  0x7c00, CRC(a96d19b6) SHA1(8d5f0cfe7e0d39bf2ab7e4c798a13004769c28b2) )

	ROM_REGION( 0x20000, "sub", 0 )
	ROM_LOAD( "subsys_c.rom", 0xd800,  0x2800, CRC(24cec93f) SHA1(50b7283db6fe1342c6063fc94046283f4feddc1c) )

	// either one of these boot ROMs are selectable via DIP switch
	ROM_REGION( 0x200, "basic", 0 )
	ROM_LOAD( "boot_bas.rom", 0x0000,  0x0200, CRC(c70f0c74) SHA1(53b63a301cba7e3030e79c59a4d4291eab6e64b0) )

	ROM_REGION( 0x200, "dos", 0 )	
	ROM_LOAD( "boot_dos.rom", 0x0000,  0x0200, CRC(198614ff) SHA1(037e5881bd3fed472a210ee894a6446965a8d2ef) )

	// optional Kanji rom?
ROM_END

ROM_START( fm77av )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD( "initiate.rom", 0x36000,  0x2000, CRC(785cb06c) SHA1(b65987e98a9564a82c85eadb86f0204eee5a5c93) )
	ROM_LOAD( "fbasic30.rom", 0x38000,  0x7c00, CRC(a96d19b6) SHA1(8d5f0cfe7e0d39bf2ab7e4c798a13004769c28b2) )

	ROM_REGION( 0x22000, "sub", 0 )
	ROM_SYSTEM_BIOS(0, "mona", "Monitor Type A" )
	ROMX_LOAD( "subsys_a.rom", 0x1e000,  0x2000, CRC(e8014fbb) SHA1(038cb0b42aee9e933b20fccd6f19942e2f476c83), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS(1, "monb", "Monitor Type B" )
	ROMX_LOAD( "subsys_b.rom", 0x1e000,  0x2000, CRC(9be69fac) SHA1(0305bdd44e7d9b7b6a17675aff0a3330a08d21a8), ROM_BIOS(2) )
	/* 4 0x800 banks to be banked at 1d800 */
	ROM_LOAD( "subsyscg.rom", 0x20000,  0x2000, CRC(e9f16c42) SHA1(8ab466b1546d023ba54987790a79e9815d2b7bb2) )

	ROM_REGION( 0x20000, "gfx1", 0 )
	ROM_LOAD( "kanji.rom", 0x0000, 0x20000, CRC(62402ac9) SHA1(bf52d22b119d54410dad4949b0687bb0edf3e143) )

	// optional dict rom?
ROM_END

ROM_START( fm7740sx )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD( "initiate.rom", 0x36000,  0x2000, CRC(785cb06c) SHA1(b65987e98a9564a82c85eadb86f0204eee5a5c93) )
	ROM_LOAD( "fbasic30.rom", 0x38000,  0x7c00, CRC(a96d19b6) SHA1(8d5f0cfe7e0d39bf2ab7e4c798a13004769c28b2) )

	ROM_REGION( 0x22000, "sub", 0 )
	ROM_SYSTEM_BIOS(0, "mona", "Monitor Type A" )
	ROMX_LOAD( "subsys_a.rom", 0x1e000,  0x2000, CRC(e8014fbb) SHA1(038cb0b42aee9e933b20fccd6f19942e2f476c83), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS(1, "monb", "Monitor Type B" )
	ROMX_LOAD( "subsys_b.rom", 0x1e000,  0x2000, CRC(9be69fac) SHA1(0305bdd44e7d9b7b6a17675aff0a3330a08d21a8), ROM_BIOS(2) )
	/* 4 0x800 banks to be banked at 1d800 */
	ROM_LOAD( "subsyscg.rom", 0x20000,  0x2000, CRC(e9f16c42) SHA1(8ab466b1546d023ba54987790a79e9815d2b7bb2) )

	ROM_REGION( 0x20000, "gfx1", 0 )
	ROM_LOAD( "kanji.rom",  0x0000, 0x20000, CRC(62402ac9) SHA1(bf52d22b119d54410dad4949b0687bb0edf3e143) )
	ROM_LOAD( "kanji2.rom", 0x0000, 0x20000, CRC(38644251) SHA1(ebfdc43c38e1380709ed08575c346b2467ad1592) )

	/* These should be loaded at 2e000-2ffff of maincpu, but I'm not sure if it is correct */
	ROM_REGION( 0x4c000, "additional", 0 )
	ROM_LOAD( "dicrom.rom", 0x00000, 0x40000, CRC(b142acbc) SHA1(fe9f92a8a2750bcba0a1d2895e75e83858e4f97f) )
	ROM_LOAD( "extsub.rom", 0x40000, 0x0c000, CRC(0f7fcce3) SHA1(a1304457eeb400b4edd3c20af948d66a04df255e) )
ROM_END


static SYSTEM_CONFIG_START(fm7)
SYSTEM_CONFIG_END


/* Driver */

/*    YEAR  NAME      PARENT  COMPAT  MACHINE  INPUT   INIT  CONFIG  COMPANY      FULLNAME        FLAGS */
COMP( 1982, fm7,      0,      0,      fm7,     fm7,    fm7,  fm7,    "Fujitsu",   "FM-7",         GAME_NOT_WORKING)
COMP( 1985, fm77av,   fm7,    0,      fm77av,  fm7,    0,    fm7,    "Fujitsu",   "FM-77AV",      GAME_NOT_WORKING)
COMP( 1985, fm7740sx, fm7,    0,      fm77av,  fm7,    0,    fm7,    "Fujitsu",   "FM-77AV40SX",  GAME_NOT_WORKING)

/***************************************************************************

    Epson PX-4

	Note: We are missing a dump of the slave 7508 CPU that controls
	the keyboard and some other things.

***************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "devices/cartslot.h"
#include "px4.lh"


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _px4_state px4_state;
struct _px4_state
{
	UINT8 cmdr;
	UINT8 bankr;

	/* lcd screen */
	UINT8 vadr;
	UINT8 yoff;
};


/***************************************************************************
    GAPNIT
***************************************************************************/

/* input capture register low command trigger */
static READ8_HANDLER( px4_icrlc_r )
{
	logerror("%s: px4_icrlc_r\n", cpuexec_describe_context(space->machine));
	return 0xff;
}

/* control register 1 */
static WRITE8_HANDLER( px4_ctrl1_w )
{
	logerror("%s: px4_ctrl1_w (0x%02x)\n", cpuexec_describe_context(space->machine), data);
}

/* input capture register high command trigger */
static READ8_HANDLER( px4_icrhc_r )
{
	logerror("%s: px4_icrhc_r\n", cpuexec_describe_context(space->machine));
	return 0xff;
}

/* command register */
static WRITE8_HANDLER( px4_cmdr_w )
{
	px4_state *px4 = space->machine->driver_data;
	logerror("%s: px4_cmdr_w (0x%02x)\n", cpuexec_describe_context(space->machine), data);

	px4->cmdr = data;
}

/* barcode trigger */
static READ8_HANDLER( px4_icrlb_r )
{
	logerror("%s: px4_icrlb_r\n", cpuexec_describe_context(space->machine));
	return 0xff;
}

/* control register 2 */
static WRITE8_HANDLER( px4_ctrl2_w )
{
	logerror("%s: px4_ctrl2_w (0x%02x)\n", cpuexec_describe_context(space->machine), data);
}

/* barcode trigger */
static READ8_HANDLER( px4_icrhb_r )
{
	logerror("%s: px4_icrhb_r\n", cpuexec_describe_context(space->machine));
	return 0xff;
}

/* interrupt status register */
static READ8_HANDLER( px4_isr_r )
{
	logerror("%s: px4_isr_r\n", cpuexec_describe_context(space->machine));
	return 0xff;
}

/* interrupt enable register */
static WRITE8_HANDLER( px4_ier_w )
{
	logerror("%s: px4_ier_w (0x%02x)\n", cpuexec_describe_context(space->machine), data);
}

/* status register */
static READ8_HANDLER( px4_str_r )
{
	px4_state *px4 = space->machine->driver_data;
	logerror("%s: px4_str_r\n", cpuexec_describe_context(space->machine));

	return (px4->bankr & 0xf0) | 0x0f;
}

/* helper function to map rom capsules */
static void install_rom_capsule(const address_space *space, int size, const char *region)
{
	/* ram, part 1 */
	memory_install_readwrite8_handler(space, 0x0000, 0xdfff - size, 0, 0, SMH_BANK(1), SMH_BANK(1));
	memory_set_bankptr(space->machine, 1, mess_ram);

	/* actual rom data */
	memory_install_readwrite8_handler(space, 0xe000 - size, 0xdfff, 0, 0, SMH_BANK(2), SMH_NOP);
	memory_set_bankptr(space->machine, 2, memory_region(space->machine, region));

	/* ram, continued */
	memory_install_readwrite8_handler(space, 0xc000, 0xffff, 0, 0, SMH_BANK(3), SMH_BANK(3));
	memory_set_bankptr(space->machine, 3, mess_ram + 0xc000);
}

/* bank register */
static WRITE8_HANDLER( px4_bankr_w )
{
	px4_state *px4 = space->machine->driver_data;
	const address_space *space_program = cpu_get_address_space(space->machine->cpu[0], ADDRESS_SPACE_PROGRAM);

	logerror("%s: px4_bankr_w (0x%02x)\n", cpuexec_describe_context(space->machine), data);

	px4->bankr = data;

	/* bank switch */
	switch (data >> 4)
	{
	case 0x00:
		/* system bank */
		memory_install_readwrite8_handler(space_program, 0x0000, 0x7fff, 0, 0, SMH_BANK(1), SMH_NOP);
		memory_set_bankptr(space->machine, 1, memory_region(space->machine, "os"));
		memory_install_readwrite8_handler(space_program, 0x8000, 0xffff, 0, 0, SMH_BANK(2), SMH_BANK(2));
		memory_set_bankptr(space->machine, 2, mess_ram + 0x8000);
		break;

	case 0x04:
		/* memory */
		memory_install_readwrite8_handler(space_program, 0x0000, 0xffff, 0, 0, SMH_BANK(1), SMH_BANK(1));
		memory_set_bankptr(space->machine, 1, mess_ram);
		break;

	case 0x08: install_rom_capsule(space_program, 0x2000, "capsule1"); break;
	case 0x09: install_rom_capsule(space_program, 0x4000, "capsule1"); break;
	case 0x0a: install_rom_capsule(space_program, 0x8000, "capsule1"); break;
	case 0x0c: install_rom_capsule(space_program, 0x2000, "capsule2"); break;
	case 0x0d: install_rom_capsule(space_program, 0x4000, "capsule2"); break;
	case 0x0e: install_rom_capsule(space_program, 0x8000, "capsule2"); break;

	default:
		logerror("invalid bank switch value: 0x%02x\n", data >> 4);

	}
}

/* serial io register */
static READ8_HANDLER( px4_sior_r )
{
	logerror("%s: px4_sior_r\n", cpuexec_describe_context(space->machine));
	return 0xff;
}

/* serial io register */
static WRITE8_HANDLER( px4_sior_w )
{
	logerror("%s: px4_sior_w (0x%02x)\n", cpuexec_describe_context(space->machine), data);

	switch (data)
	{
	case 0x01: logerror("7508 cmd: Power OFF\n"); break;
	case 0x02: logerror("7508 cmd: Read Status\n"); break;
	case 0x03: logerror("7508 cmd: KB Reset\n"); break;
	case 0x04: logerror("7508 cmd: KB Repeat Timer 1 Set\n"); break;
	case 0x14: logerror("7508 cmd: KB Repeat Timer 2 Set\n"); break;
	case 0x24: logerror("7508 cmd: KB Repeat Timer 1 Read\n"); break;
	case 0x34: logerror("7508 cmd: KB Repeat Timer 2 Read\n"); break;
	case 0x05: logerror("7508 cmd: KB Repeat OFF\n"); break;
	case 0x15: logerror("7508 cmd: KB Repeat ON\n"); break;
	case 0x06: logerror("7508 cmd: KB Interrupt OFF\n"); break;
	case 0x16: logerror("7508 cmd: KB Interrupt ON\n"); break;
	case 0x07: logerror("7508 cmd: Clock Read\n"); break;
	case 0x17: logerror("7508 cmd: Clock Write\n"); break;
	case 0x08: logerror("7508 cmd: Power Switch Read\n"); break;
	case 0x09: logerror("7508 cmd: Alarm Read\n"); break;
	case 0x19: logerror("7508 cmd: Alarm Set\n"); break;
	case 0x29: logerror("7508 cmd: Alarm OFF\n"); break;
	case 0x39: logerror("7508 cmd: Alarm ON\n"); break;
	case 0x0a: logerror("7508 cmd: DIP Switch Read\n"); break;
	case 0x0b: logerror("7508 cmd: Stop Key Interrupt disable\n"); break;
	case 0x1b: logerror("7508 cmd: Stop Key Interrupt enable\n"); break;
	case 0x0c: logerror("7508 cmd: 7 chr. Buffer\n"); break;
	case 0x1c: logerror("7508 cmd: 1 chr. Buffer\n"); break;
	case 0x0d: logerror("7508 cmd: 1 sec. Interrupt OFF\n"); break;
	case 0x1d: logerror("7508 cmd: 1 sec. Interrupt ON\n"); break;
	case 0x0e: logerror("7508 cmd: KB Clear\n"); break;
	case 0x0f: logerror("7508 cmd: System Reset\n"); break;
	}
}


/***************************************************************************
    GAPNDI
***************************************************************************/

/* vram start address register */
static WRITE8_HANDLER( px4_vadr_w )
{
	px4_state *px4 = space->machine->driver_data;
	logerror("%s: px4_vadr_w (0x%02x)\n", cpuexec_describe_context(space->machine), data);

	px4->vadr = data;
}

/* y offset register */
static WRITE8_HANDLER( px4_yoff_w )
{
	px4_state *px4 = space->machine->driver_data;
	logerror("%s: px4_yoff_w (0x%02x)\n", cpuexec_describe_context(space->machine), data);

	px4->yoff = data;
}

/* frame register */
static WRITE8_HANDLER( px4_fr_w )
{
	logerror("%s: px4_fr_w (0x%02x)\n", cpuexec_describe_context(space->machine), data);
}

/* speed-up register */
static WRITE8_HANDLER( px4_spur_w )
{
	logerror("%s: px4_spur_w (0x%02x)\n", cpuexec_describe_context(space->machine), data);
}


/***************************************************************************
    GAPNDL
***************************************************************************/

/* cartridge interface */
static READ8_HANDLER( px4_ctgif_r )
{
	logerror("%s: px4_ctgif_r @ 0x%02x\n", cpuexec_describe_context(space->machine), offset);
	return 0xff;
}

/* cartridge interface */
static WRITE8_HANDLER( px4_ctgif_w )
{
	logerror("%s: px4_ctgif_w (0x%02x @ 0x%02x)\n", cpuexec_describe_context(space->machine), data, offset);
}

/* art data input register */
static READ8_HANDLER( px4_artdir_r )
{
	logerror("%s: px4_artdir_r\n", cpuexec_describe_context(space->machine));
	return 0xff;
}

/* art data output register */
static WRITE8_HANDLER( px4_artdor_w )
{
	logerror("%s: px4_artdor_w (0x%02x)\n", cpuexec_describe_context(space->machine), data);
}

/* art status register */
static READ8_HANDLER( px4_artsr_r )
{
	logerror("%s: px4_artsr_r\n", cpuexec_describe_context(space->machine));
	return 0xff;
}

/* art mode register */
static WRITE8_HANDLER( px4_artmr_w )
{
	logerror("%s: px4_artmr_w (0x%02x)\n", cpuexec_describe_context(space->machine), data);
}

/* io status register */
static READ8_HANDLER( px4_iostr_r )
{
	logerror("%s: px4_iostr_r\n", cpuexec_describe_context(space->machine));
	return 0xff;
}

/* art command register */
static WRITE8_HANDLER( px4_artcr_w )
{
	logerror("%s: px4_artcr_w (0x%02x)\n", cpuexec_describe_context(space->machine), data);
}

/* printer data register */
static WRITE8_HANDLER( px4_pdr_w )
{
	logerror("%s: px4_pdr_w (0x%02x)\n", cpuexec_describe_context(space->machine), data);
}

/* switch register */
static WRITE8_HANDLER( px4_swr_w )
{
	logerror("%s: px4_swr_w (0x%02x)\n", cpuexec_describe_context(space->machine), data);
}

/* io control register */
static WRITE8_HANDLER( px4_ioctlr_w )
{
	logerror("%s: px4_ioctlr_w (0x%02x)\n", cpuexec_describe_context(space->machine), data);
}


/***************************************************************************
    VIDEO EMULATION
***************************************************************************/

/* TODO: y-offset wrap-around */
static VIDEO_UPDATE( px4 )
{
	px4_state *px4 = screen->machine->driver_data;

	/* display enabled? */
	if (BIT(px4->yoff, 7))
	{
		int y, x;

		/* get vram start address */
		UINT8 *vram = &mess_ram[(px4->vadr << 8) + (px4->yoff & 0x1f) * 32];

		for (y = 0; y < 64; y++)
		{
			for (x = 0; x < 240/8; x++)
			{
				*BITMAP_ADDR16(bitmap, y, x * 8 + 0) = BIT(vram[x + y * 32], 7);
				*BITMAP_ADDR16(bitmap, y, x * 8 + 1) = BIT(vram[x + y * 32], 6);
				*BITMAP_ADDR16(bitmap, y, x * 8 + 2) = BIT(vram[x + y * 32], 5);
				*BITMAP_ADDR16(bitmap, y, x * 8 + 3) = BIT(vram[x + y * 32], 4);
				*BITMAP_ADDR16(bitmap, y, x * 8 + 4) = BIT(vram[x + y * 32], 3);
				*BITMAP_ADDR16(bitmap, y, x * 8 + 5) = BIT(vram[x + y * 32], 2);
				*BITMAP_ADDR16(bitmap, y, x * 8 + 6) = BIT(vram[x + y * 32], 1);
				*BITMAP_ADDR16(bitmap, y, x * 8 + 7) = BIT(vram[x + y * 32], 0);
			}
		}
	}
	else
	{
		/* display is disabled, draw a black screen */
		bitmap_fill(bitmap, cliprect, 0);
	}

	return 0;
}


/***************************************************************************
    DRIVER INIT
***************************************************************************/

static DRIVER_INIT( px4 )
{
	/* map os rom and last half of memory */
	memory_set_bankptr(machine, 1, memory_region(machine, "os"));
	memory_set_bankptr(machine, 2, mess_ram + 0x8000);
}


/***************************************************************************
    ADDRESS MAPS
***************************************************************************/

static ADDRESS_MAP_START( px4_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROMBANK(1)
	AM_RANGE(0x8000, 0xffff) AM_RAMBANK(2)
ADDRESS_MAP_END

static ADDRESS_MAP_START( px4_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x00, 0x00) AM_READWRITE(px4_icrlc_r, px4_ctrl1_w)
	AM_RANGE(0x01, 0x01) AM_READWRITE(px4_icrhc_r, px4_cmdr_w)
	AM_RANGE(0x02, 0x02) AM_READWRITE(px4_icrlb_r, px4_ctrl2_w)
	AM_RANGE(0x03, 0x03) AM_READ(px4_icrhb_r)
	AM_RANGE(0x04, 0x04) AM_READWRITE(px4_isr_r, px4_ier_w)
	AM_RANGE(0x05, 0x05) AM_READWRITE(px4_str_r, px4_bankr_w)
	AM_RANGE(0x06, 0x06) AM_READWRITE(px4_sior_r, px4_sior_w)
	AM_RANGE(0x07, 0x07) AM_NOP
	AM_RANGE(0x08, 0x08) AM_WRITE(px4_vadr_w)
	AM_RANGE(0x09, 0x09) AM_WRITE(px4_yoff_w)
	AM_RANGE(0x0a, 0x0a) AM_WRITE(px4_fr_w)
	AM_RANGE(0x0b, 0x0b) AM_WRITE(px4_spur_w)
	AM_RANGE(0x0c, 0x0f) AM_NOP
	AM_RANGE(0x10, 0x13) AM_READWRITE(px4_ctgif_r, px4_ctgif_w)
	AM_RANGE(0x14, 0x14) AM_READWRITE(px4_artdir_r, px4_artdor_w)
	AM_RANGE(0x15, 0x15) AM_READWRITE(px4_artsr_r, px4_artmr_w)
	AM_RANGE(0x16, 0x16) AM_READWRITE(px4_iostr_r, px4_artcr_w)
	AM_RANGE(0x17, 0x17) AM_WRITE(px4_pdr_w)
	AM_RANGE(0x18, 0x18) AM_WRITE(px4_swr_w)
	AM_RANGE(0x19, 0x19) AM_WRITE(px4_ioctlr_w)
	AM_RANGE(0x1a, 0x1f) AM_NOP
	AM_RANGE(0x20, 0xff) AM_NOP /* external i/o */
ADDRESS_MAP_END


/***************************************************************************
    INPUT PORTS
***************************************************************************/

/* The PX-4 has an exchangable keyboard. Available is a standard ASCII
 * keyboard and an "item" keyboard, as well as regional variants for
 * UK, France, Germany, Denmark, Sweden, Norway, Italy and Spain.
 */
static INPUT_PORTS_START( px4_ascii )
INPUT_PORTS_END


/***************************************************************************
    MACHINE DRIVERS
***************************************************************************/

static MACHINE_DRIVER_START( px4 )
	/* basic machine hardware */
	MDRV_CPU_ADD("main", Z80, XTAL_3_6864MHz)	/* uPD70008 */
	MDRV_CPU_PROGRAM_MAP(px4_mem, 0)
	MDRV_CPU_IO_MAP(px4_io, 0)

	MDRV_DRIVER_DATA(px4_state)

	/* video hardware */
	MDRV_SCREEN_ADD("main", LCD)
	MDRV_SCREEN_REFRESH_RATE(44)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(240, 64)
	MDRV_SCREEN_VISIBLE_AREA(0, 239, 0, 63)

	MDRV_DEFAULT_LAYOUT(layout_px4)

	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(black_and_white)

	MDRV_VIDEO_UPDATE(px4)

	/* rom capsules */
	MDRV_CARTSLOT_ADD("capsule1")
	MDRV_CARTSLOT_EXTENSION_LIST("bin")
	MDRV_CARTSLOT_NOT_MANDATORY
	MDRV_CARTSLOT_ADD("capsule2")
	MDRV_CARTSLOT_EXTENSION_LIST("bin")
	MDRV_CARTSLOT_NOT_MANDATORY
MACHINE_DRIVER_END


/***************************************************************************
    ROM DEFINITIONS
***************************************************************************/

ROM_START( px4 )
    ROM_REGION(0x8000, "os", 0)
    ROM_LOAD("po_px4.bin", 0x0000, 0x8000, CRC(62d60dc6) SHA1(3d32ec79a317de7c84c378302e95f48d56505502))

	ROM_REGION(0x8000, "capsule1", 0)
	ROM_CART_LOAD("capsule1", 0x0000, 0x8000, ROM_OPTIONAL)

	ROM_REGION(0x8000, "capsule2", 0)
	ROM_CART_LOAD("capsule2", 0x0000, 0x8000, ROM_OPTIONAL)
ROM_END

ROM_START( px4p )
    ROM_REGION(0x8000, "os", 0)
    ROM_LOAD("b0_pxa.bin", 0x0000, 0x8000, CRC(d74b9ef5) SHA1(baceee076c12f5a16f7a26000e9bc395d021c455))

    ROM_REGION(0x8000, "capsule1", 0)
    ROM_CART_LOAD("capsule1", 0x0000, 0x8000, ROM_OPTIONAL)

    ROM_REGION(0x8000, "capsule2", 0)
    ROM_CART_LOAD("capsule2", 0x0000, 0x8000, ROM_OPTIONAL)
ROM_END


/***************************************************************************
    SYSTEM CONFIG
***************************************************************************/

static SYSTEM_CONFIG_START( px4 )
	CONFIG_RAM_DEFAULT(64 * 1024) /* 64KB RAM */
SYSTEM_CONFIG_END


/***************************************************************************
    GAME DRIVERS
***************************************************************************/

/*    YEAR  NAME  PARENT  COMPAT  MACHINE  INPUT      INIT  CONFIG  COMPANY  FULLNAME  FLAGS */
COMP( 1985, px4,  0,      0,      px4,     px4_ascii, px4,  px4,    "Epson", "PX-4",   GAME_NOT_WORKING )
COMP( 1985, px4p, px4,    0,      px4,     px4_ascii, px4,  px4,    "Epson", "PX-4+",  GAME_NOT_WORKING )

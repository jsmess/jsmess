/******************************************************************************
 * Enterprise 128k driver
 *
 * Kevin Thacker 1999.
 *
 * James Boulton [EP help]
 * Jean-Pierre Malisse [EP help]
 *
 * EP Hardware: Z80 (CPU), Dave (Sound Chip + I/O)
 * Nick (Graphics), WD1772 (FDC). 128k ram.
 *
 * For an 8-bit machine, this kicks ass! A sound
 * Chip which is as powerful, or more powerful than
 * the C64 SID, and a graphics chip capable of some
 * really nice graphics. Pity it doesn't have HW sprites!
 ******************************************************************************/

#include "driver.h"
#include "audio/dave.h"
#include "includes/enterp.h"
#include "video/epnick.h"
#include "machine/wd17xx.h"
#include "cpuintrf.h"
#include "devices/basicdsk.h"
/* for CPCEMU style disk images */
#include "devices/dsk.h"
#include "image.h"

/* there are 64us per line, although in reality
   about 50 are visible. */
/* there are 312 lines per screen, although in reality
   about 35*8 are visible */
#define ENTERPRISE_SCREEN_WIDTH (50*16)
#define ENTERPRISE_SCREEN_HEIGHT	(35*8)

/* Enterprise bank allocations */
#define MEM_EXOS_0		0
#define MEM_EXOS_1		1
#define MEM_CART_0		4
#define MEM_CART_1		5
#define MEM_CART_2		6
#define MEM_CART_3		7
#define MEM_EXDOS_0 	0x020
#define MEM_EXDOS_1 	0x021
/* basic 64k ram */
#define MEM_RAM_0				((unsigned int)0x0fc)
#define MEM_RAM_1				((unsigned int)0x0fd)
#define MEM_RAM_2				((unsigned int)0x0fe)
#define MEM_RAM_3				((unsigned int)0x0ff)
/* additional 64k ram */
#define MEM_RAM_4				((unsigned int)0x0f8)
#define MEM_RAM_5				((unsigned int)0x0f9)
#define MEM_RAM_6				((unsigned int)0x0fa)
#define MEM_RAM_7				((unsigned int)0x0fb)

WRITE8_HANDLER ( Nick_reg_w );


/* The Page index for each 16k page is programmed into
Dave. This index is a 8-bit number. The array below
defines what data is pointed to by each of these page index's
that can be selected. If NULL, when reading return floating
bus byte, when writing ignore */
static unsigned char * Enterprise_Pages_Read[256];
static unsigned char * Enterprise_Pages_Write[256];

/* index of keyboard line to read */
static int Enterprise_KeyboardLine = 0;

/* set read/write pointers for CPU page */
static void	Enterprise_SetMemoryPage(int CPU_Page, int EP_Page)
{
	memory_set_bankptr((CPU_Page+1), Enterprise_Pages_Read[EP_Page & 0x0ff]);
	memory_set_bankptr((CPU_Page+5), Enterprise_Pages_Write[EP_Page & 0x0ff]);
}

/* EP specific handling of dave register write */
static void enterprise_dave_reg_write(int RegIndex, int Data)
{
	switch (RegIndex)
	{

	case 0x010:
		{
		  /* set CPU memory page 0 */
			Enterprise_SetMemoryPage(0, Data);
		}
		break;

	case 0x011:
		{
		  /* set CPU memory page 1 */
			Enterprise_SetMemoryPage(1, Data);
		}
		break;

	case 0x012:
		{
		  /* set CPU memory page 2 */
			Enterprise_SetMemoryPage(2, Data);
		}
		break;

	case 0x013:
		{
		  /* set CPU memory page 3 */
			Enterprise_SetMemoryPage(3, Data);
		}
		break;

	case 0x015:
		{
		  /* write keyboard line */
			Enterprise_KeyboardLine = Data & 15;
		}
		break;

	default:
		break;
	}
}

static void enterprise_dave_reg_read(int RegIndex)
{
	switch (RegIndex)
	{
	case 0x015:
		{
		  /* read keyboard line */
			Dave_setreg(0x015,
				readinputport(Enterprise_KeyboardLine));
		}
		break;

		case 0x016:
		{
				int ExternalJoystickInputs;
				int ExternalJoystickPortInput = readinputport(10);

				if (Enterprise_KeyboardLine<=4)
				{
						ExternalJoystickInputs = ExternalJoystickPortInput>>(4-Enterprise_KeyboardLine);
				}
				else
				{
						ExternalJoystickInputs = 1;
				}

				Dave_setreg(0x016, (0x0fe | (ExternalJoystickInputs & 0x01)));
		}
		break;

	default:
		break;
	}
}

static void enterprise_dave_interrupt(int state)
{
	if (state)
		cpunum_set_input_line(0,0,HOLD_LINE);
	else
		cpunum_set_input_line(0,0,CLEAR_LINE);
}

/* enterprise interface to dave - ok, so Dave chip is unique
to Enterprise. But these functions make it nice and easy to see
whats going on. */
DAVE_INTERFACE	enterprise_dave_interface=
{
	enterprise_dave_reg_read,
		enterprise_dave_reg_write,
		enterprise_dave_interrupt,
};


static void enterp_wd177x_callback(int);

void Enterprise_Initialise()
{
	int i;

	for (i=0; i<256; i++)
	{
		/* reads to memory pages that are not set returns 0x0ff */
		Enterprise_Pages_Read[i] = mess_ram+0x020000;
		/* writes to memory pages that are not set are ignored */
		Enterprise_Pages_Write[i] = mess_ram+0x024000;
	}

	/* setup dummy read area so it will always return 0x0ff */
	memset(mess_ram+0x020000, 0x0ff, 0x04000);

	/* set read pointers */
	/* exos */
	Enterprise_Pages_Read[MEM_EXOS_0] = &memory_region(REGION_CPU1)[0x010000];
	Enterprise_Pages_Read[MEM_EXOS_1] = &memory_region(REGION_CPU1)[0x014000];
	/* basic */
	Enterprise_Pages_Read[MEM_CART_0] = &memory_region(REGION_CPU1)[0x018000];
	/* ram */
	Enterprise_Pages_Read[MEM_RAM_0] = mess_ram;
	Enterprise_Pages_Read[MEM_RAM_1] = mess_ram + 0x04000;
	Enterprise_Pages_Read[MEM_RAM_2] = mess_ram + 0x08000;
	Enterprise_Pages_Read[MEM_RAM_3] = mess_ram + 0x0c000;
	Enterprise_Pages_Read[MEM_RAM_4] = mess_ram + 0x010000;
	Enterprise_Pages_Read[MEM_RAM_5] = mess_ram + 0x014000;
	Enterprise_Pages_Read[MEM_RAM_6] = mess_ram + 0x018000;
	Enterprise_Pages_Read[MEM_RAM_7] = mess_ram + 0x01c000;
	/* exdos */
	Enterprise_Pages_Read[MEM_EXDOS_0] = &memory_region(REGION_CPU1)[0x01c000];
	Enterprise_Pages_Read[MEM_EXDOS_1] = &memory_region(REGION_CPU1)[0x020000];

	/* set write pointers */
	Enterprise_Pages_Write[MEM_RAM_0] = mess_ram;
	Enterprise_Pages_Write[MEM_RAM_1] = mess_ram + 0x04000;
	Enterprise_Pages_Write[MEM_RAM_2] = mess_ram + 0x08000;
	Enterprise_Pages_Write[MEM_RAM_3] = mess_ram + 0x0c000;
	Enterprise_Pages_Write[MEM_RAM_4] = mess_ram + 0x010000;
	Enterprise_Pages_Write[MEM_RAM_5] = mess_ram + 0x014000;
	Enterprise_Pages_Write[MEM_RAM_6] = mess_ram + 0x018000;
	Enterprise_Pages_Write[MEM_RAM_7] = mess_ram + 0x01c000;

	Dave_Init();

	Dave_SetIFace(&enterprise_dave_interface);

	Dave_reg_w(0x010,0);
	Dave_reg_w(0x011,0);
	Dave_reg_w(0x012,0);
	Dave_reg_w(0x013,0);

	cpunum_set_input_line_vector(0,0,0x0ff);

	wd179x_init(WD_TYPE_177X, enterp_wd177x_callback);

	floppy_drive_set_geometry(image_from_devtype_and_index(IO_FLOPPY, 0), FLOPPY_DRIVE_DS_80);
}

static  READ8_HANDLER ( enterprise_wd177x_read )
{
	switch (offset & 0x03)
	{
	case 0:
		return wd179x_status_r(offset);
	case 1:
		return wd179x_track_r(offset);
	case 2:
		return wd179x_sector_r(offset);
	case 3:
		return wd179x_data_r(offset);
	default:
		break;
	}

	return 0x0ff;
}

static WRITE8_HANDLER (	enterprise_wd177x_write )
{
	switch (offset & 0x03)
	{
	case 0:
		wd179x_command_w(offset, data);
		return;
	case 1:
		wd179x_track_w(offset, data);
		return;
	case 2:
		wd179x_sector_w(offset, data);
		return;
	case 3:
		wd179x_data_w(offset, data);
		return;
	default:
		break;
	}
}



/* I've done this because the ram is banked in 16k blocks, and
the rom can be paged into bank 0 and bank 3. */
ADDRESS_MAP_START( enterprise_mem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE( 0x00000, 0x03fff) AM_READWRITE( MRA8_BANK1, MWA8_BANK5 )
	AM_RANGE( 0x04000, 0x07fff) AM_READWRITE( MRA8_BANK2, MWA8_BANK6 )
	AM_RANGE( 0x08000, 0x0bfff) AM_READWRITE( MRA8_BANK3, MWA8_BANK7 )
	AM_RANGE( 0x0c000, 0x0ffff) AM_READWRITE( MRA8_BANK4, MWA8_BANK8 )
ADDRESS_MAP_END



/* bit 0 - select drive 0,
   bit 1 - select drive 1,
   bit 2 - select drive 2,
   bit 3 - select drive 3
   bit 4 - side
   bit 5 - mfm/fm select
   bit 6 - disk change reset
   bit 7 - in use
*/

static int EXDOS_GetDriveSelection(int data)
{
	if (data & 0x01)
		return 0;
	if (data & 0x02)
		return 1;
	if (data & 0x04)
		return 2;
	if (data & 0x08)
		return 3;
	return 0;
}

static char EXDOS_CARD_R = 0;

static void enterp_wd177x_callback(int State)
{
   if (State==WD179X_IRQ_CLR)
   {
		EXDOS_CARD_R &= ~0x02;
   }

   if (State==WD179X_IRQ_SET)
   {
		EXDOS_CARD_R |= 0x02;
   }

   if (State==WD179X_DRQ_CLR)
   {
		EXDOS_CARD_R &= ~0x080;
   }

   if (State==WD179X_DRQ_SET)
   {
		EXDOS_CARD_R |= 0x080;
   }
}



static WRITE8_HANDLER ( exdos_card_w )
{
	/* drive side */
	int head = (data>>4) & 0x01;

	int drive = EXDOS_GetDriveSelection(data);

	wd179x_set_drive(drive);
	wd179x_set_side(head);
}

/* bit 0 - ??
   bit 1 - IRQ from WD1772
   bit 2 - ??
   bit 3 - ??
   bit 4 - ??
   bit 5 - ??
   bit 6 - Disk change signal from disk drive
   bit 7 - DRQ from WD1772
*/


static  READ8_HANDLER ( exdos_card_r )
{
	return EXDOS_CARD_R;
}

ADDRESS_MAP_START( enterprise_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE( 0x010, 0x017) AM_READWRITE( enterprise_wd177x_read, enterprise_wd177x_write )
	AM_RANGE( 0x018, 0x018) AM_READWRITE( exdos_card_r, exdos_card_w )
	AM_RANGE( 0x01c, 0x01c) AM_READWRITE( exdos_card_r, exdos_card_w )
	AM_RANGE( 0x080, 0x08f) AM_WRITE( Nick_reg_w )
	AM_RANGE( 0x0a0, 0x0bf) AM_READWRITE( Dave_reg_r, Dave_reg_w )
ADDRESS_MAP_END



/*
Enterprise Keyboard Matrix

		Bit
Line	0	 1	  2    3	4	 5	  6    7
0		n	 \	  b    c	v	 x	  z    SHFT
1		h	 N/C  g    d	f	 s	  a    CTRL
2		u	 q	  y    r	t	 e	  w    TAB
3		7	 1	  6    4	5	 3	  2    N/C
4		F4	 F8   F3   F6	F5	 F7   F2   F1
5		8	 N/C  9    -	0	 ^	  DEL  N/C
6		j	 N/C  k    ;	l	 :	  ]    N/C
7		STOP DOWN RGHT UP	HOLD LEFT RETN ALT
8		m	 ERSE ,    /	.	 SHFT SPCE INS
9		i	 N/C  o    @	p	 [	  N/C  N/C

N/C - Not connected or just dont know!
*/


INPUT_PORTS_START( ep128 )
	/* keyboard line 0 */
	 PORT_START
	 PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("n") PORT_CODE(KEYCODE_N)
	 PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("\\") PORT_CODE(KEYCODE_SLASH)
	 PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("b") PORT_CODE(KEYCODE_B)
	 PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("c") PORT_CODE(KEYCODE_C)
	 PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("v") PORT_CODE(KEYCODE_V)
	 PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("x") PORT_CODE(KEYCODE_X)
	 PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("z") PORT_CODE(KEYCODE_Z)
	 PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SHIFT") PORT_CODE(KEYCODE_LSHIFT)

	 /* keyboard line 1 */
	 PORT_START
	 PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("h") PORT_CODE(KEYCODE_H)
	 PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("n/c")
	 PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("g") PORT_CODE(KEYCODE_G)
	 PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("d") PORT_CODE(KEYCODE_D)
	 PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("f") PORT_CODE(KEYCODE_F)
	 PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("s") PORT_CODE(KEYCODE_S)
	 PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("a") PORT_CODE(KEYCODE_A)
	 PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("CTRL") PORT_CODE(KEYCODE_LCONTROL)

	 /* keyboard line 2 */
	 PORT_START
	 PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("u") PORT_CODE(KEYCODE_U)
	 PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("q") PORT_CODE(KEYCODE_Q)
	 PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("y") PORT_CODE(KEYCODE_Y)
	 PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("r") PORT_CODE(KEYCODE_R)
	 PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("t") PORT_CODE(KEYCODE_T)
	 PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("e") PORT_CODE(KEYCODE_E)
	 PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("w") PORT_CODE(KEYCODE_W)
	 PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("TAB") PORT_CODE(KEYCODE_TAB)

	 /* keyboard line 3 */
	 PORT_START
	 PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("7") PORT_CODE(KEYCODE_7)
	 PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("1") PORT_CODE(KEYCODE_1)
	 PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("6") PORT_CODE(KEYCODE_6)
	 PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("4") PORT_CODE(KEYCODE_4)
	 PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("5") PORT_CODE(KEYCODE_5)
	 PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("3") PORT_CODE(KEYCODE_3)
	 PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("2") PORT_CODE(KEYCODE_2)
	 PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("n/c")

	 /* keyboard line 4 */
	 PORT_START
	 PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("f4") PORT_CODE(KEYCODE_F4)
	 PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("f8") PORT_CODE(KEYCODE_F8)
	 PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("f3") PORT_CODE(KEYCODE_F3)
	 PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("f6") PORT_CODE(KEYCODE_F6)
	 PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("f5") PORT_CODE(KEYCODE_F5)
	 PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("f7") PORT_CODE(KEYCODE_F7)
	 PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("f2") PORT_CODE(KEYCODE_F2)
	 PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("f1") PORT_CODE(KEYCODE_F1)

	 /* keyboard line 5 */
	 PORT_START
	 PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("8") PORT_CODE(KEYCODE_8)
	 PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("n/c")
	 PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("9") PORT_CODE(KEYCODE_9)
	 PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("-") PORT_CODE(KEYCODE_MINUS)
	 PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("0") PORT_CODE(KEYCODE_0)
	 PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("^") PORT_CODE(KEYCODE_EQUALS)
	 PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("DEL") PORT_CODE(KEYCODE_BACKSPACE)
	 PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("n/c")

	 /* keyboard line 6 */
	 PORT_START
	 PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("j") PORT_CODE(KEYCODE_J)
	 PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("n/c")
	 PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("k") PORT_CODE(KEYCODE_K)
	 PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(";") PORT_CODE(KEYCODE_COLON)
	 PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("l") PORT_CODE(KEYCODE_L)
	 PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(":") PORT_CODE(KEYCODE_QUOTE)
	 PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("]") PORT_CODE(KEYCODE_CLOSEBRACE)
	 PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("n/c")

	 /* keyboard line 7 */
	 PORT_START
	 PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("STOP") PORT_CODE(KEYCODE_END)
	 PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("DOWN") PORT_CODE(KEYCODE_DOWN) PORT_CODE(JOYCODE_1_DOWN)
	 PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("RIGHT") PORT_CODE(KEYCODE_RIGHT) PORT_CODE(JOYCODE_1_RIGHT)
	 PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("UP") PORT_CODE(KEYCODE_UP) PORT_CODE(JOYCODE_1_UP)
	 PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("HOLD") PORT_CODE(KEYCODE_HOME)
	 PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("LEFT") PORT_CODE(KEYCODE_LEFT) PORT_CODE(JOYCODE_1_LEFT)
	 PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("RETURN") PORT_CODE(KEYCODE_ENTER)
	 PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("ALT") PORT_CODE(KEYCODE_LALT)


	 /* keyboard line 8 */
	 PORT_START
	 PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("m") PORT_CODE(KEYCODE_M)
	 PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("ERASE") PORT_CODE(KEYCODE_DEL)
	 PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(",") PORT_CODE(KEYCODE_COMMA)
	 PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("/") PORT_CODE(KEYCODE_SLASH)
	 PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(".") PORT_CODE(KEYCODE_STOP)
	 PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SHIFT") PORT_CODE(KEYCODE_RSHIFT)
	 PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SPACE") PORT_CODE(KEYCODE_SPACE)
	 PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("INSERT") PORT_CODE(KEYCODE_INSERT)


	 /* keyboard line 9 */
	 PORT_START
	 PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("i") PORT_CODE(KEYCODE_I)
	 PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("n/c")
	 PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("o") PORT_CODE(KEYCODE_O)
	 PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("@")
	 PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("p") PORT_CODE(KEYCODE_P)
	 PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("[") PORT_CODE(KEYCODE_OPENBRACE)
	 PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("n/c")
	 PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("n/c")

	 /* external joystick 1 */
	 PORT_START
	 PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("EXTERNAL JOYSTICK 1 RIGHT") PORT_CODE(KEYCODE_RIGHT) PORT_CODE(JOYCODE_1_RIGHT)
	 PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("EXTERNAL JOYSTICK 1 LEFT") PORT_CODE(KEYCODE_LEFT) PORT_CODE(JOYCODE_1_LEFT)
	 PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("EXTERNAL JOYSTICK 1 DOWN") PORT_CODE(KEYCODE_DOWN) PORT_CODE(JOYCODE_1_DOWN)
	 PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("EXTERNAL JOYSTICK 1 UP") PORT_CODE(KEYCODE_UP) PORT_CODE(JOYCODE_1_UP)
	 PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("EXTERNAL JOYSTICK 1 FIRE") PORT_CODE(KEYCODE_SPACE) PORT_CODE(JOYCODE_1_BUTTON1)


INPUT_PORTS_END

static struct CustomSound_interface dave_custom_sound =
{
	Dave_sh_start
};

/* 4Mhz clock, although it can be changed to 8 Mhz! */

static MACHINE_DRIVER_START( ep128 )
	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 4000000)
	MDRV_CPU_PROGRAM_MAP(enterprise_mem, 0)
	MDRV_CPU_IO_MAP(enterprise_io, 0)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_RESET( enterprise )

    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(ENTERPRISE_SCREEN_WIDTH, ENTERPRISE_SCREEN_HEIGHT)
	MDRV_SCREEN_VISIBLE_AREA(0, ENTERPRISE_SCREEN_WIDTH-1, 0, ENTERPRISE_SCREEN_HEIGHT-1)
	/* MDRV_GFXDECODE( enterprise ) */
	MDRV_PALETTE_LENGTH(NICK_PALETTE_SIZE)
	MDRV_COLORTABLE_LENGTH(NICK_COLOURTABLE_SIZE)
	MDRV_PALETTE_INIT( nick )

	MDRV_VIDEO_START( enterprise )
	MDRV_VIDEO_UPDATE( enterprise )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(CUSTOM, 0)
	MDRV_SOUND_CONFIG(dave_custom_sound)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
MACHINE_DRIVER_END

ROM_START( ep128 )
		/* 128k ram + 32k rom (OS) + 16k rom (BASIC) + 32k rom (EXDOS) */
		ROM_REGION(0x24000,REGION_CPU1,0)
		ROM_LOAD("exos.rom",0x10000,0x8000, CRC(d421795f) SHA1(6033a0535136c40c47137e4d1cd9273c06d5fdff))
		ROM_LOAD("exbas.rom",0x18000,0x4000, CRC(683cf455) SHA1(50a548d1df3ea86f9b5fa669afd8ff124050e776))
		ROM_LOAD("exdos.rom",0x1c000,0x8000, CRC(d1d7e157) SHA1(31c8be089526aa8aa019c380cdf51ddd3ee76454))
ROM_END

ROM_START( ep128a )
		/* 128k ram + 32k rom (OS) + 16k rom (BASIC) + 32k rom (EXDOS) */
		ROM_REGION(0x24000,REGION_CPU1,0)
		ROM_LOAD("exos21.rom",0x10000,0x8000, CRC(982a3b44) SHA1(55315b20fecb4441a07ee4bc5dc7153f396e0a2e))
		ROM_LOAD("exbas.rom",0x18000,0x4000, CRC(683cf455) SHA1(50a548d1df3ea86f9b5fa669afd8ff124050e776))
		ROM_LOAD("exdos.rom",0x1c000,0x8000, CRC(d1d7e157) SHA1(31c8be089526aa8aa019c380cdf51ddd3ee76454))
ROM_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

static void ep128_floppy_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* floppy */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 4; break;

		default:										legacydsk_device_getinfo(devclass, state, info); break;
	}
}

#if 0
static void ep128_floppy_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* floppy */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 4; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_LOAD:							info->load = enterprise_floppy_init; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "dsk"); break;

		default:										legacybasicdsk_device_getinfo(devclass, state, info); break;
	}
}
#endif

SYSTEM_CONFIG_START(ep128)
	CONFIG_RAM_DEFAULT((128*1024)+32768)
	CONFIG_DEVICE(ep128_floppy_getinfo)
#if 0
	CONFIG_DEVICE(ep128_floppy_getinfo)
#endif
SYSTEM_CONFIG_END

/*      YEAR  NAME		PARENT	COMPAT	MACHINE INPUT   INIT  CONFIG, COMPANY                 FULLNAME */
COMP( 1984, ep128,		0,		0,		ep128,	ep128,	0,	  ep128,  "Intelligent Software", "Enterprise 128", GAME_IMPERFECT_SOUND )
COMP( 1984, ep128a,	ep128,	0,		ep128,	ep128,	0,	  ep128,  "Intelligent Software", "Enterprise 128 (EXOS 2.1)", GAME_IMPERFECT_SOUND )


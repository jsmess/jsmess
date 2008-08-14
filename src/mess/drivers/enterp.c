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
#include "deprecat.h"
#include "audio/dave.h"
#include "includes/enterp.h"
#include "video/epnick.h"
#include "machine/wd17xx.h"
#include "devices/basicdsk.h"
/* for CPCEMU style disk images */
#include "devices/dsk.h"


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
	static const char *keynames[] = { "LINE0", "LINE1", "LINE2", "LINE3", "LINE4", 
										"LINE5", "LINE6", "LINE7", "LINE8", "LINE9" };
	running_machine *machine = Machine;

	switch (RegIndex)
	{
	case 0x015:
		{
		/* read keyboard line */
		Dave_setreg(machine, 0x015, input_port_read(machine, keynames[Enterprise_KeyboardLine]));
		}
		break;

	case 0x016:
	{
		int ExternalJoystickInputs;
		int ExternalJoystickPortInput = input_port_read(machine, "JOY1");

		if (Enterprise_KeyboardLine<=4)
		{
				ExternalJoystickInputs = ExternalJoystickPortInput>>(4-Enterprise_KeyboardLine);
		}
		else
		{
				ExternalJoystickInputs = 1;
		}

		Dave_setreg(machine, 0x016, (0x0fe | (ExternalJoystickInputs & 0x01)));
	}
	break;

	default:
		break;
	}
}

static void enterprise_dave_interrupt(int state)
{
	if (state)
		cpunum_set_input_line(Machine, 0,0,HOLD_LINE);
	else
		cpunum_set_input_line(Machine, 0,0,CLEAR_LINE);
}

/* enterprise interface to dave - ok, so Dave chip is unique
to Enterprise. But these functions make it nice and easy to see
whats going on. */
static const DAVE_INTERFACE enterprise_dave_interface =
{
	enterprise_dave_reg_read,
	enterprise_dave_reg_write,
	enterprise_dave_interrupt,
};


static void enterp_wd177x_callback(running_machine *machine, wd17xx_state_t event, void *param);

static void enterprise_reset(running_machine *machine)
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
	Enterprise_Pages_Read[MEM_EXOS_0] = &memory_region(machine, "main")[0x010000];
	Enterprise_Pages_Read[MEM_EXOS_1] = &memory_region(machine, "main")[0x014000];
	/* basic */
	Enterprise_Pages_Read[MEM_CART_0] = &memory_region(machine, "main")[0x018000];
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
	Enterprise_Pages_Read[MEM_EXDOS_0] = &memory_region(machine, "main")[0x01c000];
	Enterprise_Pages_Read[MEM_EXDOS_1] = &memory_region(machine, "main")[0x020000];

	/* set write pointers */
	Enterprise_Pages_Write[MEM_RAM_0] = mess_ram;
	Enterprise_Pages_Write[MEM_RAM_1] = mess_ram + 0x04000;
	Enterprise_Pages_Write[MEM_RAM_2] = mess_ram + 0x08000;
	Enterprise_Pages_Write[MEM_RAM_3] = mess_ram + 0x0c000;
	Enterprise_Pages_Write[MEM_RAM_4] = mess_ram + 0x010000;
	Enterprise_Pages_Write[MEM_RAM_5] = mess_ram + 0x014000;
	Enterprise_Pages_Write[MEM_RAM_6] = mess_ram + 0x018000;
	Enterprise_Pages_Write[MEM_RAM_7] = mess_ram + 0x01c000;

	Dave_Init(machine);

	Dave_SetIFace(&enterprise_dave_interface);

	Dave_reg_w(machine, 0x010,0);
	Dave_reg_w(machine, 0x011,0);
	Dave_reg_w(machine, 0x012,0);
	Dave_reg_w(machine, 0x013,0);

	cpunum_set_input_line_vector(0,0,0x0ff);

	floppy_drive_set_geometry(image_from_devtype_and_index(IO_FLOPPY, 0), FLOPPY_DRIVE_DS_80);
}

static MACHINE_START(enterprise)
{
	wd17xx_init(machine, WD_TYPE_177X, enterp_wd177x_callback, NULL);
	add_reset_callback(machine, enterprise_reset);
}

static  READ8_HANDLER ( enterprise_wd177x_read )
{
	switch (offset & 0x03)
	{
	case 0:
		return wd17xx_status_r(machine, offset);
	case 1:
		return wd17xx_track_r(machine, offset);
	case 2:
		return wd17xx_sector_r(machine, offset);
	case 3:
		return wd17xx_data_r(machine, offset);
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
		wd17xx_command_w(machine, offset, data);
		return;
	case 1:
		wd17xx_track_w(machine, offset, data);
		return;
	case 2:
		wd17xx_sector_w(machine, offset, data);
		return;
	case 3:
		wd17xx_data_w(machine, offset, data);
		return;
	default:
		break;
	}
}



/* I've done this because the ram is banked in 16k blocks, and
the rom can be paged into bank 0 and bank 3. */
static ADDRESS_MAP_START( enterprise_mem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE( 0x00000, 0x03fff) AM_READWRITE( SMH_BANK1, SMH_BANK5 )
	AM_RANGE( 0x04000, 0x07fff) AM_READWRITE( SMH_BANK2, SMH_BANK6 )
	AM_RANGE( 0x08000, 0x0bfff) AM_READWRITE( SMH_BANK3, SMH_BANK7 )
	AM_RANGE( 0x0c000, 0x0ffff) AM_READWRITE( SMH_BANK4, SMH_BANK8 )
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

static void enterp_wd177x_callback(running_machine *machine, wd17xx_state_t State, void *param)
{
   if (State==WD17XX_IRQ_CLR)
   {
		EXDOS_CARD_R &= ~0x02;
   }

   if (State==WD17XX_IRQ_SET)
   {
		EXDOS_CARD_R |= 0x02;
   }

   if (State==WD17XX_DRQ_CLR)
   {
		EXDOS_CARD_R &= ~0x080;
   }

   if (State==WD17XX_DRQ_SET)
   {
		EXDOS_CARD_R |= 0x080;
   }
}



static WRITE8_HANDLER ( exdos_card_w )
{
	/* drive side */
	int head = (data>>4) & 0x01;

	int drive = EXDOS_GetDriveSelection(data);

	wd17xx_set_drive(drive);
	wd17xx_set_side(head);
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

static ADDRESS_MAP_START( enterprise_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE( 0x010, 0x017) AM_READWRITE( enterprise_wd177x_read, enterprise_wd177x_write )
	AM_RANGE( 0x018, 0x018) AM_READWRITE( exdos_card_r, exdos_card_w )
	AM_RANGE( 0x01c, 0x01c) AM_READWRITE( exdos_card_r, exdos_card_w )
	AM_RANGE( 0x080, 0x08f) AM_WRITE( Nick_reg_w )
	AM_RANGE( 0x0a0, 0x0bf) AM_READWRITE( Dave_reg_r, Dave_reg_w )
ADDRESS_MAP_END



/*
Enterprise Keyboard Matrix

        Bit
Line    0    1    2    3    4    5    6    7
0       n    \    b    c    v    x    z    SHFT
1       h    N/C  g    d    f    s    a    CTRL
2       u    q    y    r    t    e    w    TAB
3       7    1    6    4    5    3    2    N/C
4       F4   F8   F3   F6   F5   F7   F2   F1
5       8    N/C  9    -    0    ^    DEL  N/C
6       j    N/C  k    ;    l    :    ]    N/C
7       STOP DOWN RGHT UP   HOLD LEFT RETN ALT
8       m    ERSE ,    /    .    SHFT SPCE INS
9       i    N/C  o    @    p    [    N/C  N/C

N/C - Not connected or just dont know!

2008-05 FP:
Notice that I updated the matrix above with the following new info: 
l1:b1 is LOCK: you press it with CTRL to switch to Caps, you press it again to switch back 
	(you can also use it with ALT)
l3:b7 is ESC: you use it to exit from nested programs (e.g. if you start to write a program in BASIC,
	then start EXDOS, you can use ESC to go back to BASIC without losing the program you were writing)

According to pictures and manuals, there seem to be no more keys connected, so I label the remaining N/C 
as IPT_UNUSED.

Small note about natural keyboard support: currently
- "Lock" is mapped to 'F9' (it acts like a CAPSLOCK, but no IPT_TOGGLE)
- "Stop" is mapped to 'F10'
- "Hold" is mapped to 'F11'
*/


static INPUT_PORTS_START( ep128 )
	PORT_START("LINE0")		/* keyboard line 0 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_N)			PORT_CHAR('n') PORT_CHAR('N')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_TILDE)		PORT_CHAR('\\') PORT_CHAR('|')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_B)			PORT_CHAR('b') PORT_CHAR('B')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_C)			PORT_CHAR('c') PORT_CHAR('C')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_V)			PORT_CHAR('v') PORT_CHAR('V')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_X)			PORT_CHAR('x') PORT_CHAR('X')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Z)			PORT_CHAR('z') PORT_CHAR('Z')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_LSHIFT)		PORT_CHAR(UCHAR_SHIFT_1)

	PORT_START("LINE1")		/* keyboard line 1 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_H)			PORT_CHAR('h') PORT_CHAR('H')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("LOCK") PORT_CODE(KEYCODE_F9) PORT_CHAR(UCHAR_MAMEKEY(F9))
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_G)			PORT_CHAR('g') PORT_CHAR('G')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_D)			PORT_CHAR('d') PORT_CHAR('D')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F)			PORT_CHAR('f') PORT_CHAR('F')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_S)			PORT_CHAR('s') PORT_CHAR('S')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_A)			PORT_CHAR('a') PORT_CHAR('A')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_LCONTROL)		PORT_CHAR(UCHAR_SHIFT_2)

	PORT_START("LINE2")		/* keyboard line 2 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_U)			PORT_CHAR('u') PORT_CHAR('U')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Q)			PORT_CHAR('q') PORT_CHAR('Q')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Y)			PORT_CHAR('y') PORT_CHAR('Y')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_R)			PORT_CHAR('r') PORT_CHAR('R')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_T)			PORT_CHAR('t') PORT_CHAR('T')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_E)			PORT_CHAR('e') PORT_CHAR('E')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_W)			PORT_CHAR('w') PORT_CHAR('W')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_TAB)			PORT_CHAR('\t')

	PORT_START("LINE3")		/* keyboard line 3 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_7)			PORT_CHAR('7') PORT_CHAR('\'')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_1)			PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_6)			PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_4)			PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_5)			PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("3 \xC2\xA3")	PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('\xA3')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_2)			PORT_CHAR('2') PORT_CHAR('"')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_ESC)			PORT_CHAR(UCHAR_MAMEKEY(ESC))

	PORT_START("LINE4")		/* keyboard line 4 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F4)			PORT_CHAR(UCHAR_MAMEKEY(F4))
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F8)			PORT_CHAR(UCHAR_MAMEKEY(F8))
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F3)			PORT_CHAR(UCHAR_MAMEKEY(F3))
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F6)			PORT_CHAR(UCHAR_MAMEKEY(F6))
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F5)			PORT_CHAR(UCHAR_MAMEKEY(F5))
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F7)			PORT_CHAR(UCHAR_MAMEKEY(F7))
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F2)			PORT_CHAR(UCHAR_MAMEKEY(F2))
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F1)			PORT_CHAR(UCHAR_MAMEKEY(F1))

	PORT_START("LINE5")		/* keyboard line 5 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_8)			PORT_CHAR('8') PORT_CHAR('(')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_9)			PORT_CHAR('9') PORT_CHAR(')')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_MINUS)		PORT_CHAR('-') PORT_CHAR('=')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_0)			PORT_CHAR('0') PORT_CHAR('_')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_EQUALS)		PORT_CHAR('^') PORT_CHAR('~')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("ERASE") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("LINE6")		/* keyboard line 6 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_J)			PORT_CHAR('j') PORT_CHAR('J')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_K)			PORT_CHAR('k') PORT_CHAR('K')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_COLON)		PORT_CHAR(';') PORT_CHAR('+')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_L)			PORT_CHAR('l') PORT_CHAR('L')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_QUOTE)		PORT_CHAR(':') PORT_CHAR('*')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_BACKSLASH)	PORT_CHAR(']') PORT_CHAR('}')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_UNUSED)

	/* Notice that, in fact, ep128 only had the built-in joystick and no cursor arrow keys on the keyboard */
	PORT_START("LINE7")		/* keyboard line 7 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("STOP")			PORT_CODE(KEYCODE_END)				PORT_CHAR(UCHAR_MAMEKEY(F10))
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_DOWN)		PORT_CODE(JOYCODE_Y_DOWN_SWITCH)	PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_RIGHT)	PORT_CODE(JOYCODE_X_RIGHT_SWITCH)	PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_UP)		PORT_CODE(JOYCODE_Y_UP_SWITCH)		PORT_CHAR(UCHAR_MAMEKEY(UP))
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("HOLD")			PORT_CODE(KEYCODE_HOME)				PORT_CHAR(UCHAR_MAMEKEY(F11))
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_LEFT)		PORT_CODE(JOYCODE_X_LEFT_SWITCH)	PORT_CHAR(UCHAR_MAMEKEY(LEFT))
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_ENTER)										PORT_CHAR(13)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("ALT")			PORT_CODE(KEYCODE_LALT)				PORT_CHAR(UCHAR_MAMEKEY(LALT))

	PORT_START("LINE8")		/* keyboard line 8 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_M)			PORT_CHAR('m') PORT_CHAR('M')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_DEL)			PORT_CHAR(UCHAR_MAMEKEY(DEL))
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_COMMA)		PORT_CHAR(',') PORT_CHAR('<')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_SLASH)		PORT_CHAR('/') PORT_CHAR('?')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_STOP)			PORT_CHAR('.') PORT_CHAR('>')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SHIFT (right)") PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_SPACE)		PORT_CHAR(' ')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_INSERT)		PORT_CHAR(UCHAR_MAMEKEY(INSERT))

	PORT_START("LINE9")		/* keyboard line 9 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_I)			PORT_CHAR('i') PORT_CHAR('I')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_O)			PORT_CHAR('o') PORT_CHAR('O')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_OPENBRACE)	PORT_CHAR('@') PORT_CHAR('`')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_P)			PORT_CHAR('p') PORT_CHAR('P')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_CLOSEBRACE)	PORT_CHAR('[') PORT_CHAR('{')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("JOY1")		/* external joystick 1 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("EXTERNAL JOYSTICK 1 RIGHT") PORT_CODE(KEYCODE_RIGHT) PORT_CODE(JOYCODE_X_RIGHT_SWITCH)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("EXTERNAL JOYSTICK 1 LEFT") PORT_CODE(KEYCODE_LEFT) PORT_CODE(JOYCODE_X_LEFT_SWITCH)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("EXTERNAL JOYSTICK 1 DOWN") PORT_CODE(KEYCODE_DOWN) PORT_CODE(JOYCODE_Y_DOWN_SWITCH)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("EXTERNAL JOYSTICK 1 UP") PORT_CODE(KEYCODE_UP) PORT_CODE(JOYCODE_Y_UP_SWITCH)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("EXTERNAL JOYSTICK 1 FIRE") PORT_CODE(KEYCODE_SPACE) PORT_CODE(JOYCODE_BUTTON1)

INPUT_PORTS_END

static const custom_sound_interface dave_custom_sound =
{
	Dave_sh_start
};

/* 4Mhz clock, although it can be changed to 8 Mhz! */

static MACHINE_DRIVER_START( ep128 )
	/* basic machine hardware */
	MDRV_CPU_ADD("main", Z80, 4000000)
	MDRV_CPU_PROGRAM_MAP(enterprise_mem, 0)
	MDRV_CPU_IO_MAP(enterprise_io, 0)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_START( enterprise )

    /* video hardware */
	MDRV_SCREEN_ADD("main", RASTER)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(ENTERPRISE_SCREEN_WIDTH, ENTERPRISE_SCREEN_HEIGHT)
	MDRV_SCREEN_VISIBLE_AREA(0, ENTERPRISE_SCREEN_WIDTH-1, 0, ENTERPRISE_SCREEN_HEIGHT-1)
	/* MDRV_GFXDECODE( enterprise ) */
	MDRV_PALETTE_LENGTH(NICK_PALETTE_SIZE)
	MDRV_PALETTE_INIT( nick )

	MDRV_VIDEO_START( enterprise )
	MDRV_VIDEO_UPDATE( enterprise )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("custom", CUSTOM, 0)
	MDRV_SOUND_CONFIG(dave_custom_sound)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
MACHINE_DRIVER_END

ROM_START( ep128 )
		/* 128k ram + 32k rom (OS) + 16k rom (BASIC) + 32k rom (EXDOS) */
		ROM_REGION( 0x24000, "main", 0 )
		ROM_SYSTEM_BIOS( 0, "default", "EXOS 2.1" )
		ROMX_LOAD("exos21.rom", 0x10000, 0x8000, CRC(982a3b44) SHA1(55315b20fecb4441a07ee4bc5dc7153f396e0a2e), ROM_BIOS(1) )
		ROM_SYSTEM_BIOS( 1, "exos20", "EXOS 2.0" )
		ROMX_LOAD("exos20.rom", 0x10000, 0x8000, CRC(d421795f) SHA1(6033a0535136c40c47137e4d1cd9273c06d5fdff), ROM_BIOS(2) )
		ROM_LOAD( "exbas.rom", 0x18000, 0x4000, CRC(683cf455) SHA1(50a548d1df3ea86f9b5fa669afd8ff124050e776) )
		ROM_LOAD( "exdos.rom", 0x1c000, 0x8000, CRC(d1d7e157) SHA1(31c8be089526aa8aa019c380cdf51ddd3ee76454) )
ROM_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

static void ep128_floppy_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* floppy */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:							info->i = 4; break;

		default:										legacydsk_device_getinfo(devclass, state, info); break;
	}
}

#if 0
static void ep128_floppy_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* floppy */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:							info->i = 4; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_LOAD:							info->load = DEVICE_IMAGE_LOAD_NAME(enterprise_floppy); break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "dsk"); break;

		default:										legacybasicdsk_device_getinfo(devclass, state, info); break;
	}
}
#endif

static SYSTEM_CONFIG_START(ep128)
	CONFIG_RAM_DEFAULT((128*1024)+32768)
	CONFIG_DEVICE(ep128_floppy_getinfo)
#if 0
	CONFIG_DEVICE(ep128_floppy_getinfo)
#endif
SYSTEM_CONFIG_END

/*      YEAR  NAME      PARENT  COMPAT  MACHINE INPUT   INIT  CONFIG, COMPANY                 FULLNAME */
COMP( 1984, ep128,		0,		0,		ep128,	ep128,	0,	  ep128,  "Intelligent Software", "Enterprise 128", GAME_IMPERFECT_SOUND )

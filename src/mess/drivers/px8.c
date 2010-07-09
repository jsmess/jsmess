/***************************************************************************

    Epson PX-8

    12/05/2009 Skeleton driver.

    Seems to be a CP/M computer.

***************************************************************************/

/*

    TODO:

    - dumps of the internal ROMs
    - uPD7508 CPU core
    - keyboard
    - cassette
    - display
    - jumpers
    - ROM capsule
    - uPD7001
    - RAM disk (64K/128K RAM, Z80, 4K ROM)
    - modem (82C55)
    - Multi-Unit (60K RAM, ROM capsule, modem, 82C55)
    - attach PF-10s

*/

#include "emu.h"
#include "includes/px8.h"
#include "cpu/z80/z80.h"
#include "cpu/m6800/m6800.h"
#include "devices/cartslot.h"
#include "devices/cassette.h"
#include "devices/messram.h"
#include "machine/msm8251.h"
#include "machine/pf10.h"
#include "sound/wave.h"
#include "video/sed1330.h"
#include "px8.lh"

/***************************************************************************
    CONSTANTS
***************************************************************************/

#define XTAL_CR1		XTAL_9_8304MHz
#define XTAL_CR2		XTAL_32_768kHz

/* interrupt sources */
#define INT0_7508		0x01
#define INT1_SERIAL		0x02
#define INT2_RS232		0x04
#define INT3_BARCODE	0x08
#define INT4_FRC		0x10
#define INT5_OPTION		0x20

enum
{
	GAH40M_CTLR1 = 0,
	GAH40M_CMDR,
	GAH40M_CTLR2,
	GAH40M_IER = 4,
};

enum
{
	GAH40M_ICRL_C = 0,
	GAH40M_ICRH_C,
	GAH40M_ICRL_B,
	GAH40M_ICRH_B,
	GAH40M_ISR,
	GAH40M_STR,
	GAH40M_SIOR,
	GAH40M_IVR
};

/***************************************************************************
    READ/WRITE HANDLERS
***************************************************************************/

/*-------------------------------------------------
    bankswitch - memory bankswitching
-------------------------------------------------*/

static void bankswitch(running_machine *machine)
{
	px8_state *state = (px8_state *)machine->driver_data;
	const address_space *program = cputag_get_address_space(machine, UPD70008_TAG, ADDRESS_SPACE_PROGRAM);
	UINT8 *ram = messram_get_ptr(machine->device("messram"));
	UINT8 *ipl_rom = memory_region(machine, UPD70008_TAG);

	if (!state->bank0)
	{
		/* IPL ROM */
		memory_install_rom(program, 0x0000, 0x7fff, 0, 0, ipl_rom);
	}
	else
	{
		if (state->bk2)
		{
			/* D-RAM (L) */
			memory_install_ram(program, 0x0000, 0x7fff, 0, 0, ram);
		}
		else
		{
			/* OPTION ROM (L) */
			memory_unmap_readwrite(program, 0x0000, 0x7fff, 0, 0);
		}
	}

	if (state->bk2)
	{
		/* D-RAM (H) */
		memory_install_ram(program, 0x8000, 0xffff, 0, 0, ram + 0x8000);
	}
	else
	{
		/* OPTION ROM (H) */
		memory_unmap_readwrite(program, 0x8000, 0xffff, 0, 0);
	}
}

/*-------------------------------------------------
    gah40m_r - GAH40M read
-------------------------------------------------*/

static READ8_HANDLER( gah40m_r )
{
	switch (offset)
	{
	case GAH40M_ICRL_C:
		/*

            bit     signal      description

            0       ICR0
            1       ICR1
            2       ICR2
            3       ICR3
            4       ICR4
            5       ICR5
            6       ICR6
            7       ICR7

        */
		break;

	case GAH40M_ICRH_C:
		/*

            bit     signal      description

            0       ICR8
            1       ICR9
            2       ICR10
            3       ICR11
            4       ICR12
            5       ICR13
            6       ICR14
            7       ICR15

        */
		break;

	case GAH40M_ICRL_B:
		/*

            bit     signal      description

            0       ICR0
            1       ICR1
            2       ICR2
            3       ICR3
            4       ICR4
            5       ICR5
            6       ICR6
            7       ICR7

        */
		break;

	case GAH40M_ICRH_B:
		/*

            bit     signal      description

            0       ICR8
            1       ICR9
            2       ICR10
            3       ICR11
            4       ICR12
            5       ICR13
            6       ICR14
            7       ICR15

        */
		break;

	case GAH40M_ISR:
		/*

            bit     signal      description

            0       INT0        7508 interrupt (INT 7508)
            1       INT1        82C51 interrupt (INT 82C51)
            2       INT2        6303 interrupt (INT 6303)
            3       INT3        input capture flag timer (CF)
            4       INT4        overflow flag timer (OVF)
            5       INT5        external interrupt (INTEXT)
            6
            7

        */
		break;

	case GAH40M_STR:
		/*

            bit     signal      description

            0       _BANK0
            1       BRDT        barcode reader data timer
            2       RDY         ready (SIO)
            3       RDYSIO      SIO ready (SIO)
            4
            5
            6
            7

        */
		break;

	case GAH40M_SIOR:
		/*

            bit     signal      description

            0       SIO0
            1       SIO1
            2       SIO2
            3       SIO3
            4       SIO4
            5       SIO5
            6       SIO6
            7       SIO7

        */
		break;

	case GAH40M_IVR:
		/*

            bit     description

            0       0
            1       vect 1
            2       vect 2
            3       vect 3
            4       1
            5       1
            6       1
            7       1

        */
		break;
	}

	return 0xff;
}

/*-------------------------------------------------
    gah40m_w - GAH40M write
-------------------------------------------------*/

static WRITE8_HANDLER( gah40m_w )
{
	px8_state *state = (px8_state *)space->machine->driver_data;

	switch (offset)
	{
	case GAH40M_CTLR1:
		/*

            bit     signal      description

            0       _BANK0      bank switching
            1       BCK0        barcode mode select 0 timer (down)
            2       BCK1        barcode mode select 1 timer (up)
            3       SWBCD       barcode reader switch timer
            4       BRG0        bad rate generator select 0 timer
            5       BRG1        bad rate generator select 1 timer
            6       BRG2        bad rate generator select 2 timer
            7       BRG3        bad rate generator select 3 timer

        */

		state->bank0 = BIT(data, 0);
		bankswitch(space->machine);
		break;

	case GAH40M_CMDR:
		/*

            bit     description

            0       set RDYSIOFF (pulse)
            1       reset RDYSIOFF (pulse)
            2       reset OVF (pulse)
            3
            4
            5
            6
            7

        */
		break;

	case GAH40M_CTLR2:
		/*

            bit     signal      description

            0       LED0        LED
            1       LED1        LED
            2       LED2        LED
            3       SWRS        RS-232C switch
            4       INHRS       inhibit RS-232C
            5       AUX         external auxiliary output
            6
            7

        */

		output_set_value("led_0", BIT(data, 0));
		output_set_value("led_1", BIT(data, 1));
		output_set_value("led_2", BIT(data, 2));
		break;

	case GAH40M_IER:
		/*

            bit     signal      description

            0       IER0        INT 7508 enable
            1       IER1        INT 82C51 enable
            2       IER2        INT 6303 enable
            3       IER3        INTICF enable
            4       IER4        INTOVF enable
            5       IER5        INTEXT enable
            6
            7

        */

		state->ier = data;
		break;

	case GAH40M_SIOR:
		/*

            bit     signal      description

            0       SIO0
            1       SIO1
            2       SIO2
            3       SIO3
            4       SIO4
            5       SIO5
            6       SIO6
            7       SIO7

        */

		state->sio = data;
		break;
	}
}

/*-------------------------------------------------
    gah40s_r - GAH40S read
-------------------------------------------------*/

static READ8_HANDLER( gah40s_r )
{
	px8_state *state = (px8_state *)space->machine->driver_data;
	UINT8 data = 0xff;

	switch (offset)
	{
	case 0: /* counter (upper byte) input */
		data = (state->cnt >> 8) & 0x1f;
		break;

	case 1: /* counter (lower byte) */
		data = state->cnt & 0xff;
		break;

	case 3: /* P-ROM read data */
		data = state->prd;
		break;
	}

	return data;
}

/*-------------------------------------------------
    gah40s_w - GAH40S write
-------------------------------------------------*/

static WRITE8_HANDLER( gah40s_w )
{
	px8_state *state = (px8_state *)space->machine->driver_data;

	switch (offset)
	{
	case 0: /* counter reset */
		state->cnt = 0;
		break;

	case 1: /* command register */
		/*

            bit     signal      description

            0       SW PR       PROM power switch
            1       SW MCT      microcassette power switch
            2       MTA         microcassette drive motor control signal A
            3       MTB         microcassette drive motor control signal B
            4       MTC         microcassette drive motor control signal C
            5
            6       _FAST
            7       _STOP CNT

        */

		state->swpr = BIT(data, 0);
		break;

	case 2: /* P-ROM address (upper 8 byte) */
		state->pra = (data << 8) | (state->pra & 0xff);
		state->prd = 0; // TODO read prom!
		break;

	case 3: /* P-ROM address (lower 8 byte) */
		state->pra = (state->pra & 0xff00) | data;
		state->prd = 0; // TODO read prom!
		break;
	}
}

/*-------------------------------------------------
    gah40s_ier_w - interrupt enable register write
-------------------------------------------------*/

static WRITE8_HANDLER( gah40s_ier_w )
{
	px8_state *state = (px8_state *)space->machine->driver_data;

	state->ier = data;
}

/*-------------------------------------------------
    krtn_read - read keyboard return
-------------------------------------------------*/

static UINT8 krtn_read(running_machine *machine)
{
	px8_state *state = (px8_state *)machine->driver_data;
	UINT8 data = 0xff;

	switch (state->ksc)
	{
	case 0: data = input_port_read(machine, "KSC0"); break;
	case 1: data = input_port_read(machine, "KSC1"); break;
	case 2: data = input_port_read(machine, "KSC2"); break;
	case 3: data = input_port_read(machine, "KSC3"); break;
	case 4: data = input_port_read(machine, "KSC4"); break;
	case 5: data = input_port_read(machine, "KSC5"); break;
	case 6: data = input_port_read(machine, "KSC6"); break;
	case 7: data = input_port_read(machine, "KSC7"); break;
	case 8: data = input_port_read(machine, "KSC8"); break;
	case 9: data = input_port_read(machine, "SW4");  break;
	}

	return data;
}

/*-------------------------------------------------
    krtn_0_3_r - keyboard return 0..3 read
-------------------------------------------------*/

static READ8_HANDLER( krtn_0_3_r )
{
	return krtn_read(space->machine) & 0x0f;
}

/*-------------------------------------------------
    krtn_4_7_r - keyboard return 4..7 read
-------------------------------------------------*/

static READ8_HANDLER( krtn_4_7_r )
{
	return krtn_read(space->machine) >> 4;
}

/*-------------------------------------------------
    ksc_w - keyboard scan write
-------------------------------------------------*/

static WRITE8_HANDLER( ksc_w )
{
	px8_state *state = (px8_state *)space->machine->driver_data;

	state->ksc = data;
}

/***************************************************************************
    MEMORY MAPS
***************************************************************************/

/*-------------------------------------------------
    ADDRESS_MAP( px8_mem )
-------------------------------------------------*/

static ADDRESS_MAP_START( px8_mem, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x7fff) AM_RAMBANK("bank0")
	AM_RANGE(0x8000, 0xffff) AM_RAMBANK("bank1")
ADDRESS_MAP_END

/*-------------------------------------------------
    ADDRESS_MAP( px8_io )
-------------------------------------------------*/

static ADDRESS_MAP_START( px8_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0x0f)
	AM_RANGE(0x00, 0x07) AM_READWRITE(gah40m_r, gah40m_w)
	AM_RANGE(0x0c, 0x0c) AM_DEVREADWRITE(I8251_TAG, msm8251_data_r, msm8251_data_w)
	AM_RANGE(0x0d, 0x0d) AM_DEVREADWRITE(I8251_TAG, msm8251_status_r, msm8251_control_w)
	AM_RANGE(0x0e, 0x0e) AM_DEVREADWRITE(SED1320_TAG, sed1330_status_r, sed1330_data_w)
	AM_RANGE(0x0f, 0x0f) AM_DEVREADWRITE(SED1320_TAG, sed1330_data_r, sed1330_command_w)
ADDRESS_MAP_END

/*-------------------------------------------------
    ADDRESS_MAP( px8_slave_mem )
-------------------------------------------------*/

static ADDRESS_MAP_START( px8_slave_mem, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0020, 0x0023) AM_READWRITE(gah40s_r, gah40s_w)
//  AM_RANGE(0x0024, 0x0027) AM_DEVREADWRITE(SED1320_TAG, )
	AM_RANGE(0x0028, 0x0028) AM_WRITE(gah40s_ier_w)
	AM_RANGE(0x8000, 0x97ff) AM_RAM AM_BASE_MEMBER(px8_state, video_ram)
	AM_RANGE(0x9800, 0xefff) AM_NOP
	AM_RANGE(0xf000, 0xffff) AM_ROM AM_REGION(HD6303_TAG, 0) /* internal mask rom */
ADDRESS_MAP_END

/*-------------------------------------------------
    ADDRESS_MAP( px8_slave_io )
-------------------------------------------------*/

static ADDRESS_MAP_START( px8_slave_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_UNMAP_HIGH
//  AM_RANGE(M6803_PORT1, M6803_PORT1)
//  AM_RANGE(M6803_PORT2, M6803_PORT2)
//  AM_RANGE(M6803_PORT3, M6803_PORT3)
//  AM_RANGE(M6803_PORT4, M6803_PORT4)
ADDRESS_MAP_END

/*-------------------------------------------------
    ADDRESS_MAP( px8_sub_io )
-------------------------------------------------*/

static ADDRESS_MAP_START( px8_sub_io, ADDRESS_SPACE_IO, 8 )
//  AM_RANGE(0x00, 0x00) AM_READWRITE()
	AM_RANGE(0x01, 0x01) AM_READ(krtn_0_3_r)
//  AM_RANGE(0x02, 0x02) AM_WRITE()
	AM_RANGE(0x03, 0x03) AM_WRITE(ksc_w)
//  AM_RANGE(0x04, 0x04) AM_WRITE()
	AM_RANGE(0x05, 0x05) AM_READ(krtn_4_7_r)
//  AM_RANGE(0x06, 0x06) AM_READ()
//  AM_RANGE(0x07, 0x07) AM_WRITE()
ADDRESS_MAP_END

/***************************************************************************
    INPUT PORTS
***************************************************************************/

static INPUT_PORTS_START( px8 )
	PORT_START("KSC0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("SCRN INS") PORT_CODE(KEYCODE_INSERT) PORT_CHAR(UCHAR_MAMEKEY(INSERT))
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("DEL") PORT_CODE(KEYCODE_DEL) PORT_CHAR(UCHAR_MAMEKEY(DEL))
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Left CTRL") PORT_CODE(KEYCODE_LCONTROL) PORT_CHAR(UCHAR_MAMEKEY(LCONTROL))
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Left SHIFT") PORT_CODE(KEYCODE_LSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("CAPS LOCK") PORT_CODE(KEYCODE_CAPSLOCK)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Right CTRL") PORT_CODE(KEYCODE_RCONTROL) PORT_CHAR(UCHAR_MAMEKEY(RCONTROL))
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Right SHIFT") PORT_CODE(KEYCODE_RSHIFT)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("NUM GRPH") PORT_CODE(KEYCODE_RALT) PORT_CHAR(UCHAR_MAMEKEY(RALT))

	PORT_START("KSC1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("ESC") PORT_CODE(KEYCODE_F1) PORT_CHAR(UCHAR_MAMEKEY(ESC))
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("PAUSE") PORT_CODE(KEYCODE_F2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("HELP") PORT_CODE(KEYCODE_F3)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("PF1") PORT_CODE(KEYCODE_F4) PORT_CHAR(UCHAR_MAMEKEY(F1))
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("PF2") PORT_CODE(KEYCODE_F5) PORT_CHAR(UCHAR_MAMEKEY(F2))
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("PF3") PORT_CODE(KEYCODE_F6) PORT_CHAR(UCHAR_MAMEKEY(F3))
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("PF4") PORT_CODE(KEYCODE_F7) PORT_CHAR(UCHAR_MAMEKEY(F4))
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("PF5") PORT_CODE(KEYCODE_F8) PORT_CHAR(UCHAR_MAMEKEY(F5))

	PORT_START("KSC2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("STOP") PORT_CODE(KEYCODE_ESC)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('"')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('\'')

	PORT_START("KSC3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Q) PORT_CHAR('q') PORT_CHAR('Q')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_W) PORT_CHAR('w') PORT_CHAR('W')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_E) PORT_CHAR('e') PORT_CHAR('E')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_R) PORT_CHAR('r') PORT_CHAR('R')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_T) PORT_CHAR('t') PORT_CHAR('T')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Y) PORT_CHAR('y') PORT_CHAR('Y')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_U) PORT_CHAR('u') PORT_CHAR('U')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_I) PORT_CHAR('i') PORT_CHAR('I')

	PORT_START("KSC4")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_D) PORT_CHAR('d') PORT_CHAR('D')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F) PORT_CHAR('f') PORT_CHAR('F')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_G) PORT_CHAR('g') PORT_CHAR('G')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_H) PORT_CHAR('h') PORT_CHAR('H')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_J) PORT_CHAR('j') PORT_CHAR('J')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_K) PORT_CHAR('k') PORT_CHAR('K')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_L) PORT_CHAR('l') PORT_CHAR('L')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COLON) PORT_CHAR(';') PORT_CHAR('+')

	PORT_START("KSC5")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_B) PORT_CHAR('b') PORT_CHAR('B')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_N) PORT_CHAR('n') PORT_CHAR('N')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_M) PORT_CHAR('m') PORT_CHAR('M')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR('<')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR('>')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_SLASH) PORT_CHAR('/') PORT_CHAR('?')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F9) PORT_CHAR('[') PORT_CHAR('{')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F10) PORT_CHAR(']') PORT_CHAR('}')

	PORT_START("KSC6")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('(')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR(')')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_0) PORT_CHAR('0') PORT_CHAR('_')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_MINUS) PORT_CHAR('-') PORT_CHAR('=')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_EQUALS) PORT_CHAR('^') PORT_CHAR('~')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x91") PORT_CODE(KEYCODE_UP) PORT_CHAR(UCHAR_MAMEKEY(UP))
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("HOME BS") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("TAB") PORT_CODE(KEYCODE_TAB) PORT_CHAR('\t')

	PORT_START("KSC7")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_O) PORT_CHAR('o') PORT_CHAR('O')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_P) PORT_CHAR('p') PORT_CHAR('P')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR('@') PORT_CHAR(96)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x90") PORT_CODE(KEYCODE_LEFT) PORT_CHAR(UCHAR_MAMEKEY(LEFT))
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x93") PORT_CODE(KEYCODE_DOWN) PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x92") PORT_CODE(KEYCODE_RIGHT) PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_A) PORT_CHAR('a') PORT_CHAR('A')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_S) PORT_CHAR('s') PORT_CHAR('S')

	PORT_START("KSC8")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_QUOTE) PORT_CHAR(':') PORT_CHAR('*')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("RETURN") PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_BACKSLASH) PORT_CHAR('\\') PORT_CHAR('|')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("SPACE") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Z) PORT_CHAR('z') PORT_CHAR('Z')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_X) PORT_CHAR('x') PORT_CHAR('X')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_C) PORT_CHAR('c') PORT_CHAR('C')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_V) PORT_CHAR('v') PORT_CHAR('V')

	PORT_START("SW4")
	PORT_DIPNAME( 0xef, 0x2f, "Character Set" ) PORT_DIPLOCATION("SW4:1,2,3,4,6,7,8")
	PORT_DIPSETTING(    0x2f, "ASCII" )
	PORT_DIPSETTING(    0x2e, DEF_STR( French ) )
	PORT_DIPSETTING(    0x2d, DEF_STR( German ) )
	PORT_DIPSETTING(    0x2c, DEF_STR( English ) )
	PORT_DIPSETTING(    0x2b, "Danish" )
	PORT_DIPSETTING(    0x2a, "Swedish" )
	PORT_DIPSETTING(    0x26, "Norwegian" )
	PORT_DIPSETTING(    0x29, "Italy" )
	PORT_DIPSETTING(    0x28, "Spain" )
	PORT_DIPSETTING(    0x60, "HASCI" )
	PORT_DIPSETTING(    0x21, "Japanese (Japanese)" )
	PORT_DIPSETTING(    0x00, "Japanese (JIS)" )
	PORT_DIPSETTING(    0x01, "Japanese (touch 16)" )
	PORT_DIPNAME( 0x10, 0x00, "RAM disk check" ) PORT_DIPLOCATION("SW4:5")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
INPUT_PORTS_END

/***************************************************************************
    VIDEO
***************************************************************************/

/*-------------------------------------------------
    SED1330_INTERFACE( sed1320_intf )
-------------------------------------------------*/

static READ8_DEVICE_HANDLER( vd_r )
{
	px8_state *state = (px8_state *)device->machine->driver_data;

	return state->video_ram[offset & PX8_VIDEORAM_MASK];
}

static WRITE8_DEVICE_HANDLER( vd_w )
{
	px8_state *state = (px8_state *)device->machine->driver_data;

	state->video_ram[offset & PX8_VIDEORAM_MASK] = data;
}

static SED1330_INTERFACE( sed1320_intf )
{
	SCREEN_TAG,
	DEVCB_HANDLER(vd_r),
	DEVCB_HANDLER(vd_w)
};

/*-------------------------------------------------
    PALETTE_INIT( px8 )
-------------------------------------------------*/

static PALETTE_INIT( px8 )
{
    palette_set_color_rgb(machine, 0, 0xa5, 0xad, 0xa5);
    palette_set_color_rgb(machine, 1, 0x31, 0x39, 0x10);
}

/*-------------------------------------------------
    VIDEO_START( px8 )
-------------------------------------------------*/

static VIDEO_START( px8 )
{
}

/*-------------------------------------------------
    VIDEO_UPDATE( px8 )
-------------------------------------------------*/

static VIDEO_UPDATE( px8 )
{
	px8_state *state = (px8_state *)screen->machine->driver_data;

	sed1330_update(state->sed1320, bitmap, cliprect);

	return 0;
}

/*-------------------------------------------------
    gfx_layout px8_charlayout
-------------------------------------------------*/

static const gfx_layout px8_charlayout =
{
	8, 8,					/* 8 x 8 characters */
	256,					/* 256 characters */
	1,						/* 1 bits per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	/* y offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8					/* every char takes 8 bytes */
};

/*-------------------------------------------------
    GFXDECODE( px8 )
-------------------------------------------------*/

static GFXDECODE_START( px8 )
	GFXDECODE_ENTRY( SED1320_TAG, 0x0000, px8_charlayout, 0, 1 )
GFXDECODE_END

/***************************************************************************
    DEVICE CONFIGURATION
***************************************************************************/

/*-------------------------------------------------
    msm8251_interface i8251_intf
-------------------------------------------------*/

static msm8251_interface i8251_intf =
{
	NULL,
	NULL,
	NULL
};

/*-------------------------------------------------
    cassette_config px8_cassette_config
-------------------------------------------------*/

static const cassette_config px8_cassette_config =
{
	cassette_default_formats,
	NULL,
	(cassette_state)(CASSETTE_STOPPED | CASSETTE_MOTOR_DISABLED | CASSETTE_SPEAKER_ENABLED),
	NULL
};

/***************************************************************************
    MACHINE INITIALIZATION
***************************************************************************/

/*-------------------------------------------------
    MACHINE_START( px8 )
-------------------------------------------------*/

static MACHINE_START( px8 )
{
	px8_state *state = (px8_state *)machine->driver_data;

	/* find devices */
	state->sed1320 = machine->device(SED1320_TAG);
	state->cassette = machine->device(CASSETTE_TAG);

	/* register for state saving */
	state_save_register_global(machine, state->ier);
	state_save_register_global(machine, state->isr);
	state_save_register_global(machine, state->bank0);
	state_save_register_global(machine, state->bk2);
	state_save_register_global(machine, state->sio);
	state_save_register_global(machine, state->ksc);
}

static MACHINE_RESET( px8 )
{
	px8_state *state = (px8_state *)machine->driver_data;

	state->bank0 = 0;
	state->bk2 = 1;

	bankswitch(machine);
}

/***************************************************************************
    MACHINE DRIVERS
***************************************************************************/

static MACHINE_DRIVER_START( px8 )
	MDRV_DRIVER_DATA(px8_state)

	/* main cpu (uPD70008) */
	MDRV_CPU_ADD(UPD70008_TAG, Z80, XTAL_CR1 / 4) /* 2.45 MHz */
	MDRV_CPU_PROGRAM_MAP(px8_mem)
	MDRV_CPU_IO_MAP(px8_io)

    /* slave cpu (HD6303) */
	MDRV_CPU_ADD(HD6303_TAG, M6803, XTAL_CR1 / 4) /* 614 kHz */
	MDRV_CPU_PROGRAM_MAP(px8_slave_mem)
	MDRV_CPU_IO_MAP(px8_slave_io)
	MDRV_DEVICE_DISABLE()

    /* sub CPU (uPD7508) */
//  MDRV_CPU_ADD(UPD7508_TAG, UPD7508, 200000) /* 200 kHz */
//  MDRV_CPU_IO_MAP(px8_sub_io)
//  MDRV_DEVICE_DISABLE()

	MDRV_MACHINE_START(px8)
	MDRV_MACHINE_RESET(px8)

	/* video hardware */
	MDRV_DEFAULT_LAYOUT(layout_px8)

	MDRV_SCREEN_ADD(SCREEN_TAG, LCD)
	MDRV_SCREEN_REFRESH_RATE(72)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(480, 64)
	MDRV_SCREEN_VISIBLE_AREA(0, 479, 0, 63)
	MDRV_GFXDECODE(px8)
	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(px8)
	MDRV_VIDEO_START(px8)
	MDRV_VIDEO_UPDATE(px8)
	MDRV_SED1330_ADD(SED1320_TAG, 0, sed1320_intf)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_WAVE_ADD("wave", CASSETTE_TAG)
	MDRV_SOUND_ROUTE(0, "mono", 0.25)

	/* cartridge */
	MDRV_CARTSLOT_ADD("capsule1")
	MDRV_CARTSLOT_EXTENSION_LIST("bin,rom")

	MDRV_CARTSLOT_ADD("capsule2")
	MDRV_CARTSLOT_EXTENSION_LIST("bin,rom")

	/* devices */
	MDRV_MSM8251_ADD(I8251_TAG, i8251_intf)
	MDRV_CASSETTE_ADD(CASSETTE_TAG, px8_cassette_config)

	/* internal ram */
	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("64K")
MACHINE_DRIVER_END

/***************************************************************************
    ROMS
***************************************************************************/

ROM_START( px8 )
	ROM_REGION( 0x10000, UPD70008_TAG, 0 )
	ROM_DEFAULT_BIOS("052884")
	ROM_SYSTEM_BIOS( 0, "091383", "9/13/83" )
	ROMX_LOAD( "m25030aa.2a", 0x0000, 0x8000, CRC(bd3e4938) SHA1(5bd48abd2a563a1ae31ff137280f40c8f756e969), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "052884", "5/28/84" )
	ROMX_LOAD( "px060688.2a", 0x0000, 0x8000, CRC(44308bdf) SHA1(5c4545fcf1af9931b4699436294d9b6298052a7b), ROM_BIOS(2) )

	ROM_REGION( 0x0800, SED1320_TAG, 0 )
	ROM_LOAD( "font.rom", 0x0000, 0x0800, CRC(5b52edbd) SHA1(38197edf301bb2843bea040536af545f76b3d44f) )

	ROM_REGION( 0x10000, "capsule", 0 )
	ROM_CART_LOAD( "capsule2", 0x0000, 0x8000, ROM_NOMIRROR )
	ROM_CART_LOAD( "capsule1", 0x8000, 0x8000, ROM_NOMIRROR )

	ROM_REGION( 0x1000, HD6303_TAG, 0 )
	ROM_LOAD( "hd6303 slave cpu internal rom.13d", 0x0000, 0x1000, NO_DUMP )

	ROM_REGION( 0x1000, UPD7508_TAG, 0 )
	ROM_LOAD( "upd7508 sub cpu internal rom.2e", 0x0000, 0x1000, NO_DUMP )
ROM_END

/***************************************************************************
    SYSTEM DRIVERS
***************************************************************************/

/*    YEAR  NAME    PARENT  COMPAT  MACHINE INPUT   INIT    COMPANY     FULLNAME    FLAGS */
COMP( 1984, px8,	0,		0,		px8,	px8,	0,		"Epson",	"PX-8",		GAME_NOT_WORKING )

/***************************************************************************

    Epson PX-4

    Note: We are missing a dump of the slave 7508 CPU that controls
    the keyboard and some other things.

***************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "devices/messram.h"
#include "machine/ctronics.h"
#include "devices/cartslot.h"
#include "devices/cassette.h"
#include "machine/tf20.h"
#include "px4.lh"


/***************************************************************************
    CONSTANTS
***************************************************************************/

/* interrupt sources */
#define INT0_7508	0x01
#define INT1_ART	0x02
#define INT2_ICF	0x04
#define INT3_OVF	0x08
#define INT4_EXT	0x10

/* 7508 interrupt sources */
#define UPD7508_INT_ALARM		0x02
#define UPD7508_INT_POWER_FAIL	0x04
#define UPD7508_INT_7508_RESET	0x08
#define UPD7508_INT_Z80_RESET	0x10
#define UPD7508_INT_ONE_SECOND	0x20


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _px4_state px4_state;
struct _px4_state
{
	/* internal ram */
	const device_config *ram;

	/* gapnit register */
	UINT8 ctrl1;
	UINT16 icrb;
	UINT8 bankr;
	UINT8 isr;
	UINT8 ier;
	UINT8 str;
	UINT8 sior;

	/* gapnit internal */
	UINT16 frc_value;
	UINT16 frc_latch;

	/* gapndi register */
	UINT8 vadr;
	UINT8 yoff;

	/* 7508 internal */
	int one_sec_int_enabled;
	int alarm_int_enabled;
	int key_int_enabled;

	UINT8 key_status;
	UINT8 interrupt_status;

	/* external ramdisk */
	offs_t ramdisk_address;
	UINT8 *ramdisk;

	/* external cassette/barcode reader */
	const device_config *ext_cas;
	emu_timer *ext_cas_timer;
	int ear_last_state;
};


/***************************************************************************
    GAPNIT
***************************************************************************/

/* process interrupts */
static void gapnit_interrupt(running_machine *machine)
{
	px4_state *px4 = machine->driver_data;

	/* any interrupts enabled and pending? */
	if (px4->ier & px4->isr & INT0_7508)
	{
		px4->isr &= ~INT0_7508;
		cputag_set_input_line_and_vector(machine, "maincpu", 0, ASSERT_LINE, 0xf0);
	}
	else if (px4->ier & px4->isr & INT1_ART)
		cputag_set_input_line_and_vector(machine, "maincpu", 0, ASSERT_LINE, 0xf2);
	else if (px4->ier & px4->isr & INT2_ICF)
		cputag_set_input_line_and_vector(machine, "maincpu", 0, ASSERT_LINE, 0xf4);
	else if (px4->ier & px4->isr & INT3_OVF)
		cputag_set_input_line_and_vector(machine, "maincpu", 0, ASSERT_LINE, 0xf6);
	else if (px4->ier & px4->isr & INT4_EXT)
		cputag_set_input_line_and_vector(machine, "maincpu", 0, ASSERT_LINE, 0xf8);
	else
		cputag_set_input_line(machine, "maincpu", 0, CLEAR_LINE);
}

/* external cassette or barcode reader input */
static TIMER_CALLBACK( ext_cassette_read )
{
	px4_state *px4 = machine->driver_data;
	UINT8 result;
	int trigger = 0;

	/* sample input state */
	result = cassette_input(px4->ext_cas) > 0 ? 1 : 0;

	/* detect transition */
	switch ((px4->ctrl1 >> 1) & 0x03)
	{
	case 0: /* trigger inhibit */
		trigger = 0;
		break;
	case 1: /* falling edge trigger */
		trigger = px4->ear_last_state == 1 && result == 0;
		break;
	case 2: /* rising edge trigger */
		trigger = px4->ear_last_state == 0 && result == 1;
		break;
	case 3: /* rising/falling edge trigger */
		trigger = px4->ear_last_state != result;
		break;
	}

	/* generate an interrupt if we need to trigger */
	if (trigger)
	{
		px4->icrb = px4->frc_value;
		px4->isr |= INT2_ICF;
		gapnit_interrupt(machine);
	}

	/* save last state */
	px4->ear_last_state = result;
}

/* free running counter */
static TIMER_DEVICE_CALLBACK( frc_tick )
{
	px4_state *px4 = timer->machine->driver_data;

	px4->frc_value++;

	if (px4->frc_value == 0)
	{
		px4->isr |= INT3_OVF;
		gapnit_interrupt(timer->machine);
	}
}

/* input capture register low command trigger */
static READ8_HANDLER( px4_icrlc_r )
{
	px4_state *px4 = space->machine->driver_data;
	logerror("%s: px4_icrlc_r\n", cpuexec_describe_context(space->machine));

	/* latch value */
	px4->frc_latch = px4->frc_value;

	return px4->frc_latch & 0xff;
}

/* control register 1 */
static WRITE8_HANDLER( px4_ctrl1_w )
{
	px4_state *px4 = space->machine->driver_data;

	logerror("%s: px4_ctrl1_w (0x%02x)\n", cpuexec_describe_context(space->machine), data);

	px4->ctrl1 = data;
}

/* input capture register high command trigger */
static READ8_HANDLER( px4_icrhc_r )
{
	px4_state *px4 = space->machine->driver_data;

	logerror("%s: px4_icrhc_r\n", cpuexec_describe_context(space->machine));

	return (px4->frc_latch >> 8) & 0xff;
}

/* command register */
static WRITE8_HANDLER( px4_cmdr_w )
{
	px4_state *px4 = space->machine->driver_data;

	logerror("%s: px4_cmdr_w (0x%02x)\n", cpuexec_describe_context(space->machine), data);

	/* clear overflow interrupt? */
	if (BIT(data, 2))
	{
		px4->isr &= ~INT3_OVF;
		gapnit_interrupt(space->machine);
	}
}

/* input capture register low barcode trigger */
static READ8_HANDLER( px4_icrlb_r )
{
	px4_state *px4 = space->machine->driver_data;

	logerror("%s: px4_icrlb_r\n", cpuexec_describe_context(space->machine));

	return px4->icrb & 0xff;
}

/* control register 2 */
static WRITE8_HANDLER( px4_ctrl2_w )
{
	px4_state *px4 = space->machine->driver_data;

	logerror("%s: px4_ctrl2_w (0x%02x)\n", cpuexec_describe_context(space->machine), data);

	/* bit 0, MIC, cassette output */
	cassette_output(px4->ext_cas, BIT(data, 0) ? -1.0 : +1.0);

	/* bit 1, RMT, cassette motor */
	if (BIT(data, 1))
	{
		cassette_change_state(px4->ext_cas, CASSETTE_MOTOR_ENABLED, CASSETTE_MASK_MOTOR);
		timer_adjust_periodic(px4->ext_cas_timer, attotime_zero, 0, ATTOTIME_IN_HZ(44100));
	}
	else
	{
		cassette_change_state(px4->ext_cas, CASSETTE_MOTOR_DISABLED, CASSETTE_MASK_MOTOR);
		timer_adjust_oneshot(px4->ext_cas_timer, attotime_zero, 0);
	}
}

/* input capture register high barcode trigger */
static READ8_HANDLER( px4_icrhb_r )
{
	px4_state *px4 = space->machine->driver_data;
	logerror("%s: px4_icrhb_r\n", cpuexec_describe_context(space->machine));

	/* clear icf interrupt */
	px4->isr &= ~INT2_ICF;
	gapnit_interrupt(space->machine);

	return (px4->icrb >> 8) & 0xff;
}

/* interrupt status register */
static READ8_HANDLER( px4_isr_r )
{
	px4_state *px4 = space->machine->driver_data;
	logerror("%s: px4_isr_r\n", cpuexec_describe_context(space->machine));

	return px4->isr;
}

/* interrupt enable register */
static WRITE8_HANDLER( px4_ier_w )
{
	px4_state *px4 = space->machine->driver_data;
	logerror("%s: px4_ier_w (0x%02x)\n", cpuexec_describe_context(space->machine), data);

	px4->ier = data;
	gapnit_interrupt(space->machine);
}

/* status register */
static READ8_HANDLER( px4_str_r )
{
	px4_state *px4 = space->machine->driver_data;
	UINT8 result = 0;

	logerror("%s: px4_str_r\n", cpuexec_describe_context(space->machine));

	/* bit 0, EAR, cassette input */
	result |= cassette_input(px4->ext_cas) > 0 ? 1 : 0;

	/* bit 1, BCRD, barcode reader input */
	result |= 1 << 1;

	/* bit 2, RDY signal from 7805 */
	result |= 1 << 2;

	/* bit 3, RDYSIO, enable access to the 7805 */
	result |= 1 << 3;

	/* bit 4-7, BANK, memory bank */
	result |= px4->bankr & 0xf0;

	return result;
}

/* helper function to map rom capsules */
static void install_rom_capsule(const address_space *space, int size, const char *region)
{
	px4_state *px4 = space->machine->driver_data;

	/* ram, part 1 */
	memory_install_readwrite8_handler(space, 0x0000, 0xdfff - size, 0, 0, SMH_BANK(1), SMH_BANK(1));
	memory_set_bankptr(space->machine, 1, messram_get_ptr(px4->ram));

	/* actual rom data, part 1 */
	memory_install_readwrite8_handler(space, 0xe000 - size, 0xffff - size, 0, 0, SMH_BANK(2), SMH_NOP);
	memory_set_bankptr(space->machine, 2, memory_region(space->machine, region) + (size - 0x2000));

	/* rom data, part 2 */
	if (size != 0x2000)
	{
		memory_install_readwrite8_handler(space, 0x10000 - size, 0xdfff, 0, 0, SMH_BANK(3), SMH_NOP);
		memory_set_bankptr(space->machine, 3, memory_region(space->machine, region));
	}

	/* ram, continued */
	memory_install_readwrite8_handler(space, 0xe000, 0xffff, 0, 0, SMH_BANK(4), SMH_BANK(4));
	memory_set_bankptr(space->machine, 4, messram_get_ptr(px4->ram) + 0xe000);
}

/* bank register */
static WRITE8_HANDLER( px4_bankr_w )
{
	px4_state *px4 = space->machine->driver_data;
	const address_space *space_program = cputag_get_address_space(space->machine, "maincpu", ADDRESS_SPACE_PROGRAM);

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
		memory_set_bankptr(space->machine, 2, messram_get_ptr(px4->ram) + 0x8000);
		break;

	case 0x04:
		/* memory */
		memory_install_readwrite8_handler(space_program, 0x0000, 0xffff, 0, 0, SMH_BANK(1), SMH_BANK(1));
		memory_set_bankptr(space->machine, 1, messram_get_ptr(px4->ram));
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
	px4_state *px4 = space->machine->driver_data;
	logerror("%s: px4_sior_r\n", cpuexec_describe_context(space->machine));
	logerror("sior = 0x%02x\n", px4->sior);

	return px4->sior;
}

/* serial io register */
static WRITE8_HANDLER( px4_sior_w )
{
	px4_state *px4 = space->machine->driver_data;
	logerror("%s: px4_sior_w (0x%02x)\n", cpuexec_describe_context(space->machine), data);

	px4->sior = data;

	switch (data)
	{
	case 0x01: logerror("7508 cmd: Power OFF\n"); break;

	case 0x02:
		logerror("7508 cmd: Read Status\n");

		if (px4->interrupt_status != 0)
		{
			logerror("> 7508 has interrupts pending: 0x%02x\n", px4->interrupt_status);

			/* signal the interrupt(s) */
			px4->sior = 0xc1 | px4->interrupt_status;
			px4->interrupt_status = 0x00;
		}
		else if (px4->key_status != 0xff)
		{
			px4->sior = px4->key_status;
			px4->key_status = 0xff;
		}
		else
		{
			/* nothing happenend */
			px4->sior = 0xbf;
		}

		break;

	case 0x03: logerror("7508 cmd: KB Reset\n"); break;
	case 0x04: logerror("7508 cmd: KB Repeat Timer 1 Set\n"); break;
	case 0x14: logerror("7508 cmd: KB Repeat Timer 2 Set\n"); break;
	case 0x24: logerror("7508 cmd: KB Repeat Timer 1 Read\n"); break;
	case 0x34: logerror("7508 cmd: KB Repeat Timer 2 Read\n"); break;
	case 0x05: logerror("7508 cmd: KB Repeat OFF\n"); break;
	case 0x15: logerror("7508 cmd: KB Repeat ON\n"); break;

	case 0x06:
		logerror("7508 cmd: KB Interrupt OFF\n");
		px4->key_int_enabled = FALSE;
		break;

	case 0x16:
		logerror("7508 cmd: KB Interrupt ON\n");
		px4->key_int_enabled = TRUE;
		break;

	case 0x07: logerror("7508 cmd: Clock Read\n"); break;
	case 0x17: logerror("7508 cmd: Clock Write\n"); break;

	case 0x08:
		logerror("7508 cmd: Power Switch Read\n");

		/* indicate that the power switch is in the "ON" position */
		px4->sior = 0x01;
		break;

	case 0x09: logerror("7508 cmd: Alarm Read\n"); break;
	case 0x19: logerror("7508 cmd: Alarm Set\n"); break;
	case 0x29: logerror("7508 cmd: Alarm OFF\n"); break;
	case 0x39: logerror("7508 cmd: Alarm ON\n"); break;

	case 0x0a:
		logerror("7508 cmd: DIP Switch Read\n");
		px4->sior = input_port_read(space->machine, "dips");
		break;

	case 0x0b: logerror("7508 cmd: Stop Key Interrupt disable\n"); break;
	case 0x1b: logerror("7508 cmd: Stop Key Interrupt enable\n"); break;
	case 0x0c: logerror("7508 cmd: 7 chr. Buffer\n"); break;
	case 0x1c: logerror("7508 cmd: 1 chr. Buffer\n"); break;

	case 0x0d:
		logerror("7508 cmd: 1 sec. Interrupt OFF\n");
		px4->one_sec_int_enabled = FALSE;
		break;

	case 0x1d:
		logerror("7508 cmd: 1 sec. Interrupt ON\n");
		px4->one_sec_int_enabled = TRUE;
		break;

	case 0x0e:
		logerror("7508 cmd: KB Clear\n");
		px4->sior = 0xbf;
		break;

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
	return 0x05;
}

/* art mode register */
static WRITE8_HANDLER( px4_artmr_w )
{
	logerror("%s: px4_artmr_w (0x%02x)\n", cpuexec_describe_context(space->machine), data);
}

/* io status register */
static READ8_HANDLER( px4_iostr_r )
{
	const device_config *printer = devtag_get_device(space->machine, "centronics");
	int result = 0;

	logerror("%s: px4_iostr_r\n", cpuexec_describe_context(space->machine));

	/* centronics status */
	result |= centronics_busy_r(printer);
	result |= !centronics_pe_r(printer) << 1;

	/* cartridge option select signal, set to 'other mode' */
	result |= 0x40;

	return result;
}

/* art command register */
static WRITE8_HANDLER( px4_artcr_w )
{
	logerror("%s: px4_artcr_w (0x%02x)\n", cpuexec_describe_context(space->machine), data);
}

/* switch register */
static WRITE8_HANDLER( px4_swr_w )
{
	logerror("%s: px4_swr_w (0x%02x)\n", cpuexec_describe_context(space->machine), data);
}

/* io control register */
static WRITE8_HANDLER( px4_ioctlr_w )
{
	const device_config *printer = devtag_get_device(space->machine, "centronics");

	logerror("%s: px4_ioctlr_w (0x%02x)\n", cpuexec_describe_context(space->machine), data);

	/* centronics strobe and reset */
	centronics_strobe_w(printer, !BIT(data, 0));
	centronics_prime_w(printer, BIT(data, 1));

	/* status leds */
	output_set_value("led_0", BIT(data, 4)); /* caps lock */
	output_set_value("led_1", BIT(data, 5)); /* num lock */
	output_set_value("led_2", BIT(data, 6));
}


/***************************************************************************
    7508 RELATED
***************************************************************************/

static TIMER_DEVICE_CALLBACK( upd7508_1sec_callback )
{
	px4_state *px4 = timer->machine->driver_data;

	/* adjust interrupt status */
	px4->interrupt_status |= UPD7508_INT_ONE_SECOND;

	/* are interrupts enabled? */
	if (px4->one_sec_int_enabled)
	{
		px4->isr |= INT0_7508;
		gapnit_interrupt(timer->machine);
	}
}

static INPUT_CHANGED( key_callback )
{
	px4_state *px4 = field->port->machine->driver_data;
	UINT32 oldvalue = oldval * field->mask, newvalue = newval * field->mask;
	UINT32 delta = oldvalue ^ newvalue;
	int i, scancode = 0xff, down = 0;

	for (i = 0; i < 32; i++)
	{
		if (delta & (1 << i))
		{
			down = (newvalue & (1 << i)) ? 0x10 : 0x00;
			scancode = (FPTR)param * 32 + i;

			/* control keys */
			if ((scancode & 0xa0) == 0xa0)
				scancode |= down;

			logerror("upd7508: key callback, key=0x%02x\n", scancode);

			break;
		}
	}

	if (down || (scancode & 0xa0) == 0xa0)
	{
		px4->key_status = scancode;

		if (px4->key_int_enabled)
		{
			logerror("upd7508: key interrupt\n");
			px4->isr |= INT0_7508;
			gapnit_interrupt(field->port->machine);
		}
	}
}


/***************************************************************************
    EXTERNAL RAM-DISK
***************************************************************************/

static WRITE8_HANDLER( px4_ramdisk_address_w )
{
	px4_state *px4 = space->machine->driver_data;

	switch (offset)
	{
	case 0x00: px4->ramdisk_address = (px4->ramdisk_address & 0xffff00) | data; break;
	case 0x01: px4->ramdisk_address = (px4->ramdisk_address & 0xff00ff) | (data << 8); break;
	case 0x02: px4->ramdisk_address = (px4->ramdisk_address & 0x00ffff) | ((data & 0x07) << 16); break;
	}
}

static READ8_HANDLER( px4_ramdisk_data_r )
{
	px4_state *px4 = space->machine->driver_data;
	UINT8 ret = 0xff;

	if (px4->ramdisk_address < 0x20000)
	{
		/* read from ram */
		ret = px4->ramdisk[px4->ramdisk_address];
	}
	else if (px4->ramdisk_address < 0x40000)
	{
		/* read from rom */
		ret = memory_region(space->machine, "ramdisk")[px4->ramdisk_address];
	}

	px4->ramdisk_address = (px4->ramdisk_address & 0xffff00) | ((px4->ramdisk_address & 0xff) + 1);

	return ret;
}

static WRITE8_HANDLER( px4_ramdisk_data_w )
{
	px4_state *px4 = space->machine->driver_data;

	if (px4->ramdisk_address < 0x20000)
		px4->ramdisk[px4->ramdisk_address] = data;

	px4->ramdisk_address = (px4->ramdisk_address & 0xffff00) | ((px4->ramdisk_address & 0xff) + 1);
}

static READ8_HANDLER( px4_ramdisk_control_r )
{
	/* bit 7 determines the presence of a ram-disk */
	return 0x7f;
}

static NVRAM_HANDLER( px4_ramdisk )
{
	px4_state *px4 = machine->driver_data;

	if (read_or_write)
		mame_fwrite(file, px4->ramdisk, 0x20000);
	else
		if (file)
			mame_fread(file, px4->ramdisk, 0x20000);
}


/***************************************************************************
    VIDEO EMULATION
***************************************************************************/

static VIDEO_UPDATE( px4 )
{
	px4_state *px4 = screen->machine->driver_data;

	/* display enabled? */
	if (BIT(px4->yoff, 7))
	{
		int y, x;

		/* get vram start address */
		UINT8 *vram = &messram_get_ptr(px4->ram)[(px4->vadr & 0xf8) << 8];

		for (y = 0; y < 64; y++)
		{
			/* adjust against y-offset */
			UINT8 row = (y - (px4->yoff & 0x3f)) & 0x3f;

			for (x = 0; x < 240/8; x++)
			{
				*BITMAP_ADDR16(bitmap, row, x * 8 + 0) = BIT(*vram, 7);
				*BITMAP_ADDR16(bitmap, row, x * 8 + 1) = BIT(*vram, 6);
				*BITMAP_ADDR16(bitmap, row, x * 8 + 2) = BIT(*vram, 5);
				*BITMAP_ADDR16(bitmap, row, x * 8 + 3) = BIT(*vram, 4);
				*BITMAP_ADDR16(bitmap, row, x * 8 + 4) = BIT(*vram, 3);
				*BITMAP_ADDR16(bitmap, row, x * 8 + 5) = BIT(*vram, 2);
				*BITMAP_ADDR16(bitmap, row, x * 8 + 6) = BIT(*vram, 1);
				*BITMAP_ADDR16(bitmap, row, x * 8 + 7) = BIT(*vram, 0);

				vram++;
			}

			/* skip the last 2 unused bytes */
			vram += 2;
		}
	}
	else
	{
		/* display is disabled, draw an empty screen */
		bitmap_fill(bitmap, cliprect, 0);
	}

	return 0;
}


/***************************************************************************
    DRIVER INIT
***************************************************************************/

static DRIVER_INIT( px4 )
{
	px4_state *px4 = machine->driver_data;

	/* find devices */
	px4->ram = devtag_get_device(machine, "messram");

	/* init 7508 */
	px4->one_sec_int_enabled = TRUE;
	px4->key_int_enabled = TRUE;
	px4->alarm_int_enabled = TRUE;

	/* external cassette or barcode reader */
	px4->ext_cas_timer = timer_alloc(machine, ext_cassette_read, NULL);
	px4->ext_cas = devtag_get_device(machine, "extcas");
	px4->ear_last_state = 0;

	/* map os rom and last half of memory */
	memory_set_bankptr(machine, 1, memory_region(machine, "os"));
	memory_set_bankptr(machine, 2, messram_get_ptr(px4->ram) + 0x8000);
}

static DRIVER_INIT( px4p )
{
	px4_state *px4 = machine->driver_data;

	DRIVER_INIT_CALL(px4);

	/* reserve memory for external ram-disk */
	px4->ramdisk = auto_alloc_array(machine, UINT8, 0x20000);
}


/***************************************************************************
    ADDRESS MAPS
***************************************************************************/

static ADDRESS_MAP_START( px4_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROMBANK(1)
	AM_RANGE(0x8000, 0xffff) AM_RAMBANK(2)
ADDRESS_MAP_END

static ADDRESS_MAP_START( px4_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_UNMAP_HIGH
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
	AM_RANGE(0x17, 0x17) AM_DEVWRITE("centronics", centronics_data_w)
	AM_RANGE(0x18, 0x18) AM_WRITE(px4_swr_w)
	AM_RANGE(0x19, 0x19) AM_WRITE(px4_ioctlr_w)
	AM_RANGE(0x1a, 0x1f) AM_NOP
ADDRESS_MAP_END

static ADDRESS_MAP_START( px4p_io, ADDRESS_SPACE_IO, 8 )
	AM_IMPORT_FROM(px4_io)
	AM_RANGE(0x90, 0x92) AM_WRITE(px4_ramdisk_address_w)
	AM_RANGE(0x93, 0x93) AM_READWRITE(px4_ramdisk_data_r, px4_ramdisk_data_w)
	AM_RANGE(0x94, 0x94) AM_READ(px4_ramdisk_control_r)
ADDRESS_MAP_END


/***************************************************************************
    INPUT PORTS
***************************************************************************/

/* The PX-4 has an exchangable keyboard. Available is a standard ASCII
 * keyboard and an "item" keyboard, as well as regional variants for
 * UK, France, Germany, Denmark, Sweden, Norway, Italy and Spain.
 */

/* configuration dip switch found on the rom capsule board */
static INPUT_PORTS_START( px4_dips )
	PORT_START("dips")

	PORT_DIPNAME(0x0f, 0x0f, "Character set")
	PORT_DIPLOCATION("DIP:8,7,6,5")
	PORT_DIPSETTING(0x0f, "ASCII")
	PORT_DIPSETTING(0x0e, "France")
	PORT_DIPSETTING(0x0d, "Germany")
	PORT_DIPSETTING(0x0c, "England")
	PORT_DIPSETTING(0x0b, "Denmark")
	PORT_DIPSETTING(0x0a, "Sweden")
	PORT_DIPSETTING(0x09, "Italy")
	PORT_DIPSETTING(0x08, "Spain")
	PORT_DIPSETTING(0x07, DEF_STR(Japan))
	PORT_DIPSETTING(0x06, "Norway")

	PORT_DIPNAME(0x30, 0x30, "LST device")
	PORT_DIPLOCATION("DIP:4,3")
	PORT_DIPSETTING(0x00, "SIO")
	PORT_DIPSETTING(0x10, "Cartridge printer")
	PORT_DIPSETTING(0x20, "RS-232C")
	PORT_DIPSETTING(0x30, "Centronics printer")

	/* available for user applications */
	PORT_DIPNAME(0x40, 0x40, "Not used")
	PORT_DIPLOCATION("DIP:2")
	PORT_DIPSETTING(0x40, "Enable")
	PORT_DIPSETTING(0x00, "Disable")

	/* this is automatically selected by the os, the switch has no effect */
	PORT_DIPNAME(0x80, 0x00, "Keyboard type")
	PORT_DIPLOCATION("DIP:1")
	PORT_DIPSETTING(0x80, "Item keyboard")
	PORT_DIPSETTING(0x00, "Standard keyboard")
INPUT_PORTS_END

/* US ASCII keyboard */
static INPUT_PORTS_START( px4_h450a )
	PORT_INCLUDE(px4_dips)

	PORT_START("keyboard_0")
	PORT_BIT(0x00000001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(key_callback, (void *)0) PORT_CODE(KEYCODE_F1) PORT_CHAR(UCHAR_MAMEKEY(ESC))	// 00
	PORT_BIT(0x00000002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(key_callback, (void *)0) PORT_CODE(KEYCODE_F2) PORT_CHAR(UCHAR_MAMEKEY(PAUSE))	// 01
	PORT_BIT(0x00000004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(key_callback, (void *)0) PORT_CODE(KEYCODE_F3) PORT_CHAR(UCHAR_MAMEKEY(F6))    PORT_NAME("Help")	// 02
	PORT_BIT(0x00000008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(key_callback, (void *)0) PORT_CODE(KEYCODE_F4) PORT_CHAR(UCHAR_MAMEKEY(F1))    PORT_NAME("PF1")	// 03
	PORT_BIT(0x00000010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(key_callback, (void *)0) PORT_CODE(KEYCODE_F5) PORT_CHAR(UCHAR_MAMEKEY(F2))    PORT_NAME("PF2")	// 04
	PORT_BIT(0x00000020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(key_callback, (void *)0) PORT_CODE(KEYCODE_F6) PORT_CHAR(UCHAR_MAMEKEY(F3))    PORT_NAME("PF3")	// 05
	PORT_BIT(0x00000040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(key_callback, (void *)0) PORT_CODE(KEYCODE_F7) PORT_CHAR(UCHAR_MAMEKEY(F4))    PORT_NAME("PF4")	// 06
	PORT_BIT(0x00000080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(key_callback, (void *)0) PORT_CODE(KEYCODE_F8) PORT_CHAR(UCHAR_MAMEKEY(F5))    PORT_NAME("PF5")	// 07
	PORT_BIT(0x0000ff00, IP_ACTIVE_HIGH, IPT_UNUSED)	// 08-0f
	PORT_BIT(0x00010000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(key_callback, (void *)0) PORT_CODE(KEYCODE_ESC)	PORT_CHAR(UCHAR_MAMEKEY(CANCEL)) PORT_NAME("Stop")	// 10
	PORT_BIT(0x00020000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(key_callback, (void *)0) PORT_CODE(KEYCODE_1)   PORT_CHAR('1') PORT_CHAR('!')	// 11
	PORT_BIT(0x00040000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(key_callback, (void *)0) PORT_CODE(KEYCODE_2)   PORT_CHAR('2') PORT_CHAR('"')	// 12
	PORT_BIT(0x00080000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(key_callback, (void *)0) PORT_CODE(KEYCODE_3)   PORT_CHAR('3') PORT_CHAR('#')	// 13
	PORT_BIT(0x00100000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(key_callback, (void *)0) PORT_CODE(KEYCODE_4)   PORT_CHAR('4') PORT_CHAR('$')	// 14
	PORT_BIT(0x00200000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(key_callback, (void *)0) PORT_CODE(KEYCODE_5)   PORT_CHAR('5') PORT_CHAR('%')	// 15
	PORT_BIT(0x00400000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(key_callback, (void *)0) PORT_CODE(KEYCODE_6)   PORT_CHAR('6') PORT_CHAR('&')	// 16
	PORT_BIT(0x00800000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(key_callback, (void *)0) PORT_CODE(KEYCODE_7)   PORT_CHAR('7') PORT_CHAR('\'')	// 17
	PORT_BIT(0xff000000, IP_ACTIVE_HIGH, IPT_UNUSED)	// 18-1f

	PORT_START("keyboard_1")
	PORT_BIT(0x00000001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(key_callback, (void *)1) PORT_CODE(KEYCODE_Q) PORT_CHAR('q') PORT_CHAR('Q')	// 20
	PORT_BIT(0x00000002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(key_callback, (void *)1) PORT_CODE(KEYCODE_W) PORT_CHAR('w') PORT_CHAR('W')	// 21
	PORT_BIT(0x00000004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(key_callback, (void *)1) PORT_CODE(KEYCODE_E) PORT_CHAR('e') PORT_CHAR('E')	// 22
	PORT_BIT(0x00000008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(key_callback, (void *)1) PORT_CODE(KEYCODE_R) PORT_CHAR('r') PORT_CHAR('R')	// 23
	PORT_BIT(0x00000010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(key_callback, (void *)1) PORT_CODE(KEYCODE_T) PORT_CHAR('t') PORT_CHAR('T')	// 24
	PORT_BIT(0x00000020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(key_callback, (void *)1) PORT_CODE(KEYCODE_Y) PORT_CHAR('y') PORT_CHAR('Y')	// 25
	PORT_BIT(0x00000040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(key_callback, (void *)1) PORT_CODE(KEYCODE_U) PORT_CHAR('u') PORT_CHAR('U')	// 26
	PORT_BIT(0x00000080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(key_callback, (void *)1) PORT_CODE(KEYCODE_I) PORT_CHAR('i') PORT_CHAR('I')	// 27
	PORT_BIT(0x0000ff00, IP_ACTIVE_HIGH, IPT_UNUSED)	// 28-2f
	PORT_BIT(0x00010000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(key_callback, (void *)1) PORT_CODE(KEYCODE_D)     PORT_CHAR('d') PORT_CHAR('D')	// 30
	PORT_BIT(0x00020000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(key_callback, (void *)1) PORT_CODE(KEYCODE_F)     PORT_CHAR('f') PORT_CHAR('F')	// 31
	PORT_BIT(0x00040000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(key_callback, (void *)1) PORT_CODE(KEYCODE_G)     PORT_CHAR('g') PORT_CHAR('G')	// 32
	PORT_BIT(0x00080000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(key_callback, (void *)1) PORT_CODE(KEYCODE_H)     PORT_CHAR('h') PORT_CHAR('H')	// 33
	PORT_BIT(0x00100000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(key_callback, (void *)1) PORT_CODE(KEYCODE_J)     PORT_CHAR('j') PORT_CHAR('J')	// 34
	PORT_BIT(0x00200000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(key_callback, (void *)1) PORT_CODE(KEYCODE_K)     PORT_CHAR('k') PORT_CHAR('K')	// 35
	PORT_BIT(0x00400000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(key_callback, (void *)1) PORT_CODE(KEYCODE_L)     PORT_CHAR('l') PORT_CHAR('L')	// 36
	PORT_BIT(0x00800000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(key_callback, (void *)1) PORT_CODE(KEYCODE_COLON) PORT_CHAR(';') PORT_CHAR('+')	// 37
	PORT_BIT(0xff000000, IP_ACTIVE_HIGH, IPT_UNUSED)	// 38-3f

	PORT_START("keyboard_2")
	PORT_BIT(0x00000001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(key_callback, (void *)2) PORT_CODE(KEYCODE_B)     PORT_CHAR('b') PORT_CHAR('B')	// 40
	PORT_BIT(0x00000002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(key_callback, (void *)2) PORT_CODE(KEYCODE_N)     PORT_CHAR('n') PORT_CHAR('N')	// 41
	PORT_BIT(0x00000004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(key_callback, (void *)2) PORT_CODE(KEYCODE_M)     PORT_CHAR('m') PORT_CHAR('M')	// 42
	PORT_BIT(0x00000008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(key_callback, (void *)2) PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR('<')	// 43
	PORT_BIT(0x00000010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(key_callback, (void *)2) PORT_CODE(KEYCODE_STOP)  PORT_CHAR('.') PORT_CHAR('>')	// 44
	PORT_BIT(0x00000020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(key_callback, (void *)2) PORT_CODE(KEYCODE_SLASH) PORT_CHAR('/') PORT_CHAR('?')	// 45
	PORT_BIT(0x00000040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(key_callback, (void *)2) PORT_CODE(KEYCODE_F9)    PORT_CHAR('[') PORT_CHAR('{')	// 46
	PORT_BIT(0x00000080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(key_callback, (void *)2) PORT_CODE(KEYCODE_F10)   PORT_CHAR(']') PORT_CHAR('}')	// 47
	PORT_BIT(0x0000ff00, IP_ACTIVE_HIGH, IPT_UNUSED)	// 48-4f
	PORT_BIT(0x00010000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(key_callback, (void *)2) PORT_CODE(KEYCODE_8)         PORT_CHAR('8') PORT_CHAR('(')	// 50
	PORT_BIT(0x00020000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(key_callback, (void *)2) PORT_CODE(KEYCODE_9)         PORT_CHAR('9') PORT_CHAR(')')	// 51
	PORT_BIT(0x00040000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(key_callback, (void *)2) PORT_CODE(KEYCODE_0)         PORT_CHAR('0') PORT_CHAR('_')	// 52
	PORT_BIT(0x00080000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(key_callback, (void *)2) PORT_CODE(KEYCODE_MINUS)     PORT_CHAR('-') PORT_CHAR('=')	// 53
	PORT_BIT(0x00100000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(key_callback, (void *)2) PORT_CODE(KEYCODE_EQUALS)    PORT_CHAR('^') PORT_CHAR('~')	// 54
	PORT_BIT(0x00200000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(key_callback, (void *)2) PORT_CODE(KEYCODE_UP)        PORT_CHAR(UCHAR_MAMEKEY(UP))	// 55
	PORT_BIT(0x00400000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(key_callback, (void *)2) PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8)   PORT_CHAR(UCHAR_MAMEKEY(HOME))	// 56
	PORT_BIT(0x00800000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(key_callback, (void *)2) PORT_CODE(KEYCODE_TAB)       PORT_CHAR('\t')	// 57
	PORT_BIT(0xff000000, IP_ACTIVE_HIGH, IPT_UNUSED)	// 58-5f

	PORT_START("keyboard_3")
	PORT_BIT(0x00000001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(key_callback, (void *)3) PORT_CODE(KEYCODE_O)         PORT_CHAR('o') PORT_CHAR('O')	// 60
	PORT_BIT(0x00000002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(key_callback, (void *)3) PORT_CODE(KEYCODE_P)         PORT_CHAR('p') PORT_CHAR('P')	// 61
	PORT_BIT(0x00000004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(key_callback, (void *)3) PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR('@') PORT_CHAR(96)	// 62
	PORT_BIT(0x00000008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(key_callback, (void *)3) PORT_CODE(KEYCODE_LEFT)      PORT_CHAR(UCHAR_MAMEKEY(LEFT))	// 63
	PORT_BIT(0x00000010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(key_callback, (void *)3) PORT_CODE(KEYCODE_DOWN)      PORT_CHAR(UCHAR_MAMEKEY(DOWN))	// 64
	PORT_BIT(0x00000020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(key_callback, (void *)3) PORT_CODE(KEYCODE_RIGHT)     PORT_CHAR(UCHAR_MAMEKEY(RIGHT))	// 65
	PORT_BIT(0x00000040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(key_callback, (void *)3) PORT_CODE(KEYCODE_A)         PORT_CHAR('a') PORT_CHAR('A')	// 66
	PORT_BIT(0x00000080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(key_callback, (void *)3) PORT_CODE(KEYCODE_S)         PORT_CHAR('s') PORT_CHAR('S')	// 67
	PORT_BIT(0x0000ff00, IP_ACTIVE_HIGH, IPT_UNUSED)	// 48-4f
	PORT_BIT(0x00010000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(key_callback, (void *)3) PORT_CODE(KEYCODE_QUOTE)	  PORT_CHAR(':') PORT_CHAR('*')	// 70
	PORT_BIT(0x00020000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(key_callback, (void *)3) PORT_CODE(KEYCODE_ENTER)     PORT_CHAR(13)	// 71
	PORT_BIT(0x00040000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(key_callback, (void *)3) PORT_CODE(KEYCODE_BACKSLASH) PORT_CHAR('\\') PORT_CHAR('|')	// 72
	PORT_BIT(0x00080000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(key_callback, (void *)3) PORT_CODE(KEYCODE_SPACE)     PORT_CHAR(' ')	// 73
	PORT_BIT(0x00100000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(key_callback, (void *)3) PORT_CODE(KEYCODE_Z)         PORT_CHAR('z') PORT_CHAR('Z')	// 74
	PORT_BIT(0x00200000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(key_callback, (void *)3) PORT_CODE(KEYCODE_X)         PORT_CHAR('x') PORT_CHAR('X')	// 75
	PORT_BIT(0x00400000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(key_callback, (void *)3) PORT_CODE(KEYCODE_C)         PORT_CHAR('c') PORT_CHAR('C')	// 76
	PORT_BIT(0x00800000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(key_callback, (void *)3) PORT_CODE(KEYCODE_V)         PORT_CHAR('v') PORT_CHAR('V')	// 77
	PORT_BIT(0xff000000, IP_ACTIVE_HIGH, IPT_UNUSED)	// 58-5f

	PORT_START("keyboard_4")
	PORT_BIT(0x00000001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(key_callback, (void *)4) PORT_CODE(KEYCODE_INSERT) PORT_CHAR(UCHAR_MAMEKEY(INSERT)) PORT_CHAR(UCHAR_MAMEKEY(PRTSCR))	// 80
	PORT_BIT(0x00000002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(key_callback, (void *)4) PORT_CODE(KEYCODE_DEL)	   PORT_CHAR(UCHAR_MAMEKEY(DEL))    PORT_CHAR(12)	// 81
	PORT_BIT(0xfffffffc, IP_ACTIVE_HIGH, IPT_UNUSED)	// 82-9f

	PORT_START("keyboard_5")
	PORT_BIT(0x00000003, IP_ACTIVE_HIGH, IPT_UNUSED)	// a0-a1
	PORT_BIT(0x00000004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(key_callback, (void *)5) PORT_CODE(KEYCODE_LCONTROL) PORT_CHAR(UCHAR_SHIFT_2)	// a2
	PORT_BIT(0x00000008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(key_callback, (void *)5) PORT_CODE(KEYCODE_LSHIFT)   PORT_CHAR(UCHAR_SHIFT_1)	// a3
	PORT_BIT(0x00000010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(key_callback, (void *)5) PORT_CODE(KEYCODE_LALT)     PORT_CHAR(UCHAR_MAMEKEY(CAPSLOCK))	// a4
	PORT_BIT(0x00000020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(key_callback, (void *)5) PORT_CODE(KEYCODE_RALT)     PORT_NAME("Graph")	// a5
	PORT_BIT(0x00000040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(key_callback, (void *)5) PORT_CODE(KEYCODE_RSHIFT)   PORT_CHAR(UCHAR_SHIFT_1)	// a6
	PORT_BIT(0x00000080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(key_callback, (void *)5) PORT_CODE(KEYCODE_NUMLOCK)  PORT_CHAR(UCHAR_MAMEKEY(NUMLOCK))	// a7
	PORT_BIT(0xffffff00, IP_ACTIVE_HIGH, IPT_UNUSED)	// a8-bf /* b2-b7 are the 'make' codes for the above keys */
INPUT_PORTS_END

/* item keyboard */
static INPUT_PORTS_START( px4_h421a )
	PORT_INCLUDE(px4_dips)
INPUT_PORTS_END


/***************************************************************************
    PALETTE
***************************************************************************/

static PALETTE_INIT( px4 )
{
	palette_set_color(machine, 0, MAKE_RGB(138, 146, 148));
	palette_set_color(machine, 1, MAKE_RGB(92, 83, 88));
}

static PALETTE_INIT( px4p )
{
	palette_set_color(machine, 0, MAKE_RGB(149, 157, 130));
	palette_set_color(machine, 1, MAKE_RGB(92, 83, 88));
}


/***************************************************************************
    MACHINE DRIVERS
***************************************************************************/

static const cassette_config px4_cassette_config =
{
	cassette_default_formats,
	NULL,
	CASSETTE_PLAY | CASSETTE_SPEAKER_ENABLED | CASSETTE_MOTOR_DISABLED
};

static MACHINE_DRIVER_START( px4 )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", Z80, XTAL_7_3728MHz/2)	/* uPD70008 */
	MDRV_CPU_PROGRAM_MAP(px4_mem)
	MDRV_CPU_IO_MAP(px4_io)

	MDRV_DRIVER_DATA(px4_state)

	/* video hardware */
	MDRV_SCREEN_ADD("screen", LCD)
	MDRV_SCREEN_REFRESH_RATE(44)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(240, 64)
	MDRV_SCREEN_VISIBLE_AREA(0, 239, 0, 63)

	MDRV_DEFAULT_LAYOUT(layout_px4)

	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(px4)

	MDRV_VIDEO_UPDATE(px4)

	MDRV_TIMER_ADD_PERIODIC("one_sec", upd7508_1sec_callback, SEC(1))
	MDRV_TIMER_ADD_PERIODIC("frc", frc_tick, NSEC(1628))

	/* internal ram */
	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("64k")

	/* centronics printer */
	MDRV_CENTRONICS_ADD("centronics", standard_centronics)

	/* external cassette */
	MDRV_CASSETTE_ADD("extcas", px4_cassette_config)

	/* rom capsules */
	MDRV_CARTSLOT_ADD("capsule1")
	MDRV_CARTSLOT_NOT_MANDATORY
	MDRV_CARTSLOT_ADD("capsule2")
	MDRV_CARTSLOT_NOT_MANDATORY

	/* tf20 floppy drive */
//	MDRV_TF20_ADD("tf20")
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( px4p )
	MDRV_IMPORT_FROM(px4)

	MDRV_CPU_MODIFY("maincpu")
	MDRV_CPU_IO_MAP(px4p_io)

	MDRV_NVRAM_HANDLER(px4_ramdisk)

	MDRV_PALETTE_INIT(px4p)

	MDRV_CARTSLOT_ADD("ramdisk")
	MDRV_CARTSLOT_NOT_MANDATORY
MACHINE_DRIVER_END


/***************************************************************************
    ROM DEFINITIONS
***************************************************************************/

ROM_START( px4 )
    ROM_REGION(0x8000, "os", 0)
    ROM_LOAD("m25122aa_po_px4.10c", 0x0000, 0x8000, CRC(62d60dc6) SHA1(3d32ec79a317de7c84c378302e95f48d56505502))

    ROM_REGION(0x1000, "slave", 0)
    ROM_LOAD("upd7508.bin", 0x0000, 0x1000, NO_DUMP)

	ROM_REGION(0x8000, "capsule1", 0)
	ROM_CART_LOAD("capsule1", 0x0000, 0x8000, ROM_OPTIONAL)

	ROM_REGION(0x8000, "capsule2", 0)
	ROM_CART_LOAD("capsule2", 0x0000, 0x8000, ROM_OPTIONAL)
ROM_END

ROM_START( px4p )
    ROM_REGION(0x8000, "os", 0)
    ROM_LOAD("b0_pxa.10c", 0x0000, 0x8000, CRC(d74b9ef5) SHA1(baceee076c12f5a16f7a26000e9bc395d021c455))

    ROM_REGION(0x1000, "slave", 0)
    ROM_LOAD("upd7508.bin", 0x0000, 0x1000, NO_DUMP)

    ROM_REGION(0x8000, "capsule1", 0)
    ROM_CART_LOAD("capsule1", 0x0000, 0x8000, ROM_OPTIONAL)

    ROM_REGION(0x8000, "capsule2", 0)
    ROM_CART_LOAD("capsule2", 0x0000, 0x8000, ROM_OPTIONAL)

    ROM_REGION(0x20000, "ramdisk", 0)
    ROM_CART_LOAD("ramdisk", 0x0000, 0x20000, ROM_OPTIONAL | ROM_MIRROR)
ROM_END


/***************************************************************************
    GAME DRIVERS
***************************************************************************/

/*    YEAR  NAME  PARENT  COMPAT  MACHINE  INPUT      INIT  CONFIG  COMPANY  FULLNAME  FLAGS */
COMP( 1985, px4,  0,      0,      px4,     px4_h450a, px4,  0,      "Epson", "PX-4",   0 )
COMP( 1985, px4p, px4,    0,      px4p,    px4_h450a, px4p, 0,      "Epson", "PX-4+",  0 )

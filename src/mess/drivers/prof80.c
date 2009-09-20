/*

	PROF-80 (Prozessor RAM-Floppy Kontroller)
	GRIP-1/2/3/4/5 (Grafik-Interface-Prozessor)
	UNIO-1 (?)

	http://www.prof80.de/
	http://oldcomputers.dyndns.org/public/pub/rechner/conitec/info.html

*/

/*

	TODO:

	- PROF<>GRIP interface fails if IBFA is cleared after reading PA
	- keyboard
	- NE555 timeout is 10x too high
	- GRIP does not send keys back to PROF-80
	- grip31 does not work
	- PROF-80 RAM banking
	- PROF-80 floppy format
	- GRIP model selection
	- UNIO card (Z80-STI, Z80-SIO, 2x centronics)
	- GRIP-COLOR (192kB color RAM)
	- GRIP-5 (HD6345, 256KB RAM)
	- XR color card
	
*/

#include "driver.h"
#include "includes/prof80.h"
#include "cpu/z80/z80.h"
#include "cpu/z80/z80daisy.h"
#include "video/mc6845.h"
#include "machine/i8255a.h"
#include "machine/nec765.h"
#include "machine/upd1990a.h"
#include "machine/z80sti.h"
#include "machine/ctronics.h"
#include "machine/rescap.h"
#include "devices/flopdrv.h"
#include "formats/basicdsk.h"


INLINE const device_config *get_floppy_image(running_machine *machine, int drive)
{
	return floppy_get_device(machine, drive);
}

/* Keyboard HACK */

static const UINT8 prof80_keycodes[3][9][8] =
{
	/* unshifted */
	{
	{ 0x1e, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37 },
	{ 0x38, 0x39, 0x30, 0x2d, 0x3d, 0x08, 0x7f, 0x2d },
	{ 0x37, 0x38, 0x39, 0x09, 0x71, 0x77, 0x65, 0x72 },
	{ 0x74, 0x79, 0x75, 0x69, 0x6f, 0x70, 0x5b, 0x5d },
	{ 0x1b, 0x2b, 0x34, 0x35, 0x36, 0x61, 0x73, 0x64 },
	{ 0x66, 0x67, 0x68, 0x6a, 0x6b, 0x6c, 0x3b, 0x27 },
	{ 0x0d, 0x0a, 0x01, 0x31, 0x32, 0x33, 0x7a, 0x78 },
	{ 0x63, 0x76, 0x62, 0x6e, 0x6d, 0x2c, 0x2e, 0x2f },
	{ 0x04, 0x02, 0x03, 0x30, 0x2e, 0x20, 0x00, 0x00 }
	},

	/* shifted */
	{
	{ 0x1e, 0x21, 0x40, 0x23, 0x24, 0x25, 0x5e, 0x26 },
	{ 0x2a, 0x28, 0x29, 0x5f, 0x2b, 0x08, 0x7f, 0x2d },
	{ 0x37, 0x38, 0x39, 0x09, 0x51, 0x57, 0x45, 0x52 },
	{ 0x54, 0x59, 0x55, 0x49, 0x4f, 0x50, 0x7b, 0x7d },
	{ 0x1b, 0x2b, 0x34, 0x35, 0x36, 0x41, 0x53, 0x44 },
	{ 0x46, 0x47, 0x48, 0x4a, 0x4b, 0x4c, 0x3a, 0x22 },
	{ 0x0d, 0x0a, 0x01, 0x31, 0x32, 0x33, 0x5a, 0x58 },
	{ 0x43, 0x56, 0x42, 0x4e, 0x4d, 0x3c, 0x3e, 0x3f },
	{ 0x04, 0x02, 0x03, 0x30, 0x2e, 0x20, 0x00, 0x00 }
	},

	/* control */
	{
	{ 0x9e, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97 },
	{ 0x98, 0x99, 0x90, 0x1f, 0x9a, 0x88, 0xff, 0xad },
	{ 0xb7, 0xb8, 0xb9, 0x89, 0x11, 0x17, 0x05, 0x12 },
	{ 0x14, 0x19, 0x15, 0x09, 0x0f, 0x10, 0x1b, 0x1d },
	{ 0x9b, 0xab, 0xb4, 0xb5, 0xb6, 0x01, 0x13, 0x04 },
	{ 0x06, 0x07, 0x08, 0x0a, 0x0b, 0x0c, 0x7e, 0x60 },
	{ 0x8d, 0x8a, 0x81, 0xb1, 0xb2, 0xb3, 0x1a, 0x18 },
	{ 0x03, 0x16, 0x02, 0x0e, 0x0d, 0x1c, 0x7c, 0x5c },
	{ 0x84, 0x82, 0x83, 0xb0, 0xae, 0x00, 0x00, 0x00 }
	}
};

static void prof80_keyboard_scan(running_machine *machine)
{
	prof80_state *state = machine->driver_data;

	static const char *const keynames[] = { "ROW0", "ROW1", "ROW2", "ROW3", "ROW4", "ROW5", "ROW6", "ROW7", "ROW8" };
	int table = 0, row, col;
	int keydata = -1;

	if (input_port_read(machine, "ROW9") & 0x07)
	{
		/* shift, upper case */
		table = 1;
	}

	if (input_port_read(machine, "ROW9") & 0x18)
	{
		/* ctrl */
		table = 2;
	}

	/* scan keyboard */
	for (row = 0; row < 9; row++)
	{
		UINT8 data = input_port_read(machine, keynames[row]);

		for (col = 0; col < 8; col++)
		{
			if (!BIT(data, col))
			{
				/* latch key data */
				keydata = ~prof80_keycodes[table][row][col];
				
				if (state->keydata != keydata)
				{
					state->keydata = keydata;

					/* trigger GRIP 8255 port C bit 2 (_STBB) */
					i8255a_pc2_w(state->ppi8255, 0);
					i8255a_pc2_w(state->ppi8255, 1);
					return;
				}
			}
		}
	}

	state->keydata = keydata;
}

static TIMER_DEVICE_CALLBACK( prof80_keyboard_tick )
{
	prof80_state *state = timer->machine->driver_data;

	if (!state->kbf) prof80_keyboard_scan(timer->machine);
}

/* PROF-80 */

static void prof80_bankswitch(running_machine *machine)
{
	prof80_state *state = machine->driver_data;
	const address_space *program = cputag_get_address_space(machine, Z80_TAG, ADDRESS_SPACE_PROGRAM);
	int bank;

	for (bank = 0; bank < 16; bank++)
	{
		UINT16 start_addr = bank * 0x1000;
		UINT16 end_addr = start_addr + 0xfff;
		int block = state->mme ? (~state->mmu[bank] & 0x0f) : 15;

		memory_set_bank(machine, bank + 1, block);

		switch (block)
		{
		case 15:
		case 14:
		case 7:
		case 6:
			/* EPROM */
			memory_install_readwrite8_handler(program, start_addr, end_addr, 0, 0, SMH_BANK(bank + 1), SMH_UNMAP);
			break;

		case 13:
		case 12:
		case 5:
		case 4:
			/* RAM */
			memory_install_readwrite8_handler(program, start_addr, end_addr, 0, 0, SMH_BANK(bank + 1), SMH_BANK(bank + 1));
			break;

		default:
			memory_install_readwrite8_handler(program, start_addr, end_addr, 0, 0, SMH_BANK(bank + 1), SMH_UNMAP);
		}
		
		//logerror("Segment %u address %04x-%04x block %u\n", bank, start_addr, end_addr, block);
	}
}

static TIMER_CALLBACK( floppy_motor_off_tick )
{
	prof80_state *state = machine->driver_data;

	floppy_drive_set_motor_state(get_floppy_image(machine, 0), 0);
	floppy_drive_set_motor_state(get_floppy_image(machine, 1), 0);
	floppy_drive_set_ready_state(get_floppy_image(machine, 0), 0, 1);
	floppy_drive_set_ready_state(get_floppy_image(machine, 1), 0, 1);

	state->motor = 0;
}

static void ls259_w(running_machine *machine, int fa, int sa, int fb, int sb)
{
	prof80_state *state = machine->driver_data;

	switch (sa)
	{
	case 0: /* C0/TDI */
		upd1990a_data_w(state->upd1990a, fa);
		upd1990a_c0_w(state->upd1990a, fa);
		break;

	case 1: /* C1 */
		upd1990a_c1_w(state->upd1990a, fa);
		break;

	case 2: /* C2 */
		upd1990a_c2_w(state->upd1990a, fa);
		break;

	case 3:	/* READY */
		break;

	case 4: /* TCK */
		upd1990a_clk_w(state->upd1990a, fa);
		break;

	case 5:	/* IN USE */
		output_set_led_value(0, fa);
		break;

	case 6:	/* _MOTOR */
		if (fa)
		{
			/* trigger floppy motor off NE555 timer */
			int t = 110 * RES_M(10) * CAP_U(6.8); // t = 1.1 * R8 * C6

			timer_adjust_oneshot(state->floppy_motor_off_timer, ATTOTIME_IN_MSEC(t), 0);
		}
		else
		{
			/* turn on floppy motor */
			floppy_drive_set_motor_state(get_floppy_image(machine, 0), 1);
			floppy_drive_set_motor_state(get_floppy_image(machine, 1), 1);
			floppy_drive_set_ready_state(get_floppy_image(machine, 0), 1, 1);
			floppy_drive_set_ready_state(get_floppy_image(machine, 1), 1, 1);

			state->motor = 1;

			/* reset floppy motor off NE555 timer */
			timer_enable(state->floppy_motor_off_timer, 0);
		}
		break;

	case 7:	/* SELECT */
		break;
	}

	switch (sb)
	{
	case 0: /* RESF */
		if (fb) nec765_reset(state->nec765, 0);
		break;

	case 1: /* MINI */
		break;

	case 2: /* _RTS */
		break;

	case 3: /* TX */
		break;

	case 4: /* _MSTOP */
		if (!fb)
		{
			/* immediately turn off floppy motor */
			timer_adjust_oneshot(state->floppy_motor_off_timer, attotime_zero, 0);
		}
		break;

	case 5: /* TXP */
		break;

	case 6: /* TSTB */
		upd1990a_stb_w(state->upd1990a, fb);
		break;

	case 7: /* MME */
		state->mme = fb;
		break;
	}
}

static WRITE8_HANDLER( flr_w )
{
	/*

		bit		description

		0		FB
		1		SB0
		2		SB1
		3		SB2
		4		SA0
		5		SA1
		6		SA2
		7		FA

	*/

	int fa = BIT(data, 7);
	int sa = (data >> 4) & 0x07;

	int fb = BIT(data, 0);
	int sb = (data >> 1) & 0x07;

	ls259_w(space->machine, fa, sa, fb, sb);
}

static READ8_HANDLER( status_r )
{
	/*

		bit		signal		description

		0		_RX
		1
		2
		3
		4		CTS
		5		_INDEX
		6		
		7		CTSP

	*/

	prof80_state *state = space->machine->driver_data;

	return (state->fdc_index << 5) | 0x01;
}

static READ8_HANDLER( status2_r )
{
	/*

		bit		signal		description

		0		_MOTOR		floppy motor (0=on, 1=off)
		1
		2
		3
		4		JS4
		5		JS5
		6
		7		_TDO

	*/

	prof80_state *state = space->machine->driver_data;

	return (!state->rtc_data << 7) | !state->motor;
}

static WRITE8_HANDLER( mmu_w )
{
	prof80_state *state = space->machine->driver_data;

	int bank = offset >> 12;

	state->mmu[bank] = data & 0x0f;

	//logerror("MMU bank %u block %u\n", bank, data & 0x0f);

	prof80_bankswitch(space->machine);
}

static READ8_HANDLER( gripc_r )
{
	prof80_state *state = space->machine->driver_data;

	return state->gripc;
}

static READ8_HANDLER( gripd_r )
{
	prof80_state *state = space->machine->driver_data;

	/* trigger GRIP 8255 port C bit 6 (_ACKA) */
	i8255a_pc6_w(state->ppi8255, 0);
	i8255a_pc6_w(state->ppi8255, 1);

	return state->gripd;
}

static WRITE8_HANDLER( gripd_w )
{
	prof80_state *state = space->machine->driver_data;

	state->gripd = data;

	/* trigger GRIP 8255 port C bit 4 (_STBA) */
	i8255a_pc4_w(state->ppi8255, 0);
	i8255a_pc4_w(state->ppi8255, 1);
}

/* GRIP */

static WRITE8_HANDLER( flash_w )
{
	prof80_state *state = space->machine->driver_data;

	state->flash = BIT(data, 7);
}

static WRITE8_HANDLER( page_w )
{
	prof80_state *state = space->machine->driver_data;

	state->page = BIT(data, 7);

	memory_set_bank(space->machine, 17, state->page);
}

static READ8_HANDLER( stat_r )
{
	/*

		bit		signal		description

		0		LPA0
		1		LPA1
		2		LPA2
		3		SENSE
		4		JS0
		5		JS1
		6		_ERROR
		7		LPSTB		light pen strobe

	*/

	prof80_state *state = space->machine->driver_data;

	return (state->lps << 7) | (centronics_fault_r(state->centronics) << 6);
}

static READ8_HANDLER( lrs_r )
{
	prof80_state *state = space->machine->driver_data;

	state->lps = 0;

	return 0;
}

static WRITE8_HANDLER( lrs_w )
{
	prof80_state *state = space->machine->driver_data;

	state->lps = 0;
}

static READ8_HANDLER( cxstb_r )
{
	prof80_state *state = space->machine->driver_data;

	centronics_strobe_w(state->centronics, 0);
	centronics_strobe_w(state->centronics, 1);

	return 0;
}

static WRITE8_HANDLER( cxstb_w )
{
	prof80_state *state = space->machine->driver_data;

	centronics_strobe_w(state->centronics, 0);
	centronics_strobe_w(state->centronics, 1);
}

/* UNIO */

static WRITE8_HANDLER( unio_ctrl_w )
{
//	prof80_state *state = space->machine->driver_data;

//	int flag = BIT(data, 0);
	int flad = (data >> 1) & 0x07;

	switch (flad)
	{
	case 0: /* CG1 */
	case 1: /* CG2 */
	case 2: /* _STB1 */
	case 3: /* _STB2 */
	case 4: /* _INIT */
	case 5: /* JSO0 */
	case 6: /* JSO1 */
	case 7: /* JSO2 */
		break;
	}
}

/* Memory Maps */

static ADDRESS_MAP_START( prof80_mem, ADDRESS_SPACE_PROGRAM, 8 )
    AM_RANGE(0x0000, 0x0fff) AM_RAMBANK(1)
    AM_RANGE(0x1000, 0x1fff) AM_RAMBANK(2)
    AM_RANGE(0x2000, 0x2fff) AM_RAMBANK(3)
    AM_RANGE(0x3000, 0x3fff) AM_RAMBANK(4)
    AM_RANGE(0x4000, 0x4fff) AM_RAMBANK(5)
    AM_RANGE(0x5000, 0x5fff) AM_RAMBANK(6)
    AM_RANGE(0x6000, 0x6fff) AM_RAMBANK(7)
    AM_RANGE(0x7000, 0x7fff) AM_RAMBANK(8)
    AM_RANGE(0x8000, 0x8fff) AM_RAMBANK(9)
    AM_RANGE(0x9000, 0x9fff) AM_RAMBANK(10)
    AM_RANGE(0xa000, 0xafff) AM_RAMBANK(11)
    AM_RANGE(0xb000, 0xbfff) AM_RAMBANK(12)
    AM_RANGE(0xc000, 0xcfff) AM_RAMBANK(13)
    AM_RANGE(0xd000, 0xdfff) AM_RAMBANK(14)
    AM_RANGE(0xe000, 0xefff) AM_RAMBANK(15)
    AM_RANGE(0xf000, 0xffff) AM_RAMBANK(16)
ADDRESS_MAP_END

static ADDRESS_MAP_START( prof80_io, ADDRESS_SPACE_IO, 8 )
//	AM_RANGE(0x80, 0x8f) AM_MIRROR(0xff00) AM_DEVREADWRITE(UNIO_Z80STI_TAG, z80sti_r, z80sti_w)
//	AM_RANGE(0x94, 0x95) AM_MIRROR(0xff00) AM_DEVREADWRITE(UNIO_Z80SIO_TAG, z80sio_d_r, z80sio_d_w)
//	AM_RANGE(0x96, 0x97) AM_MIRROR(0xff00) AM_DEVREADWRITE(UNIO_Z80SIO_TAG, z80sio_c_r, z80sio_c_w)
	AM_RANGE(0x9e, 0x9e) AM_MIRROR(0xff00) AM_WRITE(unio_ctrl_w)
//	AM_RANGE(0x9c, 0x9c) AM_MIRROR(0xff00) AM_DEVWRITE(UNIO_CENTRONICS1_TAG, centronics_data_w)
//	AM_RANGE(0x9d, 0x9d) AM_MIRROR(0xff00) AM_DEVWRITE(UNIO_CENTRONICS1_TAG, centronics_data_w)
	AM_RANGE(0xc0, 0xc0) AM_MIRROR(0xff00) AM_READ(gripc_r)
	AM_RANGE(0xc1, 0xc1) AM_MIRROR(0xff00) AM_READWRITE(gripd_r, gripd_w)
	AM_RANGE(0xd8, 0xd8) AM_MIRROR(0xff00) AM_WRITE(flr_w)
	AM_RANGE(0xda, 0xda) AM_MIRROR(0xff00) AM_READ(status_r)
	AM_RANGE(0xdb, 0xdb) AM_MIRROR(0xff00) AM_READ(status2_r)
	AM_RANGE(0xdc, 0xdc) AM_MIRROR(0xff00) AM_DEVREAD(NEC765_TAG, nec765_status_r)
	AM_RANGE(0xdd, 0xdd) AM_MIRROR(0xff00) AM_DEVREADWRITE(NEC765_TAG, nec765_data_r, nec765_data_w)
	AM_RANGE(0xde, 0xde) AM_MIRROR(0xff01) AM_MASK(0xff00) AM_WRITE(mmu_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( grip_mem, ADDRESS_SPACE_PROGRAM, 8 )
    AM_RANGE(0x0000, 0x3fff) AM_ROM
    AM_RANGE(0x4000, 0x47ff) AM_RAM
    AM_RANGE(0x8000, 0xffff) AM_RAMBANK(17)
ADDRESS_MAP_END

static ADDRESS_MAP_START( grip_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x00, 0x00) AM_READWRITE(cxstb_r, cxstb_w)
//	AM_RANGE(0x10, 0x10) AM_WRITE(ccon_w)
//	AM_RANGE(0x11, 0x11) AM_WRITE(vol0_w)
//	AM_RANGE(0x12, 0x12) AM_WRITE(rts_w)
	AM_RANGE(0x13, 0x13) AM_WRITE(page_w)
//	AM_RANGE(0x14, 0x14) AM_WRITE(cc1_w)
//	AM_RANGE(0x15, 0x15) AM_WRITE(cc2_w)
	AM_RANGE(0x16, 0x16) AM_WRITE(flash_w)
//	AM_RANGE(0x17, 0x17) AM_WRITE(vol1_w)
	AM_RANGE(0x20, 0x2f) AM_DEVREADWRITE(Z80STI_TAG, z80sti_r, z80sti_w)
	AM_RANGE(0x30, 0x30) AM_READWRITE(lrs_r, lrs_w)
	AM_RANGE(0x40, 0x40) AM_READ(stat_r)
	AM_RANGE(0x50, 0x50) AM_DEVWRITE(MC6845_TAG, mc6845_address_w)
	AM_RANGE(0x52, 0x52) AM_DEVWRITE(MC6845_TAG, mc6845_register_w)
	AM_RANGE(0x53, 0x53) AM_DEVREAD(MC6845_TAG, mc6845_register_r)
	AM_RANGE(0x60, 0x60) AM_DEVWRITE("centronics", centronics_data_w)
	AM_RANGE(0x70, 0x73) AM_DEVREADWRITE(I8255A_TAG, i8255a_r, i8255a_w)
//	AM_RANGE(0x80, 0x80) AM_WRITE(bl2out_w)
//	AM_RANGE(0x90, 0x90) AM_WRITE(gr2out_w)
//	AM_RANGE(0xa0, 0xa0) AM_WRITE(rd2out_w)
//	AM_RANGE(0xb0, 0xb0) AM_WRITE(clrg2_w)
//	AM_RANGE(0xc0, 0xc0) AM_WRITE(bluout_w)
//	AM_RANGE(0xd0, 0xd0) AM_WRITE(grnout_w)
//	AM_RANGE(0xe0, 0xe0) AM_WRITE(redout_w)
//	AM_RANGE(0xf0, 0xf0) AM_WRITE(clrg1_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( grip5_mem, ADDRESS_SPACE_PROGRAM, 8 )
    AM_RANGE(0x0000, 0x3fff) AM_ROMBANK(18)
    AM_RANGE(0x4000, 0x47ff) AM_RAM
    AM_RANGE(0x8000, 0xffff) AM_RAMBANK(17)
ADDRESS_MAP_END

static ADDRESS_MAP_START( grip5_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x00, 0x00) AM_READWRITE(cxstb_r, cxstb_w)
//	AM_RANGE(0x10, 0x10) AM_WRITE(eprom_w)
//	AM_RANGE(0x11, 0x11) AM_WRITE(vol0_w)
//	AM_RANGE(0x12, 0x12) AM_WRITE(rts_w)
	AM_RANGE(0x13, 0x13) AM_WRITE(page_w)
//	AM_RANGE(0x14, 0x14) AM_WRITE(str_w)
//	AM_RANGE(0x15, 0x15) AM_WRITE(intl_w)
//	AM_RANGE(0x16, 0x16) AM_WRITE(dpage_w)
//	AM_RANGE(0x17, 0x17) AM_WRITE(vol1_w)
	AM_RANGE(0x20, 0x2f) AM_DEVREADWRITE(Z80STI_TAG, z80sti_r, z80sti_w)
	AM_RANGE(0x30, 0x30) AM_READWRITE(lrs_r, lrs_w)
	AM_RANGE(0x40, 0x40) AM_READ(stat_r)
	AM_RANGE(0x50, 0x50) AM_DEVWRITE(MC6845_TAG, mc6845_address_w)
	AM_RANGE(0x52, 0x52) AM_DEVWRITE(MC6845_TAG, mc6845_register_w)
	AM_RANGE(0x53, 0x53) AM_DEVREAD(MC6845_TAG, mc6845_register_r)
	AM_RANGE(0x60, 0x60) AM_DEVWRITE("centronics", centronics_data_w)
	AM_RANGE(0x70, 0x73) AM_DEVREADWRITE(I8255A_TAG, i8255a_r, i8255a_w)

//	AM_RANGE(0x80, 0x80) AM_WRITE(xrflgs_w)
//	AM_RANGE(0xc0, 0xc0) AM_WRITE(xrclrg_w)
//	AM_RANGE(0xe0, 0xe0) AM_WRITE(xrclu0_w)
//	AM_RANGE(0xe1, 0xe1) AM_WRITE(xrclu1_w)
//	AM_RANGE(0xe2, 0xe2) AM_WRITE(xrclu2_w)

//	AM_RANGE(0x80, 0x80) AM_WRITE(bl2out_w)
//	AM_RANGE(0x90, 0x90) AM_WRITE(gr2out_w)
//	AM_RANGE(0xa0, 0xa0) AM_WRITE(rd2out_w)
//	AM_RANGE(0xb0, 0xb0) AM_WRITE(clrg2_w)
//	AM_RANGE(0xc0, 0xc0) AM_WRITE(bluout_w)
//	AM_RANGE(0xd0, 0xd0) AM_WRITE(grnout_w)
//	AM_RANGE(0xe0, 0xe0) AM_WRITE(redout_w)
//	AM_RANGE(0xf0, 0xf0) AM_WRITE(clrg1_w)
ADDRESS_MAP_END

/* Input Ports */

static INPUT_PORTS_START( prof80 )
	PORT_START("ROW0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("HELP") PORT_CODE(KEYCODE_TILDE)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('@')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('^')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('&')

	PORT_START("ROW1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('*')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR('(')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_0) PORT_CHAR('0') PORT_CHAR(')')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_MINUS) PORT_CHAR('-') PORT_CHAR('_')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_EQUALS) PORT_CHAR('=') PORT_CHAR('+')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("BACKSPACE") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("DEL") PORT_CODE(KEYCODE_DEL)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad -") PORT_CODE(KEYCODE_MINUS_PAD)

	PORT_START("ROW2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 7") PORT_CODE(KEYCODE_7_PAD)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 8") PORT_CODE(KEYCODE_8_PAD)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 9") PORT_CODE(KEYCODE_9_PAD)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("TAB") PORT_CODE(KEYCODE_TAB) PORT_CHAR('\t')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Q) PORT_CHAR('q') PORT_CHAR('Q')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_W) PORT_CHAR('w') PORT_CHAR('W')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_E) PORT_CHAR('e') PORT_CHAR('E')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_R) PORT_CHAR('r') PORT_CHAR('R')
	
	PORT_START("ROW3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_T) PORT_CHAR('t') PORT_CHAR('T')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Y) PORT_CHAR('y') PORT_CHAR('Y')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_U) PORT_CHAR('u') PORT_CHAR('U')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_I) PORT_CHAR('i') PORT_CHAR('I')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_O) PORT_CHAR('o') PORT_CHAR('O')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_P) PORT_CHAR('p') PORT_CHAR('P')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR('[') PORT_CHAR('{')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR(']') PORT_CHAR('}')
	
	PORT_START("ROW4")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("ESC") PORT_CODE(KEYCODE_ESC) PORT_CHAR(UCHAR_MAMEKEY(ESC))
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad +") PORT_CODE(KEYCODE_PLUS_PAD)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 4") PORT_CODE(KEYCODE_4_PAD)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 5") PORT_CODE(KEYCODE_5_PAD)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 6") PORT_CODE(KEYCODE_6_PAD)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_A) PORT_CHAR('a') PORT_CHAR('A')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_S) PORT_CHAR('s') PORT_CHAR('S')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_D) PORT_CHAR('d') PORT_CHAR('D')
	
	PORT_START("ROW5")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F) PORT_CHAR('f') PORT_CHAR('F')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_G) PORT_CHAR('g') PORT_CHAR('G')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_H) PORT_CHAR('h') PORT_CHAR('H')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_J) PORT_CHAR('j') PORT_CHAR('J')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_K) PORT_CHAR('k') PORT_CHAR('K')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_L) PORT_CHAR('l') PORT_CHAR('L')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COLON) PORT_CHAR(';') PORT_CHAR(':')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_QUOTE) PORT_CHAR('\'') PORT_CHAR('"')
	
	PORT_START("ROW6")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("RETURN") PORT_CODE(KEYCODE_ENTER) PORT_CHAR('\r')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("LINE FEED") PORT_CODE(KEYCODE_ENTER_PAD)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x91") PORT_CODE(KEYCODE_UP) PORT_CHAR(UCHAR_MAMEKEY(UP))
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 1") PORT_CODE(KEYCODE_1_PAD)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 2") PORT_CODE(KEYCODE_2_PAD)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 3") PORT_CODE(KEYCODE_3_PAD)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Z) PORT_CHAR('z') PORT_CHAR('Z')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_X) PORT_CHAR('x') PORT_CHAR('X')
	
	PORT_START("ROW7")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_C) PORT_CHAR('c') PORT_CHAR('C')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_V) PORT_CHAR('v') PORT_CHAR('V')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_B) PORT_CHAR('b') PORT_CHAR('B')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_N) PORT_CHAR('n') PORT_CHAR('N')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_M) PORT_CHAR('m') PORT_CHAR('M')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR('<')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR('>')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_SLASH) PORT_CHAR('/') PORT_CHAR('?')
	
	PORT_START("ROW8")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x90") PORT_CODE(KEYCODE_LEFT) PORT_CHAR(UCHAR_MAMEKEY(LEFT))
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x93") PORT_CODE(KEYCODE_DOWN) PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x92") PORT_CODE(KEYCODE_RIGHT) PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 0") PORT_CODE(KEYCODE_0_PAD)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad .") PORT_CODE(KEYCODE_ASTERISK)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("SPACE") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) 

	PORT_START("ROW9")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("LOCK") PORT_CODE(KEYCODE_CAPSLOCK) PORT_TOGGLE
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("LEFT SHIFT") PORT_CODE(KEYCODE_LSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("RIGHT SHIFT") PORT_CODE(KEYCODE_RSHIFT)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("LEFT CTRL") PORT_CODE(KEYCODE_LCONTROL) PORT_CHAR(UCHAR_MAMEKEY(LCONTROL))
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("RIGHT CTRL") PORT_CODE(KEYCODE_RCONTROL) PORT_CHAR(UCHAR_MAMEKEY(RCONTROL))

	PORT_START("J1")
	PORT_CONFNAME( 0x01, 0x00, "J1 RDY/HDLD")
	PORT_CONFSETTING( 0x00, "HDLD" )
	PORT_CONFSETTING( 0x01, "READY" )

	PORT_START("J2")
	PORT_CONFNAME( 0x01, 0x01, "J2 RDY/DCHG")
	PORT_CONFSETTING( 0x00, "DCHG" )
	PORT_CONFSETTING( 0x01, "READY" )

	PORT_START("J3")
	PORT_CONFNAME( 0x01, 0x00, "J3 Port Address")
	PORT_CONFSETTING( 0x00, "D8-DF" )
	PORT_CONFSETTING( 0x01, "E8-EF" )

	PORT_START("J4")
	PORT_CONFNAME( 0x07, 0x00, "J4 Console")
	PORT_CONFSETTING( 0x00, "GRIP-1" )
	PORT_CONFSETTING( 0x01, "V24 DUPLEX" )
	PORT_CONFSETTING( 0x02, "USER1" )
	PORT_CONFSETTING( 0x03, "USER2" )
	PORT_CONFSETTING( 0x04, "CP/M" )

	PORT_START("J5")
	PORT_CONFNAME( 0x07, 0x00, "J5 Baud")
	PORT_CONFSETTING( 0x00, "9600" )
	PORT_CONFSETTING( 0x01, "4800" )
	PORT_CONFSETTING( 0x02, "2400" )
	PORT_CONFSETTING( 0x03, "1200" )
	PORT_CONFSETTING( 0x04, "300" )

	PORT_START("J6")
	PORT_CONFNAME( 0x01, 0x01, "J6 Interrupt")
	PORT_CONFSETTING( 0x00, "Serial" )
	PORT_CONFSETTING( 0x01, "ECB" )

	PORT_START("J7")
	PORT_CONFNAME( 0x01, 0x01, "J7 DMA MMU")
	PORT_CONFSETTING( 0x00, "PROF" )
	PORT_CONFSETTING( 0x01, "DMA Card" )

	PORT_START("J8")
	PORT_CONFNAME( 0x01, 0x01, "J8 Active Mode")
	PORT_CONFSETTING( 0x00, DEF_STR( Off ) )
	PORT_CONFSETTING( 0x01, DEF_STR( On ) )

	PORT_START("J9")
	PORT_CONFNAME( 0x01, 0x00, "J9 EPROM Type")
	PORT_CONFSETTING( 0x00, "2732/2764" )
	PORT_CONFSETTING( 0x01, "27128" )

	PORT_START("J10")
	PORT_CONFNAME( 0x03, 0x00, "J10 Wait States")
	PORT_CONFSETTING( 0x00, "On all memory accesses" )
	PORT_CONFSETTING( 0x01, "On internal memory accesses" )
	PORT_CONFSETTING( 0x02, DEF_STR( None ) )

	PORT_START("L1")
	PORT_CONFNAME( 0x01, 0x00, "L1 Write Polarity")
	PORT_CONFSETTING( 0x00, "Inverted" )
	PORT_CONFSETTING( 0x01, "Normal" )

	PORT_START("GRIP-J1")
	PORT_CONFNAME( 0x01, 0x00, "J1 EPROM Type")
	PORT_CONFSETTING( 0x00, "2732" )
	PORT_CONFSETTING( 0x01, "2764/27128" )

	PORT_START("GRIP-J2")
	PORT_CONFNAME( 0x03, 0x00, "J2 Centronics Mode")
	PORT_CONFSETTING( 0x00, "Mode 1" )
	PORT_CONFSETTING( 0x01, "Mode 2" )
	PORT_CONFSETTING( 0x02, "Mode 3" )

	PORT_START("GRIP-J3A")
	PORT_CONFNAME( 0x07, 0x00, "J3 Host")
	PORT_CONFSETTING( 0x00, "ECB Bus" )
	PORT_CONFSETTING( 0x01, "V24 9600 Baud" )
	PORT_CONFSETTING( 0x02, "V24 4800 Baud" )
	PORT_CONFSETTING( 0x03, "V24 1200 Baud" )
	PORT_CONFSETTING( 0x04, "Keyboard" )

	PORT_START("GRIP-J3B")
	PORT_CONFNAME( 0x07, 0x00, "J3 Keyboard")
	PORT_CONFSETTING( 0x00, "Parallel" )
	PORT_CONFSETTING( 0x01, "Serial (1200 Baud, 8 Bits)" )
	PORT_CONFSETTING( 0x02, "Serial (1200 Baud, 7 Bits)" )
	PORT_CONFSETTING( 0x03, "Serial (600 Baud, 8 Bits)" )
	PORT_CONFSETTING( 0x04, "Serial (600 Baud, 7 Bits)" )

	PORT_START("GRIP-J4")
	PORT_CONFNAME( 0x01, 0x00, "J4 GRIP-COLOR")
	PORT_CONFSETTING( 0x00, DEF_STR( No ) )
	PORT_CONFSETTING( 0x01, DEF_STR( Yes ) )

	PORT_START("GRIP-J5")
	PORT_CONFNAME( 0x01, 0x01, "J5 Power On Reset")
	PORT_CONFSETTING( 0x00, "External" )
	PORT_CONFSETTING( 0x01, "Internal" )

	PORT_START("GRIP-J6")
	PORT_CONFNAME( 0x01, 0x00, "J6 Serial Clock")
	PORT_CONFSETTING( 0x00, "TC/16, TD/16, TD" )
	PORT_CONFSETTING( 0x01, "TD/16, TD/16, TD" )
	PORT_CONFSETTING( 0x02, "TC/16, BAUD/16, input" )
	PORT_CONFSETTING( 0x03, "BAUD/16, BAUD/16, input" )

	PORT_START("GRIP-J7")
	PORT_CONFNAME( 0x01, 0x00, "J7 ECB Bus Address")
	PORT_CONFSETTING( 0x00, "C0/C1" )
	PORT_CONFSETTING( 0x01, "A0/A1" )

	PORT_START("GRIP-J8")
	PORT_CONFNAME( 0x01, 0x00, "J8 Video RAM")
	PORT_CONFSETTING( 0x00, "32 KB" )
	PORT_CONFSETTING( 0x01, "64 KB" )

	PORT_START("GRIP-J9")
	PORT_CONFNAME( 0x01, 0x01, "J9 CPU Clock")
	PORT_CONFSETTING( 0x00, "2 MHz" )
	PORT_CONFSETTING( 0x01, "4 MHz" )

	PORT_START("GRIP-J10")
	PORT_CONFNAME( 0x01, 0x01, "J10 Pixel Clock")
	PORT_CONFSETTING( 0x00, "External" )
	PORT_CONFSETTING( 0x01, "Internal" )
INPUT_PORTS_END

/* Video */

static MC6845_UPDATE_ROW( grip_update_row )
{
	prof80_state *state = device->machine->driver_data;
	int column, bit;

	for (column = 0; column < x_count; column++)
	{
		UINT16 address = (state->page << 12) | ((ma + column) << 3) | (ra & 0x07);
		UINT8 data = state->video_ram[address];

		for (bit = 0; bit < 8; bit++)
		{
			int x = (column * 8) + bit;
			int color = state->flash ? 0 : BIT(data, bit);

			*BITMAP_ADDR16(bitmap, y, x) = color;
		}
	}
}

static WRITE_LINE_DEVICE_HANDLER( grip_de_w )
{
	prof80_state *driver_state = device->machine->driver_data;

	z80sti_i1_w(driver_state->z80sti, state);
}

static WRITE_LINE_DEVICE_HANDLER( grip_cur_w )
{
	prof80_state *driver_state = device->machine->driver_data;

	z80sti_i2_w(driver_state->z80sti, state);
}

static const mc6845_interface grip_mc6845_interface = 
{
	SCREEN_TAG,
	8,
	NULL,
	grip_update_row,
	NULL,
	DEVCB_LINE(grip_de_w),
	DEVCB_LINE(grip_cur_w),
	DEVCB_NULL,
	DEVCB_NULL,
	NULL
};

static VIDEO_START( prof80 )
{
}

static VIDEO_UPDATE( prof80 )
{
	prof80_state *state = screen->machine->driver_data;

	mc6845_update(state->mc6845, bitmap, cliprect);

	return 0;
}

/* uPD1990A Interface */

static WRITE_LINE_DEVICE_HANDLER( prof80_upd1990a_data_w )
{
	prof80_state *driver_state = device->machine->driver_data;

	driver_state->rtc_data = state;
}

static UPD1990A_INTERFACE( prof80_upd1990a_intf )
{
	DEVCB_LINE(prof80_upd1990a_data_w),
	DEVCB_NULL
};

/* NEC765 Interface */

static void prof80_fdc_index_callback(const device_config *controller, const device_config *img, int state)
{
	prof80_state *driver_state = img->machine->driver_data;

	driver_state->fdc_index = state;
}

static const struct nec765_interface prof80_nec765_interface =
{
	DEVCB_NULL,
	NULL,
	NULL,
	NEC765_RDY_PIN_CONNECTED,
	{FLOPPY_0,FLOPPY_1, FLOPPY_2, FLOPPY_3}
};

/* PPI8255 Interface */

static READ8_DEVICE_HANDLER( grip_ppi8255_a_r )
{
	/*

        bit		description

        PA0     ECB bus
        PA1     ECB bus
        PA2     ECB bus
        PA3     ECB bus
        PA4     ECB bus
        PA5     ECB bus
        PA6     ECB bus
        PA7     ECB bus
    
	*/

	prof80_state *state = device->machine->driver_data;

	return state->gripd;
}

static WRITE8_DEVICE_HANDLER( grip_ppi8255_a_w )
{
	/*

        bit		description

        PA0     ECB bus
        PA1     ECB bus
        PA2     ECB bus
        PA3     ECB bus
        PA4     ECB bus
        PA5     ECB bus
        PA6     ECB bus
        PA7     ECB bus
    
	*/

	prof80_state *state = device->machine->driver_data;

	state->gripd = data;
}

static READ8_DEVICE_HANDLER( grip_ppi8255_b_r )
{
	/*

        bit		description

        PB0     Keyboard input
        PB1     Keyboard input
        PB2     Keyboard input
        PB3     Keyboard input
        PB4     Keyboard input
        PB5     Keyboard input
        PB6     Keyboard input
        PB7     Keyboard input
    
	*/

	prof80_state *state = device->machine->driver_data;

	return state->keydata;
}

static WRITE8_DEVICE_HANDLER( grip_ppi8255_c_w )
{
	/*

        bit		signal		description

		PC0     INTRB		interrupt B output (keyboard)
        PC1     KBF			input buffer B full output (keyboard)
        PC2     _KBSTB		strobe B input (keyboard)
        PC3     INTRA		interrupt A output (PROF-80)
        PC4		_STBA		strobe A input (PROF-80)
        PC5     IBFA		input buffer A full output (PROF-80)
        PC6		_ACKA		acknowledge A input (PROF-80)
        PC7     _OBFA		output buffer full output (PROF-80)
    
	*/

	prof80_state *state = device->machine->driver_data;

	/* keyboard interrupt */
	z80sti_i4_w(state->z80sti, BIT(data, 0));

	/* keyboard buffer full */
	state->kbf = BIT(data, 1);

	/* PROF-80 interrupt */
	z80sti_i7_w(state->z80sti, BIT(data, 3));

	/* PROF-80 handshaking */
	state->gripc = (!BIT(data, 7) << 7) | (!BIT(data, 5) << 6) | (i8255a_pa_r(state->ppi8255, 0) & 0x3f);
}

static I8255A_INTERFACE( grip_ppi8255_interface )
{
	DEVCB_HANDLER(grip_ppi8255_a_r),	// Port A read
	DEVCB_HANDLER(grip_ppi8255_b_r),	// Port B read
	DEVCB_NULL,							// Port C read
	DEVCB_HANDLER(grip_ppi8255_a_w),	// Port A write
	DEVCB_NULL,							// Port B write
	DEVCB_HANDLER(grip_ppi8255_c_w)		// Port C write
};

/* Z80-STI Interface */

static READ8_DEVICE_HANDLER( grip_z80sti_gpio_r )
{
	/*

        bit		signal		description

        I0		CTS			RS-232 clear to send input
        I1		DE			MC6845 display enable input
        I2		CURSOR		MC6845 cursor input
        I3		BUSY		Centronics busy input
        I4		PC0			PPI8255 PC0 input
        I5		SKBD		Serial keyboard input
        I6		EXIN		External interrupt input
        I7		PC3			PPI8255 PC3 input
    
	*/

	prof80_state *state = device->machine->driver_data;

	return centronics_busy_r(state->centronics) << 3;
}

static WRITE_LINE_DEVICE_HANDLER( grip_z80sti_tbo_w )
{
	// connected to speaker
}

static WRITE_LINE_DEVICE_HANDLER( grip_z80sti_irq_w )
{
	cputag_set_input_line(device->machine, GRIP_Z80_TAG, INPUT_LINE_IRQ0, state);
}

static Z80STI_INTERFACE( grip_z80sti_interface )
{
	0,									/* serial receive clock */
	0,									/* serial transmit clock */
	DEVCB_DEVICE_HANDLER(Z80STI_TAG, grip_z80sti_gpio_r),	/* GPIO read */
	DEVCB_NULL,							/* GPIO write */
	DEVCB_NULL,							/* serial input */
	DEVCB_NULL,							/* serial output */
	DEVCB_NULL,							/* timer A output */
	DEVCB_LINE(grip_z80sti_tbo_w),		/* timer B output */
	DEVCB_LINE(z80sti_tc_w),			/* timer C output */
	DEVCB_LINE(z80sti_rc_w),			/* timer D output */
	DEVCB_LINE(grip_z80sti_irq_w)		/* interrupt */
};

/* Z80 Daisy Chain */

static const z80_daisy_chain grip_daisy_chain[] =
{
	{ Z80STI_TAG },
	{ NULL }
};

/* Machine Initialization */

static MACHINE_START( prof80 )
{
	prof80_state *state = machine->driver_data;
	int bank;

	/* find devices */
	state->nec765 = devtag_get_device(machine, NEC765_TAG);
	state->upd1990a = devtag_get_device(machine, UPD1990A_TAG);
	state->mc6845 = devtag_get_device(machine, MC6845_TAG);
	state->ppi8255 = devtag_get_device(machine, I8255A_TAG);
	state->z80sti = devtag_get_device(machine, Z80STI_TAG);
	state->centronics = devtag_get_device(machine, "centronics");

	/* initialize RTC */
	upd1990a_cs_w(state->upd1990a, 1);
	upd1990a_oe_w(state->upd1990a, 1);

	/* configure FDC */
	floppy_drive_set_index_pulse_callback(floppy_get_device(machine, 0), prof80_fdc_index_callback);

	/* allocate floppy motor off timer */
	state->floppy_motor_off_timer = timer_alloc(machine, floppy_motor_off_tick, NULL);

	/* allocate video RAM */
	state->video_ram = auto_alloc_array(machine, UINT8, GRIP_VIDEORAM_SIZE);

	/* setup PROF-80 memory banking */
	for (bank = 0; bank < 16; bank++)
	{
		UINT32 offset = bank * 0x1000;
		memory_configure_bank(machine, bank + 1, 0, 16, memory_region(machine, Z80_TAG) + offset, 0x10000);
	}

	/* setup GRIP memory banking */
	memory_configure_bank(machine, 17, 0, 2, state->video_ram, 0x8000);
	memory_set_bank(machine, 17, 0);

	/* bank switch */
	prof80_bankswitch(machine);

	/* register for state saving */
	state_save_register_global_array(machine, state->mmu);
	state_save_register_global(machine, state->mme);
	state_save_register_global(machine, state->keydata);
	state_save_register_global(machine, state->kbf);
	state_save_register_global_pointer(machine, state->video_ram, GRIP_VIDEORAM_SIZE);
	state_save_register_global(machine, state->lps);
	state_save_register_global(machine, state->page);
	state_save_register_global(machine, state->flash);
	state_save_register_global(machine, state->rtc_data);
	state_save_register_global(machine, state->fdc_index);
	state_save_register_global(machine, state->gripd);
	state_save_register_global(machine, state->gripc);
}

static MACHINE_RESET( prof80 )
{
	prof80_state *state = machine->driver_data;

	int i;

	for (i = 0; i < 8; i++)
	{
		ls259_w(machine, 0, i, 0, i);
	}

	state->gripc = 0x40;
}

static FLOPPY_OPTIONS_START(prof80)
	// dsk
FLOPPY_OPTIONS_END

static const floppy_config prof80_floppy_config =
{
	FLOPPY_DRIVE_DS_80,
	FLOPPY_OPTIONS_NAME(prof80),
	DO_NOT_KEEP_GEOMETRY
};

/* Machine Drivers */
static MACHINE_DRIVER_START( prof80 )
	MDRV_DRIVER_DATA(prof80_state)

    /* basic machine hardware */
    MDRV_CPU_ADD(Z80_TAG, Z80, XTAL_6MHz)
    MDRV_CPU_PROGRAM_MAP(prof80_mem)
    MDRV_CPU_IO_MAP(prof80_io)

    MDRV_CPU_ADD(GRIP_Z80_TAG, Z80, XTAL_16MHz/4)
	MDRV_CPU_CONFIG(grip_daisy_chain)
    MDRV_CPU_PROGRAM_MAP(grip_mem)
    MDRV_CPU_IO_MAP(grip_io)

	MDRV_MACHINE_START(prof80)
	MDRV_MACHINE_RESET(prof80)

	/* keyboard hack */
	MDRV_TIMER_ADD_PERIODIC("keyboard", prof80_keyboard_tick, HZ(50))

    /* video hardware */
    MDRV_SCREEN_ADD(SCREEN_TAG, RASTER)
    MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 639, 0, 479)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)
 
    MDRV_VIDEO_START(prof80)
    MDRV_VIDEO_UPDATE(prof80)

	MDRV_MC6845_ADD(MC6845_TAG, MC6845, XTAL_16MHz/4, grip_mc6845_interface)

	/* devices */
	MDRV_UPD1990A_ADD(UPD1990A_TAG, XTAL_32_768kHz, prof80_upd1990a_intf)
	MDRV_NEC765A_ADD(NEC765_TAG, prof80_nec765_interface)
	MDRV_I8255A_ADD(I8255A_TAG, grip_ppi8255_interface)
	MDRV_Z80STI_ADD(Z80STI_TAG, XTAL_16MHz/4, grip_z80sti_interface)

	/* printer */
	MDRV_CENTRONICS_ADD("centronics", standard_centronics)
	
	MDRV_FLOPPY_4_DRIVES_ADD(prof80_floppy_config)
MACHINE_DRIVER_END

/* ROMs */

ROM_START( prof80 )
	ROM_REGION( 0x100000, Z80_TAG, 0 )
	ROM_DEFAULT_BIOS( "v17" )
	
	ROM_SYSTEM_BIOS( 0, "v15", "v1.5" )
	ROMX_LOAD( "prof80v15.z7", 0xf0000, 0x02000, CRC(8f74134c) SHA1(83f9dcdbbe1a2f50006b41d406364f4d580daa1f), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "v16", "v1.6" )
	ROMX_LOAD( "prof80v16.z7", 0xf0000, 0x02000, CRC(7d3927b3) SHA1(bcc15fd04dbf1d6640115be595255c7b9d2a7281), ROM_BIOS(2) )
	ROM_SYSTEM_BIOS( 2, "v17", "v1.7" )
	ROMX_LOAD( "prof80v17.z7", 0xf0000, 0x02000, CRC(53305ff4) SHA1(3ea209093ac5ac8a5db618a47d75b705965cdf44), ROM_BIOS(3) )

	ROM_COPY( Z80_TAG, 0xf0000, 0xf2000, 0x02000 )
	ROM_COPY( Z80_TAG, 0xf0000, 0xf4000, 0x04000 )
	ROM_COPY( Z80_TAG, 0xf0000, 0xf8000, 0x08000 ) // block 15
	ROM_COPY( Z80_TAG, 0xf0000, 0xe0000, 0x10000 ) // block 14
	ROM_COPY( Z80_TAG, 0xf0000, 0x70000, 0x10000 ) // block 7
	ROM_COPY( Z80_TAG, 0xf0000, 0x60000, 0x10000 ) // block 6

	ROM_REGION( 0x10000, GRIP_Z80_TAG, 0 )
	ROM_LOAD( "grip21.z2", 0x0000, 0x4000, CRC(7f6a37dd) SHA1(2e89f0b0c378257ff7e41c50d57d90865c6e214b) )
	ROM_LOAD( "grip26.z2", 0x0000, 0x4000, CRC(a1c424f0) SHA1(83942bc75b9475f044f936b8d9d7540551d87db9) )
	ROM_LOAD( "grip31.z2", 0x0000, 0x4000, CRC(e0e4e8ab) SHA1(73d3d14c9b06fed0c187fb0fffe5ec035d8dd256) )
	ROM_LOAD( "grip25.z2", 0x0000, 0x4000, CRC(49ebb284) SHA1(0a7eaaf89da6db2750f820146c8f480b7157c6c7) )

	ROM_REGION( 0x8000, "grip5", 0 )
	ROM_LOAD( "grip562.z2", 0x0000, 0x8000, CRC(74be0455) SHA1(1c423ecca6363345a8690ddc45dbafdf277490d3) )
ROM_END

/* System Configurations */
static SYSTEM_CONFIG_START( prof80 )
	CONFIG_RAM_DEFAULT	(128 * 1024)
SYSTEM_CONFIG_END

/* System Drivers */

/*    YEAR  NAME   PARENT  COMPAT  MACHINE INPUT   INIT  CONFIG COMPANY  FULLNAME   FLAGS */
COMP( 1984, prof80,     0,      0, prof80, prof80, 0, prof80,  "Conitec Datensysteme", "PROF-80", GAME_NO_SOUND | GAME_NOT_WORKING)

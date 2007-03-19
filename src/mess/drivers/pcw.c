/******************************************************************************

	pcw.c
	system driver

	Kevin Thacker [MESS driver]

	Thankyou to Jacob Nevins, Richard Fairhurst and Cliff Lawson,
	for their documentation that I used for the development of this
	driver.

	PCW came in 4 forms.
	PCW8256, PCW8512, PCW9256, PCW9512

    These systems were nicknamed "Joyce", apparently after a secretary who worked at
	Amstrad plc.

    These machines were designed for wordprocessing and other business applications.

    The computer came with Locoscript (wordprocessor by Locomotive Software Ltd),
	and CP/M+ (3.1).

    The original PCW8256 system came with a keyboard, green-screen monitor
	(which had 2 3" 80 track, double sided disc drives mounted vertically in it),
	and a dedicated printer. The other systems had different design but shared the
	same hardware.

    Since it was primarily designed as a wordprocessor, there were not many games
	written.
	Some of the games available:
	 - Head Over Heels
	 - Infocom adventures (Hitchhikers Guide to the Galaxy etc)

    However, it can use the CP/M OS, there is a large variety of CPM software that will
	run on it!!!!!!!!!!!!!!

    Later systems had:
		- black/white monitor,
		- dedicated printer was removed, and support for any printer was added
		- 3" internal drive replaced by a 3.5" drive

    All the logic for the system, except the FDC was found in a Amstrad designed custom
	chip.

    In the original PCW8256, there was no boot-rom. A boot-program was stored in the printer
	chip, and this was activated when the PCW was first switched on. AFAIK there are no
	dumps of this program, so I have written my own replacement.

    The boot-program performs a simple task: Load sector 0, track 0 to &f000 in ram, and execute
	&f010.

    From here CP/M is booted, and the appropiate programs can be run.

    The hardware:
	   - Z80 CPU running at 3.4Mhz
       - NEC765 FDC
	   - mono display
	   - beep (a fixed hz tone which can be turned on/off)
	   - 720x256 (PAL) bitmapped display, 720x200 (NTSC) bitmapped display
	   - Amstrad CPC6128 style keyboard

  If there are special roms for any of the PCW series I would be interested in them
  so I can implement the driver properly.

  From comp.sys.amstrad.8bit FAQ:

  "Amstrad made the following PCW systems :

  - 1) PCW8256
  - 2) PCW8512
  - 3) PCW9512
  - 4) PCW9512+
  - 5) PcW10
  - 6) PcW16

  1 had 180K drives, 2 had a 180K A drive and a 720K B drive, 3 had only
  720K drives. All subsequent models had 3.5" disks using CP/M format at
  720K until 6 when it switched to 1.44MB in MS-DOS format. The + of
  model 4 was that it had a "real" parallel interface so could be sold
  with an external printer such as the Canon BJ10. The PcW10 wasn't
  really anything more than 4 in a more modern looking case.

  The PcW16 is a radical digression who's sole "raison d'etre" was to
  make a true WYSIWYG product but this meant a change in the screen and
  processor (to 16MHz) etc. which meant that it could not be kept
  compatible with the previous models (though documents ARE compatible)"


  TODO:
  - Printer hardware emulation (8256 etc)
  - Parallel port emulation (9512, 9512+, 10)
  - emulation of serial hardware
  - emulation of other hardware...?
 ******************************************************************************/
#include "driver.h"
// nec765 interface
#include "machine/nec765.h"
#include "devices/dsk.h"
// pcw video hardware
#include "includes/pcw.h"
// pcw/pcw16 beeper
#include "image.h"
#include "sound/beep.h"

// uncomment for debug log output
//#define VERBOSE

void pcw_fdc_interrupt(int);

// pointer to pcw ram
unsigned int roller_ram_addr;
// flag to indicate if boot-program is enabled/disabled
static int 	pcw_boot;
int	pcw_system_status;
unsigned short roller_ram_offset;
// code for CPU int type generated when FDC int is triggered
static int fdc_interrupt_code;
unsigned char pcw_vdu_video_control_register;
static int pcw_interrupt_counter;

static unsigned long pcw_banks[4];
static unsigned char pcw_bank_force = 0;


static void pcw_update_interrupt_counter(void)
{
	/* never increments past 15! */
	if (pcw_interrupt_counter==0x0f)
		return;

	/* increment count */
	pcw_interrupt_counter++;
}

/* PCW uses NEC765 in NON-DMA mode. FDC Ints are connected to /INT or
/NMI depending on choice (see system control below) */
static nec765_interface pcw_nec765_interface =
{
	pcw_fdc_interrupt,
	NULL
};

/* determines if int line is held or cleared */
static void pcw_interrupt_handle(void)
{
	if (
		(pcw_interrupt_counter!=0) ||
		((fdc_interrupt_code==1) && ((pcw_system_status & (1<<5))!=0))
		)
	{
		cpunum_set_input_line(0,0,HOLD_LINE);
	}
	else
	{
		cpunum_set_input_line(0,0,CLEAR_LINE);
	}
}



/* callback for 1/300ths of a second interrupt */
static void pcw_timer_interrupt(int dummy)
{
	pcw_update_interrupt_counter();

	pcw_interrupt_handle();
}

static int previous_fdc_int_state;

/* set/clear fdc interrupt */
static void	pcw_trigger_fdc_int(void)
{
	int state;

	state = pcw_system_status & (1<<5);

	switch (fdc_interrupt_code)
	{
		/* attach fdc to nmi */
		case 0:
		{
			/* I'm assuming that the nmi is edge triggered */
			/* a interrupt from the fdc will cause a change in line state, and
			the nmi will be triggered, but when the state changes because the int
			is cleared this will not cause another nmi */
			/* I'll emulate it like this to be sure */

			if (state!=previous_fdc_int_state)
			{
				if (state)
				{
					/* I'll pulse it because if I used hold-line I'm not sure
					it would clear - to be checked */
					cpunum_set_input_line(0, INPUT_LINE_NMI, PULSE_LINE);
				}
			}
		}
		break;

		/* attach fdc to int */
		case 1:
		{

			pcw_interrupt_handle();
		}
		break;

		/* do not interrupt */
		default:
			break;
	}

	previous_fdc_int_state = state;
}

/* fdc interrupt callback. set/clear fdc int */
void pcw_fdc_interrupt(int state)
{
	pcw_system_status &= ~(1<<5);

	if (state)
	{
		/* fdc interrupt occured */
		pcw_system_status |= (1<<5);
	}

	pcw_trigger_fdc_int();
}


/* Memory is banked in 16k blocks.

	The upper 16 bytes of block 3, contains the keyboard
	state. This is updated by the hardware.

	block 3 could be paged into any bank, and this explains the
	setup of the memory below.
*/
static ADDRESS_MAP_START(pcw_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_READWRITE(MRA8_BANK1, MWA8_BANK5)
	AM_RANGE(0x4000, 0x7fff) AM_READWRITE(MRA8_BANK2, MWA8_BANK6)
	AM_RANGE(0x8000, 0xbfff) AM_READWRITE(MRA8_BANK3, MWA8_BANK7)
	AM_RANGE(0xc000, 0xffff) AM_READWRITE(MRA8_BANK4, MWA8_BANK8)
ADDRESS_MAP_END


/* PCW keyboard is mapped into memory */
static  READ8_HANDLER(pcw_keyboard_r)
{
	return readinputport(offset);
}


/* -----------------------------------------------------------------------
 * PCW Banking
 * ----------------------------------------------------------------------- */

static void pcw_update_read_memory_block(int block, int bank)
{
	memory_set_bankptr(block + 1, mess_ram + ((bank * 0x4000) % mess_ram_size));

	/* bank 3? */
	if (bank == 3)
	{
		/* when upper 16 bytes are accessed use keyboard read
		   handler */
		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM,
			block * 0x04000 + 0x3ff0, block * 0x04000 + 0x3fff, 0, 0,
			pcw_keyboard_r);
	}
	else
	{
		/* restore bank handler across entire block */
		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM,
			block * 0x04000 + 0x0000, block * 0x04000 + 0x3fff, 0, 0,
			(read8_handler) (STATIC_BANK1 + block));
	}
}



static void pcw_update_write_memory_block(int block, int bank)
{
	memory_set_bankptr(block + 5, mess_ram + ((bank * 0x4000) % mess_ram_size));
}


/* ----------------------------------------------------------------------- */

/* &F4 O  b7-b4: when set, force memory reads to access the same bank as
writes for &C000, &0000, &8000, and &4000 respectively */

static void pcw_update_mem(int block, int data)
{
	/* expansion ram select.
		if block is 0-7, selects internal ram instead for read/write
		*/
	if (data & 0x080)
	{
		int bank;

		/* same bank for reading and writing */
		bank = data & 0x7f;

		pcw_update_read_memory_block(block, bank);
		pcw_update_write_memory_block(block, bank);
	}
	else
	{
		/* specify a different bank for reading and writing */
		int write_bank;
		int read_bank;
		int mask=0;

		switch (block)
		{
			case 0:
			{
				mask = (1<<6);
			}
			break;

			case 1:
			{
				mask = (1<<4);
			}
			break;

			case 2:
			{
				mask = (1<<5);
			}
			break;

			case 3:
			{
				mask = (1<<7);
			}
			break;
		}

		if (pcw_bank_force & mask)
		{
			read_bank = data & 0x07;
		}
		else
		{
			read_bank = (data>>4) & 0x07;
		}

		pcw_update_read_memory_block(block, read_bank);

		write_bank = data & 0x07;
		pcw_update_write_memory_block(block, write_bank);
	}

	/* if boot is active, page in fake ROM */
	if ((pcw_boot) && (block==0))
	{
		unsigned char *FakeROM;

		FakeROM = &memory_region(REGION_CPU1)[0x010000];

		memory_set_bankptr(1, FakeROM);
	}
}

/* from Jacob Nevins docs */
static int pcw_get_sys_status(void)
{
	return pcw_interrupt_counter | (readinputport(16) & (0x040 | 0x010)) | pcw_system_status;
}

static  READ8_HANDLER(pcw_interrupt_counter_r)
{
	int data;

	/* from Jacob Nevins docs */

	/* get data */
	data = pcw_get_sys_status();
	/* clear int counter */
	pcw_interrupt_counter = 0;
	/* update interrupt */
	pcw_interrupt_handle();
	/* return data */
	return data;
}


static WRITE8_HANDLER(pcw_bank_select_w)
{
#ifdef VERBOSE
	logerror("BANK: %2x %x\n",offset, data);
#endif
	pcw_banks[offset] = data;

	pcw_update_mem(offset, data);
}

static WRITE8_HANDLER(pcw_bank_force_selection_w)
{
	pcw_bank_force = data;

	pcw_update_mem(0, pcw_banks[0]);
	pcw_update_mem(1, pcw_banks[1]);
	pcw_update_mem(2, pcw_banks[2]);
	pcw_update_mem(3, pcw_banks[3]);
}


static WRITE8_HANDLER(pcw_roller_ram_addr_w)
{
	/*
	Address of roller RAM. b7-5: bank (0-7). b4-1: address / 512. */

	roller_ram_addr = (((data>>5) & 0x07)<<14) |
							((data & 0x01f)<<9);
}

static WRITE8_HANDLER(pcw_pointer_table_top_scan_w)
{
	roller_ram_offset = data;
}

static WRITE8_HANDLER(pcw_vdu_video_control_register_w)
{
	pcw_vdu_video_control_register = data;
}

static WRITE8_HANDLER(pcw_system_control_w)
{
#ifdef VERBOSE
	logerror("SYSTEM CONTROL: %d\n",data);
#endif

	switch (data)
	{
		/* end bootstrap */
		case 0:
		{
			pcw_boot = 0;
			pcw_update_mem(0, pcw_banks[0]);
		}
		break;

		/* reboot */
		case 1:
		{
		}
		break;

		/* connect fdc interrupt to nmi */
		case 2:
		{
			int fdc_previous_interrupt_code = fdc_interrupt_code;

			fdc_interrupt_code = 0;

			/* previously connected to INT? */
			if (fdc_previous_interrupt_code == 1)
			{
				/* yes */

				pcw_interrupt_handle();
			}

			pcw_trigger_fdc_int();
		}
		break;


		/* connect fdc interrupt to interrupt */
		case 3:
		{
			int fdc_previous_interrupt_code = fdc_interrupt_code;

			/* connect to INT */
			fdc_interrupt_code = 1;

			/* previously connected to NMI? */
			if (fdc_previous_interrupt_code == 0)
			{
				/* yes */

				/* clear nmi interrupt */
				cpunum_set_input_line(0, INPUT_LINE_NMI, CLEAR_LINE);
			}

			/* re-issue interrupt */
			pcw_interrupt_handle();
		}
		break;


		/* connect fdc interrupt to neither */
		case 4:
		{
			int fdc_previous_interrupt_code = fdc_interrupt_code;

			fdc_interrupt_code = 2;

			/* previously connected to NMI or INT? */
			if ((fdc_previous_interrupt_code == 0) || (fdc_previous_interrupt_code == 1))
			{
				/* yes */

				/* Clear NMI */
				cpunum_set_input_line(0, INPUT_LINE_NMI, CLEAR_LINE);

			}

			pcw_interrupt_handle();
		}
		break;

		/* set fdc terminal count */
		case 5:
		{
			nec765_set_tc_state(1);
		}
		break;

		/* clear fdc terminal count */
		case 6:
		{
			nec765_set_tc_state(0);
		}
		break;

		/* screen on */
		case 7:
		{


		}
		break;

		/* screen off */
		case 8:
		{

		}
		break;

		/* disc motor on */
		case 9:
		{
			floppy_drive_set_motor_state(image_from_devtype_and_index(IO_FLOPPY, 0), 1);
			floppy_drive_set_motor_state(image_from_devtype_and_index(IO_FLOPPY, 1), 1);
			floppy_drive_set_ready_state(image_from_devtype_and_index(IO_FLOPPY, 0), 1,1);
			floppy_drive_set_ready_state(image_from_devtype_and_index(IO_FLOPPY, 1), 1,1);
		}
		break;

		/* disc motor off */
		case 10:
		{
			floppy_drive_set_motor_state(image_from_devtype_and_index(IO_FLOPPY, 0), 0);
			floppy_drive_set_motor_state(image_from_devtype_and_index(IO_FLOPPY, 1), 0);
			floppy_drive_set_ready_state(image_from_devtype_and_index(IO_FLOPPY, 0), 1,1);
			floppy_drive_set_ready_state(image_from_devtype_and_index(IO_FLOPPY, 1), 1,1);
		}
		break;

		/* beep on */
		case 11:
		{
			beep_set_state(0,1);
		}
		break;

		/* beep off */
		case 12:
		{
			beep_set_state(0,0);
		}
		break;

	}
}

static  READ8_HANDLER(pcw_system_status_r)
{
	/* from Jacob Nevins docs */
	return pcw_get_sys_status();
}

/* read from expansion hardware - additional hardware not part of
the PCW custom ASIC */
static  READ8_HANDLER(pcw_expansion_r)
{
	logerror("pcw expansion r: %04x\n",offset+0x080);

	switch (offset-0x080)
	{
		case 0x0e0:
		{
			/* spectravideo joystick */
			if (readinputport(16) & 0x020)
			{
				return readinputport(17);
			}
			else
			{
				return 0x0ff;
			}
		}

		case 0x09f:
		{

			/* kempston joystick */
			return readinputport(18);
		}

		case 0x0e1:
		{
			return 0x07f;
		}

		case 0x085:
		{
			return 0x0fe;
		}

		case 0x087:
		{

			return 0x0ff;
		}

	}

	/* result from floating bus/no peripherial at this port */
	return 0x0ff;
}

/* write to expansion hardware - additional hardware not part of
the PCW custom ASIC */
static WRITE8_HANDLER(pcw_expansion_w)
{
	logerror("pcw expansion w: %04x %02x\n",offset+0x080, data);






}

static  READ8_HANDLER(pcw_fdc_r)
{
	/* from Jacob Nevins docs. FDC I/O is not fully decoded */
	if (offset & 1)
	{
		return nec765_data_r(0);
	}

	return nec765_status_r(0);
}

static WRITE8_HANDLER(pcw_fdc_w)
{
	/* from Jacob Nevins docs. FDC I/O is not fully decoded */
	if (offset & 1)
	{
		nec765_data_w(0,data);
	}
}

/* TODO: Implement the printer for PCW8256, PCW8512,PCW9256*/
static WRITE8_HANDLER(pcw_printer_data_w)
{
}

static WRITE8_HANDLER(pcw_printer_command_w)
{
}

static  READ8_HANDLER(pcw_printer_data_r)
{
	return 0x0ff;
}

static  READ8_HANDLER(pcw_printer_status_r)
{
	return 0x0ff;
}

/* TODO: Implement parallel port! */
static  READ8_HANDLER(pcw9512_parallel_r)
{
	if (offset==1)
	{
		return 0xff^0x020;
	}

	logerror("pcw9512 parallel r: offs: %04x\n", (int) offset);
	return 0x00;
}

/* TODO: Implement parallel port! */
static WRITE8_HANDLER(pcw9512_parallel_w)
{
	logerror("pcw9512 parallel w: offs: %04x data: %02x\n",offset,data);
}


static ADDRESS_MAP_START(pcw_io, ADDRESS_SPACE_IO, 8)
	AM_RANGE(0x000, 0x07f) AM_READWRITE(pcw_fdc_r,					pcw_fdc_w)
	AM_RANGE(0x080, 0x0ef) AM_READWRITE(pcw_expansion_r,			pcw_expansion_w)
	AM_RANGE(0x0f0, 0x0f3) AM_WRITE(								pcw_bank_select_w)
	AM_RANGE(0x0f4, 0x0f4) AM_READWRITE(pcw_interrupt_counter_r,	pcw_bank_force_selection_w)	
	AM_RANGE(0x0f5, 0x0f5) AM_WRITE(								pcw_roller_ram_addr_w)
	AM_RANGE(0x0f6, 0x0f6) AM_WRITE(								pcw_pointer_table_top_scan_w)
	AM_RANGE(0x0f7, 0x0f7) AM_WRITE(								pcw_vdu_video_control_register_w)
	AM_RANGE(0x0f8, 0x0f8) AM_READWRITE(pcw_system_status_r,		pcw_system_control_w)
	AM_RANGE(0x0fc, 0x0fc) AM_READWRITE(pcw_printer_data_r,			pcw_printer_data_w)
	AM_RANGE(0x0fd, 0x0fd) AM_READWRITE(pcw_printer_status_r,		pcw_printer_command_w)
ADDRESS_MAP_END



static ADDRESS_MAP_START(pcw9512_io, ADDRESS_SPACE_IO, 8)
	AM_RANGE(0x000, 0x07f) AM_READWRITE(pcw_fdc_r,					pcw_fdc_w)
	AM_RANGE(0x080, 0x0ef) AM_READWRITE(pcw_expansion_r,			pcw_expansion_w)
	AM_RANGE(0x0f0, 0x0f3) AM_WRITE(								pcw_bank_select_w)
	AM_RANGE(0x0f4, 0x0f4) AM_READWRITE(pcw_interrupt_counter_r,	pcw_bank_force_selection_w)	
	AM_RANGE(0x0f5, 0x0f5) AM_WRITE(								pcw_roller_ram_addr_w)
	AM_RANGE(0x0f6, 0x0f6) AM_WRITE(								pcw_pointer_table_top_scan_w)
	AM_RANGE(0x0f7, 0x0f7) AM_WRITE(								pcw_vdu_video_control_register_w)
	AM_RANGE(0x0f8, 0x0f8) AM_READWRITE(pcw_system_status_r,		pcw_system_control_w)
	AM_RANGE(0x0fc, 0x0fd) AM_READWRITE(pcw9512_parallel_r,			pcw9512_parallel_w)
ADDRESS_MAP_END



static void setup_beep(int dummy)
{
	beep_set_state(0, 0);
	beep_set_frequency(0, 3750);
}



static DRIVER_INIT(pcw)
{
	pcw_boot = 1;

	cpunum_set_input_line_vector(0, 0,0x0ff);

    nec765_init(&pcw_nec765_interface,NEC765A);

	/* ram paging is actually undefined at power-on */
	pcw_banks[0] = 0;
	pcw_banks[1] = 1;
	pcw_banks[2] = 2;
	pcw_banks[3] = 3;

	pcw_update_mem(0, pcw_banks[0]);
	pcw_update_mem(1, pcw_banks[1]);
	pcw_update_mem(2, pcw_banks[2]);
	pcw_update_mem(3, pcw_banks[3]);

	/* lower 4 bits are interrupt counter */
	pcw_system_status = 0x000;
	pcw_system_status &= ~((1<<6) | (1<<5) | (1<<4));

	pcw_interrupt_counter = 0;
	fdc_interrupt_code = 0;

	floppy_drive_set_geometry(image_from_devtype_and_index(IO_FLOPPY, 0), FLOPPY_DRIVE_DS_80);

	roller_ram_offset = 0;

	/* timer interrupt */
	timer_pulse(TIME_IN_HZ(300), 0, pcw_timer_interrupt);

	timer_set(0.0, 0, setup_beep);
}


/*
b7:   k2     k1     [+]    .      ,      space  V      X      Z      del<   alt
b6:   k3     k5     1/2    /      M      N      B      C      lock          k.
b5:   k6     k4     shift  ;      K      J      F      D      A             enter
b4:   k9     k8     k7     §      L      H      G      S      tab           f8
b3:   paste  copy   #      P      I      Y      T      W      Q             [-]
b2:   f2     cut    return [      O      U      R      E      stop          can
b1:   k0     ptr    ]      -      9      7      5      3      2             extra
b0:   f4     exit   del>   =      0      8      6      4      1             f6
      &3FF0  &3FF1  &3FF2  &3FF3  &3FF4  &3FF5  &3FF6  &3FF7  &3FF8  &3FF9  &3FFA
*/

INPUT_PORTS_START(pcw)
	/* keyboard "ports". These are poked automatically into the PCW address space */

	/* 0x03ff0 */
	PORT_START
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F4") PORT_CODE(KEYCODE_F4)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("K0") PORT_CODE(KEYCODE_0_PAD)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F2") PORT_CODE(KEYCODE_F2)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("PASTE") PORT_CODE(KEYCODE_PGUP)
	PORT_BIT(0x010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("K9") PORT_CODE(KEYCODE_9_PAD)
	PORT_BIT(0x020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("K6") PORT_CODE(KEYCODE_6_PAD)
	PORT_BIT(0x040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("K3") PORT_CODE(KEYCODE_3_PAD)
	PORT_BIT(0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("K2") PORT_CODE(KEYCODE_2_PAD)

	/* 0x03ff1 */
	PORT_START
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("EXIT") PORT_CODE(KEYCODE_ESC)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("PTR") PORT_CODE(KEYCODE_PGUP)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("CUT") PORT_CODE(KEYCODE_INSERT)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("COPY") PORT_CODE(KEYCODE_HOME)
	PORT_BIT(0x010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("K8") PORT_CODE(KEYCODE_8_PAD)
	PORT_BIT(0x020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("K4") PORT_CODE(KEYCODE_4_PAD)
	PORT_BIT(0x040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("K5") PORT_CODE(KEYCODE_5_PAD)
	PORT_BIT(0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("K1") PORT_CODE(KEYCODE_1_PAD)

	/* 0x03ff2 */
	PORT_START
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("DEL>") PORT_CODE(KEYCODE_DEL)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("]") PORT_CODE(KEYCODE_CLOSEBRACE)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("RETURN") PORT_CODE(KEYCODE_ENTER)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("#") PORT_CODE(KEYCODE_TILDE)
	PORT_BIT(0x010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("K7") PORT_CODE(KEYCODE_7_PAD)
	PORT_BIT(0x020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("SHIFT") PORT_CODE(KEYCODE_LSHIFT)
	PORT_BIT(0x020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("SHIFT") PORT_CODE(KEYCODE_RSHIFT)
	PORT_BIT (0x040, 0x000, IPT_UNUSED)
	PORT_BIT(0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("+") PORT_CODE(KEYCODE_PLUS_PAD)

	/* 0x03ff3 */
	PORT_START
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("=") PORT_CODE(KEYCODE_EQUALS)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("-") PORT_CODE(KEYCODE_MINUS)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("[") PORT_CODE(KEYCODE_OPENBRACE)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("P") PORT_CODE(KEYCODE_P)
	PORT_BIT(0x010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("§") PORT_CODE(KEYCODE_QUOTE)
	PORT_BIT(0x020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(";") PORT_CODE(KEYCODE_COLON)
	PORT_BIT(0x040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("/") PORT_CODE(KEYCODE_SLASH)
	PORT_BIT(0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(".") PORT_CODE(KEYCODE_STOP)

	/* 0x03ff4 */
	PORT_START
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("0") PORT_CODE(KEYCODE_0)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("9") PORT_CODE(KEYCODE_9)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("O") PORT_CODE(KEYCODE_O)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("I") PORT_CODE(KEYCODE_I)
	PORT_BIT(0x010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("L") PORT_CODE(KEYCODE_L)
	PORT_BIT(0x020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("K") PORT_CODE(KEYCODE_K)
	PORT_BIT(0x040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("M") PORT_CODE(KEYCODE_M)
	PORT_BIT(0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(",") PORT_CODE(KEYCODE_COMMA)

	/* 0x03ff5 */
	PORT_START
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("8") PORT_CODE(KEYCODE_8)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("7") PORT_CODE(KEYCODE_7)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("U") PORT_CODE(KEYCODE_U)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Y") PORT_CODE(KEYCODE_Y)
	PORT_BIT(0x010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("H") PORT_CODE(KEYCODE_H)
	PORT_BIT(0x020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("J") PORT_CODE(KEYCODE_J)
	PORT_BIT(0x040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("N") PORT_CODE(KEYCODE_N)
	PORT_BIT(0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("SPACE") PORT_CODE(KEYCODE_SPACE)

	/* 0x03ff6 */
	PORT_START
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("6") PORT_CODE(KEYCODE_6)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("5") PORT_CODE(KEYCODE_5)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("R") PORT_CODE(KEYCODE_R)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("T") PORT_CODE(KEYCODE_T)
	PORT_BIT(0x010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("G") PORT_CODE(KEYCODE_G)
	PORT_BIT(0x020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F") PORT_CODE(KEYCODE_F)
	PORT_BIT(0x040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("B") PORT_CODE(KEYCODE_B)
	PORT_BIT(0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("V") PORT_CODE(KEYCODE_V)

	/* 0x03ff7 */
	PORT_START
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("4") PORT_CODE(KEYCODE_4)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("3") PORT_CODE(KEYCODE_3)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("E") PORT_CODE(KEYCODE_E)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("W") PORT_CODE(KEYCODE_W)
	PORT_BIT(0x010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("S") PORT_CODE(KEYCODE_S)
	PORT_BIT(0x020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("D") PORT_CODE(KEYCODE_D)
	PORT_BIT(0x040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("C") PORT_CODE(KEYCODE_C)
	PORT_BIT(0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("X") PORT_CODE(KEYCODE_X)

	/* 0x03ff8 */
	PORT_START
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("1") PORT_CODE(KEYCODE_1)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("2") PORT_CODE(KEYCODE_2)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("STOP")
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Q") PORT_CODE(KEYCODE_Q)
	PORT_BIT(0x010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("TAB") PORT_CODE(KEYCODE_TAB)
	PORT_BIT(0x020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("A") PORT_CODE(KEYCODE_A)
	PORT_BIT(0x040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("LOCK")
	PORT_BIT(0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Z") PORT_CODE(KEYCODE_Z)

	/* 0x03ff9 */
	PORT_START
	PORT_BIT(0x07f,0x00, IPT_UNUSED)
	PORT_BIT(0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("DEL<") PORT_CODE(KEYCODE_BACKSPACE)

	/* 0x03ffa */
	PORT_START
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F6") PORT_CODE(KEYCODE_6_PAD)
	PORT_BIT (0x02, 0x00, IPT_UNUSED)
	PORT_BIT (0x04, 0x00, IPT_UNUSED)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("-") PORT_CODE(KEYCODE_MINUS_PAD)
	PORT_BIT(0x010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F8") PORT_CODE(KEYCODE_8_PAD)
	PORT_BIT(0x020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("ENTER") PORT_CODE(KEYCODE_ENTER_PAD)
	PORT_BIT(0x040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("K.") PORT_CODE(KEYCODE_DEL_PAD)
	PORT_BIT(0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("ALT") PORT_CODE(KEYCODE_LALT)
	PORT_BIT(0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("ALT") PORT_CODE(KEYCODE_RALT)

	/* at this point the following reflect the above key combinations but in a incomplete
	way. No details available at this time */
	/* 0x03ffb */
	PORT_START
	PORT_BIT ( 0xff, 0x00,	 IPT_UNUSED )

	/* 0x03ffc */
	PORT_START
	PORT_BIT ( 0xff, 0x00,	 IPT_UNUSED )

	/* 0x03ffd */
	PORT_START
	PORT_BIT ( 0x03f, 0x000, IPT_UNUSED)
	PORT_BIT(0x040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("SHIFT LOCK") PORT_CODE(KEYCODE_CAPSLOCK)
	PORT_BIT (0x080, 0x000, IPT_UNUSED)

	/* 0x03ffe */
	PORT_START
	PORT_BIT ( 0xff, 0x00,	 IPT_UNUSED )

	/* 0x03fff */
	PORT_START
	PORT_BIT ( 0xff, 0x00,	 IPT_UNUSED )

	/* from here on are the pretend dipswitches for machine config etc */
	PORT_START
	/* vblank */
	PORT_BIT( 0x040, IP_ACTIVE_HIGH, IPT_VBLANK)
	/* frame rate option */
	PORT_DIPNAME( 0x010, 0x010, "50/60Hz Frame Rate Option")
	PORT_DIPSETTING(	0x00, "60hz")
	PORT_DIPSETTING(	0x10, "50Hz" )
	/* spectravideo joystick enabled */
	PORT_DIPNAME( 0x020, 0x020, "Spectravideo Joystick Enabled")
	PORT_DIPSETTING(	0x00, DEF_STR(No))
	PORT_DIPSETTING(	0x20, DEF_STR(Yes) )

	/* Spectravideo joystick */
	/* bit 7: 0
    6: 1
    5: 1
    4: 1 if in E position
    3: 1 if N
    2: 1 if W
    1: 1 if fire pressed
    0: 1 if S
	*/

	PORT_START
	PORT_BIT(0x001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("JOYSTICK DOWN") PORT_CODE(JOYCODE_1_DOWN)
	PORT_BIT(0x002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("JOYSTICK FIRE") PORT_CODE(JOYCODE_1_BUTTON1)
	PORT_BIT(0x004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("JOYSTICK LEFT") PORT_CODE(JOYCODE_1_LEFT)
	PORT_BIT(0x008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("JOYSTICK UP") PORT_CODE(JOYCODE_1_UP)
	PORT_BIT(0x010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("JOYSTICK RIGHT") PORT_CODE(JOYCODE_1_RIGHT)
	PORT_BIT (0x020, 0x020, IPT_UNUSED)
	PORT_BIT (0x040, 0x040, IPT_UNUSED)
	PORT_BIT (0x080, 0x00, IPT_UNUSED)

INPUT_PORTS_END



/* PCW8256, PCW8512, PCW9256 */
static MACHINE_DRIVER_START( pcw )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, 4000000)       /* clock supplied to chip, but in reality it is 3.4Mhz */
	MDRV_CPU_PROGRAM_MAP(pcw_map, 0)
	MDRV_CPU_IO_MAP(pcw_io, 0)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1)

    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(PCW_SCREEN_WIDTH, PCW_SCREEN_HEIGHT)
	MDRV_SCREEN_VISIBLE_AREA(0, PCW_SCREEN_WIDTH-1, 0, PCW_SCREEN_HEIGHT-1)
	MDRV_PALETTE_LENGTH(PCW_NUM_COLOURS)
	MDRV_COLORTABLE_LENGTH(PCW_NUM_COLOURS)
	MDRV_PALETTE_INIT( pcw )

	MDRV_VIDEO_START( pcw )
	MDRV_VIDEO_UPDATE( pcw )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(BEEP, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
MACHINE_DRIVER_END


/* PCW9512, PCW9512+, PCW10 */
static MACHINE_DRIVER_START( pcw9512 )
	MDRV_IMPORT_FROM( pcw )
	MDRV_CPU_MODIFY( "main" )
	MDRV_CPU_IO_MAP(pcw9512_io, 0)
MACHINE_DRIVER_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

/* I am loading the boot-program outside of the Z80 memory area, because it
is banked. */

// for now all models use the same rom
#define ROM_PCW(model)												\
	ROM_START(model)												\
		ROM_REGION(0x014000, REGION_CPU1,0)							\
		ROM_LOAD("pcwboot.bin", 0x010000, 608, BAD_DUMP CRC(679b0287))	\
	ROM_END															\

ROM_PCW(pcw8256)
ROM_PCW(pcw8512)
ROM_PCW(pcw9256)
ROM_PCW(pcw9512)
ROM_PCW(pcw10)

static void generic_pcw_floppy_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* floppy */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 2; break;

		default:										legacydsk_device_getinfo(devclass, state, info); break;
	}
}

SYSTEM_CONFIG_START(generic_pcw)
	CONFIG_DEVICE(generic_pcw_floppy_getinfo)
SYSTEM_CONFIG_END

SYSTEM_CONFIG_START(pcw_256k)
	CONFIG_IMPORT_FROM(generic_pcw)
	CONFIG_RAM_DEFAULT(256 * 1024)
SYSTEM_CONFIG_END

SYSTEM_CONFIG_START(pcw_512k)
	CONFIG_IMPORT_FROM(generic_pcw)
	CONFIG_RAM_DEFAULT(512 * 1024)
SYSTEM_CONFIG_END

/* these are all variants on the pcw design */
/* major difference is memory configuration and drive type */
/*     YEAR	NAME	    PARENT		COMPAT	MACHINE   INPUT INIT	CONFIG		COMPANY 	   FULLNAME */
COMP( 1985, pcw8256,   0,			0,		pcw,	  pcw,	pcw,	pcw_256k,	"Amstrad plc", "PCW8256",		GAME_NOT_WORKING)
COMP( 1985, pcw8512,   pcw8256,	0,		pcw,	  pcw,	pcw,	pcw_512k,	"Amstrad plc", "PCW8512",		GAME_NOT_WORKING)
COMP( 1987, pcw9256,   pcw8256,	0,		pcw,	  pcw,	pcw,	pcw_256k,	"Amstrad plc", "PCW9256",		GAME_NOT_WORKING)
COMP( 1987, pcw9512,   pcw8256,	0,		pcw9512,  pcw,	pcw,	pcw_512k,	"Amstrad plc", "PCW9512 (+)",	GAME_NOT_WORKING)
COMP( 1993, pcw10,	    pcw8256,	0,		pcw9512,  pcw,	pcw,	pcw_512k,	"Amstrad plc", "PCW10",			GAME_NOT_WORKING)


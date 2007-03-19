/******************************************************************************

	pcw16.c
	system driver

	Kevin Thacker [MESS driver]

  Thankyou to:

	- Cliff Lawson @ Amstrad plc for his documentation (Anne ASIC documentation),
					and extensive help.
			(web.ukonline.co.uk/cliff.lawson/)
			(www.amstrad.com)
    - John Elliot for his help and tips
			(he's written a CP/M implementation for the PCW16)
			(www.seasip.deomon.co.uk)
	- and others who offered their help (Richard Fairhurst, Richard Wildey)

	Hardware:
		- 2mb dram max,
		- 2mb flash-file memory max (in 2 1mb chips),
		- 16Mhz Z80 (core combined in Anne ASIC),
		- Anne ASIC (keyboard interface, video (colours), dram/flash/rom paging,
		real time clock, "glue" logic for Super I/O)
		- Winbond Super I/O chip (PC type hardware - FDC, Serial, LPT, Hard-drive)
		- PC/AT keyboard - some keys are coloured to identify special functions, but
		these are the same as normal PC keys
		- PC Serial Mouse - uses Mouse System Mouse protocol
		- PC 1.44MB Floppy drive

    Primary Purpose:
		- built as a successor to the PCW8526/PCW9512 series
		- wordprocessor system (also contains spreadsheet and other office applications)
		- 16Mhz processor used so proportional fonts and enhanced wordprocessing features
		  are possible, true WYSIWYG wordprocessing.
		- flash-file can store documents.

    To Do:
		- reduce memory usage so it is more MESSD friendly
		- different configurations
		- implement configuration register
		- extract game-port hardware from pc driver - used in any PCW16 progs?
		- extract hard-drive code from PC driver and use in this driver
		- implement printer
		- .. anything else that requires implementing

	 Info:
	   - to use this driver you need a OS rescue disc.
	   (HINT: This also contains the boot-rom)
	  - the OS will be installed from the OS rescue disc into the Flash-ROM

	Uses "MEMCARD" dir to hold flash-file data.
	To use the power button, flick the dip switch off/on

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




 ******************************************************************************/
/* PeT 19.October 2000
   added/changed printer support
   not working reliable, seams to expect parallelport in epp/ecp mode
   epp/ecp modes in parallel port not supported yet
   so ui disabled */

#include "driver.h"
#include "includes/pcw16.h"

// PC-Parallel Port
#include "includes/pclpt.h"
#include "includes/centroni.h" // centronics printer handshake simulation
#include "devices/printer.h" // printer device
// PC-AT keyboard
#include "machine/pckeybrd.h"
// change to superio later
#include "machine/pc_fdc.h"
// for pc disk images
#include "devices/mflopimg.h"
#include "formats/pc_dsk.h"
// for pc com port
#include "machine/uart8250.h"
// for pc serial mouse
#include "includes/pc_mouse.h"
// pcw/pcw16 beeper
#include "sound/beep.h"

#include "includes/28f008sa.h"

static void pcw16_machine_stop(running_machine *machine);

// interrupt counter
unsigned long pcw16_interrupt_counter;
// video control
extern int pcw16_video_control;
/* controls which bank of 2mb address space is paged into memory */
static int pcw16_banks[4];

// output of 4-bit port from Anne ASIC
static int pcw16_4_bit_port;
// code defining which INT fdc is connected to
static int pcw16_fdc_int_code;
// interrupt bits
// bit 7: ??
// bit 6: fdc
// bit 5: ??
// bit 3,4; Serial
// bit 2: Vsync state
// bit 1: keyboard int
// bit 0: Display ints
static int pcw16_system_status;

// debugging - write ram as seen by cpu
static void pcw16_refresh_ints(void)
{
	/* any bits set excluding vsync */
	if ((pcw16_system_status & (~0x04))!=0)
	{
		cpunum_set_input_line(0,0, HOLD_LINE);
	}
	else
	{
		cpunum_set_input_line(0,0, CLEAR_LINE);
	}
}


static void pcw16_timer_callback(int dummy)
{
	/* do not increment past 15 */
	if (pcw16_interrupt_counter!=15)
	{
		pcw16_interrupt_counter++;
		/* display int */
		pcw16_system_status |= (1<<0);
	}

	if (pcw16_interrupt_counter!=0)
	{
		pcw16_refresh_ints();
	}
}

static ADDRESS_MAP_START(pcw16_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_READWRITE(MRA8_BANK1, MWA8_BANK5)
	AM_RANGE(0x4000, 0x7fff) AM_READWRITE(MRA8_BANK2, MWA8_BANK6)
	AM_RANGE(0x8000, 0xbfff) AM_READWRITE(MRA8_BANK3, MWA8_BANK7)
	AM_RANGE(0xc000, 0xffff) AM_READWRITE(MRA8_BANK4, MWA8_BANK8)
ADDRESS_MAP_END


extern int pcw16_colour_palette[16];

static WRITE8_HANDLER(pcw16_palette_w)
{
	pcw16_colour_palette[offset & 0x0f] = data & 31;
}

static char *pcw16_mem_ptr[4];

static const write8_handler pcw16_write_handler_dram[4] =
{
	MWA8_BANK5,
	MWA8_BANK6,
	MWA8_BANK7,
	MWA8_BANK8
};

static const read8_handler pcw16_read_handler_dram[4] =
{
	MRA8_BANK1,
	MRA8_BANK2,
	MRA8_BANK3,
	MRA8_BANK4
};
/*******************************************/


/* PCW16 Flash interface */
/* PCW16 can have two 1mb flash chips */

/* read flash0 */
static int pcw16_flash0_bank_handler_r(int bank, int offset)
{
	int flash_offset = (pcw16_banks[bank]<<14) | offset;

	return flash_bank_handler_r(0, flash_offset);
}

/* read flash1 */
static int pcw16_flash1_bank_handler_r(int bank, int offset)
{
	int flash_offset = ((pcw16_banks[bank]&0x03f)<<14) | offset;

	return flash_bank_handler_r(1, flash_offset);
}

/* flash 0 */
static  READ8_HANDLER(pcw16_flash0_bank_handler0_r)
{
	return pcw16_flash0_bank_handler_r(0, offset);
}

static  READ8_HANDLER(pcw16_flash0_bank_handler1_r)
{
	return pcw16_flash0_bank_handler_r(1, offset);
}

static  READ8_HANDLER(pcw16_flash0_bank_handler2_r)
{
	return pcw16_flash0_bank_handler_r(2, offset);
}

static  READ8_HANDLER(pcw16_flash0_bank_handler3_r)
{
	return pcw16_flash0_bank_handler_r(3, offset);
}

/* flash 1 */
static  READ8_HANDLER(pcw16_flash1_bank_handler0_r)
{
	return pcw16_flash1_bank_handler_r(0, offset);
}

static  READ8_HANDLER(pcw16_flash1_bank_handler1_r)
{
	return pcw16_flash1_bank_handler_r(1, offset);
}

static  READ8_HANDLER(pcw16_flash1_bank_handler2_r)
{
	return pcw16_flash1_bank_handler_r(2, offset);
}

static  READ8_HANDLER(pcw16_flash1_bank_handler3_r)
{
	return pcw16_flash1_bank_handler_r(3, offset);
}

static read8_handler pcw16_flash0_bank_handlers_r[4] =
{
	pcw16_flash0_bank_handler0_r,
	pcw16_flash0_bank_handler1_r,
	pcw16_flash0_bank_handler2_r,
	pcw16_flash0_bank_handler3_r,
};

static read8_handler pcw16_flash1_bank_handlers_r[4] =
{
	pcw16_flash1_bank_handler0_r,
	pcw16_flash1_bank_handler1_r,
	pcw16_flash1_bank_handler2_r,
	pcw16_flash1_bank_handler3_r,
};

/* write flash0 */
static void pcw16_flash0_bank_handler_w(int bank, int offset, int data)
{
	int flash_offset = (pcw16_banks[bank]<<14) | offset;

	flash_bank_handler_w(0, flash_offset, data);
}

/* read flash1 */
static void pcw16_flash1_bank_handler_w(int bank, int offset, int data)
{
	int flash_offset = ((pcw16_banks[bank]&0x03f)<<14) | offset;

	flash_bank_handler_w(1, flash_offset,data);
}

/* flash 0 */
static WRITE8_HANDLER(pcw16_flash0_bank_handler0_w)
{
	pcw16_flash0_bank_handler_w(0, offset, data);
}


static WRITE8_HANDLER(pcw16_flash0_bank_handler1_w)
{
	pcw16_flash0_bank_handler_w(1, offset, data);
}

static WRITE8_HANDLER(pcw16_flash0_bank_handler2_w)
{
	pcw16_flash0_bank_handler_w(2, offset, data);
}

static WRITE8_HANDLER(pcw16_flash0_bank_handler3_w)
{
	pcw16_flash0_bank_handler_w(3, offset, data);
}


/* flash 1 */
static WRITE8_HANDLER(pcw16_flash1_bank_handler0_w)
{
	pcw16_flash1_bank_handler_w(0, offset, data);
}


static WRITE8_HANDLER(pcw16_flash1_bank_handler1_w)
{
	pcw16_flash1_bank_handler_w(1, offset, data);
}

static WRITE8_HANDLER(pcw16_flash1_bank_handler2_w)
{
	pcw16_flash1_bank_handler_w(2, offset, data);
}

static WRITE8_HANDLER(pcw16_flash1_bank_handler3_w)
{
	pcw16_flash1_bank_handler_w(3, offset, data);
}

static write8_handler pcw16_flash0_bank_handlers_w[4] =
{
	pcw16_flash0_bank_handler0_w,
	pcw16_flash0_bank_handler1_w,
	pcw16_flash0_bank_handler2_w,
	pcw16_flash0_bank_handler3_w,
};

static write8_handler pcw16_flash1_bank_handlers_w[4] =
{
	pcw16_flash1_bank_handler0_w,
	pcw16_flash1_bank_handler1_w,
	pcw16_flash1_bank_handler2_w,
	pcw16_flash1_bank_handler3_w,
};

typedef enum
{
	/* rom which is really first block of flash0 */
	PCW16_MEM_ROM,
	/* flash 0 */
	PCW16_MEM_FLASH_1,
	/* flash 1 i.e. unexpanded pcw16 */
	PCW16_MEM_FLASH_2,
	/* dram */
	PCW16_MEM_DRAM,
	/* no mem. i.e. unexpanded pcw16 */
	PCW16_MEM_NONE
} PCW16_RAM_TYPE;

static  READ8_HANDLER(pcw16_no_mem_r)
{
	return 0x0ff;
}

static void pcw16_set_bank_handlers(int bank, PCW16_RAM_TYPE type)
{
	read8_handler read_handler;
	write8_handler write_handler;

	switch (type) {
	case PCW16_MEM_ROM:
		/* rom */
		read_handler = pcw16_read_handler_dram[bank];
		write_handler = MWA8_NOP;
		break;

	case PCW16_MEM_FLASH_1:
		/* sram */
		read_handler = pcw16_flash0_bank_handlers_r[bank];
		write_handler = pcw16_flash0_bank_handlers_w[bank];
		break;

	case PCW16_MEM_FLASH_2:
		read_handler = pcw16_flash1_bank_handlers_r[bank];
		write_handler = pcw16_flash1_bank_handlers_w[bank];
		break;

	case PCW16_MEM_NONE:
		read_handler = pcw16_no_mem_r;
		write_handler = MWA8_NOP;
		break;

	default:
	case PCW16_MEM_DRAM:
		/* dram */
		read_handler = pcw16_read_handler_dram[bank];
		write_handler = pcw16_write_handler_dram[bank];
		break;
	}

	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM,
		(bank * 0x4000), (bank * 0x4000) + 0x3fff, 0, 0, read_handler);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM,
		(bank * 0x4000), (bank * 0x4000) + 0x3fff, 0, 0, write_handler);
}

static void pcw16_update_bank(int bank)
{
	unsigned char *mem_ptr = mess_ram;
	int bank_id = 0;
	int bank_offs = 0;


	/* get memory bank */
	bank_id = pcw16_banks[bank];


	if ((bank_id & 0x080)==0)
	{
		bank_offs = 0;

		if (bank_id<4)
		{
			/* lower 4 banks are write protected. Use the rom
			loaded */
			mem_ptr = &memory_region(REGION_CPU1)[0x010000];
		}
		else
		{
			/* nvram */
			if ((bank_id & 0x040)==0)
			{
				mem_ptr = (unsigned char *)flash_get_base(0);
			}
			else
			{
				mem_ptr = (unsigned char *)flash_get_base(1);
			}
		}

	}
	else
	{
		bank_offs = 128;
		/* dram */
		mem_ptr = mess_ram;
	}

	mem_ptr = mem_ptr + ((bank_id - bank_offs)<<14);
	pcw16_mem_ptr[bank] = (char*)mem_ptr;
	memory_set_bankptr((bank+1), mem_ptr);
	memory_set_bankptr((bank+5), mem_ptr);

	if ((bank_id & 0x080)==0)
	{
		/* selections 0-3 within the first 64k are write protected */
		if (bank_id<4)
		{
			/* rom */
			pcw16_set_bank_handlers(bank, PCW16_MEM_ROM);
		}
		else
		{
			/* selections 0-63 are for flash-rom 0, selections
			64-128 are for flash-rom 1 */
			if ((bank_id & 0x040)==0)
			{
				pcw16_set_bank_handlers(bank, PCW16_MEM_FLASH_1);
			}
			else
			{
				pcw16_set_bank_handlers(bank, PCW16_MEM_FLASH_2);
			}
		}
	}
	else
	{
		pcw16_set_bank_handlers(bank, PCW16_MEM_DRAM);
	}
}


/* update memory h/w */
static void pcw16_update_memory(void)
{
	pcw16_update_bank(0);
	pcw16_update_bank(1);
	pcw16_update_bank(2);
	pcw16_update_bank(3);

}

static  READ8_HANDLER(pcw16_bankhw_r)
{
//	logerror("bank r: %d \n", offset);

	return pcw16_banks[offset];
}

static WRITE8_HANDLER(pcw16_bankhw_w)
{
	//logerror("bank w: %d block: %02x\n", offset, data);

	pcw16_banks[offset] = data;

	pcw16_update_memory();
}

static WRITE8_HANDLER(pcw16_video_control_w)
{
	//logerror("video control w: %02x\n", data);

	pcw16_video_control = data;
}

/* PCW16 KEYBOARD */

unsigned char pcw16_keyboard_status;
unsigned char pcw16_keyboard_data_shift;



#define PCW16_KEYBOARD_PARITY_MASK	(1<<7)
#define PCW16_KEYBOARD_STOP_BIT_MASK (1<<6)
#define PCW16_KEYBOARD_START_BIT_MASK (1<<5)
#define PCW16_KEYBOARD_BUSY_STATUS	(1<<4)
#define PCW16_KEYBOARD_FORCE_KEYBOARD_CLOCK (1<<1)
#define PCW16_KEYBOARD_TRANSMIT_MODE (1<<0)

#define PCW16_KEYBOARD_RESET_INTERFACE (1<<2)

#define PCW16_KEYBOARD_DATA	(1<<1)
#define PCW16_KEYBOARD_CLOCK (1<<0)

/* parity table. Used to set parity bit in keyboard status register */
static int pcw16_keyboard_parity_table[256];

static int pcw16_keyboard_bits = 0;
static int pcw16_keyboard_bits_output = 0;

static int pcw16_keyboard_state = 0;
static int pcw16_keyboard_previous_state=0;
static void pcw16_keyboard_reset(void);
static void pcw16_keyboard_int(int);

static void pcw16_keyboard_init(void)
{
	int i;
	int b;

	/* if sum of all bits in the byte is even, then the data
	has even parity, otherwise it has odd parity */
	for (i=0; i<256; i++)
	{
		int data;
		int sum;

		sum = 0;
		data = i;

		for (b=0; b<8; b++)
		{
			sum+=data & 0x01;

			data = data>>1;
		}

		pcw16_keyboard_parity_table[i] = sum & 0x01;
	}


	/* clear int */
	pcw16_keyboard_int(0);
	/* reset state */
	pcw16_keyboard_state = 0;
	/* reset ready for transmit */
	pcw16_keyboard_reset();
}

static void pcw16_keyboard_refresh_outputs(void)
{
	/* generate output bits */
	pcw16_keyboard_bits_output = pcw16_keyboard_bits;

	/* force clock low? */
	if (pcw16_keyboard_state & PCW16_KEYBOARD_FORCE_KEYBOARD_CLOCK)
	{
		pcw16_keyboard_bits_output &= ~PCW16_KEYBOARD_CLOCK;
	}
}

static void pcw16_keyboard_set_clock_state(int state)
{
	pcw16_keyboard_bits &= ~PCW16_KEYBOARD_CLOCK;

	if (state)
	{
		pcw16_keyboard_bits |= PCW16_KEYBOARD_CLOCK;
	}

	pcw16_keyboard_refresh_outputs();
}

static void pcw16_keyboard_int(int state)
{
	pcw16_system_status &= ~(1<<1);

	if (state)
	{
		pcw16_system_status |= (1<<1);
	}

	pcw16_refresh_ints();
}

static void pcw16_keyboard_reset(void)
{
	/* clock set to high */
	pcw16_keyboard_set_clock_state(1);
}

/* interfaces to a pc-at keyboard */
static  READ8_HANDLER(pcw16_keyboard_data_shift_r)
{
	//logerror("keyboard data shift r: %02x\n", pcw16_keyboard_data_shift);
	pcw16_keyboard_state &= ~(PCW16_KEYBOARD_BUSY_STATUS);

	pcw16_keyboard_int(0);
	/* reset for reception */
	pcw16_keyboard_reset();

	/* read byte */
	return pcw16_keyboard_data_shift;
}

/* if force keyboard clock is low it is safe to send */
static int		pcw16_keyboard_can_transmit(void)
{
	/* clock is not forced low */
	/* and not busy - i.e. not already sent a char */
	return ((pcw16_keyboard_bits_output & PCW16_KEYBOARD_CLOCK)!=0);
}

/* issue a begin byte transfer */
static void	pcw16_begin_byte_transfer(void)
{
}


/* signal a code has been received */
static void	pcw16_keyboard_signal_byte_received(int data)
{

	/* clear clock */
	pcw16_keyboard_set_clock_state(0);

	/* set code in shift register */
	pcw16_keyboard_data_shift = data;
	/* busy */
	pcw16_keyboard_state |= PCW16_KEYBOARD_BUSY_STATUS;

	/* initialise start, stop and parity bits */
	pcw16_keyboard_state &= ~PCW16_KEYBOARD_START_BIT_MASK;
	pcw16_keyboard_state |=PCW16_KEYBOARD_STOP_BIT_MASK;

	/* "Keyboard data has odd parity, so the parity bit in the
	status register should only be set when the shift register
	data itself has even parity. */

	pcw16_keyboard_state &= ~PCW16_KEYBOARD_PARITY_MASK;

	/* if data has even parity, set parity bit */
	if ((pcw16_keyboard_parity_table[data])==0)
		pcw16_keyboard_state |= PCW16_KEYBOARD_PARITY_MASK;

	pcw16_keyboard_int(1);
}


static WRITE8_HANDLER(pcw16_keyboard_data_shift_w)
{
	//logerror("Keyboard Data Shift: %02x\n", data);
	/* writing to shift register clears parity */
	/* writing to shift register clears start bit */
	pcw16_keyboard_state &= ~(
		PCW16_KEYBOARD_PARITY_MASK |
		PCW16_KEYBOARD_START_BIT_MASK);

	/* writing to shift register sets stop bit */
	pcw16_keyboard_state |= PCW16_KEYBOARD_STOP_BIT_MASK;

	pcw16_keyboard_data_shift = data;

}

static  READ8_HANDLER(pcw16_keyboard_status_r)
{
	/* bit 2,3 are bits 8 and 9 of vdu pointer */
	return (pcw16_keyboard_state &
		(PCW16_KEYBOARD_PARITY_MASK |
		 PCW16_KEYBOARD_STOP_BIT_MASK |
		 PCW16_KEYBOARD_START_BIT_MASK |
		 PCW16_KEYBOARD_BUSY_STATUS |
		 PCW16_KEYBOARD_FORCE_KEYBOARD_CLOCK |
		 PCW16_KEYBOARD_TRANSMIT_MODE));
}

static WRITE8_HANDLER(pcw16_keyboard_control_w)
{
	//logerror("Keyboard control w: %02x\n",data);

	pcw16_keyboard_previous_state = pcw16_keyboard_state;

	/* if set, set parity */
	if (data & 0x080)
	{
		pcw16_keyboard_state |= PCW16_KEYBOARD_PARITY_MASK;
	}

	/* clear read/write bits */
	pcw16_keyboard_state &=
		~(PCW16_KEYBOARD_FORCE_KEYBOARD_CLOCK |
			PCW16_KEYBOARD_TRANSMIT_MODE);
	/* set read/write bits from data */
	pcw16_keyboard_state |= (data & 0x03);

	if (data & PCW16_KEYBOARD_RESET_INTERFACE)
	{
		pcw16_keyboard_reset();
	}

	if (data & PCW16_KEYBOARD_TRANSMIT_MODE)
	{
		/* force clock changed */
		if (((pcw16_keyboard_state^pcw16_keyboard_previous_state) & PCW16_KEYBOARD_FORCE_KEYBOARD_CLOCK)!=0)
		{
			/* just cleared? */
			if ((pcw16_keyboard_state & PCW16_KEYBOARD_FORCE_KEYBOARD_CLOCK)==0)
			{

				/* write */
				/* busy */
				pcw16_keyboard_state |= PCW16_KEYBOARD_BUSY_STATUS;
				/* keyboard takes data */
				at_keyboard_write(pcw16_keyboard_data_shift);
				/* set clock low - no furthur transmissions */
				pcw16_keyboard_set_clock_state(0);
				/* set int */
				pcw16_keyboard_int(1);
			}
		}


	}

	if (((pcw16_keyboard_state^pcw16_keyboard_previous_state) & PCW16_KEYBOARD_TRANSMIT_MODE)!=0)
	{
		if ((pcw16_keyboard_state & PCW16_KEYBOARD_TRANSMIT_MODE)==0)
		{
			if ((pcw16_system_status & (1<<1))!=0)
			{
				pcw16_keyboard_int(0);
			}
		}
	}

	pcw16_keyboard_refresh_outputs();
}


static void pcw16_keyboard_timer_callback(int dummy)
{
	at_keyboard_polling();
	if (pcw16_keyboard_can_transmit())
	{
		int data;

		data = at_keyboard_read();

		if (data!=-1)
		{
//			if (data==4)
//			{
//				pcw16_dump_cpu_ram();
//			}

			pcw16_keyboard_signal_byte_received(data);
		}
	}
}

static unsigned char rtc_seconds;
static unsigned char rtc_minutes;
static unsigned char rtc_hours;
static unsigned char rtc_days_max;
static unsigned char rtc_days;
static unsigned char rtc_months;
static unsigned char rtc_years;
static unsigned char rtc_control;
static unsigned char rtc_256ths_seconds;

static int rtc_days_in_each_month[]=
{
	31,/* jan */
	28, /* feb */
	31, /* march */
	30, /* april */
	31, /* may */
	30, /* june */
	31, /* july */
	31, /* august */
	30, /* september */
	31, /* october */
	30, /* november */
	31	/* december */
};

static int rtc_days_in_february[] =
{
	29, 28, 28, 28
};

static void rtc_setup_max_days(void)
{
	/* february? */
	if (rtc_months == 2)
	{
		/* low two bits of year select number of days in february */
		rtc_days_max = rtc_days_in_february[rtc_years & 0x03];
	}
	else
	{
		rtc_days_max = (unsigned char)rtc_days_in_each_month[rtc_months];
	}
}

static void rtc_timer_callback(int dummy)
{
	int fraction_of_second;

	/* halt counter? */
	if ((rtc_control & 0x01)!=0)
	{
		/* no */

		/* increment 256th's of a second register */
		fraction_of_second = rtc_256ths_seconds+1;
		/* add bit 8 = overflow */
		rtc_seconds+=(fraction_of_second>>8);
		/* ensure counter is in range 0-255 */
		rtc_256ths_seconds = fraction_of_second & 0x0ff;
	}

	if (rtc_seconds>59)
	{
		rtc_seconds = 0;

		rtc_minutes++;

		if (rtc_minutes>59)
		{
			rtc_minutes = 0;

			rtc_hours++;

			if (rtc_hours>23)
			{
				rtc_hours = 0;

				rtc_days++;

				if (rtc_days>rtc_days_max)
				{
					rtc_days = 1;

					rtc_months++;

					if (rtc_months>12)
					{
						rtc_months = 1;

						/* 7 bit year counter */
						rtc_years = (rtc_years + 1) & 0x07f;

					}

					rtc_setup_max_days();
				}

			}


		}
	}
}

static  READ8_HANDLER(rtc_year_invalid_r)
{
	/* year in lower 7 bits. RTC Invalid status is rtc_control bit 0
	inverted */
	return (rtc_years & 0x07f) | (((rtc_control & 0x01)<<7)^0x080);
}

static  READ8_HANDLER(rtc_month_r)
{
	return rtc_months;
}

static  READ8_HANDLER(rtc_days_r)
{
	return rtc_days;
}

static  READ8_HANDLER(rtc_hours_r)
{
	return rtc_hours;
}

static  READ8_HANDLER(rtc_minutes_r)
{
	return rtc_minutes;
}

static  READ8_HANDLER(rtc_seconds_r)
{
	return rtc_seconds;
}

static  READ8_HANDLER(rtc_256ths_seconds_r)
{
	return rtc_256ths_seconds;
}

static WRITE8_HANDLER(rtc_control_w)
{
	/* write control */
	rtc_control = data;
}

static WRITE8_HANDLER(rtc_seconds_w)
{
	/* TODO: Writing register could cause next to increment! */
	rtc_seconds = data;
}

static WRITE8_HANDLER(rtc_minutes_w)
{
	/* TODO: Writing register could cause next to increment! */
	rtc_minutes = data;
}

static WRITE8_HANDLER(rtc_hours_w)
{
	/* TODO: Writing register could cause next to increment! */
	rtc_hours = data;
}

static WRITE8_HANDLER(rtc_days_w)
{
	/* TODO: Writing register could cause next to increment! */
	rtc_days = data;
}

static WRITE8_HANDLER(rtc_month_w)
{
	/* TODO: Writing register could cause next to increment! */
	rtc_months = data;

	rtc_setup_max_days();
}


static WRITE8_HANDLER(rtc_year_w)
{
	/* TODO: Writing register could cause next to increment! */
	rtc_hours = data;

	rtc_setup_max_days();
}

static int previous_fdc_int_state;

static void pcw16_trigger_fdc_int(void)
{
	int state;

	state = pcw16_system_status & (1<<6);

	switch (pcw16_fdc_int_code)
	{
		/* nmi */
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
			pcw16_refresh_ints();
		}
		break;

		/* do not interrupt */
		default:
			break;
	}

	previous_fdc_int_state = state;
}

static  READ8_HANDLER(pcw16_system_status_r)
{
//	logerror("system status r: \n");

	return pcw16_system_status | (readinputport(0) & 0x04);
}

static  READ8_HANDLER(pcw16_timer_interrupt_counter_r)
{
	int data;

	data = pcw16_interrupt_counter;

	pcw16_interrupt_counter = 0;
	/* clear display int */
	pcw16_system_status &= ~(1<<0);

	pcw16_refresh_ints();

	return data;
}


static WRITE8_HANDLER(pcw16_system_control_w)
{
	//logerror("0x0f8: function: %d\n",data);

	/* lower 4 bits define function code */
	switch (data & 0x0f)
	{
		/* no effect */
		case 0x00:
		case 0x09:
		case 0x0a:
		case 0x0d:
		case 0x0e:
			break;

		/* system reset */
		case 0x01:
			break;

		/* connect IRQ6 input to /NMI */
		case 0x02:
		{
			pcw16_fdc_int_code = 0;
		}
		break;

		/* connect IRQ6 input to /INT */
		case 0x03:
		{
			pcw16_fdc_int_code = 1;
		}
		break;

		/* dis-connect IRQ6 input from /NMI and /INT */
		case 0x04:
		{
			pcw16_fdc_int_code = 2;
		}
		break;

		/* set terminal count */
		case 0x05:
		{
			pc_fdc_set_tc_state(1);
		}
		break;

		/* clear terminal count */
		case 0x06:
		{
			pc_fdc_set_tc_state(0);
		}
		break;

		/* bleeper on */
		case 0x0b:
		{
                        beep_set_state(0,1);
		}
		break;

		/* bleeper off */
		case 0x0c:
		{
                        beep_set_state(0,0);
		}
		break;

		/* drive video outputs */
		case 0x07:
		{
		}
		break;

		/* float video outputs */
		case 0x08:
		{
		}
		break;

		/* set 4-bit output port to value X */
		case 0x0f:
		{
			/* bit 7 - ?? */
			/* bit 6 - ?? */
			/* bit 5 - green/red led (1==green)*/
			/* bit 4 - monitor on/off (1==on) */

			pcw16_4_bit_port = data>>4;


		}
		break;
	}
}

/**** SUPER I/O connections */

/* write to Super I/O chip. FDC Data Rate. */
static WRITE8_HANDLER(pcw16_superio_fdc_datarate_w)
{
	pc_fdc_w(PC_FDC_DATA_RATE_REGISTER,data);
}

/* write to Super I/O chip. FDC Digital output register */
static WRITE8_HANDLER(pcw16_superio_fdc_digital_output_register_w)
{
	pc_fdc_w(PC_FDC_DIGITAL_OUTPUT_REGISTER, data);
}

/* write to Super I/O chip. FDC Data Register */
static WRITE8_HANDLER(pcw16_superio_fdc_data_w)
{
	pc_fdc_w(PC_FDC_DATA_REGISTER, data);
}

/* write to Super I/O chip. FDC Data Register */
static  READ8_HANDLER(pcw16_superio_fdc_data_r)
{
	return pc_fdc_r(PC_FDC_DATA_REGISTER);
}

/* write to Super I/O chip. FDC Main Status Register */
static  READ8_HANDLER(pcw16_superio_fdc_main_status_register_r)
{
	return pc_fdc_r(PC_FDC_MAIN_STATUS_REGISTER);
}

static  READ8_HANDLER(pcw16_superio_fdc_digital_input_register_r)
{
	return pc_fdc_r(PC_FDC_DIGITIAL_INPUT_REGISTER);
}

static void	pcw16_fdc_interrupt(int state)
{
	/* IRQ6 */
	/* bit 6 of PCW16 system status indicates floppy ints */
	pcw16_system_status &= ~(1<<6);

	if (state)
	{
		pcw16_system_status |= (1<<6);
	}

	pcw16_trigger_fdc_int();
}

static const struct pc_fdc_interface pcw16_fdc_interface=
{
	NEC765A,
	pcw16_fdc_interrupt,
	NULL
};

static void pcw16_com_interrupt(int nr, int state)
{
	static const int irq[2]={4,3};
	pcw16_system_status &= ~(1<<irq[nr]);

	if (state)
	{
		pcw16_system_status |= (1<<irq[nr]);
	}

	pcw16_refresh_ints();
}

static void pcw16_com_refresh_connected(int serial_port_id)
{
	switch (serial_port_id)
	{
#if 0
		case 0:
		{
			/* PC mouse on this port */
			pc_mouse_poll(0);
		}
		break;
#endif
		case 1:
		{
			int new_inputs;

			new_inputs = 0;

			/* Power switch is connected to Ring indicator */
			if (readinputport(0) & 0x040)
			{
				new_inputs = UART8250_INPUTS_RING_INDICATOR;
			}

			uart8250_handshake_in(1, new_inputs);
		}
		break;
	}
}

static uart8250_interface pcw16_com_interface[2]=
{
	{
		TYPE16550,
		1843200,
		pcw16_com_interrupt,
		NULL,
		pc_mouse_handshake_in
//		pcw16_com_refresh_connected
	},
	{
		TYPE16550,
		1843200,
		pcw16_com_interrupt,
		NULL,
		NULL,
		pcw16_com_refresh_connected
	}
};



static ADDRESS_MAP_START(pcw16_io, ADDRESS_SPACE_IO, 8)
	/* super i/o chip */
	AM_RANGE(0x01a, 0x01a) AM_WRITE(pcw16_superio_fdc_digital_output_register_w)
    AM_RANGE(0x01c, 0x01c) AM_READ(pcw16_superio_fdc_main_status_register_r)
	AM_RANGE(0x01d, 0x01d) AM_READWRITE(pcw16_superio_fdc_data_r, pcw16_superio_fdc_data_w)
	AM_RANGE(0x01f, 0x01f) AM_READWRITE(pcw16_superio_fdc_digital_input_register_r, pcw16_superio_fdc_datarate_w)
	AM_RANGE(0x020, 0x027) AM_READWRITE(uart8250_0_r, uart8250_0_w)
	AM_RANGE(0x028, 0x02f) AM_READWRITE(uart8250_1_r, uart8250_1_w)
	AM_RANGE(0x038, 0x03a) AM_READWRITE(pc_parallelport0_r, pc_parallelport0_w)
	/* anne asic */
	AM_RANGE(0x0e0, 0x0ef) AM_WRITE(pcw16_palette_w)
	AM_RANGE(0x0f0, 0x0f3) AM_READWRITE(pcw16_bankhw_r, pcw16_bankhw_w)
	AM_RANGE(0x0f4, 0x0f4) AM_READWRITE(pcw16_keyboard_data_shift_r, pcw16_keyboard_data_shift_w)
	AM_RANGE(0x0f5, 0x0f5) AM_READWRITE(pcw16_keyboard_status_r, pcw16_keyboard_control_w)
	AM_RANGE(0x0f7, 0x0f7) AM_READWRITE(pcw16_timer_interrupt_counter_r, pcw16_video_control_w)
	AM_RANGE(0x0f8, 0x0f8) AM_READWRITE(pcw16_system_status_r, pcw16_system_control_w)
	AM_RANGE(0x0f9, 0x0f9) AM_READWRITE(rtc_256ths_seconds_r, rtc_control_w)
	AM_RANGE(0x0fa, 0x0fa) AM_READWRITE(rtc_seconds_r, rtc_seconds_w)
	AM_RANGE(0x0fb, 0x0fb) AM_READWRITE(rtc_minutes_r, rtc_minutes_w)
	AM_RANGE(0x0fc, 0x0fc) AM_READWRITE(rtc_hours_r, rtc_hours_w)
	AM_RANGE(0x0fd, 0x0fd) AM_READWRITE(rtc_days_r, rtc_days_w)
	AM_RANGE(0x0fe, 0x0fe) AM_READWRITE(rtc_month_r, rtc_month_w)
	AM_RANGE(0x0ff, 0x0ff) AM_READWRITE(rtc_year_invalid_r, rtc_year_w)
ADDRESS_MAP_END


static void pcw16_reset(void)
{
	/* initialise defaults */
	pcw16_fdc_int_code = 2;
	/* clear terminal count */
	pc_fdc_set_tc_state(0);
	/* select first rom page */
	pcw16_banks[0] = 0;
	pcw16_update_memory();

	/* temp rtc setup */
	rtc_seconds = 0;
	rtc_minutes = 0;
	rtc_hours = 0;
	rtc_days_max = 0;
	rtc_days = 1;
	rtc_months = 1;
	rtc_years = 0;
	rtc_control = 1;
	rtc_256ths_seconds = 0;

	pcw16_keyboard_init();
	uart8250_reset(0);
	uart8250_reset(1);
	flash_reset(0);
	flash_reset(1);
}


static PC_LPT_CONFIG lpt_config = 
{
	1,
	LPT_UNIDIRECTIONAL, // more one of these epp/ecp aware ports
	NULL
};

static CENTRONICS_CONFIG cent_config =
{
	PRINTER_CENTRONICS,
	pc_lpt_handshake_in
};

static MACHINE_RESET( pcw16 )
{
	/* flash 0 */
	flash_init(0);
	flash_restore(0, "pcw16f1.nv");

	/* flash 1 */
	flash_init(1);
	flash_restore(1, "pcw16f2.nv");

	pcw16_system_status = 0;
	pcw16_interrupt_counter = 0;

	/* video ints */
	timer_pulse(TIME_IN_MSEC(5.83), 0,pcw16_timer_callback);
	/* rtc timer */
	timer_pulse(TIME_IN_SEC(1.0f/256.0f), 0, rtc_timer_callback);
	/* keyboard timer */
	timer_pulse(TIME_IN_HZ(50), 0, pcw16_keyboard_timer_callback);


	pc_fdc_init(&pcw16_fdc_interface);
	uart8250_init(0, pcw16_com_interface);
	uart8250_init(1, pcw16_com_interface+1);

	pc_lpt_config(0, &lpt_config);
	centronics_config(0, &cent_config);
	pc_lpt_set_device(0, &CENTRONICS_PRINTER_DEVICE);

	/* initialise mouse */
	pc_mouse_set_serial_port(0);
	pc_mouse_initialise();

	/* initialise keyboard */
	at_keyboard_init(AT_KEYBOARD_TYPE_AT);
	at_keyboard_set_scan_code_set(3);

	pcw16_reset();

	beep_set_state(0,0);
	beep_set_frequency(0,3750);

	add_exit_callback(machine, pcw16_machine_stop);
}

static void pcw16_machine_stop(running_machine *machine)
{
	/* flash 0 */
	flash_store(0,"pcw16f1.nv");
	flash_finish(0);

	flash_store(1,"pcw16f2.nv");
	flash_finish(1);
}

INPUT_PORTS_START(pcw16)
	PORT_START
	/* vblank */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_VBLANK)
	/* power switch - default is on */
	PORT_DIPNAME(0x40, 0x40, "Power Switch/Suspend")
	PORT_DIPSETTING(0x0, DEF_STR( Off) )
	PORT_DIPSETTING(0x40, DEF_STR( On) )

	PORT_INCLUDE( pc_mouse_mousesystems )	/* IN12 - IN14 */

	PORT_INCLUDE( at_keyboard )		/* IN4 - IN11 */
INPUT_PORTS_END



static MACHINE_DRIVER_START( pcw16 )
	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 16000000)
	MDRV_CPU_PROGRAM_MAP(pcw16_map, 0)
	MDRV_CPU_IO_MAP(pcw16_io, 0)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_RESET( pcw16 )

    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(PCW16_SCREEN_WIDTH, PCW16_SCREEN_HEIGHT)
	MDRV_SCREEN_VISIBLE_AREA(0, PCW16_SCREEN_WIDTH-1, 0, PCW16_SCREEN_HEIGHT-1)
	MDRV_PALETTE_LENGTH(PCW16_NUM_COLOURS)
	MDRV_COLORTABLE_LENGTH(PCW16_NUM_COLOURS)
	MDRV_PALETTE_INIT( pcw16 )

	MDRV_VIDEO_START( pcw16 )
	MDRV_VIDEO_UPDATE( pcw16 )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(BEEP, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
MACHINE_DRIVER_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

/* the lower 64k of the flash-file memory is write protected. This contains the boot
	rom. The boot rom is also on the OS rescue disc. Handy! */
ROM_START(pcw16)
	ROM_REGION((0x010000+524288), REGION_CPU1,0)
	ROM_LOAD("pcw045.sys",0x10000, 524288, CRC(c642f498))
ROM_END

static void pcw16_printer_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* printer */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 1; break;

		default:										printer_device_getinfo(devclass, state, info); break;
	}
}

static void pcw16_floppy_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* floppy */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 2; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_FLOPPY_OPTIONS:				info->p = (void *) floppyoptions_pc; break;

		default:										floppy_device_getinfo(devclass, state, info); break;
	}
}

SYSTEM_CONFIG_START(pcw16)
	CONFIG_RAM_DEFAULT(2048 * 1024)
	/*CONFIG_DEVICE(pcw16_printer_getinfo)*/
	CONFIG_DEVICE(pcw16_floppy_getinfo)
SYSTEM_CONFIG_END

/*     YEAR  NAME     PARENT	COMPAT	MACHINE    INPUT     INIT   CONFIG,  COMPANY          FULLNAME */
COMP( 1995, pcw16,	  0,		0,		pcw16,	   pcw16,    0,	    pcw16,   "Amstrad plc",   "PCW16", 0)

/****************************************************************************
	Mac hardware
	
	The hardware for Mac 128k, 512k, 512ke, Plus (SCSI, SCC, etc).

	Nate Woods
	Ernesto Corvi
	Raphael Nabet

	Mac Model Feature Summary:
		
						CPU		FDC		Keyb	PRAM	ROMMir
		 - Mac 128k		68k		IWM		orig	orig	???
		 - Mac 512k		68k		IWM		orig	orig	???
		 - Mac 512ke	68k		IWM		orig	orig	???
		 - Mac Plus		68k		IWM		orig	ext		no
		 - Mac SE		68k		IWM		ADB		ext		???
		 - Mac Classic	68k		IWM		ADB		ext		???

	Notes:
		- The Mac Plus boot code seems to check to see the extent of ROM
		  mirroring to determine if SCSI is available.  If the ROM is mirrored,
		  then SCSI is not available.  Thanks to R. Belmont for making this
		  discovery.

	TODO:
		- Call the RTC timer

		- Support Mac 128k, 512k (easy, we just need the ROM image)
		- Support Mac SE, Classic (we need to find the ROMs and implement ADB;
		  SE FDHD and Classic require SIWM support, too)
		- Check that 0x600000-0x6fffff still address RAM when overlay bit is off
		  (IM-III seems to say it does not on Mac 128k, 512k, and 512ke).
		- What on earth are 0x700000-0x7fffff mapped to ?

****************************************************************************/

#include <time.h>

#include "driver.h"
#include "state.h"
#include "machine/6522via.h"
#include "machine/8530scc.h"
#include "video/generic.h"
#include "cpu/m68000/m68000.h"
#include "machine/applefdc.h"
#include "devices/sonydriv.h"
#include "machine/ncr5380.h"
#include "includes/mac.h"

#ifdef MAME_DEBUG
#define LOG_VIA			0
#define LOG_RTC			0
#define LOG_MAC_IWM		0
#define LOG_GENERAL		0
#define LOG_KEYBOARD	0
#define LOG_MEMORY		0
#else
#define LOG_VIA			0
#define LOG_RTC			0
#define LOG_MAC_IWM		0
#define LOG_GENERAL		0
#define LOG_KEYBOARD	0
#define LOG_MEMORY		0
#endif

static int scan_keyboard(void);
static void inquiry_timeout_func(int unused);
static void keyboard_receive(int val);
static READ8_HANDLER(mac_via_in_a);
static READ8_HANDLER(mac_via_in_b);
static WRITE8_HANDLER(mac_via_out_a);
static WRITE8_HANDLER(mac_via_out_b);
static WRITE8_HANDLER(mac_via_out_cb2);
static void mac_via_irq(int state);
static offs_t mac_dasm_override(char *buffer, offs_t pc, const UINT8 *oprom, const UINT8 *opram);

static struct via6522_interface mac_via6522_intf =
{
	mac_via_in_a, mac_via_in_b,
	NULL, NULL,
	NULL, NULL,
	mac_via_out_a, mac_via_out_b,
	NULL, NULL,
	NULL, mac_via_out_cb2,
	mac_via_irq
};

/* tells which model is being emulated (set by macxxx_init) */
typedef enum
{
	MODEL_MAC_128K512K,
	MODEL_MAC_512KE,
	MODEL_MAC_PLUS,
	MODEL_MAC_SE
} mac_model_t;

static UINT32 mac_overlay = 0;
static mac_model_t mac_model;



static void mac_install_memory(offs_t memory_begin, offs_t memory_end,
	offs_t memory_size, void *memory_data, int is_rom, int bank)
{
	offs_t memory_mask;
	read16_handler rh;
	write16_handler wh;

	memory_size = MIN(memory_size, (memory_end + 1 - memory_begin));
	memory_mask = memory_size - 1;

	rh = (read16_handler) bank;
	wh = is_rom ? MWA16_ROM : (write16_handler) bank;

	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, memory_begin,
		memory_end, memory_mask, 0, rh);
	memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, memory_begin,
		memory_end, memory_mask, 0, wh);

	memory_set_bankptr(bank, memory_data);

	if (LOG_MEMORY)
	{
		logerror("mac_install_memory(): bank=%d range=[0x%06x...0x%06x] mask=0x%06x ptr=0x%p\n",
			bank, memory_begin, memory_end, memory_mask, memory_data);
	}
}



/*
	Interrupt handling
*/

static int scc_interrupt, via_interrupt;

static void mac_field_interrupts(void)
{
	if (scc_interrupt)
		/* SCC interrupt */
		cpunum_set_input_line(0, 2, ASSERT_LINE);
	else if (via_interrupt)
		/* VIA interrupt */
		cpunum_set_input_line(0, 1, ASSERT_LINE);
	else
		/* clear all interrupts */
		cpunum_set_input_line(0, 7, CLEAR_LINE);
}

static void set_scc_interrupt(int value)
{
	scc_interrupt = value;
	mac_field_interrupts();
}

static void set_via_interrupt(int value)
{
	via_interrupt = value;
	mac_field_interrupts();
}


WRITE16_HANDLER ( mac_autovector_w )
{
	if (LOG_GENERAL)
		logerror("mac_autovector_w: offset=0x%08x data=0x%04x\n", offset, data);

	/* This should throw an exception */

	/* Not yet implemented */
}

READ16_HANDLER ( mac_autovector_r )
{
	if (LOG_GENERAL)
		logerror("mac_autovector_r: offset=0x%08x\n", offset);

	/* This should throw an exception */

	/* Not yet implemented */
	return 0;
}

static void set_scc_waitrequest(int waitrequest)
{
	if (LOG_GENERAL)
		logerror("set_scc_waitrequest: waitrequest=%i\n", waitrequest);

	/* Not Yet Implemented */
}



static void set_memory_overlay(int overlay)
{
	offs_t memory_size;
	UINT8 *memory_data;
	int is_rom;

	/* normalize overlay */
	overlay = overlay ? TRUE : FALSE;

	if (overlay != mac_overlay)
	{
		/* set up either main RAM area or ROM mirror at 0x000000-0x3fffff */
		if (overlay)
		{
			/* ROM mirror */
			memory_size = memory_region_length(REGION_USER1);
			memory_data = memory_region(REGION_USER1);
			is_rom = TRUE;

			/* HACK! - copy in the initial reset/stack */
			memcpy(mess_ram, memory_data, 8);
		}
		else
		{
			/* RAM */
			memory_size = mess_ram_size;
			memory_data = mess_ram;
			is_rom = FALSE;
		}

		/* install the memory */
		mac_install_memory(0x000000, 0x3fffff, memory_size, memory_data, is_rom, 1);

		mac_overlay = overlay;

		if (LOG_GENERAL)
			logerror("set_memory_overlay: overlay=%i\n", overlay);
	}
}



/*
	R Nabet 000531 : added keyboard code
*/

/* *************************************************************************
 * non-ADB keyboard support
 *
 * The keyboard uses a i8021 (?) microcontroller.
 * It uses a bidirectional synchonous serial line, connected to the VIA (SR feature)
 *
 * Our emulation is more a hack than anything else - the keyboard controller is
 * not emulated, instead we interpret keyboard commands directly.  I made
 * many guesses, which may be wrong
 *
 * todo :
 * * find the correct model number for the Mac Plus keyboard ?
 * * emulate original Macintosh keyboards (2 layouts : US and international)
 *
 * references :
 * * IM III-29 through III-32 and III-39 through III-42
 * * IM IV-250
 * *************************************************************************/

/* used to store the reply to most keyboard commands */
static int keyboard_reply;

/* Keyboard communication in progress? */
static int kbd_comm;
static int kbd_receive;
/* timer which is used to time out inquiry */
static mame_timer *inquiry_timeout;

static int kbd_shift_reg;
static int kbd_shift_count;

/*
	scan_keyboard()

	scan the keyboard, and returns key transition code (or NULL ($7B) if none)
*/

/* keyboard matrix to detect transition */
static int key_matrix[7];

/* keycode buffer (used for keypad/arrow key transition) */
static int keycode_buf[2];
static int keycode_buf_index;

static int scan_keyboard()
{
	int i, j;
	int keybuf;
	int keycode;

	if (keycode_buf_index)
	{
		return keycode_buf[--keycode_buf_index];
	}

	for (i=0; i<7; i++)
	{
		keybuf = readinputport(i+3);

		if (keybuf != key_matrix[i])
		{
			/* if state has changed, find first bit which has changed */
			if (LOG_KEYBOARD)
				logerror("keyboard state changed, %d %X\n", i, keybuf);

			for (j=0; j<16; j++)
			{
				if (((keybuf ^ key_matrix[i]) >> j) & 1)
				{
					/* update key_matrix */
					key_matrix[i] = (key_matrix[i] & ~ (1 << j)) | (keybuf & (1 << j));

					if (i < 4)
					{
						/* create key code */
						keycode = (i << 5) | (j << 1) | 0x01;
						if (! (keybuf & (1 << j)))
						{
							/* key up */
							keycode |= 0x80;
						}
						return keycode;
					}
					else if (i < 6)
					{
						/* create key code */
						keycode = ((i & 3) << 5) | (j << 1) | 0x01;

						if ((keycode == 0x05) || (keycode == 0x0d) || (keycode == 0x11) || (keycode == 0x1b))
						{
							/* these keys cause shift to be pressed (for compatibility with mac 128/512) */
							if (keybuf & (1 << j))
							{
								/* key down */
								if (! (key_matrix[3] & 0x0100))
								{
									/* shift key is really up */
									keycode_buf[0] = keycode;
									keycode_buf[1] = 0x79;
									keycode_buf_index = 2;
									return 0x71;	/* "presses" shift down */
								}
							}
							else
							{	/* key up */
								if (! (key_matrix[3] & 0x0100))
								{
									/* shift key is really up */
									keycode_buf[0] = keycode | 0x80;;
									keycode_buf[1] = 0x79;
									keycode_buf_index = 2;
									return 0xF1;	/* "releases" shift */
								}
							}
						}

						if (! (keybuf & (1 << j)))
						{
							/* key up */
							keycode |= 0x80;
						}
						keycode_buf[0] = keycode;
						keycode_buf_index = 1;
						return 0x79;
					}
					else /* i == 6 */
					{
						/* create key code */
						keycode = (j << 1) | 0x01;
						if (! (keybuf & (1 << j)))
						{
							/* key up */
							keycode |= 0x80;
						}
						keycode_buf[0] = keycode;
						keycode_buf_index = 1;
						return 0x79;
					}
				}
			}
		}
	}

	return 0x7B;	/* return NULL */
}

/*
	power-up init
*/
static void keyboard_init(void)
{
	int i;

	/* init flag */
	kbd_comm = FALSE;
	kbd_receive = FALSE;
	kbd_shift_reg=0;
	kbd_shift_count=0;

	/* clear key matrix */
	for (i=0; i<7; i++)
	{
		key_matrix[i] = 0;
	}

	/* purge transmission buffer */
	keycode_buf_index = 0;
}

/******************* Keyboard <-> VIA communication ***********************/

static void kbd_clock(int param)
{
	int i;
	if (kbd_comm == TRUE)
	{
		for (i=0; i<8; i++)
		{
			/* Put data on CB2 if we are sending*/
			if (kbd_receive == FALSE)
				via_set_input_cb2(0, kbd_shift_reg&0x80?1:0);
			kbd_shift_reg <<= 1;
			via_set_input_cb1(0, 0);
			via_set_input_cb1(0, 1);
		}
		if (kbd_receive == TRUE)
		{
			kbd_receive = FALSE;
			/* Process the command received from mac */
			keyboard_receive(kbd_shift_reg & 0xff);
		}
		else
		{
			/* Communication is over */ 
			kbd_comm = FALSE;
		}
	}
}

static void kbd_shift_out(int data)
{
	if (kbd_comm == TRUE)
	{
		kbd_shift_reg = data;
		timer_set(1e-3, 0, kbd_clock);
	}
}

static WRITE8_HANDLER(mac_via_out_cb2)
{
	if (kbd_comm == FALSE && data == 0)
	{
		/* Mac pulls CB2 down to initiate communication */
		kbd_comm = TRUE;
		kbd_receive = TRUE;
		timer_set(1e-4, 0, kbd_clock);
	}
	if (kbd_comm == TRUE && kbd_receive == TRUE)
	{
		/* Shift in what mac is sending */
		kbd_shift_reg = (kbd_shift_reg & ~1) | data;
	}
}

/*
	called when inquiry times out (1/4s)
*/
static void inquiry_timeout_func(int unused)
{
	if (LOG_KEYBOARD)
		logerror("keyboard enquiry timeout\n");
	kbd_shift_out(0x7B);	/* always send NULL */
}

/*
	called when a command is received from the mac
*/
static void keyboard_receive(int val)
{
	switch (val)
	{
	case 0x10:
		/* inquiry - returns key transition code, or NULL ($7B) if time out (1/4s) */
		if (LOG_KEYBOARD)
			logerror("keyboard command : inquiry\n");

		keyboard_reply = scan_keyboard();
		if (keyboard_reply == 0x7B)
		{	
			/* if NULL, wait until key pressed or timeout */
			mame_timer_adjust(inquiry_timeout,
				make_mame_time(0, DOUBLE_TO_SUBSECONDS(0.25)),
				0, time_zero);
		}
		break;

	case 0x14:
		/* instant - returns key transition code, or NULL ($7B) */
		if (LOG_KEYBOARD)
			logerror("keyboard command : instant\n");

		kbd_shift_out(scan_keyboard());
		break;

	case 0x16:
		/* model number - resets keyboard, return model number */
		if (LOG_KEYBOARD)
			logerror("keyboard command : model number\n");

		{	/* reset */
			int i;

			/* clear key matrix */
			for (i=0; i<7; i++)
			{
				key_matrix[i] = 0;
			}

			/* purge transmission buffer */
			keycode_buf_index = 0;
		}

		/* format : 1 if another device (-> keypad ?) connected | next device (-> keypad ?) number 1-8
							| keyboard model number 1-8 | 1  */
		/* keyboards :
			3 : mac 512k, US and international layout ? Mac plus ???
			other values : Apple II keyboards ?
		*/
		/* keypads :
			??? : standard keypad (always available on Mac Plus) ???
		*/
		kbd_shift_out(0x17);	/* probably wrong */
		break;

	case 0x36:
		/* test - resets keyboard, return ACK ($7D) or NAK ($77) */
		if (LOG_KEYBOARD)
			logerror("keyboard command : test\n");

		kbd_shift_out(0x7D);	/* ACK */
		break;

	default:
		if (LOG_KEYBOARD)
			logerror("unknown keyboard command 0x%X\n", val);

		kbd_shift_out(0);
		break;
	}
}

/* *************************************************************************
 * Mouse
 * *************************************************************************/

static int mouse_bit_x = 0, mouse_bit_y = 0;

static void mouse_callback(void)
{
	static int	last_mx = 0, last_my = 0;
	static int	count_x = 0, count_y = 0;

	int			new_mx, new_my;
	int			x_needs_update = 0, y_needs_update = 0;

	new_mx = readinputport(1);
	new_my = readinputport(2);

	/* see if it moved in the x coord */
	if (new_mx != last_mx)
	{
		int		diff = new_mx - last_mx;

		/* check for wrap */
		if (diff > 0x80)
			diff = 0x100-diff;
		if  (diff < -0x80)
			diff = -0x100-diff;

		count_x += diff;

		last_mx = new_mx;
	}
	/* see if it moved in the y coord */
	if (new_my != last_my)
	{
		int		diff = new_my - last_my;

		/* check for wrap */
		if (diff > 0x80)
			diff = 0x100-diff;
		if  (diff < -0x80)
			diff = -0x100-diff;

		count_y += diff;

		last_my = new_my;
	}

	/* update any remaining count and then return */
	if (count_x)
	{
		if (count_x < 0)
		{
			count_x++;
			mouse_bit_x = 0;
		}
		else
		{
			count_x--;
			mouse_bit_x = 1;
		}
		x_needs_update = 1;
	}
	else if (count_y)
	{
		if (count_y < 0)
		{
			count_y++;
			mouse_bit_y = 1;
		}
		else
		{
			count_y--;
			mouse_bit_y = 0;
		}
		y_needs_update = 1;
	}

	if (x_needs_update || y_needs_update)
		/* assert Port B External Interrupt on the SCC */
		mac_scc_mouse_irq( x_needs_update, y_needs_update );
}

/* *************************************************************************
 * SCSI
 * *************************************************************************/

/*

From http://www.mac.m68k-linux.org/devel/plushw.php

The address of each register is computed as follows:

  $580drn

  where r represents the register number (from 0 through 7),
  n determines whether it a read or write operation
  (0 for reads, or 1 for writes), and
  d determines whether the DACK signal to the NCR 5380 is asserted.
  (0 for not asserted, 1 is for asserted)

Here's an example of the address expressed in binary:

  0101 1000 0000 00d0 0rrr 000n

Note:  Asserting the DACK signal applies only to write operations to
       the output data register and read operations from the input
       data register.

  Symbolic            Memory
  Location            Location   NCR 5380 Internal Register

  scsiWr+sODR+dackWr  $580201    Output Data Register with DACK
  scsiRd+sIDR+dackRd  $580260    Current SCSI Data with DACK
  scsiWr+sODR         $580001    Output Data Register
  scsiWr+sICR         $580011    Initiator Command Register
  scsiWr+sMR          $580021    Mode Register
  scsiWr+sTCR         $580031    Target Command Register
  scsiWr+sSER         $580041    Select Enable Register
  scsiWr+sDMAtx       $580051    Start DMA Send
  scsiWr+sTDMArx      $580061    Start DMA Target Receive
  scsiWr+sIDMArx      $580071    Start DMA Initiator Receive
  scsiRd+sCDR         $580000    Current SCSI Data
  scsiRd+sICR         $580010    Initiator Command Register
  scsiRd+sMR          $580020    Mode Registor
  scsiRd+sTCR         $580030    Target Command Register
  scsiRd+sCSR         $580040    Current SCSI Bus Status
  scsiRd+sBSR         $580050    Bus and Status Register
  scsiRd+sIDR         $580060    Input Data Register
  scsiRd+sRESET       $580070    Reset Parity/Interrupt

			 */

READ16_HANDLER ( macplus_scsi_r )
{
	int reg = (offset>>3) & 0xf;

	if ((reg == 6) && (offset == 0x130))
	{
		reg = R5380_CURDATA_DTACK;
	}

	return ncr5380_r(reg)<<8;
}

WRITE16_HANDLER ( macplus_scsi_w )
{
	int reg = (offset>>3) & 0xf;

	if ((reg == 0) && (offset == 0x100))
	{
		reg = R5380_OUTDATA_DTACK;
	}

	ncr5380_w(reg, data);
}

/* *************************************************************************
 * SCC
 *
 * Serial Control Chip
 * *************************************************************************/

static void mac_scc_ack(void)
{
	set_scc_interrupt(0);
}



void mac_scc_mouse_irq( int x, int y)
{
	static int last_was_x = 0;

	if (x && y)
	{
		if (last_was_x)
			scc_set_status(0x0a);
		else
			scc_set_status(0x02);

		last_was_x ^= 1;
	}
	else
	{
		if (x)
			scc_set_status(0x0a);
		else
			scc_set_status(0x02);
	}

	//cpunum_set_input_line(0, 2, ASSERT_LINE);
	set_scc_interrupt(1);
}



READ16_HANDLER ( mac_scc_r )
{
	UINT16 result;
	result = scc_r(offset);
	return (result << 8) | result;
}



WRITE16_HANDLER ( mac_scc_w )
{
	scc_w(offset, (UINT8) data);
}



static const struct scc8530_interface mac_scc8530_interface =
{
	mac_scc_ack
};



/* *************************************************************************
 * RTC
 *
 * Real Time Clock chip - contains clock information and PRAM.  This chip is
 * accessed through the VIA
 * *************************************************************************/

/* state of rTCEnb and rTCClk lines */
static unsigned char rtc_rTCEnb;
static unsigned char rtc_rTCClk;

/* serial transmit/receive register : bits are shifted in/out of this byte */
static unsigned char rtc_data_byte;
/* serial transmitted/received bit count */
static unsigned char rtc_bit_count;
/* direction of the current transfer (0 : VIA->RTC, 1 : RTC->VIA) */
static unsigned char rtc_data_dir;
/* when rtc_data_dir == 1, state of rTCData as set by RTC (-> data bit seen by VIA) */
static unsigned char rtc_data_out;

/* set to 1 when command in progress */
static unsigned char rtc_cmd;

/* write protect flag */
static unsigned char rtc_write_protect;

/* internal seconds register */
static unsigned char rtc_seconds[/*8*/4];
/* 20-byte long PRAM, or 256-byte long XPRAM */
static unsigned char rtc_ram[256];
/* current extended address and RTC state */
static unsigned char rtc_xpaddr, rtc_state;

/* a few protos */
static void rtc_write_rTCEnb(int data);
static void rtc_execute_cmd(int data);

enum
{
	RTC_STATE_NORMAL,
	RTC_STATE_WRITE,
	RTC_STATE_XPCOMMAND,
	RTC_STATE_XPWRITE
};

/* init the rtc core */
static void rtc_init(void)
{
	rtc_rTCClk = 0;

	rtc_write_protect = TRUE;	/* Mmmmh... Should be saved with the NVRAM, actually... */
	rtc_rTCEnb = 0;
	rtc_write_rTCEnb(1);
	rtc_state = RTC_STATE_NORMAL;
}

/* write the rTCEnb state */
static void rtc_write_rTCEnb(int val)
{
	if (val && (! rtc_rTCEnb))
	{
		/* rTCEnb goes high (inactive) */
		rtc_rTCEnb = 1;
		/* abort current transmission */
		rtc_data_byte = rtc_bit_count = rtc_data_dir = rtc_data_out = 0;
		rtc_state = RTC_STATE_NORMAL;
	}
	else if ((! val) && rtc_rTCEnb)
	{
		/* rTCEnb goes low (active) */
		rtc_rTCEnb = 0;
		/* abort current transmission */
		rtc_data_byte = rtc_bit_count = rtc_data_dir = rtc_data_out = 0;
		rtc_state = RTC_STATE_NORMAL;
	}
}

/* shift data (called on rTCClk high-to-low transition (?)) */
static void rtc_shift_data(int data)
{
	if (rtc_rTCEnb)
		/* if enable line inactive (high), do nothing */
		return;

	if (rtc_data_dir)
	{	/* RTC -> VIA transmission */
		rtc_data_out = (rtc_data_byte >> --rtc_bit_count) & 0x01;
		if (LOG_RTC)
			logerror("RTC shifted new data %d\n", rtc_data_out);
	}
	else
	{	/* VIA -> RTC transmission */
		rtc_data_byte = (rtc_data_byte << 1) | (data ? 1 : 0);

		if (++rtc_bit_count == 8)
		{	/* if one byte received, send to command interpreter */
			rtc_execute_cmd(rtc_data_byte);
		}
	}
}

/* called every second, to increment the Clock count */
static void rtc_incticks(void)
{
	if (LOG_RTC)
		logerror("rtc_incticks called\n");

	if (++rtc_seconds[0] == 0)
		if (++rtc_seconds[1] == 0)
			if (++rtc_seconds[2] == 0)
				++rtc_seconds[3];

	/*if (++rtc_seconds[4] == 0)
		if (++rtc_seconds[5] == 0)
			if (++rtc_seconds[6] == 0)
				++rtc_seconds[7];*/
}

/* Executes a command.
Called when the first byte after "enable" is received, and when the data byte after a write command
is received. */
static void rtc_execute_cmd(int data)
{
	int i;

	if (LOG_RTC)
		logerror("rtc_execute_cmd: data=%x, state=%x\n", data, rtc_state);

	if (rtc_state == RTC_STATE_XPCOMMAND)
	{
		rtc_xpaddr = ((rtc_cmd & 7)<<5) | ((data&0x7c)>>2);
		if ((rtc_cmd & 0x80) != 0)	
		{
			// read command
			if (LOG_RTC)
				logerror("RTC: Reading extended address %x\n", rtc_xpaddr);

			rtc_data_dir = 1;
			rtc_data_byte = rtc_ram[rtc_xpaddr];
			rtc_state = RTC_STATE_NORMAL;
		}
		else
		{
			// write command
			rtc_state = RTC_STATE_XPWRITE;
			rtc_data_byte = 0;
			rtc_bit_count = 0;
		}
	}
	else if (rtc_state == RTC_STATE_XPWRITE)
	{
		if (LOG_RTC)
			logerror("RTC: writing %x to extended address %x\n", data, rtc_xpaddr);
		rtc_ram[rtc_xpaddr] = data;
		rtc_state = RTC_STATE_NORMAL;
	}
	else if (rtc_state == RTC_STATE_WRITE)
	{
		rtc_state = RTC_STATE_NORMAL;

		/* Writing an RTC register */
		i = (rtc_cmd >> 2) & 0x1f;
		if (rtc_write_protect && (i != 13))
			/* write-protection : only write-protect can be written again */
			return;
		switch(i)
		{
		case 0: case 1: case 2: case 3:	/* seconds register */
		case 4: case 5: case 6: case 7:	/* ??? (not described in IM III) */
			/* after various tries, I assumed rtc_seconds[4+i] is mapped to rtc_seconds[i] */
			if (LOG_RTC)
				logerror("RTC clock write, address = %X, data = %X\n", i, (int) rtc_data_byte);
			rtc_seconds[i & 3] = rtc_data_byte;
			break;

		case 8: case 9: case 10: case 11:	/* RAM address $10-$13 */
			if (LOG_RTC)
				logerror("RTC RAM write, address = %X, data = %X\n", (i & 3) + 0x10, (int) rtc_data_byte);
			rtc_ram[(i & 3) + 0x10] = rtc_data_byte;
			break;

		case 12:
			/* Test register - do nothing */
			if (LOG_RTC)
				logerror("RTC write to test register, data = %X\n", (int) rtc_data_byte);
			break;

		case 13:
			/* Write-protect register  */
			if (LOG_RTC)
				logerror("RTC write to write-protect register, data = %X\n", (int) rtc_data_byte);
			rtc_write_protect = (rtc_data_byte & 0x80) ? TRUE : FALSE;
			break;

		case 16: case 17: case 18: case 19:	/* RAM address $00-$0f */
		case 20: case 21: case 22: case 23:
		case 24: case 25: case 26: case 27:
		case 28: case 29: case 30: case 31:
			if (LOG_RTC)
				logerror("RTC RAM write, address = %X, data = %X\n", i & 15, (int) rtc_data_byte);
			rtc_ram[i & 15] = rtc_data_byte;
			break;

		default:
			logerror("Unknown RTC write command : %X, data = %d\n", (int) rtc_cmd, (int) rtc_data_byte);
			break;
		}
	}
	else
	{
		// always save this byte to rtc_cmd
		rtc_cmd = rtc_data_byte;

		if ((rtc_cmd & 0x78) == 0x38)	// extended command
		{
			rtc_state = RTC_STATE_XPCOMMAND;
			rtc_data_byte = 0;
			rtc_bit_count = 0;
		}
		else
		{
			if (rtc_cmd & 0x80)
			{
				rtc_state = RTC_STATE_NORMAL;

				/* Reading an RTC register */
				rtc_data_dir = 1;
				i = (rtc_cmd >> 2) & 0x1f;
				switch(i)
				{
					case 0: case 1: case 2: case 3:
					case 4: case 5: case 6: case 7:
						rtc_data_byte = rtc_seconds[i & 3];
						if (LOG_RTC)
							logerror("RTC clock read, address = %X -> data = %X\n", i, rtc_data_byte);
						break;

					case 8: case 9: case 10: case 11:
						if (LOG_RTC)
							logerror("RTC RAM read, address = %X\n", (i & 3) + 0x10);
						rtc_data_byte = rtc_ram[(i & 3) + 0x10];
						break;

					case 16: case 17: case 18: case 19:
					case 20: case 21: case 22: case 23:
					case 24: case 25: case 26: case 27:
					case 28: case 29: case 30: case 31:
						if (LOG_RTC)
							logerror("RTC RAM read, address = %X\n", i & 15);
						rtc_data_byte = rtc_ram[i & 15];
						break;

					default:
						if (LOG_RTC)
							logerror("Unknown RTC read command : %X\n", (int) rtc_cmd);
						rtc_data_byte = 0;
						break;
				}
			}
			else
			{
				/* Writing an RTC register */
				/* wait for extra data byte */
				if (LOG_RTC)
					logerror("RTC write, waiting for data byte : %X\n", (int) rtc_cmd);
				rtc_state = RTC_STATE_WRITE;
				rtc_data_byte = 0;
				rtc_bit_count = 0;
			}
		}
	}
}

/* should save PRAM to file */
/* TODO : save write_protect flag, save time difference with host clock */
NVRAM_HANDLER( mac )
{
	if (read_or_write)
	{
		if (LOG_RTC)
			logerror("Writing PRAM to file\n");
		mame_fwrite(file, rtc_ram, sizeof(rtc_ram));
	}
	else
	{
		if (file)
		{
			if (LOG_RTC)
				logerror("Reading PRAM from file\n");
			mame_fread(file, rtc_ram, sizeof(rtc_ram));
		}
		else
		{
			if (LOG_RTC)
				logerror("trashing PRAM\n");
		}

		{
			/* Now we copy the host clock into the Mac clock */
			/* Cool, isn't it ? :-) */
			mame_system_time systime;
			struct tm mac_reference;
			UINT32 seconds;

			mame_get_base_datetime(Machine, &systime);

			/* The count starts on 1st January 1904 */
			mac_reference.tm_sec = 0;
			mac_reference.tm_min = 0;
			mac_reference.tm_hour = 0;
			mac_reference.tm_mday = 1;
			mac_reference.tm_mon = 0;
			mac_reference.tm_year = 4;
			mac_reference.tm_isdst = 0;

			seconds = difftime((time_t) systime.time, mktime(& mac_reference));

			if (LOG_RTC)
				logerror("second count 0x%lX\n", (unsigned long) seconds);

			rtc_seconds[0] = seconds & 0xff;
			rtc_seconds[1] = (seconds >> 8) & 0xff;
			rtc_seconds[2] = (seconds >> 16) & 0xff;
			rtc_seconds[3] = (seconds >> 24) & 0xff;
		}
	}
}

/* ********************************** *
 * IWM Code specific to the Mac Plus  *
 * ********************************** */

READ16_HANDLER ( mac_iwm_r )
{
	/* The first time this is called is in a floppy test, which goes from
	 * $400104 to $400126.  After that, all access to the floppy goes through
	 * the disk driver in the MacOS
	 *
	 * I just thought this would be on interest to someone trying to further
	 * this driver along
	 */

	int result = 0;

	if (LOG_MAC_IWM)
		logerror("mac_iwm_r: offset=0x%08x\n", offset);

	result = applefdc_r(offset >> 8);
	return (result << 8) | result;
}

WRITE16_HANDLER ( mac_iwm_w )
{
	if (LOG_MAC_IWM)
		logerror("mac_iwm_w: offset=0x%08x data=0x%04x\n", offset, data);

	if (ACCESSING_LSB)
		applefdc_w(offset >> 8, data & 0xff);
}

/* *************************************************************************
 * ADB
 * *************************************************************************/

static int adb_via_b3;

static int has_adb(void)
{
	return mac_model >= MODEL_MAC_SE;
}



static void mac_adb_newaction(int state)
{
	adb_via_b3 = 1;
}



/* *************************************************************************
 * VIA
 * *************************************************************************
 *
 *
 * PORT A
 *
 *	bit 7				R	SCC Wait/Request
 *	bit 6				W	Main/Alternate screen buffer select
 *	bit 5				W	Floppy Disk Line Selection
 *	bit 4				W	Overlay/Normal memory mapping
 *	bit 3				W	Main/Alternate sound buffer
 *	bit 2-0				W	Sound Volume
 *
 *
 * PORT B
 *
 *	bit 7				W	Sound enable
 *	bit 6				R	Video beam in display
 *	bit 5	(pre-ADB)	R	Mouse Y2
 *			(ADB)		W	ADB ST1
 *	bit	4	(pre-ADB)	R	Mouse X2
 *			(ADB)		W	ADB ST0
 *	bit 3	(pre-ADB)	R	Mouse button (active low)
 *			(ADB)		R	ADB INT
 *	bit 2				W	Real time clock enable
 *	bit 1				W	Real time clock data clock
 *	bit 0				RW	Real time clock data
 *
 */

static READ8_HANDLER(mac_via_in_a)
{
	return 0x80;
}

static READ8_HANDLER(mac_via_in_b)
{
	int val = 0;

	/* video beam in display (! VBLANK && ! HBLANK basically) */
	if (cpu_getvblank())
		val |= 0x40;

	if (has_adb())
	{
		if (adb_via_b3)
			val |= 0x08;
	}
	else
	{
		if (mouse_bit_y)	/* Mouse Y2 */
			val |= 0x20;
		if (mouse_bit_x)	/* Mouse X2 */
			val |= 0x10;
		if ((readinputport(0) & 0x01) == 0)
			val |= 0x08;
	}
	if (rtc_data_out)
		val |= 1;

	return val;
}

static WRITE8_HANDLER(mac_via_out_a)
{
	set_scc_waitrequest((data & 0x80) >> 7);
	mac_set_screen_buffer((data & 0x40) >> 6);
	sony_set_sel_line((data & 0x20) >> 5);
	mac_set_sound_buffer((data & 0x08) >> 3);
	mac_set_volume(data & 0x07);

	/* Early Mac models had VIA A4 control overlaying.  In the Mac SE (and
	 * possibly later models), overlay was set on reset, but cleared on the
	 * first access to the ROM. */
	if (mac_model < MODEL_MAC_SE)
		set_memory_overlay((data & 0x10) >> 4);
}

static WRITE8_HANDLER(mac_via_out_b)
{
	int new_rtc_rTCClk;

	mac_enable_sound((data & 0x80) == 0);
	rtc_write_rTCEnb(data & 0x04);
	new_rtc_rTCClk = (data >> 1) & 0x01;
	if ((! new_rtc_rTCClk) && (rtc_rTCClk))
		rtc_shift_data(data & 0x01);
	rtc_rTCClk = new_rtc_rTCClk;

	if (has_adb())
		mac_adb_newaction((data & 0x30) >> 4);
}

static void mac_via_irq(int state)
{
	/* interrupt the 68k (level 1) */
	//cpunum_set_input_line(0, 1, state);
	set_via_interrupt(state);
}

READ16_HANDLER ( mac_via_r )
{
	int data;

	offset >>= 8;
	offset &= 0x0f;

	if (LOG_VIA)
		logerror("mac_via_r: offset=0x%02x\n", offset);
	data = via_0_r(offset);

	return (data & 0xff) | (data << 8);
}

WRITE16_HANDLER ( mac_via_w )
{
	offset >>= 8;
	offset &= 0x0f;

	if (LOG_VIA)
		logerror("mac_via_w: offset=0x%02x data=0x%08x\n", offset, data);

	if (ACCESSING_MSB)
		via_0_w(offset, (data >> 8) & 0xff);
}



/* *************************************************************************
 * Main
 * *************************************************************************/

MACHINE_RESET(mac)
{
	/* initialize real-time clock */
	rtc_init();

	/* initialize serial */
	scc_init(&mac_scc8530_interface);

	/* initialize floppy */
	{
		struct applefdc_interface intf =
		{
			APPLEFDC_IWM,
			sony_set_lines,
			sony_set_enable_lines,

			sony_read_data,
			sony_write_data,
			sony_read_status
		};

		applefdc_init(&intf);
	}

	/* setup the memory overlay */
	set_memory_overlay(1);

	/* reset the via */
	via_reset();

	/* setup videoram */
	mac_set_screen_buffer(1);

	/* setup sound */
	mac_set_sound_buffer(0);

	if (mac_model == MODEL_MAC_SE)
		timer_set(0.0, 0, set_memory_overlay);
}



static void mac_state_load(void)
{
	int overlay = mac_overlay;
	mac_overlay = -1;
	set_memory_overlay(overlay);
}



static void mac_driver_init(mac_model_t model)
{
	mac_overlay = -1;
	mac_model = model;

	/* set up RAM mirror at 0x600000-0x6fffff (0x7fffff ???) */
	mac_install_memory(0x600000, 0x6fffff, mess_ram_size, mess_ram, FALSE, 2);

	/* set up ROM at 0x400000-0x43ffff (-0x5fffff for mac 128k/512k/512ke) */
	mac_install_memory(0x400000, (model == MODEL_MAC_PLUS) ? 0x43ffff : 0x5fffff,
		memory_region_length(REGION_USER1), memory_region(REGION_USER1), TRUE, 3);

	set_memory_overlay(1);

	/* configure via */
	via_config(0, &mac_via6522_intf);
	via_set_clock(0, 1000000);	/* 6522 = 1 Mhz, 6522a = 2 Mhz */

	/* setup keyboard */
	keyboard_init();

	inquiry_timeout = mame_timer_alloc(inquiry_timeout_func);

	cpuintrf_set_dasm_override(0, mac_dasm_override);

	/* save state stuff */
	state_save_register_global(mac_overlay);
	state_save_register_func_postload(mac_state_load);
}



DRIVER_INIT(mac128k512k)
{
	mac_driver_init(MODEL_MAC_128K512K);
}

DRIVER_INIT(mac512ke)
{
	mac_driver_init(MODEL_MAC_512KE);
}

static SCSIConfigTable dev_table =
{
	1,                                      /* 1 SCSI device */
	{ { SCSI_ID_6, 0, SCSI_DEVICE_HARDDISK } } /* SCSI ID 6, using CHD 0, and it's a harddisk */
};

static struct NCR5380interface macplus_5380intf =
{
	&dev_table,	// SCSI device table
	NULL		// IRQ (unconnected on the Mac Plus)
};

DRIVER_INIT(macplus)
{
	mac_driver_init(MODEL_MAC_PLUS);

	ncr5380_init(&macplus_5380intf);
}

DRIVER_INIT(macse)
{
	mac_driver_init(MODEL_MAC_SE);

	ncr5380_init(&macplus_5380intf);
}


static void mac_vblank_irq(void)
{
	static int irq_count = 0, ca1_data = 0, ca2_data = 0;

	/* handle keyboard */
	if (kbd_comm == TRUE)
	{
		int keycode = scan_keyboard();

		if (keycode != 0x7B)
		{
			/* if key pressed, send the code */

			logerror("keyboard enquiry successful, keycode %X\n", keycode);

			timer_reset(inquiry_timeout, TIME_NEVER);
			kbd_shift_out(keycode);
		}
	}

	/* signal VBlank on CA1 input on the VIA */
	ca1_data ^= 1;
	via_set_input_ca1(0, ca1_data);

	if (++irq_count == 60)
	{
		irq_count = 0;

		ca2_data ^= 1;
		/* signal 1 Hz irq on CA2 input on the VIA */
		via_set_input_ca2(0, ca2_data);

		rtc_incticks();
	}
}



INTERRUPT_GEN( mac_interrupt )
{
	int scanline;

	mac_sh_updatebuffer();

	scanline = cpu_getscanline();
	if (scanline == 342)
		mac_vblank_irq();

	/* check for mouse changes at 10 irqs per frame */
	if (!(scanline % 10))
		mouse_callback();
}



/* *************************************************************************
 * Trap Tracing
 *
 * This is debug code that will output diagnostics regarding OS traps called
 * *************************************************************************/

static const char *lookup_trap(UINT16 opcode)
{
	static const struct
	{
		UINT16 trap;
		const char *name;
	} traps[] =
	{
		{ 0xA000, "_Open" },
		{ 0xA001, "_Close" },
		{ 0xA002, "_Read" },
		{ 0xA003, "_Write" },
		{ 0xA004, "_Control" },
		{ 0xA005, "_Status" },
		{ 0xA006, "_KillIO" },
		{ 0xA007, "_GetVolInfo" },
		{ 0xA008, "_Create" },
		{ 0xA009, "_Delete" },
		{ 0xA00A, "_OpenRF" },
		{ 0xA00B, "_Rename" },
		{ 0xA00C, "_GetFileInfo" },
		{ 0xA00D, "_SetFileInfo" },
		{ 0xA00E, "_UnmountVol" },
		{ 0xA00F, "_MountVol" },
		{ 0xA010, "_Allocate" },
		{ 0xA011, "_GetEOF" },
		{ 0xA012, "_SetEOF" },
		{ 0xA013, "_FlushVol" },
		{ 0xA014, "_GetVol" },
		{ 0xA015, "_SetVol" },
		{ 0xA016, "_FInitQueue" },
		{ 0xA017, "_Eject" },
		{ 0xA018, "_GetFPos" },
		{ 0xA019, "_InitZone" },
		{ 0xA01B, "_SetZone" },
		{ 0xA01C, "_FreeMem" },
		{ 0xA01F, "_DisposePtr" },
		{ 0xA020, "_SetPtrSize" },
		{ 0xA021, "_GetPtrSize" },
		{ 0xA023, "_DisposeHandle" },
		{ 0xA024, "_SetHandleSize" },
		{ 0xA025, "_GetHandleSize" },
		{ 0xA027, "_ReallocHandle" },
		{ 0xA029, "_HLock" },
		{ 0xA02A, "_HUnlock" },
		{ 0xA02B, "_EmptyHandle" },
		{ 0xA02C, "_InitApplZone" },
		{ 0xA02D, "_SetApplLimit" },
		{ 0xA02E, "_BlockMove" },
		{ 0xA02F, "_PostEvent" },
		{ 0xA030, "_OSEventAvail" },
		{ 0xA031, "_GetOSEvent" },
		{ 0xA032, "_FlushEvents" },
		{ 0xA033, "_VInstall" },
		{ 0xA034, "_VRemove" },
		{ 0xA035, "_OffLine" },
		{ 0xA036, "_MoreMasters" },
		{ 0xA038, "_WriteParam" },
		{ 0xA039, "_ReadDateTime" },
		{ 0xA03A, "_SetDateTime" },
		{ 0xA03B, "_Delay" },
		{ 0xA03C, "_CmpString" },
		{ 0xA03D, "_DrvrInstall" },
		{ 0xA03E, "_DrvrRemove" },
		{ 0xA03F, "_InitUtil" },
		{ 0xA040, "_ResrvMem" },
		{ 0xA041, "_SetFilLock" },
		{ 0xA042, "_RstFilLock" },
		{ 0xA043, "_SetFilType" },
		{ 0xA044, "_SetFPos" },
		{ 0xA045, "_FlushFile" },
		{ 0xA047, "_SetTrapAddress" },
		{ 0xA049, "_HPurge" },
		{ 0xA04A, "_HNoPurge" },
		{ 0xA04B, "_SetGrowZone" },
		{ 0xA04C, "_CompactMem" },
		{ 0xA04D, "_PurgeMem" },
		{ 0xA04E, "_AddDrive" },
		{ 0xA04F, "_RDrvrInstall" },
		{ 0xA050, "_CompareString" },
		{ 0xA051, "_ReadXPRam" },
		{ 0xA052, "_WriteXPRam" },
		{ 0xA054, "_UprString" },
		{ 0xA055, "_StripAddress" },
		{ 0xA056, "_LowerText" },
		{ 0xA057, "_SetAppBase" },
		{ 0xA058, "_InsTime" },
		{ 0xA059, "_RmvTime" },
		{ 0xA05A, "_PrimeTime" },
		{ 0xA05B, "_PowerOff" },
		{ 0xA05C, "_MemoryDispatch" },
		{ 0xA05D, "_SwapMMUMode" },
		{ 0xA05E, "_NMInstall" },
		{ 0xA05F, "_NMRemove" },
		{ 0xA060, "_FSDispatch" },
		{ 0xA061, "_MaxBlock" },
		{ 0xA063, "_MaxApplZone" },
		{ 0xA064, "_MoveHHi" },
		{ 0xA065, "_StackSpace" },
		{ 0xA067, "_HSetRBit" },
		{ 0xA068, "_HClrRBit" },
		{ 0xA069, "_HGetState" },
		{ 0xA06A, "_HSetState" },
		{ 0xA06C, "_InitFS" },
		{ 0xA06D, "_InitEvents" },
		{ 0xA06E, "_SlotManager" },
		{ 0xA06F, "_SlotVInstall" },
		{ 0xA070, "_SlotVRemove" },
		{ 0xA071, "_AttachVBL" },
		{ 0xA072, "_DoVBLTask" },
		{ 0xA075, "_SIntInstall" },
		{ 0xA076, "_SIntRemove" },
		{ 0xA077, "_CountADBs" },
		{ 0xA078, "_GetIndADB" },
		{ 0xA079, "_GetADBInfo" },
		{ 0xA07A, "_SetADBInfo" },
		{ 0xA07B, "_ADBReInit" },
		{ 0xA07C, "_ADBOp" },
		{ 0xA07D, "_GetDefaultStartup" },
		{ 0xA07E, "_SetDefaultStartup" },
		{ 0xA07F, "_InternalWait" },
		{ 0xA080, "_GetVideoDefault" },
		{ 0xA081, "_SetVideoDefault" },
		{ 0xA082, "_DTInstall" },
		{ 0xA083, "_SetOSDefault" },
		{ 0xA084, "_GetOSDefault" },
		{ 0xA085, "_PMgrOp" },
		{ 0xA086, "_IOPInfoAccess" },
		{ 0xA087, "_IOPMsgRequest" },
		{ 0xA088, "_IOPMoveData" },
		{ 0xA089, "_SCSIAtomic" },
		{ 0xA08A, "_Sleep" },
		{ 0xA08B, "_CommToolboxDispatch" },
		{ 0xA08D, "_DebugUtil" },
		{ 0xA08F, "_DeferUserFn" },
		{ 0xA090, "_SysEnvirons" },
		{ 0xA092, "_EgretDispatch" },
		{ 0xA094, "_ServerDispatch" },
		{ 0xA09E, "_PowerMgrDispatch" },
		{ 0xA09F, "_PowerDispatch" },
		{ 0xA0A4, "_HeapDispatch" },
		{ 0xA0AC, "_FSMDispatch" },
		{ 0xA0AE, "_VADBProc" },
		{ 0xA0DD, "_PPC" },
		{ 0xA0FE, "_TEFindWord" },
		{ 0xA0FF, "_TEFindLine" },
		{ 0xA11A, "_GetZone" },
		{ 0xA11D, "_MaxMem" },
		{ 0xA11E, "_NewPtr" },
		{ 0xA122, "_NewHandle" },
		{ 0xA126, "_HandleZone" },
		{ 0xA128, "_RecoverHandle" },
		{ 0xA12F, "_PPostEvent" },
		{ 0xA146, "_GetTrapAddress" },
		{ 0xA148, "_PtrZone" },
		{ 0xA162, "_PurgeSpace" },
		{ 0xA166, "_NewEmptyHandle" },
		{ 0xA193, "_Microseconds" },
		{ 0xA198, "_HWPriv" },
		{ 0xA1AD, "_Gestalt" },
		{ 0xA200, "_HOpen" },
		{ 0xA207, "_HGetVInfo" },
		{ 0xA208, "_HCreate" },
		{ 0xA209, "_HDelete" },
		{ 0xA20A, "_HOpenRF" },
		{ 0xA20B, "_HRename" },
		{ 0xA20C, "_HGetFileInfo" },
		{ 0xA20D, "_HSetFileInfo" },
		{ 0xA20E, "_HUnmountVol" },
		{ 0xA210, "_AllocContig" },
		{ 0xA214, "_HGetVol" },
		{ 0xA215, "_HSetVol" },
		{ 0xA22E, "_BlockMoveData" },
		{ 0xA241, "_HSetFLock" },
		{ 0xA242, "_HRstFLock" },
		{ 0xA247, "_SetOSTrapAddress" },
		{ 0xA256, "_StripText" },
		{ 0xA260, "_HFSDispatch" },
		{ 0xA285, "_IdleUpdate" },
		{ 0xA28A, "_SlpQInstall" },
		{ 0xA31E, "_NewPtrClear" },
		{ 0xA322, "_NewHandleClear" },
		{ 0xA346, "_GetOSTrapAddress" },
		{ 0xA3AD, "_NewGestalt" },
		{ 0xA456, "_UpperText" },
		{ 0xA458, "_InsXTime" },
		{ 0xA485, "_IdleState" },
		{ 0xA48A, "_SlpQRemove" },
		{ 0xA51E, "_NewPtrSys" },
		{ 0xA522, "_NewHandleSys" },
		{ 0xA562, "_PurgeSpaceSys" },
		{ 0xA5AD, "_ReplaceGestalt" },
		{ 0xA647, "_SetToolBoxTrapAddress" },
		{ 0xA656, "_StripUpperText" },
		{ 0xA685, "_SerialPower" },
		{ 0xA71E, "_NewPtrSysClear" },
		{ 0xA722, "_NewHandleSysClear" },
		{ 0xA746, "_GetToolBoxTrapAddress" },
		{ 0xA7AD, "_GetGestaltProcPtr" },
		{ 0xA800, "_SoundDispatch" },
		{ 0xA801, "_SndDisposeChannel" },
		{ 0xA802, "_SndAddModifier" },
		{ 0xA803, "_SndDoCommand" },
		{ 0xA804, "_SndDoImmediate" },
		{ 0xA805, "_SndPlay" },
		{ 0xA806, "_SndControl" },
		{ 0xA807, "_SndNewChannel" },
		{ 0xA808, "_InitProcMenu" },
		{ 0xA809, "_GetControlVariant" },
		{ 0xA80A, "_GetWVariant" },
		{ 0xA80B, "_PopUpMenuSelect" },
		{ 0xA80C, "_RGetResource" },
		{ 0xA811, "_TESelView" },
		{ 0xA812, "_TEPinScroll" },
		{ 0xA813, "_TEAutoView" },
		{ 0xA814, "_SetFractEnable" },
		{ 0xA815, "_SCSIDispatch" },
		{ 0xA817, "_CopyMask" },
		{ 0xA819, "_XMunger" },
		{ 0xA81A, "_HOpenResFile" },
		{ 0xA81B, "_HCreateResFile" },
		{ 0xA81D, "_InvalMenuBar" },
		{ 0xA821, "_MaxSizeRsrc" },
		{ 0xA822, "_ResourceDispatch" },
		{ 0xA823, "_AliasDispatch" },
		{ 0xA824, "_HFSUtilDispatch" },
		{ 0xA825, "_MenuDispatch" },
		{ 0xA826, "_InsertMenuItem" },
		{ 0xA827, "_HideDialogItem" },
		{ 0xA828, "_ShowDialogItem" },
		{ 0xA82A, "_ComponentDispatch" },
		{ 0xA833, "_ScrnBitMap" },
		{ 0xA834, "_SetFScaleDisable" },
		{ 0xA835, "_FontMetrics" },
		{ 0xA836, "_GetMaskTable" },
		{ 0xA837, "_MeasureText" },
		{ 0xA838, "_CalcMask" },
		{ 0xA839, "_SeedFill" },
		{ 0xA83A, "_ZoomWindow" },
		{ 0xA83B, "_TrackBox" },
		{ 0xA83C, "_TEGetOffset" },
		{ 0xA83D, "_TEDispatch" },
		{ 0xA83E, "_TEStyleNew" },
		{ 0xA847, "_FracCos" },
		{ 0xA848, "_FracSin" },
		{ 0xA849, "_FracSqrt" },
		{ 0xA84A, "_FracMul" },
		{ 0xA84B, "_FracDiv" },
		{ 0xA84D, "_FixDiv" },
		{ 0xA84E, "_GetItemCmd" },
		{ 0xA84F, "_SetItemCmd" },
		{ 0xA850, "_InitCursor" },
		{ 0xA851, "_SetCursor" },
		{ 0xA852, "_HideCursor" },
		{ 0xA853, "_ShowCursor" },
		{ 0xA854, "_FontDispatch" },
		{ 0xA855, "_ShieldCursor" },
		{ 0xA856, "_ObscureCursor" },
		{ 0xA858, "_BitAnd" },
		{ 0xA859, "_BitXOr" },
		{ 0xA85A, "_BitNot" },
		{ 0xA85B, "_BitOr" },
		{ 0xA85C, "_BitShift" },
		{ 0xA85D, "_BitTst" },
		{ 0xA85E, "_BitSet" },
		{ 0xA85F, "_BitClr" },
		{ 0xA860, "_WaitNextEvent" },
		{ 0xA861, "_Random" },
		{ 0xA862, "_ForeColor" },
		{ 0xA863, "_BackColor" },
		{ 0xA864, "_ColorBit" },
		{ 0xA865, "_GetPixel" },
		{ 0xA866, "_StuffHex" },
		{ 0xA867, "_LongMul" },
		{ 0xA868, "_FixMul" },
		{ 0xA869, "_FixRatio" },
		{ 0xA86A, "_HiWord" },
		{ 0xA86B, "_LoWord" },
		{ 0xA86C, "_FixRound" },
		{ 0xA86D, "_InitPort" },
		{ 0xA86E, "_InitGraf" },
		{ 0xA86F, "_OpenPort" },
		{ 0xA870, "_LocalToGlobal" },
		{ 0xA871, "_GlobalToLocal" },
		{ 0xA872, "_GrafDevice" },
		{ 0xA873, "_SetPort" },
		{ 0xA874, "_GetPort" },
		{ 0xA875, "_SetPBits" },
		{ 0xA876, "_PortSize" },
		{ 0xA877, "_MovePortTo" },
		{ 0xA878, "_SetOrigin" },
		{ 0xA879, "_SetClip" },
		{ 0xA87A, "_GetClip" },
		{ 0xA87B, "_ClipRect" },
		{ 0xA87C, "_BackPat" },
		{ 0xA87D, "_ClosePort" },
		{ 0xA87E, "_AddPt" },
		{ 0xA87F, "_SubPt" },
		{ 0xA880, "_SetPt" },
		{ 0xA881, "_EqualPt" },
		{ 0xA882, "_StdText" },
		{ 0xA883, "_DrawChar" },
		{ 0xA884, "_DrawString" },
		{ 0xA885, "_DrawText" },
		{ 0xA886, "_TextWidth" },
		{ 0xA887, "_TextFont" },
		{ 0xA888, "_TextFace" },
		{ 0xA889, "_TextMode" },
		{ 0xA88A, "_TextSize" },
		{ 0xA88B, "_GetFontInfo" },
		{ 0xA88C, "_StringWidth" },
		{ 0xA88D, "_CharWidth" },
		{ 0xA88E, "_SpaceExtra" },
		{ 0xA88F, "_OSDispatch" },
		{ 0xA890, "_StdLine" },
		{ 0xA891, "_LineTo" },
		{ 0xA892, "_Line" },
		{ 0xA893, "_MoveTo" },
		{ 0xA894, "_Move" },
		{ 0xA895, "_ShutDown" },
		{ 0xA896, "_HidePen" },
		{ 0xA897, "_ShowPen" },
		{ 0xA898, "_GetPenState" },
		{ 0xA899, "_SetPenState" },
		{ 0xA89A, "_GetPen" },
		{ 0xA89B, "_PenSize" },
		{ 0xA89C, "_PenMode" },
		{ 0xA89D, "_PenPat" },
		{ 0xA89E, "_PenNormal" },
		{ 0xA89F, "_Moof" },
		{ 0xA8A0, "_StdRect" },
		{ 0xA8A1, "_FrameRect" },
		{ 0xA8A2, "_PaintRect" },
		{ 0xA8A3, "_EraseRect" },
		{ 0xA8A4, "_InverRect" },
		{ 0xA8A5, "_FillRect" },
		{ 0xA8A6, "_EqualRect" },
		{ 0xA8A7, "_SetRect" },
		{ 0xA8A8, "_OffsetRect" },
		{ 0xA8A9, "_InsetRect" },
		{ 0xA8AA, "_SectRect" },
		{ 0xA8AB, "_UnionRect" },
		{ 0xA8AD, "_PtInRect" },
		{ 0xA8AE, "_EmptyRect" },
		{ 0xA8AF, "_StdRRect" },
		{ 0xA8B0, "_FrameRoundRect" },
		{ 0xA8B1, "_PaintRoundRect" },
		{ 0xA8B2, "_EraseRoundRect" },
		{ 0xA8B3, "_InverRoundRect" },
		{ 0xA8B4, "_FillRoundRect" },
		{ 0xA8B5, "_ScriptUtil" },
		{ 0xA8B6, "_StdOval" },
		{ 0xA8B7, "_FrameOval" },
		{ 0xA8B8, "_PaintOval" },
		{ 0xA8B9, "_EraseOval" },
		{ 0xA8BA, "_InvertOval" },
		{ 0xA8BB, "_FillOval" },
		{ 0xA8BC, "_SlopeFromAngle" },
		{ 0xA8BD, "_StdArc" },
		{ 0xA8BE, "_FrameArc" },
		{ 0xA8BF, "_PaintArc" },
		{ 0xA8C0, "_EraseArc" },
		{ 0xA8C1, "_InvertArc" },
		{ 0xA8C2, "_FillArc" },
		{ 0xA8C3, "_PtToAngle" },
		{ 0xA8C4, "_AngleFromSlope" },
		{ 0xA8C5, "_StdPoly" },
		{ 0xA8C6, "_FramePoly" },
		{ 0xA8C7, "_PaintPoly" },
		{ 0xA8C8, "_ErasePoly" },
		{ 0xA8C9, "_InvertPoly" },
		{ 0xA8CA, "_FillPoly" },
		{ 0xA8CB, "_OpenPoly" },
		{ 0xA8CC, "_ClosePoly" },
		{ 0xA8CD, "_KillPoly" },
		{ 0xA8CE, "_OffsetPoly" },
		{ 0xA8CF, "_PackBits" },
		{ 0xA8D0, "_UnpackBits" },
		{ 0xA8D1, "_StdRgn" },
		{ 0xA8D2, "_FrameRgn" },
		{ 0xA8D3, "_PaintRgn" },
		{ 0xA8D4, "_EraseRgn" },
		{ 0xA8D5, "_InverRgn" },
		{ 0xA8D6, "_FillRgn" },
		{ 0xA8D7, "_BitMapToRegion" },
		{ 0xA8D8, "_NewRgn" },
		{ 0xA8D9, "_DisposeRgn" },
		{ 0xA8DA, "_OpenRgn" },
		{ 0xA8DB, "_CloseRgn" },
		{ 0xA8DC, "_CopyRgn" },
		{ 0xA8DD, "_SetEmptyRgn" },
		{ 0xA8DE, "_SetRecRgn" },
		{ 0xA8DF, "_RectRgn" },
		{ 0xA8E0, "_OffsetRgn" },
		{ 0xA8E1, "_InsetRgn" },
		{ 0xA8E2, "_EmptyRgn" },
		{ 0xA8E3, "_EqualRgn" },
		{ 0xA8E4, "_SectRgn" },
		{ 0xA8E5, "_UnionRgn" },
		{ 0xA8E6, "_DiffRgn" },
		{ 0xA8E7, "_XOrRgn" },
		{ 0xA8E8, "_PtInRgn" },
		{ 0xA8E9, "_RectInRgn" },
		{ 0xA8EA, "_SetStdProcs" },
		{ 0xA8EB, "_StdBits" },
		{ 0xA8EC, "_CopyBits" },
		{ 0xA8ED, "_StdTxMeas" },
		{ 0xA8EE, "_StdGetPic" },
		{ 0xA8EF, "_ScrollRect" },
		{ 0xA8F0, "_StdPutPic" },
		{ 0xA8F1, "_StdComment" },
		{ 0xA8F2, "_PicComment" },
		{ 0xA8F3, "_OpenPicture" },
		{ 0xA8F4, "_ClosePicture" },
		{ 0xA8F5, "_KillPicture" },
		{ 0xA8F6, "_DrawPicture" },
		{ 0xA8F7, "_Layout" },
		{ 0xA8F8, "_ScalePt" },
		{ 0xA8F9, "_MapPt" },
		{ 0xA8FA, "_MapRect" },
		{ 0xA8FB, "_MapRgn" },
		{ 0xA8FC, "_MapPoly" },
		{ 0xA8FD, "_PrGlue" },
		{ 0xA8FE, "_InitFonts" },
		{ 0xA8FF, "_GetFName" },
		{ 0xA900, "_GetFNum" },
		{ 0xA901, "_FMSwapFont" },
		{ 0xA902, "_RealFont" },
		{ 0xA903, "_SetFontLock" },
		{ 0xA904, "_DrawGrowIcon" },
		{ 0xA905, "_DragGrayRgn" },
		{ 0xA906, "_NewString" },
		{ 0xA907, "_SetString" },
		{ 0xA908, "_ShowHide" },
		{ 0xA909, "_CalcVis" },
		{ 0xA90A, "_CalcVBehind" },
		{ 0xA90B, "_ClipAbove" },
		{ 0xA90C, "_PaintOne" },
		{ 0xA90D, "_PaintBehind" },
		{ 0xA90E, "_SaveOld" },
		{ 0xA90F, "_DrawNew" },
		{ 0xA910, "_GetWMgrPort" },
		{ 0xA911, "_CheckUpDate" },
		{ 0xA912, "_InitWindows" },
		{ 0xA913, "_NewWindow" },
		{ 0xA914, "_DisposeWindow" },
		{ 0xA915, "_ShowWindow" },
		{ 0xA916, "_HideWindow" },
		{ 0xA917, "_GetWRefCon" },
		{ 0xA918, "_SetWRefCon" },
		{ 0xA919, "_GetWTitle" },
		{ 0xA91A, "_SetWTitle" },
		{ 0xA91B, "_MoveWindow" },
		{ 0xA91C, "_HiliteWindow" },
		{ 0xA91D, "_SizeWindow" },
		{ 0xA91E, "_TrackGoAway" },
		{ 0xA91F, "_SelectWindow" },
		{ 0xA920, "_BringToFront" },
		{ 0xA921, "_SendBehind" },
		{ 0xA922, "_BeginUpDate" },
		{ 0xA923, "_EndUpDate" },
		{ 0xA924, "_FrontWindow" },
		{ 0xA925, "_DragWindow" },
		{ 0xA926, "_DragTheRgn" },
		{ 0xA927, "_InvalRgn" },
		{ 0xA928, "_InvalRect" },
		{ 0xA929, "_ValidRgn" },
		{ 0xA92A, "_ValidRect" },
		{ 0xA92B, "_GrowWindow" },
		{ 0xA92C, "_FindWindow" },
		{ 0xA92D, "_CloseWindow" },
		{ 0xA92E, "_SetWindowPic" },
		{ 0xA92F, "_GetWindowPic" },
		{ 0xA930, "_InitMenus" },
		{ 0xA931, "_NewMenu" },
		{ 0xA932, "_DisposeMenu" },
		{ 0xA933, "_AppendMenu" },
		{ 0xA934, "_ClearMenuBar" },
		{ 0xA935, "_InsertMenu" },
		{ 0xA936, "_DeleteMenu" },
		{ 0xA937, "_DrawMenuBar" },
		{ 0xA938, "_HiliteMenu" },
		{ 0xA939, "_EnableItem" },
		{ 0xA93A, "_DisableItem" },
		{ 0xA93B, "_GetMenuBar" },
		{ 0xA93C, "_SetMenuBar" },
		{ 0xA93D, "_MenuSelect" },
		{ 0xA93E, "_MenuKey" },
		{ 0xA93F, "_GetItmIcon" },
		{ 0xA940, "_SetItmIcon" },
		{ 0xA941, "_GetItmStyle" },
		{ 0xA942, "_SetItmStyle" },
		{ 0xA943, "_GetItmMark" },
		{ 0xA944, "_SetItmMark" },
		{ 0xA945, "_CheckItem" },
		{ 0xA946, "_GetMenuItemText" },
		{ 0xA947, "_SetMenuItemText" },
		{ 0xA948, "_CalcMenuSize" },
		{ 0xA949, "_GetMenuHandle" },
		{ 0xA94A, "_SetMFlash" },
		{ 0xA94B, "_PlotIcon" },
		{ 0xA94C, "_FlashMenuBar" },
		{ 0xA94D, "_AppendResMenu" },
		{ 0xA94E, "_PinRect" },
		{ 0xA94F, "_DeltaPoint" },
		{ 0xA950, "_CountMItems" },
		{ 0xA951, "_InsertResMenu" },
		{ 0xA952, "_DeleteMenuItem" },
		{ 0xA953, "_UpdtControl" },
		{ 0xA954, "_NewControl" },
		{ 0xA955, "_DisposeControl" },
		{ 0xA956, "_KillControls" },
		{ 0xA957, "_ShowControl" },
		{ 0xA958, "_HideControl" },
		{ 0xA959, "_MoveControl" },
		{ 0xA95A, "_GetControlReference" },
		{ 0xA95B, "_SetControlReference" },
		{ 0xA95C, "_SizeControl" },
		{ 0xA95D, "_HiliteControl" },
		{ 0xA95E, "_GetControlTitle" },
		{ 0xA95F, "_SetControlTitle" },
		{ 0xA960, "_GetControlValue" },
		{ 0xA961, "_GetControlMinimum" },
		{ 0xA962, "_GetControlMaximum" },
		{ 0xA963, "_SetControlValue" },
		{ 0xA964, "_SetControlMinimum" },
		{ 0xA965, "_SetControlMaximum" },
		{ 0xA966, "_TestControl" },
		{ 0xA967, "_DragControl" },
		{ 0xA968, "_TrackControl" },
		{ 0xA969, "_DrawControls" },
		{ 0xA96A, "_GetControlAction" },
		{ 0xA96B, "_SetControlAction" },
		{ 0xA96C, "_FindControl" },
		{ 0xA96E, "_Dequeue" },
		{ 0xA96F, "_Enqueue" },
		{ 0xA970, "_GetNextEvent" },
		{ 0xA971, "_EventAvail" },
		{ 0xA972, "_GetMouse" },
		{ 0xA973, "_StillDown" },
		{ 0xA974, "_Button" },
		{ 0xA975, "_TickCount" },
		{ 0xA976, "_GetKeys" },
		{ 0xA977, "_WaitMouseUp" },
		{ 0xA978, "_UpdtDialog" },
		{ 0xA97B, "_InitDialogs" },
		{ 0xA97C, "_GetNewDialog" },
		{ 0xA97D, "_NewDialog" },
		{ 0xA97E, "_SelectDialogItemText" },
		{ 0xA97F, "_IsDialogEvent" },
		{ 0xA980, "_DialogSelect" },
		{ 0xA981, "_DrawDialog" },
		{ 0xA982, "_CloseDialog" },
		{ 0xA983, "_DisposeDialog" },
		{ 0xA984, "_FindDialogItem" },
		{ 0xA985, "_Alert" },
		{ 0xA986, "_StopAlert" },
		{ 0xA987, "_NoteAlert" },
		{ 0xA988, "_CautionAlert" },
		{ 0xA98B, "_ParamText" },
		{ 0xA98C, "_ErrorSound" },
		{ 0xA98D, "_GetDialogItem" },
		{ 0xA98E, "_SetDialogItem" },
		{ 0xA98F, "_SetDialogItemText" },
		{ 0xA990, "_GetDialogItemText" },
		{ 0xA991, "_ModalDialog" },
		{ 0xA992, "_DetachResource" },
		{ 0xA993, "_SetResPurge" },
		{ 0xA994, "_CurResFile" },
		{ 0xA995, "_InitResources" },
		{ 0xA996, "_RsrcZoneInit" },
		{ 0xA997, "_OpenResFile" },
		{ 0xA998, "_UseResFile" },
		{ 0xA999, "_UpdateResFile" },
		{ 0xA99A, "_CloseResFile" },
		{ 0xA99B, "_SetResLoad" },
		{ 0xA99C, "_CountResources" },
		{ 0xA99D, "_GetIndResource" },
		{ 0xA99E, "_CountTypes" },
		{ 0xA99F, "_GetIndType" },
		{ 0xA9A0, "_GetResource" },
		{ 0xA9A1, "_GetNamedResource" },
		{ 0xA9A2, "_LoadResource" },
		{ 0xA9A3, "_ReleaseResource" },
		{ 0xA9A4, "_HomeResFile" },
		{ 0xA9A5, "_SizeRsrc" },
		{ 0xA9A6, "_GetResAttrs" },
		{ 0xA9A7, "_SetResAttrs" },
		{ 0xA9A8, "_GetResInfo" },
		{ 0xA9A9, "_SetResInfo" },
		{ 0xA9AA, "_ChangedResource" },
		{ 0xA9AB, "_AddResource" },
		{ 0xA9AC, "_AddReference" },
		{ 0xA9AD, "_RmveResource" },
		{ 0xA9AE, "_RmveReference" },
		{ 0xA9AF, "_ResError" },
		{ 0xA9B0, "_WriteResource" },
		{ 0xA9B1, "_CreateResFile" },
		{ 0xA9B2, "_SystemEvent" },
		{ 0xA9B3, "_SystemClick" },
		{ 0xA9B4, "_SystemTask" },
		{ 0xA9B5, "_SystemMenu" },
		{ 0xA9B6, "_OpenDeskAcc" },
		{ 0xA9B7, "_CloseDeskAcc" },
		{ 0xA9B8, "_GetPattern" },
		{ 0xA9B9, "_GetCursor" },
		{ 0xA9BA, "_GetString" },
		{ 0xA9BB, "_GetIcon" },
		{ 0xA9BC, "_GetPicture" },
		{ 0xA9BD, "_GetNewWindow" },
		{ 0xA9BE, "_GetNewControl" },
		{ 0xA9BF, "_GetRMenu" },
		{ 0xA9C0, "_GetNewMBar" },
		{ 0xA9C1, "_UniqueID" },
		{ 0xA9C2, "_SysEdit" },
		{ 0xA9C3, "_KeyTranslate" },
		{ 0xA9C4, "_OpenRFPerm" },
		{ 0xA9C5, "_RsrcMapEntry" },
		{ 0xA9C6, "_SecondsToDate" },
		{ 0xA9C7, "_DateToSeconds" },
		{ 0xA9C8, "_SysBeep" },
		{ 0xA9C9, "_SysError" },
		{ 0xA9CA, "_PutIcon" },
		{ 0xA9CB, "_TEGetText" },
		{ 0xA9CC, "_TEInit" },
		{ 0xA9CD, "_TEDispose" },
		{ 0xA9CE, "_TETextBox" },
		{ 0xA9CF, "_TESetText" },
		{ 0xA9D0, "_TECalText" },
		{ 0xA9D1, "_TESetSelect" },
		{ 0xA9D2, "_TENew" },
		{ 0xA9D3, "_TEUpdate" },
		{ 0xA9D4, "_TEClick" },
		{ 0xA9D5, "_TECopy" },
		{ 0xA9D6, "_TECut" },
		{ 0xA9D7, "_TEDelete" },
		{ 0xA9D8, "_TEActivate" },
		{ 0xA9D9, "_TEDeactivate" },
		{ 0xA9DA, "_TEIdle" },
		{ 0xA9DB, "_TEPaste" },
		{ 0xA9DC, "_TEKey" },
		{ 0xA9DD, "_TEScroll" },
		{ 0xA9DE, "_TEInsert" },
		{ 0xA9DF, "_TESetAlignment" },
		{ 0xA9E0, "_Munger" },
		{ 0xA9E1, "_HandToHand" },
		{ 0xA9E2, "_PtrToXHand" },
		{ 0xA9E3, "_PtrToHand" },
		{ 0xA9E4, "_HandAndHand" },
		{ 0xA9E5, "_InitPack" },
		{ 0xA9E6, "_InitAllPacks" },
		{ 0xA9EF, "_PtrAndHand" },
		{ 0xA9F0, "_LoadSeg" },
		{ 0xA9F1, "_UnLoadSeg" },
		{ 0xA9F2, "_Launch" },
		{ 0xA9F3, "_Chain" },
		{ 0xA9F4, "_ExitToShell" },
		{ 0xA9F5, "_GetAppParms" },
		{ 0xA9F6, "_GetResFileAttrs" },
		{ 0xA9F7, "_SetResFileAttrs" },
		{ 0xA9F8, "_MethodDispatch" },
		{ 0xA9F9, "_InfoScrap" },
		{ 0xA9FA, "_UnloadScrap" },
		{ 0xA9FB, "_LoadScrap" },
		{ 0xA9FC, "_ZeroScrap" },
		{ 0xA9FD, "_GetScrap" },
		{ 0xA9FE, "_PutScrap" },
		{ 0xA9FF, "_Debugger" },
		{ 0xAA00, "_OpenCPort" },
		{ 0xAA01, "_InitCPort" },
		{ 0xAA02, "_CloseCPort" },
		{ 0xAA03, "_NewPixMap" },
		{ 0xAA04, "_DisposePixMap" },
		{ 0xAA05, "_CopyPixMap" },
		{ 0xAA06, "_SetPortPix" },
		{ 0xAA07, "_NewPixPat" },
		{ 0xAA08, "_DisposePixPat" },
		{ 0xAA09, "_CopyPixPat" },
		{ 0xAA0A, "_PenPixPat" },
		{ 0xAA0B, "_BackPixPat" },
		{ 0xAA0C, "_GetPixPat" },
		{ 0xAA0D, "_MakeRGBPat" },
		{ 0xAA0E, "_FillCRect" },
		{ 0xAA0F, "_FillCOval" },
		{ 0xAA10, "_FillCRoundRect" },
		{ 0xAA11, "_FillCArc" },
		{ 0xAA12, "_FillCRgn" },
		{ 0xAA13, "_FillCPoly" },
		{ 0xAA14, "_RGBForeColor" },
		{ 0xAA15, "_RGBBackColor" },
		{ 0xAA16, "_SetCPixel" },
		{ 0xAA17, "_GetCPixel" },
		{ 0xAA18, "_GetCTable" },
		{ 0xAA19, "_GetForeColor" },
		{ 0xAA1A, "_GetBackColor" },
		{ 0xAA1B, "_GetCCursor" },
		{ 0xAA1C, "_SetCCursor" },
		{ 0xAA1D, "_AllocCursor" },
		{ 0xAA1E, "_GetCIcon" },
		{ 0xAA1F, "_PlotCIcon" },
		{ 0xAA20, "_OpenCPicture" },
		{ 0xAA21, "_OpColor" },
		{ 0xAA22, "_HiliteColor" },
		{ 0xAA23, "_CharExtra" },
		{ 0xAA24, "_DisposeCTable" },
		{ 0xAA25, "_DisposeCIcon" },
		{ 0xAA26, "_DisposeCCursor" },
		{ 0xAA27, "_GetMaxDevice" },
		{ 0xAA28, "_GetCTSeed" },
		{ 0xAA29, "_GetDeviceList" },
		{ 0xAA2A, "_GetMainDevice" },
		{ 0xAA2B, "_GetNextDevice" },
		{ 0xAA2C, "_TestDeviceAttribute" },
		{ 0xAA2D, "_SetDeviceAttribute" },
		{ 0xAA2E, "_InitGDevice" },
		{ 0xAA2F, "_NewGDevice" },
		{ 0xAA30, "_DisposeGDevice" },
		{ 0xAA31, "_SetGDevice" },
		{ 0xAA32, "_GetGDevice" },
		{ 0xAA35, "_InvertColor" },
		{ 0xAA36, "_RealColor" },
		{ 0xAA37, "_GetSubTable" },
		{ 0xAA38, "_UpdatePixMap" },
		{ 0xAA39, "_MakeITable" },
		{ 0xAA3A, "_AddSearch" },
		{ 0xAA3B, "_AddComp" },
		{ 0xAA3C, "_SetClientID" },
		{ 0xAA3D, "_ProtectEntry" },
		{ 0xAA3E, "_ReserveEntry" },
		{ 0xAA3F, "_SetEntries" },
		{ 0xAA40, "_QDError" },
		{ 0xAA41, "_SetWinColor" },
		{ 0xAA42, "_GetAuxWin" },
		{ 0xAA43, "_SetControlColor" },
		{ 0xAA44, "_GetAuxiliaryControlRecord" },
		{ 0xAA45, "_NewCWindow" },
		{ 0xAA46, "_GetNewCWindow" },
		{ 0xAA47, "_SetDeskCPat" },
		{ 0xAA48, "_GetCWMgrPort" },
		{ 0xAA49, "_SaveEntries" },
		{ 0xAA4A, "_RestoreEntries" },
		{ 0xAA4B, "_NewColorDialog" },
		{ 0xAA4C, "_DelSearch" },
		{ 0xAA4D, "_DelComp" },
		{ 0xAA4E, "_SetStdCProcs" },
		{ 0xAA4F, "_CalcCMask" },
		{ 0xAA50, "_SeedCFill" },
		{ 0xAA51, "_CopyDeepMask" },
		{ 0xAA52, "_HFSPinaforeDispatch" },
		{ 0xAA53, "_DictionaryDispatch" },
		{ 0xAA54, "_TextServicesDispatch" },
		{ 0xAA56, "_SpeechRecognitionDispatch" },
		{ 0xAA57, "_DockingDispatch" },
		{ 0xAA59, "_MixedModeDispatch" },
		{ 0xAA5A, "_CodeFragmentDispatch" },
		{ 0xAA5C, "_OCEUtils" },
		{ 0xAA5D, "_DigitalSignature" },
		{ 0xAA5E, "_TBDispatch" },
		{ 0xAA60, "_DeleteMCEntries" },
		{ 0xAA61, "_GetMCInfo" },
		{ 0xAA62, "_SetMCInfo" },
		{ 0xAA63, "_DisposeMCInfo" },
		{ 0xAA64, "_GetMCEntry" },
		{ 0xAA65, "_SetMCEntries" },
		{ 0xAA66, "_MenuChoice" },
		{ 0xAA67, "_ModalDialogMenuSetup" },
		{ 0xAA68, "_DialogDispatch" },
		{ 0xAA73, "_ControlDispatch" },
		{ 0xAA74, "_AppearanceDispatch" },
		{ 0xAA7E, "_SysDebugDispatch" },
		{ 0xAA80, "_AVLTreeDispatch" },
		{ 0xAA81, "_FileMappingDispatch" },
		{ 0xAA90, "_InitPalettes" },
		{ 0xAA91, "_NewPalette" },
		{ 0xAA92, "_GetNewPalette" },
		{ 0xAA93, "_DisposePalette" },
		{ 0xAA94, "_ActivatePalette" },
		{ 0xAA95, "_NSetPalette" },
		{ 0xAA96, "_GetPalette" },
		{ 0xAA97, "_PmForeColor" },
		{ 0xAA98, "_PmBackColor" },
		{ 0xAA99, "_AnimateEntry" },
		{ 0xAA9A, "_AnimatePalette" },
		{ 0xAA9B, "_GetEntryColor" },
		{ 0xAA9C, "_SetEntryColor" },
		{ 0xAA9D, "_GetEntryUsage" },
		{ 0xAA9E, "_SetEntryUsage" },
		{ 0xAAA1, "_CopyPalette" },
		{ 0xAAA2, "_PaletteDispatch" },
		{ 0xAAA3, "_CodecDispatch" },
		{ 0xAAA4, "_ALMDispatch" },
		{ 0xAADB, "_CursorDeviceDispatch" },
		{ 0xAAF2, "_ControlStripDispatch" },
		{ 0xAAF3, "_ExpansionManager" },
		{ 0xAB1D, "_QDExtensions" },
		{ 0xABC3, "_NQDMisc" },
		{ 0xABC9, "_IconDispatch" },
		{ 0xABCA, "_DeviceLoop" },
		{ 0xABEB, "_DisplayDispatch" },
		{ 0xABED, "_DragDispatch" },
		{ 0xABF1, "_GestaltValueDispatch" },
		{ 0xABF2, "_ThreadDispatch" },
		{ 0xABF6, "_CollectionMgr" },
		{ 0xABF8, "_StdOpcodeProc" },
		{ 0xABFC, "_TranslationDispatch" },
		{ 0xABFF, "_DebugStr" }
	};

	int i;

	for (i = 0; i < (sizeof(traps) / sizeof(traps[0])); i++)
	{
		if (traps[i].trap == opcode)
			return traps[i].name;
	}
	return NULL;
}



static offs_t mac_dasm_override(char *buffer, offs_t pc, const UINT8 *oprom, const UINT8 *opram)
{
	UINT16 opcode;
	unsigned result = 0;
	const char *trap;

	opcode = *((UINT16 *) oprom);
	if ((opcode & 0xF000) == 0xA000)
	{
		trap = lookup_trap(opcode);
		if (trap)
		{
			strcpy(buffer, trap);
			result = 1;
		}
	}
	return result;
}



#ifdef MAC_TRACETRAP
static void mac_tracetrap(const char *cpu_name_local, int addr, int trap)
{
	typedef struct
	{
		int csCode;
		const char *name;
	} sonycscodeentry;

	static const sonycscodeentry cscodes[] =
	{
		{ 1, "KillIO" },
		{ 5, "VerifyDisk" },
		{ 6, "FormatDisk" },
		{ 7, "EjectDisk" },
		{ 8, "SetTagBuffer" },
		{ 9, "TrackCacheControl" },
		{ 23, "ReturnDriveInfo" }
	};

	static const char *scsisels[] =
	{
		"SCSIReset",	/* $00 */
		"SCSIGet",		/* $01 */
		"SCSISelect",	/* $02 */
		"SCSICmd",		/* $03 */
		"SCSIComplete",	/* $04 */
		"SCSIRead",		/* $05 */
		"SCSIWrite",	/* $06 */
		NULL,			/* $07 */
		"SCSIRBlind",	/* $08 */
		"SCSIWBlind",	/* $09 */
		"SCSIStat",		/* $0A */
		"SCSISelAtn",	/* $0B */
		"SCSIMsgIn",	/* $0C */
		"SCSIMsgOut",	/* $0D */
	};

	int i, a0, a7, d0, d1;
	int csCode, ioVRefNum, ioRefNum, ioCRefNum, ioCompletion, ioBuffer, ioReqCount, ioPosOffset;
	char *s;
	unsigned char *mem;
	char buf[256];
	const char *trapstr;

	trapstr = lookup_trap(trap);
	if (trapstr)
		strcpy(buf, trapstr);
	else
		sprintf(buf, "Trap $%04x", trap);

	s = &buf[strlen(buf)];
	mem = mac_ram_ptr;
	a0 = cpu_get_reg(M68K_A0);
	a7 = cpu_get_reg(M68K_A7);
	d0 = cpu_get_reg(M68K_D0);
	d1 = cpu_get_reg(M68K_D1);

	switch(trap)
	{
	case 0xa004:	/* _Control */
		ioVRefNum = *((INT16*) (mem + a0 + 22));
		ioCRefNum = *((INT16*) (mem + a0 + 24));
		csCode = *((UINT16*) (mem + a0 + 26));
		sprintf(s, " ioVRefNum=%i ioCRefNum=%i csCode=%i", ioVRefNum, ioCRefNum, csCode);

		for (i = 0; i < (sizeof(cscodes) / sizeof(cscodes[0])); i++)
		{
			if (cscodes[i].csCode == csCode)
			{
				strcat(s, "=");
				strcat(s, cscodes[i].name);
				break;
			}
		}
		break;

	case 0xa002:	/* _Read */
		ioCompletion = (*((INT16*) (mem + a0 + 12)) << 16) + *((INT16*) (mem + a0 + 14));
		ioVRefNum = *((INT16*) (mem + a0 + 22));
		ioRefNum = *((INT16*) (mem + a0 + 24));
		ioBuffer = (*((INT16*) (mem + a0 + 32)) << 16) + *((INT16*) (mem + a0 + 34));
		ioReqCount = (*((INT16*) (mem + a0 + 36)) << 16) + *((INT16*) (mem + a0 + 38));
		ioPosOffset = (*((INT16*) (mem + a0 + 46)) << 16) + *((INT16*) (mem + a0 + 48));
		sprintf(s, " ioCompletion=0x%08x ioVRefNum=%i ioRefNum=%i ioBuffer=0x%08x ioReqCount=%i ioPosOffset=%i",
			ioCompletion, ioVRefNum, ioRefNum, ioBuffer, ioReqCount, ioPosOffset);
		break;

	case 0xa04e:	/* _AddDrive */
		sprintf(s, " drvrRefNum=%i drvNum=%i qEl=0x%08x", (int) (INT16) d0, (int) (INT16) d1, a0);
		break;

	case 0xa9a0:	/* _GetResource */
		/* HACKHACK - the 'type' output assumes that the host is little endian
		 * since this is just trace code it isn't much of an issue
		 */
		sprintf(s, " type='%c%c%c%c' id=%i", (char) mem[a7+3], (char) mem[a7+2],
			(char) mem[a7+5], (char) mem[a7+4], *((INT16*) (mem + a7)));
		break;

	case 0xa815:	/* _SCSIDispatch */
		i = *((UINT16*) (mem + a7));
		if (i < (sizeof(scsisels) / sizeof(scsisels[0])))
			if (scsisels[i])
				sprintf(s, " (%s)", scsisels[i]);
		break;
	}

	logerror("mac_trace_trap: %s at 0x%08x: %s\n",cpu_name_local, addr, buf);
}
#endif




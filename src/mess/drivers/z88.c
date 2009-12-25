/******************************************************************************

        z88.c

        z88 Notepad computer

        system driver

        TODO:
        - speaker controlled by constant tone or txd

        Kevin Thacker [MESS driver]

 ******************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "includes/z88.h"
#include "sound/speaker.h"
#include "devices/messram.h"
#include "devices/messram.h"

struct blink_hw z88_blink;

static void blink_reset(void)
{
	memset(&z88_blink, 0, sizeof(struct blink_hw));

	/* rams is cleared on reset */
	z88_blink.com &=~(1<<2);
	z88_blink.sbf = 0;
	z88_blink.z88_state = Z88_AWAKE;

}


static void z88_interrupt_refresh(running_machine *machine)
{
	/* ints enabled? */
	if ((z88_blink.ints & INT_GINT)!=0)
	{
		/* yes */

		/* other ints - except timer */
		if (
			(((z88_blink.ints & z88_blink.sta) & 0x0fc)!=0) ||
			(((z88_blink.ints>>1) & z88_blink.sta & STA_TIME)!=0)
			)
		{
			logerror("set int\n");
			cputag_set_input_line(machine, "maincpu", 0, HOLD_LINE);
			return;
		}
	}

	logerror("clear int\n");
	cputag_set_input_line(machine, "maincpu", 0, CLEAR_LINE);
}

static void z88_update_rtc_interrupt(void)
{
	z88_blink.sta &=~STA_TIME;

	/* time interrupt enabled? */
	if (z88_blink.ints & INT_TIME)
	{
		/* yes */

		/* any ints occured? */
		if ((z88_blink.tsta & 0x07)!=0)
		{
			/* yes, set time int */
			z88_blink.sta |= STA_TIME;
		}
	}
}



static TIMER_CALLBACK(z88_rtc_timer_callback)
{
	int refresh_ints = 0;
	static const char *const keynames[] = { "LINE0", "LINE1", "LINE2", "LINE3", "LINE4", "LINE5", "LINE6", "LINE7" };

	/* is z88 in snooze state? */
	if (z88_blink.z88_state == Z88_SNOOZE)
	{
		int i;
		unsigned char data = 0x0ff;

		/* any key pressed will wake up z88 */
		for (i=0; i<8; i++)
		{
			data &= input_port_read(machine, keynames[i]);
		}

		/* if any key is pressed, then one or more bits will be 0 */
		if (data!=0x0ff)
		{
			logerror("Z88 wake up from snooze!\n");

			/* wake up z88 */
			z88_blink.z88_state = Z88_AWAKE;
			/* column has gone low in snooze/coma */
			z88_blink.sta |= STA_KEY;

			cpuexec_trigger(machine, Z88_SNOOZE_TRIGGER);

			z88_interrupt_refresh(machine);
		}
	}



	/* hold clock at reset? - in this mode it doesn't update */
	if ((z88_blink.com & (1<<4))==0)
	{
		/* update 5 millisecond counter */
		z88_blink.tim[0]++;

		/* tick */
		if ((z88_blink.tim[0]%10)==0)
		{
			/* set tick int has occured */
			z88_blink.tsta |= RTC_TICK_INT;
			refresh_ints = 1;
		}

		if (z88_blink.tim[0]==200)
		{
			z88_blink.tim[0] = 0;

			/* set seconds int has occured */
			z88_blink.tsta |= RTC_SEC_INT;
			refresh_ints = 1;

			z88_blink.tim[1]++;

			if (z88_blink.tim[1]==60)
			{
				/* set minutes int has occured */
				z88_blink.tsta |=RTC_MIN_INT;
				refresh_ints = 1;

				z88_blink.tim[1]=0;

				z88_blink.tim[2]++;

				if (z88_blink.tim[2]==256)
				{
					z88_blink.tim[2] = 0;

					z88_blink.tim[3]++;

					if (z88_blink.tim[3]==256)
					{
						z88_blink.tim[3] = 0;

						z88_blink.tim[4]++;

						if (z88_blink.tim[4]==32)
						{
							z88_blink.tim[4] = 0;
						}
					}
				}
			}
		}
	}

	if (refresh_ints)
	{
		z88_update_rtc_interrupt();

		/* refresh */
		z88_interrupt_refresh(machine);
	}
}



static void z88_install_memory_handler_pair(running_machine *machine, offs_t start, offs_t size, int bank_base, void *read_addr, void *write_addr)
{
	char bank_0[10];
	char bank_1[10];

	sprintf(bank_0,"bank%d",bank_base + 0);
	sprintf(bank_1,"bank%d",bank_base + 1);

	/* special case */
	if (read_addr == NULL)
		read_addr = &memory_region(machine, "maincpu")[start];

	/* install the handlers */
	if (read_addr == NULL) {
		memory_unmap_read(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM ), start, start + size - 1, 0, 0);
	} else {
		memory_install_read_bank(cputag_get_address_space(machine, "maincpu",  ADDRESS_SPACE_PROGRAM ), start, start + size - 1, 0, 0, bank_0);
	}
	if (write_addr == NULL) {
		memory_unmap_write(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM ), start, start + size - 1, 0, 0);
	} else {
		memory_install_write_bank(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM ), start, start + size - 1, 0, 0, bank_1);
	}

	/* and set the banks */
	if (read_addr != NULL)
		memory_set_bankptr(machine, bank_0, read_addr);
	if (write_addr != NULL)
		memory_set_bankptr(machine, bank_1, write_addr);
}


/* Assumption:

all banks can access the same memory blocks in the same way.
bank 0 is special. If a bit is set in the com register,
the lower 8k is replaced with the rom. Bank 0 has been split
into 2 8k chunks, and all other banks into 16k chunks.
I wanted to handle all banks in the code below, and this
explains why the extra checks are done


    bank 0      0x0000-0x3FFF
    bank 1      0x4000-0x7FFF
    bank 2      0x8000-0xBFFF
    bank 3      0xC000-0xFFFF
*/

static void z88_refresh_memory_bank(running_machine *machine, int bank)
{
	void *read_addr;
	void *write_addr;
	unsigned long block;

	assert(bank >= 0);
	assert(bank <= 3);

	/* ram? */
	if (z88_blink.mem[bank]>=0x020)
	{
		block = z88_blink.mem[bank]-0x020;

		if (block >= 128)
		{
			read_addr = NULL;
			write_addr = NULL;
		}
		else
		{
			read_addr = write_addr = messram_get_ptr(devtag_get_device(machine, "messram")) + (block<<14);
		}
	}
	else
	{
		block = z88_blink.mem[bank] & 0x07;

		/* in rom area, but rom not present */
		if (block>=8)
		{
			read_addr = NULL;
			write_addr = NULL;
		}
		else
		{
			read_addr = memory_region(machine, "maincpu") + 0x010000 + (block << 14);
			write_addr = NULL;
		}
	}

	/* install the banks */
	z88_install_memory_handler_pair(machine, bank * 0x4000, 0x4000, bank * 2 + 1, read_addr, write_addr);

	if (bank == 0)
	{
		/* override setting for lower 8k of bank 0 */

		/* enable rom? */
		if ((z88_blink.com & (1<<2))==0)
		{
			/* yes */
			read_addr = memory_region(machine, "maincpu") + 0x010000;
			write_addr = NULL;
		}
		else
		{
			/* ram bank 20 */
			read_addr = messram_get_ptr(devtag_get_device(machine, "messram"));
			write_addr = messram_get_ptr(devtag_get_device(machine, "messram"));
		}

		z88_install_memory_handler_pair(machine, 0x0000, 0x2000, 9, read_addr, write_addr);
	}
}

static MACHINE_START( z88 )
{
	timer_pulse(machine, ATTOTIME_IN_MSEC(5), NULL, 0, z88_rtc_timer_callback);
}

static MACHINE_RESET( z88 )
{
	memset(messram_get_ptr(devtag_get_device(machine, "messram")), 0x0ff, messram_get_size(devtag_get_device(machine, "messram")));

	blink_reset();

	z88_refresh_memory_bank(machine, 0);
	z88_refresh_memory_bank(machine, 1);
	z88_refresh_memory_bank(machine, 2);
	z88_refresh_memory_bank(machine, 3);
}

static ADDRESS_MAP_START(z88_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_READ_BANK("bank1") AM_WRITE_BANK("bank6")
	AM_RANGE(0x2000, 0x3fff) AM_READ_BANK("bank2") AM_WRITE_BANK("bank7")
	AM_RANGE(0x4000, 0x7fff) AM_READ_BANK("bank3") AM_WRITE_BANK("bank8")
	AM_RANGE(0x8000, 0xbfff) AM_READ_BANK("bank4") AM_WRITE_BANK("bank9")
	AM_RANGE(0xc000, 0xffff) AM_READ_BANK("bank5") AM_WRITE_BANK("bank10")
ADDRESS_MAP_END

static void blink_pb_w(int offset, int data, int reg_index)
{
    unsigned short addr_written = (offset & 0x0ff00) | (data & 0x0ff);

	logerror("reg_index: %02x addr: %04x\n",reg_index,addr_written);

    switch (reg_index)
	{

		/* 1c000 */

	case 0x00:
		{
/**/            z88_blink.pb[0] = addr_written;
            z88_blink.lores0 = ((addr_written & 0x01f)<<9) | ((addr_written & 0x01fe0)<<9);	// blink_pb_offset(-1, addr_written, 9);
            logerror("lores0 %08x\n",z88_blink.lores0);
		}
		break;

		case 0x01:
		{
            z88_blink.pb[1] = addr_written;
            z88_blink.lores1 = ((addr_written & 0x01f)<<12) | ((addr_written & 0x01fe0)<<12);	//blink_pb_offset(-1, addr_written, 12);
            logerror("lores1 %08x\n",z88_blink.lores1);
		}
		break;

		case 0x02:
		{
            z88_blink.pb[2] = addr_written;
/**/            z88_blink.hires0 = ((addr_written & 0x01f)<<13) | ((addr_written & 0x01fe0)<<13);	//blink_pb_offset(-1, addr_written, 13);
            logerror("hires0 %08x\n", z88_blink.hires0);
		}
		break;


		case 0x03:
		{
            z88_blink.pb[3] = addr_written;
            z88_blink.hires1 = ((addr_written & 0x01f)<<11) | ((addr_written & 0x01fe0)<<11);	//blink_pb_offset(-1, addr_written, 11);

            logerror("hires1 %08x\n", z88_blink.hires1);
		}
		break;

		case 0x04:
		{
            z88_blink.sbr = addr_written;

			z88_blink.sbf = ((addr_written & 0x01f)<<11) | ((addr_written & 0x01fe0)<<11);
            logerror("%08x\n", z88_blink.sbf);

		}
		break;

		default:
			break;
	}
}


/* segment register write */
static WRITE8_HANDLER(blink_srx_w)
{
	z88_blink.mem[offset] = data;

	z88_refresh_memory_bank(space->machine, offset);
}
/*
 00b0 00
blink w: 00d0 00
blink w: 00d1 00
blink w: 00d2 00
blink w: 00d3 00
blink w: 01b5 01
blink w: 01b4 01
blink w: 03b1 03
blink w: 03b6 03
*/

static WRITE8_HANDLER(z88_port_w)
{
	const device_config *speaker = devtag_get_device(space->machine, "speaker");
	unsigned char port;

	port = offset & 0x0ff;

	switch (port)
	{
		case 0x070:
		case 0x071:
		case 0x072:
		case 0x073:
		case 0x074:
			blink_pb_w(offset, data, port & 0x0f);
			return;


		/* write rtc interrupt acknowledge */
		case 0x0b4:
		{
		    logerror("tack w: %02x\n", data);

			/* set acknowledge */
			z88_blink.tack = data & 0x07;
			/* clear ints that have occured */
			z88_blink.tsta &= ~z88_blink.tack;

			/* refresh ints */
			z88_update_rtc_interrupt();
			z88_interrupt_refresh(space->machine);
		}
		return;

		/* write rtc interrupt mask */
		case 0x0b5:
		{
		    logerror("tmk w: %02x\n", data);

			/* set new int mask */
			z88_blink.tmk = data & 0x07;

			/* refresh ints */
			z88_update_rtc_interrupt();
			z88_interrupt_refresh(space->machine);
		}
		return;

		case 0x0b0:
		{
			UINT8 changed_bits;

		    logerror("com w: %02x\n", data);

			changed_bits = z88_blink.com^data;
			z88_blink.com = data;

			/* reset clock? */
			if ((data & (1<<4))!=0)
			{
				z88_blink.tim[0] = (z88_blink.tim[1] = (z88_blink.tim[2] = (z88_blink.tim[3] = (z88_blink.tim[4] = 0))));
			}

			/* SBIT controls speaker direct? */
			if ((data & (1<<7))==0)
			{
			   /* yes */

			   /* speaker controlled by SBIT */
			   if ((changed_bits & (1<<6))!=0)
			   {
				   speaker_level_w(speaker, (data>>6) & 0x01);
			   }
			}
			else
			{
			   /* speaker under control of continuous tone,
               or txd */


			}

			if ((changed_bits & (1<<2))!=0)
			{
				z88_refresh_memory_bank(space->machine, 0);
			}
		}
		return;

		case 0x0b1:
		{
			/* set int enables */
		    logerror("int w: %02x\n", data);

			z88_blink.ints = data;
			z88_update_rtc_interrupt();
			z88_interrupt_refresh(space->machine);
		}
		return;


		case 0x0b6:
		{
		    logerror("ack w: %02x\n", data);

			/* acknowledge ints */
			z88_blink.ack = data & ((1<<6) | (1<<5) | (1<<3) | (1<<2));

			z88_blink.ints &= ~z88_blink.ack;
			z88_update_rtc_interrupt();
			z88_interrupt_refresh(space->machine);
		}
		return;



		case 0x0d0:
		case 0x0d1:
		case 0x0d2:
		case 0x0d3:
			blink_srx_w(space, port & 0x03, data);
			return;
	}


    logerror("blink w: %04x %02x\n", offset, data);

}

static  READ8_HANDLER(z88_port_r)
{
	unsigned char port;

	port = offset & 0x0ff;

	switch (port)
	{
		case 0x0b1:
			z88_blink.sta &=~(1<<1);
			logerror("sta r: %02x\n",z88_blink.sta);
			return z88_blink.sta;


		case 0x0b2:
		{
			unsigned char data = 0x0ff;

			int lines;

			lines = offset>>8;

			/* if set, reading the keyboard will put z88 into snooze */
			if ((z88_blink.ints & INT_KWAIT)!=0)
			{
				z88_blink.z88_state = Z88_SNOOZE;
				/* spin cycles until rtc timer */
				cpu_spinuntil_trigger( cputag_get_cpu(space->machine, "maincpu"), Z88_SNOOZE_TRIGGER);

				logerror("z88 entering snooze!\n");
			}


			if ((lines & 0x080)==0)
				data &=input_port_read(space->machine, "LINE7");

			if ((lines & 0x040)==0)
				data &=input_port_read(space->machine, "LINE6");

			if ((lines & 0x020)==0)
				data &=input_port_read(space->machine, "LINE5");

			if ((lines & 0x010)==0)
				data &=input_port_read(space->machine, "LINE4");

			if ((lines & 0x008)==0)
				data &=input_port_read(space->machine, "LINE3");

			if ((lines & 0x004)==0)
				data &=input_port_read(space->machine, "LINE2");

			if ((lines & 0x002)==0)
				data &=input_port_read(space->machine, "LINE1");

			if ((lines & 0x001)==0)
				data &=input_port_read(space->machine, "LINE0");

			logerror("lines: %02x\n",lines);
			logerror("key r: %02x\n",data);
			return data;
		}

		/* read real time clock status */
		case 0x0b5:
			z88_blink.tsta &=~0x07;
			logerror("tsta r: %02x\n",z88_blink.tsta);
			return z88_blink.tsta;

		/* read real time clock counters */
		case 0x0d0:
		    logerror("tim0 r: %02x\n", z88_blink.tim[0]);
			return z88_blink.tim[0] & 0x0ff;
		case 0x0d1:
			logerror("tim1 r: %02x\n", z88_blink.tim[1]);
			return z88_blink.tim[1] & 0x03f;
		case 0x0d2:
			logerror("tim2 r: %02x\n", z88_blink.tim[2]);
			return z88_blink.tim[2] & 0x0ff;
		case 0x0d3:
			logerror("tim3 r: %02x\n", z88_blink.tim[3]);
			return z88_blink.tim[3] & 0x0ff;
		case 0x0d4:
			logerror("tim4 r: %02x\n", z88_blink.tim[4]);
			return z88_blink.tim[4] & 0x01f;

		default:
			break;

	}

	logerror("blink r: %04x \n", offset);


	return 0x0ff;
}


static ADDRESS_MAP_START( z88_io, ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_GLOBAL_MASK(0xFFFF)
	AM_RANGE(0x0000, 0x0ffff) AM_READWRITE(z88_port_r, z88_port_w)
ADDRESS_MAP_END



/*
-------------------------------------------------------------------------
         | D7     D6      D5      D4      D3      D2      D1      D0
-------------------------------------------------------------------------
A15 (#7) | RSH    SQR     ESC     INDEX   CAPS    .       /       ??
A14 (#6) | HELP   LSH     TAB     DIA     MENU    ,       ;       '
A13 (#5) | [      SPACE   1       Q       A       Z       L       0
A12 (#4) | ]      LFT     2       W       S       X       M       P
A11 (#3) | -      RGT     3       E       D       C       K       9
A10 (#2) | =      DWN     4       R       F       V       J       O
A9  (#1) | \      UP      5       T       G       B       U       I
A8  (#0) | DEL    ENTER   6       Y       H       N       7       8
-------------------------------------------------------------------------

2008-05 FP:
Small note about natural keyboard: currently,
- "Square" is mapped to 'Left Control'
- "Diamond" is mapped to 'F1'
- "Index" is mapped to 'F2'
- "Menu" is mapped to 'F3'
- "Help" is mapped to 'F4'
*/

static INPUT_PORTS_START( z88 )
	PORT_START("LINE0")
	PORT_BIT(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Del") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8)
	PORT_BIT(0x040, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_ENTER)		PORT_CHAR(13)
	PORT_BIT(0x020, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_6) 			PORT_CHAR('6') PORT_CHAR('^')
	PORT_BIT(0x010, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Y) 			PORT_CHAR('y') PORT_CHAR('Y')
	PORT_BIT(0x008, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_H) 			PORT_CHAR('h') PORT_CHAR('H')
	PORT_BIT(0x004, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_N) 			PORT_CHAR('n') PORT_CHAR('N')
	PORT_BIT(0x002, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_7) 			PORT_CHAR('7') PORT_CHAR('&')
	PORT_BIT(0x001, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_8) 			PORT_CHAR('8') PORT_CHAR('*')

	PORT_START("LINE1")
	PORT_BIT(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_BACKSLASH2)	PORT_CHAR('\\') PORT_CHAR('|')
	PORT_BIT(0x040, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_UP)			PORT_CHAR(UCHAR_MAMEKEY(UP))
	PORT_BIT(0x020, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_5)			PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT(0x010, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_T) 			PORT_CHAR('t') PORT_CHAR('T')
	PORT_BIT(0x008, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_G) 			PORT_CHAR('g') PORT_CHAR('G')
	PORT_BIT(0x004, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_B) 			PORT_CHAR('b') PORT_CHAR('B')
	PORT_BIT(0x002, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_U) 			PORT_CHAR('u') PORT_CHAR('U')
	PORT_BIT(0x001, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_I) 			PORT_CHAR('i') PORT_CHAR('I')

	PORT_START("LINE2")
	PORT_BIT(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_EQUALS)		PORT_CHAR('=') PORT_CHAR('+')
	PORT_BIT(0x040, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_DOWN)		PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	PORT_BIT(0x020, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_4) 			PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT(0x010, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_R) 			PORT_CHAR('r') PORT_CHAR('R')
	PORT_BIT(0x008, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F) 			PORT_CHAR('f') PORT_CHAR('F')
	PORT_BIT(0x004, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_V) 			PORT_CHAR('v') PORT_CHAR('V')
	PORT_BIT(0x002, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_J) 			PORT_CHAR('j') PORT_CHAR('J')
	PORT_BIT(0x001, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_O) 			PORT_CHAR('o') PORT_CHAR('O')

	PORT_START("LINE3")
	PORT_BIT(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_MINUS)		PORT_CHAR('-') PORT_CHAR('_')
	PORT_BIT(0x040, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_RIGHT) 		PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_BIT(0x020, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_3) 			PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT(0x010, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_E) 			PORT_CHAR('e') PORT_CHAR('E')
	PORT_BIT(0x008, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_D) 			PORT_CHAR('d') PORT_CHAR('D')
	PORT_BIT(0x004, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_C) 			PORT_CHAR('c') PORT_CHAR('C')
	PORT_BIT(0x002, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_K) 			PORT_CHAR('k') PORT_CHAR('K')
	PORT_BIT(0x001, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_9) 			PORT_CHAR('9') PORT_CHAR('(')

	PORT_START("LINE4")
	PORT_BIT(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_CLOSEBRACE)	PORT_CHAR(']') PORT_CHAR('}')
	PORT_BIT(0x040, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_LEFT)		PORT_CHAR(UCHAR_MAMEKEY(LEFT))
	PORT_BIT(0x020, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_2) 			PORT_CHAR('2') PORT_CHAR('@')
	PORT_BIT(0x010, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_W) 			PORT_CHAR('w') PORT_CHAR('W')
	PORT_BIT(0x008, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_S) 			PORT_CHAR('s') PORT_CHAR('S')
	PORT_BIT(0x004, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_X) 			PORT_CHAR('x') PORT_CHAR('X')
	PORT_BIT(0x002, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_M) 			PORT_CHAR('m') PORT_CHAR('M')
	PORT_BIT(0x001, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_P) 			PORT_CHAR('p') PORT_CHAR('P')

	PORT_START("LINE5")
	PORT_BIT(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_OPENBRACE)	PORT_CHAR('[') PORT_CHAR('{')
	PORT_BIT(0x040, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_SPACE)		PORT_CHAR(' ')
	PORT_BIT(0x020, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_1) 			PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT(0x010, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Q) 			PORT_CHAR('q') PORT_CHAR('Q')
	PORT_BIT(0x008, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_A) 			PORT_CHAR('a') PORT_CHAR('A')
	PORT_BIT(0x004, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Z) 			PORT_CHAR('z') PORT_CHAR('Z')
	PORT_BIT(0x002, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_L) 			PORT_CHAR('l') PORT_CHAR('L')
	PORT_BIT(0x001, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_0) 			PORT_CHAR('0') PORT_CHAR(')')

	PORT_START("LINE6")
	PORT_BIT(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Help") PORT_CODE(KEYCODE_F4)				PORT_CHAR(UCHAR_MAMEKEY(F4))
	PORT_BIT(0x040, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Shift (Left)") PORT_CODE(KEYCODE_LSHIFT)	PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT(0x020, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_TAB)									PORT_CHAR('\t')
	PORT_BIT(0x010, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Diamond") PORT_CODE(KEYCODE_F1)				PORT_CHAR(UCHAR_MAMEKEY(F1))
	PORT_BIT(0x008, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Menu") PORT_CODE(KEYCODE_F3)				PORT_CHAR(UCHAR_MAMEKEY(F3))
	PORT_BIT(0x004, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_COMMA)								PORT_CHAR(',') PORT_CHAR('<')
	PORT_BIT(0x002, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_COLON)								PORT_CHAR(';') PORT_CHAR(':')
	PORT_BIT(0x001, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_QUOTE)								PORT_CHAR('\'') PORT_CHAR('"')

	PORT_START("LINE7")
	PORT_BIT(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Shift (Right)") PORT_CODE(KEYCODE_RSHIFT)	PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT(0x040, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Square") PORT_CODE(KEYCODE_LCONTROL)		PORT_CHAR(UCHAR_MAMEKEY(LCONTROL))
	PORT_BIT(0x020, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_ESC) 								PORT_CHAR(UCHAR_MAMEKEY(ESC))
	PORT_BIT(0x010, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Index") PORT_CODE(KEYCODE_F2)				PORT_CHAR(UCHAR_MAMEKEY(F2))
	PORT_BIT(0x008, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Caps Lock") PORT_CODE(KEYCODE_CAPSLOCK)		PORT_CHAR(UCHAR_MAMEKEY(CAPSLOCK))
	PORT_BIT(0x004, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_STOP)								PORT_CHAR('.') PORT_CHAR('>')
	PORT_BIT(0x002, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_SLASH)								PORT_CHAR('/') PORT_CHAR('?')
	PORT_BIT(0x001, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_BACKSLASH)							PORT_CHAR('\xA3') PORT_CHAR('~')
INPUT_PORTS_END

static MACHINE_DRIVER_START( z88 )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", Z80, 3276800)
	MDRV_CPU_PROGRAM_MAP(z88_mem)
	MDRV_CPU_IO_MAP(z88_io)
	MDRV_QUANTUM_TIME(HZ(60))

	MDRV_MACHINE_START( z88 )
	MDRV_MACHINE_RESET( z88 )

	/* video hardware */
	MDRV_SCREEN_ADD("screen", LCD)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(0))
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(Z88_SCREEN_WIDTH, 480)
	MDRV_SCREEN_VISIBLE_AREA(0, (Z88_SCREEN_WIDTH - 1), 0, (480 - 1))
	MDRV_PALETTE_LENGTH(Z88_NUM_COLOURS)
	MDRV_PALETTE_INIT( z88 )

	MDRV_VIDEO_EOF( z88 )
	MDRV_VIDEO_UPDATE( z88 )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("speaker", SPEAKER, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	/* internal ram */
	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("2M")
MACHINE_DRIVER_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START(z88)
    ROM_REGION(((64*1024)+(128*1024)), "maincpu",0)
    ROM_SYSTEM_BIOS( 0, "ver3", "Version 3.0 UK")
    ROMX_LOAD("z88v300.rom" ,  0x010000, 0x020000, CRC(802cb9aa) SHA1(ceb688025b79454cf229cae4dbd0449df2747f79), ROM_BIOS(1) )
    ROM_SYSTEM_BIOS( 1, "ver4", "Version 4.0 UK")
    ROMX_LOAD("z88v400.rom",   0x010000, 0x020000, CRC(1356d440) SHA1(23c63ceced72d0a9031cba08d2ebc72ca336921d), ROM_BIOS(2) )
    ROM_SYSTEM_BIOS( 2, "ver41fi", "Version 4.1 Finnish")
  	ROMX_LOAD("z88v401fi.rom", 0x010000, 0x020000, CRC(ecd7f3f6) SHA1(bf8d3e083f1959e5a0d7e9c8d2ad0c14abd46381), ROM_BIOS(3) )
ROM_END

/*     YEAR     NAME    PARENT  COMPAT  MACHINE     INPUT       INIT     COMPANY                 FULLNAME */
COMP( 1988,	z88,	0,		0,		z88,		z88,		0,			"Cambridge Computers",	"Z88",GAME_NOT_WORKING)

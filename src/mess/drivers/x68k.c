// Preliminary X68000 driver for MESS
// Started 18/11/2006
// Written by Barry Rodewald

/*
    *** Basic memory map

    0x000000 - 0xbfffff     RAM (Max 12MB), vector table from ROM at 0xff0000 maps to 0x000000 at reset only
    0xc00000 - 0xdfffff     Graphic VRAM
    0xe00000 - 0xe1ffff     Text VRAM Plane 1
    0xe20000 - 0xe3ffff     Text VRAM Plane 2
    0xe40000 - 0xe5ffff     Text VRAM Plane 3
    0xe60000 - 0xe7ffff     Text VRAM Plane 4
    0xe80000                CRTC
    0xe82000                Video Controller
    0xe84000                DMA Controller
    0xe86000                Supervisor Area set
    0xe88000                MFP
    0xe8a000                RTC
    0xe8c000                Printer
    0xe8e000                System Port (?)
    0xe90000                FM Sound source
    0xe92000                ADPCM
    0xe94000                FDC
    0xe96000                HDC
    0xe96021                SCSI (internal model)
    0xe98000                SCC
    0xe9a000                Serial I/O (PPI)
    0xe9c000                I/O controller

    [Expansions]
    0xe9c000 / 0xe9e000     FPU (Optional, X68000 only)
    0xea0000                SCSI
    0xeaf900                FAX
    0xeafa00 / 0xeafa10     MIDI (1st/2nd)
    0xeafb00                Serial
    0xeafc00/10/20/30       EIA232E
    0xeafd00                EIA232E
    0xeafe00                GPIB (?)
    0xec0000 - 0xecffff     User I/O Expansion

    0xeb0000 - 0xeb7fff     Sprite registers
    0xeb8000 - 0xebffff     Sprite VRAM
    0xed0000 - 0xed3fff     SRAM
    0xf00000 - 0xfb0000     ROM  (CGROM.DAT)
    0xfe0000 - 0xffffff     ROM  (IPLROM.DAT)


    *** System hardware

    CPU : X68000: 68000 at 10MHz
          X68000 XVI: 68000 at 16MHz
          X68030: 68EC030 at 25MHz

    RAM : between 1MB and 4MB stock, expandable to 12MB

    FDD : 2x 5.25", Compact models use 2x 3.5" drives.
    FDC : NEC uPD72065  (hopefully backwards compatible enough for the existing uPD765A core :))

    HDD : HD models have up to an 81MB HDD.
    HDC : Fujitsu MB89352A (SCSI)

    SCC : Serial controller - Zilog z85C30  (Dual channel, 1 for RS232, 1 for mouse)
    PPI : Parallel controller  - NEC 8255   (Printer, Joystick)

    Sound : FM    - YM2151, with YM3012 DAC
            ADPCM - Okidata MSM6258

    DMA : Hitachi HD63450, DMA I/O for FDD, HDD, Expansion slots, and ADPCM

    MFP : Motorola MC68901 - monitor sync, serial port, RTC, soft power, FM synth, IRQs, keyboard

    RTC : Ricoh RP5C15

    ...plus a number of custom chips for video and other stuff...


    *** Current status (28/12/08)
    FDC/FDD : Uses the uPD765A code with a small patch to handle Sense Interrupt Status being invalid if not in seek mode
              Extra uPD72065 commands not yet implemented, although I have yet to see them used.

    MFP : Largely works, as far as the X68000 goes.

    PPI : Joystick controls work okay.

    HDC/HDD : SCSI is not implemented, not a requirement at this point.

    RTC : Seems to work. (Tested using SX-Window's Timer application)

    DMA : Works fine.

    Sound : FM works, ADPCM mostly works (timing(?) issues in a few games).

    SCC : Works enough to get the mouse running, although only with the IPL v1.0 BIOS

    Video : Text mode works, but is rather slow, especially scrolling up (uses multple "raster copy" commands).
            Graphic layers work.
            BG tiles and sprites work, but many games have the sprites offset by a small amount (some by a lot :))
            Still a few minor priority issues around.

    Other issues:
      Bus error exceptions are a bit late at times.  Currently using a fake bus error for MIDI expansion checks.  These
      are used determine if a piece of expansion hardware is present.
      Keyboard doesn't work properly (MFP USART).
      Supervisor area set isn't implemented.

    Some minor game-specific issues (at 28/12/08):
      Pacmania:      Black squares on the maze (transparency?).
      Salamander:    System error when using keys in-game.  No error if a joystick is used.
                     Some text is drawn incorrectly.
      Kyukyoku Tiger:Sprites offset by a looooong way.
      Dragon Buster: Text is black and unreadable. (Text layer actually covers it)
      Tetris:        Black dots over screen (text layer).
      Parodius Da!:  Black squares in areas.


    More detailed documentation at http://x68kdev.emuvibes.com/iomap.html - if you can stand broken english :)

*/

#include "driver.h"
#include "cpu/m68000/m68000.h"
#include "machine/68901mfp.h"
#include "machine/8255ppi.h"
#include "machine/nec765.h"
#include "sound/2151intf.h"
#include "sound/okim6258.h"
#include "machine/8530scc.h"
#include "machine/hd63450.h"
#include "machine/rp5c15.h"
#include "devices/basicdsk.h"
#include "devices/harddriv.h"
#include "machine/x68k_hdc.h"
#include "includes/x68k.h"
#include "x68000.lh"

struct x68k_system sys;

static UINT16* sram;   // SRAM
static UINT8 ppi_port[3];
static int current_vector[8];
static UINT8 current_irq_line;
static unsigned int x68k_scanline;
static int led_state;

static UINT8 mfp_key;

static emu_timer* kb_timer;
//emu_timer* mfp_timer[4];
//emu_timer* mfp_irq;
emu_timer* scanline_timer;
emu_timer* raster_irq;
emu_timer* vblank_irq;
static emu_timer* mouse_timer;  // to set off the mouse interrupts via the SCC
static emu_timer* led_timer;  // to make disk drive control LEDs blink

// MFP is clocked at 4MHz, so at /4 prescaler the timer is triggered after 1us (4 cycles)
// No longer necessary with the new MFP core
#ifdef UNUSED_FUNCTION
static attotime prescale(int val)
{
    switch(val)
    {
        case 0: return ATTOTIME_IN_NSEC(0);
        case 1: return ATTOTIME_IN_NSEC(1000);
        case 2: return ATTOTIME_IN_NSEC(2500);
        case 3: return ATTOTIME_IN_NSEC(4000);
        case 4: return ATTOTIME_IN_NSEC(12500);
        case 5: return ATTOTIME_IN_NSEC(16000);
        case 6: return ATTOTIME_IN_NSEC(25000);
        case 7: return ATTOTIME_IN_NSEC(50000);
        default:
            fatalerror("out of range");
    }
}
#endif

static void mfp_init(void);
//static TIMER_CALLBACK(mfp_update_irq);

static void mfp_init()
{
	sys.mfp.tadr = sys.mfp.tbdr = sys.mfp.tcdr = sys.mfp.tddr = 0xff;

	sys.mfp.irqline = 6;  // MFP is connected to 68000 IRQ line 6
	sys.mfp.current_irq = -1;  // No current interrupt

/*  mfp_timer[0] = timer_alloc(machine, mfp_timer_a_callback, NULL);
    mfp_timer[1] = timer_alloc(machine, mfp_timer_b_callback, NULL);
    mfp_timer[2] = timer_alloc(machine, mfp_timer_c_callback, NULL);
    mfp_timer[3] = timer_alloc(machine, mfp_timer_d_callback, NULL);
    mfp_irq = timer_alloc(machine, mfp_update_irq, NULL);
    timer_adjust_periodic(mfp_irq, attotime_zero, 0, ATTOTIME_IN_USEC(32));
*/
}

#ifdef UNUSED_FUNCTION
TIMER_CALLBACK(mfp_update_irq)
{
    int x;

    if((sys.ioc.irqstatus & 0xc0) != 0)
        return;

    // check for pending IRQs, in priority order
    if(sys.mfp.ipra != 0)
    {
        for(x=7;x>=0;x--)
        {
            if((sys.mfp.ipra & (1 << x)) && (sys.mfp.imra & (1 << x)))
            {
                current_irq_line = sys.mfp.irqline;
                sys.mfp.current_irq = x + 8;
                // assert IRQ line
//              if(sys.mfp.iera & (1 << x))
                {
                    current_vector[6] = (sys.mfp.vr & 0xf0) | (x+8);
                    cpu_set_input_line_and_vector(machine->cpu[0],sys.mfp.irqline,ASSERT_LINE,(sys.mfp.vr & 0xf0) | (x + 8));
//                  logerror("MFP: Sent IRQ vector 0x%02x (IRQ line %i)\n",(sys.mfp.vr & 0xf0) | (x+8),sys.mfp.irqline);
                    return;  // one at a time only
                }
            }
        }
    }
    if(sys.mfp.iprb != 0)
    {
        for(x=7;x>=0;x--)
        {
            if((sys.mfp.iprb & (1 << x)) && (sys.mfp.imrb & (1 << x)))
            {
                current_irq_line = sys.mfp.irqline;
                sys.mfp.current_irq = x;
                // assert IRQ line
//              if(sys.mfp.ierb & (1 << x))
                {
                    current_vector[6] = (sys.mfp.vr & 0xf0) | x;
                    cpu_set_input_line_and_vector(machine->cpu[0],sys.mfp.irqline,ASSERT_LINE,(sys.mfp.vr & 0xf0) | x);
//                  logerror("MFP: Sent IRQ vector 0x%02x (IRQ line %i)\n",(sys.mfp.vr & 0xf0) | x,sys.mfp.irqline);
                    return;  // one at a time only
                }
            }
        }
    }
}

void mfp_trigger_irq(int irq)
{
    // check if interrupt is enabled
    if(irq > 7)
    {
        if(!(sys.mfp.iera & (1 << (irq-8))))
            return;  // not enabled, no action taken
    }
    else
    {
        if(!(sys.mfp.ierb & (1 << irq)))
            return;  // not enabled, no action taken
    }

    // set requested IRQ as pending
    if(irq > 7)
        sys.mfp.ipra |= (1 << (irq-8));
    else
        sys.mfp.iprb |= (1 << irq);

    // check for IRQs to be called
//  mfp_update_irq(0);

}

TIMER_CALLBACK(mfp_timer_a_callback)
{
    sys.mfp.timer[0].counter--;
    if(sys.mfp.timer[0].counter == 0)
    {
        sys.mfp.timer[0].counter = sys.mfp.tadr;
        mfp_trigger_irq(MFP_IRQ_TIMERA);
    }
}

TIMER_CALLBACK(mfp_timer_b_callback)
{
    sys.mfp.timer[1].counter--;
    if(sys.mfp.timer[1].counter == 0)
    {
        sys.mfp.timer[1].counter = sys.mfp.tbdr;
            mfp_trigger_irq(MFP_IRQ_TIMERB);
    }
}

TIMER_CALLBACK(mfp_timer_c_callback)
{
    sys.mfp.timer[2].counter--;
    if(sys.mfp.timer[2].counter == 0)
    {
        sys.mfp.timer[2].counter = sys.mfp.tcdr;
            mfp_trigger_irq(MFP_IRQ_TIMERC);
    }
}

TIMER_CALLBACK(mfp_timer_d_callback)
{
    sys.mfp.timer[3].counter--;
    if(sys.mfp.timer[3].counter == 0)
    {
        sys.mfp.timer[3].counter = sys.mfp.tddr;
            mfp_trigger_irq(MFP_IRQ_TIMERD);
    }
}

void mfp_set_timer(int timer, unsigned char data)
{
    if((data & 0x07) == 0x0000)
    {  // Timer stop
        timer_adjust_oneshot(mfp_timer[timer], attotime_zero, 0);
        logerror("MFP: Timer #%i stopped. \n",timer);
        return;
    }

    timer_adjust_periodic(mfp_timer[timer], attotime_zero, 0, prescale(data & 0x07));
    logerror("MFP: Timer #%i set to %2.1fus\n",timer, attotime_to_double(prescale(data & 0x07)) * 1000000);

}
#endif

// LED timer callback
static TIMER_CALLBACK( x68k_led_callback )
{
	int drive;
	if(led_state == 0)
		led_state = 1;
	else
		led_state = 0;
	if(led_state == 1)
	{
		for(drive=0;drive<4;drive++)
			output_set_indexed_value("ctrl_drv",drive,sys.fdc.led_ctrl[drive] ? 0 : 1);
	}
	else
	{
		for(drive=0;drive<4;drive++)
			output_set_indexed_value("ctrl_drv",drive,1);
	}

}

// 4 channel DMA controller (Hitachi HD63450)
static WRITE16_HANDLER( x68k_dmac_w )
{
	const device_config* device = device_list_find_by_tag(space->machine->config->devicelist,HD63450,"hd63450");
	hd63450_w(device, offset, data, mem_mask);
}

static READ16_HANDLER( x68k_dmac_r )
{
	const device_config* device = device_list_find_by_tag(space->machine->config->devicelist,HD63450,"hd63450");
	return hd63450_r(device, offset, mem_mask);
}

static void x68k_keyboard_ctrl_w(int data)
{
	/* Keyboard control commands:
       00xxxxxx - TV Control
                  Not of much use as yet

       01000xxy - y = Mouse control signal

       01001xxy - y = Keyboard enable

       010100xy - y = Sharp X1 display compatibility mode

       010101xx - xx = LED brightness (00 = bright, 11 = dark)

       010110xy - y = Display control enable

       010111xy - y = Display control via the Opt. 2 key enable

       0110xxxx - xxxx = Key delay (default 500ms)
                         100 * (delay time) + 200ms

       0111xxxx - xxxx = Key repeat rate  (default 110ms)
                         (repeat rate)^2*5 + 30ms

       1xxxxxxx - xxxxxxx = keyboard LED status
                  b6 = "full size"
                  b5 = hiragana
                  b4 = insert
                  b3 = caps
                  b2 = code input
                  b1 = romaji input
                  b0 = kana
    */

	if(data & 0x80)  // LED status
	{
		output_set_value("key_led_kana",(data & 0x01) ? 0 : 1);
		output_set_value("key_led_romaji",(data & 0x02) ? 0 : 1);
		output_set_value("key_led_code",(data & 0x04) ? 0 : 1);
		output_set_value("key_led_caps",(data & 0x08) ? 0 : 1);
		output_set_value("key_led_insert",(data & 0x10) ? 0 : 1);
		output_set_value("key_led_hiragana",(data & 0x20) ? 0 : 1);
		output_set_value("key_led_fullsize",(data & 0x40) ? 0 : 1);
		logerror("KB: LED status set to %02x\n",data & 0x7f);
	}

	if((data & 0xc0) == 0)  // TV control
	{
		// nothing for now
	}

	if((data & 0xf8) == 0x48)  // Keyboard enable
	{
		sys.keyboard.enabled = data & 0x01;
		logerror("KB: Keyboard enable bit = %i\n",sys.keyboard.enabled);
	}

	if((data & 0xf0) == 0x60)  // Key delay time
	{
		sys.keyboard.delay = data & 0x0f;
		logerror("KB: Keypress delay time is now %ims\n",(data & 0x0f)*100+200);
	}

	if((data & 0xf0) == 0x70)  // Key repeat rate
	{
		sys.keyboard.repeat = data & 0x0f;
		logerror("KB: Keypress repeat rate is now %ims\n",((data & 0x0f)^2)*5+30);
	}

}

int x68k_keyboard_pop_scancode(void)
{
	int ret;
	if(sys.keyboard.keynum == 0)  // no scancodes in USART buffer
		return 0x00;

	sys.keyboard.keynum--;
	ret = sys.keyboard.buffer[sys.keyboard.tailpos++];
	if(sys.keyboard.tailpos > 15)
		sys.keyboard.tailpos = 0;

	logerror("MFP: Keyboard buffer pop 0x%02x\n",ret);
	return ret;
}

static void x68k_keyboard_push_scancode(running_machine* machine,unsigned char code)
{
	sys.keyboard.keynum++;
	if(sys.keyboard.keynum >= 1)
	{ // keyboard buffer full
		if(sys.keyboard.enabled != 0)
		{
			sys.mfp.rsr |= 0x80;  // Buffer full
//          mfp_trigger_irq(MFP_IRQ_RX_FULL);
			current_vector[6] = 0x4c;
			cpu_set_input_line_and_vector(machine->cpu[0],6,ASSERT_LINE,0x4c);
			logerror("MFP: Receive buffer full IRQ sent\n");
		}
	}
	sys.keyboard.buffer[sys.keyboard.headpos++] = code;
	if(sys.keyboard.headpos > 15)
	{
		sys.keyboard.headpos = 0;
//      mfp_trigger_irq(MFP_IRQ_RX_ERROR);
		current_vector[6] = 0x4b;
//		cpu_set_input_line_and_vector(machine->cpu[0],6,ASSERT_LINE,0x4b);
	}
}

static TIMER_CALLBACK(x68k_keyboard_poll)
{
	int x;
	static const char *const keynames[] = { "key1", "key2", "key3", "key4" };

	for(x=0;x<0x80;x++)
	{
		// adjust delay/repeat timers
		if(sys.keyboard.keytime[x] > 0)
		{
			sys.keyboard.keytime[x] -= 5;
		}
		if(!(input_port_read(machine, keynames[x / 32]) & (1 << (x % 32))))
		{
			if(sys.keyboard.keyon[x] != 0)
			{
				x68k_keyboard_push_scancode(machine,0x80 + x);
				sys.keyboard.keytime[x] = 0;
				sys.keyboard.keyon[x] = 0;
				sys.keyboard.last_pressed = 0;
				logerror("KB: Released key 0x%02x\n",x);
			}
		}
		// check to see if a key is being held
		if(sys.keyboard.keyon[x] != 0 && sys.keyboard.keytime[x] == 0 && sys.keyboard.last_pressed == x)
		{
			if(input_port_read(machine, keynames[sys.keyboard.last_pressed / 32]) & (1 << (sys.keyboard.last_pressed % 32)))
			{
				x68k_keyboard_push_scancode(machine,sys.keyboard.last_pressed);
				sys.keyboard.keytime[sys.keyboard.last_pressed] = (sys.keyboard.repeat^2)*5+30;
				logerror("KB: Holding key 0x%02x\n",sys.keyboard.last_pressed);
			}
		}
		if((input_port_read(machine,  keynames[x / 32]) & (1 << (x % 32))))
		{
			if(sys.keyboard.keyon[x] == 0)
			{
				x68k_keyboard_push_scancode(machine,x);
				sys.keyboard.keytime[x] = sys.keyboard.delay * 100 + 200;
				sys.keyboard.keyon[x] = 1;
				sys.keyboard.last_pressed = x;
				logerror("KB: Pushed key 0x%02x\n",x);
			}
		}
	}
}


#ifdef UNUSED_FUNCTION
void mfp_recv_data(int data)
{
	sys.mfp.rsr |= 0x80;  // Buffer full
	sys.mfp.tsr |= 0x80;
	sys.mfp.usart.recv_buffer = 0x00;   // TODO: set up keyboard data
	sys.mfp.vector = current_vector[6] = (sys.mfp.vr & 0xf0) | 0x0c;
//  mfp_trigger_irq(MFP_IRQ_RX_FULL);
//  logerror("MFP: Receive buffer full IRQ sent\n");
}
#endif

// mouse input
// port B of the Z8530 SCC
// typically read from the SCC data port on receive buffer full interrupt per byte
static int x68k_read_mouse(running_machine *machine)
{
	char val = 0;
	char ipt = 0;

	switch(sys.mouse.inputtype)
	{
	case 0:
		ipt = input_port_read(machine, "mouse1");
		break;
	case 1:
		val = input_port_read(machine, "mouse2");
		ipt = val - sys.mouse.last_mouse_x;
		sys.mouse.last_mouse_x = val;
		break;
	case 2:
		val = input_port_read(machine, "mouse3");
		ipt = val - sys.mouse.last_mouse_y;
		sys.mouse.last_mouse_y = val;
		break;
	}
	sys.mouse.inputtype++;
	if(sys.mouse.inputtype > 2)
	{
		const device_config *scc = device_list_find_by_tag(machine->config->devicelist, SCC8530, "scc");
		int val = scc_get_reg_b(scc, 0);
		sys.mouse.inputtype = 0;
		sys.mouse.bufferempty = 1;
		val &= ~0x01;
		scc_set_reg_b(scc, 0, val);
		logerror("SCC: mouse buffer empty\n");
	}

	return ipt;
}

/*
    0xe98001 - Z8530 command port B
    0xe98003 - Z8530 data port B  (mouse input)
    0xe98005 - Z8530 command port A
    0xe98007 - Z8530 data port A  (RS232)
*/
static READ16_HANDLER( x68k_scc_r )
{
	const device_config *scc = device_list_find_by_tag(space->machine->config->devicelist, SCC8530, "scc");
	offset %= 4;
	switch(offset)
	{
	case 0:
		return scc_r(scc, 0);
	case 1:
		return x68k_read_mouse(space->machine);
	case 2:
		return scc_r(scc, 1);
	case 3:
		return scc_r(scc, 3);
	default:
		return 0xff;
	}
}

static WRITE16_HANDLER( x68k_scc_w )
{
	const device_config *scc = device_list_find_by_tag(space->machine->config->devicelist, SCC8530, "scc");
	static unsigned char prev;
	offset %= 4;

	switch(offset)
	{
	case 0:
		scc_w(scc, 0,(UINT8)data);
		if((scc_get_reg_b(scc, 5) & 0x02) != prev)
		{
			if(scc_get_reg_b(scc, 5) & 0x02)  // Request to Send
			{
				int val = scc_get_reg_b(scc, 0);
				sys.mouse.bufferempty = 0;
				val |= 0x01;
				scc_set_reg_b(scc, 0,val);
			}
		}
		break;
	case 1:
		scc_w(scc, 2,(UINT8)data);
		break;
	case 2:
		scc_w(scc, 1,(UINT8)data);
		break;
	case 3:
		scc_w(scc, 3,(UINT8)data);
		break;
	}
	prev = scc_get_reg_b(scc, 5) & 0x02;
}

static TIMER_CALLBACK(x68k_scc_ack)
{
	const device_config *scc = device_list_find_by_tag(machine->config->devicelist, SCC8530, "scc");
	if(sys.mouse.bufferempty != 0)  // nothing to do if the mouse data buffer is empty
		return;

//	if((sys.ioc.irqstatus & 0xc0) != 0)
//		return;

	// hard-code the IRQ vector for now, until the SCC code is more complete
	if((scc_get_reg_a(scc, 9) & 0x08) || (scc_get_reg_b(scc, 9) & 0x08))  // SCC reg WR9 is the same for both channels
	{
		if((scc_get_reg_b(scc, 1) & 0x18) != 0)  // if bits 3 and 4 of WR1 are 0, then Rx IRQs are disabled on this channel
		{
			if(scc_get_reg_b(scc, 5) & 0x02)  // RTS signal
			{
				sys.mouse.irqactive = 1;
				current_vector[5] = 0x54;
				current_irq_line = 5;
				cpu_set_input_line_and_vector(machine->cpu[0],5,ASSERT_LINE,0x54);
			}
		}
	}
}

static void x68k_set_adpcm(running_machine* machine)
{
	const device_config *dev = device_list_find_by_tag(machine->config->devicelist, HD63450, "hd63450");
	UINT32 rate = 0;

	switch(sys.adpcm.rate & 0x0c)
	{
		case 0x00:
			rate = 7812/2;
			break;
		case 0x04:
			rate = 10417/2;
			break;
		case 0x08:
			rate = 15625/2;
			break;
		default:
			logerror("PPI: Invalid ADPCM sample rate set.\n");
			rate = 15625/2;
	}
	if(sys.adpcm.clock != 0)
		rate = rate/2;
	hd63450_set_timer(dev,3,ATTOTIME_IN_HZ(rate));
}


// Judging from the XM6 source code, PPI ports A and B are joystick inputs
static READ8_DEVICE_HANDLER( ppi_port_a_r )
{
	// Joystick 1
	if(sys.joy.joy1_enable == 0)
		return input_port_read(device->machine, "joy1");
	else
		return 0xff;
}

static READ8_DEVICE_HANDLER( ppi_port_b_r )
{
	// Joystick 2
	if(sys.joy.joy2_enable == 0)
		return input_port_read(device->machine, "joy2");
	else
		return 0xff;
}

static READ8_DEVICE_HANDLER( ppi_port_c_r )
{
	return ppi_port[2];
}

/* PPI port C (Joystick control, R/W)
   bit 7    - IOC7 - Function B operation of joystick 1 (?)
   bit 6    - IOC6 - Function A operation of joystick 1 (?)
   bit 5    - IOC5 - Enable Joystick 2
   bit 4    - IOC4 - Enable Joystick 1
   bits 3,2 - ADPCM Sample rate
   bits 1,0 - ADPCM Pan
*/
static WRITE8_DEVICE_HANDLER( ppi_port_c_w )
{
	// ADPCM / Joystick control
	
	ppi_port[2] = data;
	sys.adpcm.pan = data & 0x03;
	sys.adpcm.rate = data & 0x0c;
	x68k_set_adpcm(device->machine);
	okim6258_set_divider(0, (data >> 2) & 3);

	sys.joy.joy1_enable = data & 0x10;
	sys.joy.joy2_enable = data & 0x20;
	sys.joy.ioc6 = data & 0x40;
	sys.joy.ioc7 = data & 0x80;
}


// NEC uPD72065 at 0xe94000
static WRITE16_HANDLER( x68k_fdc_w )
{
	device_config *fdc = (device_config*)device_list_find_by_tag( space->machine->config->devicelist, NEC72065, "nec72065");
	unsigned int drive, x;
	switch(offset)
	{
	case 0x00:
	case 0x01:
		nec765_data_w(fdc, 0,data);
		break;
	case 0x02:  // drive option signal control
		x = data & 0x0f;
		for(drive=0;drive<4;drive++)
		{
			if(x & (1 << drive))
			{
				sys.fdc.led_ctrl[drive] = data & 0x80;  // blinking drive LED if no disk inserted
				sys.fdc.led_eject[drive] = data & 0x40;  // eject button LED (on when set to 0)
				output_set_indexed_value("eject_drv",drive,(data & 0x40) ? 1 : 0);
				if(data & 0x20)  // ejects disk
				{
					image_unload(image_from_devtype_and_index(space->machine, IO_FLOPPY, drive));
					floppy_drive_set_motor_state(image_from_devtype_and_index(space->machine, IO_FLOPPY, drive), 0);  // I'll presume ejecting the disk stops the drive motor :)
				}
			}
		}
		sys.fdc.selected_drive = data & 0x0f;
		logerror("FDC: signal control set to %02x\n",data);
		break;
	case 0x03:
		sys.fdc.media_density[data & 0x03] = data & 0x10;
		sys.fdc.motor[data & 0x03] = data & 0x80;
		floppy_drive_set_motor_state(image_from_devtype_and_index(space->machine, IO_FLOPPY, data & 0x03), (data & 0x80));
		if(data & 0x80)
		{
			for(drive=0;drive<4;drive++) // enable motor for this drive
			{
				if(drive == (data & 0x03))
				{
					floppy_drive_set_motor_state(image_from_devtype_and_index(space->machine, IO_FLOPPY, drive), 1);
					output_set_indexed_value("access_drv",drive,0);
				}
				else
					output_set_indexed_value("access_drv",drive,1);
			}
		}
		else    // BIOS code suggests that setting bit 7 of this port to 0 disables the motor of all floppy drives
		{
			for(drive=0;drive<4;drive++)
			{
				floppy_drive_set_motor_state(image_from_devtype_and_index(space->machine, IO_FLOPPY, drive), 0);
				output_set_indexed_value("access_drv",drive,1);
			}
		}
		floppy_drive_set_ready_state(image_from_devtype_and_index(space->machine, IO_FLOPPY, 0),1,1);
		floppy_drive_set_ready_state(image_from_devtype_and_index(space->machine, IO_FLOPPY, 1),1,1);
		floppy_drive_set_ready_state(image_from_devtype_and_index(space->machine, IO_FLOPPY, 2),1,1);
		floppy_drive_set_ready_state(image_from_devtype_and_index(space->machine, IO_FLOPPY, 3),1,1);
//		for(drive=0;drive<4;drive++)
//		{
//			if(floppy_drive_get_flag_state(image_from_devtype_and_index(machine, IO_FLOPPY, drive),FLOPPY_DRIVE_MOTOR_ON))
//				output_set_indexed_value("access_drv",drive,0);
//			else
//				output_set_indexed_value("access_drv",drive,1);
//		}
		logerror("FDC: Drive #%i: Drive selection set to %02x\n",data & 0x03,data);
		break;
	default:
//      logerror("FDC: [%08x] Wrote %04x to invalid FDC port %04x\n",cpu_get_pc(space->cpu),data,offset);
		break;
	}
}

static READ16_HANDLER( x68k_fdc_r )
{
	unsigned int ret;
	int x;
	device_config *fdc = (device_config*)device_list_find_by_tag( space->machine->config->devicelist, NEC72065, "nec72065");
	
	switch(offset)
	{
	case 0x00:
		return nec765_status_r(fdc, 0);
	case 0x01:
		return nec765_data_r(fdc, 0);
	case 0x02:
		ret = 0x00;
		for(x=0;x<4;x++)
		{
			if(sys.fdc.selected_drive & (1 << x))
			{
				ret = 0x00;
				if(sys.fdc.disk_inserted[x] != 0)
				{
					ret |= 0x80;
				}
				// bit 7 = disk inserted
				// bit 6 = disk error (in insertion, presumably)
				logerror("FDC: Drive #%i Disk check - returning %02x\n",x,ret);
			}
		}
		return ret;
	case 0x03:
		logerror("FDC: IOC selection is write-only\n");
		return 0xff;
	default:
		logerror("FDC: Read from invalid FDC port %04x\n",offset);
		return 0xff;
	}
}

static NEC765_INTERRUPT( fdc_irq )
{
	if((sys.ioc.irqstatus & 0x04) && state == ASSERT_LINE)
	{
		current_vector[1] = sys.ioc.fdcvector;
		sys.ioc.irqstatus |= 0x80;
		current_irq_line = 1;
		logerror("FDC: IRQ triggered\n");
		cpu_set_input_line_and_vector(device->machine->cpu[0],1,ASSERT_LINE,current_vector[1]);
	}
}

static int x68k_fdc_read_byte(running_machine *machine,int addr)
{
	int data = -1;
	device_config *fdc = (device_config*)device_list_find_by_tag( machine->config->devicelist, NEC72065, "nec72065");

	if(sys.fdc.drq_state != 0)
		data = nec765_dack_r(fdc, 0);
//  logerror("FDC: DACK reading\n");
	return data;
}

static void x68k_fdc_write_byte(running_machine *machine,int addr, int data)
{
	device_config *fdc = (device_config*)device_list_find_by_tag( machine->config->devicelist, NEC72065, "nec72065");
	nec765_dack_w(fdc, 0, data);
}

static NEC765_DMA_REQUEST ( fdc_drq )
{
	sys.fdc.drq_state = state;
}

static WRITE16_HANDLER( x68k_fm_w )
{
	switch(offset)
	{
	case 0x00:
		ym2151_register_port_0_w(space, 0, data);
		logerror("YM: Register select 0x%02x\n",data);
		break;
	case 0x01:
		ym2151_data_port_0_w(space, 0, data);
		logerror("YM: Data write 0x%02x\n",data);
		break;
	}
}

static READ16_HANDLER( x68k_fm_r )
{
	if(offset == 0x01)
		return ym2151_status_port_0_r(space, 0);

	return 0xff;
}

static WRITE8_HANDLER( x68k_ct_w )
{
	device_config *fdc = (device_config*)device_list_find_by_tag( space->machine->config->devicelist, NEC72065, "nec72065");
	// CT1 and CT2 bits from YM2151 port 0x1b
	// CT1 - ADPCM clock - 0 = 8MHz, 1 = 4MHz
	// CT2 - 1 = Set ready state of FDC
	nec765_set_ready_state(fdc,data & 0x01);
	sys.adpcm.clock = data & 0x02;
	x68k_set_adpcm(space->machine);
	okim6258_set_clock(0, data & 0x02 ? 4000000 : 8000000);
}

/*
 Custom I/O controller at 0xe9c000
 0xe9c001 (R) - Interrupt status
 0xe9c001 (W) - Interrupt mask (low nibble only)
                - bit 7 = FDC interrupt
                - bit 6 = FDD interrupt
                - bit 5 = Printer Busy signal
                - bit 4 = HDD interrupt
                - bit 3 = HDD interrupts enabled
                - bit 2 = FDC interrupts enabled
                - bit 1 = FDD interrupts enabled
                - bit 0 = Printer interrupts enabled
 0xe9c003 (W) - Interrupt vector
                - bits 7-2 = vector
                - bits 1,0 = device (00 = FDC, 01 = FDD, 10 = HDD, 11 = Printer)
*/
static WRITE16_HANDLER( x68k_ioc_w )
{
	switch(offset)
	{
	case 0x00:
		sys.ioc.irqstatus = data & 0x0f;
		logerror("I/O: Status register write %02x\n",data);
		break;
	case 0x01:
		switch(data & 0x03)
		{
		case 0x00:
			sys.ioc.fdcvector = data & 0xfc;
			logerror("IOC: FDC IRQ vector = 0x%02x\n",data & 0xfc);
			break;
		case 0x01:
			sys.ioc.fddvector = data & 0xfc;
			logerror("IOC: FDD IRQ vector = 0x%02x\n",data & 0xfc);
			break;
		case 0x02:
			sys.ioc.hdcvector = data & 0xfc;
			logerror("IOC: HDD IRQ vector = 0x%02x\n",data & 0xfc);
			break;
		case 0x03:
			sys.ioc.prnvector = data & 0xfc;
			logerror("IOC: Printer IRQ vector = 0x%02x\n",data & 0xfc);
			break;
		}
		break;
	}
}

static READ16_HANDLER( x68k_ioc_r )
{
	switch(offset)
	{
	case 0x00:
		logerror("I/O: Status register read\n");
		return (sys.ioc.irqstatus & 0xdf) | 0x20;
	default:
		return 0x00;
	}
}

/*
 System ports at 0xe8e000
 Port 1 (0xe8e001) - Monitor contrast (bits 3-0)
 Port 2 (0xe8e003) - Display / 3D scope control
                     - bit 3 - Display control signal (0 = on)
                     - bit 1 - 3D scope left shutter (0 = closed)
                     - bit 0 - 3D scope right shutter
 Port 3 (0xe8e005) - Colour image unit control (bits 3-0)
 Port 4 (0xe8e007) - Keyboard/NMI/Dot clock
                     - bit 3 - (R) 1 = Keyboard connected, (W) 1 = Key data can be transmitted
                     - bit 1 - NMI Reset
                     - bit 0 - HRL - high resolution dot clock - 1 = 1/2, 1/4, 1/8, 0 = 1/2, 1/3, 1/6 (normal)
 Port 5 (0xe8e009) - ROM (bits 7-4)/DRAM (bits 3-0) wait, X68030 only
 Port 6 (0xe8e00b) - CPU type and clock speed (XVI or later only, X68000 returns 0xFF)
                     - bits 7-4 - CPU Type (1100 = 68040, 1101 = 68030, 1110 = 68020, 1111 = 68000)
                     - bits 3-0 - clock speed (1001 = 50MHz, 40, 33, 25, 20, 16, 1111 = 10MHz)
 Port 7 (0xe8e00d) - SRAM write enable - if 0x31 is written to this port, writing to SRAM is allowed.
                                         Any other value, then SRAM is read only.
 Port 8 (0xe8e00f) - Power off control - write 0x00, 0x0f, 0x0f sequentially to switch power off.
*/
static WRITE16_HANDLER( x68k_sysport_w )
{
	switch(offset)
	{
	case 0x00:
		sys.sysport.contrast = data & 0x0f;  // often used for screen fades / blanking
		// TODO: implement a decent, not slow, brightness control
		break;
	case 0x01:
		sys.sysport.monitor = data & 0x08;
		break;
	case 0x03:
		sys.sysport.keyctrl = data & 0x08;  // bit 3 = enable keyboard data transmission
		break;
	case 0x06:
		sys.sysport.sram_writeprotect = data;
		break;
	default:
//      logerror("SYS: [%08x] Wrote %04x to invalid or unimplemented system port %04x\n",cpu_get_pc(space->cpu),data,offset);
		break;
	}
}

static READ16_HANDLER( x68k_sysport_r )
{
	int ret = 0;
	switch(offset)
	{
	case 0x00:  // monitor contrast setting (bits3-0)
		return sys.sysport.contrast;
	case 0x01:  // monitor control (bit3) / 3D Scope (bits1,0)
		ret |= sys.sysport.monitor;
		return ret;
	case 0x03:  // bit 3 = key control (is 1 if keyboard is connected)
		return 0x08;
	case 0x05:  // CPU type and speed
		return 0xff;  // 68000, 10MHz.  Technically, the port doesn't exist on the X68000, only the X68000 XVI and X68030
	default:
		logerror("Read from invalid or unimplemented system port %04x\n",offset);
		return 0xff;
	}
}

/*static READ16_HANDLER( x68k_mfp_r )
{
	const device_config *x68k_mfp = device_list_find_by_tag(space->machine->config->devicelist, MC68901, MC68901_TAG);

	return mc68901_register_r(x68k_mfp, offset);
}*/

READ16_HANDLER( x68k_mfp_r )
{
	const device_config *x68k_mfp = device_list_find_by_tag(space->machine->config->devicelist, MC68901, MC68901_TAG);
    // Initial settings indicate that IRQs are generated for FM (YM2151), Receive buffer error or full,
    // MFP Timer C, and the power switch
//  logerror("MFP: [%08x] Reading offset %i\n",cpu_get_pc(space->cpu),offset);
    switch(offset)
    {
/*    case 0x00:  // GPIP - General purpose I/O register (read-only)
        ret = 0x23;
        if(video_screen_get_vpos(machine->primary_screen) == sys.crtc.reg[9])
            ret |= 0x40;
        if(sys.crtc.vblank == 0)
            ret |= 0x10;  // Vsync signal (low if in vertical retrace)
//      if(sys.mfp.isrb & 0x08)
//          ret |= 0x08;  // FM IRQ signal
        if(video_screen_get_hpos(machine->primary_screen) > sys.crtc.width - 32)
            ret |= 0x80;  // Hsync signal
//      logerror("MFP: [%08x] Reading offset %i (ret=%02x)\n",cpu_get_pc(space->cpu),offset,ret);
        return ret;  // bit 5 is always 1
    case 3:
        return sys.mfp.iera;
    case 4:
        return sys.mfp.ierb;
    case 5:
        return sys.mfp.ipra;
    case 6:
        return sys.mfp.iprb;
    case 7:
        if(sys.mfp.eoi_mode == 0)  // forced low in auto EOI mode
            return 0;
        else
            return sys.mfp.isra;
    case 8:
        if(sys.mfp.eoi_mode == 0)  // forced low in auto EOI mode
            return 0;
        else
            return sys.mfp.isrb;
    case 9:
        return sys.mfp.imra;
    case 10:
        return sys.mfp.imrb;
    case 15:  // TADR
        return sys.mfp.timer[0].counter;  // Timer data registers return their main counter values
    case 16:  // TBDR
        return sys.mfp.timer[1].counter;
    case 17:  // TCDR
        return sys.mfp.timer[2].counter;
    case 18:  // TDDR
        return sys.mfp.timer[3].counter;*/
    case 21:  // RSR
        return sys.mfp.rsr;
    case 22:  // TSR
        return sys.mfp.tsr | 0x80;  // buffer is typically empty?
    case 23:
        return x68k_keyboard_pop_scancode();
    default:
		if (ACCESSING_BITS_0_7) return mc68901_register_r(x68k_mfp, offset);
    }
    return 0xffff;
}

static WRITE16_HANDLER( x68k_mfp_w )
{
	const device_config *x68k_mfp = device_list_find_by_tag(space->machine->config->devicelist, MC68901, MC68901_TAG);

	/* For the Interrupt registers, the bits are set out as such:
       Reg A - bit 7: GPIP7 (HSync)
               bit 6: GPIP6 (CRTC CIRQ)
               bit 5: Timer A
               bit 4: Receive buffer full
               bit 3: Receive error
               bit 2: Transmit buffer empty
               bit 1: Transmit error
               bit 0: Timer B
       Reg B - bit 7: GPIP5 (Unused, always 1)
               bit 6: GPIP4 (VSync)
               bit 5: Timer C
               bit 4: Timer D
               bit 3: GPIP3 (FM IRQ)
               bit 2: GPIP2 (Power switch)
               bit 1: GPIP1 (EXPON)
               bit 0: GPIP0 (Alarm)
    */
	switch(offset)
	{
/*  case 0:  // GPDR
        // All bits are inputs generally, so no action taken.
        break;
    case 1:  // AER
        sys.mfp.aer = data;
        break;
    case 2:  // DDR
        sys.mfp.ddr = data;  // usually all bits are 0 (input)
        break;
    case 3:  // IERA
        sys.mfp.iera = data;
        break;
    case 4:  // IERB
        sys.mfp.ierb = data;
        break;
    case 5:  // IPRA
        sys.mfp.ipra = data;
        break;
    case 6:  // IPRB
        sys.mfp.iprb = data;
        break;
    case 7:
        sys.mfp.isra = data;
        break;
    case 8:
        sys.mfp.isrb = data;
        break;
    case 9:
        sys.mfp.imra = data;
//      mfp_update_irq(0);
//      logerror("MFP: IRQ Mask A write: %02x\n",data);
        break;
    case 10:
        sys.mfp.imrb = data;
//      mfp_update_irq(0);
//      logerror("MFP: IRQ Mask B write: %02x\n",data);
        break;
    case 11:  // VR
        sys.mfp.vr = 0x40;//data;  // High 4 bits = high 4 bits of IRQ vector
        sys.mfp.eoi_mode = data & 0x08;  // 0 = Auto, 1 = Software End-of-interrupt
        if(sys.mfp.eoi_mode == 0)  // In-service registers are cleared if this bit is cleared.
        {
            sys.mfp.isra = 0;
            sys.mfp.isrb = 0;
        }
        break;
    case 12:  // TACR
        sys.mfp.tacr = data;
        mfp_set_timer(0,data & 0x0f);
        break;
    case 13:  // TBCR
        sys.mfp.tbcr = data;
        mfp_set_timer(1,data & 0x0f);
        break;
    case 14:  // TCDCR
        sys.mfp.tcdcr = data;
        mfp_set_timer(2,(data & 0x70)>>4);
        mfp_set_timer(3,data & 0x07);
        break;
    case 15:  // TADR
        sys.mfp.tadr = data;
        sys.mfp.timer[0].counter = data;
        break;
    case 16:  // TBDR
        sys.mfp.tbdr = data;
        sys.mfp.timer[1].counter = data;
        break;
    case 17:  // TCDR
        sys.mfp.tcdr = data;
        sys.mfp.timer[2].counter = data;
        break;
    case 18:  // TDDR
        sys.mfp.tddr = data;
        sys.mfp.timer[3].counter = data;
        break;
    case 20:
        sys.mfp.ucr = data;
        break;*/
    case 21:
        if(data & 0x01)
            sys.mfp.usart.recv_enable = 1;
        else
            sys.mfp.usart.recv_enable = 0;
        break;
	case 22:
		if(data & 0x01)
			sys.mfp.usart.send_enable = 1;
		else
			sys.mfp.usart.send_enable = 0;
		break;
	case 23:
		if(sys.mfp.usart.send_enable != 0)
		{
			// Keyboard control command.
			sys.mfp.usart.send_buffer = data;
			x68k_keyboard_ctrl_w(data);
//          logerror("MFP: [%08x] USART Sent data %04x\n",cpu_get_pc(space->cpu),data);
		}
		break;
	default:
		if (ACCESSING_BITS_0_7) mc68901_register_w(x68k_mfp, offset, data & 0xff);
		return;
	}
}


static WRITE16_DEVICE_HANDLER( x68k_ppi_w )
{
	ppi8255_w(device,offset & 0x03,data);
}

static READ16_DEVICE_HANDLER( x68k_ppi_r )
{
	return ppi8255_r(device,offset & 0x03);
}

static READ16_HANDLER( x68k_rtc_r )
{
	return rp5c15_r(space, offset, mem_mask);
}

static WRITE16_HANDLER( x68k_rtc_w )
{
	rp5c15_w(space, offset, data, mem_mask);
}

static void x68k_rtc_alarm_irq(int state)
{
	if(sys.mfp.aer & 0x01)
	{
		if(state == 1)
			sys.mfp.gpio |= 0x01;
			//mfp_trigger_irq(MFP_IRQ_GPIP0);  // RTC ALARM
	}
	else
	{
		if(state == 0)
			sys.mfp.gpio &= ~0x01;
			//mfp_trigger_irq(MFP_IRQ_GPIP0);  // RTC ALARM
	}
}


static WRITE16_HANDLER( x68k_sram_w )
{
	if(sys.sysport.sram_writeprotect == 0x31)
	{
		COMBINE_DATA(generic_nvram16+offset);
	}
}

static READ16_HANDLER( x68k_sram_r )
{
	// HACKS!
//	if(offset == 0x5a/2)  // 0x5a should be 0 if no SASI HDs are present.
//		return 0x0000;
	if(offset == 0x08/2)
		return mess_ram_size >> 16;  // RAM size
	/*if(offset == 0x46/2)
        return 0x0024;
    if(offset == 0x6e/2)
        return 0xff00;
    if(offset == 0x70/2)
        return 0x0700;*/
	return generic_nvram16[offset];
}

static WRITE16_HANDLER( x68k_vid_w )
{
	int val;
	if(offset < 0x100)  // Graphic layer palette
	{
		COMBINE_DATA(sys.video.gfx_pal+offset);
		val = sys.video.gfx_pal[offset];
		palette_set_color_rgb(space->machine,offset,(val & 0x07c0) >> 3,(val & 0xf800) >> 8,(val & 0x003e) << 2);
		return;
	}

	if(offset >= 0x100 && offset < 0x200)  // Text / Sprites / Tilemap palette
	{
		COMBINE_DATA(sys.video.text_pal+(offset-0x100));
		val = sys.video.text_pal[offset-0x100];
		palette_set_color_rgb(space->machine,offset,(val & 0x07c0) >> 3,(val & 0xf800) >> 8,(val & 0x003e) << 2);
		return;
	}

	switch(offset)
	{
	case 0x200:
		COMBINE_DATA(sys.video.reg);
		break;
	case 0x280:  // priority levels
		COMBINE_DATA(sys.video.reg+1);
		if(ACCESSING_BITS_0_7)
		{
			sys.video.gfxlayer_pri[0] = data & 0x0003;
			sys.video.gfxlayer_pri[1] = (data & 0x000c) >> 2;
			sys.video.gfxlayer_pri[2] = (data & 0x0030) >> 4;
			sys.video.gfxlayer_pri[3] = (data & 0x00c0) >> 6;
		}
		if(ACCESSING_BITS_8_15)
		{
			sys.video.gfx_pri = (data & 0x0300) >> 8;
			sys.video.text_pri = (data & 0x0c00) >> 10;
			sys.video.sprite_pri = (data & 0x3000) >> 12;
			if(sys.video.gfx_pri == 3)
				sys.video.gfx_pri--;
			if(sys.video.text_pri == 3)
				sys.video.text_pri--;
			if(sys.video.sprite_pri == 3)
				sys.video.sprite_pri--;
		}
		break;
	case 0x300:
		COMBINE_DATA(sys.video.reg+2);
		break;
	default:
		logerror("VC: Invalid video controller write (offset = 0x%04x, data = %04x)\n",offset,data);
	}
}

static READ16_HANDLER( x68k_vid_r )
{
	if(offset < 0x100)
		return sys.video.gfx_pal[offset];

	if(offset >= 0x100 && offset < 0x200)
		return sys.video.text_pal[offset-0x100];

	switch(offset)
	{
	case 0x200:
		return sys.video.reg[0];
	case 0x280:
		return sys.video.reg[1];
	case 0x300:
		return sys.video.reg[2];
	default:
		logerror("VC: Invalid video controller read (offset = 0x%04x)\n",offset);
	}

	return 0xff;
}

static READ16_HANDLER( x68k_areaset_r )
{
	// register is write-only
	return 0xffff;
}

static WRITE16_HANDLER( x68k_areaset_w )
{
	// TODO
	logerror("SYS: Supervisor area set: 0x%02x\n",data & 0xff);
}

static WRITE16_HANDLER( x68k_enh_areaset_w )
{
	// TODO
	logerror("SYS: Enhanced Supervisor area set (from %iMB): 0x%02x\n",(offset + 1) * 2,data & 0xff);
}

static TIMER_CALLBACK(x68k_fake_bus_error)
{
	int val = param;

	// rather hacky, but this generally works for programs that check for MIDI hardware
	if(mess_ram[0x09] != 0x02)  // normal vector for bus errors points to 02FF0540
	{
		int addr = (mess_ram[0x09] << 24) | (mess_ram[0x08] << 16) |(mess_ram[0x0b] << 8) | mess_ram[0x0a];
		int sp = cpu_get_reg(machine->cpu[0],REG_GENSP);
		int pc = cpu_get_reg(machine->cpu[0],REG_GENPC);
		int sr = cpu_get_reg(machine->cpu[0],M68K_SR);
		//int pda = cpu_get_reg(machine->cpu[0],M68K_PREF_DATA);
		cpu_set_reg(machine->cpu[0],REG_GENSP,sp-14);
		mess_ram[sp-11] = (val & 0xff000000) >> 24;
		mess_ram[sp-12] = (val & 0x00ff0000) >> 16;
		mess_ram[sp-9] = (val & 0x0000ff00) >> 8;
		mess_ram[sp-10] = (val & 0x000000ff);  // place address onto the stack
		mess_ram[sp-3] = (pc & 0xff000000) >> 24;
		mess_ram[sp-4] = (pc & 0x00ff0000) >> 16;
		mess_ram[sp-1] = (pc & 0x0000ff00) >> 8;
		mess_ram[sp-2] = (pc & 0x000000ff);  // place PC onto the stack
		mess_ram[sp-5] = (sr & 0xff00) >> 8;
		mess_ram[sp-6] = (sr & 0x00ff);  // place SR onto the stack
		cpu_set_reg(machine->cpu[0],REG_GENPC,addr);  // real exceptions seem to take too long to be acknowledged
		popmessage("Expansion access [%08x]: PC jump to %08x",val,addr);
	}
}

static READ16_HANDLER( x68k_rom0_r )
{
	/* this location contains the address of some expansion device ROM, if no ROM exists,
       then access causes a bus error */
	current_vector[2] = 0x02;  // bus error
	current_irq_line = 2;
//	cpu_set_input_line_and_vector(space->machine->cpu[0],2,ASSERT_LINE,current_vector[2]);
	if(input_port_read(space->machine, "options") & 0x02)
	{
		offset *= 2;
		if(ACCESSING_BITS_0_7)
			offset++;
		timer_set(space->machine, cpu_clocks_to_attotime(space->machine->cpu[0], 4), NULL, 0xbffffc+offset,x68k_fake_bus_error);
	}
	return 0xff;
}

static WRITE16_HANDLER( x68k_rom0_w )
{
	/* this location contains the address of some expansion device ROM, if no ROM exists,
       then access causes a bus error */
	current_vector[2] = 0x02;  // bus error
	current_irq_line = 2;
//	cpu_set_input_line_and_vector(space->machine->cpu[0],2,ASSERT_LINE,current_vector[2]);
	if(input_port_read(space->machine, "options") & 0x02)
	{
		offset *= 2;
		if(ACCESSING_BITS_0_7)
			offset++;
		timer_set(space->machine, cpu_clocks_to_attotime(space->machine->cpu[0], 4), NULL, 0xbffffc+offset,x68k_fake_bus_error);
	}
}

static READ16_HANDLER( x68k_exp_r )
{
	/* These are expansion devices, if not present, they cause a bus error */
	if(input_port_read(space->machine, "options") & 0x02)
	{
		current_vector[2] = 0x02;  // bus error
		current_irq_line = 2;
		offset *= 2;
		if(ACCESSING_BITS_0_7)
			offset++;
		timer_set(space->machine, cpu_clocks_to_attotime(space->machine->cpu[0], 16), NULL, 0xeafa00+offset,x68k_fake_bus_error);
//      cpu_set_input_line_and_vector(machine->cpu[0],2,ASSERT_LINE,current_vector[2]);
	}
	return 0xffff;
}

static WRITE16_HANDLER( x68k_exp_w )
{
	/* These are expansion devices, if not present, they cause a bus error */
	if(input_port_read(space->machine, "options") & 0x02)
	{
		current_vector[2] = 0x02;  // bus error
		current_irq_line = 2;
		offset *= 2;
		if(ACCESSING_BITS_0_7)
			offset++;
		timer_set(space->machine, cpu_clocks_to_attotime(space->machine->cpu[0], 16), NULL, 0xeafa00+offset,x68k_fake_bus_error);
//      cpu_set_input_line_and_vector(machine->cpu[0],2,ASSERT_LINE,current_vector[2]);
	}
}

static void x68k_dma_irq(running_machine *machine, int channel)
{
	const device_config *device = device_list_find_by_tag(machine->config->devicelist, HD63450, "hd63450");
	current_vector[3] = hd63450_get_vector(device, channel);
	current_irq_line = 3;
	logerror("DMA#%i: DMA End (vector 0x%02x)\n",channel,current_vector[3]);
	cpu_set_input_line_and_vector(machine->cpu[0],3,ASSERT_LINE,current_vector[3]);
}

static void x68k_dma_end(running_machine *machine, int channel,int irq)
{
	if(irq != 0)
	{
		x68k_dma_irq(machine, channel);
	}
}

static void x68k_dma_error(running_machine *machine, int channel, int irq)
{
	const device_config *device = device_list_find_by_tag(machine->config->devicelist, HD63450, "hd63450");
	if(irq != 0)
	{
		current_vector[3] = hd63450_get_error_vector(device,channel);
		current_irq_line = 3;
		cpu_set_input_line_and_vector(machine->cpu[0],3,ASSERT_LINE,current_vector[3]);
	}
}

static void x68k_fm_irq(running_machine *machine, int irq)
{
	if(irq == CLEAR_LINE)
	{
		sys.mfp.gpio |= 0x08;
	}
	else
	{
		sys.mfp.gpio &= ~0x08;
	}
}

static MC68901_GPIO_READ( mfp_gpio_r )
{
	UINT8 data = sys.mfp.gpio;

	data &= ~(sys.crtc.hblank << 7);
	data &= ~(sys.crtc.vblank << 4);
	data |= 0x23;  // GPIP5 is unused, always 1

//	mc68901_tai_w(mfp, sys.crtc.vblank);

	return data;
}

static MC68901_ON_IRQ_CHANGED( mfp_irq_callback )
{
	static int prev;
	if(prev == CLEAR_LINE && level == CLEAR_LINE)  // eliminate unnecessary calls to set the IRQ line for speed reasons
		return;
//	if((sys.ioc.irqstatus & 0xc0) != 0)  // if the FDC is busy, then we don't want to miss that IRQ
//		return;
	cpu_set_input_line(device->machine->cpu[0], 6, level);
	current_vector[6] = 0;
	prev = level;
}

static INTERRUPT_GEN( x68k_vsync_irq )
{
//  if(sys.mfp.ierb & 0x40)
//  {
//      sys.mfp.isrb |= 0x40;
//      current_vector[6] = (sys.mfp.vr & 0xf0) | 0x06;  // GPIP4 (V-DISP)
//      current_irq_line = 6;
//  mfp_timer_a_callback(0);  // Timer A is usually always in event count mode, and is tied to V-DISP
//  mfp_trigger_irq(MFP_IRQ_GPIP4);
//  }
//  if(sys.crtc.height == 256)
//      video_screen_update_partial(machine->primary_screen,256);//sys.crtc.reg[4]/2);
//  else
//      video_screen_update_partial(machine->primary_screen,512);//sys.crtc.reg[4]);
}

static IRQ_CALLBACK(x68k_int_ack)
{
	const device_config *x68k_mfp = device_list_find_by_tag(device->machine->config->devicelist, MC68901, MC68901_TAG);

	if(irqline == 6)  // MFP
	{
		sys.mfp.current_irq = -1;
		if(current_vector[6] != 0x4b && current_vector[6] != 0x4c)
			current_vector[6] = mc68901_get_vector(x68k_mfp);
		else
			cpu_set_input_line_and_vector(device->machine->cpu[0],irqline,CLEAR_LINE,current_vector[irqline]);
		logerror("SYS: IRQ acknowledged (vector=0x%02x, line = %i)\n",current_vector[6],irqline);
		return current_vector[6];
	}
	
	cpu_set_input_line_and_vector(device->machine->cpu[0],irqline,CLEAR_LINE,current_vector[irqline]);
	if(irqline == 1)  // IOSC
	{
		sys.ioc.irqstatus &= ~0xf0;
	}
	if(irqline == 5)  // SCC
	{
		sys.mouse.irqactive = 0;
	}

	logerror("SYS: IRQ acknowledged (vector=0x%02x, line = %i)\n",current_vector[irqline],irqline);
	return current_vector[irqline];
}

static ADDRESS_MAP_START(x68k_map, ADDRESS_SPACE_PROGRAM, 16)
//  AM_RANGE(0x000000, 0xbfffff) AM_RAMBANK(1)
	AM_RANGE(0xbffffc, 0xbfffff) AM_READWRITE(x68k_rom0_r, x68k_rom0_w)
//  AM_RANGE(0xc00000, 0xdfffff) AM_READWRITE(x68k_gvram_r, x68k_gvram_w) AM_BASE(&gvram)
//  AM_RANGE(0xe00000, 0xe7ffff) AM_READWRITE(x68k_tvram_r, x68k_tvram_w) AM_BASE(&tvram)
	AM_RANGE(0xc00000, 0xdfffff) AM_RAMBANK(2)
	AM_RANGE(0xe00000, 0xe7ffff) AM_RAMBANK(3)
	AM_RANGE(0xe80000, 0xe81fff) AM_READWRITE(x68k_crtc_r, x68k_crtc_w)
	AM_RANGE(0xe82000, 0xe83fff) AM_READWRITE(x68k_vid_r, x68k_vid_w)
	AM_RANGE(0xe84000, 0xe85fff) AM_READWRITE(x68k_dmac_r, x68k_dmac_w)
	AM_RANGE(0xe86000, 0xe87fff) AM_READWRITE(x68k_areaset_r, x68k_areaset_w)
	AM_RANGE(0xe88000, 0xe89fff) AM_READWRITE(x68k_mfp_r, x68k_mfp_w)
	AM_RANGE(0xe8a000, 0xe8bfff) AM_READWRITE(x68k_rtc_r, x68k_rtc_w)
//  AM_RANGE(0xe8c000, 0xe8dfff) AM_READWRITE(x68k_printer_r, x68k_printer_w)
	AM_RANGE(0xe8e000, 0xe8ffff) AM_READWRITE(x68k_sysport_r, x68k_sysport_w)
	AM_RANGE(0xe90000, 0xe91fff) AM_READWRITE(x68k_fm_r, x68k_fm_w)
	AM_RANGE(0xe92000, 0xe92001) AM_READWRITE(okim6258_status_0_lsb_r, okim6258_ctrl_0_lsb_w)
	AM_RANGE(0xe92002, 0xe92003) AM_READWRITE(okim6258_data_0_lsb_r, okim6258_data_0_lsb_w)
	AM_RANGE(0xe94000, 0xe95fff) AM_READWRITE(x68k_fdc_r, x68k_fdc_w)
	AM_RANGE(0xe96000, 0xe97fff) AM_DEVREADWRITE(X68KHDC,"x68k_hdc",x68k_hdc_r, x68k_hdc_w)
	AM_RANGE(0xe98000, 0xe99fff) AM_READWRITE(x68k_scc_r, x68k_scc_w)
	AM_RANGE(0xe9a000, 0xe9bfff) AM_DEVREADWRITE(PPI8255, "ppi8255", x68k_ppi_r, x68k_ppi_w)
	AM_RANGE(0xe9c000, 0xe9dfff) AM_READWRITE(x68k_ioc_r, x68k_ioc_w)
	AM_RANGE(0xeafa00, 0xeafa1f) AM_READWRITE(x68k_exp_r, x68k_exp_w)
	AM_RANGE(0xeafa80, 0xeafa89) AM_READWRITE(x68k_areaset_r, x68k_enh_areaset_w)
	AM_RANGE(0xeb0000, 0xeb7fff) AM_READWRITE(x68k_spritereg_r, x68k_spritereg_w)
	AM_RANGE(0xeb8000, 0xebffff) AM_READWRITE(x68k_spriteram_r, x68k_spriteram_w)
	AM_RANGE(0xec0000, 0xecffff) AM_NOP  // User I/O
//  AM_RANGE(0xed0000, 0xed3fff) AM_READWRITE(x68k_sram_r, x68k_sram_w) AM_BASE(&generic_nvram16) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0xed0000, 0xed3fff) AM_RAMBANK(4) AM_BASE(&generic_nvram16) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0xed4000, 0xefffff) AM_NOP
	AM_RANGE(0xf00000, 0xffffff) AM_ROM
ADDRESS_MAP_END

static const mc68901_interface mfp_interface =
{
	2000000, // 4MHz clock
	4000000,
	MC68901_TDO_LOOPBACK,
	0,
	&mfp_key,  // Rx
	NULL,      // Tx
	NULL,
	mfp_irq_callback,
	mfp_gpio_r,
	NULL
};

static const ppi8255_interface ppi_interface =
{
	ppi_port_a_r,
	ppi_port_b_r,
	ppi_port_c_r,
	NULL,
	NULL,
	ppi_port_c_w
};

static const hd63450_intf dmac_interface =
{
	0,  // CPU - 68000
	{STATIC_ATTOTIME_IN_USEC(32),STATIC_ATTOTIME_IN_NSEC(450),STATIC_ATTOTIME_IN_USEC(4),STATIC_ATTOTIME_IN_HZ(15625/2)},  // Cycle steal mode timing (guesstimate)
	{STATIC_ATTOTIME_IN_USEC(32),STATIC_ATTOTIME_IN_NSEC(450),STATIC_ATTOTIME_IN_NSEC(50),STATIC_ATTOTIME_IN_NSEC(50)}, // Burst mode timing (guesstimate)
	x68k_dma_end,
	x68k_dma_error,
	{ x68k_fdc_read_byte, 0, 0, 0 },
	{ x68k_fdc_write_byte, 0, 0, 0 }
//  { 0, 0, 0, 0 },
//  { 0, 0, 0, 0 }
};

static const nec765_interface fdc_interface =
{
	fdc_irq,
	fdc_drq,
	NULL,
	NEC765_RDY_PIN_CONNECTED
};

static const ym2151_interface x68k_ym2151_interface =
{
	x68k_fm_irq,
	x68k_ct_w  // CT1, CT2 from YM2151 port 0x1b
};

static const okim6258_interface x68k_okim6258_interface =
{
	FOSC_DIV_BY_512,
	TYPE_4BITS,
	OUTPUT_10BITS,
};

static const struct rp5c15_interface rtc_intf =
{
	x68k_rtc_alarm_irq
};

static INPUT_PORTS_START( x68000 )
	PORT_START( "joy1" )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_CODE(JOYCODE_Y_UP_SWITCH)	 PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_CODE(JOYCODE_Y_DOWN_SWITCH)	 PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_CODE(JOYCODE_X_LEFT_SWITCH)	 PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_CODE(JOYCODE_X_RIGHT_SWITCH)	 PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_CODE(JOYCODE_BUTTON1)	 PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_CODE(JOYCODE_BUTTON2)	 PORT_PLAYER(1)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START( "joy2" )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_CODE(JOYCODE_Y_UP_SWITCH)	 PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_CODE(JOYCODE_Y_DOWN_SWITCH)	 PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_CODE(JOYCODE_X_LEFT_SWITCH)	 PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_CODE(JOYCODE_X_RIGHT_SWITCH)	 PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_CODE(JOYCODE_BUTTON1)	 PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_CODE(JOYCODE_BUTTON2)	 PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START( "key1" )
	PORT_BIT( 0x00000001, IP_ACTIVE_HIGH, IPT_UNUSED) // unused
	PORT_BIT( 0x00000002, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("ESC") PORT_CODE(KEYCODE_ESC) PORT_CHAR(27)  /* ESC */
	PORT_BIT( 0x00000004, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("1  !  \xE3\x83\x8C") PORT_CODE(KEYCODE_1)  PORT_CHAR('1') PORT_CHAR('!') /* 1 ! */
	PORT_BIT( 0x00000008, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("2  \"  \xE3\x83\x95") PORT_CODE(KEYCODE_2)  PORT_CHAR('2') PORT_CHAR('\"') /* 2 " */
	PORT_BIT( 0x00000010, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("3  #  \xE3\x82\xA2  \xE3\x82\xA1") PORT_CODE(KEYCODE_3)  PORT_CHAR('3') PORT_CHAR('#') /* 3 # */
	PORT_BIT( 0x00000020, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("4  $  \xE3\x82\xA6  \xE3\x82\xA5") PORT_CODE(KEYCODE_4)  PORT_CHAR('4') PORT_CHAR('$') /* 4 $ */
	PORT_BIT( 0x00000040, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("5  %  \xE3\x82\xA8  \xE3\x82\xA7") PORT_CODE(KEYCODE_5)  PORT_CHAR('5') PORT_CHAR('%') /* 5 % */
	PORT_BIT( 0x00000080, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("6  &  \xE3\x82\xAA  \xE3\x82\xA9") PORT_CODE(KEYCODE_6)  PORT_CHAR('6') PORT_CHAR('&') /* 6 & */
	PORT_BIT( 0x00000100, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("7  \'  \xE3\x83\xA4  \xE3\x83\xA3") PORT_CODE(KEYCODE_7)  PORT_CHAR('7') PORT_CHAR('\'') /* 7 ' */
	PORT_BIT( 0x00000200, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("8  (  \xE3\x83\xA6  \xE3\x83\xA5") PORT_CODE(KEYCODE_8)  PORT_CHAR('8') PORT_CHAR('(') /* 8 ( */
	PORT_BIT( 0x00000400, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("9  )  \xE3\x83\xA8  \xE3\x83\xA7") PORT_CODE(KEYCODE_9)  PORT_CHAR('9') PORT_CHAR(')') /* 9 ) */
	PORT_BIT( 0x00000800, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("0  \xE3\x83\xAF  \xE3\x83\xB2") PORT_CODE(KEYCODE_0)  PORT_CHAR('0')                /* 0 */
	PORT_BIT( 0x00001000, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("-  =  \xE3\x83\x9B") PORT_CODE(KEYCODE_MINUS)  PORT_CHAR('-') PORT_CHAR('=') /* - = */
	PORT_BIT( 0x00002000, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("^  \xE3\x83\x98") PORT_CHAR('^') /* ^ */
	PORT_BIT( 0x00004000, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("\xC2\xA5  \xE3\x83\xBC  |") PORT_CODE(KEYCODE_BACKSLASH)  PORT_CHAR('\\') PORT_CHAR('|') /* Yen | */
	PORT_BIT( 0x00008000, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_CODE(KEYCODE_BACKSPACE)  PORT_CHAR(8) /* Backspace */
	PORT_BIT( 0x00010000, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_CODE(KEYCODE_TAB)  PORT_CHAR(9)  /* Tab */
	PORT_BIT( 0x00020000, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("Q  \xE3\x82\xBF") PORT_CODE(KEYCODE_Q)  PORT_CHAR('Q')  /* Q */
	PORT_BIT( 0x00040000, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("W  \xE3\x83\x86") PORT_CODE(KEYCODE_W)  PORT_CHAR('W')  /* W */
	PORT_BIT( 0x00080000, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("E  \xE3\x82\xA4  \xE3\x82\xA3") PORT_CODE(KEYCODE_E)  PORT_CHAR('E')  /* E */
	PORT_BIT( 0x00100000, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("R  \xE3\x82\xB9") PORT_CODE(KEYCODE_R)  PORT_CHAR('R')  /* R */
	PORT_BIT( 0x00200000, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("T  \xE3\x82\xAB") PORT_CODE(KEYCODE_T)  PORT_CHAR('T')  /* T */
	PORT_BIT( 0x00400000, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("Y  \xE3\x83\xB3") PORT_CODE(KEYCODE_Y)  PORT_CHAR('Y')  /* Y */
	PORT_BIT( 0x00800000, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("U  \xE3\x83\x8A") PORT_CODE(KEYCODE_U)  PORT_CHAR('U')  /* U */
	PORT_BIT( 0x01000000, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("I  \xE3\x83\x8B") PORT_CODE(KEYCODE_I)  PORT_CHAR('I')  /* I */
	PORT_BIT( 0x02000000, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("O  \xE3\x83\xA9") PORT_CODE(KEYCODE_O)  PORT_CHAR('O')  /* O */
	PORT_BIT( 0x04000000, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("P  \xE3\x82\xBB") PORT_CODE(KEYCODE_P)  PORT_CHAR('P')  /* P */
	PORT_BIT( 0x08000000, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("@  `  \xE3\x82\x9B") PORT_CHAR('@') PORT_CHAR('`')  /* @ */
	PORT_BIT( 0x10000000, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("[  {  \xE3\x82\x9C \xE3\x80\x8C") PORT_CODE(KEYCODE_OPENBRACE)  PORT_CHAR('[') PORT_CHAR('{')  /* [ { */
	PORT_BIT( 0x20000000, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_CODE(KEYCODE_ENTER)  PORT_CHAR(13)  /* Return */
	PORT_BIT( 0x40000000, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("A  \xE3\x83\x81") PORT_CODE(KEYCODE_A)  PORT_CHAR('A')  /* A */
	PORT_BIT( 0x80000000, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("S  \xE3\x83\x88") PORT_CODE(KEYCODE_S)  PORT_CHAR('S')  /* S */

	PORT_START( "key2" )
	PORT_BIT( 0x00000001, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("D  \xE3\x82\xB7") PORT_CODE(KEYCODE_D)  PORT_CHAR('D')  /* D */
	PORT_BIT( 0x00000002, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("F  \xE3\x83\x8F") PORT_CODE(KEYCODE_F)  PORT_CHAR('F')  /* F */
	PORT_BIT( 0x00000004, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("G  \xE3\x82\xAD") PORT_CODE(KEYCODE_G)  PORT_CHAR('G')  /* G */
	PORT_BIT( 0x00000008, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("H  \xE3\x82\xAF") PORT_CODE(KEYCODE_H)  PORT_CHAR('H')  /* H */
	PORT_BIT( 0x00000010, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("J  \xE3\x83\x9E") PORT_CODE(KEYCODE_J)  PORT_CHAR('J')  /* J */
	PORT_BIT( 0x00000020, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("K  \xE3\x83\x8E") PORT_CODE(KEYCODE_K)  PORT_CHAR('K')  /* K */
	PORT_BIT( 0x00000040, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("L  \xE3\x83\xAA") PORT_CODE(KEYCODE_L)  PORT_CHAR('L')  /* L */
	PORT_BIT( 0x00000080, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME(";  +  \xE3\x83\xAC") PORT_CODE(KEYCODE_COLON)  PORT_CHAR(';')  PORT_CHAR('+')  /* ; + */
	PORT_BIT( 0x00000100, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME(":  *  \xE3\x82\xB1") PORT_CODE(KEYCODE_QUOTE)  PORT_CHAR(':')  PORT_CHAR('*')  /* : * */
	PORT_BIT( 0x00000200, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("]  }  \xE3\x83\xA0  \xE3\x80\x8D") PORT_CODE(KEYCODE_CLOSEBRACE)  PORT_CHAR(']')  PORT_CHAR('}')  /* ] } */
	PORT_BIT( 0x00000400, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("Z  \xE3\x83\x84  \xE3\x83\x83") PORT_CODE(KEYCODE_Z)  PORT_CHAR('Z')  /* Z */
	PORT_BIT( 0x00000800, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("X  \xE3\x82\xB5") PORT_CODE(KEYCODE_X)  PORT_CHAR('X')  /* X */
	PORT_BIT( 0x00001000, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("C  \xE3\x82\xBD") PORT_CODE(KEYCODE_C)  PORT_CHAR('C')  /* C */
	PORT_BIT( 0x00002000, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("V  \xE3\x83\x92") PORT_CODE(KEYCODE_V)  PORT_CHAR('V')  /* V */
	PORT_BIT( 0x00004000, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("B  \xE3\x82\xB3") PORT_CODE(KEYCODE_B)  PORT_CHAR('B')  /* B */
	PORT_BIT( 0x00008000, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("N  \xE3\x83\x9F") PORT_CODE(KEYCODE_N)  PORT_CHAR('N')  /* N */
	PORT_BIT( 0x00010000, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("M  \xE3\x83\xA2") PORT_CODE(KEYCODE_M)  PORT_CHAR('M')  /* M */
	PORT_BIT( 0x00020000, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME(",  <  \xE3\x83\x8D  \xE3\x80\x81") PORT_CODE(KEYCODE_COMMA)  PORT_CHAR(',')  PORT_CHAR('<')  /* , < */
	PORT_BIT( 0x00040000, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME(".  >  \xE3\x83\xAB  \xE3\x80\x82") PORT_CODE(KEYCODE_STOP)  PORT_CHAR('.')  PORT_CHAR('>')  /* . > */
	PORT_BIT( 0x00080000, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("/  ?  \xE3\x83\xA1  \xE3\x83\xBB") PORT_CODE(KEYCODE_SLASH)  PORT_CHAR('/')  PORT_CHAR('?')  /* / ? */
	PORT_BIT( 0x00100000, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("_  \xE3\x83\xAD") PORT_CHAR('_')  /* Underscore (shifted only?) */
	PORT_BIT( 0x00200000, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("Space")  PORT_CODE(KEYCODE_SPACE)  PORT_CHAR(' ')  /* Space */
	PORT_BIT( 0x00400000, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("Home")  PORT_CODE(KEYCODE_HOME)  /* Home */
	PORT_BIT( 0x00800000, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("Delete")  PORT_CODE(KEYCODE_DEL)  /* Del */
	PORT_BIT( 0x01000000, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("Roll Up")  PORT_CODE(KEYCODE_PGUP)  /* Roll Up */
	PORT_BIT( 0x02000000, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("Roll Down")  PORT_CODE(KEYCODE_PGDN)  /* Roll Down */
	PORT_BIT( 0x04000000, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("Undo")  PORT_CODE(KEYCODE_END)  /* Undo */
	PORT_BIT( 0x08000000, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("Cursor Left")  PORT_CODE(KEYCODE_LEFT)  /* Left */
	PORT_BIT( 0x10000000, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("Cursor Up")  PORT_CODE(KEYCODE_UP)  /* Up */
	PORT_BIT( 0x20000000, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("Cursor Right")  PORT_CODE(KEYCODE_RIGHT)  /* Right */
	PORT_BIT( 0x40000000, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("Cursor Down")  PORT_CODE(KEYCODE_DOWN)  /* Down */
	PORT_BIT( 0x80000000, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("Tenkey CLR")  PORT_CODE(KEYCODE_NUMLOCK)  /* CLR */

	PORT_START( "key3" )
	PORT_BIT( 0x00000001, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("Tenkey /")  PORT_CODE(KEYCODE_SLASH_PAD)  /* / (numpad) */
	PORT_BIT( 0x00000002, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("Tenkey *")  PORT_CODE(KEYCODE_ASTERISK)  /* * (numpad) */
	PORT_BIT( 0x00000004, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("Tenkey -")  PORT_CODE(KEYCODE_MINUS_PAD)  /* - (numpad) */
	PORT_BIT( 0x00000008, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("Tenkey 7")  PORT_CODE(KEYCODE_7_PAD)  /* 7 (numpad) */
	PORT_BIT( 0x00000010, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("Tenkey 8")  PORT_CODE(KEYCODE_8_PAD)  /* 8 (numpad) */
	PORT_BIT( 0x00000020, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("Tenkey 9")  PORT_CODE(KEYCODE_9_PAD)  /* 9 (numpad) */
	PORT_BIT( 0x00000040, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("Tenkey +")  PORT_CODE(KEYCODE_PLUS_PAD)  /* + (numpad) */
	PORT_BIT( 0x00000080, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("Tenkey 4")  PORT_CODE(KEYCODE_4_PAD)  /* 4 (numpad) */
	PORT_BIT( 0x00000100, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("Tenkey 5")  PORT_CODE(KEYCODE_5_PAD)  /* 5 (numpad) */
	PORT_BIT( 0x00000200, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("Tenkey 6")  PORT_CODE(KEYCODE_6_PAD)  /* 6 (numpad) */
	PORT_BIT( 0x00000400, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("Tenkey =")  /* = (numpad) */
	PORT_BIT( 0x00000800, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("Tenkey 1")  PORT_CODE(KEYCODE_1_PAD)  /* 1 (numpad) */
	PORT_BIT( 0x00001000, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("Tenkey 2")  PORT_CODE(KEYCODE_2_PAD)  /* 2 (numpad) */
	PORT_BIT( 0x00002000, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("Tenkey 3")  PORT_CODE(KEYCODE_3_PAD)  /* 3 (numpad) */
	PORT_BIT( 0x00004000, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("Tenkey Enter")  PORT_CODE(KEYCODE_ENTER_PAD)  /* Enter (numpad) */
	PORT_BIT( 0x00008000, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("Tenkey 0")  PORT_CODE(KEYCODE_0_PAD)  /* 0 (numpad) */
	PORT_BIT( 0x00010000, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("Tenkey ,")  /* , (numpad) */
	PORT_BIT( 0x00020000, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("Tenkey .")  PORT_CODE(KEYCODE_DEL_PAD)  /* 2 (numpad) */
	PORT_BIT( 0x00040000, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("\xE8\xA8\x98\xE5\x8F\xB7 (Symbolic input)")  /* Sign / Symbolic input (babelfish translation) */
	PORT_BIT( 0x00080000, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("\xE7\x99\xBB\xE9\x8C\xB2 (Register)")  /* Register (babelfish translation) */
	PORT_BIT( 0x00100000, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("Help")  /* Help */
	PORT_BIT( 0x00200000, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("XF1")  PORT_CODE(KEYCODE_F11)  /* XF1 */
	PORT_BIT( 0x00400000, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("XF2")  PORT_CODE(KEYCODE_F12)  /* XF2 */
	PORT_BIT( 0x00800000, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("XF3")  /* XF3 */
	PORT_BIT( 0x01000000, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("XF4")  /* XF4 */
	PORT_BIT( 0x02000000, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("XF5")  /* XF5 */
	PORT_BIT( 0x04000000, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("\xe3\x81\x8b\xe3\x81\xaa (Kana)")  /* Kana */
	PORT_BIT( 0x08000000, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("\xe3\x83\xad\xe3\x83\xbc\xe3\x83\x9e\xe5\xad\x97 (Romaji)")  /* Romaji */
	PORT_BIT( 0x10000000, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("\xE3\x82\xB3\xE3\x83\xBC\xE3\x83\x89 (Code input)")  /* Code input */
	PORT_BIT( 0x20000000, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("Caps")  PORT_CODE(KEYCODE_CAPSLOCK)  /* Caps lock */
	PORT_BIT( 0x40000000, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("Insert")  PORT_CODE(KEYCODE_INSERT)  /* Insert */
	PORT_BIT( 0x80000000, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("\xE3\x81\xB2\xE3\x82\x89\xE3\x81\x8C\xE3\x81\xAA (Hiragana)")  /* Hiragana */

	PORT_START( "key4" )
	PORT_BIT( 0x00000001, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("\xE5\x85\xA8\xE8\xA7\x92 (Full size)")  /* Full size (babelfish translation) */
	PORT_BIT( 0x00000002, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("Break")  /* Break */
	PORT_BIT( 0x00000004, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("Copy")  /* Copy */
	PORT_BIT( 0x00000008, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("F1")  PORT_CODE(KEYCODE_F1)  /* F1 */
	PORT_BIT( 0x00000010, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("F2")  PORT_CODE(KEYCODE_F2)  /* F2 */
	PORT_BIT( 0x00000020, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("F3")  PORT_CODE(KEYCODE_F3)  /* F3 */
	PORT_BIT( 0x00000040, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("F4")  PORT_CODE(KEYCODE_F4)  /* F4 */
	PORT_BIT( 0x00000080, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("F5")  PORT_CODE(KEYCODE_F5)  /* F5 */
	PORT_BIT( 0x00000100, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("F6")  PORT_CODE(KEYCODE_F6)  /* F6 */
	PORT_BIT( 0x00000200, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("F7")  PORT_CODE(KEYCODE_F7)  /* F7 */
	PORT_BIT( 0x00000400, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("F8")  PORT_CODE(KEYCODE_F8)  /* F8 */
	PORT_BIT( 0x00000800, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("F9")  PORT_CODE(KEYCODE_F9)  /* F9 */
	PORT_BIT( 0x00001000, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("F10")  PORT_CODE(KEYCODE_F10)  /* F10 */
		// 0x6d reserved
		// 0x6e reserved
		// 0x6f reserved
	PORT_BIT( 0x00010000, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("Shift")  PORT_CODE(KEYCODE_LSHIFT)  /* Shift */
	PORT_BIT( 0x00020000, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("Ctrl")  PORT_CODE(KEYCODE_LCONTROL)  /* Ctrl */
	PORT_BIT( 0x00040000, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("Opt. 1")  PORT_CODE(KEYCODE_PRTSCR) /* Opt1 */
	PORT_BIT( 0x00080000, IP_ACTIVE_HIGH, IPT_KEYBOARD )  PORT_NAME("Opt. 2")  PORT_CODE(KEYCODE_PAUSE)  /* Opt2 */

	PORT_START("options")
	PORT_CONFNAME( 0x01, 0x01, "Enable partial updates on raster IRQ")
	PORT_CONFSETTING(	0x00, DEF_STR( Off ))
	PORT_CONFSETTING(	0x01, DEF_STR( On ))
	PORT_CONFNAME( 0x02, 0x02, "Enable fake bus errors")
	PORT_CONFSETTING(	0x00, DEF_STR( Off ))
	PORT_CONFSETTING(	0x02, DEF_STR( On ))
	PORT_CONFNAME( 0x04, 0x04, "Enable partial updates on each HSync")
	PORT_CONFSETTING(	0x00, DEF_STR( Off ))
	PORT_CONFSETTING(	0x04, DEF_STR( On ))

	PORT_START("mouse1")  // mouse buttons
	PORT_BIT( 0x00000001, IP_ACTIVE_HIGH, IPT_BUTTON9) PORT_NAME("Left mouse button") PORT_CODE(MOUSECODE_BUTTON1)
	PORT_BIT( 0x00000002, IP_ACTIVE_HIGH, IPT_BUTTON10) PORT_NAME("Right mouse button") PORT_CODE(MOUSECODE_BUTTON2)

	PORT_START("mouse2")  // X-axis
	PORT_BIT( 0xff, 0x00, IPT_MOUSE_X) PORT_SENSITIVITY(100) PORT_KEYDELTA(0) PORT_PLAYER(1)

	PORT_START("mouse3")  // Y-axis
	PORT_BIT( 0xff, 0x00, IPT_MOUSE_Y) PORT_SENSITIVITY(100) PORT_KEYDELTA(0) PORT_PLAYER(1)

INPUT_PORTS_END

static void dimdsk_set_geometry(const device_config* image)
{
	// DIM disk image header, most of this is guesswork
	int tracks = 77;
	int heads = 2;
	int sectors = 8;  // per track
	int sectorlen = 1024;
	int firstsector = 0x01;
	unsigned char format;
	int x;
	unsigned short temp;

	/* Offset + 0 : disk format type (1 byte):
     * 0 = 2HD / 2HDA (8 sector/track, 1024 bytes/sector, GAP#3 = 0x74)
     * 1 = 2HS        (9 sector/track, 1024 bytes/sector, GAP#3 = 0x39)
     * 2 = 2HC        (15 sector/track, 512 bytes/sector, GAP#3 = 0x54)
     * 3 = 2HDE(68)   (9 sector/track, 1024 bytes/sector, GAP#3 = 0x39)
     * 9 = 2HQ        (18 sector/track, 512 bytes/sector, GAP#3 = 0x54)
     * 17 = N88-BASIC (26 sector/track, 256 bytes/sector, GAP#3 = 0x33)
     *             or (26 sector/track, 128 bytes/sector, GAP#3 = 0x1a)
     */
	image_fread(image,&format,1);

	switch(format)
	{
	case 0x00:
		sectors = 8;
		sectorlen = 1024;
		break;
	case 0x01:
	case 0x03:
		sectors = 9;
		sectorlen = 1024;
		break;
	case 0x02:
		sectors = 15;
		sectorlen = 512;
		break;
	case 0x09:
		sectors = 18;
		sectorlen = 512;
		break;
	case 0x11:
		sectors = 26;
		sectorlen = 256;
		break;
	}
	tracks = 0;
	for (x=0;x<86;x++)
	{
		image_fread(image,&temp,2);
		if(temp == 0x0101)
			tracks++;
	}
	// TODO: expand on this basic implementation

	logerror("FDD: DIM image loaded - type %i, %i tracks, %i sectors per track, %i bytes per sector\n", format,tracks, sectors,sectorlen);
	basicdsk_set_geometry(image, tracks+1, heads, sectors, sectorlen, firstsector, 0x100, FALSE);
}

static DEVICE_IMAGE_LOAD( x68k_floppy )
{
	if (device_load_basicdsk_floppy(image)==INIT_PASS)
	{
		/* sector id's 01-08 */
		/* drive, tracks, heads, sectors per track, sector length, first sector id */
		if(strcmp(image_filetype(image),"dim") == 0)
			dimdsk_set_geometry(image);
		else
			basicdsk_set_geometry(image, 77, 2, 8, 1024, 0x01, 0, FALSE);
		if(sys.ioc.irqstatus & 0x02)
		{
			current_vector[1] = 0x61;
			sys.ioc.irqstatus |= 0x40;
			current_irq_line = 1;
			cpu_set_input_line_and_vector(image->machine->cpu[0],1,ASSERT_LINE,current_vector[1]);  // Disk insert/eject interrupt
			logerror("IOC: Disk image inserted\n");
		}
		sys.fdc.disk_inserted[image_index_in_device(image)] = 1;
		return INIT_PASS;
	}

	return INIT_FAIL;
}

static DEVICE_IMAGE_UNLOAD( x68k_floppy )
{
	if(sys.ioc.irqstatus & 0x02)
	{
		current_vector[1] = 0x61;
		sys.ioc.irqstatus |= 0x40;
		current_irq_line = 1;
		cpu_set_input_line_and_vector(image->machine->cpu[0],1,ASSERT_LINE,current_vector[1]);  // Disk insert/eject interrupt
	}
	sys.fdc.disk_inserted[image_index_in_device(image)] = 0;
}

static void x68k_floppy_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
	case MESS_DEVINFO_INT_READABLE:
		info->i = 1;
		break;
	case MESS_DEVINFO_INT_WRITEABLE:
		info->i = 1;
		break;
	case MESS_DEVINFO_INT_CREATABLE:
		info->i = 0;
		break;
	case MESS_DEVINFO_INT_COUNT:
		info->i = 4;
		break;
	case MESS_DEVINFO_PTR_LOAD:
		info->load = DEVICE_IMAGE_LOAD_NAME(x68k_floppy);
		break;
	case MESS_DEVINFO_PTR_UNLOAD:
		info->unload = DEVICE_IMAGE_UNLOAD_NAME(x68k_floppy);
		break;
	case MESS_DEVINFO_STR_FILE_EXTENSIONS:
		strcpy(info->s = device_temp_str(), "xdf,hdm,2hd,dim");
		break;
	default:
		legacybasicdsk_device_getinfo(devclass, state, info);
		break;
	}
}

static MACHINE_RESET( x68000 )
{
	/* The last half of the IPLROM is mapped to 0x000000 on reset only
       Just copying the inital stack pointer and program counter should
       more or less do the same job */

	int drive;
	UINT8* romdata = memory_region(machine, "user2");
	attotime irq_time;

	memset(mess_ram,0,mess_ram_size);
	memcpy(mess_ram,romdata,8);

	// init keyboard
	sys.keyboard.delay = 500;  // 3*100+200
	sys.keyboard.repeat = 110;  // 4^2*5+30

	// check for disks
	for(drive=0;drive<4;drive++)
	{
		if(image_exists(image_from_devtype_and_index(machine, IO_FLOPPY,drive)))
			sys.fdc.disk_inserted[drive] = 1;
		else
			sys.fdc.disk_inserted[drive] = 0;
	}

	// initialise CRTC, set registers to defaults for the standard text mode (768x512)
	sys.crtc.reg[0] = 137;  // Horizontal total  (in characters)
	sys.crtc.reg[1] = 14;   // Horizontal sync end
	sys.crtc.reg[2] = 28;   // Horizontal start
	sys.crtc.reg[3] = 124;  // Horizontal end
	sys.crtc.reg[4] = 567;  // Vertical total
	sys.crtc.reg[5] = 5;    // Vertical sync end
	sys.crtc.reg[6] = 40;   // Vertical start
	sys.crtc.reg[7] = 552;  // Vertical end
	sys.crtc.reg[8] = 27;   // Horizontal adjust

	mfp_init();

	x68k_scanline = video_screen_get_vpos(machine->primary_screen);// = sys.crtc.reg[6];  // Vertical start

	// start VBlank timer
	sys.crtc.vblank = 1;
	irq_time = video_screen_get_time_until_pos(machine->primary_screen,sys.crtc.reg[6],2);
	timer_adjust_oneshot(vblank_irq, irq_time, 0);

	// start HBlank timer
	timer_adjust_oneshot(scanline_timer, video_screen_get_scan_period(machine->primary_screen), 1);

	sys.mfp.gpio = 0xfb;

	// reset output values
	output_set_value("key_led_kana",1);
	output_set_value("key_led_romaji",1);
	output_set_value("key_led_code",1);
	output_set_value("key_led_caps",1);
	output_set_value("key_led_insert",1);
	output_set_value("key_led_hiragana",1);
	output_set_value("key_led_fullsize",1);
	for(drive=0;drive<4;drive++)
	{
		output_set_indexed_value("eject_drv",drive,1);
		output_set_indexed_value("ctrl_drv",drive,1);
		output_set_indexed_value("access_drv",drive,1);
	}
	
	// reset CPU
	device_reset(machine->cpu[0]);
}

static MACHINE_START( x68000 )
{
	const address_space *space = cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM);
	/*  Install RAM handlers  */
	x68k_spriteram = (UINT16*)memory_region(machine, "user1");
	memory_install_read16_handler(space,0x000000,mess_ram_size-1,mess_ram_size-1,0,(read16_space_func)1);
	memory_install_write16_handler(space,0x000000,mess_ram_size-1,mess_ram_size-1,0,(write16_space_func)1);
	memory_set_bankptr(machine, 1,mess_ram);
	memory_install_read16_handler(space,0xc00000,0xdfffff,0x1fffff,0,(read16_space_func)x68k_gvram_r);
	memory_install_write16_handler(space,0xc00000,0xdfffff,0x1fffff,0,(write16_space_func)x68k_gvram_w);
	memory_set_bankptr(machine, 2,gvram);  // so that code in VRAM is executable - needed for Terra Cresta
	memory_install_read16_handler(space,0xe00000,0xe7ffff,0x07ffff,0,(read16_space_func)x68k_tvram_r);
	memory_install_write16_handler(space,0xe00000,0xe7ffff,0x07ffff,0,(write16_space_func)x68k_tvram_w);
	memory_set_bankptr(machine, 3,tvram);  // so that code in VRAM is executable - needed for Terra Cresta
	memory_install_read16_handler(space,0xed0000,0xed3fff,0x003fff,0,(read16_space_func)x68k_sram_r);
	memory_install_write16_handler(space,0xed0000,0xed3fff,0x003fff,0,(write16_space_func)x68k_sram_w);
	memory_set_bankptr(machine, 4,generic_nvram16);  // so that code in SRAM is executable, there is an option for booting from SRAM

	// start keyboard timer
	timer_adjust_periodic(kb_timer, attotime_zero, 0, ATTOTIME_IN_MSEC(5));  // every 5ms

	// start mouse timer
	timer_adjust_periodic(mouse_timer, attotime_zero, 0, ATTOTIME_IN_MSEC(1));  // a guess for now
	sys.mouse.inputtype = 0;

	// start LED timer
	timer_adjust_periodic(led_timer, attotime_zero, 0, ATTOTIME_IN_MSEC(400));
}

static DRIVER_INIT( x68000 )
{
	unsigned char* rom = memory_region(machine, "main");
	unsigned char* user2 = memory_region(machine, "user2");
	gvram = auto_malloc(0x200000);
	memset(gvram,0,0x200000);
	tvram = auto_malloc(0x080000);
	memset(tvram,0,0x80000);
	sram = auto_malloc(0x4000);
	memset(sram,0,0x4000);

	x68k_spritereg = auto_malloc(0x8000);
	memset(x68k_spritereg,0,0x8000);

#ifdef USE_PREDEFINED_SRAM
	{
		unsigned char* ramptr = memory_region(machine, "user3");
		memcpy(sram,ramptr,0x4000);
	}
#endif

	// copy last half of BIOS to a user region, to use for inital startup
	memcpy(user2,(rom+0xff0000),0x10000);

	memset(&sys,0,sizeof(sys));

	mfp_init();
	rp5c15_init(machine, &rtc_intf);

	cpu_set_irq_callback(machine->cpu[0], x68k_int_ack);

	// init keyboard
	sys.keyboard.delay = 500;  // 3*100+200
	sys.keyboard.repeat = 110;  // 4^2*5+30
	kb_timer = timer_alloc(machine, x68k_keyboard_poll,NULL);
	scanline_timer = timer_alloc(machine, x68k_hsync,NULL);
	raster_irq = timer_alloc(machine, x68k_crtc_raster_irq,NULL);
	vblank_irq = timer_alloc(machine, x68k_crtc_vblank_irq,NULL);
	mouse_timer = timer_alloc(machine, x68k_scc_ack,NULL);
	led_timer = timer_alloc(machine, x68k_led_callback,NULL);
}

static const scc8530_interface x68000_scc8530_interface =
{
	NULL
};

static MACHINE_DRIVER_START( x68000 )
	/* basic machine hardware */
	MDRV_CPU_ADD("main", M68000, 10000000)  /* 10 MHz */
	MDRV_CPU_PROGRAM_MAP(x68k_map, 0)
	MDRV_CPU_VBLANK_INT("main", x68k_vsync_irq)
	MDRV_QUANTUM_TIME(HZ(60))

	MDRV_MACHINE_START( x68000 )
	MDRV_MACHINE_RESET( x68000 )

	/* device hardware */
	MDRV_MC68901_ADD(MC68901_TAG, mfp_interface)

	MDRV_PPI8255_ADD( "ppi8255",  ppi_interface )

	MDRV_HD63450_ADD( "hd63450", dmac_interface )

	MDRV_X68KHDC_ADD( "x68k_hdc" )

	MDRV_SCC8530_ADD( "scc", x68000_scc8530_interface )

    /* video hardware */
	MDRV_SCREEN_ADD("main", RASTER)
	MDRV_SCREEN_REFRESH_RATE(55.45)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
//  MDRV_GFXDECODE(x68k)
	MDRV_SCREEN_SIZE(1096, 568)  // inital setting
	MDRV_SCREEN_VISIBLE_AREA(0, 767, 0, 511)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_PALETTE_LENGTH(65536)
	MDRV_PALETTE_INIT( x68000 )

	MDRV_VIDEO_START( x68000 )
	MDRV_VIDEO_UPDATE( x68000 )

	MDRV_DEFAULT_LAYOUT( layout_x68000 )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left","right")
	MDRV_SOUND_ADD("ym2151", YM2151, 4000000)
	MDRV_SOUND_CONFIG(x68k_ym2151_interface)
	MDRV_SOUND_ROUTE(0, "left", 0.50)
	MDRV_SOUND_ROUTE(1, "right", 0.50)
    MDRV_SOUND_ADD("okim6258", OKIM6258, 4000000)
    MDRV_SOUND_CONFIG(x68k_okim6258_interface)
    MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.50)
    MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.50)

	MDRV_NVRAM_HANDLER( generic_0fill )

	MDRV_NEC72065_ADD("nec72065", fdc_interface)	
MACHINE_DRIVER_END

static SYSTEM_CONFIG_START(x68000)
	CONFIG_DEVICE(x68k_floppy_getinfo)
	CONFIG_RAM(0x100000)
	CONFIG_RAM(0x200000)
	CONFIG_RAM(0x300000)
	CONFIG_RAM_DEFAULT(0x400000)  // 4MB - should be enough for most things
	CONFIG_RAM(0x500000)
	CONFIG_RAM(0x600000)
	CONFIG_RAM(0x700000)
	CONFIG_RAM(0x800000)
	CONFIG_RAM(0x900000)
	CONFIG_RAM(0xa00000)
	CONFIG_RAM(0xb00000)
	CONFIG_RAM(0xc00000)  // 12MB - maximum possible
SYSTEM_CONFIG_END

ROM_START( x68000 )
	ROM_REGION16_BE(0x1000000, "main", 0)  // 16MB address space
	ROM_LOAD( "cgrom.dat",  0xf00000, 0xc0000, CRC(9f3195f1) SHA1(8d72c5b4d63bb14c5dbdac495244d659aa1498b6) )
	ROM_SYSTEM_BIOS(0, "ipl10",  "IPL-ROM V1.0")
	ROMX_LOAD( "iplrom.dat", 0xfe0000, 0x20000, CRC(72bdf532) SHA1(0ed038ed2133b9f78c6e37256807424e0d927560), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS(1, "ipl11",  "IPL-ROM V1.1")
	ROMX_LOAD( "iplromxv.dat", 0xfe0000, 0x020000, CRC(00eeb408) SHA1(e33cdcdb69cd257b0b211ef46e7a8b144637db57), ROM_BIOS(2) )
	ROM_SYSTEM_BIOS(2, "ipl12",  "IPL-ROM V1.2")
	ROMX_LOAD( "iplromco.dat", 0xfe0000, 0x020000, CRC(6c7ef608) SHA1(77511fc58798404701f66b6bbc9cbde06596eba7), ROM_BIOS(3) )
	ROM_SYSTEM_BIOS(3, "ipl13",  "IPL-ROM V1.3 (92/11/27)")
	ROMX_LOAD( "iplrom30.dat", 0xfe0000, 0x020000, CRC(e8f8fdad) SHA1(239e9124568c862c31d9ec0605e32373ea74b86a), ROM_BIOS(4) )
	ROM_REGION(0x8000, "user1",0)  // For Background/Sprite decoding
	ROM_FILL(0x0000,0x8000,0x00)
	ROM_REGION(0x20000, "user2", 0)
	ROM_FILL(0x000,0x20000,0x00)
ROM_END


/*    YEAR  NAME    PARENT  COMPAT  MACHINE INPUT   INIT    CONFIG  COMPANY     FULLNAME        FLAGS */
COMP( 1987, x68000, 0,      0,      x68000, x68000, x68000, x68000, "Sharp",    "Sharp X68000", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS )

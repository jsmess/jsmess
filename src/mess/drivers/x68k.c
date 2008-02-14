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


    *** Current status (12/08/07)
    FDC/FDD : Uses the uPD765A code with a small patch to handle Sense Interrupt Status being invalid if not in seek mode
              Extra uPD72065 commands not yet implemented, although I have yet to see them used.

    MFP : Largely works, as far as the X68000 goes.  Timers appear to work fine.
          Keyboard scancodes (via the MFP's USART) work, but there are some issues with in-game use.

    PPI : Joystick controls work okay.

    HDC/HDD : SASI and SCSI are not implemented, not a requirement at this point.

    RTC : Seems to work. (Tested using SX-Windows' Timer application)

    DMA : FDD reading mostly works, other channels should work for effective memory copying (channel 2, often
          used to copy data to video RAM or the palette in the background).

    Sound : FM works, ADPCM is unimplemented as yet.

    SCC : Works enough to get the mouse running

    Video : Text mode works, but is rather slow, especially scrolling up (uses multple "raster copy" commands).
            16 and 256 graphic layers work, but colours on a 65,536 colour layer are wrong.
            BG tiles and sprites work, but many games have the sprites offset by a small amount (some by a lot :))

    Other issues:
      Bus error exceptions are a bit late at times.  Currently using a fake bus error for MIDI expansion checks.  These
      are used determine if a piece of expansion hardware is present.
      Partial updates don't work at all well, and can be excruciatingly slow.
      Keyboard doesn't work well for games.
      Supervisor area set isn't implemented.

    Some minor game-specific issues (at 19/06/07):
      Pacmania:      Black squares on the maze (transparency?).
      Nemesis '94:   Menu system doesn't work except for start buttons.
      Flying Shark:  Appears to lock up at main menu.
      Salamander:    System error when using keys in-game.  No error if a joystick is used.
      Kyukyoku Tiger:Sprites offset by a looooong way.
      Dragon Buster: Text is black and unreadable (text palette should be loaded from disk, but it reads all zeroes).
      Baraduke:      Corrupt background, locks up on demo mode.
      Viewpoint:     Corrupt graphics on title screen, phantom movements on title screen, corrupt sprites, locks up.
      Mr. Do:        Locks up or resets after some time.  Happens on Mr Do vs. Unicorns, as well.
      Tetris:        Black dots over screen (text layer).
      Parodius Da!:  Water isn't animated (beginning of stage 1), black squares (raster effects?)


    More detailed documentation at http://x68kdev.emuvibes.com/iomap.html - if you can stand broken english :)

*/

#include "driver.h"
#include "render.h"
#include "deprecat.h"
#include "mslegacy.h"
#include "cpu/m68000/m68000.h"
#include "machine/68901mfp.h"
#include "machine/8255ppi.h"
#include "machine/nec765.h"
#include "sound/2151intf.h"
#include "sound/okim6295.h"
#include "machine/8530scc.h"
#include "machine/hd63450.h"
#include "machine/rp5c15.h"
#include "devices/basicdsk.h"
#include "includes/x68k.h"

struct x68k_system sys;

extern UINT16* gvram;  // Graphic VRAM
extern UINT16* tvram;  // Text VRAM
static UINT16* sram;   // SRAM
extern UINT16* x68k_spriteram;  // sprite/background RAM
extern UINT16* x68k_spritereg;  // sprite/background registers
static UINT8 ppi_port[3];
static int current_vector[8];
static UINT8 current_irq_line;
static unsigned int x68k_scanline;

static UINT8 mfp_key;

extern mame_bitmap* x68k_text_bitmap;  // 1024x1024 4x1bpp planes text
extern mame_bitmap* x68k_gfx_0_bitmap_16;  // 16 colour, 512x512, 4 pages
extern mame_bitmap* x68k_gfx_1_bitmap_16;
extern mame_bitmap* x68k_gfx_2_bitmap_16;
extern mame_bitmap* x68k_gfx_3_bitmap_16;
extern mame_bitmap* x68k_gfx_0_bitmap_256;  // 256 colour, 512x512, 2 pages
extern mame_bitmap* x68k_gfx_1_bitmap_256;
extern mame_bitmap* x68k_gfx_0_bitmap_65536;  // 65536 colour, 512x512, 1 page

extern tilemap* x68k_bg0_8;  // two 64x64 tilemaps, 8x8 characters
extern tilemap* x68k_bg1_8;
extern tilemap* x68k_bg0_16;  // two 64x64 tilemaps, 16x16 characters
extern tilemap* x68k_bg1_16;

static emu_timer* kb_timer;
//emu_timer* mfp_timer[4];
//emu_timer* mfp_irq;
emu_timer* scanline_timer;
emu_timer* raster_irq;
emu_timer* vblank_irq;
static emu_timer* mouse_timer;  // to set off the mouse interrupts via the SCC

// MFP is clocked at 4MHz, so at /4 prescaler the timer is triggered after 1us (4 cycles)
// No longer necessary with the new MFP core
/*static attotime prescale(int val)
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
}*/

static void mfp_init(void);
//static TIMER_CALLBACK(mfp_update_irq);

static void mfp_init()
{
	sys.mfp.tadr = sys.mfp.tbdr = sys.mfp.tcdr = sys.mfp.tddr = 0xff;

	sys.mfp.irqline = 6;  // MFP is connected to 68000 IRQ line 6
	sys.mfp.current_irq = -1;  // No current interrupt

/*  mfp_timer[0] = timer_alloc(mfp_timer_a_callback, NULL);
    mfp_timer[1] = timer_alloc(mfp_timer_b_callback, NULL);
    mfp_timer[2] = timer_alloc(mfp_timer_c_callback, NULL);
    mfp_timer[3] = timer_alloc(mfp_timer_d_callback, NULL);
    mfp_irq = timer_alloc(mfp_update_irq, NULL);
    timer_adjust(mfp_irq, attotime_zero, 0, ATTOTIME_IN_USEC(32));
*/
}
/*
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
                    cpunum_set_input_line_and_vector(machine, 0,sys.mfp.irqline,HOLD_LINE,(sys.mfp.vr & 0xf0) | (x + 8));
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
                    cpunum_set_input_line_and_vector(machine, 0,sys.mfp.irqline,HOLD_LINE,(sys.mfp.vr & 0xf0) | x);
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
        timer_adjust(mfp_timer[timer],attotime_zero,0,attotime_zero);
        logerror("MFP: Timer #%i stopped. \n",timer);
        return;
    }

    timer_adjust(mfp_timer[timer], attotime_zero, 0, prescale(data & 0x07));
    logerror("MFP: Timer #%i set to %2.1fus\n",timer, attotime_to_double(prescale(data & 0x07)) * 1000000);

}
*/

// 4 channel DMA controller (Hitachi HD63450)
static WRITE16_HANDLER( x68k_dmac_w )
{
	hd63450_w(offset, data, mem_mask);
}

static READ16_HANDLER( x68k_dmac_r )
{
	return hd63450_r(offset, mem_mask);
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
		// do nothing for now
		logerror("KB: LED status set to %02x\n",data & 0x7f);
	}

	if((data & 0xc0) == 0)  // TV control
	{
		// again, nothing for now
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

#ifdef UNUSED_FUNCTION
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
#endif

static void x68k_keyboard_push_scancode(unsigned char code)
{
	sys.keyboard.keynum++;
	if(sys.keyboard.keynum >= 1)
	{ // keyboard buffer full
		if(sys.keyboard.enabled != 0)
		{
			sys.mfp.rsr |= 0x80;  // Buffer full
//          mfp_trigger_irq(MFP_IRQ_RX_FULL);
			logerror("MFP: Receive buffer full IRQ sent\n");
		}
	}
	sys.keyboard.buffer[sys.keyboard.headpos++] = code;
	if(sys.keyboard.headpos > 15)
	{
		sys.keyboard.headpos = 0;
//      mfp_trigger_irq(MFP_IRQ_RX_ERROR);
	}
}

static TIMER_CALLBACK(x68k_keyboard_poll)
{
	int x;
	int port = port_tag_to_index("key1");

	for(x=0;x<0x80;x++)
	{
		// adjust delay/repeat timers
		if(sys.keyboard.keytime[x] > 0)
		{
			sys.keyboard.keytime[x] -= 5;
		}
		if(!(readinputport(port + x/32) & (1 << (x % 32))))
		{
			if(sys.keyboard.keyon[x] != 0)
			{
				x68k_keyboard_push_scancode(0x80 + x);
				sys.keyboard.keytime[x] = 0;
				sys.keyboard.keyon[x] = 0;
				sys.keyboard.last_pressed = 0;
				logerror("KB: Released key 0x%02x\n",x);
			}
		}
		// check to see if a key is being held
		if(sys.keyboard.keyon[x] != 0 && sys.keyboard.keytime[x] == 0 && sys.keyboard.last_pressed == x)
		{
			if(readinputport(port + sys.keyboard.last_pressed/32) & (1 << (sys.keyboard.last_pressed % 32)))
			{
				x68k_keyboard_push_scancode(sys.keyboard.last_pressed);
				sys.keyboard.keytime[sys.keyboard.last_pressed] = (sys.keyboard.repeat^2)*5+30;
				logerror("KB: Holding key 0x%02x\n",sys.keyboard.last_pressed);
			}
		}
		if((readinputport(port + x/32) & (1 << (x % 32))))
		{
			if(sys.keyboard.keyon[x] == 0)
			{
				x68k_keyboard_push_scancode(x);
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
static int x68k_read_mouse(void)
{
	char val = 0;
	char ipt = 0;

	switch(sys.mouse.inputtype)
	{
	case 0:
		ipt = readinputportbytag("mouse1");
		break;
	case 1:
		val = readinputportbytag("mouse2");
		ipt = val - sys.mouse.last_mouse_x;
		sys.mouse.last_mouse_x = val;
		break;
	case 2:
		val = readinputportbytag("mouse3");
		ipt = val - sys.mouse.last_mouse_y;
		sys.mouse.last_mouse_y = val;
		break;
	}
	sys.mouse.inputtype++;
	if(sys.mouse.inputtype > 2)
	{
		int val = scc_get_reg_b(0);
		sys.mouse.inputtype = 0;
		sys.mouse.bufferempty = 1;
		val &= ~0x01;
		scc_set_reg_b(0,val);
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
	offset %= 4;
	switch(offset)
	{
	case 0:
		return scc_r(0);
	case 1:
		return x68k_read_mouse();
	case 2:
		return scc_r(1);
	case 3:
		return scc_r(3);
	default:
		return 0xff;
	}
}

static WRITE16_HANDLER( x68k_scc_w )
{
	static unsigned char prev;
	offset %= 4;

	switch(offset)
	{
	case 0:
		scc_w(0,(UINT8)data);
		if((scc_get_reg_b(5) & 0x02) != prev)
		{
			if(scc_get_reg_b(5) & 0x02)  // Request to Send
			{
				int val = scc_get_reg_b(0);
				sys.mouse.bufferempty = 0;
				val |= 0x01;
				scc_set_reg_b(0,val);
			}
		}
		break;
	case 1:
		scc_w(2,(UINT8)data);
		break;
	case 2:
		scc_w(1,(UINT8)data);
		break;
	case 3:
		scc_w(3,(UINT8)data);
		break;
	}
	prev = scc_get_reg_b(5) & 0x02;
}

static TIMER_CALLBACK(x68k_scc_ack)
{
	if(sys.mouse.bufferempty != 0)  // nothing to do if the mouse data buffer is empty
		return;

	if((sys.ioc.irqstatus & 0xc0) != 0)
		return;

	// hard-code the IRQ vector for now, until the SCC code is more complete
	if((scc_get_reg_a(9) & 0x08) || (scc_get_reg_b(9) & 0x08))  // SCC reg WR9 is the same for both channels
	{
		if((scc_get_reg_b(1) & 0x18) != 0)  // if bits 3 and 4 of WR1 are 0, then Rx IRQs are disabled on this channel
		{
			if(scc_get_reg_b(5) & 0x02)  // RTS signal
			{
				sys.mouse.irqactive = 1;
				current_vector[5] = 0x54;
				current_irq_line = 5;
				cpunum_set_input_line_and_vector(machine, 0,5,HOLD_LINE,0x54);
			}
		}
	}
}

// Judging from the XM6 source code, PPI ports A and B are joystick inputs
static READ8_HANDLER( ppi_port_a_r )
{
	// Joystick 1
	if(sys.joy.joy1_enable == 0)
		return readinputportbytag("joy1");
	else
		return 0xff;
}

static READ8_HANDLER( ppi_port_b_r )
{
	// Joystick 2
	if(sys.joy.joy2_enable == 0)
		return readinputportbytag("joy2");
	else
		return 0xff;
}

static READ8_HANDLER( ppi_port_c_r )
{
	return ppi_port[2];
}

#ifdef UNUSED_FUNCTION
WRITE8_HANDLER( ppi_port_a_w )
{
	ppi_port[0] = data;
}

WRITE8_HANDLER( ppi_port_b_w )
{
	ppi_port[1] = data;
}
#endif

static WRITE8_HANDLER( ppi_port_c_w )
{
	// ADPCM / Joystick control
	ppi_port[2] = data;
	sys.adpcm.pan = data & 0x03;
	sys.adpcm.rate = data & 0x0c;
	sys.joy.joy1_enable = data & 0x10;
	sys.joy.joy2_enable = data & 0x20;
	sys.joy.ioc6 = data & 0x40;
	sys.joy.ioc7 = data & 0x80;
}



// NEC uPD72065 at 0xe94000
static WRITE16_HANDLER( x68k_fdc_w )
{
	unsigned int drive, x;
	switch(offset)
	{
	case 0x00:
	case 0x01:
		nec765_data_w(0,data);
		break;
	case 0x02:  // drive option signal control
		x = data & 0x0f;
		for(drive=0;drive<4;drive++)
		{
			if(x & (1 << drive))
			{
				sys.fdc.led_ctrl[drive] = data & 0x80;  // blinking drive LED if no disk inserted
				sys.fdc.led_eject[drive] = data & 0x40;  // eject button LED
				if(data & 0x20)  // ejects disk
				{
					image_unload(image_from_devtype_and_index(IO_FLOPPY, drive));
					floppy_drive_set_motor_state(image_from_devtype_and_index(IO_FLOPPY, drive), 0);  // I'll presume ejecting the disk stops the drive motor :)
				}
			}
		}
		sys.fdc.selected_drive = data & 0x0f;
		logerror("FDC: signal control set to %02x\n",data);
		break;
	case 0x03:
		sys.fdc.media_density[data & 0x03] = data & 0x10;
		sys.fdc.motor[data & 0x03] = data & 0x80;
		floppy_drive_set_motor_state(image_from_devtype_and_index(IO_FLOPPY, data & 0x03), (data & 0x80));
		floppy_drive_set_ready_state(image_from_devtype_and_index(IO_FLOPPY, 0),1,1);
		floppy_drive_set_ready_state(image_from_devtype_and_index(IO_FLOPPY, 1),1,1);
		floppy_drive_set_ready_state(image_from_devtype_and_index(IO_FLOPPY, 2),1,1);
		floppy_drive_set_ready_state(image_from_devtype_and_index(IO_FLOPPY, 3),1,1);
		logerror("FDC: Drive #%i: Drive selection set to %02x\n",data & 0x03,data);
		break;
	default:
//      logerror("FDC: [%08x] Wrote %04x to invalid FDC port %04x\n",activecpu_get_pc(),data,offset);
		break;
	}
}

static READ16_HANDLER( x68k_fdc_r )
{
	unsigned int ret;
	int x;
	switch(offset)
	{
	case 0x00:
		return nec765_status_r(0);
	case 0x01:
		return nec765_data_r(0);
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

static void fdc_irq(int state)
{
	if((sys.ioc.irqstatus & 0x04) && state == ASSERT_LINE)
	{
		current_vector[1] = sys.ioc.fdcvector;
		sys.ioc.irqstatus |= 0x80;
		current_irq_line = 1;
		logerror("FDC: IRQ triggered\n");
		cpunum_set_input_line_and_vector(Machine, 0,1,HOLD_LINE,current_vector[1]);
	}
}

static int x68k_fdc_read_byte(int addr)
{
	int data = -1;

	if(sys.fdc.drq_state != 0)
		data = nec765_dack_r(0);
//  logerror("FDC: DACK reading\n");
	return data;
}

static void x68k_fdc_write_byte(int addr, int data)
{
	nec765_dack_w(0,data);
}

static void fdc_drq(int state, int read_write)
{
	sys.fdc.drq_state = state;
}

static WRITE16_HANDLER( x68k_hdc_w )
{
	// SASI HDC - HDDs are not a required system component, so this is something to be done later
	logerror("SASI: write to HDC, offset %04x, data %04x\n",offset,data);
}

static READ16_HANDLER( x68k_hdc_r )
{
	logerror("SASI: [%08x] read from HDC, offset %04x\n",activecpu_get_pc(),offset);
	switch(offset)
	{
	case 0x01:
		return 0x00;
	default:
		return 0xff;
	}
}

static WRITE16_HANDLER( x68k_fm_w )
{
	switch(offset)
	{
	case 0x00:
		YM2151_register_port_0_w(0,data);
		logerror("YM: Register select 0x%02x\n",data);
		break;
	case 0x01:
		YM2151_data_port_0_w(0,data);
		logerror("YM: Data write 0x%02x\n",data);
		break;
	}
}

static READ16_HANDLER( x68k_fm_r )
{
	if(offset == 0x01)
		return YM2151_status_port_0_r(0);

	return 0xff;
}

static WRITE8_HANDLER( x68k_ct_w )
{
	// CT1 and CT2 bits from YM2151 port 0x1b
	// CT1 - ADPCM clock - 0 = 8MHz, 1 = 4MHz
	// CT2 - 1 = Set ready state of FDC
	nec765_set_ready_state(data & 0x01);
}


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

static WRITE16_HANDLER( x68k_sysport_w )
{
	render_container* container;
	switch(offset)
	{
	case 0x00:
		sys.sysport.contrast = data & 0x0f;  // often used for screen fades / blanking
		container = render_container_get_screen(0);
		render_container_set_brightness(container,(float)(sys.sysport.contrast) / 14.0);
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
//      logerror("SYS: [%08x] Wrote %04x to invalid or unimplemented system port %04x\n",activecpu_get_pc(),data,offset);
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

/*
READ16_HANDLER( x68k_mfp_r )
{
    int ret;
    // Initial settings indicate that IRQs are generated for FM (YM2151), Receive buffer error or full,
    // MFP Timer C, and the power switch
//  logerror("MFP: [%08x] Reading offset %i\n",activecpu_get_pc(),offset);
    switch(offset)
    {
    case 0x00:  // GPIP - General purpose I/O register (read-only)
        ret = 0x23;
        if(video_screen_get_vpos(0) == sys.crtc.reg[9])
            ret |= 0x40;
        if(sys.crtc.vblank == 0)
            ret |= 0x10;  // Vsync signal (low if in vertical retrace)
//      if(sys.mfp.isrb & 0x08)
//          ret |= 0x08;  // FM IRQ signal
        if(video_screen_get_hpos(0) > sys.crtc.width - 32)
            ret |= 0x80;  // Hsync signal
//      logerror("MFP: [%08x] Reading offset %i (ret=%02x)\n",activecpu_get_pc(),offset,ret);
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
        return sys.mfp.timer[3].counter;
    case 21:  // RSR
        return sys.mfp.rsr;
    case 22:  // TSR
        return sys.mfp.tsr | 0x80;  // buffer is typically empty?
    case 23:
        return x68k_keyboard_pop_scancode();
    default:
//      logerror("MFP: [%08x] Offset %i read\n",activecpu_get_pc(),offset);
        return 0xff;
    }
}
*/
static WRITE16_HANDLER( x68k_mfp_w )
{
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
        break;
    case 21:
        if(data & 0x01)
            sys.mfp.usart.recv_enable = 1;
        else
            sys.mfp.usart.recv_enable = 0;
        break;*/
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
//          logerror("MFP: [%08x] USART Sent data %04x\n",activecpu_get_pc(),data);
		}
		break;
	default:
		mfp68901_0_register_lsb_w(offset,data,mem_mask);
		return;
	}
}


static WRITE16_HANDLER( x68k_ppi_w )
{
	ppi8255_w(0,offset & 0x03,data);
}

static READ16_HANDLER( x68k_ppi_r )
{
	return ppi8255_r(0,offset & 0x03);
}

static READ16_HANDLER( x68k_rtc_r )
{
	return rp5c15_r(offset,mem_mask);
}

static WRITE16_HANDLER( x68k_rtc_w )
{
	rp5c15_w(offset,data,mem_mask);
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
	if(offset == 0x5a/2)  // 0x5a should be 0 if no SASI HDs are present.
		return 0x0000;
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
	if(offset < 0x100)
	{
		COMBINE_DATA(sys.video.gfx_pal+offset);
		val = sys.video.gfx_pal[offset];
		palette_set_color_rgb(Machine,offset,(val & 0x07c0) >> 3,(val & 0xf800) >> 8,(val & 0x003e) << 2);
		return;
	}

	if(offset >= 0x100 && offset < 0x200)
	{
		COMBINE_DATA(sys.video.text_pal+(offset-0x100));
		val = sys.video.text_pal[offset-0x100];
		palette_set_color_rgb(Machine,offset,(val & 0x07c0) >> 3,(val & 0xf800) >> 8,(val & 0x003e) << 2);
		return;
	}

	switch(offset)
	{
	case 0x200:
		sys.video.reg[0] = data;
		break;
	case 0x280:  // priority levels
		sys.video.reg[1] = data;
		sys.video.gfxlayer_pri[0] = data & 0x0003;
		sys.video.gfxlayer_pri[1] = (data & 0x000c) >> 2;
		sys.video.gfxlayer_pri[2] = (data & 0x0030) >> 4;
		sys.video.gfxlayer_pri[3] = (data & 0x00c0) >> 6;
		sys.video.gfx_pri = (data & 0x0300) >> 8;
		sys.video.text_pri = (data & 0x0c00) >> 10;
		sys.video.sprite_pri = (data & 0x3000) >> 12;
		break;
	case 0x300:
		sys.video.reg[2] = data;
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

static READ16_HANDLER( x68k_adpcm_r )
{
	return 0x0000;
}

static WRITE16_HANDLER( x68k_adpcm_w )
{
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
		int sp = cpunum_get_reg(0,REG_SP);
		int pc = cpunum_get_reg(0,REG_PC);
		int sr = cpunum_get_reg(0,M68K_SR);
		//int pda = cpunum_get_reg(0,M68K_PREF_DATA);
		cpunum_set_reg(0,REG_SP,sp-14);
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
		cpunum_set_reg(0,REG_PC,addr);  // real exceptions seem to take too long to be acknowledged
		popmessage("Expansion access [%08x]: PC jump to %08x",val,addr);
	}
}

static READ16_HANDLER( x68k_rom0_r )
{
	/* this location contains the address of some expansion device ROM, if no ROM exists,
       then access causes a bus error */
	current_vector[2] = 0x02;  // bus error
	current_irq_line = 2;
	cpunum_set_input_line_and_vector(Machine, 0,2,ASSERT_LINE,current_vector[2]);
	return 0xff;
}

static WRITE16_HANDLER( x68k_rom0_w )
{
	/* this location contains the address of some expansion device ROM, if no ROM exists,
       then access causes a bus error */
	current_vector[2] = 0x02;  // bus error
	current_irq_line = 2;
	cpunum_set_input_line_and_vector(Machine, 0,2,ASSERT_LINE,current_vector[2]);
}

static READ16_HANDLER( x68k_exp_r )
{
	/* These are expansion devices, if not present, they cause a bus error */
	if(readinputportbytag("options") & 0x02)
	{
		current_vector[2] = 0x02;  // bus error
		current_irq_line = 2;
		offset *= 2;
		if(ACCESSING_LSB)
			offset++;
		timer_set(ATTOTIME_IN_CYCLES(16,0), NULL, 0xeafa00+offset,x68k_fake_bus_error);
//      cpunum_set_input_line_and_vector(Machine, 0,2,ASSERT_LINE,current_vector[2]);
	}
	return 0xffff;
}

static WRITE16_HANDLER( x68k_exp_w )
{
	/* These are expansion devices, if not present, they cause a bus error */
	if(readinputportbytag("options") & 0x02)
	{
		current_vector[2] = 0x02;  // bus error
		current_irq_line = 2;
		offset *= 2;
		if(ACCESSING_LSB)
			offset++;
		timer_set(ATTOTIME_IN_CYCLES(16,0), NULL, 0xeafa00+offset,x68k_fake_bus_error);
//      cpunum_set_input_line_and_vector(Machine, 0,2,ASSERT_LINE,current_vector[2]);
	}
}

static void x68k_dma_irq(int channel)
{
	current_vector[3] = hd63450_get_vector(channel);
	current_irq_line = 3;
	logerror("DMA#%i: DMA End (vector 0x%02x)\n",channel,current_vector[3]);
	cpunum_set_input_line_and_vector(Machine, 0,3,ASSERT_LINE,current_vector[3]);
}

static void x68k_dma_end(int channel,int irq)
{
	if(irq != 0)
	{
		x68k_dma_irq(channel);
	}
}

static void x68k_dma_error(int channel, int irq)
{
	if(irq != 0)
	{
		current_vector[3] = hd63450_get_error_vector(channel);
		current_irq_line = 3;
		cpunum_set_input_line_and_vector(Machine, 0,3,ASSERT_LINE,current_vector[3]);
	}
}

static void x68k_fm_irq(int irq)
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

static READ8_HANDLER(mfp_gpio_r)
{
	UINT8 data = sys.mfp.gpio;

	data &= ~(sys.crtc.hblank << 7);
	data &= ~(sys.crtc.vblank << 4);
	data |= 0x23;  // GPIP5 is unused, always 1
	mfp68901_tai_w(0,sys.crtc.vblank);

	return data;
}

static void mfp_irq_callback(int which, int state)
{
	static int prev;
	if(prev == CLEAR_LINE && state == CLEAR_LINE)  // eliminate unnecessary calls to set the IRQ line for speed reasons
		return;
	if((sys.ioc.irqstatus & 0xc0) != 0)  // if the FDC is busy, then we don't want to miss that IRQ
		return;
	cpunum_set_input_line(Machine, 0, 6, state);
	prev = state;
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
//      video_screen_update_partial(0,256);//sys.crtc.reg[4]/2);
//  else
//      video_screen_update_partial(0,512);//sys.crtc.reg[4]);
}

static int x68k_int_ack(int line)
{
	if(line == 6)  // MFP
	{
//      if(sys.mfp.isra & 0x10)
//          sys.mfp.rsr &= ~0x80;
//      if(sys.mfp.isra & 0x04)
//          sys.mfp.tsr &= ~0x80;

//      if(sys.mfp.current_irq < 8)
//      {
//          sys.mfp.iprb &= ~(1 << sys.mfp.current_irq);
			// IRQ is in service
//          if(sys.mfp.eoi_mode != 0)  // automatic EOI does not set the ISR registers
//              sys.mfp.isrb |= (1 << sys.mfp.current_irq);
//      }
//      else
//      {
//          sys.mfp.ipra &= ~(1 << (sys.mfp.current_irq - 8));
			// IRQ is in service
//          if(sys.mfp.eoi_mode != 0)  // automatic EOI does not set the ISR registers
//              sys.mfp.isra |= (1 << (sys.mfp.current_irq - 8));
//      }
		sys.mfp.current_irq = -1;
		current_vector[6] = mfp68901_get_vector(0);
		logerror("SYS: IRQ acknowledged (vector=0x%02x, line = %i)\n",current_vector[6],line);
		return current_vector[6];
	}

	cpunum_set_input_line_and_vector(Machine, 0,line,CLEAR_LINE,current_vector[line]);
	if(line == 1)  // IOSC
	{
		sys.ioc.irqstatus &= ~0xf0;
	}
	if(line == 5)  // SCC
	{
		sys.mouse.irqactive = 0;
	}

	logerror("SYS: IRQ acknowledged (vector=0x%02x, line = %i)\n",current_vector[line],line);
	return current_vector[line];
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
	AM_RANGE(0xe88000, 0xe89fff) AM_READWRITE(mfp68901_0_register_lsb_r, x68k_mfp_w)
	AM_RANGE(0xe8a000, 0xe8bfff) AM_READWRITE(x68k_rtc_r, x68k_rtc_w)
//  AM_RANGE(0xe8c000, 0xe8dfff) AM_READWRITE(x68k_printer_r, x68k_printer_w)
	AM_RANGE(0xe8e000, 0xe8ffff) AM_READWRITE(x68k_sysport_r, x68k_sysport_w)
	AM_RANGE(0xe90000, 0xe91fff) AM_READWRITE(x68k_fm_r, x68k_fm_w)
	AM_RANGE(0xe92000, 0xe93fff) AM_READWRITE(x68k_adpcm_r, x68k_adpcm_w)
	AM_RANGE(0xe94000, 0xe95fff) AM_READWRITE(x68k_fdc_r, x68k_fdc_w)
	AM_RANGE(0xe96000, 0xe97fff) AM_READWRITE(x68k_hdc_r, x68k_hdc_w)
	AM_RANGE(0xe98000, 0xe99fff) AM_READWRITE(x68k_scc_r, x68k_scc_w)
	AM_RANGE(0xe9a000, 0xe9bfff) AM_READWRITE(x68k_ppi_r, x68k_ppi_w)
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

static const mfp68901_interface mfp_interface =
{
	2000000, // 4MHz clock
	4000000,
	MFP68901_TDO_LOOPBACK,
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
	1,  // 1 PPI
	{ ppi_port_a_r },
	{ ppi_port_b_r },
	{ ppi_port_c_r },
	{ NULL },
	{ NULL },
	{ ppi_port_c_w }
};

static const struct hd63450_interface dmac_interface =
{
	0,  // CPU - 68000
	{STATIC_ATTOTIME_IN_USEC(32),STATIC_ATTOTIME_IN_USEC(32),STATIC_ATTOTIME_IN_USEC(4),STATIC_ATTOTIME_IN_USEC(32)},  // Cycle steal mode timing (guesstimate)
	{STATIC_ATTOTIME_IN_USEC(32),STATIC_ATTOTIME_IN_NSEC(50),STATIC_ATTOTIME_IN_NSEC(50),STATIC_ATTOTIME_IN_NSEC(50)}, // Burst mode timing (guesstimate)
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
	fdc_drq
};

static const struct YM2151interface ym2151_interface =
{
	x68k_fm_irq,
	x68k_ct_w  // CT1, CT2 from YM2151 port 0x1b
};

static const struct scc8530_interface scc_interface =
{
	NULL//x68k_scc_ack
};

static const struct rp5c15_interface rtc_intf =
{
	x68k_rtc_alarm_irq
};

static INPUT_PORTS_START( x68000 )
	PORT_START_TAG( "joy1" )
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP) PORT_CODE(JOYCODE_Y_UP_SWITCH)	 PORT_PLAYER(1)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN) PORT_CODE(JOYCODE_Y_DOWN_SWITCH)	 PORT_PLAYER(1)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT) PORT_CODE(JOYCODE_X_LEFT_SWITCH)	 PORT_PLAYER(1)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT) PORT_CODE(JOYCODE_X_RIGHT_SWITCH)	 PORT_PLAYER(1)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_CODE(JOYCODE_BUTTON1)	 PORT_PLAYER(1)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_BUTTON2) PORT_CODE(JOYCODE_BUTTON2)	 PORT_PLAYER(1)

	PORT_START_TAG( "joy2" )
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP) PORT_CODE(JOYCODE_Y_UP_SWITCH)	 PORT_PLAYER(2)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN) PORT_CODE(JOYCODE_Y_DOWN_SWITCH)	 PORT_PLAYER(2)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT) PORT_CODE(JOYCODE_X_LEFT_SWITCH)	 PORT_PLAYER(2)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT) PORT_CODE(JOYCODE_X_RIGHT_SWITCH)	 PORT_PLAYER(2)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_CODE(JOYCODE_BUTTON1)	 PORT_PLAYER(2)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_BUTTON2) PORT_CODE(JOYCODE_BUTTON2)	 PORT_PLAYER(2)

	PORT_START_TAG( "key1" )
	PORT_BIT(0x00000001, IP_ACTIVE_HIGH, IPT_UNUSED) // unused
	PORT_BIT(0x00000002, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_CODE(KEYCODE_ESC) PORT_CHAR(27)  /* ESC */
	PORT_BIT(0x00000004, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_CODE(KEYCODE_1)  PORT_CHAR('1') PORT_CHAR('!') /* 1 ! */
	PORT_BIT(0x00000008, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_CODE(KEYCODE_2)  PORT_CHAR('2') PORT_CHAR('\"') /* 2 " */
	PORT_BIT(0x00000010, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_CODE(KEYCODE_3)  PORT_CHAR('3') PORT_CHAR('#') /* 3 # */
	PORT_BIT(0x00000020, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_CODE(KEYCODE_4)  PORT_CHAR('4') PORT_CHAR('$') /* 4 $ */
	PORT_BIT(0x00000040, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_CODE(KEYCODE_5)  PORT_CHAR('5') PORT_CHAR('%') /* 5 % */
	PORT_BIT(0x00000080, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_CODE(KEYCODE_6)  PORT_CHAR('6') PORT_CHAR('&') /* 6 & */
	PORT_BIT(0x00000100, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_CODE(KEYCODE_7)  PORT_CHAR('7') PORT_CHAR('\'') /* 7 ' */
	PORT_BIT(0x00000200, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_CODE(KEYCODE_8)  PORT_CHAR('8') PORT_CHAR('(') /* 8 ( */
	PORT_BIT(0x00000400, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_CODE(KEYCODE_9)  PORT_CHAR('9') PORT_CHAR(')') /* 9 ) */
	PORT_BIT(0x00000800, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_CODE(KEYCODE_0)  PORT_CHAR('0')                /* 0 */
	PORT_BIT(0x00001000, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_CODE(KEYCODE_MINUS)  PORT_CHAR('-') PORT_CHAR('=') /* - = */
	PORT_BIT(0x00002000, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_CHAR('^') /* ^ */
	PORT_BIT(0x00004000, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_CODE(KEYCODE_BACKSLASH)  PORT_CHAR('\\') PORT_CHAR('|') /* Yen | */
	PORT_BIT(0x00008000, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_CODE(KEYCODE_BACKSPACE)  PORT_CHAR(8) /* Backspace */
	PORT_BIT(0x00010000, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_CODE(KEYCODE_TAB)  PORT_CHAR(9)  /* Tab */
	PORT_BIT(0x00020000, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_CODE(KEYCODE_Q)  PORT_CHAR('Q')  /* Q */
	PORT_BIT(0x00040000, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_CODE(KEYCODE_W)  PORT_CHAR('W')  /* W */
	PORT_BIT(0x00080000, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_CODE(KEYCODE_E)  PORT_CHAR('E')  /* E */
	PORT_BIT(0x00100000, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_CODE(KEYCODE_R)  PORT_CHAR('R')  /* R */
	PORT_BIT(0x00200000, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_CODE(KEYCODE_T)  PORT_CHAR('T')  /* T */
	PORT_BIT(0x00400000, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_CODE(KEYCODE_Y)  PORT_CHAR('Y')  /* Y */
	PORT_BIT(0x00800000, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_CODE(KEYCODE_U)  PORT_CHAR('U')  /* U */
	PORT_BIT(0x01000000, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_CODE(KEYCODE_I)  PORT_CHAR('I')  /* I */
	PORT_BIT(0x02000000, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_CODE(KEYCODE_O)  PORT_CHAR('O')  /* O */
	PORT_BIT(0x04000000, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_CODE(KEYCODE_P)  PORT_CHAR('P')  /* P */
	PORT_BIT(0x08000000, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_CHAR('@')  /* @ */
	PORT_BIT(0x10000000, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_CODE(KEYCODE_OPENBRACE)  PORT_CHAR('[') PORT_CHAR('{')  /* [ { */
	PORT_BIT(0x20000000, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_CODE(KEYCODE_ENTER)  PORT_CHAR(13)  /* Return */
	PORT_BIT(0x40000000, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_CODE(KEYCODE_A)  PORT_CHAR('A')  /* A */
	PORT_BIT(0x80000000, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_CODE(KEYCODE_S)  PORT_CHAR('S')  /* S */

	PORT_START_TAG( "key2" )
	PORT_BIT(0x00000001, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_CODE(KEYCODE_D)  PORT_CHAR('D')  /* D */
	PORT_BIT(0x00000002, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_CODE(KEYCODE_F)  PORT_CHAR('F')  /* F */
	PORT_BIT(0x00000004, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_CODE(KEYCODE_G)  PORT_CHAR('G')  /* G */
	PORT_BIT(0x00000008, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_CODE(KEYCODE_H)  PORT_CHAR('H')  /* H */
	PORT_BIT(0x00000010, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_CODE(KEYCODE_J)  PORT_CHAR('J')  /* J */
	PORT_BIT(0x00000020, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_CODE(KEYCODE_K)  PORT_CHAR('K')  /* K */
	PORT_BIT(0x00000040, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_CODE(KEYCODE_L)  PORT_CHAR('L')  /* L */
	PORT_BIT(0x00000080, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_CODE(KEYCODE_COLON)  PORT_CHAR(';')  PORT_CHAR('+')  /* ; + */
	PORT_BIT(0x00000100, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_CODE(KEYCODE_QUOTE)  PORT_CHAR(':')  PORT_CHAR('*')  /* : * */
	PORT_BIT(0x00000200, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_CODE(KEYCODE_CLOSEBRACE)  PORT_CHAR(']')  PORT_CHAR('}')  /* ] } */
	PORT_BIT(0x00000400, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_CODE(KEYCODE_Z)  PORT_CHAR('Z')  /* Z */
	PORT_BIT(0x00000800, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_CODE(KEYCODE_X)  PORT_CHAR('X')  /* X */
	PORT_BIT(0x00001000, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_CODE(KEYCODE_C)  PORT_CHAR('C')  /* C */
	PORT_BIT(0x00002000, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_CODE(KEYCODE_V)  PORT_CHAR('V')  /* V */
	PORT_BIT(0x00004000, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_CODE(KEYCODE_B)  PORT_CHAR('B')  /* B */
	PORT_BIT(0x00008000, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_CODE(KEYCODE_N)  PORT_CHAR('N')  /* N */
	PORT_BIT(0x00010000, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_CODE(KEYCODE_M)  PORT_CHAR('M')  /* M */
	PORT_BIT(0x00020000, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_CODE(KEYCODE_COMMA)  PORT_CHAR(',')  PORT_CHAR('<')  /* , < */
	PORT_BIT(0x00040000, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_CODE(KEYCODE_STOP)  PORT_CHAR('.')  PORT_CHAR('>')  /* . > */
	PORT_BIT(0x00080000, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_CODE(KEYCODE_SLASH)  PORT_CHAR('/')  PORT_CHAR('?')  /* / ? */
	PORT_BIT(0x00100000, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_CHAR('_')  /* Underscore (shifted only?) */
	PORT_BIT(0x00200000, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_NAME("Space")  PORT_CODE(KEYCODE_SPACE)  PORT_CHAR(' ')  /* Space */
	PORT_BIT(0x00400000, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_NAME("Home")  PORT_CODE(KEYCODE_HOME)  /* Home */
	PORT_BIT(0x00800000, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_NAME("Delete")  PORT_CODE(KEYCODE_DEL)  /* Del */
	PORT_BIT(0x01000000, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_NAME("Roll Up")  PORT_CODE(KEYCODE_PGUP)  /* Roll Up */
	PORT_BIT(0x02000000, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_NAME("Roll Down")  PORT_CODE(KEYCODE_PGDN)  /* Roll Down */
	PORT_BIT(0x04000000, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_NAME("Undo")  PORT_CODE(KEYCODE_END)  /* Undo */
	PORT_BIT(0x08000000, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_NAME("Cursor Left")  PORT_CODE(KEYCODE_LEFT)  /* Left */
	PORT_BIT(0x10000000, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_NAME("Cursor Up")  PORT_CODE(KEYCODE_UP)  /* Up */
	PORT_BIT(0x20000000, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_NAME("Cursor Right")  PORT_CODE(KEYCODE_RIGHT)  /* Right */
	PORT_BIT(0x40000000, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_NAME("Cursor Down")  PORT_CODE(KEYCODE_DOWN)  /* Down */
	PORT_BIT(0x80000000, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_NAME("Tenkey CLR")  PORT_CODE(KEYCODE_NUMLOCK)  /* CLR */

	PORT_START_TAG( "key3" )
	PORT_BIT(0x00000001, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_NAME("Tenkey /")  PORT_CODE(KEYCODE_SLASH_PAD)  /* / (numpad) */
	PORT_BIT(0x00000002, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_NAME("Tenkey *")  PORT_CODE(KEYCODE_ASTERISK)  /* * (numpad) */
	PORT_BIT(0x00000004, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_NAME("Tenkey -")  PORT_CODE(KEYCODE_MINUS_PAD)  /* - (numpad) */
	PORT_BIT(0x00000008, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_NAME("Tenkey 7")  PORT_CODE(KEYCODE_7_PAD)  /* 7 (numpad) */
	PORT_BIT(0x00000010, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_NAME("Tenkey 8")  PORT_CODE(KEYCODE_8_PAD)  /* 8 (numpad) */
	PORT_BIT(0x00000020, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_NAME("Tenkey 9")  PORT_CODE(KEYCODE_9_PAD)  /* 9 (numpad) */
	PORT_BIT(0x00000040, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_NAME("Tenkey +")  PORT_CODE(KEYCODE_PLUS_PAD)  /* + (numpad) */
	PORT_BIT(0x00000080, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_NAME("Tenkey 4")  PORT_CODE(KEYCODE_4_PAD)  /* 4 (numpad) */
	PORT_BIT(0x00000100, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_NAME("Tenkey 5")  PORT_CODE(KEYCODE_5_PAD)  /* 5 (numpad) */
	PORT_BIT(0x00000200, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_NAME("Tenkey 6")  PORT_CODE(KEYCODE_6_PAD)  /* 6 (numpad) */
	PORT_BIT(0x00000400, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_NAME("Tenkey =")  /* = (numpad) */
	PORT_BIT(0x00000800, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_NAME("Tenkey 1")  PORT_CODE(KEYCODE_1_PAD)  /* 1 (numpad) */
	PORT_BIT(0x00001000, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_NAME("Tenkey 2")  PORT_CODE(KEYCODE_2_PAD)  /* 2 (numpad) */
	PORT_BIT(0x00002000, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_NAME("Tenkey 3")  PORT_CODE(KEYCODE_3_PAD)  /* 3 (numpad) */
	PORT_BIT(0x00004000, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_NAME("Tenkey Enter")  PORT_CODE(KEYCODE_ENTER_PAD)  /* Enter (numpad) */
	PORT_BIT(0x00008000, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_NAME("Tenkey 0")  PORT_CODE(KEYCODE_0_PAD)  /* 0 (numpad) */
	PORT_BIT(0x00010000, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_NAME("Tenkey ,")  /* , (numpad) */
	PORT_BIT(0x00020000, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_NAME("Tenkey .")  PORT_CODE(KEYCODE_DEL_PAD)  /* 2 (numpad) */
	PORT_BIT(0x00040000, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_NAME("Sign")  /* Sign / Symbolic input (babelfish translation) */
	PORT_BIT(0x00080000, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_NAME("Register")  /* Register (babelfish translation) */
	PORT_BIT(0x00100000, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_NAME("Help")  /* Help */
	PORT_BIT(0x00200000, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_NAME("XF1")  PORT_CODE(KEYCODE_F11)  /* XF1 */
	PORT_BIT(0x00400000, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_NAME("XF2")  PORT_CODE(KEYCODE_F12)  /* XF2 */
	PORT_BIT(0x00800000, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_NAME("XF3")  /* XF3 */
	PORT_BIT(0x01000000, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_NAME("XF4")  /* XF4 */
	PORT_BIT(0x02000000, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_NAME("XF5")  /* XF5 */
	PORT_BIT(0x04000000, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_NAME("Kana")  /* Kana */
	PORT_BIT(0x08000000, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_NAME("Romaji")  /* Romaji */
	PORT_BIT(0x10000000, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_NAME("Code")  /* Code input */
	PORT_BIT(0x20000000, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_NAME("Caps")  PORT_CODE(KEYCODE_CAPSLOCK)  /* Caps lock */
	PORT_BIT(0x40000000, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_NAME("Insert")  PORT_CODE(KEYCODE_INSERT)  /* Insert */
	PORT_BIT(0x80000000, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_NAME("Hiragana")  /* Hiragana */

	PORT_START_TAG( "key4" )
	PORT_BIT(0x00000001, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_NAME("Full size")  /* Full size (babelfish translation) */
	PORT_BIT(0x00000002, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_NAME("Break")  /* Break */
	PORT_BIT(0x00000004, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_NAME("Copy")  /* Copy */
	PORT_BIT(0x00000008, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_NAME("F1")  PORT_CODE(KEYCODE_F1)  /* F1 */
	PORT_BIT(0x00000010, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_NAME("F2")  PORT_CODE(KEYCODE_F2)  /* F2 */
	PORT_BIT(0x00000020, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_NAME("F3")  PORT_CODE(KEYCODE_F3)  /* F3 */
	PORT_BIT(0x00000040, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_NAME("F4")  PORT_CODE(KEYCODE_F4)  /* F4 */
	PORT_BIT(0x00000080, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_NAME("F5")  PORT_CODE(KEYCODE_F5)  /* F5 */
	PORT_BIT(0x00000100, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_NAME("F6")  PORT_CODE(KEYCODE_F6)  /* F6 */
	PORT_BIT(0x00000200, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_NAME("F7")  PORT_CODE(KEYCODE_F7)  /* F7 */
	PORT_BIT(0x00000400, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_NAME("F8")  PORT_CODE(KEYCODE_F8)  /* F8 */
	PORT_BIT(0x00000800, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_NAME("F9")  PORT_CODE(KEYCODE_F9)  /* F9 */
	PORT_BIT(0x00001000, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_NAME("F10")  PORT_CODE(KEYCODE_F10)  /* F10 */
		// 0x6d reserved
		// 0x6e reserved
		// 0x6f reserved
	PORT_BIT(0x00010000, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_NAME("Shift")  PORT_CODE(KEYCODE_LSHIFT)  /* Shift */
	PORT_BIT(0x00020000, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_NAME("Ctrl")  PORT_CODE(KEYCODE_LCONTROL)  /* Ctrl */
	PORT_BIT(0x00040000, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_NAME("Opt. 1")  PORT_CODE(KEYCODE_PRTSCR) /* Opt1 */
	PORT_BIT(0x00080000, IP_ACTIVE_HIGH, IPT_KEYBOARD)  PORT_NAME("Opt. 2")  PORT_CODE(KEYCODE_PAUSE)  /* Opt2 */

	PORT_START_TAG("options")
	PORT_CONFNAME(0x01, 0x01, "Enable partial updates")
	PORT_CONFSETTING( 0x00, DEF_STR( Off ))
	PORT_CONFSETTING( 0x01, DEF_STR( On ))
	PORT_CONFNAME(0x02, 0x02, "Enable fake bus errors")
	PORT_CONFSETTING( 0x00, DEF_STR( Off ))
	PORT_CONFSETTING( 0x02, DEF_STR( On ))

	PORT_START_TAG("mouse1")  // mouse buttons
	PORT_BIT(0x00000001, IP_ACTIVE_HIGH, IPT_BUTTON9) PORT_NAME("Left mouse button") PORT_CODE(MOUSECODE_BUTTON1)
	PORT_BIT(0x00000002, IP_ACTIVE_HIGH, IPT_BUTTON10) PORT_NAME("Right mouse button") PORT_CODE(MOUSECODE_BUTTON2)

	PORT_START_TAG("mouse2")  // X-axis
	PORT_BIT( 0xff, 0x00, IPT_MOUSE_X) PORT_SENSITIVITY(100) PORT_KEYDELTA(0) PORT_PLAYER(1)

	PORT_START_TAG("mouse3")  // Y-axis
	PORT_BIT( 0xff, 0x00, IPT_MOUSE_Y) PORT_SENSITIVITY(100) PORT_KEYDELTA(0) PORT_PLAYER(1)

INPUT_PORTS_END

static void dimdsk_set_geometry(mess_image* image)
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

static DEVICE_LOAD( x68k_floppy )
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
			cpunum_set_input_line_and_vector(Machine, 0,1,ASSERT_LINE,current_vector[1]);  // Disk insert/eject interrupt
			logerror("IOC: Disk image inserted\n");
		}
		sys.fdc.disk_inserted[image_index_in_device(image)] = 1;
		return INIT_PASS;
	}

	return INIT_FAIL;
}

static DEVICE_UNLOAD( x68k_floppy )
{
	if(sys.ioc.irqstatus & 0x02)
	{
		current_vector[1] = 0x61;
		sys.ioc.irqstatus |= 0x40;
		current_irq_line = 1;
		cpunum_set_input_line_and_vector(Machine, 0,1,ASSERT_LINE,current_vector[1]);  // Disk insert/eject interrupt
	}
	sys.fdc.disk_inserted[image_index_in_device(image)] = 0;
}

static void x68k_floppy_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
	case DEVINFO_INT_COUNT:
		info->i = 4;
		break;
	case DEVINFO_PTR_LOAD:
		info->load = device_load_x68k_floppy;
		break;
	case DEVINFO_PTR_UNLOAD:
		info->unload = device_unload_x68k_floppy;
		break;
	case DEVINFO_STR_FILE_EXTENSIONS:
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
	UINT8* romdata = memory_region(REGION_USER2);
	attotime irq_time;

	memset(mess_ram,0,mess_ram_size);
	memcpy(mess_ram,romdata,8);

	// init keyboard
	sys.keyboard.delay = 500;  // 3*100+200
	sys.keyboard.repeat = 110;  // 4^2*5+30

	// check for disks
	for(drive=0;drive<4;drive++)
	{
		if(image_exists(image_from_devtype_and_index(IO_FLOPPY,drive)))
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

	nec765_reset(0);
	mfp_init();

	x68k_scanline = video_screen_get_vpos(0);// = sys.crtc.reg[6];  // Vertical start

	// start VBlank timer
	sys.crtc.vblank = 1;
	irq_time = video_screen_get_time_until_pos(0,sys.crtc.reg[6],2);
	timer_adjust(vblank_irq,irq_time,0,attotime_never);

	// start HBlank timer
	timer_adjust(scanline_timer,video_screen_get_scan_period(0),1,attotime_never);

	sys.mfp.gpio = 0xfb;
}

static MACHINE_START( x68000 )
{
	/*  Install RAM handlers  */
	x68k_spriteram = (UINT16*)memory_region(REGION_USER1);
	memory_install_read16_handler(0,ADDRESS_SPACE_PROGRAM,0x000000,mess_ram_size-1,mess_ram_size-1,0,(read16_handler)1);
	memory_install_write16_handler(0,ADDRESS_SPACE_PROGRAM,0x000000,mess_ram_size-1,mess_ram_size-1,0,(write16_handler)1);
	memory_set_bankptr(1,mess_ram);
	memory_install_read16_handler(0,ADDRESS_SPACE_PROGRAM,0xc00000,0xdfffff,0x1fffff,0,(read16_handler)x68k_gvram_r);
	memory_install_write16_handler(0,ADDRESS_SPACE_PROGRAM,0xc00000,0xdfffff,0x1fffff,0,(write16_handler)x68k_gvram_w);
	memory_set_bankptr(2,gvram);  // so that code in VRAM is executable - needed for Terra Cresta
	memory_install_read16_handler(0,ADDRESS_SPACE_PROGRAM,0xe00000,0xe7ffff,0x07ffff,0,(read16_handler)x68k_tvram_r);
	memory_install_write16_handler(0,ADDRESS_SPACE_PROGRAM,0xe00000,0xe7ffff,0x07ffff,0,(write16_handler)x68k_tvram_w);
	memory_set_bankptr(3,tvram);  // so that code in VRAM is executable - needed for Terra Cresta
	memory_install_read16_handler(0,ADDRESS_SPACE_PROGRAM,0xed0000,0xed3fff,0x003fff,0,(read16_handler)x68k_sram_r);
	memory_install_write16_handler(0,ADDRESS_SPACE_PROGRAM,0xed0000,0xed3fff,0x003fff,0,(write16_handler)x68k_sram_w);
	memory_set_bankptr(4,generic_nvram16);  // so that code in SRAM is executable, there is an option for booting from SRAM

	// start keyboard timer
	timer_adjust(kb_timer,attotime_zero,0,ATTOTIME_IN_MSEC(5));  // every 5ms

	// start mouse timer
	timer_adjust(mouse_timer,attotime_zero,0,ATTOTIME_IN_MSEC(1));  // a guess for now
	sys.mouse.inputtype = 0;
}

static DRIVER_INIT( x68000 )
{
	unsigned char* rom = memory_region(REGION_CPU1);
	unsigned char* user2 = memory_region(REGION_USER2);
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
		unsigned char* ramptr = memory_region(REGION_USER3);
		memcpy(sram,ramptr,0x4000);
	}
#endif

	// copy last half of BIOS to a user region, to use for inital startup
	memcpy(user2,(rom+0xff0000),0x10000);

	ppi8255_init(&ppi_interface);
	nec765_init(&fdc_interface,NEC72065);
	hd63450_init(&dmac_interface);
	nec765_reset(0);
	mfp_init();
	scc_init(&scc_interface);
	rp5c15_init(&rtc_intf);

	memset(&sys,0,sizeof(sys));

	mfp68901_config(0,&mfp_interface);
	cpunum_set_irq_callback(0, x68k_int_ack);

	// init keyboard
	sys.keyboard.delay = 500;  // 3*100+200
	sys.keyboard.repeat = 110;  // 4^2*5+30
	kb_timer = timer_alloc(x68k_keyboard_poll,NULL);
	scanline_timer = timer_alloc(x68k_hsync,NULL);
	raster_irq = timer_alloc(x68k_crtc_raster_irq,NULL);
	vblank_irq = timer_alloc(x68k_crtc_vblank_irq,NULL);
	mouse_timer = timer_alloc(x68k_scc_ack,NULL);
}


static MACHINE_DRIVER_START( x68000 )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M68000, 10000000)  /* 10 MHz */
	MDRV_CPU_PROGRAM_MAP(x68k_map, 0)
	MDRV_CPU_VBLANK_INT(x68k_vsync_irq,1)
	MDRV_SCREEN_REFRESH_RATE(55.45)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_START( x68000 )
	MDRV_MACHINE_RESET( x68000 )

    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
//  MDRV_GFXDECODE(x68k)
	MDRV_SCREEN_SIZE(1096, 568)  // inital setting
	MDRV_SCREEN_VISIBLE_AREA(0, 767, 0, 511)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_PALETTE_LENGTH(65536)
	MDRV_COLORTABLE_LENGTH(65536 + 512)
	MDRV_PALETTE_INIT( x68000 )

	MDRV_VIDEO_START( x68000 )
	MDRV_VIDEO_UPDATE( x68000 )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(YM2151, 4000000)
	MDRV_SOUND_CONFIG(ym2151_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
//  MDRV_SOUND_ADD(OKIM6295, 0)
//  MDRV_SOUND_CONFIG(oki_interface)
//  MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	MDRV_NVRAM_HANDLER( generic_0fill )
MACHINE_DRIVER_END


SYSTEM_CONFIG_START(x68000)
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
	ROM_REGION16_BE(0x1000000, REGION_CPU1, 0)  // 16MB address space
	ROM_LOAD( "cgrom.dat",  0xf00000, 0xc0000, CRC(9f3195f1) SHA1(8d72c5b4d63bb14c5dbdac495244d659aa1498b6) )
	ROM_SYSTEM_BIOS(0, "ipl10",  "IPL-ROM V1.0")
	ROMX_LOAD( "iplrom.dat", 0xfe0000, 0x20000, CRC(72bdf532) SHA1(0ed038ed2133b9f78c6e37256807424e0d927560), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS(1, "ipl11",  "IPL-ROM V1.1")
	ROMX_LOAD( "iplromxv.dat", 0xfe0000, 0x020000, CRC(00eeb408) SHA1(e33cdcdb69cd257b0b211ef46e7a8b144637db57), ROM_BIOS(2) )
	ROM_SYSTEM_BIOS(2, "ipl12",  "IPL-ROM V1.2")
	ROMX_LOAD( "iplromco.dat", 0xfe0000, 0x020000, CRC(6c7ef608) SHA1(77511fc58798404701f66b6bbc9cbde06596eba7), ROM_BIOS(3) )
	ROM_SYSTEM_BIOS(3, "ipl13",  "IPL-ROM V1.3 (92/11/27)")
	ROMX_LOAD( "iplrom30.dat", 0xfe0000, 0x020000, CRC(e8f8fdad) SHA1(239e9124568c862c31d9ec0605e32373ea74b86a), ROM_BIOS(4) )
	ROM_REGION(0x8000, REGION_USER1,0)  // For Background/Sprite decoding
	ROM_FILL(0x0000,0x8000,0x00)
	ROM_REGION(0x20000, REGION_USER2, 0)
	ROM_FILL(0x000,0x20000,0x00)
ROM_END


/*    YEAR  NAME    PARENT  COMPAT  MACHINE INPUT   INIT    CONFIG  COMPANY     FULLNAME        FLAGS */
COMP( 1987, x68000, 0,      0,      x68000, x68000, x68000, x68000, "Sharp",    "Sharp X68000", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS )

/***********************************************************************************************

	Fujitsu Micro 7 (FM-7)

	12/05/2009 Skeleton driver.
	
	Computers in this series:
	
                 | Release |    Main CPU    |  Sub CPU  |              RAM              |
    =====================================================================================
    FM-8         | 1981-05 | M68A09 @ 1MHz  |  M6809    |    64K (main) + 48K (VRAM)    |
    FM-7         | 1982-11 | M68B09 @ 2MHz  |  M68B09   |    64K (main) + 48K (VRAM)    |
    FM-NEW7      | 1984-05 | M68B09 @ 2MHz  |  M68B09   |    64K (main) + 48K (VRAM)    |
    FM-77        | 1984-05 | M68B09 @ 2MHz  |  M68B09E  |  64/256K (main) + 48K (VRAM)  |
    FM-77AV      | 1985-10 | M68B09E @ 2MHz |  M68B09   | 128/192K (main) + 96K (VRAM)  |
    FM-77AV20    | 1986-10 | M68B09E @ 2MHz |  M68B09   | 128/192K (main) + 96K (VRAM)  |
    FM-77AV40    | 1986-10 | M68B09E @ 2MHz |  M68B09   | 192/448K (main) + 144K (VRAM) |
    FM-77AV20EX  | 1987-11 | M68B09E @ 2MHz |  M68B09   | 128/192K (main) + 96K (VRAM)  |
    FM-77AV40EX  | 1987-11 | M68B09E @ 2MHz |  M68B09   | 192/448K (main) + 144K (VRAM) |
    FM-77AV40SX  | 1988-11 | M68B09E @ 2MHz |  M68B09   | 192/448K (main) + 144K (VRAM) |

	Note: FM-77AV dumps probably come from a FM-77AV40SX. Shall we confirm that both computers
	used the same BIOS components?

	memory map info from http://www.nausicaa.net/~lgreenf/fm7page.htm
	see also http://retropc.net/ryu/xm7/xm7.shtml

************************************************************************************************/

#include "driver.h"
#include "cpu/m6809/m6809.h"
#include "sound/ay8910.h"
#include "sound/wave.h"

#include "devices/cassette.h"
#include "formats/fm7_cas.h"

// Interrupt flags
#define IRQ_FLAG_KEY      0x01
#define IRQ_FLAG_PRINTER  0x02
#define IRQ_FLAG_TIMER    0x04
#define IRQ_FLAG_UNKNOWN  0x08
// the following are not read in port 0xfd03
#define IRQ_FLAG_MFD      0x10
#define IRQ_FLAG_TXRDY    0x20
#define IRQ_FLAG_RXRDY    0x40
#define IRQ_FLAG_SYNDET   0x80

UINT8* shared_ram;
UINT8* fm7_video_ram;
static UINT8 irq_flags;  // active IRQ flags
static UINT8 irq_mask;  // IRQ mask 
emu_timer* fm7_timer;  // main timer, triggered every 2.0345ms?
emu_timer* fm7_subtimer;  // sub-CPU timer, every 1 second?
emu_timer* fm7_keyboard_timer;
static UINT8 sub_busy;
static UINT8 sub_halt;
static UINT8 basic_rom_en;
static UINT8 attn_irq;
static UINT8 vram_access;  // VRAM access flag
static UINT8 crt_enable;
unsigned int key_delay;
unsigned int key_repeat;
static UINT16 current_scancode;
static UINT32 key_data[4];
static UINT16 vram_offset;
static UINT8 break_flag;
static UINT8 fm7_pal[8];
static UINT8 fm7_psg_regsel;
static UINT8 psg_data;

/* key scancode conversion table
 * The FM-7 expects different scancodes when shift,ctrl or graph is held, or
 * when kana is active.
 */
 // TODO: fill in shift,ctrl,graph and kana code
const UINT16 fm7_key_list[0x60][5] =  
{ // norm  shift ctrl  graph kana
	{0x00, 0x00, 0x00, 0x00, 0x00},
	{0x1b, 0x1b, 0x1b, 0x1b, 0x1b},  // ESC
	{0x31, 0x21, 0x00, 0x00, 0x00},
	{0x32, 0x22, 0x00, 0x00, 0x00},
	{0x33, 0x23, 0x00, 0x00, 0x00},
	{0x34, 0x24, 0x00, 0x00, 0x00},
	{0x35, 0x25, 0x00, 0x00, 0x00},
	{0x36, 0x26, 0x00, 0x00, 0x00},
	{0x37, 0x27, 0x00, 0x00, 0x00},
	{0x38, 0x28, 0x00, 0x00, 0x00},
	{0x39, 0x29, 0x00, 0x00, 0x00},
	{0x30, 0x00, 0x00, 0x00, 0x00},
	{0x2d, 0x3d, 0x00, 0x00, 0x00},
	{0x5e, 0x7e, 0x00, 0x00, 0x00},
	{0x5c, 0x7c, 0x00, 0x00, 0x00},
	{0x08, 0x08, 0x08, 0x08, 0x08},
	{0x09, 0x09, 0x09, 0x09, 0x09},
	{0x71, 0x51, 0x00, 0x00, 0x00},
	{0x77, 0x57, 0x00, 0x00, 0x00},
	{0x65, 0x45, 0x00, 0x00, 0x00},
	{0x72, 0x52, 0x00, 0x00, 0x00},
	{0x74, 0x54, 0x00, 0x00, 0x00},
	{0x79, 0x59, 0x00, 0x00, 0x00},
	{0x75, 0x55, 0x00, 0x00, 0x00},
	{0x69, 0x49, 0x00, 0x00, 0x00},
	{0x6f, 0x4f, 0x00, 0x00, 0x00},
	{0x70, 0x50, 0x00, 0x00, 0x00},
	{0x40, 0x60, 0x00, 0x00, 0x00},
	{0x5b, 0x7b, 0x00, 0x00, 0x00},
	{0x0d, 0x0d, 0x0d, 0x0d, 0x0d},
	{0x61, 0x41, 0x00, 0x00, 0x00},
	{0x73, 0x53, 0x00, 0x00, 0x00},

	{0x64, 0x44, 0x00, 0x00, 0x00},
	{0x66, 0x46, 0x00, 0x00, 0x00},
	{0x67, 0x47, 0x00, 0x00, 0x00},
	{0x68, 0x48, 0x00, 0x00, 0x00},
	{0x6a, 0x4a, 0x00, 0x00, 0x00},
	{0x6b, 0x4b, 0x00, 0x00, 0x00},
	{0x6c, 0x4c, 0x00, 0x00, 0x00},
	{0x3b, 0x2b, 0x00, 0x00, 0x00},
	{0x3a, 0x2a, 0x00, 0x00, 0x00},
	{0x5d, 0x7d, 0x00, 0x00, 0x00},
	{0x7a, 0x5a, 0x00, 0x00, 0x00},
	{0x78, 0x58, 0x00, 0x00, 0x00},
	{0x63, 0x43, 0x00, 0x00, 0x00},
	{0x76, 0x56, 0x00, 0x00, 0x00},
	{0x62, 0x42, 0x00, 0x00, 0x00},
	{0x6e, 0x4e, 0x00, 0x00, 0x00},
	{0x6d, 0x4d, 0x00, 0x00, 0x00},
	{0x2c, 0x3c, 0x00, 0x00, 0x00},
	{0x2e, 0x3e, 0x00, 0x00, 0x00},
	{0x2f, 0x3f, 0x00, 0x00, 0x00},
	{0x22, 0x5f, 0x1f, 0x00, 0x00},
	{0x20, 0x20, 0x20, 0x20, 0x20},
	{0x2a, 0x2a, 0x00, 0x2a, 0x2a}, // Tenkey
	{0x2f, 0x2f, 0x00, 0x2f, 0x2f},
	{0x2b, 0x2b, 0x00, 0x2b, 0x2b},
	{0x2d, 0x2d, 0x00, 0x2d, 0x2d},
	{0x37, 0x37, 0x00, 0x37, 0x37},  
	{0x38, 0x38, 0x00, 0x38, 0x38},
	{0x39, 0x39, 0x00, 0x39, 0x39},
	{0x3d, 0x3d, 0x00, 0x3d, 0x3d},
	{0x34, 0x34, 0x00, 0x34, 0x34},
	{0x35, 0x35, 0x00, 0x35, 0x35},

	{0x36, 0x36, 0x00, 0x36, 0x36},
	{0x2c, 0x2c, 0x00, 0x2c, 0x2c},
	{0x31, 0x31, 0x00, 0x31, 0x31},
	{0x32, 0x32, 0x00, 0x32, 0x32},
	{0x33, 0x33, 0x00, 0x33, 0x33},
	{0x0d, 0x0d, 0x00, 0x0d, 0x0d},
	{0x30, 0x30, 0x00, 0x30, 0x30},
	{0x2e, 0x2e, 0x00, 0x2e, 0x2e},
	{0x12, 0x12, 0x00, 0x12, 0x12}, // INS
	{0x05, 0x05, 0x00, 0x05, 0x05},  // EL
	{0x0c, 0x0c, 0x00, 0x0c, 0x0c},  // CLS
	{0x7f, 0x7f, 0x00, 0x7f, 0x7f},  // DEL
	{0x11, 0x11, 0x00, 0x11, 0x11},  // DUP
	{0x1e, 0x19, 0x00, 0x1e, 0x1e},  // Cursor Up
	{0x0b, 0x0b, 0x00, 0x0b, 0x0b},  // HOME
	{0x1d, 0x02, 0x00, 0x1d, 0x1d},  // Cursor Left
	{0x1f, 0x1a, 0x00, 0x1f, 0x1f},  // Cursor Down
	{0x1c, 0x06, 0x00, 0x1c, 0x1c},  // Cursor Right
	{0x00, 0x00, 0x00, 0x00, 0x00},  // BREAK (doesn't give a scancode)
	{0x101, 0x00, 0x101, 0x101, 0x101},  // PF1
	{0x102, 0x00, 0x102, 0x102, 0x102},
	{0x103, 0x00, 0x103, 0x103, 0x103},
	{0x104, 0x00, 0x104, 0x104, 0x104},
	{0x105, 0x00, 0x105, 0x105, 0x105},
	{0x106, 0x00, 0x106, 0x106, 0x106},
	{0x107, 0x00, 0x107, 0x107, 0x107},
	{0x108, 0x00, 0x108, 0x108, 0x108},
	{0x109, 0x00, 0x109, 0x109, 0x109},
	{0x10a, 0x00, 0x10a, 0x10a, 0x10a},  // PF10
	{0x00, 0x00, 0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00, 0x00, 0x00}
};

/*
 * I/O port 0xfd02
 * 
 * On read: returns cassette data (bit 7) and printer status (bits 0-5)
 * On write: sets IRQ masks 
 *   bit 0 - keypress
 *   bit 1 - printer
 *   bit 2 - timer
 *   bit 3 - not used
 *   bit 4 - MFD
 *   bit 5 - TXRDY
 *   bit 6 - RXRDY
 *   bit 7 - SYNDET
 * 
 */
WRITE8_HANDLER( fm7_irq_mask_w )
{
	irq_mask = data;
	logerror("IRQ mask set: 0x%02x\n",irq_mask);
}

/*
 * I/O port 0xfd03
 * 
 * On read: returns which IRQ is currently active (typically read by IRQ handler)
 *   bit 0 - keypress
 *   bit 1 - printer
 *   bit 2 - timer
 *   bit 3 - ???
 * On write: Buzzer/Speaker On/Off
 *   bit 0 - speaker on/off
 *   bit 1 - ??buzzer on/off
 *   bit 2 - ??buzzer on/off
 */
READ8_HANDLER( fm7_irq_cause_r )
{
	UINT8 ret = irq_flags;
	
	irq_flags = 0;  // clear flags
	logerror("IRQ flags read: 0x%02x\n",ret);
	return ret;
}

READ8_HANDLER( vector_r )
{
	UINT8* RAM = memory_region(space->machine,"maincpu");

	return RAM[0xfff0+offset];
}

WRITE8_HANDLER( vector_w )
{
	UINT8* RAM = memory_region(space->machine,"maincpu");
	
	RAM[0xfff0+offset] = data;
}

READ8_HANDLER( fm7_fd04_r )
{
	UINT8 ret = 0xff;
	
	if(attn_irq != 0)
	{
		ret &= ~0x01;
		attn_irq = 0;
	}
	if(break_flag != 0)
	{
		ret &= ~0x02;
	}
	return ret;
}
/*
 * Sub-CPU interface (port 0xfd05)
 * 
 * Read:
 *   bit 7: Sub-CPU busy
 *   bit 0: EXTDET (?)
 * Write:
 *   bit 7: Sub-CPU halt
 *   bit 6: Sub-CPU cancel IRQ
 */

READ8_HANDLER( fm7_subintf_r )
{
	UINT8 ret = 0;
	
	if(sub_busy != 0 || sub_halt != 0)
		ret |= 0x80;
		
	return ret;
}

WRITE8_HANDLER( fm7_subintf_w )
{
	sub_halt = data & 0x80;
	cputag_set_input_line(space->machine,"sub",INPUT_LINE_HALT,(data & 0x80) ? ASSERT_LINE : CLEAR_LINE);
	if(data & 0x40)
		cputag_set_input_line(space->machine,"sub",M6809_IRQ_LINE,ASSERT_LINE);
	//popmessage("Sub CPU Interface write: %02x\n",data);
}

READ8_HANDLER( fm7_sub_busyflag_r )
{
	if(sub_halt == 0)
		sub_busy = 0x00;
	return 0x00;
}

WRITE8_HANDLER( fm7_sub_busyflag_w )
{
	sub_busy = 0x80;
}

READ8_HANDLER( fm7_rom_en_r )
{
	UINT8* RAM = memory_region(space->machine,"maincpu");
	
	basic_rom_en = 1;
	memory_set_bankptr(space->machine,1,RAM+0x38000);
	logerror("BASIC ROM enabled\n");
	return 0x00;
}

WRITE8_HANDLER( fm7_rom_en_w )
{
	UINT8* RAM = memory_region(space->machine,"maincpu");
	
	basic_rom_en = 0;
	memory_set_bankptr(space->machine,1,RAM+0x8000);
	logerror("BASIC ROM disabled\n");
}

/*
 * Sub-CPU port 0xd402
 *   Read-only: Acknowledge Cancel IRQ
 */
READ8_HANDLER( fm7_cancel_ack )
{
	cputag_set_input_line(space->machine,"sub",M6809_IRQ_LINE,CLEAR_LINE);
	return 0x00;
}

/*
 * Reading from 0xd404 (sub-CPU) causes an "Attention" FIRQ on the main CPU 
 */
READ8_HANDLER( fm7_attn_irq_r )
{
	attn_irq = 1;
	cputag_set_input_line(space->machine,"maincpu",M6809_FIRQ_LINE,ASSERT_LINE);
	return 0xff;
}

READ8_HANDLER( fm7_vram_access_r )
{
	vram_access = 1;
	return 0xff;
}

WRITE8_HANDLER( fm7_vram_access_w )
{
	vram_access = 0;
}

READ8_HANDLER( fm7_vram_r )
{
	int offs;
	
	offs = (offset & 0xc000) | ((offset + vram_offset) & 0x3fff);
	return fm7_video_ram[offs];
}

WRITE8_HANDLER( fm7_vram_w )
{
	int offs;
	
	offs = (offset & 0xc000) | ((offset + vram_offset) & 0x3fff);
	if(vram_access != 0)
		fm7_video_ram[offs] = data;
}

READ8_HANDLER( fm7_crt_r )
{
	crt_enable = 1;
	return 0xff;
}

WRITE8_HANDLER( fm7_crt_w )
{
	crt_enable = 0;
}

READ8_HANDLER( fm7_fdc_r )
{
	switch(offset)
	{
		case 0:
			return 0x80;  // drive not ready
		default:
			logerror("FDC: read from 0x%04x\n",offset+0xfd18);
	}
	return 0x00;
}

WRITE8_HANDLER( fm7_fdc_w )
{
	switch(offset)
	{
		default:
			logerror("FDC: wrote %02x to 0x%04x\n",data,offset+0xfd18);
	}
}

WRITE8_HANDLER( fm7_vram_offset_w )
{
	UINT16 new_offset = 0;
	
	switch(offset)
	{
		case 0:
			new_offset = (data << 8) | (vram_offset & 0x00ff); 
			break;
		case 1:  // low 5 bits are used on FM-77AV and later only
			new_offset = (vram_offset & 0xff00) | (data & 0xe0);
			break;
	}
	
	vram_offset = new_offset;
}

READ8_HANDLER( fm7_keyboard_r)
{
	UINT8 ret;
	switch(offset)
	{
		case 0:
			ret = (current_scancode >> 1) & 0x80;
			ret |= 0x01; // 1 = 2MHz, 0 = 1.2MHz
			return ret;
		case 1:
			return current_scancode & 0xff;
		default:
			return 0x00;
	}
}

READ8_HANDLER( fm7_sub_keyboard_r)
{
	UINT8 ret;
	switch(offset)
	{
		case 0:
			ret = (current_scancode >> 1) & 0x80;
			return ret;
		case 1:
			return current_scancode & 0xff;
		default:
			return 0x00;
	}
}

READ8_HANDLER( fm7_cassette_printer_r )
{
	// bit 7: cassette input
	// bit 3: printer PE
	// bit 2: printer acknowledge
	// bit 1: printer error
	// bit 0: printer busy
	UINT8 ret = 0x00;
	double data = cassette_input(devtag_get_device(space->machine,"cass")); 
	
	if(data > 0.03)
		ret |= 0x80;
	
	ret |= 0x40;
	
	ret |= 0x3f;  // Printer status, joystick inputs

	return ret;
}

WRITE8_HANDLER( fm7_cassette_printer_w )
{
	static UINT8 prev;
	switch(offset)
	{
		case 0:
		// bit 7: printer online
		// bit 6: printer strobe
		// bit 1: cassette motor
		// bit 0: cassette output
			if((data & 0x01) != (prev & 0x01))
				cassette_output(devtag_get_device(space->machine,"cass"),(data & 0x01) ? +1.0 : -1.0);
			if((data & 0x02) != (prev & 0x02))
				cassette_change_state(devtag_get_device(space->machine, "cass" ),(data & 0x02) ? CASSETTE_MOTOR_ENABLED : CASSETTE_MOTOR_DISABLED,CASSETTE_MASK_MOTOR);
			prev = data;
			break;
		case 1:
		// Printer data
			break;
	}
}

static READ8_HANDLER( fm7_palette_r)
{
	return fm7_pal[offset];
}

static WRITE8_HANDLER( fm7_palette_w )
{
	UINT8 r = 0,g = 0,b = 0;
	
	if(data & 0x04)
		g = 0xff;
	if(data & 0x02)
		r = 0xff;
	if(data & 0x01)
		b = 0xff;
		
	palette_set_color(space->machine,offset,MAKE_RGB(r,g,b));
	fm7_pal[offset] = data & 0x07;
}

static void fm7_update_psg(running_machine* machine)
{
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);

	switch(fm7_psg_regsel)
	{
		case 0x00:
			// High impedance
			break;
		case 0x01:
			// Data read
			psg_data = ay8910_r(devtag_get_device(space->machine,"psg"),0);
			break;
		case 0x02:
			// Data write
			ay8910_data_w(devtag_get_device(space->machine,"psg"),0,psg_data);
			break;
		case 0x03:
			// Address latch
			ay8910_address_w(devtag_get_device(space->machine,"psg"),0,psg_data);
			break;
	}
}

static READ8_HANDLER( fm7_psg_select_r )
{
	return 0xff;
}

static WRITE8_HANDLER( fm7_psg_select_w )
{
	/* 
	 * bit 0 = BC1
	 * bit 1 = BDIR
	 */
	fm7_psg_regsel = data & 0x03;
	fm7_update_psg(space->machine);
}

static READ8_HANDLER( fm7_psg_data_r )
{
	fm7_update_psg(space->machine);
	return psg_data;
}

static WRITE8_HANDLER( fm7_psg_data_w )
{
	psg_data = data;
	fm7_update_psg(space->machine);
}

// Shared RAM is only usable on the main CPU if the sub CPU is halted
static READ8_HANDLER( fm7_main_shared_r )
{
	if(sub_halt != 0)
		return shared_ram[offset];
	else
		return 0xff;
}

static WRITE8_HANDLER( fm7_main_shared_w )
{
	if(sub_halt != 0)
		shared_ram[offset] = data;
}

static READ8_HANDLER( fm7_unknown_r )
{
	// Port 0xFDFC is read by Dig Dug.  Controller port, perhaps?
	// Must return 0xff for it to read the keyboard.
	// Mappy uses ports FD15 and FD16.  On the FM77AV, this is the YM2203,
	// but on the FM-7, this is nothing, so we return 0xff for it to
	// read the keyboard correctly.
	return 0xff;
}

static TIMER_CALLBACK( fm7_timer_irq )
{
	if(irq_mask & IRQ_FLAG_TIMER)
	{
		irq_flags |= IRQ_FLAG_TIMER;
		cputag_set_input_line(machine,"maincpu",M6809_IRQ_LINE,ASSERT_LINE);
	}
}

static TIMER_CALLBACK( fm7_subtimer_irq )
{
	cputag_set_input_line(machine,"sub",INPUT_LINE_NMI,PULSE_LINE);
}

// When a key is pressed or released, an IRQ is generated on the main CPU,
// and an FIRQ on the sub CPU.  Both CPUs have ports to read keyboard data.
// Scancodes are 9 bits.
void key_press(running_machine* machine, UINT16 scancode)
{
	current_scancode = scancode;

	if(scancode == 0)
		return;
	
	if(irq_mask & IRQ_FLAG_KEY)
	{
		irq_flags |= IRQ_FLAG_KEY;
		cputag_set_input_line(machine,"maincpu",M6809_IRQ_LINE,ASSERT_LINE);
	}
	else
	{
		cputag_set_input_line(machine,"sub",M6809_FIRQ_LINE,ASSERT_LINE);
	}
	logerror("KEY: sent scancode 0x%03x\n",scancode);
}

static TIMER_CALLBACK( fm7_keyboard_poll )
{
	const char* portnames[3] = { "key1","key2","key3" };
	int x,y;
	int bit = 0;
	int mod = 0;
	UINT32 keys;
	UINT32 modifiers = input_port_read(machine,"key_modifiers");
	
	// check key modifiers (Shift, Ctrl, Kana, etc...)
	if(modifiers & 0x02)
		mod = 1;  // shift
	if(modifiers & 0x01)
		mod = 2;  // ctrl (overrides shift, if also pressed)
	
	if(input_port_read(machine,"key3") & 0x40000)
	{
		break_flag = 1;
		cputag_set_input_line(machine,"maincpu",M6809_FIRQ_LINE,ASSERT_LINE);
	}
	else
		break_flag = 0;
		
	for(x=0;x<3;x++)
	{
		keys = input_port_read(machine,portnames[x]);
		
		for(y=0;y<32;y++)  // loop through each bit in the port
		{
			if((keys & (1<<y)) != 0 && (key_data[x] & (1<<y)) == 0)
			{
				key_press(machine,fm7_key_list[bit][mod]); // key press
			}
			bit++; 
		}
		
		key_data[x] = keys;
	}
}

static IRQ_CALLBACK(fm7_irq_ack)
{
	cputag_set_input_line(device->machine,"maincpu",irqline,CLEAR_LINE);
	return -1;
}

static IRQ_CALLBACK(fm7_sub_irq_ack)
{
	cputag_set_input_line(device->machine,"sub",irqline,CLEAR_LINE);
	return -1;
}

/*
   0000 - 7FFF: (RAM) BASIC working area, user's area
   8000 - FBFF: (ROM) F-BASIC ROM
   FC00 - FC7F: Shared RAM between main and sub CPU
   FD00 - FDFF: I/O space (6809 uses memory-mapped I/O)
   FE00 - FFEF: Boot rom
   FFF0 - FFFF: Interrupt vector table
*/
// The FM-7 has only 64kB RAM, so we'll worry about banking when we do the later models
static ADDRESS_MAP_START( fm7_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000,0x7fff) AM_RAM 
	AM_RANGE(0x8000,0xfbff) AM_RAMBANK(1) // also F-BASIC ROM, when enabled
	AM_RANGE(0xfc00,0xfc7f) AM_RAM
	AM_RANGE(0xfc80,0xfcff) AM_RAM AM_READWRITE(fm7_main_shared_r,fm7_main_shared_w)
	// I/O space (FD00-FDFF)
	AM_RANGE(0xfd00,0xfd01) AM_READWRITE(fm7_keyboard_r,fm7_cassette_printer_w)
	AM_RANGE(0xfd02,0xfd02) AM_READWRITE(fm7_cassette_printer_r,fm7_irq_mask_w)  // IRQ mask
	AM_RANGE(0xfd03,0xfd03) AM_READ(fm7_irq_cause_r)  // IRQ flags
	AM_RANGE(0xfd04,0xfd04) AM_READ(fm7_fd04_r)
	AM_RANGE(0xfd05,0xfd05) AM_READWRITE(fm7_subintf_r,fm7_subintf_w)
	AM_RANGE(0xfd07,0xfd07) AM_RAM
	AM_RANGE(0xfd0d,0xfd0d) AM_READWRITE(fm7_psg_select_r,fm7_psg_select_w)
	AM_RANGE(0xfd0e,0xfd0e) AM_READWRITE(fm7_psg_data_r, fm7_psg_data_w)
	AM_RANGE(0xfd0f,0xfd0f) AM_READWRITE(fm7_rom_en_r,fm7_rom_en_w)
	AM_RANGE(0xfd15,0xfd16) AM_READ(fm7_unknown_r)
	AM_RANGE(0xfd18,0xfd1f) AM_READWRITE(fm7_fdc_r,fm7_fdc_w)
	AM_RANGE(0xfd24,0xfd2b) AM_RAM
	AM_RANGE(0xfd38,0xfd3f) AM_READWRITE(fm7_palette_r,fm7_palette_w)
	AM_RANGE(0xfdf0,0xfdff) AM_READ(fm7_unknown_r)
	// Boot ROM
	AM_RANGE(0xfe00,0xffdf) AM_ROM AM_REGION("basic",0x0000)
	AM_RANGE(0xffe0,0xffef) AM_RAM
	AM_RANGE(0xfff0,0xffff) AM_READWRITE(vector_r,vector_w) 
ADDRESS_MAP_END

/*
   0000 - 3FFF: Video RAM bank 0 (Blue plane)
   4000 - 7FFF: Video RAM bank 1 (Red plane)
   8000 - BFFF: Video RAM bank 2 (Green plane)
   C000 - C2FF: (RAM) working area
   C300 - C37F: Shared RAM between main and sub CPU
   C400 - FFDF: (ROM) Graphics command code
   FFF0 - FFFF: Interrupt vector table
*/

static ADDRESS_MAP_START( fm7_sub_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000,0xbfff) AM_READWRITE(fm7_vram_r,fm7_vram_w) // VRAM
	AM_RANGE(0xc000,0xcfff) AM_RAM // Console RAM
	AM_RANGE(0xd000,0xd37f) AM_RAM // Work RAM
	AM_RANGE(0xd380,0xd3ff) AM_RAM AM_BASE(&shared_ram)
	// I/O space (D400-D7FF)
	AM_RANGE(0xd400,0xd401) AM_READ(fm7_sub_keyboard_r)
	AM_RANGE(0xd402,0xd402) AM_READ(fm7_cancel_ack)
	AM_RANGE(0xd404,0xd404) AM_READ(fm7_attn_irq_r)
	AM_RANGE(0xd408,0xd408) AM_READWRITE(fm7_crt_r,fm7_crt_w)
	AM_RANGE(0xd409,0xd409) AM_READWRITE(fm7_vram_access_r,fm7_vram_access_w)
	AM_RANGE(0xd40a,0xd40a) AM_READWRITE(fm7_sub_busyflag_r,fm7_sub_busyflag_w)
	AM_RANGE(0xd40e,0xd40f) AM_WRITE(fm7_vram_offset_w)
	AM_RANGE(0xd800,0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( fm77av_mem, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

static ADDRESS_MAP_START( fm77av_sub_mem, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000,0xbfff) AM_RAM AM_BASE(&fm7_video_ram) // VRAM
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( fm7 )

  PORT_START("key1")
  PORT_BIT(0x00000001,IP_ACTIVE_HIGH,IPT_UNUSED)
  PORT_BIT(0x00000002,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("ESC") PORT_CODE(KEYCODE_TILDE) PORT_CHAR(27)
  PORT_BIT(0x00000004,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("1") PORT_CODE(KEYCODE_1) PORT_CHAR('1')
  PORT_BIT(0x00000008,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("2") PORT_CODE(KEYCODE_2) PORT_CHAR('2')
  PORT_BIT(0x00000010,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("3") PORT_CODE(KEYCODE_3) PORT_CHAR('3')
  PORT_BIT(0x00000020,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("4") PORT_CODE(KEYCODE_4) PORT_CHAR('4')
  PORT_BIT(0x00000040,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("5") PORT_CODE(KEYCODE_5) PORT_CHAR('5')
  PORT_BIT(0x00000080,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("6") PORT_CODE(KEYCODE_6) PORT_CHAR('6')
  PORT_BIT(0x00000100,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("7") PORT_CODE(KEYCODE_7) PORT_CHAR('7')
  PORT_BIT(0x00000200,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("8") PORT_CODE(KEYCODE_8) PORT_CHAR('8')
  PORT_BIT(0x00000400,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("9") PORT_CODE(KEYCODE_9) PORT_CHAR('9')
  PORT_BIT(0x00000800,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("0") PORT_CODE(KEYCODE_0) PORT_CHAR('0')
  PORT_BIT(0x00001000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("-") PORT_CODE(KEYCODE_MINUS) PORT_CHAR('-')
  PORT_BIT(0x00002000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("^") PORT_CODE(KEYCODE_EQUALS) PORT_CHAR('^')
  PORT_BIT(0x00004000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("\xEF\xBF\xA5")
  PORT_BIT(0x00008000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Backspace") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8)
  PORT_BIT(0x00010000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Tab") PORT_CODE(KEYCODE_TAB) PORT_CHAR(9)
  PORT_BIT(0x00020000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Q") PORT_CODE(KEYCODE_Q) PORT_CHAR('Q')
  PORT_BIT(0x00040000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("W") PORT_CODE(KEYCODE_W) PORT_CHAR('W')
  PORT_BIT(0x00080000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("E") PORT_CODE(KEYCODE_E) PORT_CHAR('E')
  PORT_BIT(0x00100000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("R") PORT_CODE(KEYCODE_R) PORT_CHAR('R')
  PORT_BIT(0x00200000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("T") PORT_CODE(KEYCODE_T) PORT_CHAR('T')
  PORT_BIT(0x00400000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Y") PORT_CODE(KEYCODE_Y) PORT_CHAR('Y')
  PORT_BIT(0x00800000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("U") PORT_CODE(KEYCODE_U) PORT_CHAR('U')
  PORT_BIT(0x01000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("I") PORT_CODE(KEYCODE_I) PORT_CHAR('I')
  PORT_BIT(0x02000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("O") PORT_CODE(KEYCODE_O) PORT_CHAR('O')
  PORT_BIT(0x04000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("P") PORT_CODE(KEYCODE_P) PORT_CHAR('P')
  PORT_BIT(0x08000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("@") PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR('@')
  PORT_BIT(0x10000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("[") PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR('[')
  PORT_BIT(0x20000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("RETURN") PORT_CODE(KEYCODE_ENTER) PORT_CHAR(27)
  PORT_BIT(0x40000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("A") PORT_CODE(KEYCODE_A) PORT_CHAR('A')
  PORT_BIT(0x80000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("S") PORT_CODE(KEYCODE_S) PORT_CHAR('S')

  PORT_START("key2")
  PORT_BIT(0x00000001,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("D") PORT_CODE(KEYCODE_D) PORT_CHAR('D')
  PORT_BIT(0x00000002,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("F") PORT_CODE(KEYCODE_F) PORT_CHAR('F')
  PORT_BIT(0x00000004,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("G") PORT_CODE(KEYCODE_G) PORT_CHAR('G')
  PORT_BIT(0x00000008,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("H") PORT_CODE(KEYCODE_H) PORT_CHAR('H')
  PORT_BIT(0x00000010,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("J") PORT_CODE(KEYCODE_J) PORT_CHAR('J')
  PORT_BIT(0x00000020,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("K") PORT_CODE(KEYCODE_K) PORT_CHAR('K')
  PORT_BIT(0x00000040,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("L") PORT_CODE(KEYCODE_L) PORT_CHAR('L')
  PORT_BIT(0x00000080,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME(";") PORT_CODE(KEYCODE_COLON) PORT_CHAR(';')
  PORT_BIT(0x00000100,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME(":") PORT_CODE(KEYCODE_QUOTE) PORT_CHAR(':')
  PORT_BIT(0x00000200,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("]") PORT_CODE(KEYCODE_BACKSLASH) PORT_CHAR(']')
  PORT_BIT(0x00000400,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Z") PORT_CODE(KEYCODE_Z) PORT_CHAR('Z')
  PORT_BIT(0x00000800,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("X") PORT_CODE(KEYCODE_X) PORT_CHAR('X')
  PORT_BIT(0x00001000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("C") PORT_CODE(KEYCODE_C) PORT_CHAR('C')
  PORT_BIT(0x00002000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("V") PORT_CODE(KEYCODE_V) PORT_CHAR('V')
  PORT_BIT(0x00004000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("B") PORT_CODE(KEYCODE_B) PORT_CHAR('B')
  PORT_BIT(0x00008000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("N") PORT_CODE(KEYCODE_N) PORT_CHAR('N')
  PORT_BIT(0x00010000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("M") PORT_CODE(KEYCODE_M) PORT_CHAR('M')
  PORT_BIT(0x00020000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME(",") PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',')
  PORT_BIT(0x00040000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME(".") PORT_CODE(KEYCODE_STOP) PORT_CHAR('.')
  PORT_BIT(0x00080000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("/") PORT_CODE(KEYCODE_SLASH) PORT_CHAR('/')
  PORT_BIT(0x00100000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("_") 
  PORT_BIT(0x00200000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Space") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')
  PORT_BIT(0x00400000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Tenkey *") PORT_CODE(KEYCODE_ASTERISK)
  PORT_BIT(0x00800000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Tenkey /") PORT_CODE(KEYCODE_SLASH_PAD)
  PORT_BIT(0x01000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Tenkey +") PORT_CODE(KEYCODE_PLUS_PAD)
  PORT_BIT(0x02000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Tenkey -") PORT_CODE(KEYCODE_MINUS_PAD)
  PORT_BIT(0x04000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Tenkey 7") PORT_CODE(KEYCODE_7_PAD)
  PORT_BIT(0x08000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Tenkey 8") PORT_CODE(KEYCODE_8_PAD) 
  PORT_BIT(0x10000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Tenkey 9") PORT_CODE(KEYCODE_9_PAD)
  PORT_BIT(0x20000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Tenkey =") 
  PORT_BIT(0x40000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Tenkey 4") PORT_CODE(KEYCODE_4_PAD)
  PORT_BIT(0x80000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Tenkey 5") PORT_CODE(KEYCODE_5_PAD)

  PORT_START("key3")
  PORT_BIT(0x00000001,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Tenkey 6") PORT_CODE(KEYCODE_6_PAD)
  PORT_BIT(0x00000002,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Tenkey ,")
  PORT_BIT(0x00000004,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Tenkey 1") PORT_CODE(KEYCODE_1_PAD)
  PORT_BIT(0x00000008,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Tenkey 2") PORT_CODE(KEYCODE_2_PAD)
  PORT_BIT(0x00000010,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Tenkey 3") PORT_CODE(KEYCODE_3_PAD)
  PORT_BIT(0x00000020,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Tenkey Enter") PORT_CODE(KEYCODE_ENTER_PAD)
  PORT_BIT(0x00000040,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Tenkey 0") PORT_CODE(KEYCODE_0_PAD)
  PORT_BIT(0x00000080,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Tenkey .") PORT_CODE(KEYCODE_DEL_PAD)
  PORT_BIT(0x00000100,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("INS") PORT_CODE(KEYCODE_INSERT)
  PORT_BIT(0x00000200,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("EL") PORT_CODE(KEYCODE_PGUP)
  PORT_BIT(0x00000400,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("CLS") PORT_CODE(KEYCODE_PGDN)
  PORT_BIT(0x00000800,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("DEL") PORT_CODE(KEYCODE_DEL)
  PORT_BIT(0x00001000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("DUP") PORT_CODE(KEYCODE_END)
  PORT_BIT(0x00002000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Up") PORT_CODE(KEYCODE_UP)
  PORT_BIT(0x00004000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("HOME") PORT_CODE(KEYCODE_HOME)
  PORT_BIT(0x00008000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Left") PORT_CODE(KEYCODE_LEFT)
  PORT_BIT(0x00010000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Down") PORT_CODE(KEYCODE_DOWN)
  PORT_BIT(0x00020000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Right") PORT_CODE(KEYCODE_RIGHT)
  PORT_BIT(0x00040000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("BREAK") PORT_CODE(KEYCODE_ESC)
  PORT_BIT(0x00080000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("PF1") PORT_CODE(KEYCODE_F1)
  PORT_BIT(0x00100000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("PF2") PORT_CODE(KEYCODE_F2) 
  PORT_BIT(0x00200000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("PF3") PORT_CODE(KEYCODE_F3)
  PORT_BIT(0x00400000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("PF4") PORT_CODE(KEYCODE_F4) 
  PORT_BIT(0x00800000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("PF5") PORT_CODE(KEYCODE_F5)
  PORT_BIT(0x01000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("PF6") PORT_CODE(KEYCODE_F6)
  PORT_BIT(0x02000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("PF7") PORT_CODE(KEYCODE_F7)
  PORT_BIT(0x04000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("PF8") PORT_CODE(KEYCODE_F8) 
  PORT_BIT(0x08000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("PF9") PORT_CODE(KEYCODE_F9)
  PORT_BIT(0x10000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("PF10") PORT_CODE(KEYCODE_F10)

  PORT_START("key_modifiers")
  PORT_BIT(0x00000001,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Ctrl") PORT_CODE(KEYCODE_LCONTROL)
  PORT_BIT(0x00000002,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Shift") PORT_CODE(KEYCODE_LSHIFT)
  PORT_BIT(0x00000008,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("CAP") PORT_CODE(KEYCODE_CAPSLOCK) 
  PORT_BIT(0x00000010,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("GRAPH") PORT_CODE(KEYCODE_RALT) 
  PORT_BIT(0x00000020,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Kana") PORT_CODE(KEYCODE_RCONTROL)
INPUT_PORTS_END

static DRIVER_INIT(fm7)
{
//	shared_ram = auto_alloc_array(machine,UINT8,0x80);
	fm7_video_ram = auto_alloc_array(machine,UINT8,0xc000);
	fm7_timer = timer_alloc(machine,fm7_timer_irq,NULL);
	fm7_subtimer = timer_alloc(machine,fm7_subtimer_irq,NULL);
	fm7_keyboard_timer = timer_alloc(machine,fm7_keyboard_poll,NULL);
	cpu_set_irq_callback(cputag_get_cpu(machine,"maincpu"),fm7_irq_ack);
	cpu_set_irq_callback(cputag_get_cpu(machine,"sub"),fm7_sub_irq_ack);
}

static MACHINE_START(fm7)
{
	// The FM-7 has no initialisation ROM, and no other obvious
	// way to set the reset vector, so for now this will have to do.
	UINT8* RAM = memory_region(machine,"maincpu");
	
	RAM[0xfffe] = 0xfe;
	RAM[0xffff] = 0x00; 
	
	memset(shared_ram,0xff,0x80);
}

static MACHINE_RESET(fm7)
{
	UINT8* RAM = memory_region(machine,"maincpu");
	
	timer_adjust_periodic(fm7_timer,ATTOTIME_IN_NSEC(2034500),0,ATTOTIME_IN_NSEC(2034500));
	timer_adjust_periodic(fm7_subtimer,ATTOTIME_IN_MSEC(20),0,ATTOTIME_IN_MSEC(20));
	timer_adjust_periodic(fm7_keyboard_timer,attotime_zero,0,ATTOTIME_IN_MSEC(10));
	irq_mask = 0xff;
	irq_flags = 0x00;
	attn_irq = 0;
	sub_busy = 0x80;  // busy at reset
	basic_rom_en = 1;  // enabled at reset
	vram_access = 0;
	crt_enable = 0;
	memory_set_bankptr(machine,1,RAM+0x38000);
	key_delay = 700;  // 700ms on FM-7
	key_repeat = 70;  // 70ms on FM-7
	vram_offset = 0x0000;
	break_flag = 0;
	fm7_psg_regsel = 0;
}

static VIDEO_START( fm7 )
{
}

static VIDEO_UPDATE( fm7 )
{
    UINT8 code_r,code_g,code_b;
    UINT8 col;
    int y, x, b;

	if(crt_enable == 0)
		return 0;

    for (y = 0; y < 200; y++)
    {
	    for (x = 0; x < 80; x++)
	    {
            code_r = fm7_video_ram[0x8000 + ((y*80 + x + vram_offset) & 0x3fff)];
            code_g = fm7_video_ram[0x4000 + ((y*80 + x + vram_offset) & 0x3fff)];
            code_b = fm7_video_ram[0x0000 + ((y*80 + x + vram_offset) & 0x3fff)];
            for (b = 0; b < 8; b++)
            {
                col = (((code_r >> b) & 0x01) ? 4 : 0) + (((code_g >> b) & 0x01) ? 2 : 0) + (((code_b >> b) & 0x01) ? 1 : 0);
                *BITMAP_ADDR16(bitmap, y,  x*8+(7-b)) =  col;
            }
        }
    }
	return 0;
}

static const rgb_t fm7_initial_palette[8] = {
	MAKE_RGB(0x00, 0x00, 0x00), // 0
	MAKE_RGB(0x00, 0x00, 0xff), // 1
	MAKE_RGB(0xff, 0x00, 0x00), // 2
	MAKE_RGB(0xff, 0x00, 0xff), // 3
	MAKE_RGB(0x00, 0xff, 0x00), // 4
	MAKE_RGB(0x00, 0xff, 0xff), // 5
	MAKE_RGB(0xff, 0xff, 0x00), // 6
	MAKE_RGB(0xff, 0xff, 0xff), // 7
};

PALETTE_INIT( fm7 )
{
	int x;
	
	palette_set_colors(machine, 0, fm7_initial_palette, ARRAY_LENGTH(fm7_initial_palette));
	for(x=0;x<8;x++)
		fm7_pal[x] = x;
}


static const ay8910_interface fm7_psg_intf =
{
	AY8910_LEGACY_OUTPUT,
	AY8910_DEFAULT_LOADS,
	DEVCB_NULL,	/* portA read */
	DEVCB_NULL,	/* portB read */
	DEVCB_NULL,					/* portA write */
	DEVCB_NULL					/* portB write */
};

static const cassette_config fm7_cassette_config =
{
	fm7_cassette_formats,
	NULL,
	CASSETTE_STOPPED | CASSETTE_MOTOR_DISABLED | CASSETTE_SPEAKER_ENABLED
};

static MACHINE_DRIVER_START( fm7 )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", M6809, XTAL_2MHz)
	MDRV_CPU_PROGRAM_MAP(fm7_mem)
	MDRV_QUANTUM_PERFECT_CPU("maincpu")

	MDRV_CPU_ADD("sub", M6809, XTAL_2MHz)
	MDRV_CPU_PROGRAM_MAP(fm7_sub_mem)
	MDRV_QUANTUM_PERFECT_CPU("sub")
	
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("psg", AY8910, 1000000)  // clock speed unknown
	MDRV_SOUND_CONFIG(fm7_psg_intf)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS,"mono",1.0)
	MDRV_SOUND_WAVE_ADD("wave","cass")
	MDRV_SOUND_ROUTE(ALL_OUTPUTS,"mono",0.20)

	MDRV_MACHINE_START(fm7)
	MDRV_MACHINE_RESET(fm7)

	/* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(640, 200)
	MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 200-1)
	MDRV_PALETTE_LENGTH(8)
	MDRV_PALETTE_INIT(fm7)

	MDRV_VIDEO_START(fm7)
	MDRV_VIDEO_UPDATE(fm7)
	
	MDRV_CASSETTE_ADD("cass",fm7_cassette_config)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( fm77av )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", M6809E, XTAL_2MHz)
	MDRV_CPU_PROGRAM_MAP(fm77av_mem)

	MDRV_CPU_ADD("sub", M6809, XTAL_2MHz)
	MDRV_CPU_PROGRAM_MAP(fm77av_sub_mem)

	MDRV_MACHINE_RESET(fm7)

	/* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(640, 200)
	MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 200-1)
	MDRV_PALETTE_LENGTH(8)
	MDRV_PALETTE_INIT(fm7)

	MDRV_VIDEO_START(fm7)
	MDRV_VIDEO_UPDATE(fm7)
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( fm7 )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD( "fbasic30.rom", 0x38000,  0x7c00, CRC(a96d19b6) SHA1(8d5f0cfe7e0d39bf2ab7e4c798a13004769c28b2) )

	ROM_REGION( 0x20000, "sub", 0 )
	ROM_LOAD( "subsys_c.rom", 0xd800,  0x2800, CRC(24cec93f) SHA1(50b7283db6fe1342c6063fc94046283f4feddc1c) )

	// either one of these boot ROMs are selectable via DIP switch
	ROM_REGION( 0x200, "basic", 0 )
	ROM_LOAD( "boot_bas.rom", 0x0000,  0x0200, CRC(c70f0c74) SHA1(53b63a301cba7e3030e79c59a4d4291eab6e64b0) )

	ROM_REGION( 0x200, "dos", 0 )	
	ROM_LOAD( "boot_dos.rom", 0x0000,  0x0200, CRC(198614ff) SHA1(037e5881bd3fed472a210ee894a6446965a8d2ef) )

	// optional Kanji rom?
ROM_END

ROM_START( fm77av )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD( "initiate.rom", 0x36000,  0x2000, CRC(785cb06c) SHA1(b65987e98a9564a82c85eadb86f0204eee5a5c93) )
	ROM_LOAD( "fbasic30.rom", 0x38000,  0x7c00, CRC(a96d19b6) SHA1(8d5f0cfe7e0d39bf2ab7e4c798a13004769c28b2) )

	ROM_REGION( 0x22000, "sub", 0 )
	ROM_SYSTEM_BIOS(0, "mona", "Monitor Type A" )
	ROMX_LOAD( "subsys_a.rom", 0x1e000,  0x2000, CRC(e8014fbb) SHA1(038cb0b42aee9e933b20fccd6f19942e2f476c83), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS(1, "monb", "Monitor Type B" )
	ROMX_LOAD( "subsys_b.rom", 0x1e000,  0x2000, CRC(9be69fac) SHA1(0305bdd44e7d9b7b6a17675aff0a3330a08d21a8), ROM_BIOS(2) )
	/* 4 0x800 banks to be banked at 1d800 */
	ROM_LOAD( "subsyscg.rom", 0x20000,  0x2000, CRC(e9f16c42) SHA1(8ab466b1546d023ba54987790a79e9815d2b7bb2) )

	ROM_REGION( 0x20000, "gfx1", 0 )
	ROM_LOAD( "kanji.rom", 0x0000, 0x20000, CRC(62402ac9) SHA1(bf52d22b119d54410dad4949b0687bb0edf3e143) )

	// optional dict rom?
ROM_END

ROM_START( fm7740sx )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD( "initiate.rom", 0x36000,  0x2000, CRC(785cb06c) SHA1(b65987e98a9564a82c85eadb86f0204eee5a5c93) )
	ROM_LOAD( "fbasic30.rom", 0x38000,  0x7c00, CRC(a96d19b6) SHA1(8d5f0cfe7e0d39bf2ab7e4c798a13004769c28b2) )

	ROM_REGION( 0x22000, "sub", 0 )
	ROM_SYSTEM_BIOS(0, "mona", "Monitor Type A" )
	ROMX_LOAD( "subsys_a.rom", 0x1e000,  0x2000, CRC(e8014fbb) SHA1(038cb0b42aee9e933b20fccd6f19942e2f476c83), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS(1, "monb", "Monitor Type B" )
	ROMX_LOAD( "subsys_b.rom", 0x1e000,  0x2000, CRC(9be69fac) SHA1(0305bdd44e7d9b7b6a17675aff0a3330a08d21a8), ROM_BIOS(2) )
	/* 4 0x800 banks to be banked at 1d800 */
	ROM_LOAD( "subsyscg.rom", 0x20000,  0x2000, CRC(e9f16c42) SHA1(8ab466b1546d023ba54987790a79e9815d2b7bb2) )

	ROM_REGION( 0x20000, "gfx1", 0 )
	ROM_LOAD( "kanji.rom",  0x0000, 0x20000, CRC(62402ac9) SHA1(bf52d22b119d54410dad4949b0687bb0edf3e143) )
	ROM_LOAD( "kanji2.rom", 0x0000, 0x20000, CRC(38644251) SHA1(ebfdc43c38e1380709ed08575c346b2467ad1592) )

	/* These should be loaded at 2e000-2ffff of maincpu, but I'm not sure if it is correct */
	ROM_REGION( 0x4c000, "additional", 0 )
	ROM_LOAD( "dicrom.rom", 0x00000, 0x40000, CRC(b142acbc) SHA1(fe9f92a8a2750bcba0a1d2895e75e83858e4f97f) )
	ROM_LOAD( "extsub.rom", 0x40000, 0x0c000, CRC(0f7fcce3) SHA1(a1304457eeb400b4edd3c20af948d66a04df255e) )
ROM_END


static SYSTEM_CONFIG_START(fm7)
SYSTEM_CONFIG_END


/* Driver */

/*    YEAR  NAME      PARENT  COMPAT  MACHINE  INPUT   INIT  CONFIG  COMPANY      FULLNAME        FLAGS */
COMP( 1982, fm7,      0,      0,      fm7,     fm7,    fm7,  fm7,    "Fujitsu",   "FM-7",         GAME_NOT_WORKING)
COMP( 1985, fm77av,   fm7,    0,      fm77av,  fm7,    0,    fm7,    "Fujitsu",   "FM-77AV",      GAME_NOT_WORKING)
COMP( 1985, fm7740sx, fm7,    0,      fm77av,  fm7,    0,    fm7,    "Fujitsu",   "FM-77AV40SX",  GAME_NOT_WORKING)

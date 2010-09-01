/*

    Fujitsu FM-Towns
    driver by Barry Rodewald

    Japanese computer system released in 1989.

    CPU:  various AMD x86 CPUs, originally 80386DX (80387 available as an add-on).
          later models use 80386SX, 80486 and Pentium CPUs
    Sound:  Yamaha YM3438
            Ricoh RF5c68
            CD-DA
    Video:  Custom
            16 or 256 colours from a 24-bit palette, or 15-bit high-colour
            1024 sprites (16x16), rendered direct to VRAM
            16 colour text mode, rendered direct to VRAM


    Fujitsu FM-Towns Marty

    Japanese console, based on the FM-Towns computer, using an AMD 80386SX CPU,
    released in 1993


    Issues: i386 protected mode is far from complete.
            Video emulation is far from complete.

*/

/* I/O port map (incomplete, could well be incorrect too)
 *
 * 0x0000   : Master 8259 PIC
 * 0x0002   : Master 8259 PIC
 * 0x0010   : Slave 8259 PIC
 * 0x0012   : Slave 8259 PIC
 * 0x0020 RW: bit 0 = soft reset (read/write), bit 6 = power off (write), bit 7 = NMI vector protect
 * 0x0022  W: bit 7 = power off (write)
 * 0x0025 R : returns 0x00? (read)
 * 0x0026 R : timer?
 * 0x0028 RW: bit 0 = NMI mask (read/write)
 * 0x0030 R : Machine ID (low)
 * 0x0031 R : Machine ID (high)
 * 0x0032 RW: bit 7 = RESET, bit 6 = CLK, bit 0 = data (serial ROM)
 * 0x0040   : 8253 PIT counter 0
 * 0x0042   : 8253 PIT counter 1
 * 0x0044   : 8253 PIT counter 2
 * 0x0046   : 8253 PIT mode port
 * 0x0060   : 8253 PIT timer control
 * 0x006c RW: returns 0x00? (read) timer? (write)
 * 0x00a0-af: DMA controller 1 (uPD71071)
 * 0x00b0-bf: DMA controller 2 (uPD71071)
 * 0x0200-0f: Floppy controller (MB8877A)
 * 0x0400   : Video / CRTC (unknown)
 * 0x0404   : Disable VRAM, CMOS, memory-mapped I/O (everything in low memory except the BIOS)
 * 0x0440-5f: Video / CRTC
 * 0x0480 RW: bit 1 = disable BIOS ROM
 * 0x04c0-cf: CD-ROM controller
 * 0x04d5   : Sound mute
 * 0x04d8   : YM3438 control port A / status
 * 0x04da   : YM3438 data port A / status
 * 0x04dc   : YM3438 control port B / status
 * 0x04de   : YM3438 data port B / status
 * 0x04e0-e3: volume ports
 * 0x04e9-ec: IRQ masks
 * 0x04f0-f8: RF5c68 registers
 * 0x05e8 R : RAM size in MB
 * 0x05ec RW: bit 0 = compatibility mode?
 * 0x0600 RW: Keyboard data port (8042)
 * 0x0602   : Keyboard control port (8042)
 * 0x0604   : (8042)
 * 0x3000 - 0x3fff : CMOS RAM
 * 0xfd90-a0: CRTC / Video
 * 0xff81: CRTC / Video - returns value in RAM location 0xcff81?
 *
 * IRQ list
 *
 *      IRQ0 - PIT Timer IRQ
 *      IRQ1 - Keyboard
 *      IRQ2 - Serial Port
 *      IRQ6 - Floppy Disc Drive
 *      IRQ7 - PIC Cascade IRQ
 *      IRQ8 - SCSI controller
 *      IRQ9 - Built-in CD-ROM controller
 *      IRQ11 - VSync interrupt
 *      IRQ12 - Printer port
 *      IRQ13 - Sound (YM3438/RF5c68), Mouse
 *      IRQ15 - 16-bit PCM (expansion?)
 *
 * Machine ID list (I/O port 0x31)
 *
    1(01h)  FM-TOWNS 1/2
    2(02h)  FM-TOWNS 1F/2F/1H/2H
    3(03h)  FM-TOWNS 10F/20F/40H/80H
    4(04h)  FM-TOWNSII UX
    5(05h)  FM-TOWNSII CX
    6(06h)  FM-TOWNSII UG
    7(07h)  FM-TOWNSII HR
    8(08h)  FM-TOWNSII HG
    9(09h)  FM-TOWNSII UR
    11(0Bh) FM-TOWNSII MA
    12(0Ch) FM-TOWNSII MX
    13(0Dh) FM-TOWNSII ME
    14(0Eh) TOWNS Application Card (PS/V Vision)
    15(0Fh) FM-TOWNSII MF/Fresh/Fresh???TV
    16(10h) FM-TOWNSII SN
    17(11h) FM-TOWNSII HA/HB/HC
    19(13h) FM-TOWNSII EA/Fresh???T/Fresh???ET/Fresh???FT
    20(14h) FM-TOWNSII Fresh???E/Fresh???ES/Fresh???FS
    22(16h) FMV-TOWNS H/Fresh???GS/Fresh???GT/H2
    23(17h) FMV-TOWNS H20
    74(4Ah) FM-TOWNS MARTY
 */

#include "emu.h"
#include "cpu/i386/i386.h"
#include "sound/2612intf.h"
#include "sound/rf5c68.h"
#include "sound/cdda.h"
#include "devices/chd_cd.h"
#include "machine/pit8253.h"
#include "machine/pic8259.h"
#include "formats/basicdsk.h"
#include "machine/wd17xx.h"
#include "devices/flopdrv.h"
#include "machine/upd71071.h"
#include "devices/messram.h"
#include "includes/fmtowns.h"

// CD controller IRQ types
#define TOWNS_CD_IRQ_MPU 1
#define TOWNS_CD_IRQ_DMA 2

enum
{
	MOUSE_START,
	MOUSE_SYNC,
	MOUSE_X_HIGH,
	MOUSE_X_LOW,
	MOUSE_Y_HIGH,
	MOUSE_Y_LOW
};

static WRITE_LINE_DEVICE_HANDLER( towns_pic_irq );

INLINE UINT8 byte_to_bcd(UINT8 val)
{
	return ((val / 10) << 4) | (val % 10);
}

INLINE UINT8 bcd_to_byte(UINT8 val)
{
	return (((val & 0xf0) >> 4) * 10) + (val & 0x0f);
}

INLINE UINT32 msf_to_lba(UINT32 val)  // because the CDROM core doesn't provide this
{
	UINT8 m,s,f;
	f = bcd_to_byte(val & 0x0000ff);
	s = (bcd_to_byte((val & 0x00ff00) >> 8));
	m = (bcd_to_byte((val & 0xff0000) >> 16));
	return ((m * (60 * 75)) + (s * 75) + f);
}

static void towns_init_serial_rom(running_machine* machine)
{
	// TODO: init serial ROM contents
	towns_state* state = machine->driver_data<towns_state>();
	int x;
	UINT8 code[8] = { 0x04,0x65,0x54,0xA4,0x95,0x45,0x35,0x5F };
	UINT8* srom = memory_region(machine,"serial");

	memset(state->towns_serial_rom,0,256/8);

	if(srom)
	{
		memcpy(state->towns_serial_rom,srom,32);
		state->towns_machine_id = (state->towns_serial_rom[0x18] << 8) | state->towns_serial_rom[0x17];
		popmessage("Machine ID in serial ROM: %04x",state->towns_machine_id);
		return;
	}

	for(x=8;x<=21;x++)
		state->towns_serial_rom[x] = 0xff;

	for(x=0;x<=7;x++)
	{
		state->towns_serial_rom[x] = code[x];
	}

	// add Machine ID
	state->towns_machine_id = 0x0101;
	state->towns_serial_rom[0x17] = 0x01;
	state->towns_serial_rom[0x18] = 0x01;

	// serial number?
	state->towns_serial_rom[29] = 0x10;
	state->towns_serial_rom[28] = 0x6e;
	state->towns_serial_rom[27] = 0x54;
	state->towns_serial_rom[26] = 0x32;
	state->towns_serial_rom[25] = 0x10;
}

static void towns_init_rtc(running_machine* machine)
{
	towns_state* state = machine->driver_data<towns_state>();
	// for now, we'll just keep a static time stored here
	// seconds
	state->towns_rtc_reg[0] = 0;
	state->towns_rtc_reg[1] = 3;
	// minutes
	state->towns_rtc_reg[2] = 2;
	state->towns_rtc_reg[3] = 1;
	// hours
	state->towns_rtc_reg[4] = 8;
	state->towns_rtc_reg[5] = 0;
	// weekday
	state->towns_rtc_reg[6] = 2;
	// day
	state->towns_rtc_reg[7] = 1;
	state->towns_rtc_reg[8] = 1;
	// month
	state->towns_rtc_reg[9] = 6;
	state->towns_rtc_reg[10] = 0;
	// year
	state->towns_rtc_reg[11] = 9;
	state->towns_rtc_reg[12] = 0;
}

static READ8_HANDLER(towns_system_r)
{
	towns_state* state = space->machine->driver_data<towns_state>();
	UINT8 ret = 0;

	switch(offset)
	{
		case 0x00:
			logerror("SYS: port 0x20 read\n");
			return 0x00;
		case 0x05:
			logerror("SYS: port 0x25 read\n");
			return 0x00;
		case 0x06:
			//logerror("SYS: (0x26) timer read\n");
			state->ftimer -= 0x13;
			return state->ftimer;
		case 0x08:
			//logerror("SYS: (0x28) NMI mask read\n");
			return state->nmi_mask & 0x01;
		case 0x10:
			logerror("SYS: (0x30) Machine ID read\n");
			return (state->towns_machine_id >> 8) & 0xff;
		case 0x11:
			logerror("SYS: (0x31) Machine ID read\n");
			return state->towns_machine_id & 0xff;
		case 0x12:
			/* Bit 0 = data, bit 6 = CLK, bit 7 = RESET, bit 5 is always 1? */
			ret = (state->towns_serial_rom[state->towns_srom_position/8] & (1 << (state->towns_srom_position%8))) ? 1 : 0;
			ret |= state->towns_srom_clk;
			ret |= state->towns_srom_reset;
			//logerror("SYS: (0x32) Serial ROM read [0x%02x, pos=%i]\n",ret,towns_srom_position);
			return ret;
		default:
			//logerror("SYS: Unknown system port read (0x%02x)\n",offset+0x20);
			return 0x00;
	}
}

static WRITE8_HANDLER(towns_system_w)
{
	towns_state* state = space->machine->driver_data<towns_state>();

	switch(offset)
	{
		case 0x00:  // bit 7 = NMI vector protect, bit 6 = power off, bit 0 = software reset, bit 3 = A20 line?
//			cputag_set_input_line(space->machine,"maincpu",INPUT_LINE_A20,(data & 0x08) ? CLEAR_LINE : ASSERT_LINE);
			logerror("SYS: port 0x20 write %02x\n",data);
			break;
		case 0x02:
			logerror("SYS: (0x22) power port write %02x\n",data);
			break;
		case 0x08:
			//logerror("SYS: (0x28) NMI mask write %02x\n",data);
			state->nmi_mask = data & 0x01;
			break;
		case 0x12:
			//logerror("SYS: (0x32) Serial ROM write %02x\n",data);
			// clocks on low-to-high transition
			if((data & 0x40) && state->towns_srom_clk == 0) // CLK
			{  // advance to next bit
				state->towns_srom_position++;
			}
			if((data & 0x80) && state->towns_srom_reset == 0) // reset
			{  // reset to beginning
				state->towns_srom_position = 0;
			}
			state->towns_srom_clk = data & 0x40;
			state->towns_srom_reset = data & 0x80;
			break;
		default:
			logerror("SYS: Unknown system port write 0x%02x (0x%02x)\n",data,offset);
	}
}

static READ8_HANDLER(towns_sys6c_r)
{
	logerror("SYS: (0x6c) Timer? read\n");
	return 0x00;
}

static WRITE8_HANDLER(towns_sys6c_w)
{
	towns_state* state = space->machine->driver_data<towns_state>();
//  logerror("SYS: (0x6c) write to timer (0x%02x)\n",data);
	state->ftimer -= 0x54;
}

static READ8_HANDLER(towns_dma1_r)
{
	towns_state* state = space->machine->driver_data<towns_state>();
	running_device* dev = state->dma_1;

	logerror("DMA#1: read register %i\n",offset);
	return upd71071_r(dev,offset);
}

static WRITE8_HANDLER(towns_dma1_w)
{
	towns_state* state = space->machine->driver_data<towns_state>();
	running_device* dev = state->dma_1;

	logerror("DMA#1: wrote 0x%02x to register %i\n",data,offset);
	upd71071_w(dev,offset,data);
}

static READ8_HANDLER(towns_dma2_r)
{
	towns_state* state = space->machine->driver_data<towns_state>();
	running_device* dev = state->dma_2;

	logerror("DMA#2: read register %i\n",offset);
	return upd71071_r(dev,offset);
}

static WRITE8_HANDLER(towns_dma2_w)
{
	towns_state* state = space->machine->driver_data<towns_state>();
	running_device* dev = state->dma_2;

	logerror("DMA#2: wrote 0x%02x to register %i\n",data,offset);
	upd71071_w(dev,offset,data);
}

/*
 *  Floppy Disc Controller (MB8877A)
 */

static WRITE_LINE_DEVICE_HANDLER( towns_mb8877a_irq_w )
{
	towns_state* tstate = device->machine->driver_data<towns_state>();

	if(tstate->towns_fdc_irq6mask == 0)
		state = 0;
	pic8259_ir6_w(device, state);  // IRQ6 = FDC
	logerror("PIC: IRQ6 (FDC) set to %i\n",state);
}

static WRITE_LINE_DEVICE_HANDLER( towns_mb8877a_drq_w )
{
	upd71071_dmarq(device, state, 0);
}

static READ8_HANDLER(towns_floppy_r)
{
	towns_state* state = space->machine->driver_data<towns_state>();
	running_device* fdc = state->fdc;

	switch(offset)
	{
		case 0x00:
			return wd17xx_status_r(fdc,offset/2);
		case 0x02:
			return wd17xx_track_r(fdc,offset/2);
		case 0x04:
			return wd17xx_sector_r(fdc,offset/2);
		case 0x06:
			return wd17xx_data_r(fdc,offset/2);
		case 0x08:  // selected drive status?
			//logerror("FDC: read from offset 0x08\n");
			if(state->towns_selected_drive < 1 || state->towns_selected_drive > 2)
				return 0x01;
			else
				return 0x07;
		case 0x0e: // DRVCHG
			logerror("FDC: read from offset 0x0e\n");
			return 0x00;
		default:
			logerror("FDC: read from invalid or unimplemented register %02x\n",offset);
	}
	return 0xff;
}

static WRITE8_HANDLER(towns_floppy_w)
{
	towns_state* state = space->machine->driver_data<towns_state>();
	running_device* fdc = state->fdc;

	switch(offset)
	{
		case 0x00:
			// Commands 0xd0 and 0xfe (Write Track) are apparently ignored?
			if(data == 0xd0)
				return;
			if(data == 0xfe)
				return;
			wd17xx_command_w(fdc,offset/2,data);
			break;
		case 0x02:
			wd17xx_track_w(fdc,offset/2,data);
			break;
		case 0x04:
			wd17xx_sector_w(fdc,offset/2,data);
			break;
		case 0x06:
			wd17xx_data_w(fdc,offset/2,data);
			break;
		case 0x08:
			// bit 5 - CLKSEL
			if(state->towns_selected_drive != 0 && state->towns_selected_drive < 2)
			{
				floppy_mon_w(floppy_get_device(space->machine, state->towns_selected_drive-1), !BIT(data, 4));
				floppy_drive_set_ready_state(floppy_get_device(space->machine, state->towns_selected_drive-1), data & 0x10,0);
			}
			wd17xx_set_side(fdc,(data & 0x04)>>2);
			wd17xx_dden_w(fdc, BIT(~data, 1));

			state->towns_fdc_irq6mask = data & 0x01;
			logerror("FDC: write %02x to offset 0x08\n",data);
			break;
		case 0x0c:  // drive select
			switch(data & 0x0f)
			{
				case 0x00:
					state->towns_selected_drive = 0;  // No drive selected
					break;
				case 0x01:
					state->towns_selected_drive = 1;
					wd17xx_set_drive(fdc,0);
					break;
				case 0x02:
					state->towns_selected_drive = 2;
					wd17xx_set_drive(fdc,1);
					break;
				case 0x04:
					state->towns_selected_drive = 3;
					wd17xx_set_drive(fdc,2);
					break;
				case 0x08:
					state->towns_selected_drive = 4;
					wd17xx_set_drive(fdc,3);
					break;
			}
			logerror("FDC: drive select %02x\n",data);
			break;
		default:
			logerror("FDC: write %02x to invalid or unimplemented register %02x\n",data,offset);
	}
}

static UINT16 towns_fdc_dma_r(running_machine* machine)
{
	towns_state* state = machine->driver_data<towns_state>();
	running_device* fdc = state->fdc;
	return wd17xx_data_r(fdc,0);
}

static void towns_fdc_dma_w(running_machine* machine, UINT16 data)
{
	towns_state* state = machine->driver_data<towns_state>();
	running_device* fdc = state->fdc;
	wd17xx_data_w(fdc,0,data);
}

/*
 *  Port 0x600-0x607 - Keyboard controller (8042 MCU)
 *
 *  Sends two-byte code on each key press and release.
 *  First byte has the MSB set, and contains shift/ctrl/alt/kana flags
 *    Known bits:
 *      bit 7 = always 1
 *      bit 4 = key release
 *      bit 3 = ctrl
 *      bit 2 = shift
 *
 *  Second byte has the MSB reset, and contains the scancode of the key
 *  pressed or released.
 *      bit 7 = always 0
 *      bits 6-0 = key scancode
 */
static void towns_kb_sendcode(running_machine* machine, UINT8 scancode, int release)
{
	towns_state* state = machine->driver_data<towns_state>();
	running_device* dev = state->pic_master;

	switch(release)
	{
		case 0:  // key press
			state->towns_kb_output = 0x80;
			state->towns_kb_extend = scancode & 0x7f;
			if(input_port_read(machine,"key3") & 0x00080000)
				state->towns_kb_output |= 0x04;
			if(input_port_read(machine,"key3") & 0x00040000)
				state->towns_kb_output |= 0x08;
			if(input_port_read(machine,"key3") & 0x06400000)
				state->towns_kb_output |= 0x20;
			break;
		case 1:  // key release
			state->towns_kb_output = 0x90;
			state->towns_kb_extend = scancode & 0x7f;
			if(input_port_read(machine,"key3") & 0x00080000)
				state->towns_kb_output |= 0x04;
			if(input_port_read(machine,"key3") & 0x00040000)
				state->towns_kb_output |= 0x08;
			if(input_port_read(machine,"key3") & 0x06400000)
				state->towns_kb_output |= 0x20;
			break;
		case 2:  // extended byte
			state->towns_kb_output = scancode;
			state->towns_kb_extend = 0xff;
			break;
	}
	state->towns_kb_status |= 0x01;
	if(state->towns_kb_irq1_enable)
	{
		pic8259_ir1_w(dev, 1);
		logerror("PIC: IRQ1 (keyboard) set high\n");
	}
	logerror("KB: sending scancode 0x%02x\n",scancode);
}

static TIMER_CALLBACK( poll_keyboard )
{
	const char* kb_ports[4] = { "key1", "key2", "key3", "key4" };
	static UINT32 kb_prev[4];
	int port,bit;
	UINT8 scan;
	UINT32 portval;

	scan = 0;
	for(port=0;port<4;port++)
	{
		portval = input_port_read(machine,kb_ports[port]);
		for(bit=0;bit<32;bit++)
		{
			if(((portval & (1<<bit))) != ((kb_prev[port] & (1<<bit))))
			{  // bit changed
				if((portval & (1<<bit)) == 0)  // release
					towns_kb_sendcode(machine,scan,1);
				else
					towns_kb_sendcode(machine,scan,0);
			}
			scan++;
		}
		kb_prev[port] = portval;
	}
}

static READ8_HANDLER(towns_keyboard_r)
{
	towns_state* state = space->machine->driver_data<towns_state>();
	UINT8 ret = 0x00;

	switch(offset)
	{
		case 0:  // scancode output
			ret = state->towns_kb_output;
			//logerror("KB: read keyboard output port, returning %02x\n",ret);
			pic8259_ir1_w(state->pic_master, 0);
			logerror("PIC: IRQ1 (keyboard) set low\n");
			if(state->towns_kb_extend != 0xff)
			{
				towns_kb_sendcode(space->machine,state->towns_kb_extend,2);
			}
			else
				state->towns_kb_status &= ~0x01;
			return ret;
		case 1:  // status
			logerror("KB: read status port, returning %02x\n",state->towns_kb_status);
			return state->towns_kb_status;
		default:
			logerror("KB: read offset %02x\n",offset);
	}
	return 0x00;
}

static WRITE8_HANDLER(towns_keyboard_w)
{
	towns_state* state = space->machine->driver_data<towns_state>();

	switch(offset)
	{
		case 0:  // command input
			state->towns_kb_status &= ~0x08;
			state->towns_kb_status |= 0x01;
			break;
		case 1:  // control
			state->towns_kb_status |= 0x08;
			break;
		case 2:  // IRQ1 enable
			state->towns_kb_irq1_enable = data & 0x01;
			break;
		default:
			logerror("KB: wrote 0x%02x to offset %02x\n",data,offset);
	}
}

/*
 *  Port 0x60 - PIT Timer control
 *  On read:    bit 0: Timer 0 output level
 *              bit 1: Timer 1 output level
 *              bits 4-2: Timer masks (timer 2 = beeper)
 *  On write:   bits 2-0: Timer mask set
 *              bit 7: Timer 0 output reset
 */
static READ8_HANDLER(towns_port60_r)
{
	towns_state* state = space->machine->driver_data<towns_state>();
	UINT8 val = 0x00;

	if ( pit8253_get_output(state->pit, 0 ) )
		val |= 0x01;
	if ( pit8253_get_output(state->pit, 1 ) )
		val |= 0x02;

	val |= (state->towns_timer_mask & 0x07) << 2;

	//logerror("PIT: port 0x60 read, returning 0x%02x\n",val);
	return val;
}

static WRITE8_HANDLER(towns_port60_w)
{
	towns_state* state = space->machine->driver_data<towns_state>();
	running_device* dev = state->pic_master;

	if(data & 0x80)
	{
		towns_pic_irq(dev,0);
	}
	state->towns_timer_mask = data & 0x07;
	// bit 2 = sound (beeper?)
	//logerror("PIT: wrote 0x%02x to port 0x60\n",data);
}

static READ32_HANDLER(towns_sys5e8_r)
{
	towns_state* state = space->machine->driver_data<towns_state>();

	switch(offset)
	{
		case 0x00:
			if(ACCESSING_BITS_0_7)
			{
				logerror("SYS: read RAM size port\n");
				return 0x06;  // 6MB is standard for the Marty
			}
			break;
		case 0x01:
			if(ACCESSING_BITS_0_7)
			{
				logerror("SYS: read port 5ec\n");
				return state->compat_mode & 0x01;
			}
			break;
	}
	return 0x00;
}

static WRITE32_HANDLER(towns_sys5e8_w)
{
	towns_state* state = space->machine->driver_data<towns_state>();

	switch(offset)
	{
		case 0x00:
			if(ACCESSING_BITS_0_7)
			{
				logerror("SYS: wrote 0x%02x to port 5e8\n",data);
			}
			break;
		case 0x01:
			if(ACCESSING_BITS_0_7)
			{
				logerror("SYS: wrote 0x%02x to port 5ec\n",data);
				state->compat_mode = data & 0x01;
			}
			break;
	}
}

// Sound/LED control (I/O port 0x4e8-0x4ef)
// R/O  -- (0x4e9) FM IRQ flag (bit 0), PCM IRQ flag (bit 3)
// (0x4ea) PCM IRQ mask
// R/W  -- (0x4eb) PCM IRQ flag
// W/O  -- (0x4ec) LED control
static READ8_HANDLER(towns_sound_ctrl_r)
{
	towns_state* state = space->machine->driver_data<towns_state>();
	UINT8 ret = 0;

	switch(offset)
	{
		case 0x01:
			if(state->towns_fm_irq_flag)
				ret |= 0x01;
			if(state->towns_pcm_irq_flag)
				ret |= 0x08;
			break;
		case 0x03:
			ret = state->towns_pcm_channel_flag;
			state->towns_pcm_channel_flag = 0;
			state->towns_pcm_irq_flag = 0;
			pic8259_ir5_w(state->pic_slave, 0);
			logerror("PIC: IRQ13 (PCM) set low\n");
			break;
		default:
			logerror("FM: unimplemented port 0x%04x read\n",offset + 0x4e8);
	}
	return ret;
}

static WRITE8_HANDLER(towns_sound_ctrl_w)
{
	towns_state* state = space->machine->driver_data<towns_state>();
	switch(offset)
	{
		case 0x02:  // PCM channel interrupt mask
			state->towns_pcm_channel_mask = data;
			break;
		default:
			logerror("FM: unimplemented port 0x%04x write %02x\n",offset + 0x4e8,data);
	}
}

// Controller ports
// Joysticks are multiplexed, with fire buttons available when bits 0 and 1 of port 0x4d6 are high. (bits 2 and 3 for second port?)
static TIMER_CALLBACK(towns_mouse_timeout)
{
	towns_state* state = machine->driver_data<towns_state>();
	state->towns_mouse_output = MOUSE_START;  // reset mouse data
}

static READ32_HANDLER(towns_padport_r)
{
	towns_state* tstate = space->machine->driver_data<towns_state>();
	UINT32 ret = 0;
	UINT32 porttype = input_port_read(space->machine,"ctrltype");
	UINT8 extra1;
	UINT8 extra2;
	UINT32 state;

	if((porttype & 0x0f) == 0x00)
		ret |= 0x000000ff;
	if((porttype & 0xf0) == 0x00)
		ret |= 0x00ff0000;
	if((porttype & 0x0f) == 0x01)
	{
		extra1 = input_port_read(space->machine,"joy1_ex");

		if(tstate->towns_pad_mask & 0x10)
			ret |= (input_port_read(space->machine,"joy1") & 0x3f) | 0x00000040;
		else
			ret |= (input_port_read(space->machine,"joy1") & 0x0f) | 0x00000030;

		if(extra1 & 0x01) // Run button = left+right
			ret &= ~0x0000000c;
		if(extra1 & 0x02) // Select button = up+down
			ret &= ~0x00000003;

		if((extra1 & 0x10) && (tstate->towns_pad_mask & 0x01))
			ret &= ~0x00000010;
		if((extra1 & 0x20) && (tstate->towns_pad_mask & 0x02))
			ret &= ~0x00000020;
	}
	if((porttype & 0xf0) == 0x10)
	{
		extra2 = input_port_read(space->machine,"joy2_ex");

		if(tstate->towns_pad_mask & 0x20)
			ret |= ((input_port_read(space->machine,"joy2") & 0x3f) << 16) | 0x00400000;
		else
			ret |= ((input_port_read(space->machine,"joy2") & 0x0f) << 16) | 0x00300000;

		if(extra2 & 0x01)
			ret &= ~0x000c0000;
		if(extra2 & 0x02)
			ret &= ~0x00030000;

		if((extra2 & 0x10) && (tstate->towns_pad_mask & 0x04))
			ret &= ~0x00100000;
		if((extra2 & 0x20) && (tstate->towns_pad_mask & 0x08))
			ret &= ~0x00200000;
	}
	if((porttype & 0x0f) == 0x02)  // mouse
	{
		switch(tstate->towns_mouse_output)
		{
			case MOUSE_X_HIGH:
				ret |= ((tstate->towns_mouse_x & 0xf0) >> 4);
				break;
			case MOUSE_X_LOW:
				ret |= (tstate->towns_mouse_x & 0x0f);
				break;
			case MOUSE_Y_HIGH:
				ret |= ((tstate->towns_mouse_y & 0xf0) >> 4);
				break;
			case MOUSE_Y_LOW:
				ret |= (tstate->towns_mouse_y & 0x0f);
				break;
			case MOUSE_START:
			case MOUSE_SYNC:
			default:
				if(tstate->towns_mouse_output < MOUSE_Y_LOW)
					ret |= 0x0000000f;
		}

		// button states are always visible
		state = input_port_read(space->machine,"mouse1");
		if(!(state & 0x01))
			ret |= 0x00000010;
		if(!(state & 0x02))
			ret |= 0x00000020;
		if(tstate->towns_pad_mask & 0x10)
			ret |= 0x00000040;
	}
	if((porttype & 0xf0) == 0x20)  // mouse
	{
		switch(tstate->towns_mouse_output)
		{
			case MOUSE_X_HIGH:
				ret |= ((tstate->towns_mouse_x & 0xf0) << 12);
				break;
			case MOUSE_X_LOW:
				ret |= ((tstate->towns_mouse_x & 0x0f) << 16);
				break;
			case MOUSE_Y_HIGH:
				ret |= ((tstate->towns_mouse_y & 0xf0) << 12);
				break;
			case MOUSE_Y_LOW:
				ret |= ((tstate->towns_mouse_y & 0x0f) << 16);
				break;
			case MOUSE_START:
			case MOUSE_SYNC:
			default:
				if(tstate->towns_mouse_output < MOUSE_Y_LOW)
					ret |= 0x000f0000;
		}

		// button states are always visible
		state = input_port_read(space->machine,"mouse1");
		if(!(state & 0x01))
			ret |= 0x00100000;
		if(!(state & 0x02))
			ret |= 0x00200000;
		if(tstate->towns_pad_mask & 0x20)
			ret |= 0x00400000;
	}

	return ret;
}

static WRITE32_HANDLER(towns_pad_mask_w)
{
	towns_state* state = space->machine->driver_data<towns_state>();
	static UINT8 prev;
	static UINT8 prev_x,prev_y;
	UINT8 current_x,current_y;
	UINT32 type = input_port_read(space->machine,"ctrltype");

	if(ACCESSING_BITS_16_23)
	{
		state->towns_pad_mask = (data & 0x00ff0000) >> 16;
		if((type & 0x0f) == 0x02)  // mouse
		{
			if((state->towns_pad_mask & 0x10) != 0 && (prev & 0x10) == 0)
			{
				if(state->towns_mouse_output == MOUSE_START)
				{
					state->towns_mouse_output = MOUSE_X_HIGH;
					current_x = input_port_read(space->machine,"mouse2");
					current_y = input_port_read(space->machine,"mouse3");
					state->towns_mouse_x = prev_x - current_x;
					state->towns_mouse_y = prev_y - current_y;
					prev_x = current_x;
					prev_y = current_y;
				}
				else
					state->towns_mouse_output++;
				timer_adjust_periodic(state->towns_mouse_timer,ATTOTIME_IN_USEC(600),0,attotime_zero);
			}
			if((state->towns_pad_mask & 0x10) == 0 && (prev & 0x10) != 0)
			{
				if(state->towns_mouse_output == MOUSE_START)
				{
					state->towns_mouse_output = MOUSE_SYNC;
					current_x = input_port_read(space->machine,"mouse2");
					current_y = input_port_read(space->machine,"mouse3");
					state->towns_mouse_x = prev_x - current_x;
					state->towns_mouse_y = prev_y - current_y;
					prev_x = current_x;
					prev_y = current_y;
				}
				else
					state->towns_mouse_output++;
				timer_adjust_periodic(state->towns_mouse_timer,ATTOTIME_IN_USEC(600),0,attotime_zero);
			}
			prev = state->towns_pad_mask;
		}
		if((type & 0xf0) == 0x20)  // mouse
		{
			if((state->towns_pad_mask & 0x20) != 0 && (prev & 0x20) == 0)
			{
				if(state->towns_mouse_output == MOUSE_START)
				{
					state->towns_mouse_output = MOUSE_X_HIGH;
					current_x = input_port_read(space->machine,"mouse2");
					current_y = input_port_read(space->machine,"mouse3");
					state->towns_mouse_x = prev_x - current_x;
					state->towns_mouse_y = prev_y - current_y;
					prev_x = current_x;
					prev_y = current_y;
				}
				else
					state->towns_mouse_output++;
				timer_adjust_periodic(state->towns_mouse_timer,ATTOTIME_IN_USEC(600),0,attotime_zero);
			}
			if((state->towns_pad_mask & 0x20) == 0 && (prev & 0x20) != 0)
			{
				if(state->towns_mouse_output == MOUSE_START)
				{
					state->towns_mouse_output = MOUSE_SYNC;
					current_x = input_port_read(space->machine,"mouse2");
					current_y = input_port_read(space->machine,"mouse3");
					state->towns_mouse_x = prev_x - current_x;
					state->towns_mouse_y = prev_y - current_y;
					prev_x = current_x;
					prev_y = current_y;
				}
				else
					state->towns_mouse_output++;
				timer_adjust_periodic(state->towns_mouse_timer,ATTOTIME_IN_USEC(600),0,attotime_zero);
			}
			prev = state->towns_pad_mask;
		}
	}
}

static READ8_HANDLER( towns_cmos8_r )
{
	towns_state* state = space->machine->driver_data<towns_state>();
	return state->towns_cmos[offset];
}

static WRITE8_HANDLER( towns_cmos8_w )
{
	towns_state* state = space->machine->driver_data<towns_state>();
	state->towns_cmos[offset] = data;
}

static READ8_HANDLER( towns_cmos_low_r )
{
	towns_state* state = space->machine->driver_data<towns_state>();
	if(state->towns_mainmem_enable != 0)
		return messram_get_ptr(state->messram)[offset + 0xd8000];

	return state->towns_cmos[offset];
}

static WRITE8_HANDLER( towns_cmos_low_w )
{
	towns_state* state = space->machine->driver_data<towns_state>();
	if(state->towns_mainmem_enable != 0)
		messram_get_ptr(state->messram)[offset+0xd8000] = data;
	else
		state->towns_cmos[offset] = data;
}

static READ8_HANDLER( towns_cmos_r )
{
	towns_state* state = space->machine->driver_data<towns_state>();
	return state->towns_cmos[offset];
}

static WRITE8_HANDLER( towns_cmos_w )
{
	towns_state* state = space->machine->driver_data<towns_state>();
	state->towns_cmos[offset] = data;
}

void towns_update_video_banks(address_space* space)
{
	towns_state* state = space->machine->driver_data<towns_state>();
	UINT8* ROM;

	if(state->towns_mainmem_enable != 0)  // first MB is RAM
	{
		ROM = memory_region(space->machine,"user");

//      memory_set_bankptr(space->machine,1,messram_get_ptr(state->messram)+0xc0000);
//      memory_set_bankptr(space->machine,2,messram_get_ptr(state->messram)+0xc8000);
//      memory_set_bankptr(space->machine,3,messram_get_ptr(state->messram)+0xc9000);
//      memory_set_bankptr(space->machine,4,messram_get_ptr(state->messram)+0xca000);
//      memory_set_bankptr(space->machine,5,messram_get_ptr(state->messram)+0xca000);
//      memory_set_bankptr(space->machine,10,messram_get_ptr(state->messram)+0xca800);
		memory_set_bankptr(space->machine,"bank6",messram_get_ptr(state->messram)+0xcb000);
		memory_set_bankptr(space->machine,"bank7",messram_get_ptr(state->messram)+0xcb000);
		if(state->towns_system_port & 0x02)
			memory_set_bankptr(space->machine,"bank11",messram_get_ptr(state->messram)+0xf8000);
		else
			memory_set_bankptr(space->machine,"bank11",ROM+0x238000);
		memory_set_bankptr(space->machine,"bank12",messram_get_ptr(state->messram)+0xf8000);
		return;
	}
	else  // enable I/O ports and VRAM
	{
		ROM = memory_region(space->machine,"user");

//      memory_set_bankptr(space->machine,1,towns_gfxvram+(towns_vram_rplane*0x8000));
//      memory_set_bankptr(space->machine,2,towns_txtvram);
//      memory_set_bankptr(space->machine,3,messram_get_ptr(state->messram)+0xc9000);
//      if(towns_ankcg_enable != 0)
//          memory_set_bankptr(space->machine,4,ROM+0x180000+0x3d000);  // ANK CG 8x8
//      else
//          memory_set_bankptr(space->machine,4,towns_txtvram+0x2000);
//      memory_set_bankptr(space->machine,5,towns_txtvram+0x2000);
//      memory_set_bankptr(space->machine,10,messram_get_ptr(state->messram)+0xca800);
		if(state->towns_ankcg_enable != 0)
			memory_set_bankptr(space->machine,"bank6",ROM+0x180000+0x3d800);  // ANK CG 8x16
		else
			memory_set_bankptr(space->machine,"bank6",messram_get_ptr(state->messram)+0xcb000);
		memory_set_bankptr(space->machine,"bank7",messram_get_ptr(state->messram)+0xcb000);
		if(state->towns_system_port & 0x02)
			memory_set_bankptr(space->machine,"bank11",messram_get_ptr(state->messram)+0xf8000);
		else
			memory_set_bankptr(space->machine,"bank11",ROM+0x238000);
		memory_set_bankptr(space->machine,"bank12",messram_get_ptr(state->messram)+0xf8000);
		return;
	}
}

static READ8_HANDLER( towns_sys480_r )
{
	towns_state* state = space->machine->driver_data<towns_state>();
	if(state->towns_system_port & 0x02)
		return 0x02;
	else
		return 0x00;
}

static WRITE8_HANDLER( towns_sys480_w )
{
	towns_state* state = space->machine->driver_data<towns_state>();

	state->towns_system_port = data;
	state->towns_ram_enable = data & 0x02;
	towns_update_video_banks(space);
}

static WRITE32_HANDLER( towns_video_404_w )
{
	towns_state* state = space->machine->driver_data<towns_state>();
	if(ACCESSING_BITS_0_7)
	{
		state->towns_mainmem_enable = data & 0x80;
		towns_update_video_banks(space);
		logerror("mainmem_enable - set to 0x%02x\n",state->towns_mainmem_enable);
	}
}

static READ32_HANDLER( towns_video_404_r )
{
	towns_state* state = space->machine->driver_data<towns_state>();
	if(ACCESSING_BITS_0_7)
	{
		logerror("mainmem_enable - reading 0x%02x\n",state->towns_mainmem_enable);
		if(state->towns_mainmem_enable != 0)
			return 0x00000080;
	}
	return 0;
}

/*
 *  I/O ports 0x4c0-0x4cf
 *  CD-ROM driver (custom?)
 *
 *  0x4c0 - Status port (R/W)
 *    bit 7 - IRQ from sub MPU (reset when read)
 *    bit 6 - IRQ from DMA end (reset when read)
 *    bit 5 - Software transfer
 *    bit 4 - DMA transfer
 *    bit 1 - status read request
 *    bit 0 - ready
 *
 *  0x4c2 - Command port (R/W)
 *    On read, returns status byte (4 in total?)
 *    On write, performs specified command:
 *      bit 7 - command type
 *      bit 6 - IRQ
 *      bit 5 - status
 *      bits 4-0 - command
 *        Type=1:
 *          0 = set state
 *          1 = set state (CDDASET)
 *        Type=0:
 *          0 = Seek
 *          2 = Read (MODE1)
 *          5 = TOC Read
 *
 *  0x4c4 - Parameter port (R/W)
 *    Inserts a byte into an array of 8 bytes used for command parameters
 *    Writing to this port puts the byte at the front of the array, and
 *    pushes the other parameters back.
 *
 *  0x4c6 (W/O)
 *    bit 3 - software transfer mode
 *    bit 4 - DMA transfer mode
 *
 */
static void towns_cdrom_set_irq(running_machine* machine,int line,int state)
{
	towns_state* tstate = machine->driver_data<towns_state>();
	switch(line)
	{
		case TOWNS_CD_IRQ_MPU:
			if(state != 0)
			{
				if(tstate->towns_cd.command & 0x40)
				{
					tstate->towns_cd.status |= 0x80;
					if(tstate->towns_cd.mpu_irq_enable)
					{
						pic8259_ir1_w(tstate->pic_slave, 1);
						logerror("PIC: IRQ9 (CD-ROM) set high\n");
					}
				}
			}
			else
			{
				tstate->towns_cd.status &= ~0x80;
				pic8259_ir1_w(tstate->pic_slave, 0);
				logerror("PIC: IRQ9 (CD-ROM) set low\n");
			}
			break;
		case TOWNS_CD_IRQ_DMA:
			if(state != 0)
			{
				if(tstate->towns_cd.command & 0x40)
				{
					tstate->towns_cd.status |= 0x40;
					if(tstate->towns_cd.dma_irq_enable)
					{
						pic8259_ir1_w(tstate->pic_slave, 1);
						logerror("PIC: IRQ9 (CD-ROM DMA) set high\n");
					}
				}
			}
			else
			{
				tstate->towns_cd.status &= ~0x40;
				pic8259_ir1_w(tstate->pic_slave, 0);
				logerror("PIC: IRQ9 (CD-ROM DMA) set low\n");
			}
			break;
	}
}

static TIMER_CALLBACK( towns_cd_status_ready )
{
	towns_state* state = machine->driver_data<towns_state>();
	state->towns_cd.status |= 0x02;  // status read request
	state->towns_cd.status |= 0x01;  // ready
	state->towns_cd.cmd_status_ptr = 0;
	towns_cdrom_set_irq((running_machine*)ptr,TOWNS_CD_IRQ_MPU,1);
}

static void towns_cd_set_status(running_machine* machine, UINT8 st0, UINT8 st1, UINT8 st2, UINT8 st3)
{
	towns_state* state = machine->driver_data<towns_state>();
	state->towns_cd.cmd_status[0] = st0;
	state->towns_cd.cmd_status[1] = st1;
	state->towns_cd.cmd_status[2] = st2;
	state->towns_cd.cmd_status[3] = st3;
	// wait a bit
	timer_set(machine,ATTOTIME_IN_MSEC(1),machine,0,towns_cd_status_ready);
}

static UINT8 towns_cd_get_track(running_machine* machine)
{
	towns_state* state = machine->driver_data<towns_state>();
	running_device* cdrom = state->cdrom;
	running_device* cdda = state->cdda;
	UINT32 lba = cdda_get_audio_lba(cdda);
	UINT8 track;

	for(track=1;track<99;track++)
	{
		if(cdrom_get_track_start(mess_cd_get_cdrom_file(cdrom),track) > lba)
			break;
	}
	return track;
}

static TIMER_CALLBACK( towns_cdrom_read_byte )
{
	running_device* device = (running_device* )ptr;
	towns_state* state = machine->driver_data<towns_state>();
	int masked;
	// TODO: support software transfers, for now DMA is assumed.

	if(state->towns_cd.buffer_ptr < 0) // transfer has ended
		return;

	masked = upd71071_dmarq(device,param,3);  // CD-ROM controller uses DMA1 channel 3
//  logerror("DMARQ: param=%i ret=%i bufferptr=%i\n",param,masked,state->towns_cd.buffer_ptr);
	if(param != 0)
	{
		timer_adjust_oneshot(state->towns_cd.read_timer,ATTOTIME_IN_HZ(300000),0);
	}
	else
	{
		if(masked != 0)  // check if the DMA channel is masked
		{
			timer_adjust_oneshot(state->towns_cd.read_timer,ATTOTIME_IN_HZ(300000),1);
			return;
		}
		if(state->towns_cd.buffer_ptr < 2048)
			timer_adjust_oneshot(state->towns_cd.read_timer,ATTOTIME_IN_HZ(300000),1);
		else
		{  // end of transfer
			state->towns_cd.status &= ~0x10;  // no longer transferring by DMA
			state->towns_cd.status &= ~0x20;  // no longer transferring by software
			logerror("DMA1: end of transfer (LBA=%08x)\n",state->towns_cd.lba_current);
			if(state->towns_cd.lba_current >= state->towns_cd.lba_last)
			{
				state->towns_cd.extra_status = 0;
				towns_cd_set_status(device->machine,0x06,0x00,0x00,0x00);
				towns_cdrom_set_irq(device->machine,TOWNS_CD_IRQ_DMA,1);
				state->towns_cd.buffer_ptr = -1;
				state->towns_cd.status |= 0x01;  // ready
			}
			else
			{
				state->towns_cd.extra_status = 0;
				towns_cd_set_status(device->machine,0x22,0x00,0x00,0x00);
				towns_cdrom_set_irq(device->machine,TOWNS_CD_IRQ_DMA,1);
				cdrom_read_data(mess_cd_get_cdrom_file(state->cdrom),++state->towns_cd.lba_current,state->towns_cd.buffer,CD_TRACK_MODE1);
				timer_adjust_oneshot(state->towns_cd.read_timer,ATTOTIME_IN_HZ(300000),1);
				state->towns_cd.buffer_ptr = -1;
			}
		}
	}
}

static void towns_cdrom_read(running_device* device)
{
	// MODE 1 read
	// load data into buffer to be sent via DMA1 channel 3
	// A set of status bytes is sent after each sector, and DMA is paused
	// so that the DMA controller than be set up again.
	// parameters:
	//          3 bytes: MSF of first sector to read
	//          3 bytes: MSF of last sector to read
	towns_state* state = device->machine->driver_data<towns_state>();
	UINT32 lba1,lba2,track;

	lba1 = state->towns_cd.parameter[7] << 16;
	lba1 += state->towns_cd.parameter[6] << 8;
	lba1 += state->towns_cd.parameter[5];
	lba2 = state->towns_cd.parameter[4] << 16;
	lba2 += state->towns_cd.parameter[3] << 8;
	lba2 += state->towns_cd.parameter[2];
	state->towns_cd.lba_current = msf_to_lba(lba1);
	state->towns_cd.lba_last = msf_to_lba(lba2);

	// first track starts at 00:02:00 - this is hardcoded in the boot procedure
	track = cdrom_get_track(mess_cd_get_cdrom_file(device),state->towns_cd.lba_current);
	if(track < 2)
	{  // recalculate LBA
		state->towns_cd.lba_current -= 150;
		state->towns_cd.lba_last -= 150;
	}

	logerror("CD: Mode 1 read from LBA next:%i last:%i track:%i\n",state->towns_cd.lba_current,state->towns_cd.lba_last,track);

	if(state->towns_cd.lba_current > state->towns_cd.lba_last)
	{
		state->towns_cd.extra_status = 0;
		towns_cd_set_status(device->machine,0x01,0x00,0x00,0x00);
	}
	else
	{
		cdrom_read_data(mess_cd_get_cdrom_file(device),state->towns_cd.lba_current,state->towns_cd.buffer,CD_TRACK_MODE1);
		state->towns_cd.status |= 0x10;  // DMA transfer begin
		state->towns_cd.status &= ~0x20;  // not a software transfer
//      state->towns_cd.buffer_ptr = 0;
//      timer_adjust_oneshot(state->towns_cd.read_timer,ATTOTIME_IN_HZ(300000),1);
		state->towns_cd.extra_status = 2;
		towns_cd_set_status(device->machine,0x00,0x00,0x00,0x00);
	}
}

static void towns_cdrom_play_cdda(running_device* device)
{
	// PLAY AUDIO
	// Plays CD-DA audio from the specified MSF
	// Parameters:
	//          3 bytes: starting MSF of audio to play
	//          3 bytes: ending MSF of audio to play (can span multiple tracks)
	towns_state* state = device->machine->driver_data<towns_state>();
	UINT32 lba1,lba2;
	running_device* cdda = state->cdda;

	lba1 = state->towns_cd.parameter[7] << 16;
	lba1 += state->towns_cd.parameter[6] << 8;
	lba1 += state->towns_cd.parameter[5];
	lba2 = state->towns_cd.parameter[4] << 16;
	lba2 += state->towns_cd.parameter[3] << 8;
	lba2 += state->towns_cd.parameter[2];
	state->towns_cd.cdda_current = msf_to_lba(lba1);
	state->towns_cd.cdda_length = msf_to_lba(lba2) - state->towns_cd.cdda_current;

	cdda_set_cdrom(cdda,mess_cd_get_cdrom_file(device));
	cdda_start_audio(cdda,state->towns_cd.cdda_current,state->towns_cd.cdda_length);
	logerror("CD: CD-DA start from LBA:%i length:%i\n",state->towns_cd.cdda_current,state->towns_cd.cdda_length);

	state->towns_cd.extra_status = 1;
	towns_cd_set_status(device->machine,0x00,0x00,0x00,0x00);
}

static void towns_cdrom_execute_command(running_device* device)
{
	towns_state* state = device->machine->driver_data<towns_state>();

	if(mess_cd_get_cdrom_file(device) == NULL)
	{  // No CD in drive
		if(state->towns_cd.command & 0x20)
		{
			state->towns_cd.extra_status = 0;
			towns_cd_set_status(device->machine,0x10,0x00,0x00,0x00);
		}
	}
	else
	{
		state->towns_cd.status &= ~0x02;
		switch(state->towns_cd.command & 0x9f)
		{
			case 0x00:  // Seek
				if(state->towns_cd.command & 0x20)
				{
					state->towns_cd.extra_status = 1;
					towns_cd_set_status(device->machine,0x00,0x00,0x00,0x00);
				}
				logerror("CD: Command 0x00: SEEK\n");
				break;
			case 0x01:  // unknown
				if(state->towns_cd.command & 0x20)
				{
					state->towns_cd.extra_status = 0;
					towns_cd_set_status(device->machine,0x00,0x00,0x00,0x00);
				}
				logerror("CD: Command 0x01: unknown\n");
				break;
			case 0x02:  // Read (MODE1)
				logerror("CD: Command 0x02: READ MODE1\n");
				towns_cdrom_read(device);
				break;
			case 0x04:  // Play Audio Track
				logerror("CD: Command 0x04: PLAY CD-DA\n");
				towns_cdrom_play_cdda(device);
				break;
			case 0x05:  // Read TOC
				logerror("CD: Command 0x05: READ TOC\n");
				state->towns_cd.extra_status = 1;
				towns_cd_set_status(device->machine,0x00,0x00,0x00,0x00);
				break;
			case 0x06:  // Read CD-DA state?
				logerror("CD: Command 0x06: READ CD-DA STATE\n");
				state->towns_cd.extra_status = 1;
				towns_cd_set_status(device->machine,0x00,0x00,0x00,0x00);
				break;
			case 0x80:  // set state
				logerror("CD: Command 0x80: set state\n");
				if(state->towns_cd.command & 0x20)
				{
					state->towns_cd.extra_status = 0;
					if(state->towns_cd.parameter[7] != 0)
					{
						towns_cd_set_status(device->machine,0x00,0x00,0x00,0x00);
						break;
					}
					if(cdda_audio_active(state->cdda))
						towns_cd_set_status(device->machine,0x00,0x03,0x00,0x00);
					else
						towns_cd_set_status(device->machine,0x00,0x01,0x00,0x00);

				}
				break;
			case 0x81:  // set state (CDDASET)
				if(state->towns_cd.command & 0x20)
				{
					state->towns_cd.extra_status = 0;
					towns_cd_set_status(device->machine,0x00,0x00,0x00,0x00);
				}
				logerror("CD: Command 0x81: set state (CDDASET)\n");
				break;
			case 0x84:   // Stop CD audio track
				if(state->towns_cd.command & 0x20)
				{
					state->towns_cd.extra_status = 1;
					towns_cd_set_status(device->machine,0x00,0x00,0x00,0x00);
					cdda_stop_audio(state->cdda);
				}
				logerror("CD: Command 0x84: STOP CD-DA\n");
				break;
			case 0x85:   // Stop CD audio track (difference from 0x84?)
				if(state->towns_cd.command & 0x20)
				{
					state->towns_cd.extra_status = 1;
					towns_cd_set_status(device->machine,0x00,0x00,0x00,0x00);
					cdda_stop_audio(state->cdda);
				}
				logerror("CD: Command 0x85: STOP CD-DA\n");
				break;
			default:
				state->towns_cd.extra_status = 0;
				towns_cd_set_status(device->machine,0x10,0x00,0x00,0x00);
				logerror("CD: Unknown or unimplemented command %02x\n",state->towns_cd.command);
		}
	}
}

static UINT16 towns_cdrom_dma_r(running_machine* machine)
{
	towns_state* state = machine->driver_data<towns_state>();
	if(state->towns_cd.buffer_ptr >= 2048)
		return 0x00;
	return state->towns_cd.buffer[state->towns_cd.buffer_ptr++];
}

static READ8_HANDLER(towns_cdrom_r)
{
	towns_state* state = space->machine->driver_data<towns_state>();
	UINT32 addr = 0;
	UINT8 ret = 0;

	ret = state->towns_cd.cmd_status[state->towns_cd.cmd_status_ptr];

	switch(offset)
	{
		case 0x00:  // status
			//logerror("CD: status read, returning %02x\n",towns_cd.status);
			return state->towns_cd.status;
		case 0x01:  // command status
			if(state->towns_cd.cmd_status_ptr >= 3)
			{
				state->towns_cd.status &= ~0x02;
				// check for more status bytes
				if(state->towns_cd.extra_status != 0)
				{
					switch(state->towns_cd.command & 0x9f)
					{
						case 0x00:  // seek
							towns_cd_set_status(space->machine,0x04,0x00,0x00,0x00);
							state->towns_cd.extra_status = 0;
							break;
						case 0x02:  // read
							if(state->towns_cd.extra_status == 2)
								towns_cd_set_status(space->machine,0x22,0x00,0x00,0x00);
							state->towns_cd.extra_status = 0;
							break;
						case 0x04:  // play cdda
							towns_cd_set_status(space->machine,0x07,0x00,0x00,0x00);
							state->towns_cd.extra_status = 0;
							break;
						case 0x05:  // read toc
							switch(state->towns_cd.extra_status)
							{
								case 1:
								case 3:
									towns_cd_set_status(space->machine,0x16,0x00,0x00,0x00);
									state->towns_cd.extra_status++;
									break;
								case 2: // st1 = first track number (BCD)
									towns_cd_set_status(space->machine,0x17,0x01,0x00,0x00);
									state->towns_cd.extra_status++;
									break;
								case 4: // st1 = last track number (BCD)
									towns_cd_set_status(space->machine,0x17,
										byte_to_bcd(cdrom_get_last_track(mess_cd_get_cdrom_file(state->cdrom))),
										0x00,0x00);
									state->towns_cd.extra_status++;
									break;
								case 5:  // st1 = control/adr of track 0xaa?
									towns_cd_set_status(space->machine,0x16,
										cdrom_get_adr_control(mess_cd_get_cdrom_file(state->cdrom),0xaa),
										0xaa,0x00);
									state->towns_cd.extra_status++;
									break;
								case 6:  // st1/2/3 = address of track 0xaa? (BCD)
									addr = cdrom_get_track_start(mess_cd_get_cdrom_file(state->cdrom),0xaa);
									addr = lba_to_msf(addr);
									towns_cd_set_status(space->machine,0x17,
										(addr & 0xff0000) >> 16,(addr & 0x00ff00) >> 8,addr & 0x0000ff);
									state->towns_cd.extra_status++;
									break;
								default:  // same as case 5 and 6, but for each individual track
									if(state->towns_cd.extra_status & 0x01)
									{
										towns_cd_set_status(space->machine,0x16,
											((cdrom_get_adr_control(mess_cd_get_cdrom_file(state->cdrom),(state->towns_cd.extra_status/2)-3) & 0x0f) << 4)
											| ((cdrom_get_adr_control(mess_cd_get_cdrom_file(state->cdrom),(state->towns_cd.extra_status/2)-3) & 0xf0) >> 4),
											(state->towns_cd.extra_status/2)-3,0x00);
										state->towns_cd.extra_status++;
									}
									else
									{
										addr = cdrom_get_track_start(mess_cd_get_cdrom_file(state->cdrom),(state->towns_cd.extra_status/2)-4);
										addr = lba_to_msf(addr);
										towns_cd_set_status(space->machine,0x17,
											(addr & 0xff0000) >> 16,(addr & 0x00ff00) >> 8,addr & 0x0000ff);
										if(((state->towns_cd.extra_status/2)-3) >= cdrom_get_last_track(mess_cd_get_cdrom_file(state->cdrom)))
										{
											state->towns_cd.extra_status = 0;
										}
										else
											state->towns_cd.extra_status++;
									}
									break;
							}
							break;
						case 0x06:  // read CD-DA state
							switch(state->towns_cd.extra_status)
							{
								case 1:  // st2 = track number
									towns_cd_set_status(space->machine,0x18,
										0x00,towns_cd_get_track(space->machine),0x00);
									state->towns_cd.extra_status++;
									break;
								case 2:  // st0/1/2 = MSF from beginning of current track
									addr = cdda_get_audio_lba(state->cdda);
									addr = lba_to_msf(addr - state->towns_cd.cdda_current);
									towns_cd_set_status(space->machine,0x19,
										(addr & 0xff0000) >> 16,(addr & 0x00ff00) >> 8,addr & 0x0000ff);
									state->towns_cd.extra_status++;
									break;
								case 3:  // st1/2 = current MSF
									addr = cdda_get_audio_lba(state->cdda);
									addr = lba_to_msf(addr);  // this data is incorrect, but will do until exact meaning is found
									towns_cd_set_status(space->machine,0x19,
										0x00,(addr & 0xff0000) >> 16,(addr & 0x00ff00) >> 8);
									state->towns_cd.extra_status++;
									break;
								case 4:
									addr = cdda_get_audio_lba(state->cdda);
									addr = lba_to_msf(addr);  // this data is incorrect, but will do until exact meaning is found
									towns_cd_set_status(space->machine,0x20,
										addr & 0x0000ff,0x00,0x00);
									state->towns_cd.extra_status = 0;
									break;
							}
							break;
						case 0x84:
							towns_cd_set_status(space->machine,0x11,0x00,0x00,0x00);
							state->towns_cd.extra_status = 0;
							break;
						case 0x85:
							towns_cd_set_status(space->machine,0x12,0x00,0x00,0x00);
							state->towns_cd.extra_status = 0;
							break;
					}
				}
			}
			logerror("CD: reading command status port (%i), returning %02x\n",state->towns_cd.cmd_status_ptr,ret);
			state->towns_cd.cmd_status_ptr++;
			if(state->towns_cd.cmd_status_ptr > 3)
			{
				state->towns_cd.cmd_status_ptr = 0;
/*              if(state->towns_cd.extra_status != 0)
                {
                    towns_cdrom_set_irq(space->machine,TOWNS_CD_IRQ_MPU,1);
                    state->towns_cd.status |= 0x02;
                }*/
			}
			return ret;
		default:
			return 0x00;
	}
}

static WRITE8_HANDLER(towns_cdrom_w)
{
	towns_state* state = space->machine->driver_data<towns_state>();
	int x;
	switch(offset)
	{
		case 0x00: // status
			if(data & 0x80)
				towns_cdrom_set_irq(space->machine,TOWNS_CD_IRQ_MPU,0);
			if(data & 0x40)
				towns_cdrom_set_irq(space->machine,TOWNS_CD_IRQ_DMA,0);
			if(data & 0x04)
				logerror("CD: sub MPU reset\n");
			state->towns_cd.mpu_irq_enable = data & 0x02;
			state->towns_cd.dma_irq_enable = data & 0x01;
			logerror("CD: status write %02x\n",data);
			break;
		case 0x01: // command
			state->towns_cd.command = data;
			towns_cdrom_execute_command(state->cdrom);
			logerror("CD: command %02x sent\n",data);
			logerror("CD: parameters: %02x %02x %02x %02x %02x %02x %02x %02x\n",
				state->towns_cd.parameter[7],state->towns_cd.parameter[6],state->towns_cd.parameter[5],
				state->towns_cd.parameter[4],state->towns_cd.parameter[3],state->towns_cd.parameter[2],
				state->towns_cd.parameter[1],state->towns_cd.parameter[0]);
			break;
		case 0x02: // parameter
			for(x=7;x>0;x--)
				state->towns_cd.parameter[x] = state->towns_cd.parameter[x-1];
			state->towns_cd.parameter[0] = data;
			logerror("CD: parameter %02x added\n",data);
			break;
		case 0x03:
			// TODO: software transfer mode (bit 3)
			if(data & 0x10)
			{
				state->towns_cd.status |= 0x10;  // DMA transfer begin
				state->towns_cd.status &= ~0x20;  // not a software transfer
				if(state->towns_cd.buffer_ptr < 0)
				{
					state->towns_cd.buffer_ptr = 0;
					timer_adjust_oneshot(state->towns_cd.read_timer,ATTOTIME_IN_HZ(300000),1);
				}
			}
			logerror("CD: transfer mode write %02x\n",data);
			break;
		default:
			logerror("CD: write %02x to port %02x\n",data,offset*2);
	}
}


/* CMOS RTC
 * 0x70: Data port
 * 0x80: Register select
 */
static READ32_HANDLER(towns_rtc_r)
{
	towns_state* state = space->machine->driver_data<towns_state>();
	if(ACCESSING_BITS_0_7)
		return 0x80 | state->towns_rtc_reg[state->towns_rtc_select];

	return 0x00;
}

static WRITE32_HANDLER(towns_rtc_w)
{
	towns_state* state = space->machine->driver_data<towns_state>();
	if(ACCESSING_BITS_0_7)
		state->towns_rtc_data = data;
}

static WRITE32_HANDLER(towns_rtc_select_w)
{
	towns_state* state = space->machine->driver_data<towns_state>();
	if(ACCESSING_BITS_0_7)
	{
		if(data & 0x80)
		{
			if(data & 0x01)
				state->towns_rtc_select = state->towns_rtc_data & 0x0f;
		}
	}
}

static void rtc_hour(running_machine* machine)
{
	towns_state* state = machine->driver_data<towns_state>();

	state->towns_rtc_reg[4]++;
	if(state->towns_rtc_reg[4] > 4 && state->towns_rtc_reg[5] == 2)
	{
		state->towns_rtc_reg[4] = 0;
		state->towns_rtc_reg[5] = 0;
	}
	else if(state->towns_rtc_reg[4] > 9)
	{
		state->towns_rtc_reg[4] = 0;
		state->towns_rtc_reg[5]++;
	}
}

static void rtc_minute(running_machine* machine)
{
	towns_state* state = machine->driver_data<towns_state>();

	state->towns_rtc_reg[2]++;
	if(state->towns_rtc_reg[2] > 9)
	{
		state->towns_rtc_reg[2] = 0;
		state->towns_rtc_reg[3]++;
		if(state->towns_rtc_reg[3] > 5)
		{
			state->towns_rtc_reg[3] = 0;
			rtc_hour(machine);
		}
	}
}

static TIMER_CALLBACK(rtc_second)
{
	towns_state* state = machine->driver_data<towns_state>();
	// increase RTC time by one second
	state->towns_rtc_reg[0]++;
	if(state->towns_rtc_reg[0] > 9)
	{
		state->towns_rtc_reg[0] = 0;
		state->towns_rtc_reg[1]++;
		if(state->towns_rtc_reg[1] > 5)
		{
			state->towns_rtc_reg[1] = 0;
			rtc_minute(machine);
		}
	}
}

// SCSI controller - I/O ports 0xc30 and 0xc32
static READ8_HANDLER(towns_scsi_r)
{
	logerror("scsi_r (offset %i) read\n",offset);
	if(offset == 2)
		return 0x08;
	return 0x00;
}

// some unknown ports...
static READ8_HANDLER(towns_41ff_r)
{
	logerror("I/O port 0x41ff read\n");
	return 0x01;
}

static IRQ_CALLBACK( towns_irq_callback )
{
	towns_state* state = device->machine->driver_data<towns_state>();
	running_device* pic1 = state->pic_master;
	running_device* pic2 = state->pic_slave;
	int r;

	r = pic8259_acknowledge(pic2);
	if(r == 0)
	{
		r = pic8259_acknowledge(pic1);
	}

	return r;
}

// YM3438 interrupt (IRQ 13)
void towns_fm_irq(running_device* device, int irq)
{
	towns_state* state = device->machine->driver_data<towns_state>();
	running_device* pic = state->pic_slave;
	if(irq)
	{
		state->towns_fm_irq_flag = 1;
		pic8259_ir5_w(pic, 1);
		logerror("PIC: IRQ13 (FM) set high\n");
	}
	else
	{
		state->towns_fm_irq_flag = 0;
		pic8259_ir5_w(pic, 0);
		logerror("PIC: IRQ13 (FM) set low\n");
	}
}

// PCM interrupt (IRQ 13)
void towns_pcm_irq(running_device* device, int channel)
{
	towns_state* state = device->machine->driver_data<towns_state>();
	running_device* pic = state->pic_slave;

	state->towns_pcm_irq_flag = 1;
	if(state->towns_pcm_channel_mask & (1 << channel))
	{
		state->towns_pcm_channel_flag |= (1 << channel);
		pic8259_ir5_w(pic, 1);
		logerror("PIC: IRQ13 (PCM) set high\n");
	}
}

static WRITE_LINE_DEVICE_HANDLER( towns_pic_irq )
{
	cputag_set_input_line(device->machine, "maincpu", 0, state ? HOLD_LINE : CLEAR_LINE);
//  logerror("PIC#1: set IRQ line to %i\n",interrupt);
}

static WRITE_LINE_DEVICE_HANDLER( towns_pit_out0_changed )
{
	towns_state* tstate = device->machine->driver_data<towns_state>();
	running_device* dev = tstate->pic_master;

	if(tstate->towns_timer_mask & 0x01)
	{
		pic8259_ir0_w(dev, state);
		logerror("PIC: IRQ0 (PIT Timer) set to %i\n",state);
	}
}

static WRITE_LINE_DEVICE_HANDLER( towns_pit_out1_changed )
{
	towns_state* tstate = device->machine->driver_data<towns_state>();
//  running_device* dev = tstate->pic_master;

	if(tstate->towns_timer_mask & 0x02)
	{
	//  pic8259_ir0_w(dev, state);
	}
}

static ADDRESS_MAP_START(towns_mem, ADDRESS_SPACE_PROGRAM, 32)
  // memory map based on FM-Towns/Bochs (Bochs modified to emulate the FM-Towns)
  // may not be (and probably is not) correct
  AM_RANGE(0x00000000, 0x000bffff) AM_RAM
  AM_RANGE(0x000c0000, 0x000c7fff) AM_READWRITE8(towns_gfx_r,towns_gfx_w,0xffffffff)
  AM_RANGE(0x000c8000, 0x000cafff) AM_READWRITE8(towns_spriteram_low_r,towns_spriteram_low_w,0xffffffff)
  AM_RANGE(0x000cb000, 0x000cbfff) AM_READ_BANK("bank6") AM_WRITE_BANK("bank7")
  AM_RANGE(0x000cc000, 0x000cff7f) AM_RAM
  AM_RANGE(0x000cff80, 0x000cffff) AM_READWRITE8(towns_video_cff80_mem_r,towns_video_cff80_mem_w,0xffffffff)
  AM_RANGE(0x000d0000, 0x000d7fff) AM_RAM
  AM_RANGE(0x000d8000, 0x000d9fff) AM_READWRITE8(towns_cmos_low_r,towns_cmos_low_w,0xffffffff) AM_BASE_SIZE_GENERIC(nvram) // CMOS? RAM
  AM_RANGE(0x000da000, 0x000effff) AM_RAM //READWRITE(SMH_BANK(11),SMH_BANK(11))
  AM_RANGE(0x000f0000, 0x000f7fff) AM_RAM //READWRITE(SMH_BANK(12),SMH_BANK(12))
  AM_RANGE(0x000f8000, 0x000fffff) AM_READ_BANK("bank11") AM_WRITE_BANK("bank12")
  AM_RANGE(0x00100000, 0x005fffff) AM_RAM  // some extra RAM
  AM_RANGE(0x80000000, 0x8007ffff) AM_READWRITE8(towns_gfx_high_r,towns_gfx_high_w,0xffffffff) AM_MIRROR(0x180000) // VRAM
  AM_RANGE(0x81000000, 0x8101ffff) AM_READWRITE8(towns_spriteram_r,towns_spriteram_w,0xffffffff) // Sprite RAM
  AM_RANGE(0xc2000000, 0xc207ffff) AM_ROM AM_REGION("user",0x000000)  // OS ROM
  AM_RANGE(0xc2080000, 0xc20fffff) AM_ROM AM_REGION("user",0x100000)  // DIC ROM
  AM_RANGE(0xc2100000, 0xc213ffff) AM_ROM AM_REGION("user",0x180000)  // FONT ROM
  AM_RANGE(0xc2140000, 0xc2141fff) AM_READWRITE8(towns_cmos_r,towns_cmos_w,0xffffffff) // CMOS (mirror?)
  AM_RANGE(0xc2180000, 0xc21fffff) AM_ROM AM_REGION("user",0x080000)  // F20 ROM
  AM_RANGE(0xc2200000, 0xc220ffff) AM_DEVREADWRITE8("pcm",rf5c68_mem_r,rf5c68_mem_w,0xffffffff)  // WAVE RAM
  AM_RANGE(0xfffc0000, 0xffffffff) AM_ROM AM_REGION("user",0x200000)  // SYSTEM ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START(marty_mem, ADDRESS_SPACE_PROGRAM, 32)
  AM_RANGE(0x00000000, 0x000bffff) AM_RAM
  AM_RANGE(0x000c0000, 0x000c7fff) AM_READWRITE8(towns_gfx_r,towns_gfx_w,0xffffffff)
  AM_RANGE(0x000c8000, 0x000cafff) AM_READWRITE8(towns_spriteram_low_r,towns_spriteram_low_w,0xffffffff)
  AM_RANGE(0x000cb000, 0x000cbfff) AM_READ_BANK("bank6") AM_WRITE_BANK("bank7")
  AM_RANGE(0x000cc000, 0x000cff7f) AM_RAM
  AM_RANGE(0x000cff80, 0x000cffff) AM_READWRITE8(towns_video_cff80_r,towns_video_cff80_w,0xffffffff)
  AM_RANGE(0x000d0000, 0x000d7fff) AM_RAM
  AM_RANGE(0x000d8000, 0x000d9fff) AM_READWRITE8(towns_cmos_low_r,towns_cmos_low_w,0xffffffff) AM_BASE_SIZE_GENERIC(nvram) // CMOS? RAM
  AM_RANGE(0x000da000, 0x000effff) AM_RAM //READWRITE(SMH_BANK(11),SMH_BANK(11))
  AM_RANGE(0x000f0000, 0x000f7fff) AM_RAM //READWRITE(SMH_BANK(12),SMH_BANK(12))
  AM_RANGE(0x000f8000, 0x000fffff) AM_READ_BANK("bank11") AM_WRITE_BANK("bank12")
  AM_RANGE(0x00100000, 0x005fffff) AM_RAM  // some extra RAM - the Marty has 6MB RAM (not upgradable)
  AM_RANGE(0x00600000, 0x0067ffff) AM_ROM AM_REGION("user",0x000000)  // OS
  AM_RANGE(0x00680000, 0x0087ffff) AM_ROM AM_REGION("user",0x280000)  // EX ROM
  AM_RANGE(0x00a00000, 0x00a7ffff) AM_READWRITE8(towns_gfx_high_r,towns_gfx_high_w,0xffffffff) AM_MIRROR(0x180000) // VRAM
  AM_RANGE(0x00b00000, 0x00b7ffff) AM_ROM AM_REGION("user",0x180000)  // FONT
  AM_RANGE(0x00c00000, 0x00c1ffff) AM_READWRITE8(towns_spriteram_r,towns_spriteram_w,0xffffffff) // Sprite RAM
  //AM_RANGE(0x00d00000, 0x00dfffff) AM_RAM // ?? - used by ssf2
  AM_RANGE(0x00e80000, 0x00efffff) AM_ROM AM_REGION("user",0x100000)  // DIC ROM
  AM_RANGE(0x00f00000, 0x00f7ffff) AM_ROM AM_REGION("user",0x180000)  // FONT
  AM_RANGE(0x00f80000, 0x00f8ffff) AM_DEVREADWRITE8("pcm",rf5c68_mem_r,rf5c68_mem_w,0xffffffff)  // WAVE RAM
  AM_RANGE(0x00fc0000, 0x00ffffff) AM_ROM AM_REGION("user",0x200000)  // SYSTEM ROM
  AM_RANGE(0xfffc0000, 0xffffffff) AM_ROM AM_REGION("user",0x200000)  // SYSTEM ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( towns_io , ADDRESS_SPACE_IO, 32)
  // I/O ports derived from FM Towns/Bochs, these are specific to the FM Towns
  // System ports
  AM_RANGE(0x0000,0x0003) AM_DEVREADWRITE8("pic8259_master", pic8259_r, pic8259_w, 0x00ff00ff)
  AM_RANGE(0x0010,0x0013) AM_DEVREADWRITE8("pic8259_slave", pic8259_r, pic8259_w, 0x00ff00ff)
  AM_RANGE(0x0020,0x0033) AM_READWRITE8(towns_system_r,towns_system_w, 0xffffffff)
  AM_RANGE(0x0040,0x0047) AM_DEVREADWRITE8("pit",pit8253_r, pit8253_w, 0x00ff00ff)
  AM_RANGE(0x0060,0x0063) AM_READWRITE8(towns_port60_r, towns_port60_w, 0x000000ff)
  AM_RANGE(0x006c,0x006f) AM_READWRITE8(towns_sys6c_r,towns_sys6c_w, 0x000000ff)
  // 0x0070/0x0080 - CMOS RTC
  AM_RANGE(0x0070,0x0073) AM_READWRITE(towns_rtc_r,towns_rtc_w)
  AM_RANGE(0x0080,0x0083) AM_WRITE(towns_rtc_select_w)
  // DMA controllers (uPD71071)
  AM_RANGE(0x00a0,0x00af) AM_READWRITE8(towns_dma1_r, towns_dma1_w, 0xffffffff)
  AM_RANGE(0x00b0,0x00bf) AM_READWRITE8(towns_dma2_r, towns_dma2_w, 0xffffffff)
  // Floppy controller
  AM_RANGE(0x0200,0x020f) AM_READWRITE8(towns_floppy_r, towns_floppy_w, 0xffffffff)
  // CRTC / Video
  AM_RANGE(0x0400,0x0403) AM_NOP  // R/O (0x400)
  AM_RANGE(0x0404,0x0407) AM_READWRITE(towns_video_404_r, towns_video_404_w)  // R/W (0x404)
  AM_RANGE(0x0440,0x045f) AM_READWRITE8(towns_video_440_r, towns_video_440_w, 0xffffffff)
  // System port
  AM_RANGE(0x0480,0x0483) AM_READWRITE8(towns_sys480_r,towns_sys480_w,0x000000ff)  // R/W (0x480)
  // CD-ROM
  AM_RANGE(0x04c0,0x04cf) AM_READWRITE8(towns_cdrom_r,towns_cdrom_w,0x00ff00ff)
  // Joystick / Mouse ports
  AM_RANGE(0x04d0,0x04d3) AM_READ(towns_padport_r)
  // Sound (YM3438 [FM], RF5c68 [PCM])
  AM_RANGE(0x04d4,0x04d7) AM_WRITE(towns_pad_mask_w)
  AM_RANGE(0x04d8,0x04df) AM_DEVREADWRITE8("fm",ym3438_r,ym3438_w,0x00ff00ff)
  AM_RANGE(0x04e0,0x04e3) AM_NOP  // R/W  -- volume ports
  AM_RANGE(0x04e8,0x04ef) AM_READWRITE8(towns_sound_ctrl_r,towns_sound_ctrl_w,0xffffffff)
  AM_RANGE(0x04f0,0x04fb) AM_DEVWRITE8("pcm",rf5c68_w,0xffffffff)
  // CRTC / Video
  AM_RANGE(0x05c8,0x05cb) AM_READWRITE8(towns_video_5c8_r, towns_video_5c8_w, 0xffffffff)
  // System ports
  AM_RANGE(0x05e8,0x05ef) AM_READWRITE(towns_sys5e8_r, towns_sys5e8_w)
  // Keyboard (8042 MCU)
  AM_RANGE(0x0600,0x0607) AM_READWRITE8(towns_keyboard_r, towns_keyboard_w,0x00ff00ff)
  // SCSI controller
  AM_RANGE(0x0c30,0x0c33) AM_READ8(towns_scsi_r,0xffffffff)
  // CMOS
  AM_RANGE(0x3000,0x3fff) AM_READWRITE8(towns_cmos8_r, towns_cmos8_w,0x00ff00ff)
  // Something (MS-DOS wants this 0x41ff to be 1)
  AM_RANGE(0x41fc,0x41ff) AM_READ8(towns_41ff_r,0xff000000)
  // CRTC / Video (again)
  AM_RANGE(0xfd90,0xfda3) AM_READWRITE8(towns_video_fd90_r, towns_video_fd90_w, 0xffffffff)
  AM_RANGE(0xff80,0xffff) AM_READWRITE8(towns_video_cff80_r,towns_video_cff80_w,0xffffffff)

ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( towns )
  PORT_START("ctrltype")
    PORT_CATEGORY_CLASS(0x0f,0x01,"Joystick port 1")
    PORT_CATEGORY_ITEM(0x00,"Nothing",10)
    PORT_CATEGORY_ITEM(0x01,"Standard 2-button joystick",11)
    PORT_CATEGORY_ITEM(0x02,"Mouse",12)
    PORT_CATEGORY_CLASS(0xf0,0x20,"Joystick port 2")
    PORT_CATEGORY_ITEM(0x00,"Nothing",20)
    PORT_CATEGORY_ITEM(0x10,"Standard 2-button joystick",21)
    PORT_CATEGORY_ITEM(0x20,"Mouse",22)

// Keyboard
  PORT_START( "key1" )  // scancodes 0x00-0x1f
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
    PORT_BIT(0x00004000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("\xEF\xBF\xA5") PORT_CODE(KEYCODE_BACKSLASH)
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

  PORT_START( "key2" )  // scancodes 0x20-0x3f
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

  PORT_START("key3")  // scancodes 0x40-0x5f
    PORT_BIT(0x00000001,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Tenkey 6") PORT_CODE(KEYCODE_6_PAD)
    PORT_BIT(0x00000002,IP_ACTIVE_HIGH,IPT_UNUSED)
    PORT_BIT(0x00000004,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Tenkey 1") PORT_CODE(KEYCODE_1_PAD)
    PORT_BIT(0x00000008,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Tenkey 2") PORT_CODE(KEYCODE_2_PAD)
    PORT_BIT(0x00000010,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Tenkey 3") PORT_CODE(KEYCODE_3_PAD)
    PORT_BIT(0x00000020,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Tenkey Enter") PORT_CODE(KEYCODE_ENTER_PAD)
    PORT_BIT(0x00000040,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Tenkey 0") PORT_CODE(KEYCODE_0_PAD)
    PORT_BIT(0x00000080,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Tenkey .") PORT_CODE(KEYCODE_DEL_PAD)
    PORT_BIT(0x00000100,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("INS(?)") PORT_CODE(KEYCODE_INSERT)
    PORT_BIT(0x00000200,IP_ACTIVE_HIGH,IPT_UNUSED)
    PORT_BIT(0x00000400,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Tenkey 000")
    PORT_BIT(0x00000800,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("DEL(?)/EL") PORT_CODE(KEYCODE_DEL)
    PORT_BIT(0x00001000,IP_ACTIVE_HIGH,IPT_UNUSED)
    PORT_BIT(0x00002000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Up") PORT_CODE(KEYCODE_UP)
    PORT_BIT(0x00004000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("HOME") PORT_CODE(KEYCODE_HOME)
    PORT_BIT(0x00008000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Left") PORT_CODE(KEYCODE_LEFT)
    PORT_BIT(0x00010000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Down") PORT_CODE(KEYCODE_DOWN)
    PORT_BIT(0x00020000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Right") PORT_CODE(KEYCODE_RIGHT)
    PORT_BIT(0x00040000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Ctrl") PORT_CODE(KEYCODE_LCONTROL)
    PORT_BIT(0x00080000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Shift") PORT_CODE(KEYCODE_LSHIFT)
    PORT_BIT(0x00100000,IP_ACTIVE_HIGH,IPT_UNUSED)
    PORT_BIT(0x00200000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("CAP") PORT_CODE(KEYCODE_CAPSLOCK)
    PORT_BIT(0x00400000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("\xE3\x81\xB2\xE3\x82\x89\xE3\x81\x8C\xE3\x81\xAA (Hiragana)") PORT_CODE(KEYCODE_RCONTROL)
    PORT_BIT(0x00800000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Key 0x57")
    PORT_BIT(0x01000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Key 0x58")
    PORT_BIT(0x02000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Kana???") PORT_CODE(KEYCODE_RALT)
    PORT_BIT(0x04000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("\xE3\x82\xAB\xE3\x82\xBF\xE3\x82\xAB\xE3\x83\x8A (Katakana)") PORT_CODE(KEYCODE_RWIN)
    PORT_BIT(0x08000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("PF12") PORT_CODE(KEYCODE_F12)
    PORT_BIT(0x10000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("ALT") PORT_CODE(KEYCODE_LALT)
    PORT_BIT(0x20000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("PF1") PORT_CODE(KEYCODE_F1)
    PORT_BIT(0x40000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("PF2") PORT_CODE(KEYCODE_F2)
    PORT_BIT(0x80000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("PF3") PORT_CODE(KEYCODE_F3)

  PORT_START("key4")
    PORT_BIT(0x00000001,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("PF4") PORT_CODE(KEYCODE_F4)
    PORT_BIT(0x00000002,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("PF5") PORT_CODE(KEYCODE_F5)
    PORT_BIT(0x00000004,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("PF6") PORT_CODE(KEYCODE_F6)
    PORT_BIT(0x00000008,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("PF7") PORT_CODE(KEYCODE_F7)
    PORT_BIT(0x00000010,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("PF8") PORT_CODE(KEYCODE_F8)
    PORT_BIT(0x00000020,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("PF9") PORT_CODE(KEYCODE_F9)
    PORT_BIT(0x00000040,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("PF10") PORT_CODE(KEYCODE_F10)
    PORT_BIT(0x00000080,IP_ACTIVE_HIGH,IPT_UNUSED)
    PORT_BIT(0x00000100,IP_ACTIVE_HIGH,IPT_UNUSED)
    PORT_BIT(0x00000200,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("PF11") PORT_CODE(KEYCODE_F11)
    PORT_BIT(0x00000400,IP_ACTIVE_HIGH,IPT_UNUSED)
    PORT_BIT(0x00000800,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Key 0x6b")
    PORT_BIT(0x00001000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Key 0x6c")
    PORT_BIT(0x00002000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Key 0x6d")
    PORT_BIT(0x00004000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Key 0x6e")
    PORT_BIT(0x00008000,IP_ACTIVE_HIGH,IPT_UNUSED)
    PORT_BIT(0x00010000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Key 0x70")
    PORT_BIT(0x00020000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Key 0x71")
    PORT_BIT(0x00040000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Key 0x72")
    PORT_BIT(0x00080000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Key 0x73")
    PORT_BIT(0x00100000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("PF13")
    PORT_BIT(0x00200000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("PF14")
    PORT_BIT(0x00400000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("PF15")
    PORT_BIT(0x00800000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("PF16")
    PORT_BIT(0x01000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("PF17")
    PORT_BIT(0x02000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("PF18")
    PORT_BIT(0x04000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("PF19")
    PORT_BIT(0x08000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("PF20")
    PORT_BIT(0x10000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("BREAK")
    PORT_BIT(0x20000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("COPY")

  PORT_START("joy1")
    PORT_BIT(0x00000001,IP_ACTIVE_LOW, IPT_JOYSTICK_UP) PORT_PLAYER(1) PORT_CATEGORY(11)
    PORT_BIT(0x00000002,IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN) PORT_PLAYER(1) PORT_CATEGORY(11)
    PORT_BIT(0x00000004,IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT) PORT_PLAYER(1) PORT_CATEGORY(11)
    PORT_BIT(0x00000008,IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT) PORT_PLAYER(1) PORT_CATEGORY(11)
    PORT_BIT(0x00000010,IP_ACTIVE_LOW, IPT_UNUSED) PORT_CATEGORY(11)
    PORT_BIT(0x00000020,IP_ACTIVE_LOW, IPT_UNUSED) PORT_CATEGORY(11)
    PORT_BIT(0x00000040,IP_ACTIVE_HIGH, IPT_UNUSED) PORT_CATEGORY(11)
    PORT_BIT(0x00000080,IP_ACTIVE_HIGH, IPT_UNUSED) PORT_CATEGORY(11)

  PORT_START("joy1_ex")
    PORT_BIT(0x00000001,IP_ACTIVE_HIGH, IPT_START) PORT_NAME("1P Run") PORT_PLAYER(1) PORT_CATEGORY(11)
    PORT_BIT(0x00000002,IP_ACTIVE_HIGH, IPT_BUTTON3) PORT_NAME("1P Select") PORT_PLAYER(1) PORT_CATEGORY(11)
    PORT_BIT(0x00000004,IP_ACTIVE_LOW, IPT_UNUSED) PORT_CATEGORY(11)
    PORT_BIT(0x00000008,IP_ACTIVE_LOW, IPT_UNUSED) PORT_CATEGORY(11)
    PORT_BIT(0x00000010,IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_PLAYER(1) PORT_CATEGORY(11)
    PORT_BIT(0x00000020,IP_ACTIVE_HIGH, IPT_BUTTON2) PORT_PLAYER(1) PORT_CATEGORY(11)
    PORT_BIT(0x00000040,IP_ACTIVE_HIGH, IPT_UNUSED) PORT_CATEGORY(11)
    PORT_BIT(0x00000080,IP_ACTIVE_HIGH, IPT_UNUSED) PORT_CATEGORY(11)

  PORT_START("joy2")
    PORT_BIT(0x00000001,IP_ACTIVE_LOW, IPT_JOYSTICK_UP) PORT_PLAYER(2) PORT_CATEGORY(21)
    PORT_BIT(0x00000002,IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN) PORT_PLAYER(2) PORT_CATEGORY(21)
    PORT_BIT(0x00000004,IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT) PORT_PLAYER(2) PORT_CATEGORY(21)
    PORT_BIT(0x00000008,IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT) PORT_PLAYER(2) PORT_CATEGORY(21)
    PORT_BIT(0x00000010,IP_ACTIVE_LOW, IPT_UNUSED) PORT_CATEGORY(21)
    PORT_BIT(0x00000020,IP_ACTIVE_LOW, IPT_UNUSED) PORT_CATEGORY(21)
    PORT_BIT(0x00000040,IP_ACTIVE_HIGH, IPT_UNUSED) PORT_CATEGORY(21)
    PORT_BIT(0x00000080,IP_ACTIVE_HIGH, IPT_UNUSED) PORT_CATEGORY(21)

  PORT_START("joy2_ex")
    PORT_BIT(0x00000001,IP_ACTIVE_HIGH, IPT_START) PORT_NAME("2P Run") PORT_PLAYER(2) PORT_CATEGORY(21)
    PORT_BIT(0x00000002,IP_ACTIVE_HIGH, IPT_BUTTON3) PORT_NAME("2P Select") PORT_PLAYER(2) PORT_CATEGORY(21)
    PORT_BIT(0x00000004,IP_ACTIVE_LOW, IPT_UNUSED) PORT_CATEGORY(21)
    PORT_BIT(0x00000008,IP_ACTIVE_LOW, IPT_UNUSED) PORT_CATEGORY(21)
    PORT_BIT(0x00000010,IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_PLAYER(2) PORT_CATEGORY(21)
    PORT_BIT(0x00000020,IP_ACTIVE_HIGH, IPT_BUTTON2) PORT_PLAYER(2) PORT_CATEGORY(21)
    PORT_BIT(0x00000040,IP_ACTIVE_HIGH, IPT_UNUSED) PORT_CATEGORY(21)
    PORT_BIT(0x00000080,IP_ACTIVE_HIGH, IPT_UNUSED) PORT_CATEGORY(21)

  PORT_START("mouse1")  // buttons
	PORT_BIT( 0x00000001, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_NAME("Left mouse button") PORT_CODE(MOUSECODE_BUTTON1)
	PORT_BIT( 0x00000002, IP_ACTIVE_HIGH, IPT_BUTTON2) PORT_NAME("Right mouse button") PORT_CODE(MOUSECODE_BUTTON2)

  PORT_START("mouse2")  // X-axis
	PORT_BIT( 0xff, 0x00, IPT_MOUSE_X) PORT_SENSITIVITY(100) PORT_KEYDELTA(0) PORT_PLAYER(1)

  PORT_START("mouse3")  // Y-axis
	PORT_BIT( 0xff, 0x00, IPT_MOUSE_Y) PORT_SENSITIVITY(100) PORT_KEYDELTA(0) PORT_PLAYER(1)

INPUT_PORTS_END

static INPUT_PORTS_START( marty )
  PORT_INCLUDE(towns)
  // Consoles don't have keyboards...
  PORT_MODIFY("key1")
	PORT_BIT(0xffffffff,IP_ACTIVE_LOW,IPT_UNUSED)
  PORT_MODIFY("key2")
	PORT_BIT(0xffffffff,IP_ACTIVE_LOW,IPT_UNUSED)
  PORT_MODIFY("key3")
	PORT_BIT(0xffffffff,IP_ACTIVE_LOW,IPT_UNUSED)
  PORT_MODIFY("key4")
	PORT_BIT(0xffffffff,IP_ACTIVE_LOW,IPT_UNUSED)
INPUT_PORTS_END

static DRIVER_INIT( towns )
{
	towns_state* state = machine->driver_data<towns_state>();
	state->pic_master = machine->device("pic8259_master");
	state->pic_slave = machine->device("pic8259_slave");
	state->towns_vram = auto_alloc_array(machine,UINT32,0x20000);
	state->towns_cmos = machine->generic.nvram.u8;
	state->towns_gfxvram = auto_alloc_array(machine,UINT8,0x80000);
	state->towns_txtvram = auto_alloc_array(machine,UINT8,0x20000);
	//towns_sprram = auto_alloc_array(machine,UINT8,0x20000);
	state->towns_serial_rom = auto_alloc_array(machine,UINT8,256/8);
	towns_init_serial_rom(machine);
	towns_init_rtc(machine);
	state->towns_rtc_timer = timer_alloc(machine,rtc_second,NULL);
	state->towns_kb_timer = timer_alloc(machine,poll_keyboard,NULL);
	state->towns_mouse_timer = timer_alloc(machine,towns_mouse_timeout,NULL);

	// CD-ROM init
	state->towns_cd.read_timer = timer_alloc(machine,towns_cdrom_read_byte,(void*)machine->device("dma_1"));

	cpu_set_irq_callback(machine->device("maincpu"), towns_irq_callback);
}

static DRIVER_INIT( marty )
{
	towns_state* state = machine->driver_data<towns_state>();
	DRIVER_INIT_CALL(towns);
	state->towns_machine_id = 0x034a;
}

static MACHINE_RESET( towns )
{
	towns_state* state = machine->driver_data<towns_state>();
	state->maincpu = machine->device("maincpu");
	state->dma_1 = machine->device("dma_1");
	state->dma_2 = machine->device("dma_2");
	state->fdc = machine->device("fdc");
	state->pic_master = machine->device("pic8259_master");
	state->pic_slave = machine->device("pic8259_slave");
	state->pit = machine->device("pit");
	state->messram = machine->device("messram");
	state->cdrom = machine->device("cdrom");
	state->cdda = machine->device("cdda");
	state->ftimer = 0x00;
	state->nmi_mask = 0x00;
	state->compat_mode = 0x00;
	state->towns_ankcg_enable = 0x00;
	state->towns_mainmem_enable = 0x00;
	state->towns_system_port = 0x00;
	state->towns_ram_enable = 0x00;
	towns_update_video_banks(cpu_get_address_space(state->maincpu,ADDRESS_SPACE_PROGRAM));
	state->towns_kb_status = 0x18;
	state->towns_kb_irq1_enable = 0;
	state->towns_pad_mask = 0x7f;
	state->towns_mouse_output = MOUSE_START;
	state->towns_cd.status = 0x01;  // CDROM controller ready
	state->towns_cd.buffer_ptr = -1;
	timer_adjust_periodic(state->towns_rtc_timer,attotime_zero,0,ATTOTIME_IN_HZ(1));
	timer_adjust_periodic(state->towns_kb_timer,attotime_zero,0,ATTOTIME_IN_MSEC(10));
}

static const struct pit8253_config towns_pit8253_config =
{
	{
		{
			307200,
			DEVCB_NULL,
			DEVCB_LINE(towns_pit_out0_changed)
		},
		{
			307200,
			DEVCB_NULL,
			DEVCB_LINE(towns_pit_out1_changed)
		},
		{
			307200,
			DEVCB_NULL,
			DEVCB_NULL
		}
	}
};

static const struct pic8259_interface towns_pic8259_master_config =
{
	DEVCB_LINE(towns_pic_irq)
};


static const struct pic8259_interface towns_pic8259_slave_config =
{
	DEVCB_DEVICE_LINE("pic8259_master", pic8259_ir7_w)
};

static const wd17xx_interface towns_mb8877a_interface =
{
	DEVCB_NULL,
	DEVCB_DEVICE_LINE("pic8259_master",towns_mb8877a_irq_w),
	DEVCB_DEVICE_LINE("dma_1", towns_mb8877a_drq_w),
	{FLOPPY_0,FLOPPY_1,0,0}
};

static FLOPPY_OPTIONS_START( towns )
	FLOPPY_OPTION( fmt_bin, "bin", "BIN disk image", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([2])
		TRACKS([77])
		SECTORS([8])
		SECTOR_LENGTH([1024])
		FIRST_SECTOR_ID([1]))
FLOPPY_OPTIONS_END

static const floppy_config towns_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_STANDARD_5_25_DSHD,
	FLOPPY_OPTIONS_NAME(towns),
	NULL
};

static const upd71071_intf towns_dma_config =
{
	"maincpu",
	1000000,
	{ towns_fdc_dma_r, 0, 0, towns_cdrom_dma_r },
	{ towns_fdc_dma_w, 0, 0, 0 }
};

static const ym3438_interface ym3438_intf =
{
	towns_fm_irq
};

static const rf5c68_interface rf5c68_intf =
{
	towns_pcm_irq
};

static const gfx_layout fnt_chars_16x16 =
{
	16,16,
	RGN_FRAC(1,1),
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7,8,9,10,11,12,13,14,15 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	16*16
};

static const gfx_layout text_chars =
{
	8,16,
	RGN_FRAC(1,1),
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7, },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 ,8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	8*16
};

static GFXDECODE_START( towns )
	GFXDECODE_ENTRY( "user",   0x180000 + 0x3d800, text_chars,  0, 16 )
	GFXDECODE_ENTRY( "user",   0x180000, fnt_chars_16x16,  0, 16 )
GFXDECODE_END

static MACHINE_CONFIG_START( towns, towns_state )

	/* basic machine hardware */
    MDRV_CPU_ADD("maincpu",I386, 16000000)
    MDRV_CPU_PROGRAM_MAP(towns_mem)
    MDRV_CPU_IO_MAP(towns_io)
	MDRV_CPU_VBLANK_INT("screen", towns_vsync_irq)

    MDRV_MACHINE_RESET(towns)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(60)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */

    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_RGB32)
    MDRV_SCREEN_SIZE(768,512)
    MDRV_SCREEN_VISIBLE_AREA(0, 768-1, 0, 512-1)
    MDRV_GFXDECODE(towns)

    /* sound hardware */
    MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("fm", YM3438, 53693100 / 7) // actual clock speed unknown
	MDRV_SOUND_CONFIG(ym3438_intf)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
	MDRV_SOUND_ADD("pcm", RF5C68, 53693100 / 7)  // actual clock speed unknown
	MDRV_SOUND_CONFIG(rf5c68_intf)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.50)
	MDRV_SOUND_ADD("cdda",CDDA,0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

    MDRV_PIT8253_ADD("pit",towns_pit8253_config)

	MDRV_PIC8259_ADD( "pic8259_master", towns_pic8259_master_config )

	MDRV_PIC8259_ADD( "pic8259_slave", towns_pic8259_slave_config )

	MDRV_MB8877_ADD("fdc",towns_mb8877a_interface)
	MDRV_FLOPPY_4_DRIVES_ADD(towns_floppy_config)

	MDRV_CDROM_ADD("cdrom")

	MDRV_UPD71071_ADD("dma_1",towns_dma_config)
	MDRV_UPD71071_ADD("dma_2",towns_dma_config)

	MDRV_NVRAM_HANDLER( generic_0fill )

    MDRV_VIDEO_START(towns)
    MDRV_VIDEO_UPDATE(towns)

	/* internal ram */
	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("6M")
MACHINE_CONFIG_END

static MACHINE_CONFIG_DERIVED( marty, towns )

	MDRV_CPU_REPLACE("maincpu",I386, 16000000)
	MDRV_CPU_PROGRAM_MAP(marty_mem)
	MDRV_CPU_IO_MAP(towns_io)
MACHINE_CONFIG_END

/* ROM definitions */
ROM_START( fmtowns )
  ROM_REGION32_LE( 0x280000, "user", 0)
	ROM_LOAD("fmt_dos.rom",  0x000000, 0x080000, CRC(112872ee) SHA1(57fd146478226f7f215caf63154c763a6d52165e) )
	ROM_LOAD("fmt_f20.rom",  0x080000, 0x080000, CRC(9f55a20c) SHA1(1920711cb66340bb741a760de187de2f76040b8c) )
	ROM_LOAD("fmt_dic.rom",  0x100000, 0x080000, CRC(82d1daa2) SHA1(7564020dba71deee27184824b84dbbbb7c72aa4e) )
	ROM_LOAD("fmt_fnt.rom",  0x180000, 0x040000, CRC(dd6fd544) SHA1(a216482ea3162f348fcf77fea78e0b2e4288091a) )
	ROM_LOAD("fmt_sys.rom",  0x200000, 0x040000, CRC(afe4ebcf) SHA1(4cd51de4fca9bd7a3d91d09ad636fa6b47a41df5) )
ROM_END

ROM_START( fmtownsa )
  ROM_REGION32_LE( 0x280000, "user", 0)
	ROM_LOAD("fmt_dos_a.rom",  0x000000, 0x080000, CRC(22270e9f) SHA1(a7e97b25ff72b14121146137db8b45d6c66af2ae) )
	ROM_LOAD("fmt_f20_a.rom",  0x080000, 0x080000, CRC(75660aac) SHA1(6a521e1d2a632c26e53b83d2cc4b0edecfc1e68c) )
	ROM_LOAD("fmt_dic_a.rom",  0x100000, 0x080000, CRC(74b1d152) SHA1(f63602a1bd67c2ad63122bfb4ffdaf483510f6a8) )
	ROM_LOAD("fmt_fnt_a.rom",  0x180000, 0x040000, CRC(0108a090) SHA1(1b5dd9d342a96b8e64070a22c3a158ca419894e1) )
	ROM_LOAD("fmt_sys_a.rom",  0x200000, 0x040000, CRC(92f3fa67) SHA1(be21404098b23465d24c4201a81c96ac01aff7ab) )
ROM_END

ROM_START( fmtmarty )
  ROM_REGION32_LE( 0x480000, "user", 0)
	ROM_LOAD("mrom.m36",  0x000000, 0x080000, CRC(9c0c060c) SHA1(5721c5f9657c570638352fa9acac57fa8d0b94bd) )
	ROM_CONTINUE(0x280000,0x180000)
	ROM_LOAD("mrom.m37",  0x400000, 0x080000, CRC(fb66bb56) SHA1(e273b5fa618373bdf7536495cd53c8aac1cce9a5) )
	ROM_CONTINUE(0x80000,0x100000)
	ROM_CONTINUE(0x180000,0x40000)
	ROM_CONTINUE(0x200000,0x40000)
ROM_END

ROM_START( carmarty )
  ROM_REGION32_LE( 0x480000, "user", 0)
	ROM_LOAD("fmt_dos.rom",  0x000000, 0x080000, CRC(2bc2af96) SHA1(99cd51c5677288ad8ef711b4ac25d981fd586884) )
	ROM_LOAD("fmt_dic.rom",  0x100000, 0x080000, CRC(82d1daa2) SHA1(7564020dba71deee27184824b84dbbbb7c72aa4e) )
	ROM_LOAD("fmt_fnt.rom",  0x180000, 0x040000, CRC(dd6fd544) SHA1(a216482ea3162f348fcf77fea78e0b2e4288091a) )
	ROM_LOAD("fmt_sys.rom",  0x200000, 0x040000, CRC(e1ff7ce1) SHA1(e6c359177e4e9fb5bbb7989c6bbf6e95c091fd88) )
	ROM_LOAD("mar_ex0.rom",  0x280000, 0x080000, CRC(e248bfbd) SHA1(0ce89952a7901dd4d256939a6bc8597f87e51ae7) )
	ROM_LOAD("mar_ex1.rom",  0x300000, 0x080000, CRC(ab2e94f0) SHA1(4b3378c772302622f8e1139ed0caa7da1ab3c780) )
	ROM_LOAD("mar_ex2.rom",  0x380000, 0x080000, CRC(ce150ec7) SHA1(1cd8c39f3b940e03f9fe999ebcf7fd693f843d04) )
	ROM_LOAD("mar_ex3.rom",  0x400000, 0x080000, CRC(582fc7fc) SHA1(a77d8014e41e9ff0f321e156c0fe1a45a0c5e58e) )
  ROM_REGION( 0x20, "serial", 0)
    ROM_LOAD("mytowns.rom",  0x00, 0x20, CRC(bc58eba6) SHA1(483087d823c3952cc29bd827e5ef36d12c57ad49) )
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT      MACHINE     INPUT    INIT    COMPANY      FULLNAME            FLAGS */
COMP( 1989, fmtowns,  0,    	0,		towns,		towns,	 towns,  "Fujitsu",   "FM-Towns",		 GAME_NOT_WORKING)
COMP( 1989, fmtownsa, fmtowns,	0,		towns,		towns,	 towns,  "Fujitsu",   "FM-Towns (alternate)", GAME_NOT_WORKING)
CONS( 1993, fmtmarty, 0,    	0,		marty,		marty,	 marty,  "Fujitsu",   "FM-Towns Marty",	 GAME_NOT_WORKING)
CONS( 1994, carmarty, fmtmarty,	0,		marty,		marty,	 marty,  "Fujitsu",   "FM-Towns Car Marty",	 GAME_NOT_WORKING)


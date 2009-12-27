/***************************************************************************

    Neo-Geo AES hardware

    Credits (from MAME neogeo.c, since this is just a minor edit of that driver):
        * This driver was made possible by the research done by
          Charles MacDonald.  For a detailed description of the Neo-Geo
          hardware, please visit his page at:
          http://cgfm2.emuviews.com/temp/mvstech.txt
        * Presented to you by the Shin Emu Keikaku team.
        * The following people have all spent probably far
          too much time on this:
          AVDB
          Bryan McPhail
          Fuzz
          Ernesto Corvi
          Andrew Prime
          Zsolt Vasvari

    MESS cartridge support by R. Belmont based on work by Michael Zapf

    Current status:
        - Cartridges run.

    ToDo :
        - Change input code to allow selection of the mahjong panel in PORT_CATEGORY.
        - Clean up code, to reduce duplication of MAME source

****************************************************************************/

#include "driver.h"
#include "cpu/m68000/m68000.h"
#include "includes/neogeo.h"
#include "machine/pd4990a.h"
#include "cpu/z80/z80.h"
#include "sound/2610intf.h"
#include "devices/aescart.h"

#include "neogeo.lh"


#define LOG_VIDEO_SYSTEM		(0)
#define LOG_CPU_COMM			(0)
#define LOG_MAIN_CPU_BANKING	(0)
#define LOG_AUDIO_CPU_BANKING	(0)



/*************************************
 *
 *  Global variables
 *
 *************************************/

static UINT8 *memcard_data;
static UINT16 *save_ram;


/*************************************
 *
 *  Forward declarations
 *
 *************************************/

//static void set_output_latch(running_machine *machine, UINT8 data);
//static void set_output_data(running_machine *machine, UINT8 data);


/*************************************
 *
 *  Main CPU interrupt generation
 *
 *************************************/

#define IRQ2CTRL_ENABLE				(0x10)
#define IRQ2CTRL_LOAD_RELATIVE		(0x20)
#define IRQ2CTRL_AUTOLOAD_VBLANK	(0x40)
#define IRQ2CTRL_AUTOLOAD_REPEAT	(0x80)


static void adjust_display_position_interrupt_timer( running_machine *machine )
{
	neogeo_state *state = (neogeo_state *)machine->driver_data;

	if ((state->display_counter + 1) != 0)
	{
		attotime period = attotime_mul(ATTOTIME_IN_HZ(NEOGEO_PIXEL_CLOCK), state->display_counter + 1);
		if (LOG_VIDEO_SYSTEM) logerror("adjust_display_position_interrupt_timer  current y: %02x  current x: %02x   target y: %x  target x: %x\n", video_screen_get_vpos(machine->primary_screen), video_screen_get_hpos(machine->primary_screen), (state->display_counter + 1) / NEOGEO_HTOTAL, (state->display_counter + 1) % NEOGEO_HTOTAL);

		timer_adjust_oneshot(state->display_position_interrupt_timer, period, 0);
	}
}


void neogeo_set_display_position_interrupt_control( running_machine *machine, UINT16 data )
{
	neogeo_state *state = (neogeo_state *)machine->driver_data;
	state->display_position_interrupt_control = data;
}


void neogeo_set_display_counter_msb( const address_space *space, UINT16 data )
{
	neogeo_state *state = (neogeo_state *)space->machine->driver_data;

	state->display_counter = (state->display_counter & 0x0000ffff) | ((UINT32)data << 16);

	if (LOG_VIDEO_SYSTEM) logerror("PC %06x: set_display_counter %08x\n", cpu_get_pc(space->cpu), state->display_counter);
}


void neogeo_set_display_counter_lsb( const address_space *space, UINT16 data )
{
	neogeo_state *state = (neogeo_state *)space->machine->driver_data;

	state->display_counter = (state->display_counter & 0xffff0000) | data;

	if (LOG_VIDEO_SYSTEM) logerror("PC %06x: set_display_counter %08x\n", cpu_get_pc(space->cpu), state->display_counter);

	if (state->display_position_interrupt_control & IRQ2CTRL_LOAD_RELATIVE)
	{
		if (LOG_VIDEO_SYSTEM) logerror("AUTOLOAD_RELATIVE ");
 		adjust_display_position_interrupt_timer(space->machine);
	}
}


static void update_interrupts( running_machine *machine )
{
	neogeo_state *state = (neogeo_state *)machine->driver_data;

	cputag_set_input_line(machine, "maincpu", 1, state->vblank_interrupt_pending ? ASSERT_LINE : CLEAR_LINE);
	cputag_set_input_line(machine, "maincpu", 2, state->display_position_interrupt_pending ? ASSERT_LINE : CLEAR_LINE);
	cputag_set_input_line(machine, "maincpu", 3, state->irq3_pending ? ASSERT_LINE : CLEAR_LINE);
}


void neogeo_acknowledge_interrupt( running_machine *machine, UINT16 data )
{
	neogeo_state *state = (neogeo_state *)machine->driver_data;

	if (data & 0x01)
		state->irq3_pending = 0;
	if (data & 0x02)
		state->display_position_interrupt_pending = 0;
	if (data & 0x04)
		state->vblank_interrupt_pending = 0;

	update_interrupts(machine);
}


static TIMER_CALLBACK( display_position_interrupt_callback )
{
	neogeo_state *state = (neogeo_state *)machine->driver_data;

	if (LOG_VIDEO_SYSTEM) logerror("--- Scanline @ %d,%d\n", video_screen_get_vpos(machine->primary_screen), video_screen_get_hpos(machine->primary_screen));

	if (state->display_position_interrupt_control & IRQ2CTRL_ENABLE)
	{
		if (LOG_VIDEO_SYSTEM) logerror("*** Scanline interrupt (IRQ2) ***  y: %02x  x: %02x\n", video_screen_get_vpos(machine->primary_screen), video_screen_get_hpos(machine->primary_screen));
		state->display_position_interrupt_pending = 1;

		update_interrupts(machine);
	}

	if (state->display_position_interrupt_control & IRQ2CTRL_AUTOLOAD_REPEAT)
	{
		if (LOG_VIDEO_SYSTEM) logerror("AUTOLOAD_REPEAT ");
		adjust_display_position_interrupt_timer(machine);
	}
}


static TIMER_CALLBACK( display_position_vblank_callback )
{
	neogeo_state *state = (neogeo_state *)machine->driver_data;

	if (state->display_position_interrupt_control & IRQ2CTRL_AUTOLOAD_VBLANK)
	{
		if (LOG_VIDEO_SYSTEM) logerror("AUTOLOAD_VBLANK ");
		adjust_display_position_interrupt_timer(machine);
	}

	/* set timer for next screen */
	timer_adjust_oneshot(state->display_position_vblank_timer, video_screen_get_time_until_pos(machine->primary_screen, NEOGEO_VBSTART, NEOGEO_VBLANK_RELOAD_HPOS), 0);
}


static TIMER_CALLBACK( vblank_interrupt_callback )
{
	neogeo_state *state = (neogeo_state *)machine->driver_data;

	if (LOG_VIDEO_SYSTEM) logerror("+++ VBLANK @ %d,%d\n", video_screen_get_vpos(machine->primary_screen), video_screen_get_hpos(machine->primary_screen));

	/* add a timer tick to the pd4990a */
	upd4990a_addretrace(state->upd4990a);

	state->vblank_interrupt_pending = 1;

	update_interrupts(machine);

	/* set timer for next screen */
	timer_adjust_oneshot(state->vblank_interrupt_timer, video_screen_get_time_until_pos(machine->primary_screen, NEOGEO_VBSTART, 0), 0);
}


static void create_interrupt_timers( running_machine *machine )
{
	neogeo_state *state = (neogeo_state *)machine->driver_data;
	state->display_position_interrupt_timer = timer_alloc(machine, display_position_interrupt_callback, NULL);
	state->display_position_vblank_timer = timer_alloc(machine, display_position_vblank_callback, NULL);
	state->vblank_interrupt_timer = timer_alloc(machine, vblank_interrupt_callback, NULL);
}


static void start_interrupt_timers( running_machine *machine )
{
	neogeo_state *state = (neogeo_state *)machine->driver_data;
	timer_adjust_oneshot(state->vblank_interrupt_timer, video_screen_get_time_until_pos(machine->primary_screen, NEOGEO_VBSTART, 0), 0);
	timer_adjust_oneshot(state->display_position_vblank_timer, video_screen_get_time_until_pos(machine->primary_screen, NEOGEO_VBSTART, NEOGEO_VBLANK_RELOAD_HPOS), 0);
}



/*************************************
 *
 *  Audio CPU interrupt generation
 *
 *************************************/

static void audio_cpu_irq(const device_config *device, int assert)
{
	neogeo_state *state = (neogeo_state *)device->machine->driver_data;
	cpu_set_input_line(state->audiocpu, 0, assert ? ASSERT_LINE : CLEAR_LINE);
}


static void audio_cpu_assert_nmi(running_machine *machine)
{
	neogeo_state *state = (neogeo_state *)machine->driver_data;
	cpu_set_input_line(state->audiocpu, INPUT_LINE_NMI, ASSERT_LINE);
}


static WRITE8_HANDLER( audio_cpu_clear_nmi_w )
{
	neogeo_state *state = (neogeo_state *)space->machine->driver_data;
	cpu_set_input_line(state->audiocpu, INPUT_LINE_NMI, CLEAR_LINE);
}



/*************************************
 *
 *  Input ports / Controllers
 *
 *************************************/

static void select_controller( running_machine *machine, UINT8 data )
{
	neogeo_state *state = (neogeo_state *)machine->driver_data;
	state->controller_select = data;
}


#if 0
static CUSTOM_INPUT( multiplexed_controller_r )
{
	int port = (FPTR)param;

	static const char *const cntrl[2][2] =
		{
			{ "IN0-0", "IN0-1" }, { "IN1-0", "IN1-1" }
		};

	return input_port_read_safe(field->port->machine, cntrl[port][controller_select & 0x01], 0x00);
}


static CUSTOM_INPUT( mahjong_controller_r )
{
	UINT32 ret;

/*
cpu #0 (PC=00C18B9A): unmapped memory word write to 00380000 = 0012 & 00FF
cpu #0 (PC=00C18BB6): unmapped memory word write to 00380000 = 001B & 00FF
cpu #0 (PC=00C18D54): unmapped memory word write to 00380000 = 0024 & 00FF
cpu #0 (PC=00C18D6C): unmapped memory word write to 00380000 = 0009 & 00FF
cpu #0 (PC=00C18C40): unmapped memory word write to 00380000 = 0000 & 00FF
*/
	switch (controller_select)
	{
	default:
	case 0x00: ret = 0x0000; break; /* nothing? */
	case 0x09: ret = input_port_read(field->port->machine, "MAHJONG1"); break;
	case 0x12: ret = input_port_read(field->port->machine, "MAHJONG2"); break;
	case 0x1b: ret = input_port_read(field->port->machine, "MAHJONG3"); break; /* player 1 normal inputs? */
	case 0x24: ret = input_port_read(field->port->machine, "MAHJONG4"); break;
	}

	return ret;
}

#endif

static WRITE16_HANDLER( io_control_w )
{
	neogeo_state *state = (neogeo_state *)space->machine->driver_data;
	switch (offset)
	{
	case 0x00: select_controller(space->machine, data & 0x00ff); break;
//	case 0x18: set_output_latch(space->machine, data & 0x00ff); break;
//	case 0x20: set_output_data(space->machine, data & 0x00ff); break;
	case 0x28: upd4990a_control_16_w(state->upd4990a, 0, data, mem_mask); break;
//  case 0x30: break; // coin counters
//  case 0x31: break; // coin counters
//  case 0x32: break; // coin lockout
//  case 0x33: break; // coui lockout

	default:
		logerror("PC: %x  Unmapped I/O control write.  Offset: %x  Data: %x\n", cpu_get_pc(space->cpu), offset, data);
		break;
	}
}



/*************************************
 *
 *  Unmapped memory access
 *
 *************************************/

READ16_HANDLER( neogeo_unmapped_r )
{
	neogeo_state *state = (neogeo_state *)space->machine->driver_data;
	UINT16 ret;

	/* unmapped memory returns the last word on the data bus, which is almost always the opcode
       of the next instruction due to prefetch */

	/* prevent recursion */
	if (state->recurse)
		ret = 0xffff;
	else
	{
		state->recurse = 1;
		ret = memory_read_word(space, cpu_get_pc(space->cpu));
		state->recurse = 0;
	}

	return ret;
}




/*************************************
 *
 *  uPD4990A calendar chip
 *
 *************************************/
#if 0
static void calendar_init(running_machine *machine)
{
	/* set the celander IC to 'right now' */
	mame_system_time systime;
	mame_system_tm time;

	mame_get_base_datetime(machine, &systime);
	time = systime.local_time;

	pd4990a.seconds = ((time.second / 10) << 4) + (time.second % 10);
	pd4990a.minutes = ((time.minute / 10) << 4) + (time.minute % 10);
	pd4990a.hours = ((time.hour / 10) <<4 ) + (time.hour % 10);
	pd4990a.days = ((time.mday / 10) << 4) + (time.mday % 10);
	pd4990a.month = time.month + 1;
	pd4990a.year = ((((time.year - 1900) % 100) / 10) << 4) + ((time.year - 1900) % 10);
	pd4990a.weekday = time.weekday;
}


static void calendar_clock(void)
{
//  pd4990a_addretrace();
}
#endif

static CUSTOM_INPUT( get_calendar_status )
{
	neogeo_state *state = (neogeo_state *)field->port->machine->driver_data;
	return (upd4990a_databit_r(state->upd4990a, 0) << 1) | upd4990a_testbit_r(state->upd4990a, 0);
}



/*************************************
 *
 *  NVRAM (Save RAM)
 *
 *************************************/

static NVRAM_HANDLER( neogeo )
{
	if (read_or_write)
		/* save the SRAM settings */
		mame_fwrite(file, save_ram, 0x2000);
	else
	{
		/* load the SRAM settings */
		if (file)
			mame_fread(file, save_ram, 0x2000);
		else
			memset(save_ram, 0, 0x10000);
	}
}


static void set_save_ram_unlock( running_machine *machine, UINT8 data )
{
	neogeo_state *state = (neogeo_state *)machine->driver_data;
	state->save_ram_unlocked = data;
}


static WRITE16_HANDLER( save_ram_w )
{
	neogeo_state *state = (neogeo_state *)space->machine->driver_data;

	if (state->save_ram_unlocked)
		COMBINE_DATA(&save_ram[offset]);
}



/*************************************
 *
 *  Memory card
 *
 *************************************/

#define MEMCARD_SIZE	0x0800


static CUSTOM_INPUT( get_memcard_status )
{
	/* D0 and D1 are memcard presence indicators, D2 indicates memcard
       write protect status (we are always write enabled) */
	return (memcard_present(field->port->machine) == -1) ? 0x07 : 0x00;
}


static READ16_HANDLER( memcard_r )
{
	UINT16 ret;

	if (memcard_present(space->machine) != -1)
		ret = memcard_data[offset] | 0xff00;
	else
		ret = 0xffff;

	return ret;
}


static WRITE16_HANDLER( memcard_w )
{
	if (ACCESSING_BITS_0_7)
	{
		if (memcard_present(space->machine) != -1)
			memcard_data[offset] = data;
	}
}


static MEMCARD_HANDLER( neogeo )
{
	switch (action)
	{
	case MEMCARD_CREATE:
		memset(memcard_data, 0, MEMCARD_SIZE);
		mame_fwrite(file, memcard_data, MEMCARD_SIZE);
		break;

	case MEMCARD_INSERT:
		mame_fread(file, memcard_data, MEMCARD_SIZE);
		break;

	case MEMCARD_EJECT:
		mame_fwrite(file, memcard_data, MEMCARD_SIZE);
		break;
	}
}



/*************************************
 *
 *  Inter-CPU communications
 *
 *************************************/

static WRITE16_HANDLER( audio_command_w )
{
	/* accessing the LSB only is not mapped */
	if (mem_mask != 0x00ff)
	{
		soundlatch_w(space, 0, data >> 8);

		audio_cpu_assert_nmi(space->machine);

		/* boost the interleave to let the audio CPU read the command */
		cpuexec_boost_interleave(space->machine, attotime_zero, ATTOTIME_IN_USEC(50));

		if (LOG_CPU_COMM) logerror("MAIN CPU PC %06x: audio_command_w %04x - %04x\n", cpu_get_pc(space->cpu), data, mem_mask);
	}
}


static READ8_HANDLER( audio_command_r )
{
	UINT8 ret = soundlatch_r(space, 0);

	if (LOG_CPU_COMM) logerror(" AUD CPU PC   %04x: audio_command_r %02x\n", cpu_get_pc(space->cpu), ret);

	/* this is a guess */
	audio_cpu_clear_nmi_w(space, 0, 0);

	return ret;
}


static WRITE8_HANDLER( audio_result_w )
{
	neogeo_state *state = (neogeo_state *)space->machine->driver_data;

	if (LOG_CPU_COMM && (state->audio_result != data)) logerror(" AUD CPU PC   %04x: audio_result_w %02x\n", cpu_get_pc(space->cpu), data);

	state->audio_result = data;
}


static CUSTOM_INPUT( get_audio_result )
{
	neogeo_state *state = (neogeo_state *)field->port->machine->driver_data;
	UINT32 ret = state->audio_result;

//  if (LOG_CPU_COMM) logerror("MAIN CPU PC %06x: audio_result_r %02x\n", cpu_get_pc(cputag_get_cpu(field->port->machine, "maincpu")), ret);

	return ret;
}



/*************************************
 *
 *  Main CPU banking
 *
 *************************************/

static void _set_main_cpu_vector_table_source( running_machine *machine )
{
	neogeo_state *state = (neogeo_state *)machine->driver_data;
	memory_set_bank(machine, NEOGEO_BANK_VECTORS, state->main_cpu_vector_table_source);
}


static void set_main_cpu_vector_table_source( running_machine *machine, UINT8 data )
{
	neogeo_state *state = (neogeo_state *)machine->driver_data;
	state->main_cpu_vector_table_source = data;

	_set_main_cpu_vector_table_source(machine);
}


static void _set_main_cpu_bank_address( running_machine *machine )
{
	neogeo_state *state = (neogeo_state *)machine->driver_data;
	memory_set_bankptr(machine, NEOGEO_BANK_CARTRIDGE, &memory_region(machine, "maincpu")[state->main_cpu_bank_address]);
}


void neogeo_set_main_cpu_bank_address( const address_space *space, UINT32 bank_address )
{
	neogeo_state *state = (neogeo_state *)space->machine->driver_data;

	if (LOG_MAIN_CPU_BANKING) logerror("MAIN CPU PC %06x: neogeo_set_main_cpu_bank_address %06x\n", cpu_get_pc(space->cpu), bank_address);

	state->main_cpu_bank_address = bank_address;

	_set_main_cpu_bank_address(space->machine);
}


static WRITE16_HANDLER( main_cpu_bank_select_w )
{
	UINT32 bank_address;
	UINT32 len = memory_region_length(space->machine, "maincpu");

	if ((len <= 0x100000) && (data & 0x07))
		logerror("PC %06x: warning: bankswitch to %02x but no banks available\n", cpu_get_pc(space->cpu), data);
	else
	{
		bank_address = ((data & 0x07) + 1) * 0x100000;

		if (bank_address >= len)
		{
			logerror("PC %06x: warning: bankswitch to empty bank %02x\n", cpu_get_pc(space->cpu), data);
			bank_address = 0x100000;
		}

		neogeo_set_main_cpu_bank_address(space, bank_address);
	}
}


static void main_cpu_banking_init( running_machine *machine )
{
	const address_space *mainspace = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);

	/* create vector banks */
	memory_configure_bank(machine, NEOGEO_BANK_VECTORS, 0, 1, memory_region(machine, "mainbios"), 0);
	memory_configure_bank(machine, NEOGEO_BANK_VECTORS, 1, 1, memory_region(machine, "maincpu"), 0);

	/* set initial main CPU bank */
	if (memory_region_length(machine, "maincpu") > 0x100000)
		neogeo_set_main_cpu_bank_address(mainspace, 0x100000);
	else
		neogeo_set_main_cpu_bank_address(mainspace, 0x000000);
}



/*************************************
 *
 *  Audio CPU banking
 *
 *************************************/

static void set_audio_cpu_banking( running_machine *machine )
{
	neogeo_state *state = (neogeo_state *)machine->driver_data;
	int region;

	for (region = 0; region < 4; region++)
		memory_set_bank(machine, NEOGEO_BANK_AUDIO_CPU_CART_BANK + region, state->audio_cpu_banks[region]);
}


static void audio_cpu_bank_select( const address_space *space, int region, UINT8 bank )
{
	neogeo_state *state = (neogeo_state *)space->machine->driver_data;

	if (LOG_AUDIO_CPU_BANKING) logerror("Audio CPU PC %03x: audio_cpu_bank_select: Region: %d   Bank: %02x\n", cpu_get_pc(space->cpu), region, bank);

	state->audio_cpu_banks[region] = bank;

	set_audio_cpu_banking(space->machine);
}


static READ8_HANDLER( audio_cpu_bank_select_f000_f7ff_r )
{
	audio_cpu_bank_select(space, 0, offset >> 8);

	return 0;
}


static READ8_HANDLER( audio_cpu_bank_select_e000_efff_r )
{
	audio_cpu_bank_select(space, 1, offset >> 8);

	return 0;
}


static READ8_HANDLER( audio_cpu_bank_select_c000_dfff_r )
{
	audio_cpu_bank_select(space, 2, offset >> 8);

	return 0;
}


static READ8_HANDLER( audio_cpu_bank_select_8000_bfff_r )
{
	audio_cpu_bank_select(space, 3, offset >> 8);

	return 0;
}


static void _set_audio_cpu_rom_source( const address_space *space )
{
	neogeo_state *state = (neogeo_state *)space->machine->driver_data;

/*  if (!memory_region(machine, "audiobios"))   */
		state->audio_cpu_rom_source = 1;

	memory_set_bank(space->machine, NEOGEO_BANK_AUDIO_CPU_MAIN_BANK, state->audio_cpu_rom_source);

	/* reset CPU if the source changed -- this is a guess */
	if (state->audio_cpu_rom_source != state->audio_cpu_rom_source_last)
	{
		state->audio_cpu_rom_source_last = state->audio_cpu_rom_source;

		cputag_set_input_line(space->machine, "audiocpu", INPUT_LINE_RESET, PULSE_LINE);

		if (LOG_AUDIO_CPU_BANKING) logerror("Audio CPU PC %03x: selectign %s ROM\n", cpu_get_pc(space->cpu), state->audio_cpu_rom_source ? "CARTRIDGE" : "BIOS");
	}
}


static void set_audio_cpu_rom_source( const address_space *space, UINT8 data )
{
	neogeo_state *state = (neogeo_state *)space->machine->driver_data;
	state->audio_cpu_rom_source = data;

	_set_audio_cpu_rom_source(space);
}


static void audio_cpu_banking_init( running_machine *machine )
{
	neogeo_state *state = (neogeo_state *)machine->driver_data;
	int region;
	int bank;
	UINT8 *rgn;
	UINT32 address_mask;

	/* audio bios/cartridge selection */
 	if (memory_region(machine, "audiobios"))
		memory_configure_bank(machine, NEOGEO_BANK_AUDIO_CPU_MAIN_BANK, 0, 1, memory_region(machine, "audiobios"), 0);
	memory_configure_bank(machine, NEOGEO_BANK_AUDIO_CPU_MAIN_BANK, 1, 1, memory_region(machine, "audiocpu"), 0);

	/* audio banking */
	address_mask = memory_region_length(machine, "audiocpu") - 0x10000 - 1;

	rgn = memory_region(machine, "audiocpu");
	for (region = 0; region < 4; region++)
	{
		for (bank = 0; bank < 0x100; bank++)
		{
			UINT32 bank_address = 0x10000 + (((bank << (11 + region)) & 0x3ffff) & address_mask);
			memory_configure_bank(machine, NEOGEO_BANK_AUDIO_CPU_CART_BANK + region, bank, 1, &rgn[bank_address], 0);
		}
	}

	/* set initial audio banks --
       how does this really work, or is it even neccessary? */
	state->audio_cpu_banks[0] = 0x1e;
	state->audio_cpu_banks[1] = 0x0e;
	state->audio_cpu_banks[2] = 0x06;
	state->audio_cpu_banks[3] = 0x02;

	set_audio_cpu_banking(machine);

	state->audio_cpu_rom_source_last = 0;
	set_audio_cpu_rom_source(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0);
}



/*************************************
 *
 *  System control register
 *
 *************************************/

static WRITE16_HANDLER( system_control_w )
{
	if (ACCESSING_BITS_0_7)
	{
		UINT8 bit = (offset >> 3) & 0x01;

		switch (offset & 0x07)
		{
		default:
		case 0x00: neogeo_set_screen_dark(space->machine, bit); break;
		case 0x01: set_main_cpu_vector_table_source(space->machine, bit);
				   set_audio_cpu_rom_source(space, bit); /* this is a guess */
				   break;
		case 0x05: neogeo_set_fixed_layer_source(space->machine, bit); break;
		case 0x06: set_save_ram_unlock(space->machine, bit); break;
		case 0x07: neogeo_set_palette_bank(space->machine, bit); break;

		case 0x02: /* unknown - HC32 middle pin 1 */
		case 0x03: /* unknown - uPD4990 pin ? */
		case 0x04: /* unknown - HC32 middle pin 10 */
			logerror("PC: %x  Unmapped system control write.  Offset: %x  Data: %x\n", cpu_get_pc(space->cpu), offset & 0x07, bit);
			break;
		}

		if (LOG_VIDEO_SYSTEM && ((offset & 0x07) != 0x06)) logerror("PC: %x  System control write.  Offset: %x  Data: %x\n", cpu_get_pc(space->cpu), offset & 0x07, bit);
	}
}

/*************************************
 *
 *  Machine initialization
 *
 *************************************/

static STATE_POSTLOAD( aes_postload )
{
	_set_main_cpu_bank_address(machine);
	_set_main_cpu_vector_table_source(machine);
	set_audio_cpu_banking(machine);
	_set_audio_cpu_rom_source(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM));
}

static MACHINE_START( neogeo )
{
	neogeo_state *state = (neogeo_state *)machine->driver_data;

	/* set the BIOS bank */
	memory_set_bankptr(machine, NEOGEO_BANK_BIOS, memory_region(machine, "mainbios"));

	/* set the initial main CPU bank */
	main_cpu_banking_init(machine);

	/* set the initial audio CPU ROM banks */
	audio_cpu_banking_init(machine);

	create_interrupt_timers(machine);

	/* initialize the memcard data structure */
	memcard_data = auto_alloc_array_clear(machine, UINT8, MEMCARD_SIZE);

	/* start with an IRQ3 - but NOT on a reset */
	state->irq3_pending = 1;

	/* get devices */
	state->maincpu = devtag_get_device(machine, "maincpu");
	state->audiocpu = devtag_get_device(machine, "audiocpu");
	state->upd4990a = devtag_get_device(machine, "upd4990a");

	/* register state save */
	state_save_register_global(machine, state->display_position_interrupt_control);
	state_save_register_global(machine, state->display_counter);
	state_save_register_global(machine, state->vblank_interrupt_pending);
	state_save_register_global(machine, state->display_position_interrupt_pending);
	state_save_register_global(machine, state->irq3_pending);
	state_save_register_global(machine, state->audio_result);
	state_save_register_global(machine, state->controller_select);
	state_save_register_global(machine, state->main_cpu_bank_address);
	state_save_register_global(machine, state->main_cpu_vector_table_source);
	state_save_register_global_array(machine, state->audio_cpu_banks);
	state_save_register_global(machine, state->audio_cpu_rom_source);
	state_save_register_global(machine, state->audio_cpu_rom_source_last);
	state_save_register_global(machine, state->save_ram_unlocked);
	state_save_register_global_pointer(machine, memcard_data, 0x800);
	state_save_register_global(machine, state->output_data);
	state_save_register_global(machine, state->output_latch);
	state_save_register_global(machine, state->el_value);
	state_save_register_global(machine, state->led1_value);
	state_save_register_global(machine, state->led2_value);
	state_save_register_global(machine, state->recurse);

	state_save_register_postload(machine, aes_postload, NULL);
}



/*************************************
 *
 *  Machine reset
 *
 *************************************/

static MACHINE_RESET( neogeo )
{
	neogeo_state *state = (neogeo_state *)machine->driver_data;
	offs_t offs;
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);

	/* reset system control registers */
	for (offs = 0; offs < 8; offs++)
		system_control_w(space, offs, 0, 0x00ff);

	device_reset(cputag_get_cpu(machine, "maincpu"));

	neogeo_reset_rng(machine);

	start_interrupt_timers(machine);

	/* trigger the IRQ3 that was set by MACHINE_START */
	update_interrupts(machine);

	state->recurse = 0;

	/* AES apparently always uses the cartridge's fixed bank mode */
	neogeo_set_fixed_layer_source(machine,1);
}



/*************************************
 *
 *  Main CPU memory handlers
 *
 *************************************/

static ADDRESS_MAP_START( main_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x00007f) AM_ROMBANK(NEOGEO_BANK_VECTORS)
	AM_RANGE(0x000080, 0x0fffff) AM_ROM
	AM_RANGE(0x100000, 0x10ffff) AM_MIRROR(0x0f0000) AM_RAM
	/* some games have protection devices in the 0x200000 region, it appears to map to cart space, not surprising, the ROM is read here too */
	AM_RANGE(0x200000, 0x2fffff) AM_ROMBANK(NEOGEO_BANK_CARTRIDGE)
	AM_RANGE(0x2ffff0, 0x2fffff) AM_WRITE(main_cpu_bank_select_w)
	AM_RANGE(0x300000, 0x300001) AM_MIRROR(0x01ff7e) AM_READ_PORT("IN0")
	AM_RANGE(0x300080, 0x300081) AM_MIRROR(0x01ff7e) AM_READ_PORT("IN4")
	AM_RANGE(0x300000, 0x300001) AM_MIRROR(0x01ffe0) AM_READ(neogeo_unmapped_r) AM_WRITENOP	// AES has no watchdog
	AM_RANGE(0x320000, 0x320001) AM_MIRROR(0x01fffe) AM_READ_PORT("IN3") AM_WRITE(audio_command_w)
	AM_RANGE(0x340000, 0x340001) AM_MIRROR(0x01fffe) AM_READ_PORT("IN1")
	AM_RANGE(0x360000, 0x37ffff) AM_READ(neogeo_unmapped_r)
	AM_RANGE(0x380000, 0x380001) AM_MIRROR(0x01fffe) AM_READ_PORT("IN2")
	AM_RANGE(0x380000, 0x38007f) AM_MIRROR(0x01ff80) AM_WRITE(io_control_w)
	AM_RANGE(0x3a0000, 0x3a001f) AM_MIRROR(0x01ffe0) AM_READWRITE(neogeo_unmapped_r, system_control_w)
	AM_RANGE(0x3c0000, 0x3c0007) AM_MIRROR(0x01fff8) AM_READ(neogeo_video_register_r)
	AM_RANGE(0x3c0000, 0x3c000f) AM_MIRROR(0x01fff0) AM_WRITE(neogeo_video_register_w)
	AM_RANGE(0x3e0000, 0x3fffff) AM_READ(neogeo_unmapped_r)
	AM_RANGE(0x400000, 0x401fff) AM_MIRROR(0x3fe000) AM_READWRITE(neogeo_paletteram_r, neogeo_paletteram_w)
	AM_RANGE(0x800000, 0x800fff) AM_READWRITE(memcard_r, memcard_w)
	AM_RANGE(0xc00000, 0xc1ffff) AM_MIRROR(0x0e0000) AM_ROMBANK(NEOGEO_BANK_BIOS)
	AM_RANGE(0xd00000, 0xd0ffff) AM_MIRROR(0x0f0000) AM_RAM_WRITE(save_ram_w) AM_BASE(&save_ram)
	AM_RANGE(0xe00000, 0xffffff) AM_READ(neogeo_unmapped_r)
ADDRESS_MAP_END



/*************************************
 *
 *  Audio CPU memory handlers
 *
 *************************************/

static ADDRESS_MAP_START( audio_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROMBANK(NEOGEO_BANK_AUDIO_CPU_MAIN_BANK)
	AM_RANGE(0x8000, 0xbfff) AM_ROMBANK(NEOGEO_BANK_AUDIO_CPU_CART_BANK + 3)
	AM_RANGE(0xc000, 0xdfff) AM_ROMBANK(NEOGEO_BANK_AUDIO_CPU_CART_BANK + 2)
	AM_RANGE(0xe000, 0xefff) AM_ROMBANK(NEOGEO_BANK_AUDIO_CPU_CART_BANK + 1)
	AM_RANGE(0xf000, 0xf7ff) AM_ROMBANK(NEOGEO_BANK_AUDIO_CPU_CART_BANK + 0)
	AM_RANGE(0xf800, 0xffff) AM_RAM
ADDRESS_MAP_END



/*************************************
 *
 *  Audio CPU port handlers
 *
 *************************************/

static ADDRESS_MAP_START( auido_io_map, ADDRESS_SPACE_IO, 8 )
  /*AM_RANGE(0x00, 0x00) AM_MIRROR(0xff00) AM_READWRITE(audio_command_r, audio_cpu_clear_nmi_w);*/  /* may not and NMI clear */
	AM_RANGE(0x00, 0x00) AM_MIRROR(0xff00) AM_READ(audio_command_r)
	AM_RANGE(0x04, 0x07) AM_MIRROR(0xff00) AM_DEVREADWRITE("ym", ym2610_r, ym2610_w)
	AM_RANGE(0x08, 0x08) AM_MIRROR(0xff00) /* write - NMI enable / acknowledge? (the data written doesn't matter) */
	AM_RANGE(0x08, 0x08) AM_MIRROR(0xfff0) AM_MASK(0xfff0) AM_READ(audio_cpu_bank_select_f000_f7ff_r)
	AM_RANGE(0x09, 0x09) AM_MIRROR(0xfff0) AM_MASK(0xfff0) AM_READ(audio_cpu_bank_select_e000_efff_r)
	AM_RANGE(0x0a, 0x0a) AM_MIRROR(0xfff0) AM_MASK(0xfff0) AM_READ(audio_cpu_bank_select_c000_dfff_r)
	AM_RANGE(0x0b, 0x0b) AM_MIRROR(0xfff0) AM_MASK(0xfff0) AM_READ(audio_cpu_bank_select_8000_bfff_r)
	AM_RANGE(0x0c, 0x0c) AM_MIRROR(0xff00) AM_WRITE(audio_result_w)
	AM_RANGE(0x18, 0x18) AM_MIRROR(0xff00) /* write - NMI disable? (the data written doesn't matter) */
ADDRESS_MAP_END



/*************************************
 *
 *  Audio interface
 *
 *************************************/

static const ym2610_interface ym2610_config =
{
	audio_cpu_irq
};



/*************************************
 *
 *  Standard Neo-Geo DIPs and
 *  input port definition
 *
 *************************************/

#define STANDARD_DIPS																		\
	PORT_DIPNAME( 0x0001, 0x0001, "Test Switch" ) PORT_DIPLOCATION("SW:1")					\
	PORT_DIPSETTING(	  0x0001, DEF_STR( Off ) )											\
	PORT_DIPSETTING(	  0x0000, DEF_STR( On ) )											\
	PORT_DIPNAME( 0x0002, 0x0002, "Coin Chutes?" ) PORT_DIPLOCATION("SW:2")					\
	PORT_DIPSETTING(	  0x0000, "1?" )													\
	PORT_DIPSETTING(	  0x0002, "2?" )													\
	PORT_DIPNAME( 0x0004, 0x0004, "Autofire (in some games)" ) PORT_DIPLOCATION("SW:3")		\
	PORT_DIPSETTING(	  0x0004, DEF_STR( Off ) )											\
	PORT_DIPSETTING(	  0x0000, DEF_STR( On ) )											\
	PORT_DIPNAME( 0x0018, 0x0018, "COMM Setting (Cabinet No.)" ) PORT_DIPLOCATION("SW:4,5")	\
	PORT_DIPSETTING(	  0x0018, "1" )														\
	PORT_DIPSETTING(	  0x0008, "2" )														\
	PORT_DIPSETTING(	  0x0010, "3" )														\
	PORT_DIPSETTING(	  0x0000, "4" )														\
	PORT_DIPNAME( 0x0020, 0x0020, "COMM Setting (Link Enable)" ) PORT_DIPLOCATION("SW:6")	\
	PORT_DIPSETTING(	  0x0020, DEF_STR( Off ) )											\
	PORT_DIPSETTING(	  0x0000, DEF_STR( On ) )											\
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Free_Play ) ) PORT_DIPLOCATION("SW:7")			\
	PORT_DIPSETTING(	  0x0040, DEF_STR( Off ) )											\
	PORT_DIPSETTING(	  0x0000, DEF_STR( On ) )											\
	PORT_DIPNAME( 0x0080, 0x0080, "Freeze" ) PORT_DIPLOCATION("SW:8")						\
	PORT_DIPSETTING(	  0x0080, DEF_STR( Off ) )											\
	PORT_DIPSETTING(	  0x0000, DEF_STR( On ) )

#define STANDARD_IN2																				\
	PORT_START("IN2")																				\
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNUSED )													\
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )   												\
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("Next Game") PORT_CODE(KEYCODE_7)		\
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_START2 )   												\
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("Previous Game") PORT_CODE(KEYCODE_8)	\
	PORT_BIT( 0x7000, IP_ACTIVE_HIGH, IPT_SPECIAL ) PORT_CUSTOM(get_memcard_status, NULL)			\
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )


#define STANDARD_IN3																				\
	PORT_START("IN3")																				\
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )													\
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )													\
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )													\
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* having this ACTIVE_HIGH causes you to start with 2 credits using USA bios roms */	\
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* having this ACTIVE_HIGH causes you to start with 2 credits using USA bios roms */	\
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_SPECIAL ) /* what is this? */ 								\
	PORT_BIT( 0x00c0, IP_ACTIVE_HIGH, IPT_SPECIAL ) PORT_CUSTOM(get_calendar_status, NULL)			\
	PORT_BIT( 0xff00, IP_ACTIVE_HIGH, IPT_SPECIAL ) PORT_CUSTOM(get_audio_result, NULL)


#define STANDARD_IN4																			\
	PORT_START("IN4")																			\
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_UNKNOWN )												\
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_UNKNOWN )												\
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_UNKNOWN )												\
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_UNKNOWN )												\
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_UNKNOWN )												\
	PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_UNKNOWN )												\
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_SPECIAL ) /* what is this? */							\
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("Enter BIOS") PORT_CODE(KEYCODE_F2)	\
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

static INPUT_PORTS_START( controller )
	PORT_START("IN0")
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1) PORT_CATEGORY (1)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1) PORT_CATEGORY (1)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1) PORT_CATEGORY (1)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1) PORT_CATEGORY (1)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1) PORT_CATEGORY (1)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1) PORT_CATEGORY (1)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1) PORT_CATEGORY (1)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1) PORT_CATEGORY (1)


	PORT_START("IN1")
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2) PORT_CATEGORY (2)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2) PORT_CATEGORY (2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2) PORT_CATEGORY (2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2) PORT_CATEGORY (2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2) PORT_CATEGORY (2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2) PORT_CATEGORY (2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2) PORT_CATEGORY (2)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2) PORT_CATEGORY (2)
INPUT_PORTS_END

static INPUT_PORTS_START( mjpanel )
	PORT_START("MJ01_P1")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_MAHJONG_A ) PORT_PLAYER(1) PORT_CATEGORY (3)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_MAHJONG_B ) PORT_PLAYER(1) PORT_CATEGORY (3)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_MAHJONG_C ) PORT_PLAYER(1) PORT_CATEGORY (3)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_MAHJONG_D ) PORT_PLAYER(1) PORT_CATEGORY (3)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_MAHJONG_E ) PORT_PLAYER(1) PORT_CATEGORY (3)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_MAHJONG_F ) PORT_PLAYER(1) PORT_CATEGORY (3)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_MAHJONG_G ) PORT_PLAYER(1) PORT_CATEGORY (3)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0xff00, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START("MJ02_P1")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_MAHJONG_H ) PORT_PLAYER(1) PORT_CATEGORY (3)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_MAHJONG_I ) PORT_PLAYER(1) PORT_CATEGORY (3)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_MAHJONG_J ) PORT_PLAYER(1) PORT_CATEGORY (3)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_MAHJONG_K ) PORT_PLAYER(1) PORT_CATEGORY (3)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_MAHJONG_L ) PORT_PLAYER(1) PORT_CATEGORY (3)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_MAHJONG_M ) PORT_PLAYER(1) PORT_CATEGORY (3)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_MAHJONG_N ) PORT_PLAYER(1) PORT_CATEGORY (3)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0xff00, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START("MJ03_P1")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1) PORT_CATEGORY (3)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1) PORT_CATEGORY (3)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1) PORT_CATEGORY (3)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1) PORT_CATEGORY (3)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1) PORT_CATEGORY (3)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1) PORT_CATEGORY (3)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1) PORT_CATEGORY (3)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1) PORT_CATEGORY (3)
	PORT_BIT( 0xff00, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START("MJ04_P1")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_MAHJONG_PON ) PORT_PLAYER(1) PORT_CATEGORY (3)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_MAHJONG_CHI ) PORT_PLAYER(1) PORT_CATEGORY (3)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_MAHJONG_KAN ) PORT_PLAYER(1) PORT_CATEGORY (3)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_MAHJONG_RON ) PORT_PLAYER(1) PORT_CATEGORY (3)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_MAHJONG_REACH ) PORT_PLAYER(1) PORT_CATEGORY (3)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0xff00, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START("MJ01_P2")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_MAHJONG_A ) PORT_PLAYER(2) PORT_CATEGORY (4)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_MAHJONG_B ) PORT_PLAYER(2) PORT_CATEGORY (4)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_MAHJONG_C ) PORT_PLAYER(2) PORT_CATEGORY (4)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_MAHJONG_D ) PORT_PLAYER(2) PORT_CATEGORY (4)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_MAHJONG_E ) PORT_PLAYER(2) PORT_CATEGORY (4)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_MAHJONG_F ) PORT_PLAYER(2) PORT_CATEGORY (4)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_MAHJONG_G ) PORT_PLAYER(2) PORT_CATEGORY (4)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0xff00, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START("MJ02_P2")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_MAHJONG_H ) PORT_PLAYER(2) PORT_CATEGORY (4)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_MAHJONG_I ) PORT_PLAYER(2) PORT_CATEGORY (4)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_MAHJONG_J ) PORT_PLAYER(2) PORT_CATEGORY (4)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_MAHJONG_K ) PORT_PLAYER(2) PORT_CATEGORY (4)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_MAHJONG_L ) PORT_PLAYER(2) PORT_CATEGORY (4)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_MAHJONG_M ) PORT_PLAYER(2) PORT_CATEGORY (4)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_MAHJONG_N ) PORT_PLAYER(2) PORT_CATEGORY (4)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0xff00, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START("MJ03_P2")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2) PORT_CATEGORY (4)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2) PORT_CATEGORY (4)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2) PORT_CATEGORY (4)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2) PORT_CATEGORY (4)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2) PORT_CATEGORY (4)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2) PORT_CATEGORY (4)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2) PORT_CATEGORY (4)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2) PORT_CATEGORY (4)
	PORT_BIT( 0xff00, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START("MJ04_P2")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_MAHJONG_PON ) PORT_PLAYER(2) PORT_CATEGORY (4)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_MAHJONG_CHI ) PORT_PLAYER(2) PORT_CATEGORY (4)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_MAHJONG_KAN ) PORT_PLAYER(2) PORT_CATEGORY (4)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_MAHJONG_RON ) PORT_PLAYER(2) PORT_CATEGORY (4)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_MAHJONG_REACH ) PORT_PLAYER(2) PORT_CATEGORY (4)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0xff00, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END

static INPUT_PORTS_START( aes )
// was there anyting in place of dipswitch in the home console?
/*  PORT_START("IN0")
    PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNUSED )
//  PORT_DIPNAME( 0x0001, 0x0001, "Test Switch" ) PORT_DIPLOCATION("SW:1")
//  PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
//  PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
//  PORT_DIPNAME( 0x0002, 0x0002, "Coin Chutes?" ) PORT_DIPLOCATION("SW:2")
//  PORT_DIPSETTING(      0x0000, "1?" )
//  PORT_DIPSETTING(      0x0002, "2?" )
//  PORT_DIPNAME( 0x0004, 0x0000, "Mahjong Control Panel (or Auto Fire)" ) PORT_DIPLOCATION("SW:3")
//  PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
//  PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
//  PORT_DIPNAME( 0x0018, 0x0018, "COMM Setting (Cabinet No.)" ) PORT_DIPLOCATION("SW:4,5")
//  PORT_DIPSETTING(      0x0018, "1" )
//  PORT_DIPSETTING(      0x0008, "2" )
//  PORT_DIPSETTING(      0x0010, "3" )
//  PORT_DIPSETTING(      0x0000, "4" )
//  PORT_DIPNAME( 0x0020, 0x0020, "COMM Setting (Link Enable)" ) PORT_DIPLOCATION("SW:6")
//  PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
//  PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
//  PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Free_Play ) ) PORT_DIPLOCATION("SW:7")
//  PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
//  PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
//  PORT_DIPNAME( 0x0080, 0x0080, "Freeze" ) PORT_DIPLOCATION("SW:8")
//  PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
//  PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
    PORT_BIT( 0xff00, IP_ACTIVE_HIGH, IPT_SPECIAL ) PORT_CUSTOM(aes_controller_r, NULL) */

	PORT_START("CTRLSEL")  /* Select Controller Type */
	PORT_CATEGORY_CLASS( 0x0f, 0x01, "P1 Controller")
	PORT_CATEGORY_ITEM(  0x00, "Unconnected",			0 )
	PORT_CATEGORY_ITEM(  0x01, "NeoGeo Controller",		1 )
	PORT_CATEGORY_ITEM(  0x02, "NeoGeo Mahjong Panel",	3 )
	PORT_CATEGORY_CLASS( 0xf0, 0x10, "P2 Controller")
	PORT_CATEGORY_ITEM(  0x00, "Unconnected",		0 )
	PORT_CATEGORY_ITEM(  0x10, "NeoGeo Controller",		2 )
	PORT_CATEGORY_ITEM(  0x20, "NeoGeo Mahjong Panel",	4 )

	PORT_INCLUDE( controller )

	PORT_INCLUDE( mjpanel )

// were some of these present in the AES?!?
	STANDARD_IN2

	STANDARD_IN3

	STANDARD_IN4
INPUT_PORTS_END



/*************************************
 *
 *  Machine driver
 *
 *************************************/

static MACHINE_DRIVER_START( neogeo )

	/* driver data */
	MDRV_DRIVER_DATA(neogeo_state)

	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", M68000, NEOGEO_MAIN_CPU_CLOCK)
	MDRV_CPU_PROGRAM_MAP(main_map)

	MDRV_CPU_ADD("audiocpu", Z80, NEOGEO_AUDIO_CPU_CLOCK)
	MDRV_CPU_PROGRAM_MAP(audio_map)
	MDRV_CPU_IO_MAP(auido_io_map)

	MDRV_MACHINE_START(neogeo)
	MDRV_MACHINE_RESET(neogeo)
	MDRV_NVRAM_HANDLER(neogeo)
	MDRV_MEMCARD_HANDLER(neogeo)

	/* video hardware */
	MDRV_VIDEO_START(neogeo)
	MDRV_VIDEO_RESET(neogeo)
	MDRV_VIDEO_UPDATE(neogeo)
	MDRV_DEFAULT_LAYOUT(layout_neogeo)

	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_RGB32)
	MDRV_SCREEN_RAW_PARAMS(NEOGEO_PIXEL_CLOCK, NEOGEO_HTOTAL, NEOGEO_HBEND, NEOGEO_HBSTART, NEOGEO_VTOTAL, NEOGEO_VBEND, NEOGEO_VBSTART)

	/* audio hardware */
	MDRV_SPEAKER_STANDARD_STEREO("lspeaker", "rspeaker")

	MDRV_SOUND_ADD("ym", YM2610, NEOGEO_YM2610_CLOCK)
	MDRV_SOUND_CONFIG(ym2610_config)
	MDRV_SOUND_ROUTE(0, "lspeaker",  0.60)
	MDRV_SOUND_ROUTE(0, "rspeaker", 0.60)
	MDRV_SOUND_ROUTE(1, "lspeaker",  1.0)
	MDRV_SOUND_ROUTE(2, "rspeaker", 1.0)

	/* NEC uPD4990A RTC */
	MDRV_UPD4990A_ADD("upd4990a")
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( aes )
	MDRV_IMPORT_FROM(neogeo)
	MDRV_AES_CARTRIDGE_ADD("aes_multicart")
MACHINE_DRIVER_END

/*************************************
 *
 *  Driver initalization
 *
 *************************************/

static DRIVER_INIT( neogeo )
{
}


ROM_START( aes )
	ROM_REGION16_BE( 0x80000, "mainbios", 0 )
	ROM_SYSTEM_BIOS( 0, "jap-aes",   "Japan AES" )
	ROMX_LOAD("neo-po.bin",  0x00000, 0x020000, CRC(16d0c132) SHA1(4e4a440cae46f3889d20234aebd7f8d5f522e22c), ROM_GROUPWORD | ROM_REVERSE | ROM_BIOS(1))	/* AES Console (Japan) Bios */
	ROM_SYSTEM_BIOS( 1, "asia-aes",   "Asia AES" )
	ROMX_LOAD("neo-epo.bin", 0x00000, 0x020000, CRC(d27a71f1) SHA1(1b3b22092f30c4d1b2c15f04d1670eb1e9fbea07), ROM_GROUPWORD | ROM_REVERSE | ROM_BIOS(2))	/* AES Console (Asia?) Bios */

	ROM_REGION( 0x200000, "maincpu", ROMREGION_ERASEFF )

	ROM_REGION( 0x20000, "audiobios", 0 )
	ROM_LOAD( "sm1.sm1", 0x00000, 0x20000, CRC(94416d67) SHA1(42f9d7ddd6c0931fd64226a60dc73602b2819dcf) )

	ROM_REGION( 0x90000, "audiocpu", ROMREGION_ERASEFF )

	ROM_REGION( 0x20000, "zoomy", 0 )
	ROM_LOAD( "000-lo.lo", 0x00000, 0x20000, CRC(5a86cff2) SHA1(5992277debadeb64d1c1c64b0a92d9293eaf7e4a) )

	ROM_REGION( 0x20000, "fixedbios", 0 )
	ROM_LOAD( "sfix.sfix", 0x000000, 0x20000, CRC(c2ea0cfd) SHA1(fd4a618cdcdbf849374f0a50dd8efe9dbab706c3) )

	ROM_REGION( 0x20000, "fixed", ROMREGION_ERASEFF )

	ROM_REGION( 0x400000, "ym", ROMREGION_ERASEFF )

	ROM_REGION( 0x200000, "ym.deltat", ROMREGION_ERASEFF )

	ROM_REGION( 0x900000, "sprites", ROMREGION_ERASEFF )
ROM_END

ROM_START( neocd )
	ROM_REGION16_BE( 0x80000, "mainbios", 0 )
	ROM_LOAD16_WORD_SWAP( "neocd.rom",    0x00000, 0x80000, CRC(33697892) SHA1(b0f1c4fa8d4492a04431805f6537138b842b549f) )

	ROM_REGION( 0x100000, "maincpu", ROMREGION_ERASEFF )

	ROM_REGION( 0x20000, "audiobios", ROMREGION_ERASEFF )

	ROM_REGION( 0x50000, "audiocpu", ROMREGION_ERASEFF )

	ROM_REGION( 0x20000, "zoomy", 0 )
	ROM_LOAD( "000-lo.lo", 0x00000, 0x20000, CRC(5a86cff2) SHA1(5992277debadeb64d1c1c64b0a92d9293eaf7e4a) )

	ROM_REGION( 0x20000, "fixedbios", ROMREGION_ERASEFF )

	ROM_REGION( 0x20000, "fixed", ROMREGION_ERASEFF )

	ROM_REGION( 0x10000, "ym", ROMREGION_ERASEFF )

//  NO_DELTAT_REGION

	ROM_REGION( 0x100000, "sprites", ROMREGION_ERASEFF )
ROM_END

ROM_START( neocdz )
	ROM_REGION16_BE( 0x80000, "mainbios", 0 )
	ROM_LOAD16_WORD_SWAP( "neocd.bin",    0x00000, 0x80000, CRC(df9de490) SHA1(7bb26d1e5d1e930515219cb18bcde5b7b23e2eda) )

	ROM_REGION( 0x100000, "maincpu", ROMREGION_ERASEFF )

	ROM_REGION( 0x20000, "audiobios", ROMREGION_ERASEFF )

	ROM_REGION( 0x50000, "audiocpu", ROMREGION_ERASEFF )

	ROM_REGION( 0x20000, "zoomy", 0 )
	ROM_LOAD( "000-lo.lo", 0x00000, 0x20000, CRC(5a86cff2) SHA1(5992277debadeb64d1c1c64b0a92d9293eaf7e4a) )

	ROM_REGION( 0x20000, "fixedbios", ROMREGION_ERASEFF )

	ROM_REGION( 0x20000, "fixed", ROMREGION_ERASEFF )

	ROM_REGION( 0x10000, "ym", ROMREGION_ERASEFF )

//  NO_DELTAT_REGION

	ROM_REGION( 0x100000, "sprites", ROMREGION_ERASEFF )
ROM_END


CONS( 1990, aes,    0,   0,   aes,      aes,   neogeo,  "SNK", "Neo-Geo AES", 0)
CONS( 1994, neocd,  aes, 0,   neogeo,   aes,   neogeo,  "SNK", "Neo-Geo CD", GAME_NOT_WORKING )
CONS( 1996, neocdz, aes, 0,   neogeo,   aes,   neogeo,  "SNK", "Neo-Geo CDZ", GAME_NOT_WORKING )

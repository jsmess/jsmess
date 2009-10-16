#include "driver.h"
#include "image.h"
#include "includes/sms.h"
#include "video/smsvdp.h"
#include "sound/2413intf.h"
#include "machine/eeprom.h"
#include "devices/cartslot.h"

#define VERBOSE 0
#define LOG(x) do { if (VERBOSE) logerror x; } while (0)

#define CF_CODEMASTERS_MAPPER 0x01
#define CF_KOREAN_MAPPER      0x02
#define CF_93C46_EEPROM       0x04
#define CF_ONCART_RAM         0x08
#define CF_GG_SMS_MODE        0x10

#define MAX_CARTRIDGES        16

typedef struct _sms_driver_data sms_driver_data;
struct _sms_driver_data {
	UINT8 bios_page_count;
	UINT8 fm_detect;
	UINT8 ctrl_reg;
	int paused;
	UINT8 bios_port;
	UINT8 *BIOS;
	UINT8 *mapper_ram;
	UINT8 mapper[4];
	UINT8 *banking_bios[5]; /* we are going to use 1-4, same as bank numbers */
	UINT8 *banking_cart[5]; /* we are going to use 1-4, same as bank numbers */
	UINT8 *banking_none[5]; /* we are going to use 1-4, same as bank numbers */
	UINT8 gg_sio[5];
	UINT8 store_control;
	UINT8 input_port0;
	UINT8 input_port1;

	/* Model identifiers */
	UINT8 is_gamegear;
	UINT8 is_region_japan;
	UINT8 has_bios_0400;
	UINT8 has_bios_2000;
	UINT8 has_bios_full;
	UINT8 has_bios;
	UINT8 has_fm;

	/* Data needed for Rapid Fire Unit support */
	emu_timer *rapid_fire_timer;
	UINT8 rapid_fire_state_1;
	UINT8 rapid_fire_state_2;
	
	/* Data needed for Paddle Control controller */
	UINT32 last_paddle_read_time;
	UINT8 paddle_read_state;
	
	/* Data needed for Sports Pad controller */
	UINT32 last_sports_pad_time_1;
	UINT32 last_sports_pad_time_2;
	UINT8 sports_pad_state_1;
	UINT8 sports_pad_state_2;
	UINT8 sports_pad_last_data_1;
	UINT8 sports_pad_last_data_2;
	UINT8 sports_pad_1_x;
	UINT8 sports_pad_1_y;
	UINT8 sports_pad_2_x;
	UINT8 sports_pad_2_y;
	
	/* Data needed for Light Phaser */
	UINT16 lphaser_latch;
	UINT16 lphaser_1_x;
	UINT16 lphaser_1_y;
	UINT16 lphaser_2_x;
	UINT16 lphaser_2_y;
	int lphaser_x_offs;	/* Needed to 'calibrate' lphaser; set at cart loading */

	/* Cartridge slot info */
	UINT8 current_cartridge;
	struct
	{
		UINT8 *ROM;        /* Pointer to ROM image data */
		UINT32 size;       /* Size of the ROM image */
		UINT8 features;    /* on-cartridge special hardware */
		UINT8 *cartSRAM;   /* on-cartridge SRAM */
		UINT8 sram_save;   /* should be the contents of the on-cartridge SRAM be saved */
		UINT8 *cartRAM;    /* additional on-cartridge RAM (64KB for Ernie Els Golf) */
		UINT32 ram_size;   /* size of the on-cartridge RAM */
		UINT8 ram_page;    /* currently swapped in cartridge RAM */
	} cartridge[MAX_CARTRIDGES];
};

static sms_driver_data sms_state;


static void setup_rom(const address_space *space);


static TIMER_CALLBACK( rapid_fire_callback )
{
	sms_state.rapid_fire_state_1 ^= 0xff;
	sms_state.rapid_fire_state_2 ^= 0xff;
}


static WRITE8_HANDLER( sms_input_write )
{
	switch (offset)
	{
	case 0:
		switch (input_port_read_safe(space->machine, "CTRLSEL", 0x00) & 0x0f)
		{
		case 0x03:	/* Sports Pad */
			if (data != sms_state.sports_pad_last_data_1)
			{
				UINT32 cpu_cycles = cpu_get_total_cycles(space->cpu);

				sms_state.sports_pad_last_data_1 = data;
				if (cpu_cycles - sms_state.last_sports_pad_time_1 > 512)
				{
					sms_state.sports_pad_state_1 = 3;
					sms_state.sports_pad_1_x = input_port_read(space->machine, "SPORT0");
					sms_state.sports_pad_1_y = input_port_read(space->machine, "SPORT1");
				}
				sms_state.last_sports_pad_time_1 = cpu_cycles;
				sms_state.sports_pad_state_1 = (sms_state.sports_pad_state_1 + 1) & 3;
			}
			break;
		}
		break;

	case 1:
		switch (input_port_read_safe(space->machine, "CTRLSEL", 0x00) >> 4)
		{
		case 0x03:	/* Sports Pad */
			if (data != sms_state.sports_pad_last_data_2)
			{
				UINT32 cpu_cycles = cpu_get_total_cycles(space->cpu);

				sms_state.sports_pad_last_data_2 = data;
				if (cpu_cycles - sms_state.last_sports_pad_time_2 > 2048)
				{
					sms_state.sports_pad_state_2 = 3;
					sms_state.sports_pad_2_x = input_port_read(space->machine, "SPORT2");
					sms_state.sports_pad_2_y = input_port_read(space->machine, "SPORT3");
				}
				sms_state.last_sports_pad_time_2 = cpu_cycles;
				sms_state.sports_pad_state_2 = (sms_state.sports_pad_state_2 + 1) & 3;
			}
			break;
		}
		break;
	}
}

static TIMER_CALLBACK( lightgun_tick )
{
	if ((input_port_read_safe(machine, "CTRLSEL", 0x00) & 0x0f) == 0x01)
	{
			/* enable crosshair */
			crosshair_set_screen(machine, 0, CROSSHAIR_SCREEN_ALL);
	}
	else
	{
			/* disable crosshair */
			crosshair_set_screen(machine, 0, CROSSHAIR_SCREEN_NONE);
	}
	
	if ((input_port_read_safe(machine, "CTRLSEL", 0x00) & 0xf0) == 0x10)
	{
			/* enable crosshair */
			crosshair_set_screen(machine, 1, CROSSHAIR_SCREEN_ALL);
	}
	else
	{
			/* disable crosshair */
			crosshair_set_screen(machine, 1, CROSSHAIR_SCREEN_NONE);
	}
}


/* FIXME: this function is a hack. For Light Phaser X pos sms_vdp_hcount_latch
 function should be used instead, but emulation (timming?) needs to improve. */
void sms_vdp_hcount_lphaser( running_machine *machine, int hpos)
{
	UINT8 tmp = ((hpos - 46) >> 1) & 0xff;
	const device_config *smsvdp = devtag_get_device(machine, "sms_vdp");
	
	//printf ("sms_vdp_hcount_lphaser: hpos %3d => hcount %2X\n", hpos, tmp);
	sms_vdp_hcount_latch_w(smsvdp, 0, tmp);
}


static UINT8 sms_vdp_hcount( running_machine *machine )
{
	UINT8 tmp;
	UINT64 calc_cycles;
	attotime time_end;
	const device_config *screen = video_screen_first(machine->config);

	//int hpos = video_screen_get_hpos(screen);
	int vpos = video_screen_get_vpos(screen);
	int width = video_screen_get_width(screen);
	
	/* this is simpler and independent of other clocks */
	//tmp = ((hpos - 46) >> 1) & 0xff;
	
	/* ... but this is closer to the behavior described by Flubba */
	/* and makes use of his equation posted at SMSPower forum.    */
	time_end = video_screen_get_time_until_pos(screen, vpos, width - 1);
	calc_cycles = cpu_attotime_to_clocks(cputag_get_cpu(machine, "maincpu"), time_end);
	tmp = ((590 - (calc_cycles * 3)) / 4) & 0xff;
	
	//printf ("sms_vdp_hcount: hpos %3d => hcount %2X\n", hpos, tmp);
	return tmp;
}


static void sms_vdp_hcount_latch( running_machine *machine )
{
	UINT8 value = sms_vdp_hcount(machine);
	const device_config *smsvdp = devtag_get_device(machine, "sms_vdp");
	
	sms_vdp_hcount_latch_w(smsvdp, 0, value);
}

static int lphaser_sensor_is_on( running_machine *machine, int x, int y )
{
	const device_config *screen = video_screen_first(machine->config);
	int dx, dy;
	//const int r_x_r = 20 * 20; /* radius^2 */
	int hpos, vpos;
	
	if (x < LBORDER_START + LBORDER_X_PIXELS || x >= LBORDER_START + LBORDER_X_PIXELS + 256)
	{
		sms_state.lphaser_latch = 0;
		return 0;
	}
	
	hpos = video_screen_get_hpos(screen);
	vpos = video_screen_get_vpos(screen);
	
	/* some offset seems needed for sms_vdp_hcount_latch() */
	//hpos -= 44;
	
	dx = x - hpos;
	dy = y - vpos;
	
	//printf ("Light Phaser polling at %3d x %3d for shot %3d x %3d\n", hpos, vpos, x, y);
	
	/* SMSPower Wiki: the Light Phaser sensor "sees" a circular portion of the screen */
	//if ( (dx * dx) + (dy * dy) - (r_x_r) <= 0 )
	if (abs(dy) <= 5 && abs(dx) <= 60) /* simpler method used by SMSPlus-GX */
	{
		/* avoid latching hcount more than once in a line */
		if (sms_state.lphaser_latch == 0)
		{
			sms_state.lphaser_latch = 1;
			//sms_vdp_hcount_latch(screen);
			/* FIXME: sms_vdp_hcount_lphaser() is a hack necessary while normal latch gives too inconstant shot positions */
			x += sms_state.lphaser_x_offs;
			sms_vdp_hcount_lphaser(machine, x);
		}

		return 1;
	}
	else
	{
		sms_state.lphaser_latch = 0;
		return 0;
	}
}


static UINT16 screen_hpos_nonscaled( const device_config *screen, int scaled_hpos )
{
	const rectangle *visarea = video_screen_get_visible_area(screen);
	int offset_x = (scaled_hpos * (visarea->max_x - visarea->min_x)) / 255;
	return visarea->min_x + offset_x;
}


static UINT16 screen_vpos_nonscaled( const device_config *screen, int scaled_vpos )
{
	const rectangle *visarea = video_screen_get_visible_area(screen);
	int offset_y = (scaled_vpos * (visarea->max_y - visarea->min_y)) / 255;
	return visarea->min_y + offset_y;
}


static void sms_get_inputs( const address_space *space )
{
	UINT8 data = 0x00;
	UINT8 data_x = 0x00;
	UINT8 data_y = 0x00;
	UINT32 cpu_cycles = cpu_get_total_cycles(space->cpu);
	running_machine *machine = space->machine;

	sms_state.input_port0 = 0xff;
	sms_state.input_port1 = 0xff;

	if (cpu_cycles - sms_state.last_paddle_read_time > 256) 
	{
		sms_state.paddle_read_state ^= 0xff;
		sms_state.last_paddle_read_time = cpu_cycles;
	}

	/* Check if lightgun has been chosen as input: if so, enable crosshair */
	timer_set(machine, attotime_zero, NULL, 0, lightgun_tick);

	/* Player 1 */
	switch (input_port_read_safe(machine, "CTRLSEL", 0x00) & 0x0f)
	{
	case 0x00:  /* Joystick */
		data = input_port_read(machine, "PORT_DC");
		/* Check Rapid Fire setting for Button A */
		if (!(data & 0x10) && (input_port_read(machine, "RFU") & 0x01))
			data |= sms_state.rapid_fire_state_1 & 0x10;

		/* Check Rapid Fire setting for Button B */
		if (!(data & 0x20) && (input_port_read(machine, "RFU") & 0x02))
			data |= sms_state.rapid_fire_state_1 & 0x20;

		sms_state.input_port0 = (sms_state.input_port0 & 0xc0) | (data & 0x3f);
		break;

	case 0x01:  /* Light Phaser */
		data = (input_port_read(machine, "CTRLIPT") & 0x01) << 4;
		if (!(data & 0x10))
		{
			if (input_port_read(machine, "RFU") & 0x01)
				data |= sms_state.rapid_fire_state_1 & 0x10;

			data_x = input_port_read(machine, "LPHASER0");
			data_y = input_port_read(machine, "LPHASER1");

			sms_state.lphaser_1_x = screen_hpos_nonscaled(machine->primary_screen, data_x);
			sms_state.lphaser_1_y = screen_vpos_nonscaled(machine->primary_screen, data_y);
		}
		/* just consider the button (trigger) bit */
		data |= ~0x10;
		sms_state.input_port0 = (sms_state.input_port0 & 0xc0) | (data & 0x3f);
		break;

	case 0x02:  /* Paddle Control */
		/* Get button A state */
		data = input_port_read(machine, "PADDLE0");

		if (sms_state.paddle_read_state)
			data = data >> 4;

		sms_state.input_port0 = (sms_state.input_port0 & 0xc0) | (data & 0x0f) | (sms_state.paddle_read_state & 0x20)
		                | ((input_port_read(machine, "CTRLIPT") & 0x02) << 3);
		break;

	case 0x03:	/* Sega Sports Pad */
		switch (sms_state.sports_pad_state_1)
		{
		case 0:
			data = (sms_state.sports_pad_1_x >> 4) & 0x0f;
			break;
		case 1:
			data = sms_state.sports_pad_1_x & 0x0f;
			break;
		case 2:
			data = (sms_state.sports_pad_1_y >> 4) & 0x0f;
			break;
		case 3:
			data = sms_state.sports_pad_1_y & 0x0f;
			break;
		}
		sms_state.input_port0 = (sms_state.input_port0 & 0xc0) | data | ((input_port_read(machine, "CTRLIPT") & 0x0c) << 2);
		break;
	}

	/* Player 2 */
	switch (input_port_read_safe(machine, "CTRLSEL", 0x00)  & 0xf0)
	{
	case 0x00:	/* Joystick */
		data = input_port_read(machine, "PORT_DC");
		sms_state.input_port0 = (sms_state.input_port0 & 0x3f) | (data & 0xc0);

		data = input_port_read(machine, "PORT_DD");
		/* Check Rapid Fire setting for Button A */
		if (!(data & 0x04) && (input_port_read(machine, "RFU") & 0x04))
			data |= sms_state.rapid_fire_state_2 & 0x04;

		/* Check Rapid Fire setting for Button B */
		if (!(data & 0x08) && (input_port_read(machine, "RFU") & 0x08))
			data |= sms_state.rapid_fire_state_2 & 0x08;

		sms_state.input_port1 = (sms_state.input_port1 & 0xf0) | (data & 0x0f);
		break;

	case 0x10:	/* Light Phaser */
		data = (input_port_read(machine, "CTRLIPT") & 0x10) >> 2;
		if (!(data & 0x04))
		{
			if (input_port_read(machine, "RFU") & 0x04)
				data |= sms_state.rapid_fire_state_2 & 0x04;

			data_x = input_port_read(machine, "LPHASER2");
			data_y = input_port_read(machine, "LPHASER3");

			sms_state.lphaser_2_x = screen_hpos_nonscaled (machine->primary_screen, data_x);
			sms_state.lphaser_2_y = screen_vpos_nonscaled (machine->primary_screen, data_y);
		}
		/* just consider the button (trigger) bit */
		data |= ~0x04;
		sms_state.input_port1 = (sms_state.input_port1 & 0xf0) | (data & 0x0f);
		break;

	case 0x20:	/* Paddle Control */
		/* Get button A state */
		data = input_port_read(machine, "PADDLE1");
		if (sms_state.paddle_read_state)
			data = data >> 4;

		sms_state.input_port0 = (sms_state.input_port0 & 0x3f) | ((data & 0x03) << 6);
		sms_state.input_port1 = (sms_state.input_port1 & 0xf0) | ((data & 0x0c) >> 2) | (sms_state.paddle_read_state & 0x08)
		                | ((input_port_read(machine, "CTRLIPT") & 0x20) >> 3);
		break;

	case 0x30:	/* Sega Sports Pad */
		switch (sms_state.sports_pad_state_2)
		{
		case 0:
			data = sms_state.sports_pad_2_x & 0x0f;
			break;
		case 1:
			data = (sms_state.sports_pad_2_x >> 4) & 0x0f;
			break;
		case 2:
			data = sms_state.sports_pad_2_y & 0x0f;
			break;
		case 3:
			data = (sms_state.sports_pad_2_y >> 4) & 0x0f;
			break;
		}
		sms_state.input_port0 = (sms_state.input_port0 & 0x3f) | ((data & 0x03) << 6);
		sms_state.input_port1 = (sms_state.input_port1 & 0xf0) | (data >> 2) | ((input_port_read(machine, "CTRLIPT") & 0xc0) >> 4);
		break;
	}
}


WRITE8_HANDLER( sms_fm_detect_w )
{
	if (sms_state.has_fm)
		sms_state.fm_detect = (data & 0x01);
}


READ8_HANDLER( sms_fm_detect_r )
{
	if (sms_state.has_fm)
	{
		return sms_state.fm_detect;
	}
	else
	{
		if (sms_state.bios_port & IO_CHIP)
		{
			return 0xff;
		}
		else
		{
			sms_get_inputs(space);
			return sms_state.input_port0;
		}
	}
}

WRITE8_HANDLER( sms_io_control_w )
{
	if (data & 0x08)
	{
		/* check if TH pin level is high (1) and was low last time */
		if (data & 0x80 && !(sms_state.ctrl_reg & 0x80))
		{
			sms_vdp_hcount_latch(space->machine);
		}
		sms_input_write(space, 0, (data & 0x20) >> 5);
	}

	if (data & 0x02)
	{
		if (data & 0x20 && !(sms_state.ctrl_reg & 0x20))
		{
			sms_vdp_hcount_latch(space->machine);
		}
		sms_input_write(space, 1, (data & 0x80) >> 7);
	}

	sms_state.ctrl_reg = data;
}


READ8_HANDLER( sms_count_r )
{
	const device_config *smsvdp = devtag_get_device(space->machine, "sms_vdp");

	if (offset & 0x01)
		return sms_vdp_hcount_latch_r(smsvdp, offset);
	else
		return sms_vdp_vcount_r(smsvdp, offset);
}


/*
 Check if the pause button is pressed.
 If the gamegear is in sms mode, check if the start button is pressed.
 */
void sms_pause_callback( running_machine *machine )
{
	if (sms_state.is_gamegear && !(sms_state.cartridge[sms_state.current_cartridge].features & CF_GG_SMS_MODE))
		return;
	
	if (!(input_port_read(machine, sms_state.is_gamegear ? "START" : "PAUSE") & 0x80)) 
	{
		if (!sms_state.paused) 
		{
			cputag_set_input_line(machine, "maincpu", INPUT_LINE_NMI, PULSE_LINE);
		}
		sms_state.paused = 1;
	}
	else
	{
		sms_state.paused = 0;
	}
}

READ8_HANDLER( sms_input_port_0_r )
{
	if (sms_state.bios_port & IO_CHIP)
	{
		return 0xff;
	}
	else
	{
		sms_get_inputs(space);
		return sms_state.input_port0;
	}
}


READ8_HANDLER( sms_input_port_1_r )
{
	if (sms_state.bios_port & IO_CHIP)
		return 0xff;
	
	sms_get_inputs(space);
	
	/* Do region detection if TH of ports A and B are set to output (0) */
	if (!(sms_state.ctrl_reg & 0x0a))
 	{
		/* Move bits 7,5 of IO control port into bits 7, 6 */
		sms_state.input_port1 = (sms_state.input_port1 & 0x3f) | (sms_state.ctrl_reg & 0x80) | (sms_state.ctrl_reg & 0x20) << 1;
		
		/* Inverse region detect value for Japanese machines */
		if (sms_state.is_region_japan)
			sms_state.input_port1 ^= 0xc0;
	}
	else if (sms_state.ctrl_reg & 0x02 && lphaser_sensor_is_on(space->machine, sms_state.lphaser_1_x, sms_state.lphaser_1_y))
	{
		sms_state.input_port1 &= ~0x40;
	}
	else if (sms_state.ctrl_reg & 0x08 && lphaser_sensor_is_on(space->machine, sms_state.lphaser_2_x, sms_state.lphaser_2_y))
	{	
		sms_state.input_port1 &= ~0x80;
	}	
		
	return sms_state.input_port1;
}
	

	
WRITE8_HANDLER( sms_ym2413_register_port_0_w )
{
	if (sms_state.has_fm)
	{
		const device_config *ym = devtag_get_device(space->machine, "ym2413");
		ym2413_w(ym, 0, (data & 0x3f));
	}
}


WRITE8_HANDLER( sms_ym2413_data_port_0_w )
{
	if (sms_state.has_fm)
	{
		const device_config *ym = devtag_get_device(space->machine, "ym2413");
		logerror("data_port_0_w %x %x\n", offset, data);
		ym2413_w(ym, 1, data);
	}
}


READ8_HANDLER( gg_input_port_2_r )
{
	//logerror("joy 2 read, val: %02x, pc: %04x\n", ((sms_state.is_region_japan ? 0x00 : 0x40) | (input_port_read(machine, "START") & 0x80)), activecpu_get_pc());
	return ((sms_state.is_region_japan ? 0x00 : 0x40) | (input_port_read(space->machine, "START") & 0x80));
}


READ8_HANDLER( sms_mapper_r )
{
	return sms_state.mapper[offset];
}


WRITE8_HANDLER( sms_mapper_w )
{
	int page;
	UINT8 *SOURCE_BIOS;
	UINT8 *SOURCE_CART;
	UINT8 *SOURCE;
	UINT8 rom_page_count = sms_state.cartridge[sms_state.current_cartridge].size / 0x4000;

	offset &= 3;

	sms_state.mapper[offset] = data;
	sms_state.mapper_ram[offset] = data;

	if (sms_state.cartridge[sms_state.current_cartridge].ROM)
	{
		SOURCE_CART = sms_state.cartridge[sms_state.current_cartridge].ROM + (( rom_page_count > 0) ? data % rom_page_count : 0) * 0x4000;
	}
	else
	{
		SOURCE_CART = sms_state.banking_none[1];
	}

	if (sms_state.BIOS)
	{
		SOURCE_BIOS = sms_state.BIOS + ((sms_state.bios_page_count > 0) ? data % sms_state.bios_page_count : 0) * 0x4000;
	}
	else
	{
		SOURCE_BIOS = sms_state.banking_none[1];
	}

	if (sms_state.bios_port & IO_BIOS_ROM || (sms_state.is_gamegear && sms_state.BIOS == NULL))
	{
		if (!(sms_state.bios_port & IO_CARTRIDGE) || (sms_state.is_gamegear && sms_state.BIOS == NULL))
		{
			page = (rom_page_count > 0) ? data % rom_page_count : 0;
			if (!sms_state.cartridge[sms_state.current_cartridge].ROM)
				return;
			SOURCE = SOURCE_CART;
		}
		else
		{
			/* nothing to page in */
			return;
		}
	}
	else
	{
		page = (sms_state.bios_page_count > 0) ? data % sms_state.bios_page_count : 0;
		if (!sms_state.BIOS)
			return;
		SOURCE = SOURCE_BIOS;
	}

	switch (offset)
	{
	case 0: /* Control */
		/* Is it ram or rom? */
		if (data & 0x08) /* it's ram */
		{
			sms_state.cartridge[sms_state.current_cartridge].sram_save = 1;			/* SRAM should be saved on exit. */
			if (data & 0x04)
			{
				LOG(("ram 1 paged.\n"));
				SOURCE = sms_state.cartridge[sms_state.current_cartridge].cartSRAM + 0x4000;
			}
			else
			{
				LOG(("ram 0 paged.\n"));
				SOURCE = sms_state.cartridge[sms_state.current_cartridge].cartSRAM;
			}
			memory_set_bankptr(space->machine,  4, SOURCE);
			memory_set_bankptr(space->machine,  5, SOURCE + 0x2000);
		}
		else /* it's rom */
		{
			if (sms_state.bios_port & IO_BIOS_ROM || ! sms_state.has_bios)
			{
				page = (rom_page_count > 0) ? sms_state.mapper[3] % rom_page_count : 0;
				SOURCE = sms_state.banking_cart[4];
			}
			else
			{
				page = (sms_state.bios_page_count > 0) ? sms_state.mapper[3] % sms_state.bios_page_count : 0;
				SOURCE = sms_state.banking_bios[4];
			}
			LOG(("rom 2 paged in %x.\n", page));
			memory_set_bankptr(space->machine,  4, SOURCE);
			memory_set_bankptr(space->machine,  5, SOURCE + 0x2000);
		}
		break;

	case 1: /* Select 16k ROM bank for 0400-3FFF */
		LOG(("rom 0 paged in %x.\n", page));
		sms_state.banking_bios[2] = SOURCE_BIOS + 0x0400;
		sms_state.banking_cart[2] = SOURCE_CART + 0x0400;
		if (sms_state.is_gamegear)
			SOURCE = SOURCE_CART;

		memory_set_bankptr(space->machine,  2, SOURCE + 0x0400);
		break;

	case 2: /* Select 16k ROM bank for 4000-7FFF */
		LOG(("rom 1 paged in %x.\n", page));
		sms_state.banking_bios[3] = SOURCE_BIOS;
		sms_state.banking_cart[3] = SOURCE_CART;
		if (sms_state.is_gamegear)
			SOURCE = SOURCE_CART;

		memory_set_bankptr(space->machine,  3, SOURCE);
		break;

	case 3: /* Select 16k ROM bank for 8000-BFFF */
		sms_state.banking_bios[4] = SOURCE_BIOS;
		if (sms_state.is_gamegear)
			SOURCE = SOURCE_CART;

		if ( sms_state.cartridge[sms_state.current_cartridge].features & CF_CODEMASTERS_MAPPER)
		{
			if (SOURCE == SOURCE_CART)
			{
				SOURCE = sms_state.banking_cart[4];
			}
		}
		else
		{
			sms_state.banking_cart[4] = SOURCE_CART;
		}

		if (!(sms_state.mapper[0] & 0x08)) /* is RAM disabled? */
		{
			LOG(("rom 2 paged in %x.\n", page));
			memory_set_bankptr(space->machine,  4, SOURCE);
			memory_set_bankptr(space->machine,  5, SOURCE + 0x2000);
		}
		break;
	}
}


static WRITE8_HANDLER( sms_codemasters_page0_w )
{
	if (sms_state.cartridge[sms_state.current_cartridge].ROM && sms_state.cartridge[sms_state.current_cartridge].features & CF_CODEMASTERS_MAPPER)
	{
		UINT8 rom_page_count = sms_state.cartridge[sms_state.current_cartridge].size / 0x4000;
		sms_state.banking_cart[1] = sms_state.cartridge[sms_state.current_cartridge].ROM + ((rom_page_count > 0) ? data % rom_page_count : 0) * 0x4000;
		sms_state.banking_cart[2] = sms_state.banking_cart[1] + 0x0400;
		memory_set_bankptr(space->machine, 1, sms_state.banking_cart[1]);
		memory_set_bankptr(space->machine, 2, sms_state.banking_cart[2]);
	}
}


static WRITE8_HANDLER( sms_codemasters_page1_w )
{
	if (sms_state.cartridge[sms_state.current_cartridge].ROM && sms_state.cartridge[sms_state.current_cartridge].features & CF_CODEMASTERS_MAPPER)
	{
		/* Check if we need to switch in some RAM */
		if (data & 0x80)
		{
			sms_state.cartridge[sms_state.current_cartridge].ram_page = data & 0x07;
			memory_set_bankptr(space->machine, 5, sms_state.cartridge[sms_state.current_cartridge].cartRAM + sms_state.cartridge[sms_state.current_cartridge].ram_page * 0x2000);
		}
		else
		{
			UINT8 rom_page_count = sms_state.cartridge[sms_state.current_cartridge].size / 0x4000;
			sms_state.banking_cart[3] = sms_state.cartridge[sms_state.current_cartridge].ROM + ((rom_page_count > 0) ? data % rom_page_count : 0) * 0x4000;
			memory_set_bankptr(space->machine, 3, sms_state.banking_cart[3]);
			memory_set_bankptr(space->machine, 5, sms_state.banking_cart[4] + 0x2000);
		}
	}
}


WRITE8_HANDLER( sms_bios_w )
{
	sms_state.bios_port = data;

	logerror("bios write %02x, pc: %04x\n", data, cpu_get_pc(space->cpu));

	setup_rom(space);
}


WRITE8_HANDLER( sms_cartram2_w )
{
	if (sms_state.mapper[0] & 0x08)
	{
		logerror("write %02X to cartram at offset #%04X\n", data, offset + 0x2000);
		if (sms_state.mapper[0] & 0x04)
		{
			sms_state.cartridge[sms_state.current_cartridge].cartSRAM[offset + 0x6000] = data;
		}
		else
		{
			sms_state.cartridge[sms_state.current_cartridge].cartSRAM[offset + 0x2000] = data;
		}
	}
	if (sms_state.cartridge[sms_state.current_cartridge].features & CF_CODEMASTERS_MAPPER)
	{
		sms_state.cartridge[sms_state.current_cartridge].cartRAM[sms_state.cartridge[sms_state.current_cartridge].ram_page * 0x2000 + offset] = data;
	}
	if (sms_state.cartridge[sms_state.current_cartridge].features & CF_KOREAN_MAPPER && offset == 0) /* Dodgeball King mapper */
	{
		UINT8 rom_page_count = sms_state.cartridge[sms_state.current_cartridge].size / 0x4000;
		int page = (rom_page_count > 0) ? data % rom_page_count : 0;

		if (!sms_state.cartridge[sms_state.current_cartridge].ROM)
			return;

		sms_state.banking_cart[4] = sms_state.cartridge[sms_state.current_cartridge].ROM + page * 0x4000;
		memory_set_bankptr(space->machine, 4, sms_state.banking_cart[4]);
		memory_set_bankptr(space->machine, 5, sms_state.banking_cart[4] + 0x2000);
		LOG(("rom 2 paged in %x dodgeball king.\n", page));
	}
}


WRITE8_HANDLER( sms_cartram_w )
{
	int page;

	if (sms_state.mapper[0] & 0x08)
	{
		logerror("write %02X to cartram at offset #%04X\n", data, offset);
		if (sms_state.mapper[0] & 0x04)
		{
			sms_state.cartridge[sms_state.current_cartridge].cartSRAM[offset + 0x4000] = data;
		}
		else
		{
			sms_state.cartridge[sms_state.current_cartridge].cartSRAM[offset] = data;
		}
	}
	else
	{
		if (sms_state.cartridge[sms_state.current_cartridge].features & CF_CODEMASTERS_MAPPER && offset == 0) /* Codemasters mapper */
		{
			UINT8 rom_page_count = sms_state.cartridge[sms_state.current_cartridge].size / 0x4000;
			page = (rom_page_count > 0) ? data % rom_page_count : 0;
			if (!sms_state.cartridge[sms_state.current_cartridge].ROM)
				return;
			sms_state.banking_cart[4] = sms_state.cartridge[sms_state.current_cartridge].ROM + page * 0x4000;
			memory_set_bankptr(space->machine, 4, sms_state.banking_cart[4]);
			memory_set_bankptr(space->machine, 5, sms_state.banking_cart[4] + 0x2000);
			LOG(("rom 2 paged in %x codemasters.\n", page));
		}
		else if (sms_state.cartridge[sms_state.current_cartridge].features & CF_ONCART_RAM)
		{
			sms_state.cartridge[sms_state.current_cartridge].cartRAM[offset & (sms_state.cartridge[sms_state.current_cartridge].ram_size - 1)] = data;
		}
		else
		{
			logerror("INVALID write %02X to cartram at offset #%04X\n", data, offset);
		}
	}
}


WRITE8_HANDLER( gg_sio_w )
{
	logerror("*** write %02X to SIO register #%d\n", data, offset);

	sms_state.gg_sio[offset & 0x07] = data;
	switch (offset & 7)
	{
		case 0x00: /* Parallel Data */
			break;

		case 0x01: /* Data Direction/ NMI Enable */
			break;

		case 0x02: /* Serial Output */
			break;

		case 0x03: /* Serial Input */
			break;

		case 0x04: /* Serial Control / Status */
			break;
	}
}


READ8_HANDLER( gg_sio_r )
{
	logerror("*** read SIO register #%d\n", offset);

	switch (offset & 7)
	{
		case 0x00: /* Parallel Data */
			break;

		case 0x01: /* Data Direction/ NMI Enable */
			break;

		case 0x02: /* Serial Output */
			break;

		case 0x03: /* Serial Input */
			break;

		case 0x04: /* Serial Control / Status */
			break;
	}

	return sms_state.gg_sio[offset];
}


READ8_HANDLER( gg_psg_r )
{
	return 0xff;
}


WRITE8_HANDLER( gg_psg_w )
{
	logerror("write %02X to psg at offset #%d.\n",data , offset);

	/* D7 = Noise Left */
	/* D6 = Tone3 Left */
	/* D5 = Tone2 Left */
	/* D4 = Tone1 Left */

	/* D3 = Noise Right */
	/* D2 = Tone3 Right */
	/* D1 = Tone2 Right */
	/* D0 = Tone1 Right */
}


static void sms_machine_stop( running_machine *machine )
{
	/* Does the cartridge have SRAM that should be saved? */
	if (sms_state.cartridge[sms_state.current_cartridge].sram_save)
		image_battery_save(devtag_get_device(machine, "cart1"), sms_state.cartridge[sms_state.current_cartridge].cartSRAM, sizeof(UINT8) * NVRAM_SIZE );
}


static void setup_rom( const address_space *space )
{
	running_machine *machine = space->machine;

	/* 1. set up bank pointers to point to nothing */
	memory_set_bankptr(machine, 1, sms_state.banking_none[1]);
	memory_set_bankptr(machine, 2, sms_state.banking_none[2]);
	memory_set_bankptr(machine, 3, sms_state.banking_none[3]);
	memory_set_bankptr(machine, 4, sms_state.banking_none[4]);
	memory_set_bankptr(machine, 5, sms_state.banking_none[4] + 0x2000);

	/* 2. check and set up expansion port */
	if (!(sms_state.bios_port & IO_EXPANSION) && (sms_state.bios_port & IO_CARTRIDGE) && (sms_state.bios_port & IO_CARD))
	{
		/* TODO: Implement me */
		logerror("Switching to unsupported expansion port.\n");
	}

	/* 3. check and set up card rom */
	if (!(sms_state.bios_port & IO_CARD) && (sms_state.bios_port & IO_CARTRIDGE) && (sms_state.bios_port & IO_EXPANSION))
	{
		/* TODO: Implement me */
		logerror("Switching to unsupported card rom port.\n");
	}

	/* 4. check and set up cartridge rom */
	/* if ((!(bios_port & IO_CARTRIDGE) && (bios_port & IO_EXPANSION) && (bios_port & IO_CARD)) || sms_state.is_gamegear) { */
	/* Out Run Europa initially writes a value to port 3E where IO_CARTRIDGE, IO_EXPANSION and IO_CARD are reset */
	if ((!(sms_state.bios_port & IO_CARTRIDGE)) || sms_state.is_gamegear)
	{
		memory_set_bankptr(machine, 1, sms_state.banking_cart[1]);
		memory_set_bankptr(machine, 2, sms_state.banking_cart[2]);
		memory_set_bankptr(machine, 3, sms_state.banking_cart[3]);
		memory_set_bankptr(machine, 4, sms_state.banking_cart[4]);
		memory_set_bankptr(machine, 5, sms_state.banking_cart[4] + 0x2000);
		logerror("Switched in cartridge rom.\n");
	}

	/* 5. check and set up bios rom */
	if (!(sms_state.bios_port & IO_BIOS_ROM))
	{
		/* 0x0400 bioses */
		if (sms_state.has_bios_0400)
		{
			memory_set_bankptr(machine, 1, sms_state.banking_bios[1]);
			logerror("Switched in 0x0400 bios.\n");
		}
		/* 0x2000 bioses */
		if (sms_state.has_bios_2000)
		{
			memory_set_bankptr(machine, 1, sms_state.banking_bios[1]);
			memory_set_bankptr(machine, 2, sms_state.banking_bios[2]);
			logerror("Switched in 0x2000 bios.\n");
		}
		if (sms_state.has_bios_full)
		{
			memory_set_bankptr(machine, 1, sms_state.banking_bios[1]);
			memory_set_bankptr(machine, 2, sms_state.banking_bios[2]);
			memory_set_bankptr(machine, 3, sms_state.banking_bios[3]);
			memory_set_bankptr(machine, 4, sms_state.banking_bios[4]);
			memory_set_bankptr(machine, 5, sms_state.banking_bios[4] + 0x2000);
			logerror("Switched in full bios.\n");
		}
	}

	if (sms_state.cartridge[sms_state.current_cartridge].features & CF_ONCART_RAM)
	{
		memory_set_bankptr(machine, 4, sms_state.cartridge[sms_state.current_cartridge].cartRAM);
		memory_set_bankptr(machine, 5, sms_state.cartridge[sms_state.current_cartridge].cartRAM);
	}
}


static int sms_verify_cart( UINT8 *magic, int size )
{
	int retval;

	retval = IMAGE_VERIFY_FAIL;

	/* Verify the file is a valid image - check $7ff0 for "TMR SEGA" */
	if (size >= 0x8000)
	{
		if (!strncmp((char*)&magic[0x7ff0], "TMR SEGA", 8))
		{
			/* Technically, it should be this, but remove for now until verified:
            if (!strcmp(sysname, "gamegear"))
            {
                if ((unsigned char)magic[0x7ffd] < 0x50)
                    retval = IMAGE_VERIFY_PASS;
            }
            if (!strcmp(sysname, "sms"))
            {
                if ((unsigned char)magic[0x7ffd] >= 0x50)
                    retval = IMAGE_VERIFY_PASS;
            }
            */
			retval = IMAGE_VERIFY_PASS;
		}
	}

		/* Check at $81f0 also */
		//if (!retval) {
	 //  if (!strncmp(&magic[0x81f0], "TMR SEGA", 8)) {
				/* Technically, it should be this, but remove for now until verified:
                if (!strcmp(sysname, "gamegear")) 
			{
                    if ((unsigned char)magic[0x81fd] < 0x50)
                        retval = IMAGE_VERIFY_PASS;
                }
                if (!strcmp(sysname, "sms")) 
			{
                    if ((unsigned char)magic[0x81fd] >= 0x50)
                        retval = IMAGE_VERIFY_PASS;
                }
                */
		 //  retval = IMAGE_VERIFY_PASS;
		//  }
		//}

	return retval;
}

/* Check for Codemasters mapper
  0x7FE3 - 93 - sms Cosmis Spacehead
              - sms Dinobasher
              - sms The Excellent Dizzy Collection
              - sms Fantastic Dizzy
              - sms Micro Machines
              - gamegear Cosmic Spacehead
              - gamegear Micro Machines
         - 94 - gamegear Dropzone
              - gamegear Ernie Els Golf (also has 64KB additional RAM on the cartridge)
              - gamegear Pete Sampras Tennis
              - gamegear S.S. Lucifer
         - 95 - gamegear Micro Machines 2 - Turbo Tournament

The Korean game Jang Pung II also seems to use a codemasters style mapper.
 */
static int detect_codemasters_mapper( UINT8 *rom )
{
	static const UINT8 jang_pung2[16] = { 0x00, 0xba, 0x38, 0x0d, 0x00, 0xb8, 0x38, 0x0c, 0x00, 0xb6, 0x38, 0x0b, 0x00, 0xb4, 0x38, 0x0a };

	if (((rom[0x7fe0] & 0x0f ) <= 9) && (rom[0x7fe3] == 0x93 || rom[0x7fe3] == 0x94 || rom[0x7fe3] == 0x95) &&  rom[0x7fef] == 0x00)
		return 1;

	if (!memcmp(&rom[0x7ff0], jang_pung2, 16))
		return 1;

	return 0;
}


static int detect_korean_mapper( UINT8 *rom )
{
	static const UINT8 signatures[2][16] =
	{
		{ 0x3e, 0x11, 0x32, 0x00, 0xa0, 0x78, 0xcd, 0x84, 0x85, 0x3e, 0x02, 0x32, 0x00, 0xa0, 0xc9, 0xff }, /* Dodgeball King */
		{ 0x41, 0x48, 0x37, 0x37, 0x44, 0x37, 0x4e, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x20 },	/* Sangokushi 3 */
	};
	int i;

	for (i = 0; i < 2; i++)
	{
		if (!memcmp(&rom[0x7ff0], signatures[i], 16))
		{
			return 1;
		}
	}
	return 0;
}

static int detect_lphaser_xoffset( UINT8 *rom )
{
	static const UINT8 signatures[6][16] =
	{
		/* Spacegun */
		{ 0x54, 0x4d, 0x52, 0x20, 0x53, 0x45, 0x47, 0x41, 0xff, 0xff, 0x9d, 0x99, 0x10, 0x90, 0x00, 0x40 },
		/* Gangster Town */
		{ 0x54, 0x4d, 0x52, 0x20, 0x53, 0x45, 0x47, 0x41, 0x19, 0x87, 0x1b, 0xc9, 0x74, 0x50, 0x00, 0x4f },
		/* Shooting Gallery */
		{ 0x54, 0x4d, 0x52, 0x20, 0x53, 0x45, 0x47, 0x41, 0x20, 0x20, 0x8a, 0x3a, 0x72, 0x50, 0x00, 0x4f },
		/* Rescue Mission */
		{ 0x54, 0x4d, 0x52, 0x20, 0x53, 0x45, 0x47, 0x41, 0x20, 0x20, 0xfb, 0xd3, 0x06, 0x51, 0x00, 0x4f },
		/* Laser Ghost */
		{ 0x54, 0x4d, 0x52, 0x20, 0x53, 0x45, 0x47, 0x41, 0x00, 0x00, 0xb7, 0x55, 0x74, 0x70, 0x00, 0x40 },
		/* Assault City */
		{ 0x54, 0x4d, 0x52, 0x20, 0x53, 0x45, 0x47, 0x41, 0xff, 0xff, 0x9f, 0x74, 0x34, 0x70, 0x00, 0x40 },
	};
	
	if (!(sms_state.bios_port & IO_CARTRIDGE) && sms_state.cartridge[sms_state.current_cartridge].size >= 0x8000)
	{
		if (!memcmp(&rom[0x7ff0], signatures[0], 16) || !memcmp(&rom[0x7ff0], signatures[1], 16))
			return 40;
		
		if (!memcmp(&rom[0x7ff0], signatures[2], 16))
			return 49;
		
		if (!memcmp(&rom[0x7ff0], signatures[3], 16))
			return 47;
		
		if (!memcmp(&rom[0x7ff0], signatures[4], 16))
			return 44;
		
		if (!memcmp(&rom[0x7ff0], signatures[5], 16))
			return 53;
		
	}
	return 50;	
}


DEVICE_START( sms_cart )
{
	int i;

	for (i = 0; i < MAX_CARTRIDGES; i++)
	{
		sms_state.cartridge[i].ROM = NULL;
		sms_state.cartridge[i].size = 0;
		sms_state.cartridge[i].features = 0;
		sms_state.cartridge[i].cartSRAM = NULL;
		sms_state.cartridge[i].sram_save = 0;
		sms_state.cartridge[i].cartRAM = NULL;
		sms_state.cartridge[i].ram_size = 0;
		sms_state.cartridge[i].ram_page = 0;
	}
	sms_state.current_cartridge = 0;

	sms_state.bios_port = (IO_EXPANSION | IO_CARTRIDGE | IO_CARD);
	if (!sms_state.is_gamegear && ! sms_state.has_bios)
	{
		sms_state.bios_port &= ~(IO_CARTRIDGE);
		sms_state.bios_port |= IO_BIOS_ROM;
	}
}


DEVICE_IMAGE_LOAD( sms_cart )
{
	int size = image_length(image);
	int index = 0;
	const char *fname = image_filename(image);
	int fname_len = fname ? strlen(fname) : 0;
	const char *extrainfo = image_extrainfo(image);

	if (strcmp(image->tag,"cart1") == 0)
		index = 0;

	if (strcmp(image->tag,"cart2") == 0)
		index = 1;
	if (strcmp(image->tag,"cart3") == 0)
		index = 2;
	if (strcmp(image->tag,"cart4") == 0)
		index = 3;
	if (strcmp(image->tag,"cart5") == 0)
		index = 4;
	if (strcmp(image->tag,"cart6") == 0)
		index = 5;
	if (strcmp(image->tag,"cart7") == 0)
		index = 6;
	if (strcmp(image->tag,"cart8") == 0)
		index = 7;
	if (strcmp(image->tag,"cart9") == 0)
		index = 8;
	if (strcmp(image->tag,"cart10") == 0)
		index = 9;
	if (strcmp(image->tag,"cart11") == 0)
		index = 10;
	if (strcmp(image->tag,"cart12") == 0)
		index = 11;
	if (strcmp(image->tag,"cart13") == 0)
		index = 12;
	if (strcmp(image->tag,"cart14") == 0)
		index = 13;
	if (strcmp(image->tag,"cart15") == 0)
		index = 14;
	if (strcmp(image->tag,"cart16") == 0)
		index = 15;

	/* Check for 512-byte header */
	if ((size / 512) & 1)
	{
		image_fseek(image, 512, SEEK_SET);
		size -= 512;
	}

	if (!size)
	{
		image_seterror(image, IMAGE_ERROR_UNSPECIFIED, "Invalid ROM image: ROM image is too small");
		return INIT_FAIL;
	}

	/* Create a new memory region to hold the ROM. */
	/* Make sure the region holds only complete (0x4000) rom banks */
	sms_state.cartridge[index].size = (size & 0x3fff) ? (((size >> 14) + 1) << 14) : size;
	sms_state.cartridge[index].ROM = auto_alloc_array(image->machine, UINT8, sms_state.cartridge[index].size);

	sms_state.cartridge[index].cartSRAM = auto_alloc_array(image->machine, UINT8, NVRAM_SIZE);

	/* Load ROM banks */
	size = image_fread(image, sms_state.cartridge[index].ROM, size);

	/* check the image */
	if (!sms_state.has_bios)
	{
		if (sms_verify_cart(sms_state.cartridge[index].ROM, size) == IMAGE_VERIFY_FAIL)
		{
			logerror("Warning loading image: sms_verify_cart failed\n");
		}
	}

	sms_state.cartridge[index].features = 0;

	/* Detect special features from the extrainfo field */
	if (extrainfo)
	{
		/* Check for codemasters mapper */
		if (strstr(extrainfo, "CODEMASTERS"))
			sms_state.cartridge[index].features |= CF_CODEMASTERS_MAPPER;

		/* Check for korean mapper */
		if (strstr(extrainfo, "KOREAN"))
			sms_state.cartridge[index].features |= CF_KOREAN_MAPPER;

		/* Check for special SMS Compatibility mode gamegear cartridges */
		if (sms_state.is_gamegear && strstr(extrainfo, "GGSMS"))
			sms_state.cartridge[index].features |= CF_GG_SMS_MODE;

		/* Check for 93C46 eeprom */
		if (strstr( extrainfo, "93C46"))
			sms_state.cartridge[index].features |= CF_93C46_EEPROM;

		/* Check for 8KB on-cart RAM */
		if (strstr(extrainfo, "8KB_CART_RAM"))
		 {
			sms_state.cartridge[index].features |= CF_ONCART_RAM;
			sms_state.cartridge[index].ram_size = 0x2000;
			sms_state.cartridge[index].cartRAM = auto_alloc_array(image->machine, UINT8, sms_state.cartridge[index].ram_size);
		}
	}
	else
	{
		/* If no extrainfo information is available try to find special information out on our own */
		/* Check for special cartridge features */
		if (size >= 0x8000)
		{
			/* Check for special mappers */
			if (detect_codemasters_mapper(sms_state.cartridge[index].ROM))
				sms_state.cartridge[index].features |= CF_CODEMASTERS_MAPPER;

			if (detect_korean_mapper(sms_state.cartridge[index].ROM))
				sms_state.cartridge[index].features |= CF_KOREAN_MAPPER;

		}

		/* Check for special SMS Compatibility mode gamegear cartridges */
		if (sms_state.is_gamegear)
		{
			/* Just in case someone passes us an sms file */
			if (fname_len > 3 && ! mame_stricmp(fname + fname_len - 3, "sms"))
				sms_state.cartridge[index].features |= CF_GG_SMS_MODE;
		}
	}

	if (sms_state.cartridge[index].features & CF_CODEMASTERS_MAPPER)
	{
		sms_state.cartridge[index].ram_size = 0x10000;
		sms_state.cartridge[index].cartRAM = auto_alloc_array(image->machine, UINT8, sms_state.cartridge[index].ram_size);
		sms_state.cartridge[index].ram_page = 0;
	}
	
	/* For Light Phaser games, we have to detect the x offset */
	sms_state.lphaser_x_offs = detect_lphaser_xoffset(sms_state.cartridge[index].ROM);

	/* Load battery backed RAM, if available */
	image_battery_load(image, sms_state.cartridge[index].cartSRAM, sizeof(UINT8) * NVRAM_SIZE);

	return INIT_PASS;
}


static void setup_cart_banks( void )
{
	if (sms_state.cartridge[sms_state.current_cartridge].ROM)
	{
		UINT8 rom_page_count = sms_state.cartridge[sms_state.current_cartridge].size / 0x4000;
		sms_state.banking_cart[1] = sms_state.cartridge[sms_state.current_cartridge].ROM;
		sms_state.banking_cart[2] = sms_state.cartridge[sms_state.current_cartridge].ROM + 0x0400;
		sms_state.banking_cart[3] = sms_state.cartridge[sms_state.current_cartridge].ROM + ((1 < rom_page_count) ? 0x4000 : 0);
		sms_state.banking_cart[4] = sms_state.cartridge[sms_state.current_cartridge].ROM + ((2 < rom_page_count) ? 0x8000 : 0);
		/* Codemasters mapper points to bank 0 for page 2 */
		if (sms_state.cartridge[sms_state.current_cartridge].features & CF_CODEMASTERS_MAPPER)
		{
			sms_state.banking_cart[4] = sms_state.cartridge[sms_state.current_cartridge].ROM;
		}
	}
	else
	{
		sms_state.banking_cart[1] = sms_state.banking_none[1];
		sms_state.banking_cart[2] = sms_state.banking_none[2];
		sms_state.banking_cart[3] = sms_state.banking_none[3];
		sms_state.banking_cart[4] = sms_state.banking_none[4];
	}
}


static void setup_banks( running_machine *machine )
{
	UINT8 *mem = memory_region(machine, "maincpu");
	sms_state.banking_bios[1] = sms_state.banking_cart[1] = sms_state.banking_none[1] = mem;
	sms_state.banking_bios[2] = sms_state.banking_cart[2] = sms_state.banking_none[2] = mem;
	sms_state.banking_bios[3] = sms_state.banking_cart[3] = sms_state.banking_none[3] = mem;
	sms_state.banking_bios[4] = sms_state.banking_cart[4] = sms_state.banking_none[4] = mem;

	sms_state.BIOS = memory_region(machine, "user1");

	sms_state.bios_page_count = (sms_state.BIOS ? memory_region_length(machine, "user1") / 0x4000 : 0);

	setup_cart_banks();

	if (sms_state.BIOS == NULL || sms_state.BIOS[0] == 0x00)
	{
		sms_state.BIOS = NULL;
		sms_state.bios_port |= IO_BIOS_ROM;
	}

	if (sms_state.BIOS)
	{
		sms_state.banking_bios[1] = sms_state.BIOS;
		sms_state.banking_bios[2] = sms_state.BIOS + 0x0400;
		sms_state.banking_bios[3] = sms_state.BIOS + ((1 < sms_state.bios_page_count) ? 0x4000 : 0);
		sms_state.banking_bios[4] = sms_state.BIOS + ((2 < sms_state.bios_page_count) ? 0x8000 : 0);
	}
}


MACHINE_START( sms )
{
	add_exit_callback(machine, sms_machine_stop);
}


MACHINE_RESET( sms )
{
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	const device_config *smsvdp = devtag_get_device(machine, "sms_vdp");

	sms_state.ctrl_reg = 0xff;
	if (sms_state.has_fm)
		sms_state.fm_detect = 0x01;

	sms_state.mapper_ram = memory_get_write_ptr(space, 0xdffc);

	sms_state.bios_port = 0;

	if (sms_state.cartridge[sms_state.current_cartridge].features & CF_CODEMASTERS_MAPPER)
	{
		/* Install special memory handlers */
		memory_install_write8_handler(space, 0x0000, 0x0000, 0, 0, sms_codemasters_page0_w);
		memory_install_write8_handler(space, 0x4000, 0x4000, 0, 0, sms_codemasters_page1_w);
	}

	if (sms_state.cartridge[sms_state.current_cartridge].features & CF_GG_SMS_MODE)
		sms_set_ggsmsmode(smsvdp, 1);

	/* Initialize SIO stuff for GG */
	sms_state.gg_sio[0] = 0x7f;
	sms_state.gg_sio[1] = 0xff;
	sms_state.gg_sio[2] = 0x00;
	sms_state.gg_sio[3] = 0xff;
	sms_state.gg_sio[4] = 0x00;

	sms_state.store_control = 0;

	setup_banks(machine);

	setup_rom(space);

	sms_state.rapid_fire_state_1 = 0;
	sms_state.rapid_fire_state_2 = 0;
	sms_state.rapid_fire_timer = timer_alloc(machine, rapid_fire_callback , NULL);
	timer_adjust_periodic(sms_state.rapid_fire_timer, ATTOTIME_IN_HZ(10), 0, ATTOTIME_IN_HZ(10));

	sms_state.last_paddle_read_time = 0;
	sms_state.paddle_read_state = 0;

	sms_state.last_sports_pad_time_1 = 0;
	sms_state.last_sports_pad_time_2 = 0;
	sms_state.sports_pad_state_1 = 0;
	sms_state.sports_pad_state_2 = 0;
	sms_state.sports_pad_last_data_1 = 0;
	sms_state.sports_pad_last_data_2 = 0;
	sms_state.sports_pad_1_x = 0;
	sms_state.sports_pad_1_y = 0;
	sms_state.sports_pad_2_x = 0;
	sms_state.sports_pad_2_y = 0;

	sms_state.lphaser_latch = 0;
	sms_state.lphaser_1_x = 0;
	sms_state.lphaser_1_y = 0;
	sms_state.lphaser_2_x = 0;
	sms_state.lphaser_2_x = 0;

	/* Check if lightgun has been chosen as input: if so, enable crosshair */
	timer_set(space->machine, attotime_zero, NULL, 0, lightgun_tick);
}


READ8_HANDLER( sms_store_cart_select_r )
{
	return 0xff;
}


WRITE8_HANDLER( sms_store_cart_select_w )
{
	UINT8 slot = data >> 4;
	UINT8 slottype = data & 0x08;

	logerror("switching in part of %s slot #%d\n", slottype ? "card" : "cartridge", slot );
	/* cartridge? slot #0 */
	if (slottype == 0)
		sms_state.current_cartridge = slot;

	setup_cart_banks();
	memory_set_bankptr(space->machine, 10, sms_state.banking_cart[3] + 0x2000);
	setup_rom(space);
}


READ8_HANDLER( sms_store_select1 )
{
	return 0xff;
}


READ8_HANDLER( sms_store_select2 )
{
	return 0xff;
}


READ8_HANDLER( sms_store_control_r )
{
	return sms_state.store_control;
}


WRITE8_HANDLER( sms_store_control_w )
{
	logerror("0x%04X: sms_store_control write 0x%02X\n", cpu_get_pc(space->cpu), data);
	if (data & 0x02)
	{
		cputag_resume(space->machine, "maincpu", SUSPEND_REASON_HALT);
	}
	else
	{
		/* Pull reset line of CPU #0 low */
		cputag_suspend(space->machine, "maincpu", SUSPEND_REASON_HALT, 1);
		device_reset(cputag_get_cpu(space->machine, "maincpu"));
	}
	sms_state.store_control = data;
}

void sms_store_int_callback( running_machine *machine, int state )
{
	cputag_set_input_line(machine, sms_state.store_control & 0x01 ? "control" : "maincpu", 0, state);
}


static void sms_set_zero_flag( void )
{
	sms_state.is_gamegear = 0;
	sms_state.is_region_japan = 0;
	sms_state.has_bios_0400 = 0;
	sms_state.has_bios_2000 = 0;
	sms_state.has_bios_full = 0;
	sms_state.has_bios = 0;
	sms_state.has_fm = 0;	
}

DRIVER_INIT( sg1000m3 )
{
	sms_set_zero_flag();
	sms_state.is_region_japan = 1;
	sms_state.has_fm = 1;
}


DRIVER_INIT( sms1 )
{
	sms_set_zero_flag();
	sms_state.has_bios_full = 1;
}


DRIVER_INIT( smsj )
{
	sms_set_zero_flag();
	sms_state.is_region_japan = 1;
	sms_state.has_bios_2000 = 1;
	sms_state.has_fm = 1;
}


DRIVER_INIT( sms2kr )
{
	sms_set_zero_flag();
	sms_state.is_region_japan = 1;
	sms_state.has_bios_full = 1;
	sms_state.has_fm = 1;
}


DRIVER_INIT( smssdisp )
{
	sms_set_zero_flag();
}


DRIVER_INIT( gamegear )
{
	sms_set_zero_flag();
	sms_state.is_gamegear = 1;
}


DRIVER_INIT( gamegeaj )
{
	sms_set_zero_flag();
	sms_state.is_region_japan = 1;
	sms_state.is_gamegear = 1;
	sms_state.has_bios_0400 = 1;
}

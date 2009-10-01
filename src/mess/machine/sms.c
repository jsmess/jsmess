#include "driver.h"
#include "image.h"
#include "includes/sms.h"
#include "video/smsvdp.h"
#include "sound/2413intf.h"
#include "machine/eeprom.h"
#include "devices/cartslot.h"

#define VERBOSE 0
#define LOG(x) do { if (VERBOSE) logerror x; } while (0)

#define FLAG_GAMEGEAR		0x00020000
#define FLAG_BIOS_0400		0x00040000
#define FLAG_BIOS_2000		0x00080000
#define FLAG_BIOS_FULL		0x00100000
#define FLAG_FM				0x00200000
#define FLAG_REGION_JAPAN	0x00400000


#define IS_GAMEGEAR			( sms_flags & FLAG_GAMEGEAR )
#define HAS_BIOS_0400		( sms_flags & FLAG_BIOS_0400 )
#define HAS_BIOS_2000		( sms_flags & FLAG_BIOS_2000 )
#define HAS_BIOS_FULL		( sms_flags & FLAG_BIOS_FULL )
#define HAS_BIOS			( sms_flags & ( FLAG_BIOS_0400 | FLAG_BIOS_2000 | FLAG_BIOS_FULL ) )
#define HAS_FM				( sms_flags & FLAG_FM )
#define IS_REGION_JAPAN		( sms_flags & FLAG_REGION_JAPAN )


#define CF_CODEMASTERS_MAPPER	0x01
#define CF_KOREAN_MAPPER	0x02
#define CF_93C46_EEPROM		0x04
#define CF_ONCART_RAM		0x08
#define CF_GG_SMS_MODE		0x10

#define MAX_CARTRIDGES		16

static UINT8 smsBiosPageCount;
static UINT8 smsFMDetect;
static UINT8 smsVersion;
static int smsPaused;

static UINT8 biosPort;
static UINT32 sms_flags;
static UINT8 *BIOS;

static UINT8 *sms_mapper_ram;
static UINT8 sms_mapper[4];
static UINT8 *sms_banking_bios[5]; /* we are going to use 1-4, same as bank numbers */
static UINT8 *sms_banking_cart[5]; /* we are going to use 1-4, same as bank numbers */
static UINT8 *sms_banking_none[5]; /* we are going to use 1-4, same as bank numbers */
static UINT8 ggSIO[5];
static UINT8 sms_store_control = 0;

static UINT8 sms_input_port0;
static UINT8 sms_input_port1;

/* Data needed for Rapid Fire Unit support */
static emu_timer	*rapid_fire_timer;
static UINT8 rapid_fire_state_1;
static UINT8 rapid_fire_state_2;

/* Data needed for Paddle Control controller */
static UINT32 last_paddle_read_time;
static UINT8 paddle_read_state;

/* Data needed for Sports Pad controller */
static UINT32 last_sports_pad_time_1;
static UINT32 last_sports_pad_time_2;
static UINT8 sports_pad_state_1;
static UINT8 sports_pad_state_2;
static UINT8 sports_pad_last_data_1;
static UINT8 sports_pad_last_data_2;
static UINT8 sports_pad_1_x;
static UINT8 sports_pad_1_y;
static UINT8 sports_pad_2_x;
static UINT8 sports_pad_2_y;

static struct
{
	UINT8	*ROM;			/* Pointer to ROM image data */
	UINT32	size;			/* Size of the ROM image */
	UINT8	features;		/* on-cartridge special hardware */
	UINT8	*cartSRAM;		/* on-cartridge SRAM */
	UINT8	sram_save;		/* should be the contents of the on-cartridge SRAM be saved */
	UINT8	*cartRAM;		/* additional on-cartridge RAM (64KB for Ernie Els Golf) */
	UINT32	ram_size;		/* size of the on-cartridge RAM */
	UINT8	ram_page;		/* currently swapped in cartridge RAM */
} sms_cartridge[MAX_CARTRIDGES];
static UINT8	sms_current_cartridge;

static void setup_rom(const address_space *space);


static TIMER_CALLBACK( rapid_fire_callback )
{
	rapid_fire_state_1 ^= 0xFF;
	rapid_fire_state_2 ^= 0xFF;
}


static WRITE8_HANDLER( sms_input_write )
{
	switch( offset )
	{
	case 0:
		switch( input_port_read_safe(space->machine, "CTRLSEL", 0x00) & 0x0F )
		{
		case 0x03:	/* Sports Pad */
			if ( data != sports_pad_last_data_1 )
			{
				UINT32 cpu_cycles = cpu_get_total_cycles(space->cpu);

				sports_pad_last_data_1 = data;
				if ( cpu_cycles - last_sports_pad_time_1 > 512 )
				{
					sports_pad_state_1 = 3;
					sports_pad_1_x = input_port_read(space->machine, "SPORT0");
					sports_pad_1_y = input_port_read(space->machine, "SPORT1");
				}
				last_sports_pad_time_1 = cpu_cycles;
				sports_pad_state_1 = ( sports_pad_state_1 + 1 ) & 3;
			}
			break;
		}
		break;

	case 1:
		switch( input_port_read_safe(space->machine, "CTRLSEL", 0x00) >> 4 )
		{
		case 0x03:	/* Sports Pad */
			if ( data != sports_pad_last_data_2 )
			{
				UINT32 cpu_cycles = cpu_get_total_cycles(space->cpu);

				sports_pad_last_data_2 = data;
				if ( cpu_cycles - last_sports_pad_time_2 > 2048 )
				{
					sports_pad_state_2 = 3;
					sports_pad_2_x = input_port_read(space->machine, "SPORT2");
					sports_pad_2_y = input_port_read(space->machine, "SPORT3");
				}
				last_sports_pad_time_2 = cpu_cycles;
				sports_pad_state_2 = ( sports_pad_state_2 + 1 ) & 3;
			}
			break;
		}
		break;
	}
}

static void sms_get_inputs(const address_space *space)
{
	UINT8 data = 0x00;
	UINT32 cpu_cycles = cpu_get_total_cycles(space->cpu);
	running_machine *machine = space->machine;

	sms_input_port0 = 0xFF;
	sms_input_port1 = 0xFF;

	if ( cpu_cycles - last_paddle_read_time > 256 ) {
		paddle_read_state ^= 0xFF;
		last_paddle_read_time = cpu_cycles;
	}

	/* Player 1 */
	switch( input_port_read_safe(machine, "CTRLSEL", 0x00) & 0x0F )
	{
	case 0x00:  /* Joystick */
		data = input_port_read(machine, "PORT_DC");
		/* Rapid Fire setting for Button A */
		if ( input_port_read(machine, "RFU") & 0x01 )
		{
			data = ( data & 0xEF ) | ( rapid_fire_state_1 & 0x10 );
		}
		/* Check Rapid Fire setting for Button B */
		if ( input_port_read(machine, "RFU") & 0x02 )
		{
			data = ( data & 0xDF ) | ( rapid_fire_state_1 & 0x20 );
		}
		sms_input_port0 = ( sms_input_port0 & 0xC0 ) | ( data & 0x3F );
		break;

	case 0x01:  /* Light Phaser */
		break;

	case 0x02:  /* Paddle Control */
		/* Get button A state */
		data = input_port_read(machine, "PADDLE0");
		if ( paddle_read_state )
		{
			data = data >> 4;
		}
		sms_input_port0 = ( sms_input_port0 & 0xC0 ) | ( data & 0x0F ) | ( paddle_read_state & 0x20 )
		                | ( ( input_port_read(machine, "CTRLIPT") & 0x02 ) << 3 );
		break;

	case 0x03:	/* Sega Sports Pad */
		switch( sports_pad_state_1 )
		{
		case 0:
			data = ( sports_pad_1_x >> 4 ) & 0x0F;
			break;
		case 1:
			data = sports_pad_1_x & 0x0F;
			break;
		case 2:
			data = ( sports_pad_1_y >> 4 ) & 0x0F;
			break;
		case 3:
			data = sports_pad_1_y & 0x0F;
			break;
		}
		sms_input_port0 = ( sms_input_port0 & 0xC0 ) | data | ( ( input_port_read(machine, "CTRLIPT") & 0x0C ) << 2 );
		break;
	}

	/* Player 2 */
	switch( input_port_read_safe(machine, "CTRLSEL", 0x00) >> 4 )
	{
	case 0x00:	/* Joystick */
		data = input_port_read(machine, "PORT_DC");
		sms_input_port0 = ( sms_input_port0 & 0x3F ) | ( data & 0xC0 );
		data = input_port_read(machine, "PORT_DD");
		if ( input_port_read(machine, "RFU") & 0x04 )
		{
			data = ( data & 0xFB ) | ( rapid_fire_state_2 & 0x04 );
		}
		if ( input_port_read(machine, "RFU") & 0x08 )
		{
			data = ( data & 0xF7 ) | ( rapid_fire_state_2 & 0x08 );
		}
		sms_input_port1 = ( sms_input_port1 & 0xF0 ) | ( data & 0x0F );
		break;

	case 0x01:	/* Light Phaser */
		break;

	case 0x02:	/* Paddle Control */
		/* Get button A state */
		data = input_port_read(machine, "PADDLE1");
		if ( paddle_read_state )
		{
			data = data >> 4;
		}
		sms_input_port0 = ( sms_input_port0 & 0x3F ) | ( ( data & 0x03 ) << 6 );
		sms_input_port1 = ( sms_input_port1 & 0xF0 ) | ( ( data & 0x0C ) >> 2 ) | ( paddle_read_state & 0x08 )
		                | ( ( input_port_read(machine, "CTRLIPT") & 0x20 ) >> 3 );
		break;

	case 0x03:	/* Sega Sports Pad */
		switch( sports_pad_state_2 )
		{
		case 0:
			data = sports_pad_2_x & 0x0F;
			break;
		case 1:
			data = ( sports_pad_2_x >> 4 ) & 0x0F;
		case 2:
			data = sports_pad_2_y & 0x0F;
			break;
		case 3:
			data = ( sports_pad_2_y >> 4 ) & 0x0F;
			break;
		}
		sms_input_port0 = ( sms_input_port0 & 0x3F ) | ( ( data & 0x03 ) << 6 );
		sms_input_port1 = ( sms_input_port1 & 0xF0 ) | ( data >> 2 ) | ( ( input_port_read(machine, "CTRLIPT") & 0xC0 ) >> 4 );
		break;
	}
}


WRITE8_HANDLER(sms_fm_detect_w)
{
	if ( HAS_FM )
	{
		smsFMDetect = (data & 0x01);
	}
}


READ8_HANDLER(sms_fm_detect_r)
{
	if ( HAS_FM )
	{
		return smsFMDetect;
	}
	else
	{
		if ( biosPort & IO_CHIP )
		{
			return 0xFF;
		}
		else
		{
			sms_get_inputs(space);
			return sms_input_port0;
		}
	}
}


WRITE8_HANDLER(sms_version_w)
{
	if ((data & 0x01) && (data & 0x04))
	{
		smsVersion = (data & 0xA0);
	}
	if ( data & 0x08 )
	{
		sms_input_write( space, 0, ( data & 0x20 ) >> 5 );
	}
	if ( data & 0x02 )
	{
		sms_input_write( space, 1, ( data & 0x80 ) >> 7 );
	}
}


READ8_HANDLER(sms_version_r)
{
	UINT8 temp;

	if (biosPort & IO_CHIP)
	{
		return (0xFF);
	}

	/* Move bits 7,5 of port 3F into bits 7, 6 */
	temp = (smsVersion & 0x80) | (smsVersion & 0x20) << 1;

	/* Inverse version detect value for Japanese machines */
	if ( IS_REGION_JAPAN )
	{
		temp ^= 0xC0;
	}

	/* Merge version data with input port #2 data */
	sms_get_inputs(space);
	temp = (temp & 0xC0) | (sms_input_port1 & 0x3F);

	return (temp);
}


READ8_HANDLER(sms_count_r)
{
	if ( offset & 0x01 )
	{
		return sms_vdp_hcount_r(space, offset);
	}
	else
	{
		/* VCount read */
		return sms_vdp_vcount_r(space, offset);
	}
}


/*
  Check if the pause button is pressed.
  If the gamegear is in sms mode, check if the start button is pressed.
 */
void sms_check_pause_button(const address_space *space)
{
	if ( IS_GAMEGEAR && ! ( sms_cartridge[sms_current_cartridge].features & CF_GG_SMS_MODE ) )
	{
		return;
	}
	if ( ! (input_port_read(space->machine, IS_GAMEGEAR ? "START" : "PAUSE") & 0x80) ) {
		if ( ! smsPaused ) {
			cputag_set_input_line(space->machine, "maincpu", INPUT_LINE_NMI, PULSE_LINE );
		}
		smsPaused = 1;
	}
	else
	{
		smsPaused = 0;
	}
}


READ8_HANDLER(sms_input_port_0_r)
{
	if (biosPort & IO_CHIP)
	{
		return (0xFF);
	}
	else
	{
		sms_get_inputs(space);
		return sms_input_port0;
	}
}


WRITE8_HANDLER(sms_ym2413_register_port_0_w)
{
	if ( HAS_FM )
	{
		const device_config *ym = devtag_get_device(space->machine, "ym2413");
		ym2413_w(ym, 0, (data & 0x3F));
	}
}


WRITE8_HANDLER(sms_ym2413_data_port_0_w)
{
	if ( HAS_FM )
	{
		const device_config *ym = devtag_get_device(space->machine, "ym2413");
		logerror("data_port_0_w %x %x\n", offset, data);
		ym2413_w(ym, 1, data);
	}
}


 READ8_HANDLER(gg_input_port_2_r)
{
	//logerror("joy 2 read, val: %02x, pc: %04x\n", (( IS_REGION_JAPAN ? 0x00 : 0x40) | (input_port_read(machine, "START") & 0x80)), activecpu_get_pc());
	return (( IS_REGION_JAPAN ? 0x00 : 0x40 ) | (input_port_read(space->machine, "START") & 0x80));
}


 READ8_HANDLER(sms_mapper_r)
{
	return sms_mapper[offset];
}


WRITE8_HANDLER(sms_mapper_w)
{
	int page;
	UINT8 *SOURCE_BIOS;
	UINT8 *SOURCE_CART;
	UINT8 *SOURCE;
	UINT8	rom_page_count = sms_cartridge[sms_current_cartridge].size / 0x4000;

	offset &= 3;

	sms_mapper[offset] = data;
	sms_mapper_ram[offset] = data;

	if ( sms_cartridge[sms_current_cartridge].ROM )
	{
		SOURCE_CART = sms_cartridge[sms_current_cartridge].ROM + ( ( rom_page_count > 0) ? data % rom_page_count : 0 ) * 0x4000;
	}
	else
	{
		SOURCE_CART = sms_banking_none[1];
	}
	if ( BIOS )
	{
		SOURCE_BIOS = BIOS + ( (smsBiosPageCount > 0) ? data % smsBiosPageCount : 0 ) * 0x4000;
	}
	else
	{
		SOURCE_BIOS = sms_banking_none[1];
	}

	if (biosPort & IO_BIOS_ROM || ( IS_GAMEGEAR && BIOS == NULL ) )
	{
		if (!(biosPort & IO_CARTRIDGE) || ( IS_GAMEGEAR && BIOS == NULL ) )
		{
			page = ( rom_page_count > 0) ? data % rom_page_count : 0;
			if ( ! sms_cartridge[sms_current_cartridge].ROM )
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
		page = (smsBiosPageCount > 0) ? data % smsBiosPageCount : 0;
		if ( ! BIOS )
			return;
		SOURCE = SOURCE_BIOS;
	}

	switch(offset)
	{
		case 0: /* Control */
			/* Is it ram or rom? */
			if (data & 0x08) /* it's ram */
			{
				sms_cartridge[sms_current_cartridge].sram_save = 1;			/* SRAM should be saved on exit. */
				if (data & 0x04)
				{
					LOG(("ram 1 paged.\n"));
					SOURCE = sms_cartridge[sms_current_cartridge].cartSRAM + 0x4000;
				}
				else
				{
					LOG(("ram 0 paged.\n"));
					SOURCE = sms_cartridge[sms_current_cartridge].cartSRAM;
				}
				memory_set_bankptr(space->machine,  4, SOURCE );
				memory_set_bankptr(space->machine,  5, SOURCE + 0x2000 );
			}
			else /* it's rom */
			{
				if ( biosPort & IO_BIOS_ROM || ! HAS_BIOS )
				{
					page = ( rom_page_count > 0) ? sms_mapper[3] % rom_page_count : 0;
					SOURCE = sms_banking_cart[4];
				}
				else
				{
					page = (smsBiosPageCount > 0) ? sms_mapper[3] % smsBiosPageCount : 0;
					SOURCE = sms_banking_bios[4];
				}
				LOG(("rom 2 paged in %x.\n", page));
				memory_set_bankptr(space->machine,  4, SOURCE );
				memory_set_bankptr(space->machine,  5, SOURCE + 0x2000 );
			}
			break;

		case 1: /* Select 16k ROM bank for 0400-3FFF */
			LOG(("rom 0 paged in %x.\n", page));
			sms_banking_bios[2] = SOURCE_BIOS + 0x0400;
			sms_banking_cart[2] = SOURCE_CART + 0x0400;
			if ( IS_GAMEGEAR )
			{
				SOURCE = SOURCE_CART;
			}
			memory_set_bankptr(space->machine,  2, SOURCE + 0x0400 );
			break;

		case 2: /* Select 16k ROM bank for 4000-7FFF */
			LOG(("rom 1 paged in %x.\n", page));
			sms_banking_bios[3] = SOURCE_BIOS;
			sms_banking_cart[3] = SOURCE_CART;
			if ( IS_GAMEGEAR )
			{
				SOURCE = SOURCE_CART;
			}
			memory_set_bankptr(space->machine,  3, SOURCE );
			break;

		case 3: /* Select 16k ROM bank for 8000-BFFF */
			sms_banking_bios[4] = SOURCE_BIOS;
			if ( IS_GAMEGEAR )
			{
				SOURCE = SOURCE_CART;
			}
			if ( sms_cartridge[sms_current_cartridge].features & CF_CODEMASTERS_MAPPER )
			{
				if ( SOURCE == SOURCE_CART )
				{
					SOURCE = sms_banking_cart[4];
				}
			}
			else
			{
				sms_banking_cart[4] = SOURCE_CART;
			}
			if ( ! ( sms_mapper[0] & 0x08 ) ) /* is RAM disabled? */
			{
				LOG(("rom 2 paged in %x.\n", page));
				memory_set_bankptr(space->machine,  4, SOURCE );
				memory_set_bankptr(space->machine,  5, SOURCE + 0x2000 );
			}
			break;
	}
}


static WRITE8_HANDLER(sms_codemasters_page0_w)
{
	if ( sms_cartridge[sms_current_cartridge].ROM && sms_cartridge[sms_current_cartridge].features & CF_CODEMASTERS_MAPPER )
	{
		UINT8 rom_page_count = sms_cartridge[sms_current_cartridge].size / 0x4000;
		sms_banking_cart[1] = sms_cartridge[sms_current_cartridge].ROM + ( ( rom_page_count > 0) ? data % rom_page_count : 0 ) * 0x4000;
		sms_banking_cart[2] = sms_banking_cart[1] + 0x0400;
		memory_set_bankptr(space->machine, 1, sms_banking_cart[1] );
		memory_set_bankptr(space->machine,  2, sms_banking_cart[2] );
	}
}


static WRITE8_HANDLER(sms_codemasters_page1_w)
{
	if ( sms_cartridge[sms_current_cartridge].ROM && sms_cartridge[sms_current_cartridge].features & CF_CODEMASTERS_MAPPER )
	{
		/* Check if we need to switch in some RAM */
		if ( data & 0x80 )
		{
			sms_cartridge[sms_current_cartridge].ram_page = data & 0x07;
			memory_set_bankptr(space->machine,  5, sms_cartridge[sms_current_cartridge].cartRAM + sms_cartridge[sms_current_cartridge].ram_page * 0x2000 );
		}
		else
		{
			UINT8 rom_page_count = sms_cartridge[sms_current_cartridge].size / 0x4000;
			sms_banking_cart[3] = sms_cartridge[sms_current_cartridge].ROM + ( ( rom_page_count > 0) ? data % rom_page_count : 0 ) * 0x4000;
			memory_set_bankptr(space->machine,  3, sms_banking_cart[3] );
			memory_set_bankptr(space->machine,  5, sms_banking_cart[4] + 0x2000 );
		}
	}
}


WRITE8_HANDLER(sms_bios_w)
{
	biosPort = data;

	logerror("bios write %02x, pc: %04x\n", data, cpu_get_pc(space->cpu));

	setup_rom(space);
}


WRITE8_HANDLER(sms_cartram2_w)
{
	if (sms_mapper[0] & 0x08)
	{
		logerror("write %02X to cartram at offset #%04X\n", data, offset + 0x2000);
		if (sms_mapper[0] & 0x04)
		{
			sms_cartridge[sms_current_cartridge].cartSRAM[offset + 0x6000] = data;
		}
		else
		{
			sms_cartridge[sms_current_cartridge].cartSRAM[offset + 0x2000] = data;
		}
	}
	if ( sms_cartridge[sms_current_cartridge].features & CF_CODEMASTERS_MAPPER )
	{
		sms_cartridge[sms_current_cartridge].cartRAM[sms_cartridge[sms_current_cartridge].ram_page * 0x2000 + offset] = data;
	}
	if ( sms_cartridge[sms_current_cartridge].features & CF_KOREAN_MAPPER && offset == 0 ) /* Dodgeball King mapper */
	{
		UINT8	rom_page_count = sms_cartridge[sms_current_cartridge].size / 0x4000;
		int		page = (rom_page_count > 0) ? data % rom_page_count : 0;
		if ( ! sms_cartridge[sms_current_cartridge].ROM )
			return;
		sms_banking_cart[4] = sms_cartridge[sms_current_cartridge].ROM + page * 0x4000;
		memory_set_bankptr(space->machine,  4, sms_banking_cart[4] );
		memory_set_bankptr(space->machine,  5, sms_banking_cart[4] + 0x2000 );
		LOG(("rom 2 paged in %x dodgeball king.\n", page));
	}
}


WRITE8_HANDLER(sms_cartram_w)
{
	int page;

	if (sms_mapper[0] & 0x08)
	{
		logerror("write %02X to cartram at offset #%04X\n", data, offset);
		if (sms_mapper[0] & 0x04)
		{
			sms_cartridge[sms_current_cartridge].cartSRAM[offset + 0x4000] = data;
		}
		else
		{
			sms_cartridge[sms_current_cartridge].cartSRAM[offset] = data;
		}
	}
	else
	{
		if ( sms_cartridge[sms_current_cartridge].features & CF_CODEMASTERS_MAPPER && offset == 0 ) /* Codemasters mapper */
		{
			UINT8	rom_page_count = sms_cartridge[sms_current_cartridge].size / 0x4000;
			page = (rom_page_count > 0) ? data % rom_page_count : 0;
			if ( ! sms_cartridge[sms_current_cartridge].ROM )
				return;
			sms_banking_cart[4] = sms_cartridge[sms_current_cartridge].ROM + page * 0x4000;
			memory_set_bankptr(space->machine,  4, sms_banking_cart[4] );
			memory_set_bankptr(space->machine,  5, sms_banking_cart[4] + 0x2000 );
			LOG(("rom 2 paged in %x codemasters.\n", page));
		}
		else if ( sms_cartridge[sms_current_cartridge].features & CF_ONCART_RAM )
		{
			sms_cartridge[sms_current_cartridge].cartRAM[offset & ( sms_cartridge[sms_current_cartridge].ram_size - 1 ) ] = data;
		}
		else
		{
			logerror("INVALID write %02X to cartram at offset #%04X\n", data, offset);
		}
	}
}


WRITE8_HANDLER(gg_sio_w)
{
	logerror("*** write %02X to SIO register #%d\n", data, offset);

	ggSIO[offset & 0x07] = data;
	switch(offset & 7)
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


 READ8_HANDLER(gg_sio_r)
{
	logerror("*** read SIO register #%d\n", offset);

	switch(offset & 7)
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

	return ggSIO[offset];
}


 READ8_HANDLER(gg_psg_r)
{
	return 0xFF;
}


WRITE8_HANDLER(gg_psg_w)
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


static void sms_machine_stop(running_machine *machine)
{
	/* Does the cartridge have SRAM that should be saved? */
	if ( sms_cartridge[sms_current_cartridge].sram_save )
		image_battery_save(devtag_get_device(machine, "cart1"), sms_cartridge[sms_current_cartridge].cartSRAM, sizeof(UINT8) * NVRAM_SIZE );
}


static void setup_rom(const address_space *space)
{
	running_machine *machine = space->machine;

	/* 1. set up bank pointers to point to nothing */
	memory_set_bankptr(machine,  1, sms_banking_none[1] );
	memory_set_bankptr(machine,  2, sms_banking_none[2] );
	memory_set_bankptr(machine,  3, sms_banking_none[3] );
	memory_set_bankptr(machine,  4, sms_banking_none[4] );
	memory_set_bankptr(machine,  5, sms_banking_none[4] + 0x2000 );

	/* 2. check and set up expansion port */
	if (!(biosPort & IO_EXPANSION) && (biosPort & IO_CARTRIDGE) && (biosPort & IO_CARD))
	{
		/* TODO: Implement me */
		logerror( "Switching to unsupported expansion port.\n" );
	}

	/* 3. check and set up card rom */
	if (!(biosPort & IO_CARD) && (biosPort & IO_CARTRIDGE) && (biosPort & IO_EXPANSION))
	{
		/* TODO: Implement me */
		logerror( "Switching to unsupported card rom port.\n" );
	}

	/* 4. check and set up cartridge rom */
	/* if ( ( !(biosPort & IO_CARTRIDGE) && (biosPort & IO_EXPANSION) && (biosPort & IO_CARD) ) || IS_GAMEGEAR ) { */
	/* Out Run Europa initially writes a value to port 3E where IO_CARTRIDGE, IO_EXPANSION and IO_CARD are reset */
	if ( ( ! ( biosPort & IO_CARTRIDGE ) ) || IS_GAMEGEAR )
	{
		memory_set_bankptr(machine,  1, sms_banking_cart[1] );
		memory_set_bankptr(machine,  2, sms_banking_cart[2] );
		memory_set_bankptr(machine,  3, sms_banking_cart[3] );
		memory_set_bankptr(machine,  4, sms_banking_cart[4] );
		memory_set_bankptr(machine,  5, sms_banking_cart[4] + 0x2000 );
		logerror( "Switched in cartridge rom.\n" );
	}

	/* 5. check and set up bios rom */
	if ( !(biosPort & IO_BIOS_ROM) )
	{
		/* 0x0400 bioses */
		if ( HAS_BIOS_0400 )
		{
			memory_set_bankptr(machine,  1, sms_banking_bios[1] );
			logerror( "Switched in 0x0400 bios.\n" );
		}
		/* 0x2000 bioses */
		if ( HAS_BIOS_2000 )
		{
			memory_set_bankptr(machine,  1, sms_banking_bios[1] );
			memory_set_bankptr(machine,  2, sms_banking_bios[2] );
			logerror( "Switched in 0x2000 bios.\n" );
		}
		if ( HAS_BIOS_FULL )
		{
			memory_set_bankptr(machine,  1, sms_banking_bios[1] );
			memory_set_bankptr(machine,  2, sms_banking_bios[2] );
			memory_set_bankptr(machine,  3, sms_banking_bios[3] );
			memory_set_bankptr(machine,  4, sms_banking_bios[4] );
			memory_set_bankptr(machine,  5, sms_banking_bios[4] + 0x2000 );
			logerror( "Switched in full bios.\n" );
		}
	}

	if ( sms_cartridge[sms_current_cartridge].features & CF_ONCART_RAM )
	{
		memory_set_bankptr(machine,  4, sms_cartridge[sms_current_cartridge].cartRAM );
		memory_set_bankptr(machine,  5, sms_cartridge[sms_current_cartridge].cartRAM );
	}
}


static int sms_verify_cart(UINT8 *magic, int size)
{
	int retval;

	retval = IMAGE_VERIFY_FAIL;

	/* Verify the file is a valid image - check $7ff0 for "TMR SEGA" */
	if (size >= 0x8000)
	{
		if (!strncmp((char*)&magic[0x7FF0], "TMR SEGA", 8))
		{
			/* Technically, it should be this, but remove for now until verified:
            if (!strcmp(sysname, "gamegear"))
            {
                if ((unsigned char)magic[0x7FFD] < 0x50)
                    retval = IMAGE_VERIFY_PASS;
            }
            if (!strcmp(sysname, "sms"))
            {
                if ((unsigned char)magic[0x7FFD] >= 0x50)
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
                if (!strcmp(sysname, "gamegear")) {
                    if ((unsigned char)magic[0x81fd] < 0x50)
                        retval = IMAGE_VERIFY_PASS;
                }
                if (!strcmp(sysname, "sms")) {
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
	static const UINT8 jang_pung2[16] = { 0x00, 0xBA, 0x38, 0x0D, 0x00, 0xB8, 0x38, 0x0C, 0x00, 0xB6, 0x38, 0x0B, 0x00, 0xB4, 0x38, 0x0A };

	if ( ( ( rom[0x7fe0] & 0x0F ) <= 9 ) &&
	     ( rom[0x7fe3] == 0x93 || rom[0x7fe3] == 0x94 || rom[0x7fe3] == 0x95 ) &&
	     rom[0x7fef] == 0x00 )
	{
		return 1;
	}

	if ( ! memcmp( &rom[0x7ff0], jang_pung2, 16 ) )
	{
		return 1;
	}

	return 0;
}


static int detect_korean_mapper( UINT8 *rom )
{
	static const UINT8 signatures[2][16] =
	{
		{ 0x3E, 0x11, 0x32, 0x00, 0xA0, 0x78, 0xCD, 0x84, 0x85, 0x3E, 0x02, 0x32, 0x00, 0xA0, 0xC9, 0xFF }, /* Dodgeball King */
		{ 0x41, 0x48, 0x37, 0x37, 0x44, 0x37, 0x4E, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x20 },	/* Sangokushi 3 */
	};
	int i;

	for( i = 0; i < 2; i++ )
	{
		if ( !memcmp( &rom[0x7ff0], signatures[i], 16 ) )
		{
			return 1;
		}
	}
	return 0;
}


DEVICE_START( sms_cart )
{
	int i;

	for ( i = 0; i < MAX_CARTRIDGES; i++ )
	{
		sms_cartridge[i].ROM = NULL;
		sms_cartridge[i].size = 0;
		sms_cartridge[i].features = 0;
		sms_cartridge[i].cartSRAM = NULL;
		sms_cartridge[i].sram_save = 0;
		sms_cartridge[i].cartRAM = NULL;
		sms_cartridge[i].ram_size = 0;
		sms_cartridge[i].ram_page = 0;
	}
	sms_current_cartridge = 0;

	biosPort = (IO_EXPANSION | IO_CARTRIDGE | IO_CARD);
	if ( ! IS_GAMEGEAR && ! HAS_BIOS )
	{
		biosPort &= ~(IO_CARTRIDGE);
		biosPort |= IO_BIOS_ROM;
	}
}


DEVICE_IMAGE_LOAD( sms_cart )
{
	int size = image_length(image);
	int index = 0;
	const char *fname = image_filename( image );
	int fname_len = fname ? strlen( fname ) : 0;
	const char *extrainfo = image_extrainfo( image );

	if (strcmp(image->tag,"cart1")==0)
	{
		index = 0;
	}
	if (strcmp(image->tag,"cart2")==0)
	{
		index = 1;
	}
	if (strcmp(image->tag,"cart3")==0)
	{
		index = 2;
	}
	if (strcmp(image->tag,"cart4")==0)
	{
		index = 3;
	}
	if (strcmp(image->tag,"cart5")==0)
	{
		index = 4;
	}
	/* Check for 512-byte header */
	if ((size / 512) & 1)
	{
		image_fseek(image, 512, SEEK_SET);
		size -= 512;
	}

	if ( ! size )
	{
		image_seterror( image, IMAGE_ERROR_UNSPECIFIED, "Invalid ROM image: ROM image is too small" );
		return INIT_FAIL;
	}

	/* Create a new memory region to hold the ROM. */
	/* Make sure the region holds only complete (0x4000) rom banks */
	sms_cartridge[index].size = (size&0x3FFF) ? (((size>>14)+1)<<14) : size;
	sms_cartridge[index].ROM = auto_alloc_array(image->machine, UINT8, sms_cartridge[index].size );

	sms_cartridge[index].cartSRAM = auto_alloc_array(image->machine, UINT8, NVRAM_SIZE );

	/* Load ROM banks */
	size = image_fread(image, sms_cartridge[index].ROM, size);

	/* check the image */
	if ( ! HAS_BIOS )
	{
		if (sms_verify_cart(sms_cartridge[index].ROM, size) == IMAGE_VERIFY_FAIL)
		{
			logerror("Warning loading image: sms_verify_cart failed\n");
		}
	}

	sms_cartridge[index].features = 0;

	/* Detect special features from the extrainfo field */
	if ( extrainfo )
	{
		/* Check for codemasters mapper */
		if ( strstr( extrainfo, "CODEMASTERS" ) )
		{
			sms_cartridge[index].features |= CF_CODEMASTERS_MAPPER;
		}

		/* Check for korean mapper */
		if ( strstr( extrainfo, "KOREAN" ) )
		{
			sms_cartridge[index].features |= CF_KOREAN_MAPPER;
		}

		/* Check for special SMS Compatibility mode gamegear cartridges */
		if ( IS_GAMEGEAR && strstr( extrainfo, "GGSMS" ) )
		{
			sms_cartridge[index].features |= CF_GG_SMS_MODE;
		}

		/* Check for 93C46 eeprom */
		if ( strstr( extrainfo, "93C46" ) )
		{
			sms_cartridge[index].features |= CF_93C46_EEPROM;
		}

		/* Check for 8KB on-cart RAM */
		if ( strstr( extrainfo, "8KB_CART_RAM" ) )
		 {
			sms_cartridge[index].features |= CF_ONCART_RAM;
			sms_cartridge[index].ram_size = 0x2000;
			sms_cartridge[index].cartRAM = auto_alloc_array(image->machine, UINT8, sms_cartridge[index].ram_size );
		}
	}
	else
	{
		/* If no extrainfo information is available try to find special information out on our own */
		/* Check for special cartridge features */
		if ( size >= 0x8000 )
		{
			/* Check for special mappers */
			if ( detect_codemasters_mapper( sms_cartridge[index].ROM ) )
			{
				sms_cartridge[index].features |= CF_CODEMASTERS_MAPPER;
			}
			if ( detect_korean_mapper( sms_cartridge[index].ROM ) )
			{
				sms_cartridge[index].features |= CF_KOREAN_MAPPER;
			}
		}

		/* Check for special SMS Compatibility mode gamegear cartridges */
		if ( IS_GAMEGEAR )
		{
			/* Just in case someone passes us an sms file */
			if ( fname_len > 3 && ! mame_stricmp( fname + fname_len - 3, "sms" ) )
			{
				sms_cartridge[index].features |= CF_GG_SMS_MODE;
			}
		}
	}

	if ( sms_cartridge[index].features & CF_CODEMASTERS_MAPPER )
	{
		sms_cartridge[index].ram_size = 0x10000;
		sms_cartridge[index].cartRAM = auto_alloc_array(image->machine, UINT8, sms_cartridge[index].ram_size );
		sms_cartridge[index].ram_page = 0;
	}

	/* Load battery backed RAM, if available */
	image_battery_load( image, sms_cartridge[index].cartSRAM, sizeof(UINT8) * NVRAM_SIZE );

	return INIT_PASS;
}


static void setup_cart_banks( void )
{
	if ( sms_cartridge[sms_current_cartridge].ROM )
	{
		UINT8   rom_page_count = sms_cartridge[sms_current_cartridge].size / 0x4000;
		sms_banking_cart[1] = sms_cartridge[sms_current_cartridge].ROM;
		sms_banking_cart[2] = sms_cartridge[sms_current_cartridge].ROM + 0x0400;
		sms_banking_cart[3] = sms_cartridge[sms_current_cartridge].ROM + ( ( 1 < rom_page_count ) ? 0x4000 : 0 );
		sms_banking_cart[4] = sms_cartridge[sms_current_cartridge].ROM + ( ( 2 < rom_page_count ) ? 0x8000 : 0 );
		/* Codemasters mapper points to bank 0 for page 2 */
		if ( sms_cartridge[sms_current_cartridge].features & CF_CODEMASTERS_MAPPER ) {
			sms_banking_cart[4] = sms_cartridge[sms_current_cartridge].ROM;
		}
	}
	else
	{
		sms_banking_cart[1] = sms_banking_none[1];
		sms_banking_cart[2] = sms_banking_none[2];
		sms_banking_cart[3] = sms_banking_none[3];
		sms_banking_cart[4] = sms_banking_none[4];
	}
}


static void setup_banks( running_machine *machine )
{
	UINT8 *mem = memory_region(machine, "maincpu");
	sms_banking_bios[1] = sms_banking_cart[1] = sms_banking_none[1] = mem;
	sms_banking_bios[2] = sms_banking_cart[2] = sms_banking_none[2] = mem;
	sms_banking_bios[3] = sms_banking_cart[3] = sms_banking_none[3] = mem;
	sms_banking_bios[4] = sms_banking_cart[4] = sms_banking_none[4] = mem;

	BIOS = memory_region(machine, "user1");

	smsBiosPageCount = ( BIOS ? memory_region_length(machine, "user1") / 0x4000 : 0 );

	setup_cart_banks();

	if ( BIOS == NULL || BIOS[0] == 0x00 )
	{
		BIOS = NULL;
                biosPort |= IO_BIOS_ROM;
	}

	if ( BIOS )
	{
		sms_banking_bios[1] = BIOS;
		sms_banking_bios[2] = BIOS + 0x0400;
		sms_banking_bios[3] = BIOS + ( ( 1 < smsBiosPageCount) ? 0x4000 : 0 );
		sms_banking_bios[4] = BIOS + ( ( 2 < smsBiosPageCount) ? 0x8000 : 0 );
	}
}


MACHINE_START(sms)
{
	add_exit_callback(machine, sms_machine_stop);
}


MACHINE_RESET(sms)
{
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);

	smsVersion = 0x00;
	if ( HAS_FM )
	{
		smsFMDetect = 0x01;
	}

	sms_mapper_ram = memory_get_write_ptr( space, 0xDFFC );

	biosPort = 0;

	if ( sms_cartridge[sms_current_cartridge].features & CF_CODEMASTERS_MAPPER )
	{
		/* Install special memory handlers */
		memory_install_write8_handler(space, 0x0000, 0x0000, 0, 0, sms_codemasters_page0_w );
		memory_install_write8_handler(space, 0x4000, 0x4000, 0, 0, sms_codemasters_page1_w );
	}

	if ( sms_cartridge[sms_current_cartridge].features & CF_GG_SMS_MODE )
	{
		sms_set_ggsmsmode( 1 );
	}

	/* Initialize SIO stuff for GG */
	ggSIO[0] = 0x7F;
	ggSIO[1] = 0xFF;
	ggSIO[2] = 0x00;
	ggSIO[3] = 0xFF;
	ggSIO[4] = 0x00;

	sms_store_control = 0;

	setup_banks(machine);

	setup_rom(space);

	rapid_fire_state_1 = 0;
	rapid_fire_state_2 = 0;
	rapid_fire_timer = timer_alloc(machine,  rapid_fire_callback , NULL);
	timer_adjust_periodic(rapid_fire_timer, ATTOTIME_IN_HZ(10), 0, ATTOTIME_IN_HZ(10));

	last_paddle_read_time = 0;
	paddle_read_state = 0;

	last_sports_pad_time_1 = 0;
	last_sports_pad_time_2 = 0;
	sports_pad_state_1 = 0;
	sports_pad_state_2 = 0;
	sports_pad_last_data_1 = 0;
	sports_pad_last_data_2 = 0;
	sports_pad_1_x = 0;
	sports_pad_1_y = 0;
	sports_pad_2_x = 0;
	sports_pad_2_y = 0;
}


READ8_HANDLER(sms_store_cart_select_r)
{
	return 0xFF;
}


WRITE8_HANDLER(sms_store_cart_select_w)
{
	UINT8 slot = data >> 4;
	UINT8 slottype = data & 0x08;

	logerror("switching in part of %s slot #%d\n", slottype ? "card" : "cartridge", slot );
	/* cartridge? slot #0 */
	if ( slottype == 0 )
	{
		sms_current_cartridge = slot;
	}
	setup_cart_banks();
	memory_set_bankptr(space->machine,  10, sms_banking_cart[3] + 0x2000 );
	setup_rom(space);
}


READ8_HANDLER(sms_store_select1)
{
	return 0xFF;
}


READ8_HANDLER(sms_store_select2)
{
	return 0xFF;
}


READ8_HANDLER(sms_store_control_r)
{
	return sms_store_control;
}


WRITE8_HANDLER(sms_store_control_w)
{
	logerror( "0x%04X: sms_store_control write 0x%02X\n", cpu_get_pc(space->cpu), data );
	if ( data & 0x02 )
	{
		cputag_resume( space->machine, "maincpu", SUSPEND_REASON_HALT );
	}
	else
	{
		/* Pull reset line of CPU #0 low */
		cputag_suspend( space->machine, "maincpu", SUSPEND_REASON_HALT, 1 );
		device_reset(cputag_get_cpu(space->machine, "maincpu"));
	}
	sms_store_control = data;
}


void sms_int_callback( running_machine *machine, int state )
{
	cputag_set_input_line(machine, "maincpu", 0, state );
}


void sms_store_int_callback( running_machine *machine, int state )
{
	cputag_set_input_line(machine, "maincpu", sms_store_control & 0x01 ? 1 : 0, state );
}


DRIVER_INIT( sg1000m3 )
{
	sms_flags = FLAG_REGION_JAPAN | FLAG_FM;
}


DRIVER_INIT( sms1 )
{
	sms_flags = FLAG_BIOS_FULL;
}


DRIVER_INIT( smsj )
{
	sms_flags = FLAG_REGION_JAPAN | FLAG_BIOS_2000 | FLAG_FM;
}


DRIVER_INIT( sms2kr )
{
	sms_flags = FLAG_REGION_JAPAN | FLAG_BIOS_FULL | FLAG_FM;
}


DRIVER_INIT( smssdisp )
{
	sms_flags = 0;
}


DRIVER_INIT( gamegear )
{
	sms_flags = FLAG_GAMEGEAR;
}


DRIVER_INIT( gamegeaj )
{
	sms_flags = FLAG_REGION_JAPAN | FLAG_GAMEGEAR | FLAG_BIOS_0400;
}

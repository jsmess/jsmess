/***************************************************************************

  Atari VCS 2600 driver

***************************************************************************/

#include "driver.h"
#include "machine/6532riot.h"
#include "cpu/m6502/m6502.h"
#include "sound/tiaintf.h"
#include "devices/cartslot.h"
#include "devices/cassette.h"
#include "formats/a26_cas.h"
#include "video/tia.h"
#include "inputx.h"

#define CART memory_region(REGION_USER1)

#define MASTER_CLOCK_NTSC	3579575
#define MASTER_CLOCK_PAL	3546894
#define CATEGORY_SELECT		256

enum
{
	mode2K,
	mode4K,
	mode8K,
	mode12,
	mode16,
	mode32,
	modeAV,
	modePB,
	modeTV,
	modeUA,
	modeMN,
	modeDC,
	modeCV,
	mode3E,
	modeSS
};


static UINT8* extra_RAM;
static UINT8* bank_base[5];
static UINT8* ram_base;

static UINT8 keypad_left_column;
static UINT8 keypad_right_column;

static unsigned cart_size;
static unsigned number_banks;
static unsigned current_bank;
static unsigned mode3E_ram_enabled;
static UINT8 modeSS_byte;
static UINT32 modeSS_byte_started;
static unsigned modeSS_write_delay;
static unsigned modeSS_write_enabled;
static unsigned modeSS_write_pending;
static unsigned modeSS_high_ram_enabled;
static unsigned modeSS_diff_adjust;


static int detect_modeDC(void)
{
	int i,numfound = 0;
	// signature is also in 'video reflex'.. maybe figure out that controller port someday...
	unsigned char signature[3] = { 0x8d, 0xf0, 0xff };
	if (cart_size == 0x10000)
	{
		for (i = 0; i < cart_size - sizeof signature; i++)
		{
			if (!memcmp(&CART[i], signature,sizeof signature))
			{
				numfound = 1;
			}
		}
	}
	if (numfound) return 1;
	return 0;
}

static int detect_mode3E(void)
{
	// this one is a little hacky.. looks for STY $3e, which is unique to
	// 'not boulderdash', but is the only example i have (cow)
	// Would have used STA $3e, but 'Alien' and 'Star Raiders' do that for unknown reasons

	int i,numfound = 0;
	unsigned char signature[3] = { 0x84, 0x3e, 0x9d };
	if (cart_size == 0x0800 || cart_size == 0x1000)
	{
		for (i = 0; i < cart_size - sizeof signature; i++)
		{
			if (!memcmp(&CART[i], signature,sizeof signature))
			{
				numfound = 1;
			}
		}
	}
	if (numfound) return 1;
	return 0;
}

static int detect_modeSS(void)
{
	int i,numfound = 0;
	unsigned char signature[5] = { 0xbd, 0xe5, 0xff, 0x95, 0x81 };
	if (cart_size == 0x0800 || cart_size == 0x1000)
	{
		for (i = 0; i < cart_size - sizeof signature; i++)
		{
			if (!memcmp(&CART[i], signature,sizeof signature))
			{
				numfound = 1;
			}
		}
	}
	if (numfound) return 1;
	return 0;
}

static int detect_modeAV(void)
{
	int i,j,numfound = 0;
	unsigned char signatures[][5] =  {
									{ 0x20, 0x00, 0xd0, 0xc6, 0xc5 },
									{ 0x20, 0xc3, 0xf8, 0xa5, 0x82 },
									{ 0xd0, 0xfb, 0x20, 0x73, 0xfe },
									{ 0x20, 0x00, 0xf0, 0x84, 0xd6 }};
	if (cart_size == 0x2000)
	{
		for (i = 0; i < cart_size - (sizeof signatures/sizeof signatures[0]); i++)
		{
			for (j = 0; j < (sizeof signatures/sizeof signatures[0]) && !numfound; j++)
			{
				if (!memcmp(&CART[i], &signatures[j],sizeof signatures[0]))
				{
					numfound = 1;
				}
			}
		}
	}
	if (numfound) return 1;
	return 0;
}

static int detect_modePB(void)
{
	int i,j,numfound = 0;
	unsigned char signatures[][3] =  {
									{ 0x8d, 0xe0, 0x1f },
									{ 0x8d, 0xe0, 0x5f },
									{ 0x8d, 0xe9, 0xff },
									{ 0xad, 0xe9, 0xff },
									{ 0xad, 0xed, 0xff },
									{ 0xad, 0xf3, 0xbf }};
	if (cart_size == 0x2000)
	{
		for (i = 0; i < cart_size - (sizeof signatures/sizeof signatures[0]); i++)
		{
			for (j = 0; j < (sizeof signatures/sizeof signatures[0]) && !numfound; j++)
			{
				if (!memcmp(&CART[i], &signatures[j],sizeof signatures[0]))
				{
					numfound = 1;
				}
			}
		}
	}
	if (numfound) return 1;
	return 0;
}

static int detect_modeCV(void)
{
	int i,j,numfound = 0;
	unsigned char signatures[][3] = {
									{ 0x9d, 0xff, 0xf3 },
									{ 0x99, 0x00, 0xf4 }};
	if (cart_size == 0x0800 || cart_size == 0x1000)
	{
		for (i = 0; i < cart_size - (sizeof signatures/sizeof signatures[0]); i++)
		{
			for (j = 0; j < (sizeof signatures/sizeof signatures[0]) && !numfound; j++)
			{
				if (!memcmp(&CART[i], &signatures[j],sizeof signatures[0]))
				{
					numfound = 1;
				}
			}
		}
	}
	if (numfound) return 1;
	return 0;
}

static int detect_modeMN(void)
{
	int i,j,numfound = 0;
	unsigned char signatures[][3] = {
									{ 0xad, 0xe5, 0xff },
									{ 0x8d, 0xe7, 0xff }};
	if (cart_size == 0x4000)
	{
		for (i = 0; i < cart_size - (sizeof signatures/sizeof signatures[0]); i++)
		{
			for (j = 0; j < (sizeof signatures/sizeof signatures[0]) && !numfound; j++)
			{
				if (!memcmp(&CART[i], &signatures[j],sizeof signatures[0]))
				{
					numfound = 1;
				}
			}
		}
	}
	if (numfound) return 1;
	return 0;
}

static int detect_modeUA(void)
{
	int i,numfound = 0;
	unsigned char signature[3] = { 0x8d, 0x40, 0x02 };
	if (cart_size == 0x2000)
	{
		for (i = 0; i < cart_size - sizeof signature; i++)
		{
			if (!memcmp(&CART[i], signature,sizeof signature))
			{
				numfound = 1;
			}
		}
	}
	if (numfound) return 1;
	return 0;
}

static int detect_8K_modeTV(void)
{
	int i,numfound = 0;
	unsigned char signature1[4] = { 0xa9, 0x01, 0x85, 0x3f };
	unsigned char signature2[4] = { 0xa9, 0x02, 0x85, 0x3f };
	// have to look for two signatures because 'not boulderdash' gives false positive otherwise
	if (cart_size == 0x2000)
	{
		for (i = 0; i < cart_size - sizeof signature1; i++)
		{
			if (!memcmp(&CART[i], signature1,sizeof signature1))
			{
				numfound |= 0x01;
			}
			if (!memcmp(&CART[i], signature2,sizeof signature2))
			{
				numfound |= 0x02;
			}
		}
	}
	if (numfound == 0x03) return 1;
	return 0;
}

static int detect_32K_modeTV(void)
{
	int i,numfound = 0;
	unsigned char signature[4] = { 0xa9, 0x0e, 0x85, 0x3f };
	if (cart_size >= 0x8000)
	{
		for (i = 0; i < cart_size - sizeof signature; i++)
		{
			if (!memcmp(&CART[i], signature,sizeof signature))
			{
				numfound++;
			}
		}
	}
	if (numfound > 1) return 1;
	return 0;
}

static int detect_super_chip(void)
{
	int i,j;
	unsigned char signatures[][5] = {
									{ 0xa2, 0x7f, 0x9d, 0x00, 0xf0 }, // dig dug
									{ 0xae, 0xf6, 0xff, 0x4c, 0x00 }}; // off the wall

	if (cart_size == 0x4000)
	{
		for (i = 0; i < cart_size - (sizeof signatures/sizeof signatures[0]); i++)
		{
			for (j = 0; j < (sizeof signatures/sizeof signatures[0]); j++)
			{
				if (!memcmp(&CART[i], &signatures[j],sizeof signatures[0]))
				{
					return 1;
				}
			}
		}
	}
	for (i = 0x1000; i < cart_size; i += 0x1000)
	{
		if (memcmp(CART, CART + i, 0x100))
		{
			return 0;
		}
	}
	/* Check the reset vector does not point into the super chip RAM area */
	i = ( CART[0x0FFD] << 8 ) | CART[0x0FFC];
	if ( ( i & 0x0FFF ) < 0x0100 ) {
		return 0;
	}
	return 1;
}


static DEVICE_LOAD( a2600_cart )
{
	cart_size = image_length(image);

	switch (cart_size)
	{
	case 0x00800:
	case 0x01000:
	case 0x02000:
	case 0x03000:
	case 0x04000:
	case 0x08000:
	case 0x10000:
	case 0x80000:
		break;

	default:
		image_seterror( image, IMAGE_ERROR_UNSUPPORTED, "Invalid rom file size" );
		return 1; /* unsupported image format */
	}

	current_bank = 0;

	image_fread(image, CART, cart_size);

	return 0;
}


static int next_bank(void)
{
	return current_bank = (current_bank + 1) % 16;
}


void mode8K_switch(UINT16 offset, UINT8 data)
{
	bank_base[1] = CART + 0x1000 * offset;
	memory_set_bankptr(1, bank_base[1]);
}
void mode12_switch(UINT16 offset, UINT8 data)
{
	bank_base[1] = CART + 0x1000 * offset;
	memory_set_bankptr(1, bank_base[1]);
}
void mode16_switch(UINT16 offset, UINT8 data)
{
	bank_base[1] = CART + 0x1000 * offset;
	memory_set_bankptr(1, bank_base[1]);
}
void mode32_switch(UINT16 offset, UINT8 data)
{
	bank_base[1] = CART + 0x1000 * offset;
	memory_set_bankptr(1, bank_base[1]);
}
void modeTV_switch(UINT16 offset, UINT8 data)
{
	bank_base[1] = CART + 0x800 * (data & (number_banks - 1));
	memory_set_bankptr(1, bank_base[1]);
}
void modeUA_switch(UINT16 offset, UINT8 data)
{
	bank_base[1] = CART + (offset >> 6) * 0x1000;
	memory_set_bankptr(1, bank_base[1]);
}
void modePB_switch(UINT16 offset, UINT8 data)
{
	int bank = 1 + (offset >> 3);
	bank_base[bank] = CART + 0x400 * (offset & 7);
	memory_set_bankptr(bank, bank_base[bank]);
}
void modeMN_switch(UINT16 offset, UINT8 data)
{
	bank_base[1] = CART + 0x800 * offset;
	memory_set_bankptr(1, bank_base[1]);
}
void modeMN_RAM_switch(UINT16 offset, UINT8 data)
{
	memory_set_bankptr(9, extra_RAM + (4 + offset) * 256 );
}
void modeDC_switch(UINT16 offset, UINT8 data)
{
	bank_base[1] = CART + 0x1000 * next_bank();
	memory_set_bankptr(1, bank_base[1]);
}
void mode3E_switch(UINT16 offset, UINT8 data)
{
	bank_base[1] = CART + 0x800 * (data & (number_banks - 1));
	memory_set_bankptr(1, bank_base[1]);
	mode3E_ram_enabled = 0;
}
void mode3E_RAM_switch(UINT16 offset, UINT8 data)
{
	ram_base = extra_RAM + 0x200 * ( data & 0x3F );
	memory_set_bankptr(1, ram_base );
	mode3E_ram_enabled = 1;
}


/* These read handlers will return the byte from the new bank */
static  READ8_HANDLER(mode8K_switch_r) { mode8K_switch(offset, 0); return bank_base[1][0xff8 + offset]; }
static  READ8_HANDLER(mode12_switch_r) { mode12_switch(offset, 0); return bank_base[1][0xff8 + offset]; }
static  READ8_HANDLER(mode16_switch_r) { mode16_switch(offset, 0); return bank_base[1][0xff6 + offset]; }
static  READ8_HANDLER(mode32_switch_r) { mode32_switch(offset, 0); return bank_base[1][0xff4 + offset]; }
static  READ8_HANDLER(modePB_switch_r) { modePB_switch(offset, 0); return bank_base[4][0x3e0 + offset]; }
static  READ8_HANDLER(modeMN_switch_r) { modeMN_switch(offset, 0); return bank_base[1][0xfe0 + offset]; }
static  READ8_HANDLER(modeMN_RAM_switch_r) { modeMN_RAM_switch(offset, 0); return 0; }
static  READ8_HANDLER(modeUA_switch_r) { modeUA_switch(offset, 0); return 0; }
static  READ8_HANDLER(modeDC_switch_r) { modeDC_switch(offset, 0); return bank_base[1][0xff0 + offset]; }


static WRITE8_HANDLER(mode8K_switch_w) { mode8K_switch(offset, data); }
static WRITE8_HANDLER(mode12_switch_w) { mode12_switch(offset, data); }
static WRITE8_HANDLER(mode16_switch_w) { mode16_switch(offset, data); }
static WRITE8_HANDLER(mode32_switch_w) { mode32_switch(offset, data); }
static WRITE8_HANDLER(modePB_switch_w) { modePB_switch(offset, data); }
static WRITE8_HANDLER(modeMN_switch_w) { modeMN_switch(offset, data); }
static WRITE8_HANDLER(modeMN_RAM_switch_w) { modeMN_RAM_switch(offset, data); }
static WRITE8_HANDLER(modeTV_switch_w) { modeTV_switch(offset, data); }
static WRITE8_HANDLER(modeUA_switch_w) { modeUA_switch(offset, data); }
static WRITE8_HANDLER(modeDC_switch_w) { modeDC_switch(offset, data); }
static WRITE8_HANDLER(mode3E_switch_w) { mode3E_switch(offset, data); }
static WRITE8_HANDLER(mode3E_RAM_switch_w) { mode3E_RAM_switch(offset, data); }
static WRITE8_HANDLER(mode3E_RAM_w) {
	if ( mode3E_ram_enabled ) {
		ram_base[offset] = data;
	}
}

OPBASE_HANDLER( modeSS_opbase )
{
	if ( address & 0x1000 ) {
		opcode_mask = 0x7ff;
		opcode_memory_min = ( address & 0xf800 );
		opcode_memory_max = ( address & 0xf800 ) | 0x7ff;
		if ( address & 0x800 ) {
			opcode_arg_base = bank_base[2];
			opcode_base = bank_base[2];
		} else {
			opcode_arg_base = bank_base[1];
			opcode_base = bank_base[1];
		}
		return ~0;
	}
	return address;
}

static READ8_HANDLER(modeSS_r)
{
	UINT8 data = ( offset & 0x800 ) ? bank_base[2][offset & 0x7FF] : bank_base[1][offset];

	//logerror("%04X: read from modeSS area offset = %04X\n", activecpu_get_pc(), offset);
	if ( offset < 0x100 && ! modeSS_write_pending )
	{
		modeSS_byte = offset;
		modeSS_byte_started = activecpu_gettotalcycles();
		modeSS_write_pending = 1;
	} else {
		modeSS_write_pending = 0;
		/* Control register "write" */
		if ( offset == 0xFF8 ) {
			//logerror("%04X: write to modeSS control register data = %02X\n", activecpu_get_pc(), modeSS_byte);
			modeSS_write_enabled = modeSS_byte & 0x02;
			modeSS_write_delay = modeSS_byte >> 5;
			switch ( modeSS_byte & 0x1C ) {
			case 0x00:
				bank_base[1] = extra_RAM + 2 * 0x800;
				bank_base[2] = ( modeSS_byte & 0x01 ) ? memory_region(REGION_CPU1) + 0x1800 : CART;
				modeSS_high_ram_enabled = 0;
				break;
			case 0x04:
				bank_base[1] = extra_RAM;
				bank_base[2] = ( modeSS_byte & 0x01 ) ? memory_region(REGION_CPU1) + 0x1800 : CART;
				modeSS_high_ram_enabled = 0;
				break;
			case 0x08:
				bank_base[1] = extra_RAM + 2 * 0x800;
				bank_base[2] = extra_RAM;
				modeSS_high_ram_enabled = 1;
				break;
			case 0x0C:
				bank_base[1] = extra_RAM;
				bank_base[2] = extra_RAM + 2 * 0x800;
				modeSS_high_ram_enabled = 1;
				break;
			case 0x10:
				bank_base[1] = extra_RAM + 2 * 0x800;
				bank_base[2] = ( modeSS_byte & 0x01 ) ? memory_region(REGION_CPU1) + 0x1800 : CART;
				modeSS_high_ram_enabled = 0;
				break;
			case 0x14:
				bank_base[1] = extra_RAM + 0x800;
				bank_base[2] = ( modeSS_byte & 0x01 ) ? memory_region(REGION_CPU1) + 0x1800 : CART;
				modeSS_high_ram_enabled = 0;
				break;
			case 0x18:
				bank_base[1] = extra_RAM + 2 * 0x800;
				bank_base[2] = extra_RAM + 0x800;
				modeSS_high_ram_enabled = 1;
				break;
			case 0x1C:
				bank_base[1] = extra_RAM + 0x800;
				bank_base[2] = extra_RAM + 2 * 0x800;
				modeSS_high_ram_enabled = 1;
				break;
			}
			memory_set_bankptr( 1, bank_base[1] );
			memory_set_bankptr( 2, bank_base[2] );
		} else if ( offset == 0xFF9 ) {
			/* Cassette port read */
			double tap_val = cassette_input( image_from_devtype_and_index( IO_CASSETTE, 0 ) );
			//logerror("%04X: Cassette port read, tap_val = %f\n", activecpu_get_pc(), tap_val);
			if ( tap_val < 0 ) {
				data = 0x00;
			} else {
				data = 0x01;
			}
		} else {
			/* Possible RAM write */
			if ( modeSS_write_enabled ) {
				int diff = activecpu_gettotalcycles() - modeSS_byte_started;
				//logerror("%04X: offset = %04X, %d\n", activecpu_get_pc(), offset, diff);
				if ( diff - modeSS_diff_adjust == 5 ) {
					//logerror("%04X: RAM write offset = %04X, data = %02X\n", activecpu_get_pc(), offset, modeSS_byte );
					if ( offset & 0x800 ) {
						if ( modeSS_high_ram_enabled ) {
							bank_base[2][offset & 0x7FF] = modeSS_byte;
							data = modeSS_byte;
						}
					} else {
						bank_base[1][offset] = modeSS_byte;
						data = modeSS_byte;
					}
				}
				/* Check for dummy read from same address */
				if ( diff == 2 ) {
					modeSS_diff_adjust = 1;
				} else {
					modeSS_diff_adjust = 0;
				}
			}
		}
	}
	return data;
}

/*

There seems to be a kind of lag between the writing to address 0x1FE and the
Activision switcher springing into action. It waits for the next byte to arrive
on the data bus, which is the new PCH in the case of a JSR, and the PCH of the
stored PC on the stack in the case of an RTS.

depending on last byte & 0x20 -> 0x00 -> switch to bank #1
                              -> 0x20 -> switch to bank #0

 */

static opbase_handler AV_old_opbase_handler;
static int AVTimer;

OPBASE_HANDLER(modeAV_opbase_handler)
{
	if ( ! AVTimer )
	{
		/* Still cheating a bit here by looking bit 13 of the address..., but the high byte of the
		   cpu should be the last byte that was on the data bus and so should determine the bank
		   we should switch in. */
		bank_base[1] = CART + 0x1000 * ( ( address & 0x2000 ) ? 0 : 1 );
		memory_set_bankptr( 1, bank_base[1] );
		/* and restore old opbase handler */
		memory_set_opbase_handler(0, AV_old_opbase_handler);
	}
	else
	{
		/* Wait for one memory access to have passed (reading of new PCH either from code or from stack) */
		AVTimer--;
	}
	return address;
}

void modeAV_switch(UINT16 offset, UINT8 data)
{
	/* Retrieve last byte read by the cpu (for this mapping scheme this
	   should be the last byte that was on the data bus
	*/
	AVTimer = 1;
	AV_old_opbase_handler = memory_set_opbase_handler(0, modeAV_opbase_handler);
	catch_nextBranch();
}

static READ8_HANDLER(modeAV_switch_r)
{
	modeAV_switch(offset, 0 );
	return program_read_byte_8( 0xFE );
}

static WRITE8_HANDLER(modeAV_switch_w)
{
	program_write_byte_8( 0xFE, data );
	modeAV_switch(offset, 0 );
}

static  READ8_HANDLER(current_bank_r)
{
	return current_bank;
}

static ADDRESS_MAP_START(a2600_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_FLAGS( AMEF_ABITS(13) )
	AM_RANGE(0x0000, 0x007F) AM_MIRROR(0x0F00) AM_READWRITE(tia_r, tia_w)
	AM_RANGE(0x0080, 0x00FF) AM_MIRROR(0x0D00) AM_RAM
	AM_RANGE(0x0280, 0x029F) AM_MIRROR(0x0D00) AM_READWRITE(r6532_0_r, r6532_0_w)
	AM_RANGE(0x1000, 0x1FFF)                   AM_ROMBANK(1)
ADDRESS_MAP_END

static WRITE8_HANDLER( switch_A_w )
{
	/* Left controller port */
	if ( readinputport(9) / CATEGORY_SELECT == 0x04 ) {
		keypad_left_column = data / 16;
	}

	/* Right controller port */
	switch( readinputport(9) % CATEGORY_SELECT ) {
	case 0x04:	/* Keypad */
		keypad_right_column = data & 0x0F;
		break;
	case 0x20:	/* KidVid voice module */
		cassette_change_state( image_from_devtype_and_index( IO_CASSETTE, 0 ), ( data & 0x02 ) ? CASSETTE_MOTOR_DISABLED : CASSETTE_MOTOR_ENABLED | CASSETTE_PLAY, CASSETTE_MOTOR_DISABLED );
		break;
	}
}

static  READ8_HANDLER( switch_A_r )
{
	static const UINT8 driving_lookup[4] = { 0x00, 0x02, 0x03, 0x01 };
	UINT8 val = 0;

	/* Left controller port */
	switch( readinputport(9) / CATEGORY_SELECT ) {
	case 0x00:  /* Joystick */
	case 0x10:	/* Joystick w/Boostergrip */
		val |= readinputport(6) & 0xF0;
		break;
	case 0x01:  /* Paddle */
		val |= readinputport(7) & 0xF0;
		break;
	case 0x02:	/* Driving */
		val |= 0xC0;
		val |= ( driving_lookup[ ( readinputport(12) & 0x18 ) >> 3 ] << 4 );
		break;
	default:
		val |= 0xF0;
		break;
	}

	/* Right controller port */
	switch( readinputport(9) % CATEGORY_SELECT ) {
	case 0x00:	/* Joystick */
	case 0x10:	/* Joystick w/Boostergrip */
		val |= readinputport(6) & 0x0F;
		break;
	case 0x01:	/* Paddle */
		val |= readinputport(7) & 0x0F;
		break;
	case 0x02:	/* Driving */
		val |= 0x0C;
		val |= ( driving_lookup[ ( readinputport(13) & 0x18 ) >> 3 ] );
		break;
	default:
		val |= 0x0F;
		break;
	}

	return val;
}


static struct R6532interface r6532_interface =
{
	switch_A_r,
	input_port_8_r,
	switch_A_w,
	NULL
};


static void install_banks(int count, unsigned init)
{
	int i;

	for (i = 0; i < count; i++)
	{
		static const read8_handler handler[] =
		{
			MRA8_BANK1,
			MRA8_BANK2,
			MRA8_BANK3,
			MRA8_BANK4,
		};

		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM,
			0x1000 + (i + 0) * 0x1000 / count - 0,
			0x1000 + (i + 1) * 0x1000 / count - 1, 0, 0, handler[i]);

		bank_base[i + 1] = memory_region(REGION_USER1) + init;
		memory_set_bankptr(i + 1, bank_base[i + 1]);
	}
}

static READ16_HANDLER(a2600_read_input_port) {
	int i;

	switch( offset ) {
	case 0:
		switch ( readinputport(9) / CATEGORY_SELECT ) {
		case 0x00:	/* Joystick */
			return TIA_INPUT_PORT_ALWAYS_OFF;
		case 0x01:	/* Paddle */
			return readinputport(0);
		case 0x04:	/* Keypad */
			for ( i = 0; i < 4; i++ ) {
				if ( ! ( ( keypad_left_column >> i ) & 0x01 ) ) {
					if ( ( readinputport(10) >> 3*i ) & 0x01 ) {
						return TIA_INPUT_PORT_ALWAYS_ON;
					} else {
						return TIA_INPUT_PORT_ALWAYS_OFF;
					}
				}
			}
			return TIA_INPUT_PORT_ALWAYS_ON;
		case 0x10:	/* Boostergrip joystick */
			return ( readinputport(4) & 0x40 ) ? TIA_INPUT_PORT_ALWAYS_OFF : TIA_INPUT_PORT_ALWAYS_ON;
		default:
			return TIA_INPUT_PORT_ALWAYS_OFF;
		}
		break;
	case 1:
		switch ( readinputport(9) / CATEGORY_SELECT ) {
		case 0x00:	/* Joystick */
			return TIA_INPUT_PORT_ALWAYS_OFF;
		case 0x01:	/* Paddle */
			return readinputport(1);
		case 0x04:	/* Keypad */
			for ( i = 0; i < 4; i++ ) {
				if ( ! ( ( keypad_left_column >> i ) & 0x01 ) ) {
					if ( ( readinputport(10) >> 3*i ) & 0x02 ) {
						return TIA_INPUT_PORT_ALWAYS_ON;
					} else {
						return TIA_INPUT_PORT_ALWAYS_OFF;
					}
				}
			}
			return TIA_INPUT_PORT_ALWAYS_ON;
		case 0x10:	/* Joystick w/Boostergrip */
			return ( readinputport(4) & 0x20 ) ? TIA_INPUT_PORT_ALWAYS_OFF : TIA_INPUT_PORT_ALWAYS_ON;
		default:
			return TIA_INPUT_PORT_ALWAYS_OFF;
		}
		break;
	case 2:
		switch ( readinputport(9) % CATEGORY_SELECT ) {
		case 0x00:	/* Joystick */
			return TIA_INPUT_PORT_ALWAYS_OFF;
		case 0x01:	/* Paddle */
			return readinputport(2);
		case 0x04:	/* Keypad */
			for ( i = 0; i < 4; i++ ) {
				if ( ! ( ( keypad_right_column >> i ) & 0x01 ) ) {
					if ( ( readinputport(11) >> 3*i ) & 0x01 ) {
						return TIA_INPUT_PORT_ALWAYS_ON;
					} else {
						return TIA_INPUT_PORT_ALWAYS_OFF;
					}
				}
			}
			return TIA_INPUT_PORT_ALWAYS_ON;
		case 0x10:	/* Joystick w/Boostergrip */
			return ( readinputport(5) & 0x40 ) ? TIA_INPUT_PORT_ALWAYS_ON : TIA_INPUT_PORT_ALWAYS_OFF;
		default:
			return TIA_INPUT_PORT_ALWAYS_OFF;
		}
		break;
	case 3:
		switch ( readinputport(9) % CATEGORY_SELECT ) {
		case 0x00:	/* Joystick */
			return TIA_INPUT_PORT_ALWAYS_OFF;
		case 0x01:	/* Paddle */
			return readinputport(3);
		case 0x04:	/* Keypad */
			for ( i = 0; i < 4; i++ ) {
				if ( ! ( ( keypad_right_column >> i ) & 0x01 ) ) {
					if ( ( readinputport(11) >> 3*i ) & 0x02 ) {
						return TIA_INPUT_PORT_ALWAYS_ON;
					} else {
						return TIA_INPUT_PORT_ALWAYS_OFF;
					}
				}
			}
			return TIA_INPUT_PORT_ALWAYS_ON;
		case 0x10:	/* Joystick w/Boostergrip */
			return ( readinputport(5) & 0x20 ) ? TIA_INPUT_PORT_ALWAYS_ON : TIA_INPUT_PORT_ALWAYS_OFF;
		default:
			return TIA_INPUT_PORT_ALWAYS_OFF;
		}
		break;
	case 4:
		switch ( readinputport(9) / CATEGORY_SELECT ) {
		case 0x00:	/* Joystick */
		case 0x10:	/* Joystick w/Boostergrip */
			return readinputport(4);
		case 0x01:	/* Paddle */
			return 0xff;
		case 0x02:	/* Driving */
			return readinputport(4) << 3;
		case 0x04:	/* Keypad */
			for ( i = 0; i < 4; i++ ) {
				if ( ! ( ( keypad_left_column >> i ) & 0x01 ) ) {
					if ( ( readinputport(10) >> 3*i ) & 0x04 ) {
						return 0xff;
					} else {
						return 0x00;
					}
				}
			}
			return 0xff;
		default:
			return 0xff;
		}
		break;
	case 5:
		switch ( readinputport(9) % CATEGORY_SELECT ) {
		case 0x00:	/* Joystick */
		case 0x10:	/* Joystick w/Boostergrip */
			return readinputport(5);
		case 0x01:	/* Paddle */
			return 0xff;
		case 0x02:	/* Driving */
			return readinputport(5) << 3;
		case 0x04:	/* Keypad */
			for ( i = 0; i < 4; i++ ) {
				if ( ! ( ( keypad_right_column >> i ) & 0x01 ) ) {
					if ( ( readinputport(11) >> 3*i ) & 0x04 ) {
						return 0xff;
					} else {
						return 0x00;
					}
				}
			}
			return 0xff;
		default:
			return 0xff;
		}
	}
	return TIA_INPUT_PORT_ALWAYS_OFF;
}

/* There are a few games that do an LDA ($80-$FF),Y instruction.
   The contents off the databus then depend on whatever was read
   from the RAM. To do this really properly the 6502 core would
   need to keep track of the last databus contents so we can query
   that. For now this is a quick hack to determine that value anyway.
   Examples:
   Q-Bert's Qubes (NTSC,F6) at 0x1594
   Berzerk at 0xF093.
*/
static READ8_HANDLER(a2600_get_databus_contents) {
	UINT16	last_address, prev_address;
	UINT8	last_byte, prev_byte;

	last_address = activecpu_get_pc() - 1;
	if ( ! ( last_address & 0x1080 ) ) {
		return offset;
	}
	last_byte = program_read_byte_8( last_address );
	if ( last_byte < 0x80 || last_byte == 0xFF ) {
		return last_byte;
	}
	prev_address = last_address - 1;
	if ( ! ( prev_address & 0x1080 ) ) {
		return last_byte;
	}
	prev_byte = program_read_byte_8( prev_address );
	if ( prev_byte == 0xB1 ) {	/* LDA (XX),Y */
		return program_read_byte_8( last_byte + 1 );
	}
	return last_byte;
}

static struct tia_interface tia_interface =
{
	a2600_read_input_port,
	a2600_get_databus_contents
};

static void setup_riot(int dummy) {
	/* It appears the 6532 is running in 64 cycle mode with some random timer value
	   on startup. This is not verified against real hardware yet! */
	cpuintrf_push_context(0);
//	r6532_0_w( 0x14, 100 /*( mame_rand(Machine) & 0x7F ) | 0x60 */);
	cpuintrf_pop_context();
}

static MACHINE_START( a2600 )
{

	int mode = 0xFF;
	int chip = 0xFF;

	extra_RAM = new_memory_region( machine, REGION_USER2, 0x8600, ROM_REQUIRED );

	r6532_init(0, &r6532_interface);

	timer_set( 0.0, 0, setup_riot );

	if ( !strcmp( Machine->gamedrv->name, "a2600p" ) ) {
		tia_init_pal( &tia_interface );
	} else {
		tia_init( &tia_interface );
	}

	/* auto-detect bank mode */

	switch (cart_size)
	{
	case 0x800:
		mode = mode2K;
		break;
	case 0x1000:
		mode = mode4K;
		break;
	case 0x2000:
		mode = mode8K;
		break;
	case 0x3000:
		mode = mode12;
		break;
	case 0x4000:
		mode = mode16;
		break;
	case 0x8000:
		mode = mode32;
		break;
	case 0x80000:
		mode = modeTV;
		break;
	}
	if (detect_modeDC()) mode = modeDC;
	if (detect_mode3E()) mode = mode3E;
	if (detect_modeAV()) mode = modeAV;
	if (detect_modeSS()) mode = modeSS;
	if (detect_modePB()) mode = modePB;
	if (detect_modeCV()) mode = modeCV;
	if (detect_modeMN()) mode = modeMN;
	if (detect_modeUA()) mode = modeUA;
	if (detect_8K_modeTV()) mode = modeTV;
	if (detect_32K_modeTV()) mode = modeTV;

	/* auto-detect super chip */

	chip = 0;

	if (cart_size == 0x2000 || cart_size == 0x4000 || cart_size == 0x8000)
	{
		chip = detect_super_chip();
	}

	/* Super chip games:
	   dig dig, crystal castles, millipede, stargate, defender ii, jr. Pac Man,
	   desert falcon, dark chambers, super football, sprintmaster, fatal run,
	   off the wall, shooting arcade, secret quest, radar lock, save mary, klax
	*/

	/* set up ROM banks */

	switch (mode)
	{
	case mode2K:
		install_banks(2, 0x0000);
		break;

	case mode4K:
		install_banks(1, 0x0000);
		break;

	case mode8K:
		install_banks(1, 0x1000);
		break;

	case mode12:
		install_banks(1, 0x2000);
		break;

	case mode16:
		install_banks(1, 0x0000);
		break;

	case mode32:
		install_banks(1, 0x7000);
		break;

	case modeAV:
		install_banks(1, 0x0000);
		break;

	case modePB:
		install_banks(4, 0x1c00);
		break;

	case modeTV:
		install_banks(2, cart_size - 0x800);
		number_banks = cart_size / 0x800;
		break;

	case modeUA:
		install_banks(1, 0x1000);
		break;

	case modeMN:
		install_banks(2, 0x3800);
		break;

	case modeDC:
		install_banks(1, 0x1000 * current_bank);
		break;

	case modeCV:
		install_banks(2, 0x0000);
		break;

	case mode3E:
		install_banks(2, cart_size - 0x800);
		number_banks = cart_size / 0x800;
		mode3E_ram_enabled = 0;
		break;

	case modeSS:
		install_banks(2, 0x0000);
		break;
	}

	/* set up bank counter */

	if (mode == modeDC)
	{
		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x1fec, 0x1fec, 0, 0, current_bank_r);
	}

	/* set up bank switch registers */

	switch (mode)
	{
	case mode8K:
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x1ff8, 0x1ff9, 0, 0, mode8K_switch_w);
		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x1ff8, 0x1ff9, 0, 0, mode8K_switch_r);
		break;

	case mode12:
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x1ff8, 0x1ffa, 0, 0, mode12_switch_w);
		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x1ff8, 0x1ffa, 0, 0, mode12_switch_r);
		break;

	case mode16:
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x1ff6, 0x1ff9, 0, 0, mode16_switch_w);
		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x1ff6, 0x1ff9, 0, 0, mode16_switch_r);
		break;

	case mode32:
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x1ff4, 0x1ffb, 0, 0, mode32_switch_w);
		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x1ff4, 0x1ffb, 0, 0, mode32_switch_r);
		break;

	case modePB:
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x1fe0, 0x1ff8, 0, 0, modePB_switch_w);
		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x1fe0, 0x1ff8, 0, 0, modePB_switch_r);
		break;

	case modeTV:
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x00, 0x3f, 0, 0, modeTV_switch_w);
		break;

	case modeUA:
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x200, 0x27f, 0, 0, modeUA_switch_w);
		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x200, 0x27f, 0, 0, modeUA_switch_r);
		break;

	case modeMN:
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x1fe0, 0x1fe7, 0, 0, modeMN_switch_w);
		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x1fe0, 0x1fe7, 0, 0, modeMN_switch_r);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x1fe8, 0x1feb, 0, 0, modeMN_RAM_switch_w);
		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x1fe8, 0x1feb, 0, 0, modeMN_RAM_switch_r);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x1800, 0x18ff, 0, 0, MWA8_BANK9);
		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x1900, 0x19ff, 0, 0, MRA8_BANK9);
		memory_set_bankptr( 9, extra_RAM + 4 * 256 );
		break;

	case modeDC:
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x1ff0, 0x1ff0, 0, 0, modeDC_switch_w);
		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x1ff0, 0x1ff0, 0, 0, modeDC_switch_r);
		break;

	case modeAV:
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x01fe, 0x01fe, 0, 0, modeAV_switch_w);
		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x01fe, 0x01fe, 0, 0, modeAV_switch_r);
		break;

	case mode3E:
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x3e, 0x3e, 0, 0, mode3E_RAM_switch_w);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x3f, 0x3f, 0, 0, mode3E_switch_w);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x1400, 0x15ff, 0, 0, mode3E_RAM_w);
		break;

	case modeSS:
		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x1000, 0x1fff, 0, 0, modeSS_r);
		bank_base[1] = extra_RAM + 2 * 0x800;
		bank_base[2] = CART;
		memory_set_bankptr( 1, bank_base[1] );
		memory_set_bankptr( 2, bank_base[2] );
		modeSS_write_enabled = 0;
		modeSS_write_pending = 0;
		memory_set_opbase_handler( 0, modeSS_opbase );
		/* Already start the motor of the cassette for the user */
		cassette_change_state( image_from_devtype_and_index( IO_CASSETTE, 0 ), CASSETTE_MOTOR_ENABLED, CASSETTE_MOTOR_DISABLED );
		break;
	}

	/* set up extra RAM */

	if (mode == mode12)
	{
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x1000, 0x10ff, 0, 0, MWA8_BANK9);
		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x1100, 0x11ff, 0, 0, MRA8_BANK9);

		memory_set_bankptr(9, extra_RAM);
	}

	if (mode == modeCV)
	{
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x1400, 0x17ff, 0, 0, MWA8_BANK9);
		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x1000, 0x13ff, 0, 0, MRA8_BANK9);

		memory_set_bankptr(9, extra_RAM);
	}

	if (chip)
	{
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x1000, 0x107f, 0, 0, MWA8_BANK9);
		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x1080, 0x10ff, 0, 0, MRA8_BANK9);

		memory_set_bankptr(9, extra_RAM);
	}
}


INPUT_PORTS_START( a2600 )

	PORT_START /* [0] */
	PORT_BIT( 0xff, 0x80, IPT_PADDLE) PORT_SENSITIVITY(40) PORT_KEYDELTA(10) PORT_CENTERDELTA(0) PORT_MINMAX(0,255) PORT_CATEGORY(11) PORT_PLAYER(1) PORT_REVERSE
	PORT_START /* [1] */
	PORT_BIT( 0xff, 0x80, IPT_PADDLE) PORT_SENSITIVITY(40) PORT_KEYDELTA(10) PORT_CENTERDELTA(0) PORT_MINMAX(0,255) PORT_CATEGORY(11) PORT_PLAYER(3) PORT_REVERSE
	PORT_START /* [2] */
	PORT_BIT( 0xff, 0x80, IPT_PADDLE) PORT_SENSITIVITY(40) PORT_KEYDELTA(10) PORT_CENTERDELTA(0) PORT_MINMAX(0,255) PORT_CATEGORY(21) PORT_PLAYER(2) PORT_REVERSE
	PORT_START /* [3] */
	PORT_BIT( 0xff, 0x80, IPT_PADDLE) PORT_SENSITIVITY(40) PORT_KEYDELTA(10) PORT_CENTERDELTA(0) PORT_MINMAX(0,255) PORT_CATEGORY(21) PORT_PLAYER(4) PORT_REVERSE PORT_CODE_DEC(KEYCODE_4_PAD) PORT_CODE_INC(KEYCODE_6_PAD)

	PORT_START /* [4] */
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_CATEGORY(12) PORT_PLAYER(1)
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3) PORT_CATEGORY(10) PORT_PLAYER(1)
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2) PORT_CATEGORY(10) PORT_PLAYER(1)
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_CATEGORY(10) PORT_PLAYER(1)

	PORT_START /* [5] */
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_CATEGORY(22) PORT_PLAYER(2)
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3) PORT_CATEGORY(20) PORT_PLAYER(2)
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2) PORT_CATEGORY(20) PORT_PLAYER(2)
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_CATEGORY(20) PORT_PLAYER(2)

	PORT_START /* [6] SWCHA joystick */
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP) PORT_CATEGORY(20) PORT_PLAYER(2)
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN) PORT_CATEGORY(20) PORT_PLAYER(2)
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT) PORT_CATEGORY(20) PORT_PLAYER(2)
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT) PORT_CATEGORY(20) PORT_PLAYER(2)
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP) PORT_CATEGORY(10) PORT_PLAYER(1)
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN) PORT_CATEGORY(10) PORT_PLAYER(1)
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT) PORT_CATEGORY(10) PORT_PLAYER(1)
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT) PORT_CATEGORY(10) PORT_PLAYER(1)

	PORT_START /* [7] SWCHA paddles */
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_CATEGORY(21) PORT_PLAYER(4)
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_CATEGORY(21) PORT_PLAYER(2)
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_CATEGORY(11) PORT_PLAYER(3)
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_CATEGORY(11) PORT_PLAYER(1)

	PORT_START /* [8] SWCHB */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Reset Game") PORT_CODE(KEYCODE_2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START) PORT_NAME("Select Game")
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x08, 0x08, "TV Type" ) PORT_CODE(KEYCODE_C) PORT_TOGGLE
	PORT_DIPSETTING(    0x08, "Color" )
	PORT_DIPSETTING(    0x00, "B&W" )
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x40, 0x00, "Left Diff. Switch" ) PORT_CODE(KEYCODE_3) PORT_TOGGLE
	PORT_DIPSETTING(    0x40, "A" )
	PORT_DIPSETTING(    0x00, "B" )
	PORT_DIPNAME( 0x80, 0x00, "Right Diff. Switch" ) PORT_CODE(KEYCODE_4) PORT_TOGGLE
	PORT_DIPSETTING(    0x80, "A" )
	PORT_DIPSETTING(    0x00, "B" )

	PORT_START /* [9] */
	PORT_CATEGORY_CLASS( 0x1f00, 0x00, "Left Controller" )
	PORT_CATEGORY_ITEM(    0x0000, DEF_STR( Joystick ), 10 )
	PORT_CATEGORY_ITEM(    0x0100, "Paddles", 11 )
	PORT_CATEGORY_ITEM(    0x0200, "Driving", 12 )
	PORT_CATEGORY_ITEM(    0x0400, "Keypad", 13 )
	//PORT_CATEGORY_ITEM(    0x0800, "Lightgun", 14 )
	PORT_CATEGORY_ITEM(    0x1000, "Joystick w/Boostergrip", 10 )
	PORT_CATEGORY_CLASS( 0x003f, 0x00, "Right Controller" )
	PORT_CATEGORY_ITEM(    0x0000, DEF_STR( Joystick ), 20 )
	PORT_CATEGORY_ITEM(    0x0001, "Paddles", 21 )
	PORT_CATEGORY_ITEM(    0x0002, "Driving", 22 )
	PORT_CATEGORY_ITEM(    0x0004, "Keypad", 23 )
	//PORT_CATEGORY_ITEM(    0x0008, "Lightgun", 24 )
	PORT_CATEGORY_ITEM(    0x0010, "Joystick w/Boostergrip", 20 )
	PORT_CATEGORY_ITEM(    0x0020, "KidVid Voice Module", 26 )

	PORT_START	/* [10] left keypad */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CATEGORY(13) PORT_NAME("left 1") PORT_CODE(KEYCODE_7_PAD)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CATEGORY(13) PORT_NAME("left 2") PORT_CODE(KEYCODE_8_PAD)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CATEGORY(13) PORT_NAME("left 3") PORT_CODE(KEYCODE_9_PAD)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CATEGORY(13) PORT_NAME("left 4") PORT_CODE(KEYCODE_4_PAD)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CATEGORY(13) PORT_NAME("left 5") PORT_CODE(KEYCODE_5_PAD)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CATEGORY(13) PORT_NAME("left 6") PORT_CODE(KEYCODE_6_PAD)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CATEGORY(13) PORT_NAME("left 7") PORT_CODE(KEYCODE_1_PAD)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CATEGORY(13) PORT_NAME("left 8") PORT_CODE(KEYCODE_2_PAD)
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CATEGORY(13) PORT_NAME("left 9") PORT_CODE(KEYCODE_3_PAD)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CATEGORY(13) PORT_NAME("left *") PORT_CODE(KEYCODE_0_PAD)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CATEGORY(13) PORT_NAME("left 0") PORT_CODE(KEYCODE_DEL_PAD)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CATEGORY(13) PORT_NAME("left #") PORT_CODE(KEYCODE_ENTER_PAD)

	PORT_START  /* [11] right keypad */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CATEGORY(23) PORT_NAME("right 1") PORT_CODE(KEYCODE_5)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CATEGORY(23) PORT_NAME("right 2") PORT_CODE(KEYCODE_6)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CATEGORY(23) PORT_NAME("right 3") PORT_CODE(KEYCODE_7)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CATEGORY(23) PORT_NAME("right 4") PORT_CODE(KEYCODE_R)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CATEGORY(23) PORT_NAME("right 5") PORT_CODE(KEYCODE_T)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CATEGORY(23) PORT_NAME("right 6") PORT_CODE(KEYCODE_Y)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CATEGORY(23) PORT_NAME("right 7") PORT_CODE(KEYCODE_F)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CATEGORY(23) PORT_NAME("right 8") PORT_CODE(KEYCODE_G)
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CATEGORY(23) PORT_NAME("right 9") PORT_CODE(KEYCODE_H)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CATEGORY(23) PORT_NAME("right *") PORT_CODE(KEYCODE_V)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CATEGORY(23) PORT_NAME("right 0") PORT_CODE(KEYCODE_B)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CATEGORY(23) PORT_NAME("right #") PORT_CODE(KEYCODE_N)

	PORT_START	/* [12] left driving controller */
	PORT_BIT( 0xff, 0x00, IPT_TRACKBALL_X ) PORT_CATEGORY(12) PORT_SENSITIVITY(120) PORT_KEYDELTA(10) PORT_PLAYER(1)

	PORT_START	/* [13] right driving controller */
	PORT_BIT( 0xff, 0x00, IPT_TRACKBALL_X ) PORT_CATEGORY(22) PORT_SENSITIVITY(120) PORT_KEYDELTA(10) PORT_PLAYER(2)

INPUT_PORTS_END


static MACHINE_DRIVER_START( a2600 )
	/* basic machine hardware */
	MDRV_CPU_ADD(M6502, MASTER_CLOCK_NTSC / 3)	/* actually M6507 */
	MDRV_CPU_PROGRAM_MAP(a2600_mem, 0)

	MDRV_MACHINE_START(a2600)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_ADD("main",0)
	MDRV_SCREEN_RAW_PARAMS( MASTER_CLOCK_NTSC, 228, 26, 26 + 160 + 16, 262, 24 , 24 + 192 + 32 )
	MDRV_PALETTE_LENGTH(128)
	MDRV_PALETTE_INIT(tia_NTSC)

	MDRV_VIDEO_START(tia)
	MDRV_VIDEO_UPDATE(tia)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(TIA, MASTER_CLOCK_NTSC/114)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.90)
	MDRV_SOUND_ADD(WAVE, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.10)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( a2600p )
	/* basic machine hardware */
	MDRV_CPU_ADD(M6502, MASTER_CLOCK_PAL / 3)    /* actually M6507 */
	MDRV_CPU_PROGRAM_MAP(a2600_mem, 0)

	MDRV_MACHINE_START(a2600)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_ADD("main",0)
	MDRV_SCREEN_RAW_PARAMS( MASTER_CLOCK_PAL, 228, 26, 26 + 160 + 16, 312, 32, 32 + 228 + 32 )
	MDRV_PALETTE_LENGTH(128)
	MDRV_PALETTE_INIT(tia_PAL)

	MDRV_VIDEO_START(tia)
	MDRV_VIDEO_UPDATE(tia)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(TIA, MASTER_CLOCK_PAL/114)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.90)
	MDRV_SOUND_ADD(WAVE, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.10)
MACHINE_DRIVER_END


ROM_START( a2600 )
	ROM_REGION( 0x2000, REGION_CPU1, 0 )
	ROM_FILL( 0x0000, 0x2000, 0xFF )
	ROM_REGION( 0x80000, REGION_USER1, 0 )
	ROM_FILL( 0x00000, 0x80000, 0xFF )
ROM_END

#define rom_a2600p rom_a2600

static void a2600_cartslot_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cartslot */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 1; break;
		case DEVINFO_INT_MUST_BE_LOADED:				info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_LOAD:							info->load = device_load_a2600_cart; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "bin,a26"); break;

		default:										cartslot_device_getinfo(devclass, state, info); break;
	}
}

static void a2600_cassette_getinfo( const device_class *devclass, UINT32 state, union devinfo *info ) {
	switch( state ) {
	case DEVINFO_INT_COUNT:
		info->i = 1;
		break;
	case DEVINFO_INT_WRITEABLE:
		info->i = 0;
		break;
	case DEVINFO_INT_CREATABLE:
		info->i = 0;
		break;
	case DEVINFO_INT_CASSETTE_DEFAULT_STATE:
		info->i = CASSETTE_PLAY | CASSETTE_MOTOR_DISABLED | CASSETTE_SPEAKER_ENABLED;
		break;
	case DEVINFO_PTR_CASSETTE_FORMATS:
		info->p = (void *)a26_cassette_formats;
		break;
	default:
		cassette_device_getinfo( devclass, state, info );
		break;
	}
}

SYSTEM_CONFIG_START(a2600)
	CONFIG_DEVICE(a2600_cartslot_getinfo)
	CONFIG_DEVICE(a2600_cassette_getinfo)
SYSTEM_CONFIG_END


/*    YEAR	NAME	PARENT	COMPAT	MACHINE	INPUT	INIT	CONFIG	COMPANY		FULLNAME */
CONS( 1977,	a2600,	0,		0,		a2600,	a2600,	0,		a2600,	"Atari",	"Atari 2600 (NTSC)" , 0)
CONS( 1978,	a2600p,	a2600,	0,		a2600p,	a2600,	0,		a2600,  "Atari",    "Atari 2600 (PAL)", 0)

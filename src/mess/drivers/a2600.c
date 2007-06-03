/***************************************************************************

  Atari VCS 2600 driver

  On a real TV it appears there's some space between the pixels on the
  display. The current TIA implementation does not display these spaces
  between the pixels.

***************************************************************************/

#include "driver.h"
#include "machine/6532riot.h"
#include "cpu/m6502/m6502.h"
#include "sound/tiaintf.h"
#include "devices/cartslot.h"
#include "devices/cassette.h"
//#include "formats/a26_cas.h"
#include "video/tia.h"
#include "inputx.h"
#include "zlib.h"

#define CART memory_region(REGION_USER1)

#define MASTER_CLOCK_NTSC	3579575
#define MASTER_CLOCK_PAL	3546894

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


static const UINT32 games[][2] =
{
	{ 0x91b8f1b2, modeAV }, // Decathlon
	{ 0x2452adab, modeAV }, // Decathlon PAL
	{ 0xe127c012, modeAV }, // Robot Tank
	{ 0xcded5569, modeAV }, // Robot Tank PAL
	{ 0x600e7c77, modeAV }, // Space Shuttle PAL (Only the PAL version uses Activision style banking, the US release is regular 8K banking)
	{ 0xb60ab310, modeAV }, // Thwocker Prototype
	{ 0x3ba0d9bf, modePB }, // Frogger II
	{ 0x09cdd3ea, modePB }, // Frogger II PAL
	{ 0x0d78e8a9, modePB }, // Gyruss
	{ 0xba14b37b, modePB }, // Gyruss PAL
	{ 0x34d3ffc8, modePB }, // James Bond 007
	{ 0x59b96db3, modePB }, // Lord of the Rings Prototype
	{ 0xe680a1c9, modePB }, // Montezuma's Revenge
	{ 0xb687a91f, modePB }, // Montezuma's Revenge (trained)
	{ 0x044735b9, modePB }, // Mr Do's Castle
	{ 0x7d287f20, modePB }, // Popeye
	{ 0x742ac749, modePB }, // Popeye PAL
	{ 0xa87be8fd, modePB }, // Q-bert's Qubes
	{ 0xd9f499c5, modePB }, // Q-bert's Qubes
	{ 0xde97103d, modePB }, // Super Cobra
	{ 0x380d78b3, modePB }, // Super Cobra PAL
	{ 0x0886a55d, modePB }, // Star Wars Death Star Battle
	{ 0x2a2bd248, modePB }, // Star Wars Death Star Battle PAL
	{ 0xd11E05fe, modePB }, // Star Wars Ewok Adventure Prototype PAL
	{ 0x65c31ca4, modePB }, // Star Wars Arcade Game
	{ 0x273bda48, modePB }, // Star Wars Arcade Game PAL
	{ 0x47efd61d, modePB }, // Star Wars Arcade Game Prototype
	{ 0xfd8c81e5, modePB }, // Tooth Protectors
	{ 0xec959bf2, modePB }, // Tutankham
	{ 0x8fbe2b84, modePB }, // Tutankham PAL
	{ 0xc5baae6f, modePB }, // Tutankham Zelda Hack
/*
	{ 0x1f95351a, modeTV }, // Espial
	{ 0x34b80a97, modeTV }, // Espial PAL
	{ 0xbd08d915, modeTV }, // Miner 2049er
	{ 0xbfa477cd, modeTV }, // Miner 2049er Volume II
	{ 0x71e814e9, modeTV }, // Miner 2049er Volume II
	{ 0xdb376663, modeTV }, // Polaris
	{ 0x25b78f89, modeTV }, // Polaris
	{ 0x71ecefaf, modeTV }, // Polaris
	{ 0xc820bd75, modeTV }, // River Patrol
	{ 0xdd183a4f, modeTV }, // Springer
*/
	{ 0xAAFF3AB1, mode3E }, // not BoulderDash
	{ 0x8cdfb7ec, mode3E }, // not BoulderDash PAL
/*
	{ 0xb53b33f1, modeUA }, // Funky Fish Prototype
	{ 0x35589cec, modeUA }, // Pleiads Prototype
*/
/*
	{ 0xdf2bc303, modeMN }, // Bump 'n' Jump
	{ 0xc183fbbc, modeMN }, // Burgertime
	{ 0x66f1849e, modeMN }, // Burgertime (E7)
	{ 0x0603e177, modeMN }, // Masters of the Universe
*/

/*	{ 0x14f126c0, modeCV }, // Magicard
	{ 0x34b0b5c2, modeCV }  // Video Life
*/
	{ 0xc3a3f073, modeSS }, // Starpath Supercharger BIOS
};

static int detect_modeCV(void)
{
	int i,numfound = 0;
	unsigned char TVsignature1[3] = { 0x9d, 0xff, 0xf3 };
	unsigned char TVsignature2[3] = { 0x99, 0x00, 0xf4 };
	if (cart_size == 0x0800 || cart_size == 0x1000)
	{
		for (i = 0; i < cart_size - sizeof TVsignature1; i++)
		{
			if (!memcmp(&CART[i], TVsignature1,sizeof TVsignature1))
			{
				numfound = 1;
			}
			if (!memcmp(&CART[i], TVsignature2,sizeof TVsignature2))
			{
				numfound = 1;
			}
		}
	}
	if (numfound) return 1;
	return 0;
}

static int detect_modeMN(void)
{
	int i,numfound = 0;
	unsigned char TVsignature[3] = { 0xad, 0xe5, 0xff };
	if (cart_size == 0x4000)
	{
		for (i = 0; i < cart_size - sizeof TVsignature; i++)
		{
			if (!memcmp(&CART[i], TVsignature,sizeof TVsignature))
			{
				numfound = 1;
			}
		}
	}
	if (numfound) return 1;
	return 0;
}

static int detect_modeUA(void)
{
	int i,numfound = 0;
	unsigned char TVsignature[3] = { 0x8d, 0x40, 0x02 };
	if (cart_size == 0x2000)
	{
		for (i = 0; i < cart_size - sizeof TVsignature; i++)
		{
			if (!memcmp(&CART[i], TVsignature,sizeof TVsignature))
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
	unsigned char TVsignature1[4] = { 0xa9, 0x01, 0x85, 0x3f };
	unsigned char TVsignature2[4] = { 0xa9, 0x02, 0x85, 0x3f };
	// have to look for two signatures because 'not boulderdash' gives false positive otherwise
	if (cart_size == 0x2000)
	{
		for (i = 0; i < cart_size - sizeof TVsignature1; i++)
		{
			if (!memcmp(&CART[i], TVsignature1,sizeof TVsignature1))
			{
				numfound |= 0x01;
			}
			if (!memcmp(&CART[i], TVsignature2,sizeof TVsignature2))
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
	unsigned char TVsignature[4] = { 0xa9, 0x0e, 0x85, 0x3f };
	if (cart_size >= 0x8000)
	{
		for (i = 0; i < cart_size - sizeof TVsignature; i++)
		{
			if (!memcmp(&CART[i], TVsignature,sizeof TVsignature))
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
	int i;

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
	if ( readinputport(9) / 16 == 0x04 ) {
		keypad_left_column = data / 16;
	}

	/* Right controller port */
	if ( readinputport(9) % 16 == 0x04 ) {
		keypad_right_column = data & 0x0F;
	}
}

static  READ8_HANDLER( switch_A_r )
{
	UINT8 val = 0;

	/* Left controller port */
	switch( readinputport(9) / 16 ) {
	case 0x00:  /* Joystick */
		val |= readinputport(6) & 0xF0;
		break;
	case 0x01:  /* Paddle */
		val |= readinputport(7) & 0xF0;
		break;
	default:
		val |= 0xF0;
		break;
	}

	/* Right controller port */
	switch( readinputport(9) % 16 ) {
	case 0x00:	/* Joystick */
		val |= readinputport(6) & 0x0F;
		break;
	case 0x01:	/* Paddle */
		val |= readinputport(7) & 0x0F;
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
		switch ( readinputport(9) / 16 ) {
		case 0x00:	/* Joystick */
			return TIA_INPUT_PORT_ALWAYS_OFF;
		case 0x01:	/* Paddle */
			return readinputport(0);
		case 0x04:	/* Keypad */
			for ( i = 0; i < 4; i++ ) {
				if ( ! ( ( keypad_left_column >> i ) & 0x01 ) ) {
					if ( ( readinputport(12) >> 3*i ) & 0x01 ) {
						return TIA_INPUT_PORT_ALWAYS_OFF;
					} else {
						return TIA_INPUT_PORT_ALWAYS_ON;
					}
				}
			}
		default:
			return TIA_INPUT_PORT_ALWAYS_OFF;
		}
		break;
	case 1:
		switch ( readinputport(9) / 16 ) {
		case 0x00:	/* Joystick */
			return TIA_INPUT_PORT_ALWAYS_OFF;
		case 0x01:	/* Paddle */
			return readinputport(1);
		case 0x04:	/* Keypad */
			for ( i = 0; i < 4; i++ ) {
				if ( ! ( ( keypad_left_column >> i ) & 0x01 ) ) {
					if ( ( readinputport(12) >> 3*i ) & 0x02 ) {
						return TIA_INPUT_PORT_ALWAYS_OFF;
					} else {
						return TIA_INPUT_PORT_ALWAYS_ON;
					}
				}
			}
		default:
			return TIA_INPUT_PORT_ALWAYS_OFF;
		}
		break;
	case 2:
		switch ( readinputport(9) & 0x0f ) {
		case 0x00:	/* Joystick */
			return TIA_INPUT_PORT_ALWAYS_OFF;
		case 0x01:	/* Paddle */
			return readinputport(2);
		case 0x04:	/* Keypad */
			for ( i = 0; i < 4; i++ ) {
				if ( ! ( ( keypad_right_column >> i ) & 0x01 ) ) {
				}
			}
		default:
			return TIA_INPUT_PORT_ALWAYS_OFF;
		}
		break;
	case 3:
		switch ( readinputport(9) & 0x0f ) {
		case 0x00:	/* Joystick */
			return TIA_INPUT_PORT_ALWAYS_OFF;
		case 0x01:	/* Paddle */
			return readinputport(3);
		case 0x04:	/* Keypad */
			for ( i = 0; i < 4; i++ ) {
				if ( ! ( ( keypad_right_column >> i ) & 0x01 ) ) {
				}
			}
		default:
			return TIA_INPUT_PORT_ALWAYS_OFF;
		}
		break;
	case 4:
		switch ( readinputport(9) / 16 ) {
		case 0x00:	/* Joystick */
			return readinputport(4);
		case 0x04:	/* Keypad */
			return 0xff;
		case 0x01:	/* Paddle */
			for ( i = 0; i < 4; i++ ) {
				if ( ! ( ( keypad_left_column >> i ) & 0x01 ) ) {
					if ( ( readinputport(12) >> 3*i ) & 0x04 ) {
						return TIA_INPUT_PORT_ALWAYS_OFF;
					} else {
						return TIA_INPUT_PORT_ALWAYS_ON;
					}
				}
			}
		default:
			return 0xff;
		}
		break;
	case 5:
		switch ( readinputport(9) & 0x0f ) {
		case 0x00:	/* Joystick */
			return readinputport(5);
		case 0x04:	/* Keypad */
			return 0xff;
		case 0x01:	/* Paddle */
			for ( i = 0; i < 4; i++ ) {
				if ( ! ( ( keypad_right_column >> i ) & 0x01 ) ) {
				}
			}
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
	r6532_0_w( 0x16, ( mame_rand(Machine) & 0x7F ) | 0x60 );
	cpuintrf_pop_context();
}

static MACHINE_START( a2600 )
{
	/* NPW 6-Mar-2006 - The MAME core changed, and now I cannot use readinputport() here properly */
	int mode = 0xFF; /* readinputport(10); */
	int chip = 0xFF; /* readinputport(11); */

	extra_RAM = new_memory_region( machine, REGION_USER2, 0x8600, ROM_REQUIRED );

	r6532_init(0, &r6532_interface);

	timer_set( 0.0, 0, setup_riot );

	if ( !strcmp( Machine->gamedrv->name, "a2600p" ) ) {
		tia_init_pal( &tia_interface );
	} else {
		tia_init( &tia_interface );
	}

	/* auto-detect bank mode */

	if (mode == 0xff)
	{
		UINT32 crc = crc32(0, CART, cart_size);

		int i;

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
		case 0x10000:
			mode = modeDC;
			break;
		case 0x80000:
			mode = modeTV;
			break;
		}

		if (detect_modeCV())
		{
			mode = modeCV;
		}
		if (detect_modeMN())
		{
			mode = modeMN;
		}
		if (detect_modeUA())
		{
			mode = modeUA;
		}
		if (detect_8K_modeTV())
		{
			mode = modeTV;
		}
		if (detect_32K_modeTV())
		{
			mode = modeTV;
		}
		for (i = 0; 8 * i < sizeof games; i++)
		{
			if (games[i][0] == crc)
			{
				mode = games[i][1];
			}
		}
	}

	/* auto-detect super chip */

	if (chip == 0xff)
	{
		UINT32 crc = crc32(0, CART, cart_size);

		chip = 0;

		if (cart_size == 0x2000 || cart_size == 0x4000 || cart_size == 0x8000)
		{
			chip = detect_super_chip();
		}

		if (crc == 0xee7b80d1) chip = 1; // Dig Dug
		if (crc == 0xa09779ea) chip = 1; // Off the Wall
		if (crc == 0x861c4aca) chip = 1; // Fatal Run (PAL) (alt)
		if (crc == 0x991d2348) chip = 1; // Fatal Run (PAL)
		if (crc == 0x7169337a) chip = 1; // Fatal Run (PAL) (NTSC fixed)

	}

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
	return 0;
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
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_CATEGORY(10) PORT_PLAYER(1)

	PORT_START /* [5] */
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
	PORT_CATEGORY_CLASS( 0xf0, 0x00, "Left Controller" )
	PORT_CATEGORY_ITEM(    0x00, DEF_STR( Joystick ), 10 )
	PORT_CATEGORY_ITEM(    0x10, "Paddles", 11 )
	//PORT_CATEGORY_ITEM(    0x20, "Driving", 12 )
	//PORT_CATEGORY_ITEM(    0x40, "Keypad", 13 )
	//PORT_CATEGORY_ITEM(    0x80, "Lightgun", 14 )
	PORT_CATEGORY_CLASS( 0x0f, 0x00, "Right Controller" )
	PORT_CATEGORY_ITEM(    0x00, DEF_STR( Joystick ), 20 )
	PORT_CATEGORY_ITEM(    0x01, "Paddles", 21 )
	//PORT_CATEGORY_ITEM(    0x02, "Driving", 22 )
	//PORT_CATEGORY_ITEM(    0x04, "Keypad", 23 )
	//PORT_CATEGORY_ITEM(    0x08, "Lightgun", 24 )

	PORT_START /* [10] */
	PORT_CONFNAME( 0xff, 0xff, "Bank Mode" )
	PORT_CONFSETTING(    0xff, "Auto" )
	PORT_CONFSETTING(    mode2K, "2K" )
	PORT_CONFSETTING(    mode4K, "4K" )
	PORT_CONFSETTING(    mode8K, "8K" )
	PORT_CONFSETTING(    mode12, "12K" )
	PORT_CONFSETTING(    mode16, "16K" )
	PORT_CONFSETTING(    mode32, "32K" )
	PORT_CONFSETTING(    modeAV, "Activision" )
	PORT_CONFSETTING(    modeCV, "CommaVid" )
	PORT_CONFSETTING(    modeDC, "Dynacom" )
	PORT_CONFSETTING(    modeMN, "M Network" )
	PORT_CONFSETTING(    modePB, "Parker Brothers" )
	PORT_CONFSETTING(    modeTV, "Tigervision" )
	PORT_CONFSETTING(    modeUA, "UA Limited" )

	PORT_START /* [11] */
	PORT_CONFNAME( 0xff, 0xff, "Super Chip" )
	PORT_CONFSETTING(    0xff, "Auto" )
	PORT_CONFSETTING(    0x00, DEF_STR( Off ))
	PORT_CONFSETTING(    0x01, DEF_STR( On ))

	PORT_START	/* [12] left keypad */
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

	PORT_START  /* [13] right keypad */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CATEGORY(23) PORT_NAME("right 1")
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CATEGORY(23) PORT_NAME("right 2")
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CATEGORY(23) PORT_NAME("right 3")
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CATEGORY(23) PORT_NAME("right 4")
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CATEGORY(23) PORT_NAME("right 5")
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CATEGORY(23) PORT_NAME("right 6")
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CATEGORY(23) PORT_NAME("right 7")
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CATEGORY(23) PORT_NAME("right 8")
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CATEGORY(23) PORT_NAME("right 9")
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CATEGORY(23) PORT_NAME("right *")
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CATEGORY(23) PORT_NAME("right 0")
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CATEGORY(23) PORT_NAME("right #")
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
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
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
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
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
//	case DEVINFO_PTR_CASSETTE_FORMATS:
//		info->p = (void *)a26_cassette_formats;
//		break;
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

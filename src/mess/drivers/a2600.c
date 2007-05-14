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
	modeCV
};


static UINT8* extra_RAM;

static unsigned cart_size;
static unsigned current_bank;


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
	{ 0x34b80a97, modeTV }, // Espial
	{ 0xbd08d915, modeTV }, // Miner 2049er
	{ 0xbfa477cd, modeTV }, // Miner 2049er Volume II
	{ 0xdb376663, modeTV }, // Polaris
	{ 0x25b7f8f9, modeTV }, // Polaris
	{ 0x71ecefaf, modeTV }, // Polaris
	{ 0xc820bd75, modeTV }, // River Patrol
	{ 0xdd183a4f, modeTV }, // Springer
	{ 0xb53b33f1, modeUA }, // Funky Fish Prototype
	{ 0x35589cec, modeUA }, // Pleiads Prototype
	{ 0xdf2bc303, modeMN }, // Bump 'n' Jump
	{ 0xc183fbbc, modeMN }, // Burgertime
	{ 0x66f1849e, modeMN }, // Burgertime (E7)
	{ 0x0603e177, modeMN }, // Masters of the Universe
	{ 0x14f126c0, modeCV }, // Magicard
	{ 0x34b0b5c2, modeCV }  // Video Life
};


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

	return 1;
}


static int device_load_a2600_cart(mess_image *image)
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
		break;

	default:
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
	memory_set_bankptr(1, CART + 0x1000 * offset);
}
void mode12_switch(UINT16 offset, UINT8 data)
{
	memory_set_bankptr(1, CART + 0x1000 * offset);
}
void mode16_switch(UINT16 offset, UINT8 data)
{
	memory_set_bankptr(1, CART + 0x1000 * offset);
}
void mode32_switch(UINT16 offset, UINT8 data)
{
	memory_set_bankptr(1, CART + 0x1000 * offset);
}
void modeTV_switch(UINT16 offset, UINT8 data)
{
	memory_set_bankptr(1, CART + 0x800 * (data & 3));
}
void modeUA_switch(UINT16 offset, UINT8 data)
{
	memory_set_bankptr(1, CART + (offset >> 6) * 0x1000);
}
void modePB_switch(UINT16 offset, UINT8 data)
{
	memory_set_bankptr(1 + (offset >> 3), CART + 0x400 * (offset & 7));
}
void modeMN_switch(UINT16 offset, UINT8 data)
{
	memory_set_bankptr(1, CART + 0x800 * offset);
}
void modeMN_RAM_switch(UINT16 offset, UINT8 data)
{
	memory_set_bankptr(9, extra_RAM + (4 + offset) * 256 );
}
void modeDC_switch(UINT16 offset, UINT8 data)
{
	memory_set_bankptr(1, CART + 0x1000 * next_bank());
}


static  READ8_HANDLER(mode8K_switch_r) { mode8K_switch(offset, 0); return 0; }
static  READ8_HANDLER(mode12_switch_r) { mode12_switch(offset, 0); return 0; }
static  READ8_HANDLER(mode16_switch_r) { mode16_switch(offset, 0); return 0; }
static  READ8_HANDLER(mode32_switch_r) { mode32_switch(offset, 0); return 0; }
static  READ8_HANDLER(modePB_switch_r) { modePB_switch(offset, 0); return 0; }
static  READ8_HANDLER(modeMN_switch_r) { modeMN_switch(offset, 0); return 0; }
static  READ8_HANDLER(modeMN_RAM_switch_r) { modeMN_RAM_switch(offset, 0); return 0; }
static  READ8_HANDLER(modeTV_switch_r) { modeTV_switch(offset, 0); return 0; }
static  READ8_HANDLER(modeUA_switch_r) { modeUA_switch(offset, 0); return 0; }
static  READ8_HANDLER(modeDC_switch_r) { modeDC_switch(offset, 0); return 0; }


static WRITE8_HANDLER(mode8K_switch_w) { mode8K_switch(offset, data); }
static WRITE8_HANDLER(mode12_switch_w) { mode12_switch(offset, data); }
static WRITE8_HANDLER(mode16_switch_w) { mode16_switch(offset, data); }
static WRITE8_HANDLER(mode32_switch_w) { mode32_switch(offset, data); }
static WRITE8_HANDLER(modePB_switch_w) {	modePB_switch(offset, data); }
static WRITE8_HANDLER(modeMN_switch_w) {	modeMN_switch(offset, data); }
static WRITE8_HANDLER(modeMN_RAM_switch_w) { modeMN_RAM_switch(offset, data); }
static WRITE8_HANDLER(modeTV_switch_w) { modeTV_switch(offset, data); }
static WRITE8_HANDLER(modeUA_switch_w) { modeUA_switch(offset, data); }
static WRITE8_HANDLER(modeDC_switch_w) { modeDC_switch(offset, data); }

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
		memory_set_bankptr( 1, CART + 0x1000 * ( ( address & 0x2000) ? 0 : 1 ) );
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
	AM_RANGE(0x0000, 0x007F) AM_MIRROR(0x0100) AM_READWRITE(tia_r, tia_w)
	AM_RANGE(0x0080, 0x00FF) AM_MIRROR(0x0100) AM_RAM
	AM_RANGE(0x0280, 0x029F)                   AM_READWRITE(r6532_0_r, r6532_0_w)
	AM_RANGE(0x1000, 0x1FFF)                   AM_ROMBANK(1)
ADDRESS_MAP_END


static  READ8_HANDLER( switch_A_r )
{
	UINT8 val = 0;

	int control0 = readinputport(9) % 16;
	int control1 = readinputport(9) / 16;

	val |= readinputport(6 + control0) & 0x0F;
	val |= readinputport(6 + control1) & 0xF0;

	return val;
}


static struct R6532interface r6532_interface =
{
	switch_A_r,
	input_port_8_r,
	NULL,
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

		memory_set_bankptr(i + 1, memory_region(REGION_USER1) + init);
	}
}


static MACHINE_START( a2600 )
{
	/* NPW 6-Mar-2006 - The MAME core changed, and now I cannot use readinputport() here properly */
	int mode = 0xFF; /* readinputport(10); */
	int chip = 0xFF; /* readinputport(11); */

	extra_RAM = auto_malloc(0x800);

	r6532_init(0, &r6532_interface);

	if ( !strcmp( Machine->gamedrv->name, "a2600p" ) ) {
		tia_init_pal();
	} else {
		tia_init();
	}

	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x00, 0x7f, 0, 0, tia_w);
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x00, 0x7f, 0, 0, tia_r);

	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x200, 0x27f, 0, 0, MWA8_NOP);
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x200, 0x27f, 0, 0, MRA8_NOP);

	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x1000, 0x1fff, 0, 0, MWA8_NOP);
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x1000, 0x1fff, 0, 0, MRA8_NOP);

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
		install_banks(2, 0x1800);
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
		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x00, 0x3f, 0, 0, modeTV_switch_r);
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
	PORT_BIT( 0xff, 0x80, IPT_PADDLE) PORT_SENSITIVITY(40) PORT_KEYDELTA(10) PORT_CENTERDELTA(0) PORT_MINMAX(0,255) PORT_CATEGORY(2) PORT_PLAYER(1) PORT_REVERSE
	PORT_START /* [1] */
	PORT_BIT( 0xff, 0x80, IPT_PADDLE) PORT_SENSITIVITY(40) PORT_KEYDELTA(10) PORT_CENTERDELTA(0) PORT_MINMAX(0,255) PORT_CATEGORY(2) PORT_PLAYER(2) PORT_REVERSE
	PORT_START /* [2] */
	PORT_BIT( 0xff, 0x80, IPT_PADDLE) PORT_SENSITIVITY(40) PORT_KEYDELTA(10) PORT_CENTERDELTA(0) PORT_MINMAX(0,255) PORT_CATEGORY(4) PORT_PLAYER(3) PORT_REVERSE
	PORT_START /* [3] */
	PORT_BIT( 0xff, 0x80, IPT_PADDLE) PORT_SENSITIVITY(40) PORT_KEYDELTA(10) PORT_CENTERDELTA(0) PORT_MINMAX(0,255) PORT_CATEGORY(4) PORT_PLAYER(4) PORT_REVERSE PORT_CODE_DEC(KEYCODE_4_PAD) PORT_CODE_INC(KEYCODE_6_PAD)

	PORT_START /* [4] */
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_CATEGORY(1) PORT_PLAYER(1)

	PORT_START /* [5] */
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_CATEGORY(3) PORT_PLAYER(2)

	PORT_START /* [6] SWCHA joystick */
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP) PORT_CATEGORY(3) PORT_PLAYER(2)
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN) PORT_CATEGORY(3) PORT_PLAYER(2)
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT) PORT_CATEGORY(3) PORT_PLAYER(2)
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT) PORT_CATEGORY(3) PORT_PLAYER(2)
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP) PORT_CATEGORY(1) PORT_PLAYER(1)
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN) PORT_CATEGORY(1) PORT_PLAYER(1)
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT) PORT_CATEGORY(1) PORT_PLAYER(1)
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT) PORT_CATEGORY(1) PORT_PLAYER(1)

	PORT_START /* [7] SWCHA paddles */
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_CATEGORY(4) PORT_PLAYER(4)
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_CATEGORY(4) PORT_PLAYER(3)
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_CATEGORY(2) PORT_PLAYER(2)
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_CATEGORY(2) PORT_PLAYER(1)

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
	PORT_CATEGORY_ITEM(    0x00, DEF_STR( Joystick ), 1 )
	PORT_CATEGORY_ITEM(    0x10, "Paddles", 2 )
	PORT_CATEGORY_CLASS( 0x0f, 0x00, "Right Controller" )
	PORT_CATEGORY_ITEM(    0x00, DEF_STR( Joystick ), 3 )
	PORT_CATEGORY_ITEM(    0x01, "Paddles", 4 )

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
	MDRV_SCREEN_RAW_PARAMS( MASTER_CLOCK_PAL, 228, 26, 26 + 160 + 16, 312, 32, 32 + 192 + 32 )
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
	ROM_REGION( 0x10000, REGION_USER1, 0 )
	ROM_FILL( 0x00000, 0x10000, 0xFF )
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

SYSTEM_CONFIG_START(a2600)
	CONFIG_DEVICE(a2600_cartslot_getinfo)
SYSTEM_CONFIG_END


/*    YEAR	NAME	PARENT	COMPAT	MACHINE	INPUT	INIT	CONFIG	COMPANY		FULLNAME */
CONS( 1977,	a2600,	0,		0,		a2600,	a2600,	0,		a2600,	"Atari",	"Atari 2600 (NTSC)" , 0)
CONS( 1978,	a2600p,	a2600,	0,		a2600p,	a2600,	0,		a2600,  "Atari",    "Atari 2600 (PAL)", 0)

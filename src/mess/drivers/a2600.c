/***************************************************************************

  Atari VCS 2600 driver

***************************************************************************/

#include "driver.h"
#include "machine/6532riot.h"
#include "cpu/m6502/m6502.h"
#include "sound/wave.h"
#include "sound/tiaintf.h"
#include "devices/cartslot.h"
#include "devices/cassette.h"
#include "formats/a26_cas.h"
#include "video/tia.h"


#define CART memory_region(machine, "user1")

#define MASTER_CLOCK_NTSC	3579545
#define MASTER_CLOCK_PAL	3546894
#define CATEGORY_SELECT		16

/* Some defines for the naming of the controller ports and the controllers */
#define	STR_LEFT_CONTROLLER		"Left Controller"
#define STR_RIGHT_CONTROLLER	"Right Controller"
#define STR_PADDLES				"Paddles"
#define STR_DRIVING				"Driving"
#define STR_KEYPAD				"Keypad"
#define STR_LIGHTGUN			"Lightgun"
#define STR_BOOSTERGRIP			"Booster Grip"
#define STR_CX22TRAKBALL		"CX-22 Trak-Ball"
#define STR_CX80TRAKBALL		"CX-80 Trak-Ball (TB Mode) / AtariST Mouse"
#define STR_AMIGAMOUSE			"Amiga Mouse"
#define STR_KIDVID				"KidVid Voice Module"

enum
{
	mode2K,
	mode4K,
	modeF8,
	modeFA,
	modeF6,
	modeF4,
	modeFE,
	modeE0,
	mode3F,
	modeUA,
	modeE7,
	modeDC,
	modeCV,
	mode3E,
	modeSS,
	modeFV,
	modeDPC,
	mode32in1,
	modeJVP
};

struct _extrainfo_banking_def {
	char	extrainfo[5];
	int		bank_mode;
};

static const struct _extrainfo_banking_def extrainfo_banking_defs[] = {
	/* banking schemes */
	{ "F8",	modeF8 },
	{ "FA", modeFA },
	{ "F6", modeF6 },
	{ "F4", modeF4 },
	{ "FE", modeFE },
	{ "E0", modeE0 },
	{ "3F", mode3F },
	{ "UA", modeUA },
	{ "E7", modeE7 },
	{ "DC", modeDC },
	{ "CV", modeCV },
	{ "3E", mode3E },
	{ "SS", modeSS },
	{ "FV", modeFV },
	{ "DPC", modeDPC },
	{ "32in1", mode32in1 },
	{ "JVP", modeJVP },

	/* end of list - do not remove */
	{ "\0", 0 },
};

struct DPC_DF {
	UINT8	top;
	UINT8	bottom;
	UINT8	low;
	UINT8	high;
	UINT8	flag;
	UINT8	music_mode;		/* Only used by data fetchers 5,6, and 7 */
	UINT8	osc_clk;		/* Only used by data fetchers 5,6, and 7 */
};

static struct DPC {
	struct DPC_DF	df[8];
	UINT8	movamt;
	UINT8	latch_62;
	UINT8	latch_64;
	UINT8	dlc;
	UINT8	shift_reg;
	emu_timer	*oscillator;
} dpc;

static UINT8* extra_RAM;
static UINT8* bank_base[5];
static UINT8* ram_base;
static UINT8* riot_ram;

static UINT8 banking_mode;
static UINT8 keypad_left_column;
static UINT8 keypad_right_column;

static unsigned cart_size;
static unsigned number_banks;
static unsigned current_bank;
static unsigned current_reset_bank_counter;
static unsigned mode3E_ram_enabled;
static UINT8 modeSS_byte;
static UINT32 modeSS_byte_started;
static unsigned modeSS_write_delay;
static unsigned modeSS_write_enabled;
static unsigned modeSS_high_ram_enabled;
static unsigned modeSS_diff_adjust;
static unsigned FVlocked;

static UINT16 current_screen_height;
static const UINT16 supported_screen_heights[4] = { 262, 312, 328, 342 };

// try to detect 2600 controller setup. returns 32bits with left/right controller info

static unsigned long detect_2600controllers(running_machine *machine)
{
#define JOYS 0x001
#define PADD 0x002
#define KEYP 0x004
#define LGUN 0x008
#define INDY 0x010
#define BOOS 0x020
#define KVID 0x040
#define CMTE 0x080
#define MLNK 0x100
#define AMSE 0x200
#define CX22 0x400
#define CX80 0x800

	unsigned int left,right;
	int i,j,foundkeypad = 0;
	UINT8 *cart;
	static const unsigned char signatures[][5] =  {
									{ 0x55, 0xa5, 0x3c, 0x29, 0}, // star raiders
									{ 0xf9, 0xff, 0xa5, 0x80, 1}, // sentinel
									{ 0x81, 0x02, 0xe8, 0x86, 1}, // shooting arcade
									{ 0x02, 0xa9, 0xec, 0x8d, 1}, // guntest4 tester
									{ 0x85, 0x2c, 0x85, 0xa7, 2}, // INDY 500
									{ 0xa1, 0x8d, 0x9d, 0x02, 2}, // omega race INDY
									{ 0x65, 0x72, 0x44, 0x43, 2}, // Sprintmaster INDY
									{ 0x89, 0x8a, 0x99, 0xaa, 3}, // omega race
									{ 0x9a, 0x8e, 0x81, 0x02, 4},
									{ 0xdd, 0x8d, 0x80, 0x02, 4},
									{ 0x85, 0x8e, 0x81, 0x02, 4},
									{ 0x8d, 0x81, 0x02, 0xe6, 4},
									{ 0xff, 0x8d, 0x81, 0x02, 4},
									{ 0xa9, 0x03, 0x8d, 0x81, 5},
									{ 0xa9, 0x73, 0x8d, 0x80, 6},
//                                  { 0x82, 0x02, 0x85, 0x8f, 7}, // Mind Maze (really Mind Link??)
									{ 0xa9, 0x30, 0x8d, 0x80, 7}, // Bionic Breakthrough
									{ 0x02, 0x8e, 0x81, 0x02, 7}, // Telepathy
									{ 0x41, 0x6d, 0x69, 0x67, 9}, // Missile Command Amiga Mouse
									{ 0x43, 0x58, 0x2d, 0x32, 10}, // Missile Command CX22 TrackBall
									{ 0x43, 0x58, 0x2d, 0x38, 11}, // Missile Command CX80 TrackBall
									{ 0x4e, 0xa8, 0xa4, 0xa2, 12}, // Omega Race for Joystick ONLY
									{ 0xa6, 0xef, 0xb5, 0x38, 8} // Warlords.. paddles ONLY
	};
	// start with this.. if anyone finds a game that does NOT work with both controllers enabled
	// it can be fixed here with a new signature (note that the Coleco Gemini has this setup also)
	left = JOYS+PADD; right = JOYS+PADD;
	// default for bad dumps and roms too large to have special controllers
	if ((cart_size > 0x4000) || (cart_size & 0x7ff)) return (left << 16) + right;

	cart = CART;
	for (i = 0; i < cart_size - (sizeof signatures/sizeof signatures[0]); i++)
	{
		for (j = 0; j < (sizeof signatures/sizeof signatures[0]); j++)
		{
			if (!memcmp(&cart[i], &signatures[j],sizeof signatures[0] - 1))
			{
				int k = signatures[j][4];
				if (k == 0) return (JOYS << 16) + KEYP;
				if (k == 1) return (LGUN << 16);
				if (k == 2) return (INDY << 16) + INDY;
				if (k == 3) return (BOOS << 16) + BOOS;
				if (k == 5) return (JOYS << 16) + KVID;
				if (k == 6) return (CMTE << 16) + CMTE;
				if (k == 7) return (MLNK << 16) + MLNK;
				if (k == 8) return (PADD << 16) + PADD;
				if (k == 9) return (AMSE << 16) + AMSE;
				if (k == 10) return (CX22 << 16) + CX22;
				if (k == 11) return (CX80 << 16) + CX80;
				if (k == 12) return (JOYS << 16) + JOYS;
				if (k == 4) foundkeypad = 1;
			}
		}
	}
	if (foundkeypad) return (KEYP << 16) + KEYP;
	return (left << 16) + right;
}

static int detect_modeDC(running_machine *machine)
{
	int i,numfound = 0;
	// signature is also in 'video reflex'.. maybe figure out that controller port someday...
	static const unsigned char signature[3] = { 0x8d, 0xf0, 0xff };
	if (cart_size == 0x10000)
	{
		UINT8 *cart = CART;
		for (i = 0; i < cart_size - sizeof signature; i++)
		{
			if (!memcmp(&cart[i], signature,sizeof signature))
			{
				numfound = 1;
			}
		}
	}
	if (numfound) return 1;
	return 0;
}

static int detect_modef6(running_machine *machine)
{
	int i,numfound = 0;
	static const unsigned char signature[3] = { 0x8d, 0xf6, 0xff };
	if (cart_size == 0x4000)
	{
		UINT8 *cart = CART;
		for (i = 0; i < cart_size - sizeof signature; i++)
		{
			if (!memcmp(&cart[i], signature,sizeof signature))
			{
				numfound = 1;
			}
		}
	}
	if (numfound) return 1;
	return 0;
}

static int detect_mode3E(running_machine *machine)
{
	// this one is a little hacky.. looks for STY $3e, which is unique to
	// 'not boulderdash', but is the only example i have (cow)
	// Would have used STA $3e, but 'Alien' and 'Star Raiders' do that for unknown reasons

	int i,numfound = 0;
	static const unsigned char signature[3] = { 0x84, 0x3e, 0x9d };
	if (cart_size == 0x0800 || cart_size == 0x1000)
	{
		UINT8 *cart = CART;
		for (i = 0; i < cart_size - sizeof signature; i++)
		{
			if (!memcmp(&cart[i], signature,sizeof signature))
			{
				numfound = 1;
			}
		}
	}
	if (numfound) return 1;
	return 0;
}

static int detect_modeSS(running_machine *machine)
{
	int i,numfound = 0;
	static const unsigned char signature[5] = { 0xbd, 0xe5, 0xff, 0x95, 0x81 };
	if (cart_size == 0x0800 || cart_size == 0x1000)
	{
		UINT8 *cart = CART;
		for (i = 0; i < cart_size - sizeof signature; i++)
		{
			if (!memcmp(&cart[i], signature,sizeof signature))
			{
				numfound = 1;
			}
		}
	}
	if (numfound) return 1;
	return 0;
}

static int detect_modeFE(running_machine *machine)
{
	int i,j,numfound = 0;
	static const unsigned char signatures[][5] =  {
									{ 0x20, 0x00, 0xd0, 0xc6, 0xc5 },
									{ 0x20, 0xc3, 0xf8, 0xa5, 0x82 },
									{ 0xd0, 0xfb, 0x20, 0x73, 0xfe },
									{ 0x20, 0x00, 0xf0, 0x84, 0xd6 }
	};
	if (cart_size == 0x2000)
	{
		UINT8 *cart = CART;
		for (i = 0; i < cart_size - (sizeof signatures/sizeof signatures[0]); i++)
		{
			for (j = 0; j < (sizeof signatures/sizeof signatures[0]) && !numfound; j++)
			{
				if (!memcmp(&cart[i], &signatures[j],sizeof signatures[0]))
				{
					numfound = 1;
				}
			}
		}
	}
	if (numfound) return 1;
	return 0;
}

static int detect_modeE0(running_machine *machine)
{
	int i,j,numfound = 0;
	static const unsigned char signatures[][3] =  {
									{ 0x8d, 0xe0, 0x1f },
									{ 0x8d, 0xe0, 0x5f },
									{ 0x8d, 0xe9, 0xff },
									{ 0xad, 0xe9, 0xff },
									{ 0xad, 0xed, 0xff },
									{ 0xad, 0xf3, 0xbf }
	};
	if (cart_size == 0x2000)
	{
		UINT8 *cart = CART;
		for (i = 0; i < cart_size - (sizeof signatures/sizeof signatures[0]); i++)
		{
			for (j = 0; j < (sizeof signatures/sizeof signatures[0]) && !numfound; j++)
			{
				if (!memcmp(&cart[i], &signatures[j],sizeof signatures[0]))
				{
					numfound = 1;
				}
			}
		}
	}
	if (numfound) return 1;
	return 0;
}

static int detect_modeCV(running_machine *machine)
{
	int i,j,numfound = 0;
	static const unsigned char signatures[][3] = {
									{ 0x9d, 0xff, 0xf3 },
									{ 0x99, 0x00, 0xf4 }
	};
	if (cart_size == 0x0800 || cart_size == 0x1000)
	{
		UINT8 *cart = CART;
		for (i = 0; i < cart_size - (sizeof signatures/sizeof signatures[0]); i++)
		{
			for (j = 0; j < (sizeof signatures/sizeof signatures[0]) && !numfound; j++)
			{
				if (!memcmp(&cart[i], &signatures[j],sizeof signatures[0]))
				{
					numfound = 1;
				}
			}
		}
	}
	if (numfound) return 1;
	return 0;
}

static int detect_modeFV(running_machine *machine)
{
	int i,j,numfound = 0;
	static const unsigned char signatures[][3] = {
									{ 0x2c, 0xd0, 0xff }
	};
	if (cart_size == 0x2000)
	{
		UINT8 *cart = CART;
		for (i = 0; i < cart_size - (sizeof signatures/sizeof signatures[0]); i++)
		{
			for (j = 0; j < (sizeof signatures/sizeof signatures[0]) && !numfound; j++)
			{
				if (!memcmp(&cart[i], &signatures[j],sizeof signatures[0]))
				{
					numfound = 1;
				}
			}
		}
		FVlocked = 0;
	}
	if (numfound) return 1;
	return 0;
}

static int detect_modeJVP(running_machine *machine)
{
	int i,j,numfound = 0;
	static const unsigned char signatures[][4] = {
									{ 0x2c, 0xc0, 0xef, 0x60 },
									{ 0x8d, 0xa0, 0x0f, 0xf0 }
	};
	if (cart_size == 0x4000 || cart_size == 0x2000)
	{
		UINT8 *cart = CART;
		for (i = 0; i < cart_size - (sizeof signatures/sizeof signatures[0]); i++)
		{
			for (j = 0; j < (sizeof signatures/sizeof signatures[0]) && !numfound; j++)
			{
				if (!memcmp(&cart[i], &signatures[j],sizeof signatures[0]))
				{
					numfound = 1;
				}
			}
		}
	}
	if (numfound) return 1;
	return 0;
}

static int detect_modeE7(running_machine *machine)
{
	int i,j,numfound = 0;
	static const unsigned char signatures[][3] = {
									{ 0xad, 0xe5, 0xff },
									{ 0x8d, 0xe7, 0xff }
	};
	if (cart_size == 0x2000 || cart_size == 0x4000)
	{
		UINT8 *cart = CART;
		for (i = 0; i < cart_size - (sizeof signatures/sizeof signatures[0]); i++)
		{
			for (j = 0; j < (sizeof signatures/sizeof signatures[0]) && !numfound; j++)
			{
				if (!memcmp(&cart[i], &signatures[j],sizeof signatures[0]))
				{
					numfound = 1;
				}
			}
		}
	}
	if (numfound) return 1;
	return 0;
}

static int detect_modeUA(running_machine *machine)
{
	int i,numfound = 0;
	static const unsigned char signature[3] = { 0x8d, 0x40, 0x02 };
	if (cart_size == 0x2000)
	{
		UINT8 *cart = CART;
		for (i = 0; i < cart_size - sizeof signature; i++)
		{
			if (!memcmp(&cart[i], signature,sizeof signature))
			{
				numfound = 1;
			}
		}
	}
	if (numfound) return 1;
	return 0;
}

static int detect_8K_mode3F(running_machine *machine)
{
	int i,numfound = 0;
	static const unsigned char signature1[4] = { 0xa9, 0x01, 0x85, 0x3f };
	static const unsigned char signature2[4] = { 0xa9, 0x02, 0x85, 0x3f };
	// have to look for two signatures because 'not boulderdash' gives false positive otherwise
	if (cart_size == 0x2000)
	{
		UINT8 *cart = CART;
		for (i = 0; i < cart_size - sizeof signature1; i++)
		{
			if (!memcmp(&cart[i], signature1,sizeof signature1))
			{
				numfound |= 0x01;
			}
			if (!memcmp(&cart[i], signature2,sizeof signature2))
			{
				numfound |= 0x02;
			}
		}
	}
	if (numfound == 0x03) return 1;
	return 0;
}

static int detect_32K_mode3F(running_machine *machine)
{
	int i,numfound = 0;
	static const unsigned char signature[4] = { 0xa9, 0x0e, 0x85, 0x3f };
	if (cart_size >= 0x8000)
	{
		UINT8 *cart = CART;
		for (i = 0; i < cart_size - sizeof signature; i++)
		{
			if (!memcmp(&cart[i], signature,sizeof signature))
			{
				numfound++;
			}
		}
	}
	if (numfound > 1) return 1;
	return 0;
}

static int detect_super_chip(running_machine *machine)
{
	int i,j;
	UINT8 *cart = CART;
	static const unsigned char signatures[][5] = {
									{ 0xa2, 0x7f, 0x9d, 0x00, 0xf0 }, // dig dug
									{ 0xae, 0xf6, 0xff, 0x4c, 0x00 } // off the wall
	};

	if (cart_size == 0x4000)
	{
		for (i = 0; i < cart_size - (sizeof signatures/sizeof signatures[0]); i++)
		{
			for (j = 0; j < (sizeof signatures/sizeof signatures[0]); j++)
			{
				if (!memcmp(&cart[i], &signatures[j],sizeof signatures[0]))
				{
					return 1;
				}
			}
		}
	}
	for (i = 0x1000; i < cart_size; i += 0x1000)
	{
		if (memcmp(cart, cart + i, 0x100))
		{
			return 0;
		}
	}
	/* Check the reset vector does not point into the super chip RAM area */
	i = ( cart[0x0FFD] << 8 ) | cart[0x0FFC];
	if ( ( i & 0x0FFF ) < 0x0100 )
	{
		return 0;
	}
	return 1;
}


static DEVICE_START( a2600_cart )
{
	banking_mode = 0xFF;
}


static DEVICE_IMAGE_LOAD( a2600_cart )
{
	running_machine *machine = image->machine;
	const struct _extrainfo_banking_def *eibd;
	UINT8 *cart = CART;
	const char	*extrainfo;

	cart_size = image_length(image);

	switch (cart_size)
	{
	case 0x00800:
	case 0x01000:
	case 0x02000:
	case 0x028FF:
	case 0x02900:
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

	image_fread(image, cart, cart_size);

	if (!(cart_size == 0x4000 && detect_modef6(image->machine)))
	{
		while (cart_size > 0x00800)
		{
			if (!memcmp(cart, &cart[cart_size/2],cart_size/2)) cart_size /= 2;
			else break;
		}
	}

	extrainfo = image_extrainfo( image );

	if ( extrainfo && extrainfo[0] )
	{
		for ( eibd = extrainfo_banking_defs; eibd->extrainfo[0]; eibd++ )
		{
			if ( ! mame_stricmp( eibd->extrainfo, extrainfo ) )
			{
				banking_mode = eibd->bank_mode;
			}
		}
	}
	return 0;
}


static int next_bank(void)
{
	return current_bank = (current_bank + 1) % 16;
}


static void modeF8_switch(running_machine *machine, UINT16 offset, UINT8 data)
{
	bank_base[1] = CART + 0x1000 * offset;
	memory_set_bankptr(machine,"bank1", bank_base[1]);
}
static void modeFA_switch(running_machine *machine, UINT16 offset, UINT8 data)
{
	bank_base[1] = CART + 0x1000 * offset;
	memory_set_bankptr(machine,"bank1", bank_base[1]);
}
static void modeF6_switch(running_machine *machine, UINT16 offset, UINT8 data)
{
	bank_base[1] = CART + 0x1000 * offset;
	memory_set_bankptr(machine,"bank1", bank_base[1]);
}
static void modeF4_switch(running_machine *machine, UINT16 offset, UINT8 data)
{
	bank_base[1] = CART + 0x1000 * offset;
	memory_set_bankptr(machine,"bank1", bank_base[1]);
}
static void mode3F_switch(running_machine *machine, UINT16 offset, UINT8 data)
{
	bank_base[1] = CART + 0x800 * (data & (number_banks - 1));
	memory_set_bankptr(machine,"bank1", bank_base[1]);
}
static void modeUA_switch(running_machine *machine, UINT16 offset, UINT8 data)
{
	bank_base[1] = CART + (offset >> 6) * 0x1000;
	memory_set_bankptr(machine,"bank1", bank_base[1]);
}
static void modeE0_switch(running_machine *machine, UINT16 offset, UINT8 data)
{
	int bank = 1 + (offset >> 3);
	char bank_name[10];
	sprintf(bank_name,"bank%d",bank);
	bank_base[bank] = CART + 0x400 * (offset & 7);
	memory_set_bankptr(machine,bank_name, bank_base[bank]);
}
static void modeE7_switch(running_machine *machine, UINT16 offset, UINT8 data)
{
	bank_base[1] = CART + 0x800 * offset;
	memory_set_bankptr(machine,"bank1", bank_base[1]);
}
static void modeE7_RAM_switch(running_machine *machine, UINT16 offset, UINT8 data)
{
	memory_set_bankptr(machine,"bank9", extra_RAM + (4 + offset) * 256 );
}
static void modeDC_switch(running_machine *machine, UINT16 offset, UINT8 data)
{
	bank_base[1] = CART + 0x1000 * next_bank();
	memory_set_bankptr(machine,"bank1", bank_base[1]);
}
static void mode3E_switch(running_machine *machine, UINT16 offset, UINT8 data)
{
	bank_base[1] = CART + 0x800 * (data & (number_banks - 1));
	memory_set_bankptr(machine,"bank1", bank_base[1]);
	mode3E_ram_enabled = 0;
}
static void mode3E_RAM_switch(running_machine *machine, UINT16 offset, UINT8 data)
{
	ram_base = extra_RAM + 0x200 * ( data & 0x3F );
	memory_set_bankptr(machine,"bank1", ram_base );
	mode3E_ram_enabled = 1;
}
static void modeFV_switch(running_machine *machine, UINT16 offset, UINT8 data)
{
	//printf("ModeFV %04x\n",offset);
	if (!FVlocked && ( cpu_get_pc(cputag_get_cpu(machine, "maincpu")) & 0x1F00 ) == 0x1F00 )
	{
		FVlocked = 1;
		current_bank = current_bank ^ 0x01;
		bank_base[1] = CART + 0x1000 * current_bank;
		memory_set_bankptr(machine,"bank1", bank_base[1]);
	}
}
static void modeJVP_switch(running_machine *machine, UINT16 offset, UINT8 data)
{
	switch( offset )
	{
	case 0x00:
	case 0x20:
		current_bank ^= 1;
		break;
	default:
		printf("%04X: write to unknown mapper address %02X\n", cpu_get_pc(cputag_get_cpu(machine, "maincpu")), 0xfa0 + offset );
		break;
	}
	bank_base[1] = CART + 0x1000 * current_bank;
	memory_set_bankptr(machine, "bank1", bank_base[1] );
}


/* These read handlers will return the byte from the new bank */
static  READ8_HANDLER(modeF8_switch_r) { modeF8_switch(space->machine, offset, 0); return bank_base[1][0xff8 + offset]; }
static  READ8_HANDLER(modeFA_switch_r) { modeFA_switch(space->machine, offset, 0); return bank_base[1][0xff8 + offset]; }
static  READ8_HANDLER(modeF6_switch_r) { modeF6_switch(space->machine, offset, 0); return bank_base[1][0xff6 + offset]; }
static  READ8_HANDLER(modeF4_switch_r) { modeF4_switch(space->machine, offset, 0); return bank_base[1][0xff4 + offset]; }
static  READ8_HANDLER(modeE0_switch_r) { modeE0_switch(space->machine, offset, 0); return bank_base[4][0x3e0 + offset]; }
static  READ8_HANDLER(modeE7_switch_r) { modeE7_switch(space->machine, offset, 0); return bank_base[1][0xfe0 + offset]; }
static  READ8_HANDLER(modeE7_RAM_switch_r) { modeE7_RAM_switch(space->machine, offset, 0); return 0; }
static  READ8_HANDLER(modeUA_switch_r) { modeUA_switch(space->machine, offset, 0); return 0; }
static  READ8_HANDLER(modeDC_switch_r) { modeDC_switch(space->machine, offset, 0); return bank_base[1][0xff0 + offset]; }
static  READ8_HANDLER(modeFV_switch_r) { modeFV_switch(space->machine, offset, 0); return bank_base[1][0xfd0 + offset]; }
static  READ8_HANDLER(modeJVP_switch_r) { modeJVP_switch(space->machine, offset, 0); return riot_ram[ 0x20 + offset ]; }


static WRITE8_HANDLER(modeF8_switch_w) { modeF8_switch(space->machine, offset, data); }
static WRITE8_HANDLER(modeFA_switch_w) { modeFA_switch(space->machine, offset, data); }
static WRITE8_HANDLER(modeF6_switch_w) { modeF6_switch(space->machine, offset, data); }
static WRITE8_HANDLER(modeF4_switch_w) { modeF4_switch(space->machine, offset, data); }
static WRITE8_HANDLER(modeE0_switch_w) { modeE0_switch(space->machine, offset, data); }
static WRITE8_HANDLER(modeE7_switch_w) { modeE7_switch(space->machine, offset, data); }
static WRITE8_HANDLER(modeE7_RAM_switch_w) { modeE7_RAM_switch(space->machine, offset, data); }
static WRITE8_HANDLER(mode3F_switch_w) { mode3F_switch(space->machine, offset, data); }
static WRITE8_HANDLER(modeUA_switch_w) { modeUA_switch(space->machine, offset, data); }
static WRITE8_HANDLER(modeDC_switch_w) { modeDC_switch(space->machine, offset, data); }
static WRITE8_HANDLER(mode3E_switch_w) { mode3E_switch(space->machine, offset, data); }
static WRITE8_HANDLER(mode3E_RAM_switch_w) { mode3E_RAM_switch(space->machine, offset, data); }
static WRITE8_HANDLER(mode3E_RAM_w)
{
	if ( mode3E_ram_enabled )
	{
		ram_base[offset] = data;
	}
}
static WRITE8_HANDLER(modeFV_switch_w) { modeFV_switch(space->machine, offset, data); }
static WRITE8_HANDLER(modeJVP_switch_w) { modeJVP_switch(space->machine, offset, data); riot_ram[ 0x20 + offset ] = data; }


static DIRECT_UPDATE_HANDLER( modeF6_opbase )
{
	if ( ( address & 0x1FFF ) >= 0x1FF6 && ( address & 0x1FFF ) <= 0x1FF9 )
	{
		modeF6_switch_w( space, ( address & 0x1FFF ) - 0x1FF6, 0 );
	}
	return address;
}

static DIRECT_UPDATE_HANDLER( modeSS_opbase )
{
	if ( address & 0x1000 )
	{
		direct->bytemask = 0x7ff;
		direct->bytestart = ( address & 0xf800 );
		direct->byteend = ( address & 0xf800 ) | 0x7ff;
		if ( address & 0x800 )
		{
			direct->decrypted = bank_base[2];
			direct->raw = bank_base[2];
		}
		else
		{
			direct->decrypted = bank_base[1];
			direct->raw = bank_base[1];
		}
		return ~0;
	}
	return address;
}

static READ8_HANDLER(modeSS_r)
{
	UINT8 data = ( offset & 0x800 ) ? bank_base[2][offset & 0x7FF] : bank_base[1][offset];

	//logerror("%04X: read from modeSS area offset = %04X\n", cpu_get_pc(cputag_get_cpu(space->machine, "maincpu")), offset);
	/* Check for control register "write" */
	if ( offset == 0xFF8 )
	{
		//logerror("%04X: write to modeSS control register data = %02X\n", cpu_get_pc(cputag_get_cpu(space->machine, "maincpu")), modeSS_byte);
		modeSS_write_enabled = modeSS_byte & 0x02;
		modeSS_write_delay = modeSS_byte >> 5;
		switch ( modeSS_byte & 0x1C )
		{
		case 0x00:
			bank_base[1] = extra_RAM + 2 * 0x800;
			bank_base[2] = ( modeSS_byte & 0x01 ) ? memory_region(space->machine, "maincpu") + 0x1800 : memory_region(space->machine, "user1");
			modeSS_high_ram_enabled = 0;
			break;
		case 0x04:
			bank_base[1] = extra_RAM;
			bank_base[2] = ( modeSS_byte & 0x01 ) ? memory_region(space->machine, "maincpu") + 0x1800 : memory_region(space->machine, "user1");
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
			bank_base[2] = ( modeSS_byte & 0x01 ) ? memory_region(space->machine, "maincpu") + 0x1800 : memory_region(space->machine, "user1");
			modeSS_high_ram_enabled = 0;
			break;
		case 0x14:
			bank_base[1] = extra_RAM + 0x800;
			bank_base[2] = ( modeSS_byte & 0x01 ) ? memory_region(space->machine, "maincpu") + 0x1800 : memory_region(space->machine, "user1");
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
		memory_set_bankptr(space->machine, "bank1", bank_base[1] );
		memory_set_bankptr(space->machine, "bank2", bank_base[2] );

		/* Check if we should stop the tape */
		if ( cpu_get_pc(cputag_get_cpu(space->machine, "maincpu")) == 0x00FD )
		{
			const device_config *img = devtag_get_device(space->machine, "cassette");
			if ( img )
			{
				cassette_change_state(img, CASSETTE_MOTOR_DISABLED, CASSETTE_MASK_MOTOR);
			}
		}
	}
	else if ( offset == 0xFF9 )
	{
		/* Cassette port read */
		double tap_val = cassette_input( devtag_get_device(space->machine, "cassette") );
		//logerror("%04X: Cassette port read, tap_val = %f\n", cpu_get_pc(cputag_get_cpu(space->machine, "maincpu")), tap_val);
		if ( tap_val < 0 )
		{
			data = 0x00;
		}
		else
		{
			data = 0x01;
		}
	}
	else
	{
		/* Possible RAM write */
		if ( modeSS_write_enabled )
		{
			int diff = cputag_get_total_cycles(space->machine, "maincpu") - modeSS_byte_started;
			//logerror("%04X: offset = %04X, %d\n", cpu_get_pc(cputag_get_cpu(space->machine, "maincpu")), offset, diff);
			if ( diff - modeSS_diff_adjust == 5 )
			{
				//logerror("%04X: RAM write offset = %04X, data = %02X\n", cpu_get_pc(cputag_get_cpu(space->machine, "maincpu")), offset, modeSS_byte );
				if ( offset & 0x800 )
				{
					if ( modeSS_high_ram_enabled )
					{
						bank_base[2][offset & 0x7FF] = modeSS_byte;
						data = modeSS_byte;
					}
				}
				else
				{
					bank_base[1][offset] = modeSS_byte;
					data = modeSS_byte;
				}
			}
			else if ( offset < 0x0100 )
			{
				modeSS_byte = offset;
				modeSS_byte_started = cputag_get_total_cycles(space->machine, "maincpu");
			}
			/* Check for dummy read from same address */
			if ( diff == 2 )
			{
				modeSS_diff_adjust = 1;
			}
			else
			{
				modeSS_diff_adjust = 0;
			}
		}
		else if ( offset < 0x0100 )
		{
			modeSS_byte = offset;
			modeSS_byte_started = cputag_get_total_cycles(space->machine, "maincpu");
		}
	}
	/* Because the mame core caches opcode data and doesn't perform reads like normal */
	/* we have to put in this little hack here to get Suicide Mission to work. */
	if ( offset != 0xFF8 && ( cpu_get_pc(cputag_get_cpu(space->machine, "maincpu")) & 0x1FFF ) == 0x1FF8 )
	{
		modeSS_r( space, 0xFF8 );
	}
	return data;
}

INLINE void modeDPC_check_flag(UINT8 data_fetcher)
{
	/* Set flag when low counter equals top */
	if ( dpc.df[data_fetcher].low == dpc.df[data_fetcher].top )
	{
		dpc.df[data_fetcher].flag = 1;
	}
	/* Reset flag when low counter equals bottom */
	if ( dpc.df[data_fetcher].low == dpc.df[data_fetcher].bottom )
	{
		dpc.df[data_fetcher].flag = 0;
	}
}

INLINE void modeDPC_decrement_counter(UINT8 data_fetcher)
{
	dpc.df[data_fetcher].low -= 1;
	if ( dpc.df[data_fetcher].low == 0xFF )
	{
		dpc.df[data_fetcher].high -= 1;
		if ( data_fetcher > 4 && dpc.df[data_fetcher].music_mode )
		{
			dpc.df[data_fetcher].low = dpc.df[data_fetcher].top;
		}
	}

	modeDPC_check_flag( data_fetcher );
}

static TIMER_CALLBACK(modeDPC_timer_callback)
{
	int data_fetcher;
	for( data_fetcher = 5; data_fetcher < 8; data_fetcher++ )
	{
		if ( dpc.df[data_fetcher].osc_clk )
		{
			modeDPC_decrement_counter( data_fetcher );
		}
	}
}

static DIRECT_UPDATE_HANDLER(modeDPC_opbase_handler)
{
	UINT8	new_bit;
	new_bit = ( dpc.shift_reg & 0x80 ) ^ ( ( dpc.shift_reg & 0x20 ) << 2 );
	new_bit = new_bit ^ ( ( ( dpc.shift_reg & 0x10 ) << 3 ) ^ ( ( dpc.shift_reg & 0x08 ) << 4 ) );
	new_bit = new_bit ^ 0x80;
	dpc.shift_reg = new_bit | ( dpc.shift_reg >> 1 );
	return address;
}

static READ8_HANDLER(modeDPC_r)
{
	static const UINT8 dpc_amplitude[8] = { 0x00, 0x04, 0x05, 0x09, 0x06, 0x0A, 0x0B, 0x0F };
	UINT8	data_fetcher = offset & 0x07;
	UINT8	data = 0xFF;

	logerror("%04X: Read from DPC offset $%02X\n", cpu_get_pc(cputag_get_cpu(space->machine, "maincpu")), offset);
	if ( offset < 0x08 )
	{
		switch( offset & 0x06 )
		{
		case 0x00:		/* Random number generator */
		case 0x02:
			return dpc.shift_reg;
		case 0x04:		/* Sound value, MOVAMT value AND'd with Draw Line Carry; with Draw Line Add */
			dpc.latch_62 = dpc.latch_64;
		case 0x06:		/* Sound value, MOVAMT value AND'd with Draw Line Carry; without Draw Line Add */
			dpc.latch_64 = dpc.latch_62 + dpc.df[4].top;
			dpc.dlc = ( dpc.latch_62 + dpc.df[4].top > 0xFF ) ? 1 : 0;
			data = 0;
			if ( dpc.df[5].music_mode && dpc.df[5].flag )
			{
				data |= 0x01;
			}
			if ( dpc.df[6].music_mode && dpc.df[6].flag )
			{
				data |= 0x02;
			}
			if ( dpc.df[7].music_mode && dpc.df[7].flag )
			{
				data |= 0x04;
			}
			return ( dpc.dlc ? dpc.movamt & 0xF0 : 0 ) | dpc_amplitude[data];
		}
	}
	else
	{
		UINT8	display_data = memory_region(space->machine, "user1")[0x2000 + ( ~ ( ( dpc.df[data_fetcher].low | ( dpc.df[data_fetcher].high << 8 ) ) ) & 0x7FF ) ];

		switch( offset & 0x38 )
		{
		case 0x08:			/* display data */
			data = display_data;
			break;
		case 0x10:			/* display data AND'd w/flag */
			data = dpc.df[data_fetcher].flag ? display_data : 0x00;
			break;
		case 0x18:			/* display data AND'd w/flag, nibbles swapped */
			data = dpc.df[data_fetcher].flag ? BITSWAP8(display_data,3,2,1,0,7,6,5,4) : 0x00;
			break;
		case 0x20:			/* display data AND'd w/flag, byte reversed */
			data = dpc.df[data_fetcher].flag ? BITSWAP8(display_data,0,1,2,3,4,5,6,7) : 0x00;
			break;
		case 0x28:			/* display data AND'd w/flag, rotated right */
			data = dpc.df[data_fetcher].flag ? ( display_data >> 1 ) : 0x00;
			break;
		case 0x30:			/* display data AND'd w/flag, rotated left */
			data = dpc.df[data_fetcher].flag ? ( display_data << 1 ) : 0x00;
			break;
		case 0x38:			/* flag */
			data = dpc.df[data_fetcher].flag ? 0xFF : 0x00;
			break;
		}

		if ( data_fetcher < 5 || ! dpc.df[data_fetcher].osc_clk )
		{
			modeDPC_decrement_counter( data_fetcher );
		}
	}
	return data;
}

static WRITE8_HANDLER(modeDPC_w)
{
	UINT8	data_fetcher = offset & 0x07;

	switch( offset & 0x38 )
	{
	case 0x00:			/* Top count */
		dpc.df[data_fetcher].top = data;
		dpc.df[data_fetcher].flag = 0;
		modeDPC_check_flag( data_fetcher );
		break;
	case 0x08:			/* Bottom count */
		dpc.df[data_fetcher].bottom = data;
		modeDPC_check_flag( data_fetcher );
		break;
	case 0x10:			/* Counter low */
		dpc.df[data_fetcher].low = data;
		if ( data_fetcher == 4 )
		{
			dpc.latch_64 = data;
		}
		if ( data_fetcher > 4 && dpc.df[data_fetcher].music_mode )
		{
			dpc.df[data_fetcher].low = dpc.df[data_fetcher].top;
		}
		modeDPC_check_flag( data_fetcher );
		break;
	case 0x18:			/* Counter high */
		dpc.df[data_fetcher].high = data;
		dpc.df[data_fetcher].music_mode = data & 0x10;
		dpc.df[data_fetcher].osc_clk = data & 0x20;
		if ( data_fetcher > 4 && dpc.df[data_fetcher].music_mode && dpc.df[data_fetcher].low == 0xFF )
		{
			dpc.df[data_fetcher].low = dpc.df[data_fetcher].top;
			modeDPC_check_flag( data_fetcher );
		}
		break;
	case 0x20:			/* Draw line movement value / MOVAMT */
		dpc.movamt = data;
		break;
	case 0x28:			/* Not used */
		logerror("%04X: Write to unused DPC register $%02X, data $%02X\n", cpu_get_pc(cputag_get_cpu(space->machine, "maincpu")), offset, data);
		break;
	case 0x30:			/* Random number generator reset */
		dpc.shift_reg = 0;
		break;
	case 0x38:			/* Not used */
		logerror("%04X: Write to unused DPC register $%02X, data $%02X\n", cpu_get_pc(cputag_get_cpu(space->machine, "maincpu")), offset, data);
		break;
	}
}

/*

There seems to be a kind of lag between the writing to address 0x1FE and the
Activision switcher springing into action. It waits for the next byte to arrive
on the data bus, which is the new PCH in the case of a JSR, and the PCH of the
stored PC on the stack in the case of an RTS.

depending on last byte & 0x20 -> 0x00 -> switch to bank #1
                              -> 0x20 -> switch to bank #0

 */

static direct_update_func FE_old_opbase_handler;
static int FETimer;

static DIRECT_UPDATE_HANDLER(modeFE_opbase_handler)
{
	if ( ! FETimer )
	{
		/* Still cheating a bit here by looking bit 13 of the address..., but the high byte of the
           cpu should be the last byte that was on the data bus and so should determine the bank
           we should switch in. */
		bank_base[1] = memory_region(space->machine, "user1") + 0x1000 * ( ( address & 0x2000 ) ? 0 : 1 );
		memory_set_bankptr(space->machine, "bank1", bank_base[1] );
		/* and restore old opbase handler */
		memory_set_direct_update_handler(space, FE_old_opbase_handler);
	}
	else
	{
		/* Wait for one memory access to have passed (reading of new PCH either from code or from stack) */
		FETimer--;
	}
	return address;
}

static void modeFE_switch(running_machine *machine,UINT16 offset, UINT8 data)
{
	const address_space* space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	/* Retrieve last byte read by the cpu (for this mapping scheme this
       should be the last byte that was on the data bus
    */
	FETimer = 1;
	FE_old_opbase_handler = memory_set_direct_update_handler(space, modeFE_opbase_handler);
}

static READ8_HANDLER(modeFE_switch_r)
{
	modeFE_switch(space->machine,offset, 0 );
	return memory_read_byte(space,  0xFE );
}

static WRITE8_HANDLER(modeFE_switch_w)
{
	memory_write_byte(space,  0xFE, data );
	modeFE_switch(space->machine,offset, 0 );
}

static  READ8_HANDLER(current_bank_r)
{
	return current_bank;
}

static ADDRESS_MAP_START(a2600_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_GLOBAL_MASK(0x1fff)
	AM_RANGE(0x0000, 0x007F) AM_MIRROR(0x0F00) AM_READWRITE(tia_r, tia_w)
	AM_RANGE(0x0080, 0x00FF) AM_MIRROR(0x0D00) AM_RAM AM_BASE(&riot_ram)
	AM_RANGE(0x0280, 0x029F) AM_MIRROR(0x0D00) AM_DEVREADWRITE("riot", riot6532_r, riot6532_w)
	AM_RANGE(0x1000, 0x1FFF)                   AM_ROMBANK("bank1")
ADDRESS_MAP_END

static WRITE8_DEVICE_HANDLER(switch_A_w)
{
	running_machine *machine = device->machine;

	/* Left controller port */
	if ( input_port_read(machine, "CONTROLLERS") / CATEGORY_SELECT == 0x03 )
	{
		keypad_left_column = data / 16;
	}

	/* Right controller port */
	switch( input_port_read(machine, "CONTROLLERS") % CATEGORY_SELECT )
	{
	case 0x03:	/* Keypad */
		keypad_right_column = data & 0x0F;
		break;
	case 0x0a:	/* KidVid voice module */
		cassette_change_state( devtag_get_device(machine, "cassette"), ( data & 0x02 ) ? CASSETTE_MOTOR_DISABLED : CASSETTE_MOTOR_ENABLED | CASSETTE_PLAY, CASSETTE_MOTOR_DISABLED );
		break;
	}
}

static READ8_DEVICE_HANDLER( switch_A_r )
{
	static const UINT8 driving_lookup[4] = { 0x00, 0x02, 0x03, 0x01 };
	running_machine *machine = device->machine;
	UINT8 val = 0;

	/* Left controller port PINs 1-4 ( 4321 ) */
	switch( input_port_read(machine, "CONTROLLERS") / CATEGORY_SELECT )
	{
	case 0x00:  /* Joystick */
	case 0x05:	/* Joystick w/Boostergrip */
		val |= input_port_read(machine, "SWA_JOY") & 0xF0;
		break;
	case 0x01:  /* Paddle */
		val |= input_port_read(machine, "SWA_PAD") & 0xF0;
		break;
	case 0x02:	/* Driving */
		val |= 0xC0;
		val |= ( driving_lookup[ ( input_port_read(machine, "WHEEL_L") & 0x18 ) >> 3 ] << 4 );
		break;
	case 0x06:	/* Trakball CX-22 */
	case 0x07:	/* Trakball CX-80 / ST mouse */
	case 0x09:	/* Amiga mouse */
	default:
		val |= 0xF0;
		break;
	}

	/* Right controller port PINs 1-4 ( 4321 ) */
	switch( input_port_read(machine, "CONTROLLERS") % CATEGORY_SELECT )
	{
	case 0x00:	/* Joystick */
	case 0x05:	/* Joystick w/Boostergrip */
		val |= input_port_read(machine, "SWA_JOY") & 0x0F;
		break;
	case 0x01:	/* Paddle */
		val |= input_port_read(machine, "SWA_PAD") & 0x0F;
		break;
	case 0x02:	/* Driving */
		val |= 0x0C;
		val |= ( driving_lookup[ ( input_port_read(machine, "WHEEL_R") & 0x18 ) >> 3 ] );
		break;
	case 0x06:	/* Trakball CX-22 */
	case 0x07:	/* Trakball CX-80 / ST mouse */
	case 0x09:	/* Amiga mouse */
	default:
		val |= 0x0F;
		break;
	}

	return val;
}

static WRITE8_DEVICE_HANDLER(switch_B_w )
{
}

static WRITE_LINE_DEVICE_HANDLER( irq_callback )
{
}

static READ8_DEVICE_HANDLER( riot_input_port_8_r )
{
	return input_port_read(device->machine, "SWB");
}

static const riot6532_interface r6532_interface =
{
	DEVCB_HANDLER(switch_A_r),
	DEVCB_HANDLER(riot_input_port_8_r),
	DEVCB_HANDLER(switch_A_w),
	DEVCB_HANDLER(switch_B_w),
	DEVCB_LINE(irq_callback)
};


static void install_banks(running_machine *machine, int count, unsigned init)
{
	int i;
	UINT8 *cart = CART;

	for (i = 0; i < count; i++)
	{
		static const char *handler[] =
		{
			"bank1",
			"bank2",
			"bank3",
			"bank4",
		};

		memory_install_read_bank(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM),
			0x1000 + (i + 0) * 0x1000 / count - 0,
			0x1000 + (i + 1) * 0x1000 / count - 1, 0, 0, handler[i]);

		bank_base[i + 1] = cart + init;
		memory_set_bankptr(machine,handler[i], bank_base[i + 1]);
	}
}

static READ16_HANDLER(a2600_read_input_port)
{
	int i;

	switch( offset )
	{
	case 0:	/* Left controller port PIN 5 */
		switch ( input_port_read(space->machine, "CONTROLLERS") / CATEGORY_SELECT )
		{
		case 0x00:	/* Joystick */
			return TIA_INPUT_PORT_ALWAYS_OFF;
		case 0x01:	/* Paddle */
			return input_port_read(space->machine, "PADDLE1");
		case 0x03:	/* Keypad */
			for ( i = 0; i < 4; i++ )
			{
				if ( ! ( ( keypad_left_column >> i ) & 0x01 ) )
				{
					if ( ( input_port_read(space->machine, "KEYPAD_L") >> 3*i ) & 0x01 )
					{
						return TIA_INPUT_PORT_ALWAYS_ON;
					}
					else
					{
						return TIA_INPUT_PORT_ALWAYS_OFF;
					}
				}
			}
			return TIA_INPUT_PORT_ALWAYS_ON;
		case 0x05:	/* Boostergrip joystick */
			return ( input_port_read(space->machine, "BUTTONS_L") & 0x40 ) ? TIA_INPUT_PORT_ALWAYS_OFF : TIA_INPUT_PORT_ALWAYS_ON;
		default:
			return TIA_INPUT_PORT_ALWAYS_OFF;
		}
		break;
	case 1:	/* Right controller port PIN 5 */
		switch ( input_port_read(space->machine, "CONTROLLERS") / CATEGORY_SELECT )
		{
		case 0x00:	/* Joystick */
			return TIA_INPUT_PORT_ALWAYS_OFF;
		case 0x01:	/* Paddle */
			return input_port_read(space->machine, "PADDLE3");
		case 0x03:	/* Keypad */
			for ( i = 0; i < 4; i++ )
			{
				if ( ! ( ( keypad_left_column >> i ) & 0x01 ) )
				{
					if ( ( input_port_read(space->machine, "KEYPAD_L") >> 3*i ) & 0x02 )
					{
						return TIA_INPUT_PORT_ALWAYS_ON;
					}
					else
					{
						return TIA_INPUT_PORT_ALWAYS_OFF;
					}
				}
			}
			return TIA_INPUT_PORT_ALWAYS_ON;
		case 0x05:	/* Joystick w/Boostergrip */
			return ( input_port_read(space->machine, "BUTTONS_L") & 0x20 ) ? TIA_INPUT_PORT_ALWAYS_OFF : TIA_INPUT_PORT_ALWAYS_ON;
		default:
			return TIA_INPUT_PORT_ALWAYS_OFF;
		}
		break;
	case 2:	/* Left controller port PIN 9 */
		switch ( input_port_read(space->machine, "CONTROLLERS") % CATEGORY_SELECT )
		{
		case 0x00:	/* Joystick */
			return TIA_INPUT_PORT_ALWAYS_OFF;
		case 0x01:	/* Paddle */
			return input_port_read(space->machine, "PADDLE2");
		case 0x03:	/* Keypad */
			for ( i = 0; i < 4; i++ )
			{
				if ( ! ( ( keypad_right_column >> i ) & 0x01 ) )
				{
					if ( ( input_port_read(space->machine, "KEYPAD_R") >> 3*i ) & 0x01 )
					{
						return TIA_INPUT_PORT_ALWAYS_ON;
					}
					else
					{
						return TIA_INPUT_PORT_ALWAYS_OFF;
					}
				}
			}
			return TIA_INPUT_PORT_ALWAYS_ON;
		case 0x05:	/* Joystick w/Boostergrip */
			return ( input_port_read(space->machine, "BUTTONS_R") & 0x40 ) ? TIA_INPUT_PORT_ALWAYS_OFF : TIA_INPUT_PORT_ALWAYS_ON;
		default:
			return TIA_INPUT_PORT_ALWAYS_OFF;
		}
		break;
	case 3:	/* Right controller port PIN 9 */
		switch ( input_port_read(space->machine, "CONTROLLERS") % CATEGORY_SELECT )
		{
		case 0x00:	/* Joystick */
			return TIA_INPUT_PORT_ALWAYS_OFF;
		case 0x01:	/* Paddle */
			return input_port_read(space->machine, "PADDLE4");
		case 0x03:	/* Keypad */
			for ( i = 0; i < 4; i++ )
			{
				if ( ! ( ( keypad_right_column >> i ) & 0x01 ) )
				{
					if ( ( input_port_read(space->machine, "KEYPAD_R") >> 3*i ) & 0x02 )
					{
						return TIA_INPUT_PORT_ALWAYS_ON;
					}
					else
					{
						return TIA_INPUT_PORT_ALWAYS_OFF;
					}
				}
			}
			return TIA_INPUT_PORT_ALWAYS_ON;
		case 0x05:	/* Joystick w/Boostergrip */
			return ( input_port_read(space->machine, "BUTTONS_R") & 0x20 ) ? TIA_INPUT_PORT_ALWAYS_OFF : TIA_INPUT_PORT_ALWAYS_ON;
		default:
			return TIA_INPUT_PORT_ALWAYS_OFF;
		}
		break;
	case 4:	/* Left controller port PIN 6 */
		switch ( input_port_read(space->machine, "CONTROLLERS") / CATEGORY_SELECT )
		{
		case 0x00:	/* Joystick */
		case 0x05:	/* Joystick w/Boostergrip */
			return input_port_read(space->machine, "BUTTONS_L");
		case 0x01:	/* Paddle */
			return 0xff;
		case 0x02:	/* Driving */
			return input_port_read(space->machine, "BUTTONS_L") << 3;
		case 0x03:	/* Keypad */
			for ( i = 0; i < 4; i++ )
			{
				if ( ! ( ( keypad_left_column >> i ) & 0x01 ) )
				{
					if ( ( input_port_read(space->machine, "KEYPAD_L") >> 3*i ) & 0x04 )
					{
						return 0xff;
					}
					else
					{
						return 0x00;
					}
				}
			}
			return 0xff;
		case 0x06:	/* Trakball CX-22 */
			return input_port_read(space->machine, "BUTTONS_L") << 4;
		default:
			return 0xff;
		}
		break;
	case 5:	/* Right controller port PIN 6 */
		switch ( input_port_read(space->machine, "CONTROLLERS") % CATEGORY_SELECT )
		{
		case 0x00:	/* Joystick */
		case 0x05:	/* Joystick w/Boostergrip */
			return input_port_read(space->machine, "BUTTONS_R");
		case 0x01:	/* Paddle */
			return 0xff;
		case 0x02:	/* Driving */
			return input_port_read(space->machine, "BUTTONS_R") << 3;
		case 0x03:	/* Keypad */
			for ( i = 0; i < 4; i++ )
			{
				if ( ! ( ( keypad_right_column >> i ) & 0x01 ) )
				{
					if ( ( input_port_read(space->machine, "KEYPAD_R") >> 3*i ) & 0x04 )
					{
						return 0xff;
					}
					else
					{
						return 0x00;
					}
				}
			}
			return 0xff;
		case 0x06:	/* Trakball CX-22 */
			return input_port_read(space->machine, "BUTTONS_R") << 4;
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
static READ8_HANDLER(a2600_get_databus_contents)
{
	UINT16	last_address, prev_address;
	UINT8	last_byte, prev_byte;

	last_address = cpu_get_pc(cputag_get_cpu(space->machine, "maincpu")) - 1;
	if ( ! ( last_address & 0x1080 ) )
	{
		return offset;
	}
	last_byte = memory_read_byte(space,  last_address );
	if ( last_byte < 0x80 || last_byte == 0xFF )
	{
		return last_byte;
	}
	prev_address = last_address - 1;
	if ( ! ( prev_address & 0x1080 ) )
	{
		return last_byte;
	}
	prev_byte = memory_read_byte(space,  prev_address );
	if ( prev_byte == 0xB1 )
	{	/* LDA (XX),Y */
		return memory_read_byte(space,  last_byte + 1 );
	}
	return last_byte;
}

static const rectangle visarea[4] = {
	{ 26, 26 + 160 + 16, 24, 24 + 192 + 31 },	/* 262 */
	{ 26, 26 + 160 + 16, 32, 32 + 228 + 31 },	/* 312 */
	{ 26, 26 + 160 + 16, 45, 45 + 240 + 31 },	/* 328 */
	{ 26, 26 + 160 + 16, 48, 48 + 240 + 31 }	/* 342 */
};

static WRITE16_HANDLER( a2600_tia_vsync_callback )
{
	int i;

	for ( i = 0; i < ARRAY_LENGTH(supported_screen_heights); i++ )
	{
		if ( data >= supported_screen_heights[i] - 3 && data <= supported_screen_heights[i] + 3 )
		{
			if ( supported_screen_heights[i] != current_screen_height )
			{
				current_screen_height = supported_screen_heights[i];
//              video_screen_configure( machine->primary_screen, 228, current_screen_height, &visarea[i], HZ_TO_ATTOSECONDS( MASTER_CLOCK_NTSC ) * 228 * current_screen_height );
			}
		}
	}
}

static WRITE16_HANDLER( a2600_tia_vsync_callback_pal )
{
	int i;

	for ( i = 0; i < ARRAY_LENGTH(supported_screen_heights); i++ )
	{
		if ( data >= supported_screen_heights[i] - 3 && data <= supported_screen_heights[i] + 3 )
		{
			if ( supported_screen_heights[i] != current_screen_height )
			{
				current_screen_height = supported_screen_heights[i];
//              video_screen_configure( machine->primary_screen, 228, current_screen_height, &visarea[i], HZ_TO_ATTOSECONDS( MASTER_CLOCK_PAL ) * 228 * current_screen_height );
			}
		}
	}
}

static const struct tia_interface tia_interface =
{
	a2600_read_input_port,
	a2600_get_databus_contents,
	a2600_tia_vsync_callback
};

static const struct tia_interface tia_interface_pal =
{
	a2600_read_input_port,
	a2600_get_databus_contents,
	a2600_tia_vsync_callback_pal
};

static MACHINE_START( a2600 )
{
	const device_config *screen = video_screen_first(machine->config);
	current_screen_height = video_screen_get_height(screen);
	extra_RAM = memory_region_alloc( machine, "user2", 0x8600, ROM_REQUIRED );
	tia_init( machine, &tia_interface );
	memset( riot_ram, 0x00, 0x80 );
	current_reset_bank_counter = 0xFF;
}

static MACHINE_START( a2600p )
{
	const device_config *screen = video_screen_first(machine->config);
	current_screen_height = video_screen_get_height(screen);
	extra_RAM = memory_region_alloc( machine, "user2", 0x8600, ROM_REQUIRED );
	tia_init( machine, &tia_interface_pal );
	memset( riot_ram, 0x00, 0x80 );
	current_reset_bank_counter = 0xFF;
}

static void set_category_value( running_machine *machine, const char* cat, const char *cat_selection )
{
	/* NPW 22-May-2008 - FIXME */
#if 0
	input_port_entry	*cat_in = NULL;
	input_port_entry	*in;

	for( in = machine->input_ports; in->type != IPT_END; in++ )
	{
		if ( in->type == IPT_CATEGORY_NAME && ! mame_stricmp( cat, input_port_name(in) ) )
		{
			cat_in = in;
		}
		if ( cat_in && in->type == IPT_CATEGORY_SETTING && ! mame_stricmp( cat_selection, input_port_name(in) ) )
		{
			cat_in->default_value = in->default_value;
			return;
		}
	}
#endif
}

static void set_controller( running_machine *machine, const char *controller, unsigned int selection )
{
	/* Defaulting to only joystick when joysstick and paddle are set for now... */
	if ( selection == JOYS + PADD )
		selection = JOYS;

	switch( selection )
	{
	case JOYS:	set_category_value( machine, controller, "Joystick" ); break;
	case PADD:	set_category_value( machine, controller, STR_PADDLES ); break;
	case KEYP:	set_category_value( machine, controller, STR_KEYPAD ); break;
	case LGUN:	set_category_value( machine, controller, STR_LIGHTGUN ); break;
	case INDY:	set_category_value( machine, controller, STR_DRIVING ); break;
	case BOOS:	set_category_value( machine, controller, STR_BOOSTERGRIP ); break;
	case KVID:	set_category_value( machine, controller, STR_KIDVID ); break;
	case CMTE:	break;
	case MLNK:	break;
	case AMSE:	set_category_value( machine, controller, STR_AMIGAMOUSE ); break;
	case CX22:	set_category_value( machine, controller, STR_CX22TRAKBALL ); break;
	case CX80:	set_category_value( machine, controller, STR_CX80TRAKBALL ); break;
	}
}

static MACHINE_RESET( a2600 )
{
	const address_space* space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	int chip = 0xFF;
	unsigned long controltemp;
	static const unsigned char snowwhite[] = { 0x10, 0xd0, 0xff, 0xff }; // Snow White Proto

	current_reset_bank_counter++;

	/* auto-detect special controllers */
	controltemp = detect_2600controllers(machine);
	set_controller( machine, STR_LEFT_CONTROLLER, controltemp >> 16 );
	set_controller( machine, STR_RIGHT_CONTROLLER, controltemp & 0xFFFF );

	/* auto-detect bank mode */

	if (banking_mode == 0xff) if (detect_modeDC(machine)) banking_mode = modeDC;
	if (banking_mode == 0xff) if (detect_mode3E(machine)) banking_mode = mode3E;
	if (banking_mode == 0xff) if (detect_modeFE(machine)) banking_mode = modeFE;
	if (banking_mode == 0xff) if (detect_modeSS(machine)) banking_mode = modeSS;
	if (banking_mode == 0xff) if (detect_modeE0(machine)) banking_mode = modeE0;
	if (banking_mode == 0xff) if (detect_modeCV(machine)) banking_mode = modeCV;
	if (banking_mode == 0xff) if (detect_modeFV(machine)) banking_mode = modeFV;
	if (banking_mode == 0xff) if (detect_modeJVP(machine)) banking_mode = modeJVP;
	if (banking_mode == 0xff) if (detect_modeUA(machine)) banking_mode = modeUA;
	if (banking_mode == 0xff) if (detect_8K_mode3F(machine)) banking_mode = mode3F;
	if (banking_mode == 0xff) if (detect_32K_mode3F(machine)) banking_mode = mode3F;
	if (banking_mode == 0xff) if (detect_modeE7(machine)) banking_mode = modeE7;

	if (banking_mode == 0xff)
	{
		switch (cart_size)
		{
		case 0x800:
			banking_mode = mode2K;
			break;
		case 0x1000:
			banking_mode = mode4K;
			break;
		case 0x2000:
			banking_mode = modeF8;
			break;
		case 0x28FF:
		case 0x2900:
			banking_mode = modeDPC;
			break;
		case 0x3000:
			banking_mode = modeFA;
			break;
		case 0x4000:
			banking_mode = modeF6;
			break;
		case 0x8000:
			banking_mode = modeF4;
			break;
		case 0x10000:
			banking_mode = mode32in1;
			break;
		case 0x80000:
			banking_mode = mode3F;
			break;
		}
	}

	/* auto-detect super chip */

	chip = 0;

	if (cart_size == 0x2000 || cart_size == 0x4000 || cart_size == 0x8000)
	{
		chip = detect_super_chip(machine);
	}

	/* Super chip games:
       dig dig, crystal castles, millipede, stargate, defender ii, jr. Pac Man,
       desert falcon, dark chambers, super football, sprintmaster, fatal run,
       off the wall, shooting arcade, secret quest, radar lock, save mary, klax
    */

	/* set up ROM banks */

	switch (banking_mode)
	{
	case mode2K:
		install_banks(machine, 2, 0x0000);
		break;

	case mode4K:
		install_banks(machine, 1, 0x0000);
		break;

	case modeF8:
		if (!memcmp(&CART[0x1ffc],snowwhite,sizeof(snowwhite)))
		{
			install_banks(machine, 1, 0x0000);
		}
		else
		{
			install_banks(machine, 1, 0x1000);
		}
		break;

	case modeFA:
		install_banks(machine, 1, 0x2000);
		break;

	case modeF6:
		install_banks(machine, 1, 0x0000);
		break;

	case modeF4:
		install_banks(machine, 1, 0x7000);
		break;

	case modeFE:
		install_banks(machine, 1, 0x0000);
		break;

	case modeE0:
		install_banks(machine, 4, 0x1c00);
		break;

	case mode3F:
		install_banks(machine, 2, cart_size - 0x800);
		number_banks = cart_size / 0x800;
		break;

	case modeUA:
		install_banks(machine, 1, 0x1000);
		break;

	case modeE7:
		install_banks(machine, 2, 0x3800);
		break;

	case modeDC:
		install_banks(machine, 1, 0x1000 * current_bank);
		break;

	case modeCV:
		install_banks(machine, 2, 0x0000);
		break;

	case mode3E:
		install_banks(machine, 2, cart_size - 0x800);
		number_banks = cart_size / 0x800;
		mode3E_ram_enabled = 0;
		break;

	case modeSS:
		install_banks(machine, 2, 0x0000);
		break;

	case modeFV:
		install_banks(machine, 1, 0x0000);
		current_bank = 0;
		break;

	case modeDPC:
		install_banks(machine, 1, 0x0000);
		break;

	case mode32in1:
		install_banks(machine, 2, 0x0000);
		current_reset_bank_counter = current_reset_bank_counter & 0x1F;
		break;

	case modeJVP:
		current_reset_bank_counter = current_reset_bank_counter & 1;
		if ( cart_size == 0x2000 )
			current_reset_bank_counter = 0;
		current_bank = current_reset_bank_counter * 2;
		install_banks(machine, 1, 0x1000 * current_bank);
		break;
	}

	/* set up bank counter */

	if (banking_mode == modeDC)
	{
		memory_install_read8_handler(space, 0x1fec, 0x1fec, 0, 0, current_bank_r);
	}

	/* set up bank switch registers */

	switch (banking_mode)
	{
	case modeF8:
		memory_install_write8_handler(space, 0x1ff8, 0x1ff9, 0, 0, modeF8_switch_w);
		memory_install_read8_handler(space, 0x1ff8, 0x1ff9, 0, 0, modeF8_switch_r);
		break;

	case modeFA:
		memory_install_write8_handler(space, 0x1ff8, 0x1ffa, 0, 0, modeFA_switch_w);
		memory_install_read8_handler(space, 0x1ff8, 0x1ffa, 0, 0, modeFA_switch_r);
		break;

	case modeF6:
		memory_install_write8_handler(space, 0x1ff6, 0x1ff9, 0, 0, modeF6_switch_w);
		memory_install_read8_handler(space, 0x1ff6, 0x1ff9, 0, 0, modeF6_switch_r);
		memory_set_direct_update_handler(space, modeF6_opbase );
		break;

	case modeF4:
		memory_install_write8_handler(space, 0x1ff4, 0x1ffb, 0, 0, modeF4_switch_w);
		memory_install_read8_handler(space, 0x1ff4, 0x1ffb, 0, 0, modeF4_switch_r);
		break;

	case modeE0:
		memory_install_write8_handler(space, 0x1fe0, 0x1ff8, 0, 0, modeE0_switch_w);
		memory_install_read8_handler(space, 0x1fe0, 0x1ff8, 0, 0, modeE0_switch_r);
		break;

	case mode3F:
		memory_install_write8_handler(space, 0x00, 0x3f, 0, 0, mode3F_switch_w);
		break;

	case modeUA:
		memory_install_write8_handler(space, 0x200, 0x27f, 0, 0, modeUA_switch_w);
		memory_install_read8_handler(space, 0x200, 0x27f, 0, 0, modeUA_switch_r);
		break;

	case modeE7:
		memory_install_write8_handler(space, 0x1fe0, 0x1fe7, 0, 0, modeE7_switch_w);
		memory_install_read8_handler(space, 0x1fe0, 0x1fe7, 0, 0, modeE7_switch_r);
		memory_install_write8_handler(space, 0x1fe8, 0x1feb, 0, 0, modeE7_RAM_switch_w);
		memory_install_read8_handler(space, 0x1fe8, 0x1feb, 0, 0, modeE7_RAM_switch_r);
		memory_install_readwrite_bank(space, 0x1800, 0x18ff, 0, 0, "bank9");
		memory_set_bankptr(machine, "bank9", extra_RAM + 4 * 256 );
		break;

	case modeDC:
		memory_install_write8_handler(space, 0x1ff0, 0x1ff0, 0, 0, modeDC_switch_w);
		memory_install_read8_handler(space, 0x1ff0, 0x1ff0, 0, 0, modeDC_switch_r);
		break;

	case modeFE:
		memory_install_write8_handler(space, 0x01fe, 0x01fe, 0, 0, modeFE_switch_w);
		memory_install_read8_handler(space, 0x01fe, 0x01fe, 0, 0, modeFE_switch_r);
		break;

	case mode3E:
		memory_install_write8_handler(space, 0x3e, 0x3e, 0, 0, mode3E_RAM_switch_w);
		memory_install_write8_handler(space, 0x3f, 0x3f, 0, 0, mode3E_switch_w);
		memory_install_write8_handler(space, 0x1400, 0x15ff, 0, 0, mode3E_RAM_w);
		break;

	case modeSS:
		memory_install_read8_handler(space, 0x1000, 0x1fff, 0, 0, modeSS_r);
		bank_base[1] = extra_RAM + 2 * 0x800;
		bank_base[2] = CART;
		memory_set_bankptr(machine, "bank1", bank_base[1] );
		memory_set_bankptr(machine, "bank2", bank_base[2] );
		modeSS_write_enabled = 0;
		modeSS_byte_started = 0;
		memory_set_direct_update_handler(space, modeSS_opbase );
		/* Already start the motor of the cassette for the user */
		cassette_change_state( devtag_get_device(machine, "cassette"), CASSETTE_MOTOR_ENABLED, CASSETTE_MOTOR_DISABLED );
		break;

	case modeFV:
		memory_install_write8_handler(space, 0x1fd0, 0x1fd0, 0, 0, modeFV_switch_w);
		memory_install_read8_handler(space, 0x1fd0, 0x1fd0, 0, 0, modeFV_switch_r);
		break;

	case modeDPC:
		memory_install_read8_handler(space, 0x1000, 0x103f, 0, 0, modeDPC_r);
		memory_install_write8_handler(space, 0x1040, 0x107f, 0, 0, modeDPC_w);
		memory_install_write8_handler(space, 0x1ff8, 0x1ff9, 0, 0, modeF8_switch_w);
		memory_install_read8_handler(space, 0x1ff8, 0x1ff9, 0, 0, modeF8_switch_r);
		memory_set_direct_update_handler(space, modeDPC_opbase_handler );
		{
			int	data_fetcher;
			for( data_fetcher = 0; data_fetcher < 8; data_fetcher++ )
			{
				dpc.df[data_fetcher].osc_clk = 0;
				dpc.df[data_fetcher].flag = 0;
				dpc.df[data_fetcher].music_mode = 0;
			}
		}
		dpc.oscillator = timer_alloc(machine,  modeDPC_timer_callback , NULL);
		timer_adjust_periodic(dpc.oscillator, ATTOTIME_IN_HZ(42000), 0, ATTOTIME_IN_HZ(42000));
		break;

	case mode32in1:
		memory_set_bankptr(machine, "bank1", CART + current_reset_bank_counter * 0x800 );
		memory_set_bankptr(machine, "bank2", CART + current_reset_bank_counter * 0x800 );
		break;

	case modeJVP:
		memory_install_read8_handler(space, 0x0FA0, 0x0FC0, 0, 0, modeJVP_switch_r);
		memory_install_write8_handler(space, 0x0FA0, 0x0FC0, 0, 0, modeJVP_switch_w);
		break;
	}

	/* set up extra RAM */

	if (banking_mode == modeFA)
	{
		memory_install_write_bank(space, 0x1000, 0x10ff, 0, 0, "bank9");
		memory_install_read_bank(space, 0x1100, 0x11ff, 0, 0, "bank9");

		memory_set_bankptr(machine,"bank9", extra_RAM);
	}

	if (banking_mode == modeCV)
	{
		memory_install_write_bank(space, 0x1400, 0x17ff, 0, 0, "bank9");
		memory_install_read_bank(space, 0x1000, 0x13ff, 0, 0, "bank9");

		memory_set_bankptr(machine,"bank9", extra_RAM);
	}

	if (chip)
	{
		memory_install_write_bank(space, 0x1000, 0x107f, 0, 0, "bank9");
		memory_install_read_bank(space, 0x1080, 0x10ff, 0, 0, "bank9");

		memory_set_bankptr(machine,"bank9", extra_RAM);
	}

	/* Banks may have changed, reset the cpu so it uses the correct reset vector */
	device_reset( cputag_get_cpu(machine, "maincpu") );
}


static INPUT_PORTS_START( a2600 )

	PORT_START("PADDLE1") /* [0] */
	PORT_BIT( 0xff, 0x80, IPT_PADDLE) PORT_SENSITIVITY(40) PORT_KEYDELTA(10) PORT_CENTERDELTA(0) PORT_MINMAX(0,255) PORT_CATEGORY(11) PORT_PLAYER(1) PORT_REVERSE
	PORT_START("PADDLE3") /* [1] */
	PORT_BIT( 0xff, 0x80, IPT_PADDLE) PORT_SENSITIVITY(40) PORT_KEYDELTA(10) PORT_CENTERDELTA(0) PORT_MINMAX(0,255) PORT_CATEGORY(11) PORT_PLAYER(3) PORT_REVERSE
	PORT_START("PADDLE2") /* [2] */
	PORT_BIT( 0xff, 0x80, IPT_PADDLE) PORT_SENSITIVITY(40) PORT_KEYDELTA(10) PORT_CENTERDELTA(0) PORT_MINMAX(0,255) PORT_CATEGORY(21) PORT_PLAYER(2) PORT_REVERSE
	PORT_START("PADDLE4") /* [3] */
	PORT_BIT( 0xff, 0x80, IPT_PADDLE) PORT_SENSITIVITY(40) PORT_KEYDELTA(10) PORT_CENTERDELTA(0) PORT_MINMAX(0,255) PORT_CATEGORY(21) PORT_PLAYER(4) PORT_REVERSE PORT_CODE_DEC(KEYCODE_4_PAD) PORT_CODE_INC(KEYCODE_6_PAD)

	PORT_START("BUTTONS_L") /* [4] left port button(s) */
//  PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_BUTTON3) PORT_CATEGORY(15) PORT_PLAYER(1)
//  PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2) PORT_CATEGORY(15) PORT_PLAYER(1)
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_CATEGORY(15) PORT_PLAYER(1)
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_CATEGORY(12) PORT_PLAYER(1)
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3) PORT_CATEGORY(10) PORT_PLAYER(1)
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2) PORT_CATEGORY(10) PORT_PLAYER(1)
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_CATEGORY(10) PORT_PLAYER(1)

	PORT_START("BUTTONS_R") /* [5] right port button(s) */
//  PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_BUTTON3) PORT_CATEGORY(25) PORT_PLAYER(2)
//  PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2) PORT_CATEGORY(25) PORT_PLAYER(2)
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_CATEGORY(25) PORT_PLAYER(2)
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_CATEGORY(22) PORT_PLAYER(2)
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3) PORT_CATEGORY(20) PORT_PLAYER(2)
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2) PORT_CATEGORY(20) PORT_PLAYER(2)
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_CATEGORY(20) PORT_PLAYER(2)

	PORT_START("SWA_JOY") /* [6] SWCHA joystick */
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP) PORT_CATEGORY(20) PORT_PLAYER(2)
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN) PORT_CATEGORY(20) PORT_PLAYER(2)
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT) PORT_CATEGORY(20) PORT_PLAYER(2)
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT) PORT_CATEGORY(20) PORT_PLAYER(2)
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP) PORT_CATEGORY(10) PORT_PLAYER(1)
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN) PORT_CATEGORY(10) PORT_PLAYER(1)
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT) PORT_CATEGORY(10) PORT_PLAYER(1)
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT) PORT_CATEGORY(10) PORT_PLAYER(1)

	PORT_START("SWA_PAD") /* [7] SWCHA paddles */
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_CATEGORY(21) PORT_PLAYER(4)
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_CATEGORY(21) PORT_PLAYER(2)
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_CATEGORY(11) PORT_PLAYER(3)
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_CATEGORY(11) PORT_PLAYER(1)

	PORT_START("SWB") /* [8] SWCHB */
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

	PORT_START("CONTROLLERS") /* [9] */
	PORT_CATEGORY_CLASS( 0xf0, 0x00, STR_LEFT_CONTROLLER )
	PORT_CATEGORY_ITEM(    0x00, DEF_STR( Joystick ), 10 )
	PORT_CATEGORY_ITEM(    0x10, STR_PADDLES, 11 )
	PORT_CATEGORY_ITEM(    0x20, STR_DRIVING, 12 )
	PORT_CATEGORY_ITEM(    0x30, STR_KEYPAD, 13 )
	//PORT_CATEGORY_ITEM(    0x40, STR_LIGHTGUN, 14 )
	PORT_CATEGORY_ITEM(    0x50, STR_BOOSTERGRIP, 10 )
	//PORT_CATEGORY_ITEM(    0x60, STR_CX22TRAKBALL, 15 )
	//PORT_CATEGORY_ITEM(    0x70, STR_CX80TRAKBALL, 15 )
	//PORT_CATEGORY_ITEM(    0x80, "CX-80 Trak-Ball (JS Mode)", 17 )
	//PORT_CATEGORY_ITEM(    0x90, STR_AMIGAMOUSE, 15 )
	PORT_CATEGORY_CLASS( 0x0f, 0x00, STR_RIGHT_CONTROLLER )
	PORT_CATEGORY_ITEM(    0x00, DEF_STR( Joystick ), 20 )
	PORT_CATEGORY_ITEM(    0x01, STR_PADDLES, 21 )
	PORT_CATEGORY_ITEM(    0x02, STR_DRIVING, 22 )
	PORT_CATEGORY_ITEM(    0x03, STR_KEYPAD, 23 )
	//PORT_CATEGORY_ITEM(    0x04, STR_LIGHTGUN, 24 )
	PORT_CATEGORY_ITEM(    0x05, STR_BOOSTERGRIP, 20 )
	//PORT_CATEGORY_ITEM(    0x06, STR_CX22TRAKBALL, 25 )
	//PORT_CATEGORY_ITEM(    0x07, STR_CX80TRAKBALL, 25 )
	//PORT_CATEGORY_ITEM(    0x08, "CX-80 Trak-Ball (JS Mode)", 27 )
	//PORT_CATEGORY_ITEM(    0x09, STR_AMIGAMOUSE, 25 )
	PORT_CATEGORY_ITEM(    0x0a, STR_KIDVID, 30 )
	//PORT_CATEGORY_ITEM(    0x0b, "Save Key", 31 )

	PORT_START("KEYPAD_L")	/* [10] left keypad */
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

	PORT_START("KEYPAD_R")  /* [11] right keypad */
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

	PORT_START("WHEEL_L")	/* [12] left driving controller */
	PORT_BIT( 0xff, 0x00, IPT_TRACKBALL_X ) PORT_CATEGORY(12) PORT_SENSITIVITY(40) PORT_KEYDELTA(5) PORT_PLAYER(1)

	PORT_START("WHEEL_R")	/* [13] right driving controller */
	PORT_BIT( 0xff, 0x00, IPT_TRACKBALL_X ) PORT_CATEGORY(22) PORT_SENSITIVITY(40) PORT_KEYDELTA(5) PORT_PLAYER(2)

	PORT_START("GUNX_L")	/* [14] left light gun X */
//  PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_X ) PORT_CATEGORY(14) PORT_CROSSHAIR( X, 1.0, 0.0, 0 ) PORT_SENSITIVITY(25) PORT_KEYDELTA(15) PORT_PLAYER(1)

	PORT_START("GUNY_L")	/* [15] left light gun Y */
//  PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_Y ) PORT_CATEGORY(14) PORT_CROSSHAIR( Y, 1.0, 0.0, 0 ) PORT_SENSITIVITY(25) PORT_KEYDELTA(15) PORT_PLAYER(1)

	PORT_START("GUNX_R")	/* [16] right light gun X */
//  PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_X ) PORT_CATEGORY(24) PORT_CROSSHAIR( X, 1.0, 0.0, 0 ) PORT_SENSITIVITY(25) PORT_KEYDELTA(15) PORT_PLAYER(2)

	PORT_START("GUNY_R")	/* [17] right light gun Y */
//  PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_Y ) PORT_CATEGORY(24) PORT_CROSSHAIR( Y, 1.0, 0.0, 0 ) PORT_SENSITIVITY(25) PORT_KEYDELTA(15) PORT_PLAYER(2)

	PORT_START("TRAKX_L")	/* [18] left trak ball X */
	PORT_BIT( 0xff, 0x00, IPT_TRACKBALL_X ) PORT_CATEGORY(15) PORT_SENSITIVITY(40) PORT_KEYDELTA(5) PORT_PLAYER(1)

	PORT_START("TRAKY_L")	/* [19] left trak ball Y */
	PORT_BIT( 0xff, 0x00, IPT_TRACKBALL_Y ) PORT_CATEGORY(15) PORT_SENSITIVITY(40) PORT_KEYDELTA(5) PORT_PLAYER(1)

	PORT_START("TRAKX_R")	/* [20] right trak ball X */
	PORT_BIT( 0xff, 0x00, IPT_TRACKBALL_X ) PORT_CATEGORY(25) PORT_SENSITIVITY(40) PORT_KEYDELTA(5) PORT_PLAYER(2)

	PORT_START("TRAKY_R")	/* [21] right trak ball Y */
	PORT_BIT( 0xff, 0x00, IPT_TRACKBALL_Y ) PORT_CATEGORY(25) PORT_SENSITIVITY(40) PORT_KEYDELTA(5) PORT_PLAYER(2)

INPUT_PORTS_END


static const cassette_config a2600_cassette_config =
{
	a26_cassette_formats,
	NULL,
	CASSETTE_PLAY | CASSETTE_MOTOR_DISABLED | CASSETTE_SPEAKER_ENABLED
};

static MACHINE_DRIVER_START( a2600_cartslot )
	MDRV_CARTSLOT_ADD("cart")
	MDRV_CARTSLOT_EXTENSION_LIST("bin,a26")
	MDRV_CARTSLOT_MANDATORY
	MDRV_CARTSLOT_START(a2600_cart)
	MDRV_CARTSLOT_LOAD(a2600_cart)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( a2600 )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", M6502, MASTER_CLOCK_NTSC / 3)	/* actually M6507 */
	MDRV_CPU_PROGRAM_MAP(a2600_mem)

	MDRV_MACHINE_START(a2600)
	MDRV_MACHINE_RESET(a2600)

	/* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_RAW_PARAMS( MASTER_CLOCK_NTSC, 228, 26, 26 + 160 + 16, 262, 24 , 24 + 192 + 31 )
	MDRV_PALETTE_LENGTH( TIA_PALETTE_LENGTH )
	MDRV_PALETTE_INIT(tia_NTSC)

	MDRV_VIDEO_START(tia)
	MDRV_VIDEO_UPDATE(tia)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("tia", TIA, MASTER_CLOCK_NTSC/114)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.90)
	MDRV_SOUND_WAVE_ADD("wave", "cassette")
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.10)

	/* devices */
	MDRV_RIOT6532_ADD("riot", MASTER_CLOCK_NTSC / 3, r6532_interface)
	MDRV_IMPORT_FROM(a2600_cartslot)
	MDRV_CASSETTE_ADD( "cassette", a2600_cassette_config )
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( a2600p )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", M6502, MASTER_CLOCK_PAL / 3)    /* actually M6507 */
	MDRV_CPU_PROGRAM_MAP(a2600_mem)

	MDRV_MACHINE_START(a2600p)
	MDRV_MACHINE_RESET(a2600)

	/* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_RAW_PARAMS( MASTER_CLOCK_PAL, 228, 26, 26 + 160 + 16, 312, 32, 32 + 228 + 31 )
	MDRV_PALETTE_LENGTH( TIA_PALETTE_LENGTH )
	MDRV_PALETTE_INIT(tia_PAL)

	MDRV_VIDEO_START(tia)
	MDRV_VIDEO_UPDATE(tia)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("tia", TIA, MASTER_CLOCK_PAL/114)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.90)
	MDRV_SOUND_WAVE_ADD("wave", "cassette")
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.10)

	/* devices */
	MDRV_RIOT6532_ADD("riot", MASTER_CLOCK_PAL / 3, r6532_interface)
	MDRV_IMPORT_FROM(a2600_cartslot)
	MDRV_CASSETTE_ADD( "cassette", a2600_cassette_config )
MACHINE_DRIVER_END


ROM_START( a2600 )
	ROM_REGION( 0x2000, "maincpu", 0 )
	ROM_FILL( 0x0000, 0x2000, 0xFF )
	ROM_REGION( 0x80000, "user1", 0 )
	ROM_FILL( 0x00000, 0x80000, 0xFF )
ROM_END

#define rom_a2600p rom_a2600

/*    YEAR  NAME    PARENT  COMPAT  MACHINE INPUT   INIT    COMPANY     FULLNAME */
CONS( 1977,	a2600,	0,		0,		a2600,	a2600,	0,		"Atari",	"Atari 2600 (NTSC)" , 0)
CONS( 1978,	a2600p,	a2600,	0,		a2600p,	a2600,	0,		"Atari",    "Atari 2600 (PAL)", 0)

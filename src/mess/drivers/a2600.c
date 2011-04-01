/***************************************************************************

  Atari VCS 2600 driver

***************************************************************************/

#include "emu.h"
#include "machine/6532riot.h"
#include "cpu/m6502/m6502.h"
#include "sound/wave.h"
#include "sound/tiaintf.h"
#include "imagedev/cartslot.h"
#include "imagedev/cassette.h"
#include "formats/a26_cas.h"
#include "video/tia.h"
#include "hashfile.h"

typedef struct {
	UINT8	top;
	UINT8	bottom;
	UINT8	low;
	UINT8	high;
	UINT8	flag;
	UINT8	music_mode;		/* Only used by data fetchers 5,6, and 7 */
	UINT8	osc_clk;		/* Only used by data fetchers 5,6, and 7 */
} df_t;

typedef struct
{
	df_t df[8];
	UINT8	movamt;
	UINT8	latch_62;
	UINT8	latch_64;
	UINT8	dlc;
	UINT8	shift_reg;
	emu_timer	*oscillator;
} dpc_t;


class a2600_state : public driver_device
{
public:
	a2600_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	dpc_t m_dpc;
	memory_region* m_extra_RAM;
	UINT8* m_bank_base[5];
	UINT8* m_ram_base;
	UINT8* m_riot_ram;
	UINT8 m_banking_mode;
	UINT8 m_keypad_left_column;
	UINT8 m_keypad_right_column;
	unsigned m_cart_size;
	unsigned m_number_banks;
	unsigned m_current_bank;
	unsigned m_current_reset_bank_counter;
	unsigned m_mode3E_ram_enabled;
	UINT8 m_modeSS_byte;
	UINT32 m_modeSS_byte_started;
	unsigned m_modeSS_write_delay;
	unsigned m_modeSS_write_enabled;
	unsigned m_modeSS_high_ram_enabled;
	unsigned m_modeSS_diff_adjust;
	unsigned m_FVlocked;
	UINT16 m_current_screen_height;
	int m_FETimer;
};



#define CART machine.region("user1")->base()

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

static const UINT16 supported_screen_heights[4] = { 262, 312, 328, 342 };

// try to detect 2600 controller setup. returns 32bits with left/right controller info

static unsigned long detect_2600controllers(running_machine &machine)
{
	a2600_state *state = machine.driver_data<a2600_state>();
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
	if ((state->m_cart_size > 0x4000) || (state->m_cart_size & 0x7ff)) return (left << 16) + right;

	cart = CART;
	for (i = 0; i < state->m_cart_size - (sizeof signatures/sizeof signatures[0]); i++)
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

static int detect_modeDC(running_machine &machine)
{
	a2600_state *state = machine.driver_data<a2600_state>();
	int i,numfound = 0;
	// signature is also in 'video reflex'.. maybe figure out that controller port someday...
	static const unsigned char signature[3] = { 0x8d, 0xf0, 0xff };
	if (state->m_cart_size == 0x10000)
	{
		UINT8 *cart = CART;
		for (i = 0; i < state->m_cart_size - sizeof signature; i++)
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

static int detect_modef6(running_machine &machine)
{
	a2600_state *state = machine.driver_data<a2600_state>();
	int i,numfound = 0;
	static const unsigned char signature[3] = { 0x8d, 0xf6, 0xff };
	if (state->m_cart_size == 0x4000)
	{
		UINT8 *cart = CART;
		for (i = 0; i < state->m_cart_size - sizeof signature; i++)
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

static int detect_mode3E(running_machine &machine)
{
	a2600_state *state = machine.driver_data<a2600_state>();
	// this one is a little hacky.. looks for STY $3e, which is unique to
	// 'not boulderdash', but is the only example i have (cow)
	// Would have used STA $3e, but 'Alien' and 'Star Raiders' do that for unknown reasons

	int i,numfound = 0;
	static const unsigned char signature[3] = { 0x84, 0x3e, 0x9d };
	if (state->m_cart_size == 0x0800 || state->m_cart_size == 0x1000)
	{
		UINT8 *cart = CART;
		for (i = 0; i < state->m_cart_size - sizeof signature; i++)
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

static int detect_modeSS(running_machine &machine)
{
	a2600_state *state = machine.driver_data<a2600_state>();
	int i,numfound = 0;
	static const unsigned char signature[5] = { 0xbd, 0xe5, 0xff, 0x95, 0x81 };
	if (state->m_cart_size == 0x0800 || state->m_cart_size == 0x1000)
	{
		UINT8 *cart = CART;
		for (i = 0; i < state->m_cart_size - sizeof signature; i++)
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

static int detect_modeFE(running_machine &machine)
{
	a2600_state *state = machine.driver_data<a2600_state>();
	int i,j,numfound = 0;
	static const unsigned char signatures[][5] =  {
									{ 0x20, 0x00, 0xd0, 0xc6, 0xc5 },
									{ 0x20, 0xc3, 0xf8, 0xa5, 0x82 },
									{ 0xd0, 0xfb, 0x20, 0x73, 0xfe },
									{ 0x20, 0x00, 0xf0, 0x84, 0xd6 }
	};
	if (state->m_cart_size == 0x2000)
	{
		UINT8 *cart = CART;
		for (i = 0; i < state->m_cart_size - (sizeof signatures/sizeof signatures[0]); i++)
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

static int detect_modeE0(running_machine &machine)
{
	a2600_state *state = machine.driver_data<a2600_state>();
	int i,j,numfound = 0;
	static const unsigned char signatures[][3] =  {
									{ 0x8d, 0xe0, 0x1f },
									{ 0x8d, 0xe0, 0x5f },
									{ 0x8d, 0xe9, 0xff },
									{ 0xad, 0xe9, 0xff },
									{ 0xad, 0xed, 0xff },
									{ 0xad, 0xf3, 0xbf }
	};
	if (state->m_cart_size == 0x2000)
	{
		UINT8 *cart = CART;
		for (i = 0; i < state->m_cart_size - (sizeof signatures/sizeof signatures[0]); i++)
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

static int detect_modeCV(running_machine &machine)
{
	a2600_state *state = machine.driver_data<a2600_state>();
	int i,j,numfound = 0;
	static const unsigned char signatures[][3] = {
									{ 0x9d, 0xff, 0xf3 },
									{ 0x99, 0x00, 0xf4 }
	};
	if (state->m_cart_size == 0x0800 || state->m_cart_size == 0x1000)
	{
		UINT8 *cart = CART;
		for (i = 0; i < state->m_cart_size - (sizeof signatures/sizeof signatures[0]); i++)
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

static int detect_modeFV(running_machine &machine)
{
	a2600_state *state = machine.driver_data<a2600_state>();
	int i,j,numfound = 0;
	static const unsigned char signatures[][3] = {
									{ 0x2c, 0xd0, 0xff }
	};
	if (state->m_cart_size == 0x2000)
	{
		UINT8 *cart = CART;
		for (i = 0; i < state->m_cart_size - (sizeof signatures/sizeof signatures[0]); i++)
		{
			for (j = 0; j < (sizeof signatures/sizeof signatures[0]) && !numfound; j++)
			{
				if (!memcmp(&cart[i], &signatures[j],sizeof signatures[0]))
				{
					numfound = 1;
				}
			}
		}
		state->m_FVlocked = 0;
	}
	if (numfound) return 1;
	return 0;
}

static int detect_modeJVP(running_machine &machine)
{
	a2600_state *state = machine.driver_data<a2600_state>();
	int i,j,numfound = 0;
	static const unsigned char signatures[][4] = {
									{ 0x2c, 0xc0, 0xef, 0x60 },
									{ 0x8d, 0xa0, 0x0f, 0xf0 }
	};
	if (state->m_cart_size == 0x4000 || state->m_cart_size == 0x2000)
	{
		UINT8 *cart = CART;
		for (i = 0; i < state->m_cart_size - (sizeof signatures/sizeof signatures[0]); i++)
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

static int detect_modeE7(running_machine &machine)
{
	a2600_state *state = machine.driver_data<a2600_state>();
	int i,j,numfound = 0;
	static const unsigned char signatures[][3] = {
									{ 0xad, 0xe5, 0xff },
									{ 0x8d, 0xe7, 0xff }
	};
	if (state->m_cart_size == 0x2000 || state->m_cart_size == 0x4000)
	{
		UINT8 *cart = CART;
		for (i = 0; i < state->m_cart_size - (sizeof signatures/sizeof signatures[0]); i++)
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

static int detect_modeUA(running_machine &machine)
{
	a2600_state *state = machine.driver_data<a2600_state>();
	int i,numfound = 0;
	static const unsigned char signature[3] = { 0x8d, 0x40, 0x02 };
	if (state->m_cart_size == 0x2000)
	{
		UINT8 *cart = CART;
		for (i = 0; i < state->m_cart_size - sizeof signature; i++)
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

static int detect_8K_mode3F(running_machine &machine)
{
	a2600_state *state = machine.driver_data<a2600_state>();
	int i,numfound = 0;
	static const unsigned char signature1[4] = { 0xa9, 0x01, 0x85, 0x3f };
	static const unsigned char signature2[4] = { 0xa9, 0x02, 0x85, 0x3f };
	// have to look for two signatures because 'not boulderdash' gives false positive otherwise
	if (state->m_cart_size == 0x2000)
	{
		UINT8 *cart = CART;
		for (i = 0; i < state->m_cart_size - sizeof signature1; i++)
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

static int detect_32K_mode3F(running_machine &machine)
{
	a2600_state *state = machine.driver_data<a2600_state>();
	int i,numfound = 0;
	static const unsigned char signature[4] = { 0xa9, 0x0e, 0x85, 0x3f };
	if (state->m_cart_size >= 0x8000)
	{
		UINT8 *cart = CART;
		for (i = 0; i < state->m_cart_size - sizeof signature; i++)
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

static int detect_super_chip(running_machine &machine)
{
	a2600_state *state = machine.driver_data<a2600_state>();
	int i,j;
	UINT8 *cart = CART;
	static const unsigned char signatures[][5] = {
									{ 0xa2, 0x7f, 0x9d, 0x00, 0xf0 }, // dig dug
									{ 0xae, 0xf6, 0xff, 0x4c, 0x00 } // off the wall
	};

	if (state->m_cart_size == 0x4000)
	{
		for (i = 0; i < state->m_cart_size - (sizeof signatures/sizeof signatures[0]); i++)
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
	for (i = 0x1000; i < state->m_cart_size; i += 0x1000)
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
	a2600_state *state = device->machine().driver_data<a2600_state>();
	state->m_banking_mode = 0xff;
}


static DEVICE_IMAGE_LOAD( a2600_cart )
{
	a2600_state *state = image.device().machine().driver_data<a2600_state>();
	running_machine &machine = image.device().machine();
	UINT8 *cart = CART;

	state->m_cart_size = image.length();

	switch (state->m_cart_size)
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
		image.seterror(IMAGE_ERROR_UNSUPPORTED, "Invalid rom file size" );
		return 1; /* unsupported image format */
	}

	state->m_current_bank = 0;

	image.fread(cart, state->m_cart_size);

	if (!(state->m_cart_size == 0x4000 && detect_modef6(image.device().machine())))
	{
		while (state->m_cart_size > 0x00800)
		{
			if (!memcmp(cart, &cart[state->m_cart_size/2],state->m_cart_size/2)) state->m_cart_size /= 2;
			else break;
		}
	}

	return 0;
}


static int next_bank(a2600_state *state)
{
	return state->m_current_bank = (state->m_current_bank + 1) % 16;
}


static void modeF8_switch(running_machine &machine, UINT16 offset, UINT8 data)
{
	a2600_state *state = machine.driver_data<a2600_state>();
	state->m_bank_base[1] = CART + 0x1000 * offset;
	memory_set_bankptr(machine,"bank1", state->m_bank_base[1]);
}
static void modeFA_switch(running_machine &machine, UINT16 offset, UINT8 data)
{
	a2600_state *state = machine.driver_data<a2600_state>();
	state->m_bank_base[1] = CART + 0x1000 * offset;
	memory_set_bankptr(machine,"bank1", state->m_bank_base[1]);
}
static void modeF6_switch(running_machine &machine, UINT16 offset, UINT8 data)
{
	a2600_state *state = machine.driver_data<a2600_state>();
	state->m_bank_base[1] = CART + 0x1000 * offset;
	memory_set_bankptr(machine,"bank1", state->m_bank_base[1]);
}
static void modeF4_switch(running_machine &machine, UINT16 offset, UINT8 data)
{
	a2600_state *state = machine.driver_data<a2600_state>();
	state->m_bank_base[1] = CART + 0x1000 * offset;
	memory_set_bankptr(machine,"bank1", state->m_bank_base[1]);
}
static void mode3F_switch(running_machine &machine, UINT16 offset, UINT8 data)
{
	a2600_state *state = machine.driver_data<a2600_state>();
	state->m_bank_base[1] = CART + 0x800 * (data & (state->m_number_banks - 1));
	memory_set_bankptr(machine,"bank1", state->m_bank_base[1]);
}
static void modeUA_switch(running_machine &machine, UINT16 offset, UINT8 data)
{
	a2600_state *state = machine.driver_data<a2600_state>();
	state->m_bank_base[1] = CART + (offset >> 6) * 0x1000;
	memory_set_bankptr(machine,"bank1", state->m_bank_base[1]);
}
static void modeE0_switch(running_machine &machine, UINT16 offset, UINT8 data)
{
	a2600_state *state = machine.driver_data<a2600_state>();
	int bank = 1 + (offset >> 3);
	char bank_name[10];
	sprintf(bank_name,"bank%d",bank);
	state->m_bank_base[bank] = CART + 0x400 * (offset & 7);
	memory_set_bankptr(machine,bank_name, state->m_bank_base[bank]);
}
static void modeE7_switch(running_machine &machine, UINT16 offset, UINT8 data)
{
	a2600_state *state = machine.driver_data<a2600_state>();
	state->m_bank_base[1] = CART + 0x800 * offset;
	memory_set_bankptr(machine,"bank1", state->m_bank_base[1]);
}
static void modeE7_RAM_switch(running_machine &machine, UINT16 offset, UINT8 data)
{
	a2600_state *state = machine.driver_data<a2600_state>();
	memory_set_bankptr(machine,"bank9", state->m_extra_RAM->base() + (4 + offset) * 256 );
}
static void modeDC_switch(running_machine &machine, UINT16 offset, UINT8 data)
{
	a2600_state *state = machine.driver_data<a2600_state>();
	state->m_bank_base[1] = CART + 0x1000 * next_bank(state);
	memory_set_bankptr(machine,"bank1", state->m_bank_base[1]);
}
static void mode3E_switch(running_machine &machine, UINT16 offset, UINT8 data)
{
	a2600_state *state = machine.driver_data<a2600_state>();
	state->m_bank_base[1] = CART + 0x800 * (data & (state->m_number_banks - 1));
	memory_set_bankptr(machine,"bank1", state->m_bank_base[1]);
	state->m_mode3E_ram_enabled = 0;
}
static void mode3E_RAM_switch(running_machine &machine, UINT16 offset, UINT8 data)
{
	a2600_state *state = machine.driver_data<a2600_state>();
	state->m_ram_base = state->m_extra_RAM->base() + 0x200 * ( data & 0x3F );
	memory_set_bankptr(machine,"bank1", state->m_ram_base );
	state->m_mode3E_ram_enabled = 1;
}
static void modeFV_switch(running_machine &machine, UINT16 offset, UINT8 data)
{
	a2600_state *state = machine.driver_data<a2600_state>();
	//printf("ModeFV %04x\n",offset);
	if (!state->m_FVlocked && ( cpu_get_pc(machine.device("maincpu")) & 0x1F00 ) == 0x1F00 )
	{
		state->m_FVlocked = 1;
		state->m_current_bank = state->m_current_bank ^ 0x01;
		state->m_bank_base[1] = CART + 0x1000 * state->m_current_bank;
		memory_set_bankptr(machine,"bank1", state->m_bank_base[1]);
	}
}
static void modeJVP_switch(running_machine &machine, UINT16 offset, UINT8 data)
{
	a2600_state *state = machine.driver_data<a2600_state>();
	switch( offset )
	{
	case 0x00:
	case 0x20:
		state->m_current_bank ^= 1;
		break;
	default:
		printf("%04X: write to unknown mapper address %02X\n", cpu_get_pc(machine.device("maincpu")), 0xfa0 + offset );
		break;
	}
	state->m_bank_base[1] = CART + 0x1000 * state->m_current_bank;
	memory_set_bankptr(machine, "bank1", state->m_bank_base[1] );
}


/* These read handlers will return the byte from the new bank */
static  READ8_HANDLER(modeF8_switch_r)
{
	a2600_state *state = space->machine().driver_data<a2600_state>();
	modeF8_switch(space->machine(), offset, 0);
	return state->m_bank_base[1][0xff8 + offset];
}

static  READ8_HANDLER(modeFA_switch_r)
{
	a2600_state *state = space->machine().driver_data<a2600_state>();
	modeFA_switch(space->machine(), offset, 0);
	return state->m_bank_base[1][0xff8 + offset];
}

static  READ8_HANDLER(modeF6_switch_r)
{
	a2600_state *state = space->machine().driver_data<a2600_state>();
	modeF6_switch(space->machine(), offset, 0);
	return state->m_bank_base[1][0xff6 + offset];
}

static  READ8_HANDLER(modeF4_switch_r)
{
	a2600_state *state = space->machine().driver_data<a2600_state>();
	modeF4_switch(space->machine(), offset, 0);
	return state->m_bank_base[1][0xff4 + offset];
}

static  READ8_HANDLER(modeE0_switch_r)
{
	a2600_state *state = space->machine().driver_data<a2600_state>();
	modeE0_switch(space->machine(), offset, 0);
	return state->m_bank_base[4][0x3e0 + offset];
}

static  READ8_HANDLER(modeE7_switch_r)
{
	a2600_state *state = space->machine().driver_data<a2600_state>();
	modeE7_switch(space->machine(), offset, 0);
	return state->m_bank_base[1][0xfe0 + offset];
}

static  READ8_HANDLER(modeE7_RAM_switch_r)
{
	modeE7_RAM_switch(space->machine(), offset, 0);
	return 0;
}

static  READ8_HANDLER(modeUA_switch_r)
{
	modeUA_switch(space->machine(), offset, 0);
	return 0;
}

static  READ8_HANDLER(modeDC_switch_r)
{
	a2600_state *state = space->machine().driver_data<a2600_state>();
	modeDC_switch(space->machine(), offset, 0);
	return state->m_bank_base[1][0xff0 + offset];
}

static  READ8_HANDLER(modeFV_switch_r)
{
	a2600_state *state = space->machine().driver_data<a2600_state>();
	modeFV_switch(space->machine(), offset, 0);
	return state->m_bank_base[1][0xfd0 + offset];
}

static  READ8_HANDLER(modeJVP_switch_r)
{
	a2600_state *state = space->machine().driver_data<a2600_state>();
	modeJVP_switch(space->machine(), offset, 0);
	return state->m_riot_ram[ 0x20 + offset ];
}


static WRITE8_HANDLER(modeF8_switch_w) { modeF8_switch(space->machine(), offset, data); }
static WRITE8_HANDLER(modeFA_switch_w) { modeFA_switch(space->machine(), offset, data); }
static WRITE8_HANDLER(modeF6_switch_w) { modeF6_switch(space->machine(), offset, data); }
static WRITE8_HANDLER(modeF4_switch_w) { modeF4_switch(space->machine(), offset, data); }
static WRITE8_HANDLER(modeE0_switch_w) { modeE0_switch(space->machine(), offset, data); }
static WRITE8_HANDLER(modeE7_switch_w) { modeE7_switch(space->machine(), offset, data); }
static WRITE8_HANDLER(modeE7_RAM_switch_w) { modeE7_RAM_switch(space->machine(), offset, data); }
static WRITE8_HANDLER(mode3F_switch_w) { mode3F_switch(space->machine(), offset, data); }
static WRITE8_HANDLER(modeUA_switch_w) { modeUA_switch(space->machine(), offset, data); }
static WRITE8_HANDLER(modeDC_switch_w) { modeDC_switch(space->machine(), offset, data); }
static WRITE8_HANDLER(mode3E_switch_w) { mode3E_switch(space->machine(), offset, data); }
static WRITE8_HANDLER(mode3E_RAM_switch_w) { mode3E_RAM_switch(space->machine(), offset, data); }
static WRITE8_HANDLER(mode3E_RAM_w)
{
	a2600_state *state = space->machine().driver_data<a2600_state>();
	if ( state->m_mode3E_ram_enabled )
	{
		state->m_ram_base[offset] = data;
	}
}
static WRITE8_HANDLER(modeFV_switch_w) { modeFV_switch(space->machine(), offset, data); }
static WRITE8_HANDLER(modeJVP_switch_w)
{
	a2600_state *state = space->machine().driver_data<a2600_state>();
	modeJVP_switch(space->machine(), offset, data); state->m_riot_ram[ 0x20 + offset ] = data;
}


DIRECT_UPDATE_HANDLER( modeF6_opbase )
{
	if ( ( address & 0x1FFF ) >= 0x1FF6 && ( address & 0x1FFF ) <= 0x1FF9 )
	{
		modeF6_switch_w( machine->device("maincpu")->memory().space(AS_PROGRAM), ( address & 0x1FFF ) - 0x1FF6, 0 );
	}
	return address;
}

DIRECT_UPDATE_HANDLER( modeSS_opbase )
{
	a2600_state *state = machine->driver_data<a2600_state>();
	if ( address & 0x1000 )
	{
		if ( address & 0x800 )
		{
			direct.explicit_configure(( address & 0xf800 ), ( address & 0xf800 ) | 0x7ff, 0x7ff, state->m_bank_base[2]);
		}
		else
		{
			direct.explicit_configure(( address & 0xf800 ), ( address & 0xf800 ) | 0x7ff, 0x7ff, state->m_bank_base[1]);
		}
		return ~0;
	}
	return address;
}

static READ8_HANDLER(modeSS_r)
{
	a2600_state *state = space->machine().driver_data<a2600_state>();
	UINT8 data = ( offset & 0x800 ) ? state->m_bank_base[2][offset & 0x7FF] : state->m_bank_base[1][offset];

	//logerror("%04X: read from modeSS area offset = %04X\n", cpu_get_pc(space->machine().device("maincpu")), offset);
	/* Check for control register "write" */
	if ( offset == 0xFF8 )
	{
		//logerror("%04X: write to modeSS control register data = %02X\n", cpu_get_pc(space->machine().device("maincpu")), state->m_modeSS_byte);
		state->m_modeSS_write_enabled = state->m_modeSS_byte & 0x02;
		state->m_modeSS_write_delay = state->m_modeSS_byte >> 5;
		switch ( state->m_modeSS_byte & 0x1C )
		{
		case 0x00:
			state->m_bank_base[1] = state->m_extra_RAM->base() + 2 * 0x800;
			state->m_bank_base[2] = ( state->m_modeSS_byte & 0x01 ) ? space->machine().region("maincpu")->base() + 0x1800 : space->machine().region("user1")->base();
			state->m_modeSS_high_ram_enabled = 0;
			break;
		case 0x04:
			state->m_bank_base[1] = state->m_extra_RAM->base();
			state->m_bank_base[2] = ( state->m_modeSS_byte & 0x01 ) ? space->machine().region("maincpu")->base() + 0x1800 : space->machine().region("user1")->base();
			state->m_modeSS_high_ram_enabled = 0;
			break;
		case 0x08:
			state->m_bank_base[1] = state->m_extra_RAM->base() + 2 * 0x800;
			state->m_bank_base[2] = state->m_extra_RAM->base();
			state->m_modeSS_high_ram_enabled = 1;
			break;
		case 0x0C:
			state->m_bank_base[1] = state->m_extra_RAM->base();
			state->m_bank_base[2] = state->m_extra_RAM->base() + 2 * 0x800;
			state->m_modeSS_high_ram_enabled = 1;
			break;
		case 0x10:
			state->m_bank_base[1] = state->m_extra_RAM->base() + 2 * 0x800;
			state->m_bank_base[2] = ( state->m_modeSS_byte & 0x01 ) ? space->machine().region("maincpu")->base() + 0x1800 : space->machine().region("user1")->base();
			state->m_modeSS_high_ram_enabled = 0;
			break;
		case 0x14:
			state->m_bank_base[1] = state->m_extra_RAM->base() + 0x800;
			state->m_bank_base[2] = ( state->m_modeSS_byte & 0x01 ) ? space->machine().region("maincpu")->base() + 0x1800 : space->machine().region("user1")->base();
			state->m_modeSS_high_ram_enabled = 0;
			break;
		case 0x18:
			state->m_bank_base[1] = state->m_extra_RAM->base() + 2 * 0x800;
			state->m_bank_base[2] = state->m_extra_RAM->base() + 0x800;
			state->m_modeSS_high_ram_enabled = 1;
			break;
		case 0x1C:
			state->m_bank_base[1] = state->m_extra_RAM->base() + 0x800;
			state->m_bank_base[2] = state->m_extra_RAM->base() + 2 * 0x800;
			state->m_modeSS_high_ram_enabled = 1;
			break;
		}
		memory_set_bankptr(space->machine(), "bank1", state->m_bank_base[1] );
		memory_set_bankptr(space->machine(), "bank2", state->m_bank_base[2] );
	}
	else if ( offset == 0xFF9 )
	{
		/* Cassette port read */
		double tap_val = cassette_input( space->machine().device("cassette") );
		//logerror("%04X: Cassette port read, tap_val = %f\n", cpu_get_pc(space->machine().device("maincpu")), tap_val);
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
		if ( state->m_modeSS_write_enabled )
		{
			int diff = space->machine().device<cpu_device>("maincpu")->total_cycles() - state->m_modeSS_byte_started;
			//logerror("%04X: offset = %04X, %d\n", cpu_get_pc(space->machine().device("maincpu")), offset, diff);
			if ( diff - state->m_modeSS_diff_adjust == 5 )
			{
				//logerror("%04X: RAM write offset = %04X, data = %02X\n", cpu_get_pc(space->machine().device("maincpu")), offset, state->m_modeSS_byte );
				if ( offset & 0x800 )
				{
					if ( state->m_modeSS_high_ram_enabled )
					{
						state->m_bank_base[2][offset & 0x7FF] = state->m_modeSS_byte;
						data = state->m_modeSS_byte;
					}
				}
				else
				{
					state->m_bank_base[1][offset] = state->m_modeSS_byte;
					data = state->m_modeSS_byte;
				}
			}
			else if ( offset < 0x0100 )
			{
				state->m_modeSS_byte = offset;
				state->m_modeSS_byte_started = space->machine().device<cpu_device>("maincpu")->total_cycles();
			}
			/* Check for dummy read from same address */
			if ( diff == 2 )
			{
				state->m_modeSS_diff_adjust = 1;
			}
			else
			{
				state->m_modeSS_diff_adjust = 0;
			}
		}
		else if ( offset < 0x0100 )
		{
			state->m_modeSS_byte = offset;
			state->m_modeSS_byte_started = space->machine().device<cpu_device>("maincpu")->total_cycles();
		}
	}
	/* Because the mame core caches opcode data and doesn't perform reads like normal */
	/* we have to put in this little hack here to get Suicide Mission to work. */
	if ( offset != 0xFF8 && ( cpu_get_pc(space->machine().device("maincpu")) & 0x1FFF ) == 0x1FF8 )
	{
		modeSS_r( space, 0xFF8 );
	}
	return data;
}

INLINE void modeDPC_check_flag(a2600_state *state, UINT8 data_fetcher)
{
	/* Set flag when low counter equals top */
	if ( state->m_dpc.df[data_fetcher].low == state->m_dpc.df[data_fetcher].top )
	{
		state->m_dpc.df[data_fetcher].flag = 1;
	}
	/* Reset flag when low counter equals bottom */
	if ( state->m_dpc.df[data_fetcher].low == state->m_dpc.df[data_fetcher].bottom )
	{
		state->m_dpc.df[data_fetcher].flag = 0;
	}
}

INLINE void modeDPC_decrement_counter(a2600_state *state, UINT8 data_fetcher)
{
	state->m_dpc.df[data_fetcher].low -= 1;
	if ( state->m_dpc.df[data_fetcher].low == 0xFF )
	{
		state->m_dpc.df[data_fetcher].high -= 1;
		if ( data_fetcher > 4 && state->m_dpc.df[data_fetcher].music_mode )
		{
			state->m_dpc.df[data_fetcher].low = state->m_dpc.df[data_fetcher].top;
		}
	}

	modeDPC_check_flag( state, data_fetcher );
}

static TIMER_CALLBACK(modeDPC_timer_callback)
{
	a2600_state *state = machine.driver_data<a2600_state>();
	int data_fetcher;
	for( data_fetcher = 5; data_fetcher < 8; data_fetcher++ )
	{
		if ( state->m_dpc.df[data_fetcher].osc_clk )
		{
			modeDPC_decrement_counter( state, data_fetcher );
		}
	}
}

DIRECT_UPDATE_HANDLER(modeDPC_opbase_handler)
{
	a2600_state *state = machine->driver_data<a2600_state>();
	UINT8	new_bit;
	new_bit = ( state->m_dpc.shift_reg & 0x80 ) ^ ( ( state->m_dpc.shift_reg & 0x20 ) << 2 );
	new_bit = new_bit ^ ( ( ( state->m_dpc.shift_reg & 0x10 ) << 3 ) ^ ( ( state->m_dpc.shift_reg & 0x08 ) << 4 ) );
	new_bit = new_bit ^ 0x80;
	state->m_dpc.shift_reg = new_bit | ( state->m_dpc.shift_reg >> 1 );
	return address;
}

static READ8_HANDLER(modeDPC_r)
{
	a2600_state *state = space->machine().driver_data<a2600_state>();
	static const UINT8 dpc_amplitude[8] = { 0x00, 0x04, 0x05, 0x09, 0x06, 0x0A, 0x0B, 0x0F };
	UINT8	data_fetcher = offset & 0x07;
	UINT8	data = 0xFF;

	logerror("%04X: Read from DPC offset $%02X\n", cpu_get_pc(space->machine().device("maincpu")), offset);
	if ( offset < 0x08 )
	{
		switch( offset & 0x06 )
		{
		case 0x00:		/* Random number generator */
		case 0x02:
			return state->m_dpc.shift_reg;
		case 0x04:		/* Sound value, MOVAMT value AND'd with Draw Line Carry; with Draw Line Add */
			state->m_dpc.latch_62 = state->m_dpc.latch_64;
		case 0x06:		/* Sound value, MOVAMT value AND'd with Draw Line Carry; without Draw Line Add */
			state->m_dpc.latch_64 = state->m_dpc.latch_62 + state->m_dpc.df[4].top;
			state->m_dpc.dlc = ( state->m_dpc.latch_62 + state->m_dpc.df[4].top > 0xFF ) ? 1 : 0;
			data = 0;
			if ( state->m_dpc.df[5].music_mode && state->m_dpc.df[5].flag )
			{
				data |= 0x01;
			}
			if ( state->m_dpc.df[6].music_mode && state->m_dpc.df[6].flag )
			{
				data |= 0x02;
			}
			if ( state->m_dpc.df[7].music_mode && state->m_dpc.df[7].flag )
			{
				data |= 0x04;
			}
			return ( state->m_dpc.dlc ? state->m_dpc.movamt & 0xF0 : 0 ) | dpc_amplitude[data];
		}
	}
	else
	{
		UINT8	display_data = space->machine().region("user1")->base()[0x2000 + ( ~ ( ( state->m_dpc.df[data_fetcher].low | ( state->m_dpc.df[data_fetcher].high << 8 ) ) ) & 0x7FF ) ];

		switch( offset & 0x38 )
		{
		case 0x08:			/* display data */
			data = display_data;
			break;
		case 0x10:			/* display data AND'd w/flag */
			data = state->m_dpc.df[data_fetcher].flag ? display_data : 0x00;
			break;
		case 0x18:			/* display data AND'd w/flag, nibbles swapped */
			data = state->m_dpc.df[data_fetcher].flag ? BITSWAP8(display_data,3,2,1,0,7,6,5,4) : 0x00;
			break;
		case 0x20:			/* display data AND'd w/flag, byte reversed */
			data = state->m_dpc.df[data_fetcher].flag ? BITSWAP8(display_data,0,1,2,3,4,5,6,7) : 0x00;
			break;
		case 0x28:			/* display data AND'd w/flag, rotated right */
			data = state->m_dpc.df[data_fetcher].flag ? ( display_data >> 1 ) : 0x00;
			break;
		case 0x30:			/* display data AND'd w/flag, rotated left */
			data = state->m_dpc.df[data_fetcher].flag ? ( display_data << 1 ) : 0x00;
			break;
		case 0x38:			/* flag */
			data = state->m_dpc.df[data_fetcher].flag ? 0xFF : 0x00;
			break;
		}

		if ( data_fetcher < 5 || ! state->m_dpc.df[data_fetcher].osc_clk )
		{
			modeDPC_decrement_counter( state, data_fetcher );
		}
	}
	return data;
}

static WRITE8_HANDLER(modeDPC_w)
{
	a2600_state *state = space->machine().driver_data<a2600_state>();
	UINT8	data_fetcher = offset & 0x07;

	switch( offset & 0x38 )
	{
	case 0x00:			/* Top count */
		state->m_dpc.df[data_fetcher].top = data;
		state->m_dpc.df[data_fetcher].flag = 0;
		modeDPC_check_flag( state, data_fetcher );
		break;
	case 0x08:			/* Bottom count */
		state->m_dpc.df[data_fetcher].bottom = data;
		modeDPC_check_flag( state, data_fetcher );
		break;
	case 0x10:			/* Counter low */
		state->m_dpc.df[data_fetcher].low = data;
		if ( data_fetcher == 4 )
		{
			state->m_dpc.latch_64 = data;
		}
		if ( data_fetcher > 4 && state->m_dpc.df[data_fetcher].music_mode )
		{
			state->m_dpc.df[data_fetcher].low = state->m_dpc.df[data_fetcher].top;
		}
		modeDPC_check_flag( state, data_fetcher );
		break;
	case 0x18:			/* Counter high */
		state->m_dpc.df[data_fetcher].high = data;
		state->m_dpc.df[data_fetcher].music_mode = data & 0x10;
		state->m_dpc.df[data_fetcher].osc_clk = data & 0x20;
		if ( data_fetcher > 4 && state->m_dpc.df[data_fetcher].music_mode && state->m_dpc.df[data_fetcher].low == 0xFF )
		{
			state->m_dpc.df[data_fetcher].low = state->m_dpc.df[data_fetcher].top;
			modeDPC_check_flag( state, data_fetcher );
		}
		break;
	case 0x20:			/* Draw line movement value / MOVAMT */
		state->m_dpc.movamt = data;
		break;
	case 0x28:			/* Not used */
		logerror("%04X: Write to unused DPC register $%02X, data $%02X\n", cpu_get_pc(space->machine().device("maincpu")), offset, data);
		break;
	case 0x30:			/* Random number generator reset */
		state->m_dpc.shift_reg = 0;
		break;
	case 0x38:			/* Not used */
		logerror("%04X: Write to unused DPC register $%02X, data $%02X\n", cpu_get_pc(space->machine().device("maincpu")), offset, data);
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

direct_update_delegate FE_old_opbase_handler;

DIRECT_UPDATE_HANDLER(modeFE_opbase_handler)
{
	a2600_state *state = machine->driver_data<a2600_state>();
	if ( ! state->m_FETimer )
	{
		/* Still cheating a bit here by looking bit 13 of the address..., but the high byte of the
           cpu should be the last byte that was on the data bus and so should determine the bank
           we should switch in. */
		state->m_bank_base[1] = machine->region("user1")->base() + 0x1000 * ( ( address & 0x2000 ) ? 0 : 1 );
		memory_set_bankptr(*machine, "bank1", state->m_bank_base[1] );
		/* and restore old opbase handler */
		machine->device("maincpu")->memory().space(AS_PROGRAM)->set_direct_update_handler(FE_old_opbase_handler);
	}
	else
	{
		/* Wait for one memory access to have passed (reading of new PCH either from code or from stack) */
		state->m_FETimer--;
	}
	return address;
}

static void modeFE_switch(running_machine &machine,UINT16 offset, UINT8 data)
{
	a2600_state *state = machine.driver_data<a2600_state>();
	address_space* space = machine.device("maincpu")->memory().space(AS_PROGRAM);
	/* Retrieve last byte read by the cpu (for this mapping scheme this
       should be the last byte that was on the data bus
    */
	state->m_FETimer = 1;
	FE_old_opbase_handler = space->set_direct_update_handler(direct_update_delegate_create_static(modeFE_opbase_handler, machine));
}

static READ8_HANDLER(modeFE_switch_r)
{
	modeFE_switch(space->machine(),offset, 0 );
	return space->read_byte(0xFE );
}

static WRITE8_HANDLER(modeFE_switch_w)
{
	space->write_byte(0xFE, data );
	modeFE_switch(space->machine(),offset, 0 );
}

static  READ8_HANDLER(current_bank_r)
{
	a2600_state *state = space->machine().driver_data<a2600_state>();
	return state->m_current_bank;
}

static ADDRESS_MAP_START(a2600_mem, AS_PROGRAM, 8)
	ADDRESS_MAP_GLOBAL_MASK(0x1fff)
	AM_RANGE(0x0000, 0x007F) AM_MIRROR(0x0F00) AM_READWRITE(tia_r, tia_w)
	AM_RANGE(0x0080, 0x00FF) AM_MIRROR(0x0D00) AM_RAM AM_BASE_MEMBER(a2600_state, m_riot_ram)
	AM_RANGE(0x0280, 0x029F) AM_MIRROR(0x0D00) AM_DEVREADWRITE("riot", riot6532_r, riot6532_w)
	AM_RANGE(0x1000, 0x1FFF)                   AM_ROMBANK("bank1")
ADDRESS_MAP_END

static WRITE8_DEVICE_HANDLER(switch_A_w)
{
	a2600_state *state = device->machine().driver_data<a2600_state>();
	running_machine &machine = device->machine();

	/* Left controller port */
	if ( input_port_read(machine, "CONTROLLERS") / CATEGORY_SELECT == 0x03 )
	{
		state->m_keypad_left_column = data / 16;
	}

	/* Right controller port */
	switch( input_port_read(machine, "CONTROLLERS") % CATEGORY_SELECT )
	{
	case 0x03:	/* Keypad */
		state->m_keypad_right_column = data & 0x0F;
		break;
	case 0x0a:	/* KidVid voice module */
		cassette_change_state( machine.device("cassette"), ( data & 0x02 ) ? (cassette_state)CASSETTE_MOTOR_DISABLED : (cassette_state)(CASSETTE_MOTOR_ENABLED | CASSETTE_PLAY), (cassette_state)CASSETTE_MOTOR_DISABLED );
		break;
	}
}

static READ8_DEVICE_HANDLER( switch_A_r )
{
	static const UINT8 driving_lookup[4] = { 0x00, 0x02, 0x03, 0x01 };
	running_machine &machine = device->machine();
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
	return input_port_read(device->machine(), "SWB");
}

static const riot6532_interface r6532_interface =
{
	DEVCB_HANDLER(switch_A_r),
	DEVCB_HANDLER(riot_input_port_8_r),
	DEVCB_HANDLER(switch_A_w),
	DEVCB_HANDLER(switch_B_w),
	DEVCB_LINE(irq_callback)
};


static void install_banks(running_machine &machine, int count, unsigned init)
{
	a2600_state *state = machine.driver_data<a2600_state>();
	int i;
	UINT8 *cart = CART;

	for (i = 0; i < count; i++)
	{
		static const char *const handler[] =
		{
			"bank1",
			"bank2",
			"bank3",
			"bank4",
		};

		machine.device("maincpu")->memory().space(AS_PROGRAM)->install_read_bank(
			0x1000 + (i + 0) * 0x1000 / count - 0,
			0x1000 + (i + 1) * 0x1000 / count - 1, handler[i]);

		state->m_bank_base[i + 1] = cart + init;
		memory_set_bankptr(machine,handler[i], state->m_bank_base[i + 1]);
	}
}

static READ16_HANDLER(a2600_read_input_port)
{
	a2600_state *state = space->machine().driver_data<a2600_state>();
	int i;

	switch( offset )
	{
	case 0:	/* Left controller port PIN 5 */
		switch ( input_port_read(space->machine(), "CONTROLLERS") / CATEGORY_SELECT )
		{
		case 0x00:	/* Joystick */
			return TIA_INPUT_PORT_ALWAYS_OFF;
		case 0x01:	/* Paddle */
			return input_port_read(space->machine(), "PADDLE1");
		case 0x03:	/* Keypad */
			for ( i = 0; i < 4; i++ )
			{
				if ( ! ( ( state->m_keypad_left_column >> i ) & 0x01 ) )
				{
					if ( ( input_port_read(space->machine(), "KEYPAD_L") >> 3*i ) & 0x01 )
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
			return ( input_port_read(space->machine(), "BUTTONS_L") & 0x40 ) ? TIA_INPUT_PORT_ALWAYS_OFF : TIA_INPUT_PORT_ALWAYS_ON;
		default:
			return TIA_INPUT_PORT_ALWAYS_OFF;
		}
		break;
	case 1:	/* Right controller port PIN 5 */
		switch ( input_port_read(space->machine(), "CONTROLLERS") / CATEGORY_SELECT )
		{
		case 0x00:	/* Joystick */
			return TIA_INPUT_PORT_ALWAYS_OFF;
		case 0x01:	/* Paddle */
			return input_port_read(space->machine(), "PADDLE3");
		case 0x03:	/* Keypad */
			for ( i = 0; i < 4; i++ )
			{
				if ( ! ( ( state->m_keypad_left_column >> i ) & 0x01 ) )
				{
					if ( ( input_port_read(space->machine(), "KEYPAD_L") >> 3*i ) & 0x02 )
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
			return ( input_port_read(space->machine(), "BUTTONS_L") & 0x20 ) ? TIA_INPUT_PORT_ALWAYS_OFF : TIA_INPUT_PORT_ALWAYS_ON;
		default:
			return TIA_INPUT_PORT_ALWAYS_OFF;
		}
		break;
	case 2:	/* Left controller port PIN 9 */
		switch ( input_port_read(space->machine(), "CONTROLLERS") % CATEGORY_SELECT )
		{
		case 0x00:	/* Joystick */
			return TIA_INPUT_PORT_ALWAYS_OFF;
		case 0x01:	/* Paddle */
			return input_port_read(space->machine(), "PADDLE2");
		case 0x03:	/* Keypad */
			for ( i = 0; i < 4; i++ )
			{
				if ( ! ( ( state->m_keypad_right_column >> i ) & 0x01 ) )
				{
					if ( ( input_port_read(space->machine(), "KEYPAD_R") >> 3*i ) & 0x01 )
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
			return ( input_port_read(space->machine(), "BUTTONS_R") & 0x40 ) ? TIA_INPUT_PORT_ALWAYS_OFF : TIA_INPUT_PORT_ALWAYS_ON;
		default:
			return TIA_INPUT_PORT_ALWAYS_OFF;
		}
		break;
	case 3:	/* Right controller port PIN 9 */
		switch ( input_port_read(space->machine(), "CONTROLLERS") % CATEGORY_SELECT )
		{
		case 0x00:	/* Joystick */
			return TIA_INPUT_PORT_ALWAYS_OFF;
		case 0x01:	/* Paddle */
			return input_port_read(space->machine(), "PADDLE4");
		case 0x03:	/* Keypad */
			for ( i = 0; i < 4; i++ )
			{
				if ( ! ( ( state->m_keypad_right_column >> i ) & 0x01 ) )
				{
					if ( ( input_port_read(space->machine(), "KEYPAD_R") >> 3*i ) & 0x02 )
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
			return ( input_port_read(space->machine(), "BUTTONS_R") & 0x20 ) ? TIA_INPUT_PORT_ALWAYS_OFF : TIA_INPUT_PORT_ALWAYS_ON;
		default:
			return TIA_INPUT_PORT_ALWAYS_OFF;
		}
		break;
	case 4:	/* Left controller port PIN 6 */
		switch ( input_port_read(space->machine(), "CONTROLLERS") / CATEGORY_SELECT )
		{
		case 0x00:	/* Joystick */
		case 0x05:	/* Joystick w/Boostergrip */
			return input_port_read(space->machine(), "BUTTONS_L");
		case 0x01:	/* Paddle */
			return 0xff;
		case 0x02:	/* Driving */
			return input_port_read(space->machine(), "BUTTONS_L") << 3;
		case 0x03:	/* Keypad */
			for ( i = 0; i < 4; i++ )
			{
				if ( ! ( ( state->m_keypad_left_column >> i ) & 0x01 ) )
				{
					if ( ( input_port_read(space->machine(), "KEYPAD_L") >> 3*i ) & 0x04 )
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
			return input_port_read(space->machine(), "BUTTONS_L") << 4;
		default:
			return 0xff;
		}
		break;
	case 5:	/* Right controller port PIN 6 */
		switch ( input_port_read(space->machine(), "CONTROLLERS") % CATEGORY_SELECT )
		{
		case 0x00:	/* Joystick */
		case 0x05:	/* Joystick w/Boostergrip */
			return input_port_read(space->machine(), "BUTTONS_R");
		case 0x01:	/* Paddle */
			return 0xff;
		case 0x02:	/* Driving */
			return input_port_read(space->machine(), "BUTTONS_R") << 3;
		case 0x03:	/* Keypad */
			for ( i = 0; i < 4; i++ )
			{
				if ( ! ( ( state->m_keypad_right_column >> i ) & 0x01 ) )
				{
					if ( ( input_port_read(space->machine(), "KEYPAD_R") >> 3*i ) & 0x04 )
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
			return input_port_read(space->machine(), "BUTTONS_R") << 4;
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

	last_address = cpu_get_pc(space->machine().device("maincpu")) - 1;
	if ( ! ( last_address & 0x1080 ) )
	{
		return offset;
	}
	last_byte = space->read_byte(last_address );
	if ( last_byte < 0x80 || last_byte == 0xFF )
	{
		return last_byte;
	}
	prev_address = last_address - 1;
	if ( ! ( prev_address & 0x1080 ) )
	{
		return last_byte;
	}
	prev_byte = space->read_byte(prev_address );
	if ( prev_byte == 0xB1 )
	{	/* LDA (XX),Y */
		return space->read_byte(last_byte + 1 );
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
	a2600_state *state = space->machine().driver_data<a2600_state>();
	int i;

	for ( i = 0; i < ARRAY_LENGTH(supported_screen_heights); i++ )
	{
		if ( data >= supported_screen_heights[i] - 3 && data <= supported_screen_heights[i] + 3 )
		{
			if ( supported_screen_heights[i] != state->m_current_screen_height )
			{
				state->m_current_screen_height = supported_screen_heights[i];
//              machine.primary_screen->configure(228, state->m_current_screen_height, &visarea[i], HZ_TO_ATTOSECONDS( MASTER_CLOCK_NTSC ) * 228 * state->m_current_screen_height );
			}
		}
	}
}

static WRITE16_HANDLER( a2600_tia_vsync_callback_pal )
{
	a2600_state *state = space->machine().driver_data<a2600_state>();
	int i;

	for ( i = 0; i < ARRAY_LENGTH(supported_screen_heights); i++ )
	{
		if ( data >= supported_screen_heights[i] - 3 && data <= supported_screen_heights[i] + 3 )
		{
			if ( supported_screen_heights[i] != state->m_current_screen_height )
			{
				state->m_current_screen_height = supported_screen_heights[i];
//              machine.primary_screen->configure(228, state->m_current_screen_height, &visarea[i], HZ_TO_ATTOSECONDS( MASTER_CLOCK_PAL ) * 228 * state->m_current_screen_height );
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


static void common_init(running_machine &machine)
{
	a2600_state *state = machine.driver_data<a2600_state>();
	screen_device *screen = machine.first_screen();
	state->m_current_screen_height = screen->height();
	state->m_extra_RAM = machine.region_alloc("user2", 0x8600, 1, ENDIANNESS_LITTLE);
	memset( state->m_riot_ram, 0x00, 0x80 );
	state->m_current_reset_bank_counter = 0xFF;
	state->m_dpc.oscillator = machine.scheduler().timer_alloc(FUNC(modeDPC_timer_callback));
}

static MACHINE_START( a2600 )
{
	common_init(machine);
	tia_init( machine, &tia_interface );
}

static MACHINE_START( a2600p )
{
	common_init(machine);
	tia_init( machine, &tia_interface_pal );
}

static void set_category_value( running_machine &machine, const char* cat, const char *cat_selection )
{
	/* NPW 22-May-2008 - FIXME */
#if 0
	input_port_entry	*cat_in = NULL;
	input_port_entry	*in;

	for( in = machine.input_ports; in->type != IPT_END; in++ )
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

static void set_controller( running_machine &machine, const char *controller, unsigned int selection )
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
	a2600_state *state = machine.driver_data<a2600_state>();
	address_space* space = machine.device("maincpu")->memory().space(AS_PROGRAM);
	int chip = 0xFF;
	unsigned long controltemp;
	static const unsigned char snowwhite[] = { 0x10, 0xd0, 0xff, 0xff }; // Snow White Proto

	state->m_current_reset_bank_counter++;

	/* auto-detect special controllers */
	controltemp = detect_2600controllers(machine);
	set_controller( machine, STR_LEFT_CONTROLLER, controltemp >> 16 );
	set_controller( machine, STR_RIGHT_CONTROLLER, controltemp & 0xFFFF );

	/* auto-detect bank mode */

	if (state->m_banking_mode == 0xff) if (detect_modeDC(machine)) state->m_banking_mode = modeDC;
	if (state->m_banking_mode == 0xff) if (detect_mode3E(machine)) state->m_banking_mode = mode3E;
	if (state->m_banking_mode == 0xff) if (detect_modeFE(machine)) state->m_banking_mode = modeFE;
	if (state->m_banking_mode == 0xff) if (detect_modeSS(machine)) state->m_banking_mode = modeSS;
	if (state->m_banking_mode == 0xff) if (detect_modeE0(machine)) state->m_banking_mode = modeE0;
	if (state->m_banking_mode == 0xff) if (detect_modeCV(machine)) state->m_banking_mode = modeCV;
	if (state->m_banking_mode == 0xff) if (detect_modeFV(machine)) state->m_banking_mode = modeFV;
	if (state->m_banking_mode == 0xff) if (detect_modeJVP(machine)) state->m_banking_mode = modeJVP;
	if (state->m_banking_mode == 0xff) if (detect_modeUA(machine)) state->m_banking_mode = modeUA;
	if (state->m_banking_mode == 0xff) if (detect_8K_mode3F(machine)) state->m_banking_mode = mode3F;
	if (state->m_banking_mode == 0xff) if (detect_32K_mode3F(machine)) state->m_banking_mode = mode3F;
	if (state->m_banking_mode == 0xff) if (detect_modeE7(machine)) state->m_banking_mode = modeE7;

	if (state->m_banking_mode == 0xff)
	{
		switch (state->m_cart_size)
		{
		case 0x800:
			state->m_banking_mode = mode2K;
			break;
		case 0x1000:
			state->m_banking_mode = mode4K;
			break;
		case 0x2000:
			state->m_banking_mode = modeF8;
			break;
		case 0x28FF:
		case 0x2900:
			state->m_banking_mode = modeDPC;
			break;
		case 0x3000:
			state->m_banking_mode = modeFA;
			break;
		case 0x4000:
			state->m_banking_mode = modeF6;
			break;
		case 0x8000:
			state->m_banking_mode = modeF4;
			break;
		case 0x10000:
			state->m_banking_mode = mode32in1;
			break;
		case 0x80000:
			state->m_banking_mode = mode3F;
			break;
		}
	}

	/* auto-detect super chip */

	chip = 0;

	if (state->m_cart_size == 0x2000 || state->m_cart_size == 0x4000 || state->m_cart_size == 0x8000)
	{
		chip = detect_super_chip(machine);
	}

	/* Super chip games:
       dig dig, crystal castles, millipede, stargate, defender ii, jr. Pac Man,
       desert falcon, dark chambers, super football, sprintmaster, fatal run,
       off the wall, shooting arcade, secret quest, radar lock, save mary, klax
    */

	/* set up ROM banks */

	switch (state->m_banking_mode)
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
		install_banks(machine, 2, state->m_cart_size - 0x800);
		state->m_number_banks = state->m_cart_size / 0x800;
		break;

	case modeUA:
		install_banks(machine, 1, 0x1000);
		break;

	case modeE7:
		install_banks(machine, 2, 0x3800);
		break;

	case modeDC:
		install_banks(machine, 1, 0x1000 * state->m_current_bank);
		break;

	case modeCV:
		install_banks(machine, 2, 0x0000);
		break;

	case mode3E:
		install_banks(machine, 2, state->m_cart_size - 0x800);
		state->m_number_banks = state->m_cart_size / 0x800;
		state->m_mode3E_ram_enabled = 0;
		break;

	case modeSS:
		install_banks(machine, 2, 0x0000);
		break;

	case modeFV:
		install_banks(machine, 1, 0x0000);
		state->m_current_bank = 0;
		break;

	case modeDPC:
		install_banks(machine, 1, 0x0000);
		break;

	case mode32in1:
		install_banks(machine, 2, 0x0000);
		state->m_current_reset_bank_counter = state->m_current_reset_bank_counter & 0x1F;
		break;

	case modeJVP:
		state->m_current_reset_bank_counter = state->m_current_reset_bank_counter & 1;
		if ( state->m_cart_size == 0x2000 )
			state->m_current_reset_bank_counter = 0;
		state->m_current_bank = state->m_current_reset_bank_counter * 2;
		install_banks(machine, 1, 0x1000 * state->m_current_bank);
		break;
	}

	/* set up bank counter */

	if (state->m_banking_mode == modeDC)
	{
		space->install_legacy_read_handler(0x1fec, 0x1fec, FUNC(current_bank_r));
	}

	/* set up bank switch registers */

	switch (state->m_banking_mode)
	{
	case modeF8:
		space->install_legacy_write_handler(0x1ff8, 0x1ff9, FUNC(modeF8_switch_w));
		space->install_legacy_read_handler(0x1ff8, 0x1ff9, FUNC(modeF8_switch_r));
		break;

	case modeFA:
		space->install_legacy_write_handler(0x1ff8, 0x1ffa, FUNC(modeFA_switch_w));
		space->install_legacy_read_handler(0x1ff8, 0x1ffa, FUNC(modeFA_switch_r));
		break;

	case modeF6:
		space->install_legacy_write_handler(0x1ff6, 0x1ff9, FUNC(modeF6_switch_w));
		space->install_legacy_read_handler(0x1ff6, 0x1ff9, FUNC(modeF6_switch_r));
		space->set_direct_update_handler(direct_update_delegate_create_static(modeF6_opbase, machine));
		break;

	case modeF4:
		space->install_legacy_write_handler(0x1ff4, 0x1ffb, FUNC(modeF4_switch_w));
		space->install_legacy_read_handler(0x1ff4, 0x1ffb, FUNC(modeF4_switch_r));
		break;

	case modeE0:
		space->install_legacy_write_handler(0x1fe0, 0x1ff8, FUNC(modeE0_switch_w));
		space->install_legacy_read_handler(0x1fe0, 0x1ff8, FUNC(modeE0_switch_r));
		break;

	case mode3F:
		space->install_legacy_write_handler(0x00, 0x3f, FUNC(mode3F_switch_w));
		break;

	case modeUA:
		space->install_legacy_write_handler(0x200, 0x27f, FUNC(modeUA_switch_w));
		space->install_legacy_read_handler(0x200, 0x27f, FUNC(modeUA_switch_r));
		break;

	case modeE7:
		space->install_legacy_write_handler(0x1fe0, 0x1fe7, FUNC(modeE7_switch_w));
		space->install_legacy_read_handler(0x1fe0, 0x1fe7, FUNC(modeE7_switch_r));
		space->install_legacy_write_handler(0x1fe8, 0x1feb, FUNC(modeE7_RAM_switch_w));
		space->install_legacy_read_handler(0x1fe8, 0x1feb, FUNC(modeE7_RAM_switch_r));
		space->install_readwrite_bank(0x1800, 0x18ff, "bank9");
		memory_set_bankptr(machine, "bank9", state->m_extra_RAM->base() + 4 * 256 );
		break;

	case modeDC:
		space->install_legacy_write_handler(0x1ff0, 0x1ff0, FUNC(modeDC_switch_w));
		space->install_legacy_read_handler(0x1ff0, 0x1ff0, FUNC(modeDC_switch_r));
		break;

	case modeFE:
		space->install_legacy_write_handler(0x01fe, 0x01fe, FUNC(modeFE_switch_w));
		space->install_legacy_read_handler(0x01fe, 0x01fe, FUNC(modeFE_switch_r));
		break;

	case mode3E:
		space->install_legacy_write_handler(0x3e, 0x3e, FUNC(mode3E_RAM_switch_w));
		space->install_legacy_write_handler(0x3f, 0x3f, FUNC(mode3E_switch_w));
		space->install_legacy_write_handler(0x1400, 0x15ff, FUNC(mode3E_RAM_w));
		break;

	case modeSS:
		space->install_legacy_read_handler(0x1000, 0x1fff, FUNC(modeSS_r));
		state->m_bank_base[1] = state->m_extra_RAM->base() + 2 * 0x800;
		state->m_bank_base[2] = CART;
		memory_set_bankptr(machine, "bank1", state->m_bank_base[1] );
		memory_set_bankptr(machine, "bank2", state->m_bank_base[2] );
		state->m_modeSS_write_enabled = 0;
		state->m_modeSS_byte_started = 0;
		space->set_direct_update_handler(direct_update_delegate_create_static(modeSS_opbase, machine));
		/* The Supercharger has no motor control so just enable it */
		cassette_change_state( machine.device("cassette"), CASSETTE_MOTOR_ENABLED, CASSETTE_MOTOR_DISABLED );
		break;

	case modeFV:
		space->install_legacy_write_handler(0x1fd0, 0x1fd0, FUNC(modeFV_switch_w));
		space->install_legacy_read_handler(0x1fd0, 0x1fd0, FUNC(modeFV_switch_r));
		break;

	case modeDPC:
		space->install_legacy_read_handler(0x1000, 0x103f, FUNC(modeDPC_r));
		space->install_legacy_write_handler(0x1040, 0x107f, FUNC(modeDPC_w));
		space->install_legacy_write_handler(0x1ff8, 0x1ff9, FUNC(modeF8_switch_w));
		space->install_legacy_read_handler(0x1ff8, 0x1ff9, FUNC(modeF8_switch_r));
		space->set_direct_update_handler(direct_update_delegate_create_static(modeDPC_opbase_handler, machine));
		{
			int	data_fetcher;
			for( data_fetcher = 0; data_fetcher < 8; data_fetcher++ )
			{
				state->m_dpc.df[data_fetcher].osc_clk = 0;
				state->m_dpc.df[data_fetcher].flag = 0;
				state->m_dpc.df[data_fetcher].music_mode = 0;
			}
		}
		state->m_dpc.oscillator->adjust(attotime::from_hz(42000), 0, attotime::from_hz(42000));
		break;

	case mode32in1:
		memory_set_bankptr(machine, "bank1", CART + state->m_current_reset_bank_counter * 0x800 );
		memory_set_bankptr(machine, "bank2", CART + state->m_current_reset_bank_counter * 0x800 );
		break;

	case modeJVP:
		space->install_legacy_read_handler(0x0FA0, 0x0FC0, FUNC(modeJVP_switch_r));
		space->install_legacy_write_handler(0x0FA0, 0x0FC0, FUNC(modeJVP_switch_w));
		break;
	}

	/* set up extra RAM */

	if (state->m_banking_mode == modeFA)
	{
		space->install_write_bank(0x1000, 0x10ff, "bank9");
		space->install_read_bank(0x1100, 0x11ff, "bank9");

		memory_set_bankptr(machine,"bank9", state->m_extra_RAM->base());
	}

	if (state->m_banking_mode == modeCV)
	{
		space->install_write_bank(0x1400, 0x17ff, "bank9");
		space->install_read_bank(0x1000, 0x13ff, "bank9");

		memory_set_bankptr(machine,"bank9", state->m_extra_RAM->base());
	}

	if (chip)
	{
		space->install_write_bank(0x1000, 0x107f, "bank9");
		space->install_read_bank(0x1080, 0x10ff, "bank9");

		memory_set_bankptr(machine,"bank9", state->m_extra_RAM->base());
	}

	/* Banks may have changed, reset the cpu so it uses the correct reset vector */
	machine.device("maincpu")->reset();
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
//  PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_CATEGORY(15) PORT_PLAYER(1)
//  PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_CATEGORY(15) PORT_PLAYER(1)
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_CATEGORY(15) PORT_PLAYER(1)
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_CATEGORY(12) PORT_PLAYER(1)
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_CATEGORY(10) PORT_PLAYER(1)
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_CATEGORY(10) PORT_PLAYER(1)
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_CATEGORY(10) PORT_PLAYER(1)

	PORT_START("BUTTONS_R") /* [5] right port button(s) */
//  PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_CATEGORY(25) PORT_PLAYER(2)
//  PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_CATEGORY(25) PORT_PLAYER(2)
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_CATEGORY(25) PORT_PLAYER(2)
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_CATEGORY(22) PORT_PLAYER(2)
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_CATEGORY(20) PORT_PLAYER(2)
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_CATEGORY(20) PORT_PLAYER(2)
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_CATEGORY(20) PORT_PLAYER(2)

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
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_CATEGORY(21) PORT_PLAYER(4)
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_CATEGORY(21) PORT_PLAYER(2)
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_CATEGORY(11) PORT_PLAYER(3)
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_CATEGORY(11) PORT_PLAYER(1)

	PORT_START("SWB") /* [8] SWCHB */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("Reset Game") PORT_CODE(KEYCODE_2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START ) PORT_NAME("Select Game")
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
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CATEGORY(13) PORT_NAME("left 1") PORT_CODE(KEYCODE_7_PAD)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CATEGORY(13) PORT_NAME("left 2") PORT_CODE(KEYCODE_8_PAD)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CATEGORY(13) PORT_NAME("left 3") PORT_CODE(KEYCODE_9_PAD)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CATEGORY(13) PORT_NAME("left 4") PORT_CODE(KEYCODE_4_PAD)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CATEGORY(13) PORT_NAME("left 5") PORT_CODE(KEYCODE_5_PAD)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CATEGORY(13) PORT_NAME("left 6") PORT_CODE(KEYCODE_6_PAD)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CATEGORY(13) PORT_NAME("left 7") PORT_CODE(KEYCODE_1_PAD)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CATEGORY(13) PORT_NAME("left 8") PORT_CODE(KEYCODE_2_PAD)
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CATEGORY(13) PORT_NAME("left 9") PORT_CODE(KEYCODE_3_PAD)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CATEGORY(13) PORT_NAME("left *") PORT_CODE(KEYCODE_0_PAD)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CATEGORY(13) PORT_NAME("left 0") PORT_CODE(KEYCODE_DEL_PAD)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CATEGORY(13) PORT_NAME("left #") PORT_CODE(KEYCODE_ENTER_PAD)

	PORT_START("KEYPAD_R")  /* [11] right keypad */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CATEGORY(23) PORT_NAME("right 1") PORT_CODE(KEYCODE_5)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CATEGORY(23) PORT_NAME("right 2") PORT_CODE(KEYCODE_6)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CATEGORY(23) PORT_NAME("right 3") PORT_CODE(KEYCODE_7)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CATEGORY(23) PORT_NAME("right 4") PORT_CODE(KEYCODE_R)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CATEGORY(23) PORT_NAME("right 5") PORT_CODE(KEYCODE_T)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CATEGORY(23) PORT_NAME("right 6") PORT_CODE(KEYCODE_Y)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CATEGORY(23) PORT_NAME("right 7") PORT_CODE(KEYCODE_F)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CATEGORY(23) PORT_NAME("right 8") PORT_CODE(KEYCODE_G)
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CATEGORY(23) PORT_NAME("right 9") PORT_CODE(KEYCODE_H)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CATEGORY(23) PORT_NAME("right *") PORT_CODE(KEYCODE_V)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CATEGORY(23) PORT_NAME("right 0") PORT_CODE(KEYCODE_B)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CATEGORY(23) PORT_NAME("right #") PORT_CODE(KEYCODE_N)

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
	(cassette_state)(CASSETTE_PLAY | CASSETTE_MOTOR_DISABLED | CASSETTE_SPEAKER_ENABLED),
	NULL
};

static MACHINE_CONFIG_FRAGMENT(a2600_cartslot)
	MCFG_CARTSLOT_ADD("cart")
	MCFG_CARTSLOT_EXTENSION_LIST("bin,a26")
	MCFG_CARTSLOT_MANDATORY
	MCFG_CARTSLOT_START(a2600_cart)
	MCFG_CARTSLOT_LOAD(a2600_cart)
MACHINE_CONFIG_END

static MACHINE_CONFIG_START( a2600, a2600_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", M6502, MASTER_CLOCK_NTSC / 3)	/* actually M6507 */
	MCFG_CPU_PROGRAM_MAP(a2600_mem)

	MCFG_MACHINE_START(a2600)
	MCFG_MACHINE_RESET(a2600)

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_RAW_PARAMS( MASTER_CLOCK_NTSC, 228, 26, 26 + 160 + 16, 262, 24 , 24 + 192 + 31 )
	MCFG_SCREEN_UPDATE(tia)

	MCFG_PALETTE_LENGTH( TIA_PALETTE_LENGTH )
	MCFG_PALETTE_INIT(tia_NTSC)

	MCFG_VIDEO_START(tia)

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD("tia", TIA, MASTER_CLOCK_NTSC/114)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.90)
	MCFG_SOUND_WAVE_ADD("wave", "cassette")
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.10)

	/* devices */
	MCFG_RIOT6532_ADD("riot", MASTER_CLOCK_NTSC / 3, r6532_interface)
	MCFG_FRAGMENT_ADD(a2600_cartslot)
	MCFG_CASSETTE_ADD( "cassette", a2600_cassette_config )
MACHINE_CONFIG_END


static MACHINE_CONFIG_START( a2600p, a2600_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", M6502, MASTER_CLOCK_PAL / 3)    /* actually M6507 */
	MCFG_CPU_PROGRAM_MAP(a2600_mem)

	MCFG_MACHINE_START(a2600p)
	MCFG_MACHINE_RESET(a2600)

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_RAW_PARAMS( MASTER_CLOCK_PAL, 228, 26, 26 + 160 + 16, 312, 32, 32 + 228 + 31 )
	MCFG_SCREEN_UPDATE(tia)

	MCFG_PALETTE_LENGTH( TIA_PALETTE_LENGTH )
	MCFG_PALETTE_INIT(tia_PAL)

	MCFG_VIDEO_START(tia)

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD("tia", TIA, MASTER_CLOCK_PAL/114)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.90)
	MCFG_SOUND_WAVE_ADD("wave", "cassette")
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.10)

	/* devices */
	MCFG_RIOT6532_ADD("riot", MASTER_CLOCK_PAL / 3, r6532_interface)
	MCFG_FRAGMENT_ADD(a2600_cartslot)
	MCFG_CASSETTE_ADD( "cassette", a2600_cassette_config )
MACHINE_CONFIG_END


ROM_START( a2600 )
	ROM_REGION( 0x2000, "maincpu", 0 )
	ROM_FILL( 0x0000, 0x2000, 0xFF )
	ROM_REGION( 0x80000, "user1", 0 )
	ROM_FILL( 0x00000, 0x80000, 0xFF )
ROM_END

#define rom_a2600p rom_a2600

/*    YEAR  NAME    PARENT  COMPAT  MACHINE INPUT   INIT    COMPANY     FULLNAME */
CONS( 1977,	a2600,	0,		0,		a2600,	a2600,	0,		"Atari",	"Atari 2600 (NTSC)" , 0)
CONS( 1978,	a2600p,	a2600,	0,		a2600p,	a2600,	0,		"Atari",    "Atari 2600 (PAL)",   0)

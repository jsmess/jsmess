/***************************************************************************

  gb.c

  Machine file to handle emulation of the Nintendo GameBoy.

  Changes:

	13/2/2002		AK - MBC2 and MBC3 support and added NVRAM support.
	23/2/2002		AK - MBC5 support, and MBC2 RAM support.
	13/3/2002		AK - Tidied up the MBC code, window layer now has it's
						 own palette. Tidied init code.
	15/3/2002		AK - More init code tidying with a slight hack to stop
						 sound when the machine starts.
	19/3/2002		AK - Changed NVRAM code to the new battery_* functions.
	24/3/2002		AK - Added MBC1 mode switching, and partial MBC3 RTC support.
	28/3/2002		AK - Improved LCD status timing and interrupts.
						 Free memory when we shutdown instead of leaking.
	31/3/2002		AK - Handle IO memory reading so we return 0xFF for registers
						 that are unsupported.
	 7/4/2002		AK - Free memory from battery load/save. General tidying.
	13/4/2002		AK - Ok, don't free memory when we shutdown as that causes
						 a crash on reset.
	28/4/2002		AK - General code tidying.
						 Fixed MBC3's RAM/RTC banking.
						 Added support for games with more than 128 ROM banks.
	12/6/2002		AK - Rewrote the way bg and sprite palettes are handled.
						 The window layer no longer has it's own palette.
						 Added Super GameBoy support.
	13/6/2002		AK - Added GameBoy Color support.

	17/5/2004       WP - Added Megaduck/Cougar Boy support.
	13/6/2005		WP - Added support for bootstrap rom banking.

***************************************************************************/
#define __MACHINE_GB_C

#include "driver.h"
#include "includes/gb.h"
#include "cpu/z80gb/z80gb.h"
#include "image.h"

/* Memory bank controller types */
enum {
	MBC_NONE=0,		/*  32KB ROM - No memory bank controller         */
	MBC_MBC1,		/*  ~2MB ROM,   8KB RAM -or- 512KB ROM, 32KB RAM */
	MBC_MBC2,		/* 256KB ROM,  32KB RAM                          */
	MBC_MMM01,		/*    ?? ROM,    ?? RAM                          */
	MBC_MBC3,		/*   2MB ROM,  32KB RAM, RTC                     */
	MBC_MBC4,		/*    ?? ROM,    ?? RAM                          */
	MBC_MBC5,		/*   8MB ROM, 128KB RAM (32KB w/ Rumble)         */
	MBC_TAMA5,		/*    ?? ROM     ?? RAM - What is this?          */
	MBC_HUC1,		/*    ?? ROM,    ?? RAM - Hudson Soft Controller */
	MBC_HUC3,		/*    ?? ROM,    ?? RAM - Hudson Soft Controller */
	MBC_MBC7,		/*    ?? ROM,    ?? RAM                          */
	MBC_WISDOM,		/*    ?? ROM,    ?? RAM - Wisdom tree controller */
	MBC_MEGADUCK,	/* MEGADUCK style banking                        */
	MBC_UNKNOWN,	/* Unknown mapper                                */
};

/* mess_ram layout defines */
#define CGB_START_VRAM_BANKS	0x0000
#define CGB_START_RAM_BANKS	( 2 * 8 * 1024 )

#define MAX_ROMBANK 512
#define MAX_RAMBANK 256

#define JOYPAD		gb_io[0x00]	/* Joystick: 1.1.P15.P14.P13.P12.P11.P10       */
#define SIODATA		gb_io[0x01]	/* Serial IO data buffer                       */
#define SIOCONT		gb_io[0x02]	/* Serial IO control register                  */
#define DIVREG		gb_io[0x04]	/* Divider register (???)                      */
#define TIMECNT		gb_io[0x05]	/* Timer counter. Gen. int. when it overflows  */
#define TIMEMOD		gb_io[0x06]	/* New value of TimeCount after it overflows   */
#define TIMEFRQ		gb_io[0x07]	/* Timer frequency and start/stop switch       */

static UINT16 MBCType;				   /* MBC type: 0 for none                        */
static UINT8 CartType;				   /* Cart Type (battery, ram, timer etc)         */
static UINT8 *ROMMap[MAX_ROMBANK];		   /* Addresses of ROM banks                      */
static UINT16 ROMBank;				   /* Number of ROM bank currently used           */
static UINT16 ROMBank00;			   /* Number of ROM bank currently used at 0000-3FFF */
static UINT8 ROMMask;				   /* Mask for the ROM bank number                */
static UINT16 ROMBanks;				   /* Total number of ROM banks                   */
static UINT8 *RAMMap[MAX_RAMBANK];		   /* Addresses of RAM banks                      */
static UINT8 RAMBank;				   /* Number of RAM bank currently used           */
static UINT8 RAMMask;				   /* Mask for the RAM bank number                */
static UINT8 RAMBanks;				   /* Total number of RAM banks                   */
static UINT8 MBC1Mode;				   /* MBC1 ROM/RAM mode                           */
static UINT8 *MBC3RTCData;			   /* MBC3 actual RTC data                        */
static UINT8 MBC3RTCMap[5];			   /* MBC3 Real-Time-Clock banks                  */
static UINT8 MBC3RTCBank;			   /* Number of RTC bank for MBC3                 */
static UINT8 *GBC_RAMMap[8];		   /* (GBC) Addresses of internal RAM banks       */
static UINT8 GBC_RAMBank;			   /* (GBC) Number of RAM bank currently used     */
UINT8 *GBC_VRAMMap[2];				   /* (GBC) Addressses of video RAM banks         */
UINT8 *gbc_vram_bank;
static UINT8 sgb_atf_data[4050];	   /* (SGB) Attribute files                       */
UINT8 *sgb_tile_data;
UINT8 *gb_cart = NULL;
UINT8 *gb_cart_ram = NULL;
UINT8 gb_io[0x10];
UINT8 gb_ie;
UINT8 *gb_dummy_rom_bank = NULL;
UINT8 *gb_dummy_ram_bank = NULL;
/* TAMA5 related global variables */
UINT8 gbTama5Memory[32];
UINT8 gbTama5Byte;
UINT8 gbTama5Address;
UINT8 gbLastTama5Command;
/* Timer related globals */
UINT16	gb_divcount;
UINT8 gb_timer_count;
UINT8 gb_timer_shift;
/* Serial I/O related */
static UINT32 SIOCount;			/* Serial I/O counter                          */
mame_timer	*gb_serial_timer = NULL;

/*
  Prototypes
*/

static void gb_serial_timer_proc( int dummy );
static void gb_machine_stop(running_machine *machine);
WRITE8_HANDLER( gb_rom_bank_select_mbc1 );
WRITE8_HANDLER( gb_ram_bank_select_mbc1 );
WRITE8_HANDLER( gb_mem_mode_select_mbc1 );
WRITE8_HANDLER( gb_rom_bank_select_mbc2 );
WRITE8_HANDLER( gb_rom_bank_select_mbc3 );
WRITE8_HANDLER( gb_ram_bank_select_mbc3 );
WRITE8_HANDLER( gb_mem_mode_select_mbc3 );
WRITE8_HANDLER( gb_rom_bank_select_mbc5 );
WRITE8_HANDLER( gb_ram_bank_select_mbc5 );
WRITE8_HANDLER( gb_rom_bank_select_mbc7 );
WRITE8_HANDLER( gb_rom_bank_unknown_mbc7 );
WRITE8_HANDLER( gb_ram_tama5 );
WRITE8_HANDLER( gb_rom_bank_select_wisdom );
WRITE8_HANDLER( gb_rom_bank_mmm01_0000_w );
WRITE8_HANDLER( gb_rom_bank_mmm01_2000_w );
WRITE8_HANDLER( gb_rom_bank_mmm01_4000_w );
WRITE8_HANDLER( gb_rom_bank_mmm01_6000_w );

#ifdef MAME_DEBUG
/* #define V_GENERAL*/		/* Display general debug information */
/* #define V_BANK*/			/* Display bank switching debug information */
#endif

static void gb_init_regs(void) {
	/* Initialize the registers */
	SIODATA = 0x00;
	SIOCONT = 0x7E;

	gb_io_w( 0x05, 0x00 );		/* TIMECNT */
	gb_io_w( 0x06, 0x00 );		/* TIMEMOD */
	gb_io_w( 0x07, 0x00 );		/* TIMEFRQ */
	gb_video_w( 0x0, 0x91 );	/* LCDCONT */
	gb_video_w( 0x7, 0xFC );	/* BGRDPAL */
	gb_video_w( 0x8, 0xFC );	/* SPR0PAL */
	gb_video_w( 0x9, 0xFC );	/* SPR1PAL */
}

static void gb_init(void) {
	gb_vram = memory_get_read_ptr( 0, ADDRESS_SPACE_PROGRAM, 0x8000 );

	/* Initialize the memory banks */
	MBC1Mode = 0;
	MBC3RTCBank = 0;
	ROMBank = ROMBank00 + 1;
	RAMBank = 0;
	memory_set_bankptr (1, ROMMap[ROMBank] ? ROMMap[ROMBank] : gb_dummy_rom_bank);
	if ( MBCType != MBC_MEGADUCK ) {
		memory_set_bankptr (2, RAMMap[RAMBank] ? RAMMap[RAMBank] : gb_dummy_ram_bank);
	} else {
		memory_set_bankptr( 10, ROMMap[0] );
	}

	/* Set handlers based on the Memory Bank Controller in the cart */
	switch( MBCType )
	{
		case MBC_NONE:
			memory_install_write8_handler( 0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x1fff, 0, 0, MWA8_ROM );
			memory_install_write8_handler( 0, ADDRESS_SPACE_PROGRAM, 0x2000, 0x3fff, 0, 0, MWA8_ROM );
			memory_install_write8_handler( 0, ADDRESS_SPACE_PROGRAM, 0x4000, 0x5fff, 0, 0, MWA8_ROM );
			memory_install_write8_handler( 0, ADDRESS_SPACE_PROGRAM, 0x6000, 0x7fff, 0, 0, MWA8_ROM );
			break;
		case MBC_MMM01:
			memory_install_write8_handler( 0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x1fff, 0, 0, gb_rom_bank_mmm01_0000_w );
			memory_install_write8_handler( 0, ADDRESS_SPACE_PROGRAM, 0x2000, 0x3fff, 0, 0, gb_rom_bank_mmm01_2000_w);
			memory_install_write8_handler( 0, ADDRESS_SPACE_PROGRAM, 0x4000, 0x5fff, 0, 0, gb_rom_bank_mmm01_4000_w);
			memory_install_write8_handler( 0, ADDRESS_SPACE_PROGRAM, 0x6000, 0x7fff, 0, 0, gb_rom_bank_mmm01_6000_w);
			break;
		case MBC_MBC1:
			memory_install_write8_handler( 0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x1fff, 0, 0, gb_ram_enable );	/* We don't emulate RAM enable yet */
			memory_install_write8_handler( 0, ADDRESS_SPACE_PROGRAM, 0x2000, 0x3fff, 0, 0, gb_rom_bank_select_mbc1 );
			memory_install_write8_handler( 0, ADDRESS_SPACE_PROGRAM, 0x4000, 0x5fff, 0, 0, gb_ram_bank_select_mbc1 );
			memory_install_write8_handler( 0, ADDRESS_SPACE_PROGRAM, 0x6000, 0x7fff, 0, 0, gb_mem_mode_select_mbc1 );
			break;
		case MBC_MBC2:
			memory_install_write8_handler( 0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x1fff, 0, 0, MWA8_ROM );
			memory_install_write8_handler( 0, ADDRESS_SPACE_PROGRAM, 0x2000, 0x3fff, 0, 0, gb_rom_bank_select_mbc2 );
			memory_install_write8_handler( 0, ADDRESS_SPACE_PROGRAM, 0x4000, 0x5fff, 0, 0, MWA8_ROM );
			memory_install_write8_handler( 0, ADDRESS_SPACE_PROGRAM, 0x6000, 0x7fff, 0, 0, MWA8_ROM );
			break;
		case MBC_MBC3:
		case MBC_HUC1:	/* Possibly wrong */
		case MBC_HUC3:	/* Possibly wrong */
			memory_install_write8_handler( 0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x1fff, 0, 0, gb_ram_enable );	/* We don't emulate RAM enable yet */
			memory_install_write8_handler( 0, ADDRESS_SPACE_PROGRAM, 0x2000, 0x3fff, 0, 0, gb_rom_bank_select_mbc3 );
			memory_install_write8_handler( 0, ADDRESS_SPACE_PROGRAM, 0x4000, 0x5fff, 0, 0, gb_ram_bank_select_mbc3 );
			memory_install_write8_handler( 0, ADDRESS_SPACE_PROGRAM, 0x6000, 0x7fff, 0, 0, gb_mem_mode_select_mbc3 );
			break;
		case MBC_MBC5:
			memory_install_write8_handler( 0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x1fff, 0, 0, gb_ram_enable );
			memory_install_write8_handler( 0, ADDRESS_SPACE_PROGRAM, 0x2000, 0x3fff, 0, 0, gb_rom_bank_select_mbc5 );
			memory_install_write8_handler( 0, ADDRESS_SPACE_PROGRAM, 0x4000, 0x5fff, 0, 0, gb_ram_bank_select_mbc5 );
			memory_install_write8_handler( 0, ADDRESS_SPACE_PROGRAM, 0x6000, 0x7fff, 0, 0, MWA8_ROM );
			break;
		case MBC_MBC7:
			memory_install_write8_handler( 0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x1fff, 0, 0, gb_ram_enable );
			memory_install_write8_handler( 0, ADDRESS_SPACE_PROGRAM, 0x2000, 0x2fff, 0, 0, gb_rom_bank_select_mbc7 );
			memory_install_write8_handler( 0, ADDRESS_SPACE_PROGRAM, 0x3000, 0x7fff, 0, 0, gb_rom_bank_unknown_mbc7 );
			break;
		case MBC_TAMA5:
			memory_install_write8_handler( 0, ADDRESS_SPACE_PROGRAM, 0xA000, 0xBFFF, 0, 0, gb_ram_tama5 );
			break;
		case MBC_WISDOM:
			memory_install_write8_handler( 0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x3fff, 0, 0, gb_rom_bank_select_wisdom );
			break;
		case MBC_MEGADUCK:
			memory_install_write8_handler( 0, ADDRESS_SPACE_PROGRAM, 0x0001, 0x0001, 0, 0, megaduck_rom_bank_select_type1 );
			memory_install_write8_handler( 0, ADDRESS_SPACE_PROGRAM, 0xB000, 0xB000, 0, 0, megaduck_rom_bank_select_type2 );
			break;
	}

	gb_sound_w( 0x16, 0x00 );       /* Initialize sound hardware */

	/* Allocate the serial timer, and disable it */
	gb_serial_timer = mame_timer_alloc( gb_serial_timer_proc );
	mame_timer_enable( gb_serial_timer, 0 );

}

MACHINE_START( gb )
{
	add_exit_callback(machine, gb_machine_stop);
	return 0;
}

MACHINE_RESET( gb )
{
        gb_init();

	gb_video_init();

        /* Enable BIOS rom */
        memory_set_bankptr(5, memory_region(REGION_CPU1) );
        memory_set_bankptr(10, ROMMap[ROMBank00] + 0x0100 );
}

MACHINE_RESET( sgb )
{
	gb_init();

	sgb_video_init();

	gb_init_regs();

	memory_set_bankptr(5, ROMMap[ROMBank00] ? ROMMap[ROMBank00] : gb_dummy_rom_bank );
	memory_set_bankptr(10, ROMMap[ROMBank00] ? ROMMap[ROMBank00] + 0x0100 : gb_dummy_rom_bank + 0x0100 );

	if ( sgb_tile_data == NULL ) {
		sgb_tile_data = auto_malloc( 0x2000 );
	}
	memset( sgb_tile_data, 0, 0x2000 );

	/* Initialize the Sound Registers */
	gb_sound_w(0x16,0xF0);	/* F0 for SGB */
	gb_sound_w(0x15,0xF3);
	gb_sound_w(0x14,0x77);

	sgb_window_mask = 0;
	memset( sgb_pal_map, 0, 20*18 );
	memset( sgb_atf_data, 0, 4050 );

	/* HACKS for Donkey Kong Land 2 + 3.
	   For some reason that I haven't figured out, they store the tile
	   data differently.  Hacks will go once I figure it out */
	sgb_hack = 0;
	if( strncmp( (const char*)(gb_cart + 0x134), "DONKEYKONGLAND 2", 16 ) == 0 ||
		strncmp( (const char*)(gb_cart + 0x134), "DONKEYKONGLAND 3", 16 ) == 0 )
	{
		sgb_hack = 1;
	}
}

MACHINE_RESET( gbpocket )
{
	gb_init();

	gb_video_init();

	gb_init_regs();

	/* Initialize the Sound registers */
	gb_sound_w(0x16,0x80);
	gb_sound_w(0x15,0xF3);
	gb_sound_w(0x14,0x77);

	/* Enable BIOS rom if we have one */
	memory_set_bankptr(5, ROMMap[ROMBank00] ? ROMMap[ROMBank00] : gb_dummy_rom_bank );
	memory_set_bankptr(10, ROMMap[ROMBank00] ? ROMMap[ROMBank00] + 0x0100 : gb_dummy_rom_bank + 0x0100);
}

MACHINE_RESET( gbc )
{
	int ii;

	gb_init();

	/* Allocate memory for video ram */
	for( ii = 0; ii < 2; ii++ ) {
		GBC_VRAMMap[ii] = mess_ram + CGB_START_VRAM_BANKS + ii * 0x2000;
		memset (GBC_VRAMMap[ii], 0, 0x2000);
	}
	gbc_io2_w( 0x0F, 0x00 );

	gbc_video_init();

	gb_init_regs();

	/* Initialize the Sound registers */
	gb_sound_w(0x16, 0x80);
	gb_sound_w(0x15, 0xF3);
	gb_sound_w(0x14, 0x77);

	memory_set_bankptr(5, ROMMap[ROMBank00] ? ROMMap[ROMBank00] : gb_dummy_rom_bank );
	memory_set_bankptr(10, ROMMap[ROMBank00] ? ROMMap[ROMBank00] + 0x0100 : gb_dummy_rom_bank + 0x0100);

	/* Allocate memory for internal ram */
	for( ii = 0; ii < 8; ii++ ) {
		GBC_RAMMap[ii] = mess_ram + CGB_START_RAM_BANKS + ii * 0x1000;
		memset (GBC_RAMMap[ii], 0, 0x1000);
	}
	gbc_io2_w( 0x30, 0x00 );

	/* Initialise registers */
	gb_io_w( 0x6C, 0xFE );
	gb_io_w( 0x72, 0x00 );
	gb_io_w( 0x73, 0x00 );
	gb_io_w( 0x74, 0x8F );
	gb_io_w( 0x75, 0x00 );
	gb_io_w( 0x76, 0x00 );
	gb_io_w( 0x77, 0x00 );

	/* Are we in colour or mono mode? */
	if( gb_cart[0x143] == 0x80 || gb_cart[0x143] == 0xC0 )
		gbc_mode = GBC_MODE_GBC;
	else
		gbc_mode = GBC_MODE_MONO;
}

static void gb_machine_stop(running_machine *machine)
{
	/* Don't save if there was no battery */
	if( !(CartType & BATTERY) || ! RAMBanks )
		return;

	/* NOTE: The reason we save the carts RAM this way instead of using MAME's
	   built in macros is because they force the filename to be the name of
	   the machine.  We need to have a separate name for each game. */
	image_battery_save(image_from_devtype_and_index(IO_CARTSLOT, 0), gb_cart_ram, RAMBanks * 0x2000 );
}

void gb_set_mbc1_banks( void ) {
	memory_set_bankptr( 1, ROMMap[ ROMBank ] );
	memory_set_bankptr( 2, RAMMap[ MBC1Mode ? ( ROMBank >> 5 ) : 0 ] );
}

WRITE8_HANDLER( gb_rom_bank_select_mbc1 )
{
	data &= 0x1F; /* Only uses lower 5 bits */
	/* Selecting bank 0 == selecting bank 1 */
	if( data == 0 )
		data = 1;

	ROMBank = ( ROMBank & 0x01E0 ) | data;
	/* Switch banks */
	gb_set_mbc1_banks();
}

WRITE8_HANDLER( gb_rom_bank_select_mbc2 )
{
	data &= 0x0F; /* Only uses lower 4 bits */
	/* Selecting bank 0 == selecting bank 1 */
	if( data == 0 )
		data = 1;

	/* The least significant bit of the upper address byte must be 1 */
	if( offset & 0x0100 )
		ROMBank = ( ROMBank & 0x100 ) | data;
	/* Switch banks */
	memory_set_bankptr (1, ROMMap[ROMBank] );
}

WRITE8_HANDLER( gb_rom_bank_select_mbc3 )
{
	logerror( "0x%04X: write to mbc3 rom bank select register 0x%04X <- 0x%02X\n", activecpu_get_pc(), offset, data );
	data &= 0x7F; /* Only uses lower 7 bits */
	/* Selecting bank 0 == selecting bank 1 */
	if( data == 0 )
		data = 1;

	ROMBank = ( ROMBank & 0x0100 ) | data;
	/* Switch banks */
	memory_set_bankptr (1, ROMMap[ROMBank] );
}

WRITE8_HANDLER( gb_rom_bank_select_mbc5 )
{
	/* MBC5 has a 9 bit bank select
	  Writing into 2000-2FFF sets the lower 8 bits
	  Writing into 3000-3FFF sets the 9th bit
	*/
	logerror( "0x%04X: MBC5 ROM Bank select write 0x%04X <- 0x%02X\n", activecpu_get_pc(), offset, data );
	if( offset & 0x1000 ) {
		ROMBank = (ROMBank & 0xFF ) | ( ( data & 0x01 ) << 8 );
	} else {
		ROMBank = (ROMBank & 0x100 ) | data;
	}
	/* Switch banks */
	memory_set_bankptr (1, ROMMap[ROMBank] );
}

WRITE8_HANDLER( gb_rom_bank_select_mbc7 ) {
	logerror( "0x%04X: write to mbc7 rom select register: 0x%04X <- 0x%02X\n", activecpu_get_pc(), 0x2000 + offset, data );
	/* Bit 12 must be set for writing to the mbc register */
	if ( offset & 0x0100 ) {
		ROMBank = data;
		memory_set_bankptr( 1, ROMMap[ROMBank] );
	}
}

WRITE8_HANDLER( gb_rom_bank_unknown_mbc7 ) {
        logerror( "0x%04X: write to mbc7 rom area: 0x%04X <- 0x%02X\n", activecpu_get_pc(), 0x3000 + offset, data );
	/* Bit 12 must be set for writing to the mbc register */
	if ( offset & 0x0100 ) {
		switch( offset & 0x7000 ) {
		case 0x0000:	/* 0x3000-0x3fff */
		case 0x1000:	/* 0x4000-0x4fff */
		case 0x2000:	/* 0x5000-0x5fff */
		case 0x3000:	/* 0x6000-0x6fff */
		case 0x4000:	/* 0x7000-0x7fff */
			break;
		}
	}
}

WRITE8_HANDLER( gb_rom_bank_select_wisdom ) {
	logerror( "0x%04X: wisdom tree mapper write to address 0x%04X\n", activecpu_get_pc(), offset );
	/* The address determines the bank to select */
	ROMBank = ( offset << 1 ) & 0x1FF;
	memory_set_bankptr( 5, ROMMap[ ROMBank ] );
	memory_set_bankptr( 10, ROMMap[ ROMBank ] + 0x0100 );
	memory_set_bankptr( 1, ROMMap[ ROMBank + 1 ] );
}

WRITE8_HANDLER( gb_ram_bank_select_mbc1 )
{
	data &= 0x3; /* Only uses the lower 2 bits */

	/* Select the upper bits of the ROMMask */
	ROMBank = ( ROMBank & 0x1F ) | ( data << 5 );

	/* Switch banks */
	gb_set_mbc1_banks();
}

WRITE8_HANDLER( gb_ram_bank_select_mbc3 )
{
	logerror( "0x%04X: write mbc3 ram bank select register 0x%04X <- 0x%02X\n", activecpu_get_pc(), offset, data );
	if( data & 0x8 ) {	/* RTC banks */
		if ( CartType & TIMER ) {
			MBC3RTCBank = data & 0x07;
			if ( data < 5 ) {
				memset( MBC3RTCData, MBC3RTCMap[MBC3RTCBank], 0x2000 );
				memory_set_bankptr( 2, MBC3RTCData );
			}
		}
	} else {	/* RAM banks */
		RAMBank = data & 0x3;
		MBC3RTCBank = 0xFF;
		/* Switch banks */
		memory_set_bankptr( 2, RAMMap[RAMBank] );
	}
}

WRITE8_HANDLER( gb_ram_bank_select_mbc5 )
{
	logerror( "0x%04X: MBC5 RAM Bank select write 0x%04X <- 0x%02X\n", activecpu_get_pc(), offset, data );
	data &= 0x0F;
	if( CartType & RUMBLE ) {
		data &= 0x7;
	}
	RAMBank = data;
	/* Switch banks */
	memory_set_bankptr (2, RAMMap[RAMBank] );
}

WRITE8_HANDLER ( gb_ram_enable )
{
	/* FIXME: Currently we don't handle this, but a value of 0xA will enable
	 * writing to the cart's RAM banks */
	logerror( "0x%04X: Write to ram enable register 0x%04X <- 0x%02X\n", activecpu_get_pc(), offset, data );
}

WRITE8_HANDLER( gb_mem_mode_select_mbc1 )
{
	MBC1Mode = data & 0x1;
	gb_set_mbc1_banks();
}

WRITE8_HANDLER( gb_mem_mode_select_mbc3 )
{
        logerror( "0x%04X: Write to mbc3 mem mode select register 0x%04X <- 0x%02X\n", activecpu_get_pc(), offset, data );
	if( CartType & TIMER ) {
		/* FIXME: RTC Latch goes here */
		MBC3RTCMap[0] = 50;    /* Seconds */
		MBC3RTCMap[1] = 40;    /* Minutes */
		MBC3RTCMap[2] = 15;    /* Hours */
		MBC3RTCMap[3] = 25;    /* Day counter lowest 8 bits */
		MBC3RTCMap[4] = 0x01;  /* Day counter upper bit, timer off, no day overflow occured (bit7) */
	}
}

WRITE8_HANDLER( gb_ram_tama5 ) {
	logerror( "0x%04X: TAMA5 write 0x%04X <- 0x%02X\n", activecpu_get_pc(), 0xA000 + offset, data );
	switch( offset & 0x0001 ) {
	case 0x0000:    /* Write to data register */
		switch( gbLastTama5Command ) {
		case 0x00:      /* Bits 0-3 for rom bank selection */
			ROMBank = ( ROMBank & 0xF0 ) | ( data & 0x0F );
			memory_set_bankptr (1, ROMMap[ROMBank] );
			break;
		case 0x01:      /* Bit 4(-7?) for rom bank selection */
			ROMBank = ( ROMBank & 0x0F ) | ( ( data & 0x0F ) << 4 );
			memory_set_bankptr (1, ROMMap[ROMBank] );
			break;
		case 0x04:      /* Data to write lo */
			gbTama5Byte = ( gbTama5Byte & 0xF0 ) | ( data & 0x0F );
			break;
		case 0x05:      /* Data to write hi */
			gbTama5Byte = ( gbTama5Byte & 0x0F ) | ( ( data & 0x0F ) << 4 );
			break;
		case 0x06:      /* Address selection hi */
			gbTama5Address = ( gbTama5Address & 0x0F ) | ( ( data & 0x0F ) << 4 );
			break;
		case 0x07:      /* Address selection lo */
				/* This address always seems to written last, so we'll just
				   execute the command here */
			gbTama5Address = ( gbTama5Address & 0xF0 ) | ( data & 0x0F );
			switch ( gbTama5Address & 0xE0 ) {
			case 0x00:      /* Write memory */
				logerror( "Write tama5 memory 0x%02X <- 0x%02X\n", gbTama5Address & 0x1F, gbTama5Byte );
				gbTama5Memory[ gbTama5Address & 0x1F ] = gbTama5Byte;
				break;
			case 0x20:      /* Read memory */
				logerror( "Read tama5 memory 0x%02X\n", gbTama5Address & 0x1F );
				gbTama5Byte = gbTama5Memory[ gbTama5Address & 0x1F ];
				break;
			case 0x40:      /* Unknown, some kind of read */
				if ( ( gbTama5Address & 0x1F ) == 0x12 ) {
					gbTama5Byte = 0xFF;
				}
			case 0x80:      /* Unknown, some kind of read (when 07=01)/write (when 07=00/02) */
			default:
				logerror( "0x%04X: Unknown addressing mode\n", activecpu_get_pc() );
				break;
			}
			break;
		}
		break;
	case 0x0001:    /* Write to control register */
		switch( data ) {
		case 0x00:      /* Bits 0-3 for rom bank selection */
		case 0x01:      /* Bits 4-7 for rom bank selection */
		case 0x04:      /* Data write register lo */
		case 0x05:      /* Data write register hi */
		case 0x06:      /* Address register hi */
		case 0x07:      /* Address register lo */
			break;
		case 0x0A:      /* Are we ready for the next command? */
			MBC3RTCData[0] = 0x01;
			memory_set_bankptr( 2, MBC3RTCData );
			break;
		case 0x0C:      /* Data read register lo */
			MBC3RTCData[0] = gbTama5Byte & 0x0F;
			break;
		case 0x0D:      /* Data read register hi */
			MBC3RTCData[0] = ( gbTama5Byte & 0xF0 ) >> 4;
			break;
		default:
			logerror( "0x%04X: Unknown tama5 command 0x%02X\n", activecpu_get_pc(), data );
			break;
		}
		gbLastTama5Command = data;
		break;
	}
}

/* This mmm01 implementation is mostly guess work, no clue how correct it all is */

UINT8 mmm01_bank_offset;
UINT8 mmm01_reg1;
UINT8 mmm01_bank;
UINT8 mmm01_bank_mask;

WRITE8_HANDLER( gb_rom_bank_mmm01_0000_w ) {
	logerror( "0x%04X: write 0x%02X to 0x%04X\n", activecpu_get_pc(), data, offset+0x000 );
	if ( data & 0x40 ) {
		mmm01_bank_offset = mmm01_reg1;
		memory_set_bankptr( 5, ROMMap[ mmm01_bank_offset ] );
		memory_set_bankptr( 10, ROMMap[ mmm01_bank_offset ] + 0x0100 );
		memory_set_bankptr( 1, ROMMap[ mmm01_bank_offset + mmm01_bank ] );
	}
}

WRITE8_HANDLER( gb_rom_bank_mmm01_2000_w ) {
	logerror( "0x%04X: write 0x%02X to 0x%04X\n", activecpu_get_pc(), data, offset+0x2000 );

	mmm01_reg1 = data & ROMMask;
	mmm01_bank = mmm01_reg1 & mmm01_bank_mask;
	if ( mmm01_bank == 0 ) {
		mmm01_bank = 1;
	}
	memory_set_bankptr( 1, ROMMap[ mmm01_bank_offset + mmm01_bank ] );
}

WRITE8_HANDLER( gb_rom_bank_mmm01_4000_w ) {
	logerror( "0x%04X: write 0x%02X to 0x%04X\n", activecpu_get_pc(), data, offset+0x4000 );
}

WRITE8_HANDLER( gb_rom_bank_mmm01_6000_w ) {
	logerror( "0x%04X: write 0x%02X to 0x%04X\n", activecpu_get_pc(), data, offset+0x6000 );
	/* Not sure if this is correct, Taito Variety Pack sets these values */
	/* Momotarou Collection 2 writes 01 and 21 here */
	switch( data ) {
	case 0x30:	mmm01_bank_mask = 0x07;	break;
	case 0x38:	mmm01_bank_mask = 0x03;	break;
	default:	mmm01_bank_mask = 0xFF; break;
	}
}

WRITE8_HANDLER ( gb_io_w )
{
	static UINT8 timer_shifts[4] = {10, 4, 6, 8};

	switch (offset)
	{
	case 0x00:						/* JOYP - Joypad */
		JOYPAD = 0xCF | data;
		if (!(data & 0x20))
			JOYPAD &= (readinputport (0) >> 4) | 0xF0;
		if (!(data & 0x10))
			JOYPAD &= readinputport (0) | 0xF0;
		return;
	case 0x01:						/* SB - Serial transfer data */
		break;
	case 0x02:						/* SC - SIO control */
		switch( data & 0x81 ) {
		case 0x00:
		case 0x01:
		case 0x80:				/* enabled & external clock */
			SIOCount = 0;
			break;
		case 0x81:				/* enabled & internal clock */
			SIODATA = 0xFF;
			SIOCount = 8;
			mame_timer_adjust( gb_serial_timer, MAME_TIME_IN_CYCLES( 512, 0 ), 0, MAME_TIME_IN_CYCLES( 512, 0 ) );
			mame_timer_enable( gb_serial_timer, 1 );
			break;
		}
		break;
	case 0x04:						/* DIV - Divider register */
		gb_divcount = 0xFFF7;				/* The actual value here is closely tied with some implementation details of the z80gb cpu core */
		return;
	case 0x05:						/* TIMA - Timer counter */
		break;
	case 0x06:						/* TMA - Timer module */
		break;
	case 0x07:						/* TAC - Timer control */
		data |= 0xF8;
		gb_timer_shift = timer_shifts[data & 0x03];
		break;
	case 0x0F:						/* IF - Interrupt flag */
		data &= 0x1F;
		cpunum_set_reg( 0, Z80GB_IF, data );
		break;
	}

	gb_io [offset] = data;
}

WRITE8_HANDLER ( gb_io2_w )
{
	if ( offset == 0x10 ) {
		/* disable BIOS ROM */
		memory_set_bankptr(5, ROMMap[ROMBank00]);
	} else {
		gb_video_w( offset, data );
	}
}

#ifdef MAME_DEBUG
static const char *sgbcmds[26] =
{
	"PAL01   ",
	"PAL23   ",
	"PAL03   ",
	"PAL12   ",
	"ATTR_BLK",
	"ATTR_LIN",
	"ATTR_DIV",
	"ATTR_CHR",
	"SOUND   ",
	"SOU_TRN ",
	"PAL_SET ",
	"PAL_TRN ",
	"ATRC_EN ",
	"TEST_EN ",
	"ICON_EN ",
	"DATA_SND",
	"DATA_TRN",
	"MLT_REG ",
	"JUMP    ",
	"CHR_TRN ",
	"PCT_TRN ",
	"ATTR_TRN",
	"ATTR_SET",
	"MASK_EN ",
	"OBJ_TRN ",
	"????????",
};
#endif

WRITE8_HANDLER ( sgb_io_w )
{
	static UINT8 sgb_bitcount = 0, sgb_bytecount = 0, sgb_start = 0, sgb_rest = 0;
	static UINT8 sgb_controller_no = 0, sgb_controller_mode = 0;
	static INT8 sgb_packets = -1;
	static UINT8 sgb_data[112];
	static UINT32 sgb_atf;

	switch( offset )
	{
		case 0x00:
			switch (data & 0x30)
			{
			case 0x00:				   /* start condition */
				if (sgb_start)
					logerror("SGB: Start condition before end of transfer ??");
				sgb_bitcount = 0;
				sgb_start = 1;
				sgb_rest = 0;
				JOYPAD = 0x0F & ((readinputport (0) >> 4) | readinputport (0) | 0xF0);
				break;
			case 0x10:				   /* data true */
				if (sgb_rest)
				{
					/* We should test for this case , but the code below won't
					   work with the current setup */
					/* if (sgb_bytecount == 16)
					{
						logerror("SGB: end of block is not zero!");
						sgb_start = 0;
					}*/
					sgb_data[sgb_bytecount] >>= 1;
					sgb_data[sgb_bytecount] |= 0x80;
					sgb_bitcount++;
					if (sgb_bitcount == 8)
					{
						sgb_bitcount = 0;
						sgb_bytecount++;
					}
					sgb_rest = 0;
				}
				JOYPAD = 0x1F & ((readinputport (0) >> 4) | 0xF0);
				break;
			case 0x20:				/* data false */
				if (sgb_rest)
				{
					if( sgb_bytecount == 16 && sgb_packets == -1 )
					{
#ifdef MAME_DEBUG
						logerror("SGB: %s (%02X) pkts: %d data: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\n",
								sgbcmds[sgb_data[0] >> 3],sgb_data[0] >> 3, sgb_data[0] & 0x07, sgb_data[1], sgb_data[2], sgb_data[3],
								sgb_data[4], sgb_data[5], sgb_data[6], sgb_data[7],
								sgb_data[8], sgb_data[9], sgb_data[10], sgb_data[11],
								sgb_data[12], sgb_data[13], sgb_data[14], sgb_data[15]);
#endif
						sgb_packets = sgb_data[0] & 0x07;
						sgb_start = 0;
					}
					if (sgb_bytecount == (sgb_packets << 4) )
					{
						switch( sgb_data[0] >> 3 )
						{
							case 0x00:	/* PAL01 */
								Machine->remapped_colortable[0*4 + 0] = sgb_data[1] | (sgb_data[2] << 8);
								Machine->remapped_colortable[0*4 + 1] = sgb_data[3] | (sgb_data[4] << 8);
								Machine->remapped_colortable[0*4 + 2] = sgb_data[5] | (sgb_data[6] << 8);
								Machine->remapped_colortable[0*4 + 3] = sgb_data[7] | (sgb_data[8] << 8);
								Machine->remapped_colortable[1*4 + 0] = sgb_data[1] | (sgb_data[2] << 8);
								Machine->remapped_colortable[1*4 + 1] = sgb_data[9] | (sgb_data[10] << 8);
								Machine->remapped_colortable[1*4 + 2] = sgb_data[11] | (sgb_data[12] << 8);
								Machine->remapped_colortable[1*4 + 3] = sgb_data[13] | (sgb_data[14] << 8);
								break;
							case 0x01:	/* PAL23 */
								Machine->remapped_colortable[2*4 + 0] = sgb_data[1] | (sgb_data[2] << 8);
								Machine->remapped_colortable[2*4 + 1] = sgb_data[3] | (sgb_data[4] << 8);
								Machine->remapped_colortable[2*4 + 2] = sgb_data[5] | (sgb_data[6] << 8);
								Machine->remapped_colortable[2*4 + 3] = sgb_data[7] | (sgb_data[8] << 8);
								Machine->remapped_colortable[3*4 + 0] = sgb_data[1] | (sgb_data[2] << 8);
								Machine->remapped_colortable[3*4 + 1] = sgb_data[9] | (sgb_data[10] << 8);
								Machine->remapped_colortable[3*4 + 2] = sgb_data[11] | (sgb_data[12] << 8);
								Machine->remapped_colortable[3*4 + 3] = sgb_data[13] | (sgb_data[14] << 8);
								break;
							case 0x02:	/* PAL03 */
								Machine->remapped_colortable[0*4 + 0] = sgb_data[1] | (sgb_data[2] << 8);
								Machine->remapped_colortable[0*4 + 1] = sgb_data[3] | (sgb_data[4] << 8);
								Machine->remapped_colortable[0*4 + 2] = sgb_data[5] | (sgb_data[6] << 8);
								Machine->remapped_colortable[0*4 + 3] = sgb_data[7] | (sgb_data[8] << 8);
								Machine->remapped_colortable[3*4 + 0] = sgb_data[1] | (sgb_data[2] << 8);
								Machine->remapped_colortable[3*4 + 1] = sgb_data[9] | (sgb_data[10] << 8);
								Machine->remapped_colortable[3*4 + 2] = sgb_data[11] | (sgb_data[12] << 8);
								Machine->remapped_colortable[3*4 + 3] = sgb_data[13] | (sgb_data[14] << 8);
								break;
							case 0x03:	/* PAL12 */
								Machine->remapped_colortable[1*4 + 0] = sgb_data[1] | (sgb_data[2] << 8);
								Machine->remapped_colortable[1*4 + 1] = sgb_data[3] | (sgb_data[4] << 8);
								Machine->remapped_colortable[1*4 + 2] = sgb_data[5] | (sgb_data[6] << 8);
								Machine->remapped_colortable[1*4 + 3] = sgb_data[7] | (sgb_data[8] << 8);
								Machine->remapped_colortable[2*4 + 0] = sgb_data[1] | (sgb_data[2] << 8);
								Machine->remapped_colortable[2*4 + 1] = sgb_data[9] | (sgb_data[10] << 8);
								Machine->remapped_colortable[2*4 + 2] = sgb_data[11] | (sgb_data[12] << 8);
								Machine->remapped_colortable[2*4 + 3] = sgb_data[13] | (sgb_data[14] << 8);
								break;
							case 0x04:	/* ATTR_BLK */
								{
									UINT8 I, J, K, o;
									for( K = 0; K < sgb_data[1]; K++ )
									{
										o = K * 6;
										if( sgb_data[o + 2] & 0x1 )
										{
											for( I = sgb_data[ o + 4]; I <= sgb_data[o + 6]; I++ )
											{
												for( J = sgb_data[o + 5]; J <= sgb_data[o + 7]; J++ )
												{
													sgb_pal_map[I][J] = sgb_data[o + 3] & 0x3;
												}
											}
										}
									}
								}
								break;
							case 0x05:	/* ATTR_LIN */
								{
									UINT8 J, K;
									if( sgb_data[1] > 15 )
										sgb_data[1] = 15;
									for( K = 0; K < sgb_data[1]; K++ )
									{
										if( sgb_data[K + 1] & 0x80 )
										{
											for( J = 0; J < 20; J++ )
											{
												sgb_pal_map[J][sgb_data[K + 1] & 0x1f] = (sgb_data[K + 1] & 0x60) >> 5;
											}
										}
										else
										{
											for( J = 0; J < 18; J++ )
											{
												sgb_pal_map[sgb_data[K + 1] & 0x1f][J] = (sgb_data[K + 1] & 0x60) >> 5;
											}
										}
									}
								}
								break;
							case 0x06:	/* ATTR_DIV */
								{
									UINT8 I, J;
									if( sgb_data[1] & 0x40 ) /* Vertical */
									{
										for( I = 0; I < sgb_data[2]; I++ )
										{
											for( J = 0; J < 20; J++ )
											{
												sgb_pal_map[J][I] = (sgb_data[1] & 0xC) >> 2;
											}
										}
										for( J = 0; J < 20; J++ )
										{
											sgb_pal_map[J][sgb_data[2]] = (sgb_data[1] & 0x30) >> 4;
										}
										for( I = sgb_data[2] + 1; I < 18; I++ )
										{
											for( J = 0; J < 20; J++ )
											{
												sgb_pal_map[J][I] = sgb_data[1] & 0x3;
											}
										}
									}
									else /* Horizontal */
									{
										for( I = 0; I < sgb_data[2]; I++ )
										{
											for( J = 0; J < 18; J++ )
											{
												sgb_pal_map[I][J] = (sgb_data[1] & 0xC) >> 2;
											}
										}
										for( J = 0; J < 18; J++ )
										{
											sgb_pal_map[sgb_data[2]][J] = (sgb_data[1] & 0x30) >> 4;
										}
										for( I = sgb_data[2] + 1; I < 20; I++ )
										{
											for( J = 0; J < 18; J++ )
											{
												sgb_pal_map[I][J] = sgb_data[1] & 0x3;
											}
										}
									}
								}
								break;
							case 0x07:	/* ATTR_CHR */
								{
									UINT16 I, sets;
									UINT8 x, y;
									sets = (sgb_data[3] | (sgb_data[4] << 8) );
									if( sets > 360 )
										sets = 360;
									sets >>= 2;
									sets += 6;
									x = sgb_data[1];
									y = sgb_data[2];
									if( sgb_data[5] ) /* Vertical */
									{
										for( I = 6; I < sets; I++ )
										{
											sgb_pal_map[x][y++] = (sgb_data[I] & 0xC0) >> 6;
											if( y > 17 )
											{
												y = 0;
												x++;
												if( x > 19 )
													x = 0;
											}

											sgb_pal_map[x][y++] = (sgb_data[I] & 0x30) >> 4;
											if( y > 17 )
											{
												y = 0;
												x++;
												if( x > 19 )
													x = 0;
											}

											sgb_pal_map[x][y++] = (sgb_data[I] & 0xC) >> 2;
											if( y > 17 )
											{
												y = 0;
												x++;
												if( x > 19 )
													x = 0;
											}

											sgb_pal_map[x][y++] = sgb_data[I] & 0x3;
											if( y > 17 )
											{
												y = 0;
												x++;
												if( x > 19 )
													x = 0;
											}
										}
									}
									else /* horizontal */
									{
										for( I = 6; I < sets; I++ )
										{
											sgb_pal_map[x++][y] = (sgb_data[I] & 0xC0) >> 6;
											if( x > 19 )
											{
												x = 0;
												y++;
												if( y > 17 )
													y = 0;
											}

											sgb_pal_map[x++][y] = (sgb_data[I] & 0x30) >> 4;
											if( x > 19 )
											{
												x = 0;
												y++;
												if( y > 17 )
													y = 0;
											}

											sgb_pal_map[x++][y] = (sgb_data[I] & 0xC) >> 2;
											if( x > 19 )
											{
												x = 0;
												y++;
												if( y > 17 )
													y = 0;
											}

											sgb_pal_map[x++][y] = sgb_data[I] & 0x3;
											if( x > 19 )
											{
												x = 0;
												y++;
												if( y > 17 )
													y = 0;
											}
										}
									}
								}
								break;
							case 0x08:	/* SOUND */
								/* This command enables internal sound effects */
								/* Not Implemented */
								break;
							case 0x09:	/* SOU_TRN */
								/* This command sends data to the SNES sound processor.
								   We'll need to emulate that for this to be used */
								/* Not Implemented */
								break;
							case 0x0A:	/* PAL_SET */
								{
									UINT16 index_, J, I;

									/* Palette 0 */
									index_ = (UINT16)(sgb_data[1] | (sgb_data[2] << 8)) * 4;
									Machine->remapped_colortable[0] = sgb_pal_data[index_];
									Machine->remapped_colortable[1] = sgb_pal_data[index_ + 1];
									Machine->remapped_colortable[2] = sgb_pal_data[index_ + 2];
									Machine->remapped_colortable[3] = sgb_pal_data[index_ + 3];
									/* Palette 1 */
									index_ = (UINT16)(sgb_data[3] | (sgb_data[4] << 8)) * 4;
									Machine->remapped_colortable[4] = sgb_pal_data[index_];
									Machine->remapped_colortable[5] = sgb_pal_data[index_ + 1];
									Machine->remapped_colortable[6] = sgb_pal_data[index_ + 2];
									Machine->remapped_colortable[7] = sgb_pal_data[index_ + 3];
									/* Palette 2 */
									index_ = (UINT16)(sgb_data[5] | (sgb_data[6] << 8)) * 4;
									Machine->remapped_colortable[8] = sgb_pal_data[index_];
									Machine->remapped_colortable[9] = sgb_pal_data[index_ + 1];
									Machine->remapped_colortable[10] = sgb_pal_data[index_ + 2];
									Machine->remapped_colortable[11] = sgb_pal_data[index_ + 3];
									/* Palette 3 */
									index_ = (UINT16)(sgb_data[7] | (sgb_data[8] << 8)) * 4;
									Machine->remapped_colortable[12] = sgb_pal_data[index_];
									Machine->remapped_colortable[13] = sgb_pal_data[index_ + 1];
									Machine->remapped_colortable[14] = sgb_pal_data[index_ + 2];
									Machine->remapped_colortable[15] = sgb_pal_data[index_ + 3];
									/* Attribute File */
									if( sgb_data[9] & 0x40 )
										sgb_window_mask = 0;
									sgb_atf = (sgb_data[9] & 0x3f) * (18 * 5);
									if( sgb_data[9] & 0x80 )
									{
										for( J = 0; J < 18; J++ )
										{
											for( I = 0; I < 5; I++ )
											{
												sgb_pal_map[I * 4][J] = (sgb_atf_data[(J * 5) + sgb_atf + I] & 0xC0) >> 6;
												sgb_pal_map[(I * 4) + 1][J] = (sgb_atf_data[(J * 5) + sgb_atf + I] & 0x30) >> 4;
												sgb_pal_map[(I * 4) + 2][J] = (sgb_atf_data[(J * 5) + sgb_atf + I] & 0xC) >> 2;
												sgb_pal_map[(I * 4) + 3][J] = sgb_atf_data[(J * 5) + sgb_atf + I] & 0x3;
											}
										}
									}
								}
								break;
							case 0x0B:	/* PAL_TRN */
								{
									UINT16 I, col;

									for( I = 0; I < 2048; I++ )
									{
										col = program_read_byte_8 (0x8800 + (I*2));
										col |= (UINT16)(program_read_byte_8 (0x8800 + (I*2) + 1)) << 8;
										sgb_pal_data[I] = col;
									}
								}
								break;
							case 0x0C:	/* ATRC_EN */
								/* Not Implemented */
								break;
							case 0x0D:	/* TEST_EN */
								/* Not Implemented */
								break;
							case 0x0E:	/* ICON_EN */
								/* Not Implemented */
								break;
							case 0x0F:	/* DATA_SND */
								/* Not Implemented */
								break;
							case 0x10:	/* DATA_TRN */
								/* Not Implemented */
								break;
							case 0x11:	/* MLT_REQ - Multi controller request */
								if (sgb_data[1] == 0x00)
									sgb_controller_mode = 0;
								else if (sgb_data[1] == 0x01)
									sgb_controller_mode = 2;
								break;
							case 0x12:	/* JUMP */
								/* Not Implemented */
								break;
							case 0x13:	/* CHR_TRN */
								if( sgb_data[1] & 0x1 )
									memcpy( sgb_tile_data + 4096, gb_vram + 0x0800, 4096 );
								else
									memcpy( sgb_tile_data, gb_vram + 0x0800, 4096 );
								break;
							case 0x14:	/* PCT_TRN */
								{
									int I;
									UINT16 col;
									if( sgb_hack )
									{
										memcpy( sgb_tile_map, gb_vram + 0x1000, 2048 );
										for( I = 0; I < 64; I++ )
										{
											col = program_read_byte_8 (0x8800 + (I*2));
											col |= (UINT16)(program_read_byte_8 (0x8800 + (I*2) + 1)) << 8;
											Machine->remapped_colortable[SGB_BORDER_PAL_OFFSET + I] = col;
										}
									}
									else /* Do things normally */
									{
										memcpy( sgb_tile_map, gb_vram + 0x0800, 2048 );
										for( I = 0; I < 64; I++ )
										{
											col = program_read_byte_8 (0x9000 + (I*2));
											col |= (UINT16)(program_read_byte_8 (0x9000 + (I*2) + 1)) << 8;
											Machine->remapped_colortable[SGB_BORDER_PAL_OFFSET + I] = col;
										}
									}
								}
								break;
							case 0x15:	/* ATTR_TRN */
								memcpy( sgb_atf_data, gb_vram + 0x0800, 4050 );
								break;
							case 0x16:	/* ATTR_SET */
								{
									UINT8 J, I;

									/* Attribute File */
									if( sgb_data[1] & 0x40 )
										sgb_window_mask = 0;
									sgb_atf = (sgb_data[1] & 0x3f) * (18 * 5);
									for( J = 0; J < 18; J++ )
									{
										for( I = 0; I < 5; I++ )
										{
											sgb_pal_map[I * 4][J] = (sgb_atf_data[(J * 5) + sgb_atf + I] & 0xC0) >> 6;
											sgb_pal_map[(I * 4) + 1][J] = (sgb_atf_data[(J * 5) + sgb_atf + I] & 0x30) >> 4;
											sgb_pal_map[(I * 4) + 2][J] = (sgb_atf_data[(J * 5) + sgb_atf + I] & 0xC) >> 2;
											sgb_pal_map[(I * 4) + 3][J] = sgb_atf_data[(J * 5) + sgb_atf + I] & 0x3;
										}
									}
								}
								break;
							case 0x17:	/* MASK_EN */
								sgb_window_mask = sgb_data[1];
								break;
							case 0x18:	/* OBJ_TRN */
								/* Not Implemnted */
								break;
							case 0x19:	/* ? */
								/* Called by: dkl,dkl2,dkl3,zeldadx
								   But I don't know what it is for. */
								/* Not Implemented */
								break;
							default:
								logerror( "\tSGB: Unknown Command!\n" );
						}

						sgb_start = 0;
						sgb_bytecount = 0;
						sgb_packets = -1;
					}
					if( sgb_start )
					{
						sgb_data[sgb_bytecount] >>= 1;
						sgb_bitcount++;
						if (sgb_bitcount == 8)
						{
							sgb_bitcount = 0;
							sgb_bytecount++;
						}
					}
					sgb_rest = 0;
				}
				JOYPAD = 0x2F & (readinputport (0) | 0xF0);
				break;
			case 0x30:				   /* rest condition */
				if (sgb_start)
					sgb_rest = 1;
				if (sgb_controller_mode)
				{
					sgb_controller_no++;
					if (sgb_controller_no == sgb_controller_mode)
						sgb_controller_no = 0;
					JOYPAD = 0x3F - sgb_controller_no;
				}
				else
					JOYPAD = 0x3F;
				break;
			}
			return;
		default:
			/* we didn't handle the write, so pass it to the GB handler */
			gb_io_w( offset, data );
			return;
	}

	gb_io[offset] = data;
}

/* Interrupt Enable register */
READ8_HANDLER( gb_ie_r )
{
	return cpunum_get_reg( 0, Z80GB_IE );
}

WRITE8_HANDLER ( gb_ie_w )
{
	cpunum_set_reg( 0, Z80GB_IE, data & 0x1F );
}

/* IO read */
READ8_HANDLER ( gb_io_r )
{
	switch(offset)
	{
		case 0x04:
			return ( gb_divcount >> 8 ) & 0xFF;
		case 0x00:
		case 0x01:
		case 0x02:
		case 0x03:
		case 0x05:
		case 0x06:
		case 0x07:
			return gb_io[offset];
		case 0x0F:
			return cpunum_get_reg( 0, Z80GB_IF );
		default:
			/* It seems unsupported registers return 0xFF */
			return 0xFF;
	}
}

WRITE8_HANDLER( gb_oam_w ) {
	if ( gb_video_oam_locked() || offset >= 0xa0 ) { 
                return;
        }
        gb_oam[offset] = data;
}

WRITE8_HANDLER( gb_vram_w ) {
	if ( gb_video_vram_locked() ) {
                return;
        }
        gb_vram[offset] = data;
}

WRITE8_HANDLER( gbc_vram_w ) {
	if ( gb_video_vram_locked() ) {
		return;
	}
	gbc_vram_bank[offset] = data;
}

DEVICE_INIT(gb_cart)
{
	int I;

	if ( gb_dummy_rom_bank == NULL ) {
		gb_dummy_rom_bank = auto_malloc( 0x4000 );
		memset( gb_dummy_rom_bank, 0xFF, 0x4000 );
	}

	if ( gb_dummy_ram_bank == NULL ) {
		gb_dummy_ram_bank = auto_malloc( 0x2000 );
		memset( gb_dummy_ram_bank, 0x00, 0x2000 );
	}

	for(I = 0; I < MAX_ROMBANK; I++) {
		ROMMap[I] = gb_dummy_rom_bank;
	}
	for(I = 0; I < MAX_RAMBANK; I++) {
		RAMMap[I] = gb_dummy_ram_bank;
	}
	ROMBank00 = 0;
	ROMBanks = 0;
	RAMBanks = 0;
	MBCType = MBC_NONE;
	CartType = 0;
	ROMMask = 0;
	RAMMask = 0;
	return INIT_PASS;
}

DEVICE_LOAD(gb_cart)
{
	static const char *CartTypes[] =
	{
		"ROM ONLY",
		"ROM+MBC1",
		"ROM+MBC1+RAM",
		"ROM+MBC1+RAM+BATTERY",
        "UNKNOWN",
		"ROM+MBC2",
		"ROM+MBC2+BATTERY",
        "UNKNOWN",
		"ROM+RAM",
		"ROM+RAM+BATTERY",
        "UNKNOWN",
		"ROM+MMM01",
		"ROM+MMM01+SRAM",
		"ROM+MMM01+SRAM+BATTERY",
        "UNKNOWN",
		"ROM+MBC3+TIMER+BATTERY",
		"ROM+MBC3+TIMER+RAM+BATTERY",
		"ROM+MBC3",
		"ROM+MBC3+RAM",
		"ROM+MBC3+RAM+BATTERY",
        "UNKNOWN",
        "UNKNOWN",
        "UNKNOWN",
        "UNKNOWN",
        "UNKNOWN",
		"ROM+MBC5",
		"ROM+MBC5+RAM",
		"ROM+MBC5+RAM+BATTERY",
		"ROM+MBC5+RUMBLE",
		"ROM+MBC5+RUMBLE+SRAM",
		"ROM+MBC5+RUMBLE+SRAM+BATTERY",
		"Pocket Camera",
		"Bandai TAMA5",
		/* Need heaps of unknowns here */
		"Hudson HuC-3",
		"Hudson HuC-1"
	};

/*** Following are some known manufacturer codes *************************/
	static struct
	{
		UINT16 Code;
		const char *Name;
	}
	Companies[] =
	{
		{0x3301, "Nintendo"},
		{0x7901, "Accolade"},
		{0xA400, "Konami"},
		{0x6701, "Ocean"},
		{0x5601, "LJN"},
		{0x9900, "ARC?"},
		{0x0101, "Nintendo"},
		{0x0801, "Capcom"},
		{0x0100, "Nintendo"},
		{0xBB01, "SunSoft"},
		{0xA401, "Konami"},
		{0xAF01, "Namcot?"},
		{0x4901, "Irem"},
		{0x9C01, "Imagineer"},
		{0xA600, "Kawada?"},
		{0xB101, "Nexoft"},
		{0x5101, "Acclaim"},
		{0x6001, "Titus"},
		{0xB601, "HAL"},
		{0x3300, "Nintendo"},
		{0x0B00, "Coconuts?"},
		{0x5401, "Gametek"},
		{0x7F01, "Kemco?"},
		{0xC001, "Taito"},
		{0xEB01, "Atlus"},
		{0xE800, "Asmik?"},
		{0xDA00, "Tomy?"},
		{0xB100, "ASCII?"},
		{0xEB00, "Atlus"},
		{0xC000, "Taito"},
		{0x9C00, "Imagineer"},
		{0xC201, "Kemco?"},
		{0xD101, "Sofel?"},
		{0x6101, "Virgin"},
		{0xBB00, "SunSoft"},
		{0xCE01, "FCI?"},
		{0xB400, "Enix?"},
		{0xBD01, "Imagesoft"},
		{0x0A01, "Jaleco?"},
		{0xDF00, "Altron?"},
		{0xA700, "Takara?"},
		{0xEE00, "IGS?"},
		{0x8300, "Lozc?"},
		{0x5001, "Absolute?"},
		{0xDD00, "NCS?"},
		{0xE500, "Epoch?"},
		{0xCB00, "VAP?"},
		{0x8C00, "Vic Tokai"},
		{0xC200, "Kemco?"},
		{0xBF00, "Sammy?"},
		{0x1800, "Hudson Soft"},
		{0xCA01, "Palcom/Ultra"},
		{0xCA00, "Palcom/Ultra"},
		{0xC500, "Data East?"},
		{0xA900, "Technos Japan?"},
		{0xD900, "Banpresto?"},
		{0x7201, "Broderbund?"},
		{0x7A01, "Triffix Entertainment?"},
		{0xE100, "Towachiki?"},
		{0x9300, "Tsuburava?"},
		{0xC600, "Tonkin House?"},
		{0xCE00, "Pony Canyon"},
		{0x7001, "Infogrames?"},
		{0x8B01, "Bullet-Proof Software?"},
		{0x5501, "Park Place?"},
		{0xEA00, "King Records?"},
		{0x5D01, "Tradewest?"},
		{0x6F01, "ElectroBrain?"},
		{0xAA01, "Broderbund?"},
		{0xC301, "SquareSoft"},
		{0x5201, "Activision?"},
		{0x5A01, "Bitmap Brothers/Mindscape"},
		{0x5301, "American Sammy"},
		{0x4701, "Spectrum Holobyte"},
		{0x1801, "Hudson Soft"},
		{0x0000, NULL}
	};

	int Checksum, I, J, filesize;
	UINT16 reported_rom_banks;
	UINT8	*gb_header;
	int rambanks[8] = {0, 1, 1, 4, 16, 8, 0, 0};

	filesize = image_length(image);

	/* Check for presence of a header, and skip that header */
	J = filesize % 0x4000;
	if ( J == 512 ) {
		logerror( "Rom-header found, skipping\n" );
		image_fseek( image, J, SEEK_SET );
		filesize -= 512;
	}

	/* Verify that the file contains 16kb blocks */
	if ( ( filesize == 0 ) || ( ( filesize % 0x4000 ) != 0 ) ) {
		image_seterror( image, IMAGE_ERROR_UNSPECIFIED, "Invalid rom file size" );
		return INIT_FAIL;
	}

	/* Claim memory */
	gb_cart = auto_malloc( filesize );

	/* Read cartridge */
	if ( image_fread( image, gb_cart, filesize ) != filesize ) {
		image_seterror( image, IMAGE_ERROR_UNSPECIFIED, "Unable to fully read from file" );
		return INIT_FAIL;
	}

	gb_header = gb_cart;
	ROMBank00 = 0;

	/* Check for presence of MMM01 mapper */
	if ( filesize >= 0x8000 ) {
		static const UINT8 nintendo_logo[0x18] = {
			0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B,
			0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D,
			0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E
		};
		int	bytes_matched = 0;
		gb_header = gb_cart + filesize - 0x8000;
		for ( I = 0; I < 0x18; I++ ) {
			if ( gb_header[0x0104 + I] == nintendo_logo[I] ) {
				bytes_matched++;
			}
		}
		if ( bytes_matched == 0x18 && gb_header[0x0147] >= 0x0B && gb_header[0x0147] <= 0x0D ) {
			ROMBank00 = ( filesize / 0x4000 ) - 2;
			mmm01_bank_offset = ROMBank00;
		} else {
			gb_header = gb_cart;
		}
	}

	/* Fill in our cart details */
	switch( gb_header[0x0147] ) {
	case 0x00:	MBCType = MBC_NONE;	CartType = 0;				break;
	case 0x01:	MBCType = MBC_MBC1;	CartType = 0;				break;
	case 0x02:	MBCType = MBC_MBC1;	CartType = RAM;				break;
	case 0x03:	MBCType = MBC_MBC1;	CartType = RAM | BATTERY;		break;
	case 0x05:	MBCType = MBC_MBC2;	CartType = 0;				break;
	case 0x06:	MBCType = MBC_MBC2;	CartType = BATTERY;			break;
	case 0x08:	MBCType = MBC_NONE;	CartType = RAM;				break;
	case 0x09:	MBCType = MBC_NONE;	CartType = RAM | BATTERY;		break;
	case 0x0B:	MBCType = MBC_MMM01;	CartType = 0;				break;
	case 0x0C:	MBCType = MBC_MMM01;	CartType = RAM;				break;
	case 0x0D:	MBCType = MBC_MMM01;	CartType = RAM | BATTERY;		break;
	case 0x0F:	MBCType = MBC_MBC3;	CartType = TIMER | BATTERY;		break;
	case 0x10:	MBCType = MBC_MBC3;	CartType = TIMER | RAM | BATTERY;	break;
	case 0x11:	MBCType = MBC_MBC3;	CartType = 0;				break;
	case 0x12:	MBCType = MBC_MBC3;	CartType = RAM;				break;
	case 0x13:	MBCType = MBC_MBC3;	CartType = RAM | BATTERY;		break;
	case 0x15:	MBCType = MBC_MBC4;	CartType = 0;				break;
	case 0x16:	MBCType = MBC_MBC4;	CartType = RAM;				break;
	case 0x17:	MBCType = MBC_MBC4;	CartType = RAM | BATTERY;		break;
	case 0x19:	MBCType = MBC_MBC5;	CartType = 0;				break;
	case 0x1A:	MBCType = MBC_MBC5;	CartType = RAM;				break;
	case 0x1B:	MBCType = MBC_MBC5;	CartType = RAM | BATTERY;		break;
	case 0x1C:	MBCType = MBC_MBC5;	CartType = RUMBLE;			break;
	case 0x1D:	MBCType = MBC_MBC5;	CartType = RUMBLE | SRAM;		break;
	case 0x1E:	MBCType = MBC_MBC5;	CartType = RUMBLE | SRAM | BATTERY;	break;
	case 0x22:	MBCType = MBC_MBC7;	CartType = SRAM | BATTERY;		break;
	case 0xBE:	MBCType = MBC_NONE;	CartType = 0;				break;	/* used in Flash2Advance GB Bridge boot program */
	case 0xFD:	MBCType = MBC_TAMA5;	CartType = 0 /*RTC | BATTERY?*/;	break;
	case 0xFE:	MBCType = MBC_HUC3;	CartType = 0;				break;
	case 0xFF:	MBCType = MBC_HUC1;	CartType = 0;				break;
	default:	MBCType = MBC_UNKNOWN;	CartType = UNKNOWN;			break;
	}

	/* Check whether we're dealing with a (possible) Wisdom Tree game here */
	if ( gb_header[0x0147] == 0x00 ) {
		int count = 0;
		for( I = 0x0134; I <= 0x014C; I++ ) {
			count += gb_header[I];
		}
		if ( count == 0 ) {
			MBCType = MBC_WISDOM;
		}
	}

	if ( MBCType == MBC_UNKNOWN ) {
		image_seterror( image, IMAGE_ERROR_UNSUPPORTED, "Unknown mapper type" );
		return INIT_FAIL;
	}
	if ( MBCType == MBC_MMM01 ) {
//		image_seterror( image, IMAGE_ERROR_UNSUPPORTED, "Mapper MMM01 is not supported yet" );
//		return INIT_FAIL;
	}
	if ( MBCType == MBC_MBC4 ) {
		image_seterror( image, IMAGE_ERROR_UNSUPPORTED, "Mapper MBC4 is not supported yet" );
		return INIT_FAIL;
	}
	/* MBC7 support is still work-in-progress, so only enable it for debug builds */
#ifndef MAME_DEBUG
	if ( MBCType == MBC_MBC7 ) {
		image_seterror( image, IMAGE_ERROR_UNSUPPORTED, "Mapper MBC7 is not supported yet" );
		return INIT_FAIL;
	}
#endif

	ROMBanks = filesize / 0x4000;
	switch( gb_header[0x0148] ) {
	case 0x52:
		reported_rom_banks = 72;
		break;
	case 0x53:
		reported_rom_banks = 80;
		break;
	case 0x54:
		reported_rom_banks = 96;
		break;
	case 0x00: case 0x01: case 0x02: case 0x03:
	case 0x04: case 0x05: case 0x06: case 0x07:
		reported_rom_banks = 2 << gb_header[0x0148];
		break;
	default:
		logerror( "Warning loading cartridge: Unknown ROM size in header.\n" );
		reported_rom_banks = 256;
		break;
	}
	if ( ROMBanks != reported_rom_banks && MBCType != MBC_WISDOM ) {
		logerror( "Warning loading cartridge: Filesize and reported ROM banks don't match.\n" );
	}

        RAMBanks = rambanks[gb_header[0x0149] & 7];

	/* Calculate and check checksum */
        Checksum = ((UINT16) gb_header[0x014E] << 8) + gb_header[0x014F];
        Checksum += gb_header[0x014E] + gb_header[0x014F];
        for (I = 0; I < filesize; I++)
        {
                Checksum -= gb_cart[I];
        }
        if (Checksum & 0xFFFF)
        {
                logerror("Warning loading cartridge: Checksum is wrong.");
        }

	/* Initialize ROMMap pointers */
        for (I = 0; I < ROMBanks; I++)
        {
                ROMMap[I] = gb_cart + ( I * 0x4000 );
        }

	/*
	  Handle odd-sized cartridges (72,80,96 banks)
	  ROMBanks      ROMMask
	  72 (1001000)  1000111 (71)
	  80 (1010000)  1001111 (79)
	  96 (1100000)  1011111 (95)
	*/
	ROMMask = I - 1;
	if ( ( ROMBanks & ROMMask ) != 0 ) {
		for( ; I & ROMBanks; I++ ) {
			ROMMap[ I ] = ROMMap[ I & ROMMask ];
		}
		ROMMask = I - 1;
	}

	/* Fill out the remaining rom bank pointers, if any. */
	for ( ; I < MAX_ROMBANK; I++ ) {
		ROMMap[ I ] = ROMMap[ I & ROMMask ];
	}

	/* Log cart information */
	{
		const char *P;
		char S[50];
		int ramsize[8] = { 0, 2, 8, 32, 128, 64, 0, 0 };


		strncpy (S, (char *)&gb_header[0x0134], 16);
		S[16] = '\0';
		logerror("Cart Information\n");
		logerror("\tName:             %s\n", S);
		logerror("\tType:             %s [0x%2X]\n", CartTypes[gb_header[0x0147]], gb_header[0x0147] );
		logerror("\tGameBoy:          %s\n", (gb_header[0x0143] == 0xc0) ? "No" : "Yes" );
		logerror("\tSuper GB:         %s [0x%2X]\n", (gb_header[0x0146] == 0x03) ? "Yes" : "No", gb_header[0x0146] );
		logerror("\tColor GB:         %s [0x%2X]\n", (gb_header[0x0143] == 0x80 || gb_header[0x0143] == 0xc0) ? "Yes" : "No", gb_cart[0x0143] );
		logerror("\tROM Size:         %d 16kB Banks [0x%2X]\n", ROMBanks, gb_header[0x0148]);
		logerror("\tRAM Size:         %d kB [0x%2X]\n", ramsize[ gb_header[0x0149] & 0x07 ], gb_header[0x0149]);
		logerror("\tLicense code:     0x%2X%2X\n", gb_header[0x0145], gb_header[0x0144] );
		J = ((UINT16) gb_header[0x014B] << 8) + gb_header[0x014A];
		for (I = 0, P = NULL; !P && Companies[I].Name; I++)
			if (J == Companies[I].Code)
				P = Companies[I].Name;
		logerror("\tManufacturer ID:  0x%2X", J);
		logerror(" [%s]\n", P ? P : "?");
		logerror("\tVersion Number:   0x%2X\n", gb_header[0x014C]);
		logerror("\tComplement Check: 0x%2X\n", gb_header[0x014D]);
		logerror("\tChecksum:         0x%2X\n", ( ( gb_header[0x014E] << 8 ) + gb_header[0x014F] ) );
		J = ((UINT16) gb_header[0x0103] << 8) + gb_header[0x0102];
		logerror("\tStart Address:    0x%2X\n", J);
	}

	/* MBC2 has 512 * 4bits (8kb) internal RAM */
	if( MBCType == MBC_MBC2 )
		RAMBanks = 1;
	/* MBC7 has 512 bytes(?) of internal RAM */
	if ( MBCType == MBC_MBC7 ) {
		RAMBanks = 1;
	}

	if (RAMBanks && MBCType)
	{
		/* Release any previously claimed memory */
		if ( gb_cart_ram != NULL ) {
			free( gb_cart_ram );
			gb_cart_ram = NULL;
		}
		/* Claim memory */
		gb_cart_ram = auto_malloc( RAMBanks * 0x2000 );

		for (I = 0; I < RAMBanks; I++)
		{
			RAMMap[I] = gb_cart_ram + ( I * 0x2000 );
		}

		/* Set up rest of the (mirrored) RAM pages */
		RAMMask = I - 1;
		for ( ; I < MAX_RAMBANK; I++ ) {
			RAMMap[ I ] = RAMMap [ I & RAMMask ];
		}
	} else {
		RAMMask = 0;
	}

	/* If there's an RTC claim memory to store the RTC contents */
	if ( CartType & TIMER ) {
		MBC3RTCData = auto_malloc( 0x2000 );
	}

	if ( MBCType == MBC_TAMA5 ) {
		MBC3RTCData = auto_malloc( 0x2000 );
		memset( gbTama5Memory, 0xFF, sizeof(gbTama5Memory) );
	}

	/* Load the saved RAM if this cart has a battery */
	if( CartType & BATTERY && RAMBanks )
	{
		image_battery_load( image, gb_cart_ram, RAMBanks * 0x2000 );
	}

	return INIT_PASS;
}

void gb_scanline_interrupt (void) {
}

void gb_serial_timer_proc( int dummy ) {
	/* Shift in a received bit */
	SIODATA = (SIODATA << 1) | 0x01;
	/* Decrement number of handled bits */
	SIOCount--;
	/* If all bits done, stop timer and trigger interrupt */
	if ( ! SIOCount ) {
		SIOCONT &= 0x7F;
		mame_timer_enable( gb_serial_timer, 0 );
		cpunum_set_input_line(0, SIO_INT, HOLD_LINE);
	}
}

void gb_timer_callback(int cycles) {
	UINT16 old_gb_divcount = gb_divcount;
	gb_divcount += cycles;
	if ( TIMEFRQ & 0x04 ) {
		UINT16 old_count = old_gb_divcount >> gb_timer_shift;
		UINT16 new_count = gb_divcount >> gb_timer_shift;
		if ( new_count != old_count ) {
			/* this increment is probably not correct when cycles > 1 timer cycle */
			TIMECNT += 1;
			if ( TIMECNT == 0x00 ) {
				TIMECNT = TIMEMOD;
				cpunum_set_input_line( 0, TIM_INT, HOLD_LINE );
			}
		}
	}
}

WRITE8_HANDLER ( gbc_io2_w )
{
	switch( offset ) {
		case 0x0D:	/* KEY1 - Prepare speed switch */
			cpunum_set_reg( 0, Z80GB_SPEED, data );
			return;
		case 0x0F:	/* VBK - VRAM bank select */
			gbc_vram_bank = GBC_VRAMMap[ data & 0x01 ];
			memory_set_bankptr( 4, gbc_vram_bank );
			data |= 0xFE;
			break;
		case 0x16:	/* RP - Infrared port */
			break;
		case 0x30:	/* SVBK - RAM bank select */
			GBC_RAMBank = data & 0x7;
			memory_set_bankptr (3, GBC_RAMMap[GBC_RAMBank]);
			break;
		default:
			break;
	}
	gbc_video_w( offset, data );
}

READ8_HANDLER( gbc_io2_r ) {
	switch( offset ) {
	case 0x0D:	/* KEY1 */
		return cpunum_get_reg( 0, Z80GB_SPEED );
	case 0x16:	/* RP - Infrared port */
		break;
	default:
		break;
	}
	return gb_video_r( offset );
}

/****************************************************************************

  Megaduck routines

 ****************************************************************************/

MACHINE_RESET( megaduck )
{
	/* We may have to add some more stuff here, if not then it can be merged back into gb */
	gb_init();

	gb_video_init();
}

/*
 Map megaduck video related area on to regular Gameboy video area

 Different locations of the video registers:
 Register      GameBoy    MegaDuck
 LCDC          FF40       FF10  (See different bit order below)
 STAT          FF41       FF11
 SCY           FF42       FF12
 SCX           FF43       FF13
 LY            FF44       FF18
 LYC           FF45       FF19
 DMA           FF46       FF1A
 BGP           FF47       FF1B
 OBP0          FF48       FF14
 OBP1          FF49       FF15
 WY            FF4A       FF16
 WX            FF4B       FF17
 Unused        FF4C       FF4C (?)
 Unused        FF4D       FF4D (?)
 Unused        FF4E       FF4E (?)
 Unused        FF4F       FF4F (?)

 Different LCDC register

 GameBoy        MegaDuck
 0                      6       - BG & Window Display : 0 - Off, 1 - On
 1                      0       - OBJ Display: 0 - Off, 1 - On
 2                      1       - OBJ Size: 0 - 8x8, 1 - 8x16
 3                      2       - BG Tile Map Display: 0 - 9800, 1 - 9C00
 4                      4       - BG & Window Tile Data Select: 0 - 8800, 1 - 8000
 5                      5       - Window Display: 0 - Off, 1 - On
 6                      3       - Window Tile Map Display Select: 0 - 9800, 1 - 9C00
 7                      7       - LCD Operation

 **************/

 READ8_HANDLER( megaduck_video_r )
{
	UINT8 data;

	if ( (offset & 0x0C) && ((offset & 0x0C) ^ 0x0C) ) {
		offset ^= 0x0C;
	}
	data = gb_video_r( offset );
	if ( offset )
		return data;
	return BITSWAP8(data,7,0,5,4,6,3,2,1);
}

WRITE8_HANDLER ( megaduck_video_w )
{
	if ( !offset ) {
		data = BITSWAP8(data,7,3,5,4,2,1,0,6);
	}
	if ( (offset & 0x0C) && ((offset & 0x0C) ^ 0x0C) ) {
		offset ^= 0x0C;
	}
	gb_video_w(offset, data );
}

/* Map megaduck audio offset to gameboy audio offsets */

static UINT8 megaduck_sound_offsets[16] = { 0, 2, 1, 3, 4, 6, 5, 7, 8, 9, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F };

WRITE8_HANDLER( megaduck_sound_w1 )
{
	gb_sound_w( megaduck_sound_offsets[offset], data );
}

 READ8_HANDLER( megaduck_sound_r1 )
{
	return gb_sound_r( megaduck_sound_offsets[offset] );
}

WRITE8_HANDLER( megaduck_sound_w2 )
{
	switch(offset) {
		case 0x00:	gb_sound_w( 0x10, data );	break;
		case 0x01:	gb_sound_w( 0x12, data );	break;
		case 0x02:	gb_sound_w( 0x11, data );	break;
		case 0x03:	gb_sound_w( 0x13, data );	break;
		case 0x04:	gb_sound_w( 0x14, data );	break;
		case 0x05:	gb_sound_w( 0x16, data );	break;
		case 0x06:	gb_sound_w( 0x15, data );	break;
		case 0x07:
		case 0x08:
		case 0x09:
		case 0x0A:
		case 0x0B:
		case 0x0C:
		case 0x0D:
		case 0x0E:
		case 0x0F:
			break;
	}
}

READ8_HANDLER( megaduck_sound_r2 )
{
	return gb_sound_r(0x10 + megaduck_sound_offsets[offset]);
}

WRITE8_HANDLER( megaduck_rom_bank_select_type1 )
{
	if( ROMMask )
	{
		ROMBank = data & ROMMask;

		/* Switch banks */
		memory_set_bankptr (1, ROMMap[ROMBank]);
	}
}

WRITE8_HANDLER( megaduck_rom_bank_select_type2 )
{
	if( ROMMask )
	{
		ROMBank = (data << 1) & ROMMask;

		/* Switch banks */
		memory_set_bankptr( 10, ROMMap[ROMBank]);
		memory_set_bankptr( 1, ROMMap[ROMBank + 1]);
	}
}

DEVICE_LOAD(megaduck_cart)
{
	int I, filesize;

	for (I = 0; I < MAX_ROMBANK; I++)
		ROMMap[I] = NULL;
	for (I = 0; I < MAX_RAMBANK; I++)
		RAMMap[I] = NULL;

	filesize = image_length(image);
	ROMBanks = filesize / 0x4000;

	if ( ( filesize == 0 ) || ( ( filesize % 0x4000 ) != 0 ) ) {
		image_seterror( image, IMAGE_ERROR_UNSPECIFIED, "Invalid rom file size" );
		return INIT_FAIL;
	}

	/* Claim memory */
	gb_cart = auto_malloc( filesize );

	/* Read cartridge */
	if (image_fread (image, gb_cart, filesize) != filesize) {
		image_seterror( image, IMAGE_ERROR_UNSPECIFIED, "Unable to fully read from file" );
		return INIT_FAIL;
	}

	/* Log cart information */
	{
		logerror("Cart Information\n");
		logerror("\tRom Banks:        %d\n", ROMBanks);
	}

	for (I = 0; I < ROMBanks; I++)
	{
		ROMMap[I] = gb_cart + ( I * 0x4000 );
	}

	/* Build rom bank Mask */
	if (ROMBanks < 3)
		ROMMask = 0;
	else
	{
		for (I = 1; I < ROMBanks; I <<= 1) ;
		ROMMask = I - 1;
	}

	MBCType = MBC_MEGADUCK;

	return INIT_PASS;
}

/***************************************************************************

  Konami System 573
  ===========================================================
  Driver by R. Belmont & smf

  NOTE: The first time you run each game, it will go through a special initialization
  procedure.  This can be quite lengthy (in the case of Dark Horse Legend).  Let it
  complete all the way before exiting MAME and you will not have to do it again!

  NOTE 2: The first time you run Konami 80's Gallery, it will dump you on a clock
  setting screen.  Press DOWN to select "SAVE AND EXIT" then press player 1 START
  to continue.

  TODO:
  * fix root counters in machine/psx.c so the hack here (actually MAME 0.89's machine/psx.c code)
    can be removed
  * integrate ATAPI code with Aaron's ATA/IDE code

  -----------------------------------------------------------------------------------------

  System 573 Hardware Overview
  Konami, 1998-2001

  This system uses Konami PSX-based hardware with an ATAPI CDROM drive.
  There is a slot for a security cart (cart is installed in CN14) and also a PCMCIA card slot,
  which is unused. The main board and CDROM drive are housed in a black metal box.
  The games can be swapped by exchanging the CDROM disc and the security cart, whereby the main-board
  FlashROMs are re-programmed after a small wait. On subsequent power-ups, there is a check to test if the
  contents of the FlashROMs matches the CDROM, then the game boots up immediately.

  The games that run on this system include...

  Game                                  Year    Hardware Code     CD Code
  --------------------------------------------------------------------------
  *Anime Champ                          2000
  Bass Angler                           1998    GE765 JA          765 JA A02
  *Bass Angler 2                        1999
  Dark Horse Legend                     1998    GX706 JA          706 JA A02
  *Dark Horse 2                         ?
  *Fighting Mania                       2000
  Fisherman's Bait                      1998    GE765 UA          765 UA B02
  Fisherman's Bait 2                    1998    GC865 UA          865 UA B02
  Fisherman's Bait Marlin Challenge     1999
  *Gun Mania                            2001
  *Gun Mania Zone Plus                  2001
  *Gachaga Champ                        1999
  *Hyper Bishi Bashi                    1999
  *Hyper Bishi Bashi Champ              2000
  Jikkyo Pawafuru Pro Yakyu             1998    GX802 JA          802 JA B02
  *Kick & Kick                          2001
  Konami 80's Arcade Gallery            1998    GC826 JA          826 JA A01
  Konami 80's AC Special                1998    GC826 UA          826 UA A01
  Salary Man Champ                      2001
  *Step Champ                           1999

  Note:
       Not all games listed above are confirmed to run on System 573.
       * - denotes not dumped yet. If you can help with the remaining undumped System 573 games,
       please contact http://www.mameworld.net/gurudumps/comments.html


  Main PCB Layout
  ---------------
                                                     External controls port
  GX700-PWB(A)B                                               ||
  (C)1997 KONAMI CO. LTD.                                     \/
  |-----------------------------------------------------==============-------|
  |   CN15            CNA                     CN10                           |
  |        CN16                                                              |
  |                                                 |------------------------|
  | PQ30RV21                                        |                        |
  |                         |-------|               |                        |
  |             KM416V256   |SONY   |               |     PCMCIA SLOT        |
  |                         |CXD2925|               |                        |
  |                         |-------|               |                        |
  |                                                 |                        |
  |                                                 |------------------------|
  | |-----|                                        CN21                      |
  | |32M  |  |---------|     |---------|                                     |
  | |-----|  |SONY     |     |SONY     |                                     |
  |          |CXD8561Q |     |CXD8530CQ|           29F016   29F016   |--|    |
  | |-----|  |         |     |         |                             |  |    |
  | |32M  |  |         |     |         |                             |  |    |
  | |-----|  |---------|     |---------|           29F016   29F016   |  |    |
  |      53.693175MHz    67.7376MHz                                  |  |    |
  |                                     |-----|                      |  |CN14|
  |      KM48V514      KM48V514         |9536 |    29F016   29F016   |  |    |
  |            KM48V514       KM48V514  |     |                      |  |    |
  |      KM48V514      KM48V514         |-----|                      |  |    |
  |            KM48V514      KM48V514              29F016   29F016   |--|    |
  | MC44200FT                          M48T58Y-70PC1                         |
  |                                                                      CN12|
  |                                    700A01.22                             |
  |                             14.7456MHz                                   |
  |                  |-------|                                               |
  |                  |KONAMI |    |----|                               LA4705|
  |   058232         |056879 |    |3644|                            SM5877   |
  |                  |       |    |----|         ADC0834                LM358|
  |                  |-------|            ADM485           CN4               |
  |                                                         CN3      CN17    |
  |                                TEST_SW  DIP4 USB   CN8     RCA-L/R   CN9 |
  |--|          JAMMA            |-------------------------------------------|
     |---------------------------|
  Notes:
  CNA       - 40-pin IDE cable connector
  CN3       - 10-pin connector labelled 'ANALOG', connected to a 9-pin DSUB connector mounted in the
              front face of the housing, labelled 'OPTION1'
  CN4       - 12-pin connector labelled 'EXT-OUT'
  CN5       - 10-pin connector labelled 'EXT-IN', connected to a 9-pin DSUB connector mounted in the
              front face of the housing, labelled 'OPTION2'
  CN8       - 15-pin DSUB plug labelled 'VGA-DSUB15' extending from the front face of the housing
              labelled 'RGB'. Use of this connector is optional because the video is output via the
              standard JAMMA connector
  CN9       - 4-pin connector for amplified stereo sound output to 2 speakers
  CN10      - Custom 80-pin connector (for mounting an additional plug-in board for extra controls,
              possibly with CN21 also)
  CN12      - 4-pin CD-DA input connector (for Red-Book audio from CDROM drive to main board)
  CN14      - 44-pin security cartridge connector. The cartridge only contains a small PCB labelled
              'GX700-PWB(D) (C)1997 KONAMI' and has locations for 2 ICs only
              IC1 - Small SOIC8 chip, identified as a XICOR X76F041 security supervisor containing 4X
              128 x8 secureFLASH arrays, stamped '0038323 E9750'
              IC2 - Solder pads for mounting of a PLCC68 or QFP68 packaged IC (not populated)
  CN15      - 4-pin CDROM power connector
  CN16      - 2-pin fan connector
  CN17      - 6-pin power connector, connected to an 8-pin power plug mounted in the front face
              of the housing. This can be left unused because the JAMMA connector supplies all power
              requirements to the PCB
  CN21      - Custom 30-pin connector (purpose unknown, but probably for mounting an additional
              plug-in board with CN10 also)
  TEST_SW   - Push-button test switch
  DIP4      - 4-position DIP switch
  USB       - USB connector extended from the front face of the housing labelled 'I/O'
  RCA-L/R   - RCA connectors for left/right audio output
  PQ30RV21  - Sharp PQ30RV21 low-power voltage regulator (5 Volt to 3 Volt)
  LA4705    - Sanyo LA4705 15W 2-channel power amplifier (SIP18)
  LM358     - National Semiconductor LM358 low power dual operational amplifier (SOIC8, @ 33C)
  CXD2925Q  - Sony CXD2925Q SPU (QFP100, @ 15Q)
  CXD8561Q  - Sony CXD8561Q GTE (QFP208, @ 10M)
  CXD8530CQ - Sony CXD8530CQ R3000-based CPU (QFP208, @ 17M)
  9536      - Xilinx XC9536 in-system-programmable CPLD (PLCC44, @ 22J)
  3644      - Hitachi H8/3644 HD6473644H microcontroller with 32k ROM & 1k RAM (QFP64, @ 18E,
              labelled '700 02 38920')
  056879    - Konami 056879 custom IC (QFP120, @ 13E)
  MC44200FT - Motorola MC44200FT Triple 8-bit Video DAC (QFP44)
  058232    - Konami 058232 custom ceramic IC (SIP14, @ 6C)
  SM5877    - Nippon Precision Circuits SM5877 2-channel D/A convertor (SSOP24, @32D)
  ADM485    - Analog Devices ADM485 low power EIA RS-485 transceiver (SOIC8, @ 20C)
  ADC0834   - National Semiconductor ADC0834 8-Bit Serial I/O A/D Converter with Multiplexer
              Option (SOIC14, @ 24D)
  M48T58Y-70- STMicroelectronics M48T58Y-70PC1 8k x8 Timekeeper RAM (DIP32, @ 22H)
              Note that this is not used for protection. If you put in a new blank Timekeeper RAM
              it will be programmed with some data on power-up. If you swap games, the Timekeeper
              is updated with the new game data
  29F016      Fujitsu 29F016A-90PFTN 2M x8 FlashROM (TSOP48, @ 27H/J/L/M & 31H/J/L/M)
              Also found Sharp LH28F016S (2M x8 TSOP40) in some units
  KM416V256 - Samsung Electronics KM416V256BT-7 256k x 16 DRAM (TSOP44/40, @ 11Q)
  KM48V514  - Samsung Electronics KM48V514BJ-6 512k x 8 EDO DRAM (SOJ28, @ 16G/H, 14G/H, 12G/H, 9G/H)
  32M       - NEC D481850GF-A12 128k x 32Bit x 2 Banks SGRAM (QFP100, @ 4P & 4L)

  Software  -
              - 700A01.22G 4M MaskROM (DIP32, @ 22G)
              - SONY ATAPI CDROM drive, with CDROM disc containing program + graphics + sound
                Some System 573 units contain a CR-583 drive dated October 1997, some contain a
                CR-587 drive dated March 1998. Note that the CR-587 will not read CDR discs ;-)


  Auxillary Controls PCB
  ----------------------

  GE765-PWB(B)A (C)1998 KONAMI CO. LTD.
  |-----------------------------|
  |          CN33     C2242     |
  |                   C2242     |
  |       NRPS11-G1A            |
  |                         CN35|
  |  D4701                      |
  |        74LS14     PC817     |-----------------|
  |                                               |
  |  PAL         PAL                              |
  | (E765B1)    (E765B2)         LCX245           |
  |                                               |
  |  74LS174     PAL                              |
  |             (E765B1)                          |
  |                                               |
  |              74LS174       CN31               |
  |-----------------------------------------------|
  Notes: (all IC's shown)
        This PCB is known to be used for the fishing reel controls on all of the fishing games (at least).

        CN31       - Connector joining this PCB to the MAIN PCB
        CN33       - Connector used to join the external controls connector mounted on the outside of the
                     metal case to this PCB.
        CN35       - Power connector
        NRPS11-G1A - Relay?
        D4701      - NEC uPD4701 Encoder (SOP24)
        C2242      - 2SC2242 Transistor
        PC817      - Sharp PC817 Photo-coupler IC (DIP4)
        PAL        - AMD PALCE16V8Q, stamped 'E765Bx' (DIP20)
*/

#include "driver.h"
#include "state.h"
#include "cdrom.h"
#include "cpu/mips/psx.h"
#include "includes/psx.h"
#include "machine/intelfsh.h"
#include "machine/scsidev.h"
#include "machine/timekpr.h"
#include "machine/adc083x.h"
#include "machine/upd4701.h"
#include "machine/x76f041.h"
#include "sound/psx.h"
#include "sound/cdda.h"
#include <time.h>

#define VERBOSE_LEVEL ( 0 )

INLINE void verboselog( int n_level, const char *s_fmt, ... )
{
	if( VERBOSE_LEVEL >= n_level )
	{
		va_list v;
		char buf[ 32768 ];
		va_start( v, s_fmt );
		vsprintf( buf, s_fmt, v );
		va_end( v );
		if( cpu_getactivecpu() != -1 )
		{
			logerror( "%08x: %s", activecpu_get_pc(), buf );
		}
		else
		{
			logerror( "(timer) : %s", buf );
		}
	}
}

static INT32 flash_bank;
static int flash_chips;
static int onboard_flash_start;
static int pccard1_flash_start;
static int pccard2_flash_start;
static int pccard3_flash_start;
static int pccard4_flash_start;


/* EEPROM handlers */

static NVRAM_HANDLER( konami573 )
{
	int i;

	nvram_handler_timekeeper_0( machine, file, read_or_write );
	nvram_handler_x76f041_0( machine, file, read_or_write );

	for( i = 0; i < flash_chips; i++ )
	{
		nvram_handler_intelflash( machine, i, file, read_or_write );
	}
}

static WRITE32_HANDLER( mb89371_w )
{
	verboselog( 2, "mb89371_w %08x %08x %08x\n", offset, mem_mask, data );
}

static READ32_HANDLER( mb89371_r )
{
	UINT32 data = 0xffffffff;
	verboselog( 2, "mb89371_r %08x %08x\n", offset, mem_mask, data );
	return data;
}

UINT32 stage_mask = 0xffffffff;

static READ32_HANDLER( jamma_r )
{
	UINT32 data = 0;

	switch (offset)
	{
	case 0:
		data = readinputport(0);
		break;
	case 1:
		data = readinputport(1);
		data |= 0x000000c0;
		data |= adc083x_do_read( 0 ) << 16;
		data |= x76f041_sda_read( 0 ) << 18;
		if( pccard1_flash_start < 0 )
		{
			data |= ( 1 << 26 );
		}
		if( pccard2_flash_start < 0 )
		{
			data |= ( 1 << 27 );
		}
		break;
	case 2:
		data = readinputport(2) & stage_mask;
		break;
	case 3:
		data = readinputport(3);
		break;
	}

	verboselog( 2, "jamma_r( %08x, %08x ) %08x\n", offset, mem_mask, data );

	return data;
}

static WRITE32_HANDLER( jamma_w )
{
	verboselog( 2, "jamma_w( %08x, %08x, %08x )\n", offset, mem_mask, data );

	switch( offset )
	{
	case 0:
		adc083x_cs_write( 0, ( data >> 1 ) & 1 );
		adc083x_clk_write( 0, ( data >> 2 ) & 1 );
		adc083x_di_write( 0, ( data >> 0 ) & 1 );
		adc083x_se_write( 0, 0 );
		break;

	default:
		verboselog( 0, "jamma_w: unhandled offset %08x %08x %08x\n", offset, mem_mask, data );
		break;
	}
}

static UINT32 control;

static READ32_HANDLER( control_r )
{
	verboselog( 2, "control_r( %08x, %08x ) %08x\n", offset, mem_mask, control );

	return control;
}

static WRITE32_HANDLER( control_w )
{
//  int old_bank = flash_bank;
	COMBINE_DATA(&control);

	verboselog( 2, "control_w( %08x, %08x, %08x )\n", offset, mem_mask, data );

	flash_bank = -1;

	if( onboard_flash_start >= 0 && ( control & ~0x43 ) == 0x00 )
	{
		flash_bank = onboard_flash_start + ( ( control & 3 ) * 2 );
//      if( flash_bank != old_bank ) mame_printf_debug( "onboard %d\r", control & 3 );
	}
	else if( pccard1_flash_start >= 0 && ( control & ~0x47 ) == 0x10 )
	{
		flash_bank = pccard1_flash_start + ( ( control & 7 ) * 2 );
//      if( flash_bank != old_bank ) mame_printf_debug( "pccard1 %d\r", control & 7 );
	}
	else if( pccard2_flash_start >= 0 && ( control & ~0x47 ) == 0x20 )
	{
		flash_bank = pccard2_flash_start + ( ( control & 7 ) * 2 );
//      if( flash_bank != old_bank ) mame_printf_debug( "pccard2 %d\r", control & 7 );
	}
	else if( pccard3_flash_start >= 0 && ( control & ~0x47 ) == 0x20 )
	{
		flash_bank = pccard3_flash_start + ( ( control & 7 ) * 2 );
//      if( flash_bank != old_bank ) mame_printf_debug( "pccard3 %d\r", control & 7 );
	}
	else if( pccard4_flash_start >= 0 && ( control & ~0x47 ) == 0x28 )
	{
		flash_bank = pccard4_flash_start + ( ( control & 7 ) * 2 );
//      if( flash_bank != old_bank ) mame_printf_debug( "pccard4 %d\r", control & 7 );
	}
}

#define ATAPI_CYCLES_PER_SECTOR (5000)	// plenty of time to allow DMA setup etc.  BIOS requires this be at least 2000, individual games may vary.

static UINT8 atapi_regs[16];
static void *atapi_timer;
static pSCSIDispatch atapi_device;
static void *atapi_device_data;

#define ATAPI_STAT_BSY	   0x80
#define ATAPI_STAT_DRDY    0x40
#define ATAPI_STAT_DMARDDF 0x20
#define ATAPI_STAT_SERVDSC 0x10
#define ATAPI_STAT_DRQ     0x08
#define ATAPI_STAT_CORR    0x04
#define ATAPI_STAT_CHECK   0x01

#define ATAPI_INTREASON_COMMAND 0x01
#define ATAPI_INTREASON_IO      0x02
#define ATAPI_INTREASON_RELEASE 0x04

#define ATAPI_REG_DATA		0
#define ATAPI_REG_ERRFEAT	1
#define ATAPI_REG_INTREASON	2
#define ATAPI_REG_SAMTAG	3
#define ATAPI_REG_COUNTLOW	4
#define ATAPI_REG_COUNTHIGH	5
#define ATAPI_REG_DRIVESEL	6
#define ATAPI_REG_CMDSTATUS	7

static UINT16 atapi_data[32*1024];
static UINT8  atapi_scsi_packet[32*1024];
static int atapi_data_ptr, atapi_data_len, atapi_xferlen, atapi_xferbase, atapi_cdata_wait, atapi_xfermod;

static UINT8 sector_buffer[ 4096 ];

#define MAX_TRANSFER_SIZE ( 63488 )

static void atapi_xfer_end( int x )
{
	int i, n_this;

	timer_adjust(atapi_timer, TIME_NEVER, 0, 0);

//  verboselog( 2, "atapi_xfer_end( %d ) atapi_xferlen = %d, atapi_xfermod=%d\n", x, atapi_xfermod, atapi_xferlen );

//  mame_printf_debug("ATAPI: xfer_end.  xferlen = %d, atapi_xfermod = %d\n", atapi_xferlen, atapi_xfermod);

	while (atapi_xferlen > 0 )
	{
		// get a sector from the SCSI device
		atapi_device(SCSIOP_READ_DATA, atapi_device_data, 2048, sector_buffer);

		atapi_xferlen -= 2048;

		i = 0;
		n_this = 2048 / 4;
		while( n_this > 0 )
		{
			g_p_n_psxram[ atapi_xferbase / 4 ] =
				( sector_buffer[ i + 0 ] << 0 ) |
				( sector_buffer[ i + 1 ] << 8 ) |
				( sector_buffer[ i + 2 ] << 16 ) |
				( sector_buffer[ i + 3 ] << 24 );
			atapi_xferbase += 4;
			i += 4;
			n_this--;
		}
	}

	if (atapi_xfermod > MAX_TRANSFER_SIZE)
	{
		atapi_xferlen = MAX_TRANSFER_SIZE;
		atapi_xfermod = atapi_xfermod - MAX_TRANSFER_SIZE;
	}
	else
	{
		atapi_xferlen = atapi_xfermod;
		atapi_xfermod = 0;
	}

	if (atapi_xferlen > 0)
	{
		//mame_printf_debug("ATAPI: starting next piece of multi-part transfer\n");
		atapi_regs[ATAPI_REG_COUNTLOW] = atapi_xferlen & 0xff;
		atapi_regs[ATAPI_REG_COUNTHIGH] = (atapi_xferlen>>8)&0xff;

		timer_adjust(atapi_timer, TIME_IN_CYCLES((ATAPI_CYCLES_PER_SECTOR * (atapi_xferlen/2048)), 0), 0, 0);
	}
	else
	{
		//mame_printf_debug("ATAPI: Transfer completed, dropping DRQ\n");
		atapi_regs[ATAPI_REG_CMDSTATUS] = ATAPI_STAT_DRDY;
		atapi_regs[ATAPI_REG_INTREASON] = ATAPI_INTREASON_IO | ATAPI_INTREASON_COMMAND;
	}

	psx_irq_set(0x400);
}

static READ32_HANDLER( atapi_r )
{
	int reg, data;
	int i;

	if (mem_mask == 0xffff0000)	// word-wide command read
	{
//      mame_printf_debug("ATAPI: packet read = %04x\n", atapi_data[atapi_data_ptr]);

		// assert IRQ and drop DRQ
		if (atapi_data_ptr == 0 && atapi_data_len == 0)
		{
			while( atapi_xferlen > 0 )
			{
				int transfer = atapi_xferlen;
				if( transfer > 2048 )
				{
					transfer = 2048;
				}

				// get the data from the device
				atapi_device(SCSIOP_READ_DATA, atapi_device_data, transfer, sector_buffer);

				// fix it up in an endian-safe way
				for (i = 0; i < transfer; i += 2)
				{
					atapi_data[ atapi_data_len++ ] = sector_buffer[i] | sector_buffer[i+1]<<8;
				}

				atapi_xferlen -= transfer;
			}

			if (atapi_xfermod > MAX_TRANSFER_SIZE)
			{
				atapi_xferlen = MAX_TRANSFER_SIZE;
				atapi_xfermod = atapi_xfermod - MAX_TRANSFER_SIZE;
			}
			else
			{
				atapi_xferlen = atapi_xfermod;
				atapi_xfermod = 0;
			}

			verboselog( 2, "atapi_r: atapi_xferlen=%d\n", atapi_xferlen );
			if( atapi_xferlen != 0 )
			{
				atapi_regs[ATAPI_REG_CMDSTATUS] = ATAPI_STAT_DRQ | ATAPI_STAT_SERVDSC;
				atapi_regs[ATAPI_REG_INTREASON] = ATAPI_INTREASON_IO;
			}
			else
			{
				//mame_printf_debug("ATAPI: dropping DRQ\n");
				atapi_regs[ATAPI_REG_CMDSTATUS] = 0;
				atapi_regs[ATAPI_REG_INTREASON] = ATAPI_INTREASON_IO;
			}

			atapi_regs[ATAPI_REG_COUNTLOW] = atapi_xferlen & 0xff;
			atapi_regs[ATAPI_REG_COUNTHIGH] = (atapi_xferlen>>8)&0xff;

			psx_irq_set(0x400);
		}

		if( atapi_data_ptr < atapi_data_len )
		{
			data = atapi_data[atapi_data_ptr++];
			if( atapi_data_ptr >= atapi_data_len )
			{
//              verboselog( 2, "atapi_r: read all bytes\n" );
				atapi_data_ptr = 0;
				atapi_data_len = 0;

				if( atapi_xferlen == 0 )
				{
					atapi_regs[ATAPI_REG_CMDSTATUS] = 0;
					atapi_regs[ATAPI_REG_INTREASON] = ATAPI_INTREASON_IO;
					psx_irq_set(0x400);
				}
			}
		}
		else
		{
			data = 0;
		}
	}
	else
	{
		int shift;
		reg = offset<<1;
		shift = 0;
		if (mem_mask == 0xff00ffff)
		{
			reg += 1;
			shift = 16;
		}

		data = atapi_regs[reg];

		switch( reg )
		{
		case ATAPI_REG_DATA:
			verboselog( 1, "atapi_r: data=%02x\n", data );
			break;
		case ATAPI_REG_ERRFEAT:
			verboselog( 1, "atapi_r: errfeat=%02x\n", data );
			break;
		case ATAPI_REG_INTREASON:
			verboselog( 1, "atapi_r: intreason=%02x\n", data );
			break;
		case ATAPI_REG_SAMTAG:
			verboselog( 1, "atapi_r: samtag=%02x\n", data );
			break;
		case ATAPI_REG_COUNTLOW:
			verboselog( 1, "atapi_r: countlow=%02x\n", data );
			break;
		case ATAPI_REG_COUNTHIGH:
			verboselog( 1, "atapi_r: counthigh=%02x\n", data );
			break;
		case ATAPI_REG_DRIVESEL:
			verboselog( 1, "atapi_r: drivesel=%02x\n", data );
			break;
		case ATAPI_REG_CMDSTATUS:
			verboselog( 1, "atapi_r: cmdstatus=%02x\n", data );
			break;
		}

//      mame_printf_debug("ATAPI: read reg %d = %x (PC=%x)\n", reg, data, activecpu_get_pc());

		data <<= shift;
	}

	verboselog( 2, "atapi_r( %08x, %08x ) %08x\n", offset, mem_mask, data );
	return data;
}

static WRITE32_HANDLER( atapi_w )
{
	int reg;
	int i;

	verboselog( 2, "atapi_w( %08x, %08x, %08x )\n", offset, mem_mask, data );

	if (mem_mask == 0xffff0000)	// word-wide command write
	{
		verboselog( 2, "atapi_w: data=%04x\n", data );

//      mame_printf_debug("ATAPI: packet write %04x\n", data);
		atapi_data[atapi_data_ptr++] = data;

		if (atapi_cdata_wait)
		{
//          mame_printf_debug("ATAPI: waiting, ptr %d wait %d\n", atapi_data_ptr, atapi_cdata_wait);
			if (atapi_data_ptr == atapi_cdata_wait)
			{
				// decompose SCSI packet into proper byte order
				for (i = 0; i < atapi_cdata_wait; i += 2)
				{
					atapi_scsi_packet[i] = atapi_data[i/2]&0xff;
					atapi_scsi_packet[i+1] = atapi_data[i/2]>>8;
				}

				// send it to the device
				atapi_device(SCSIOP_WRITE_DATA, atapi_device_data, atapi_cdata_wait, atapi_scsi_packet);

				// assert IRQ
				psx_irq_set(0x400);

				// not sure here, but clear DRQ at least?
				atapi_regs[ATAPI_REG_CMDSTATUS] = 0;
			}
		}

		if ((!atapi_cdata_wait) && (atapi_data_ptr == 6))
		{
			verboselog( 2, "atapi_w: command %02x\n", atapi_data[0]&0xff );

			// reset data pointer for reading SCSI results
			atapi_data_ptr = 0;
			atapi_data_len = 0;

			// decompose SCSI packet into proper byte order
			for (i = 0; i < 16; i += 2)
			{
				atapi_scsi_packet[i] = atapi_data[i/2]&0xff;
				atapi_scsi_packet[i+1] = atapi_data[i/2]>>8;
			}

			// send it to the SCSI device
			atapi_xferlen = atapi_device(SCSIOP_EXEC_COMMAND, atapi_device_data, 0, atapi_scsi_packet);

			if (atapi_xferlen != -1)
			{
//              mame_printf_debug("ATAPI: SCSI command %02x returned %d bytes from the device\n", atapi_data[0]&0xff, atapi_xferlen);

				// store the returned command length in the ATAPI regs, splitting into
				// multiple transfers if necessary
				atapi_xfermod = 0;
				if (atapi_xferlen > MAX_TRANSFER_SIZE)
				{
					atapi_xfermod = atapi_xferlen - MAX_TRANSFER_SIZE;
					atapi_xferlen = MAX_TRANSFER_SIZE;
				}

				atapi_regs[ATAPI_REG_COUNTLOW] = atapi_xferlen & 0xff;
				atapi_regs[ATAPI_REG_COUNTHIGH] = (atapi_xferlen>>8)&0xff;

				if (atapi_xferlen == 0)
				{
					// if no data to return, set the registers properly
					atapi_regs[ATAPI_REG_CMDSTATUS] = ATAPI_STAT_DRDY;
					atapi_regs[ATAPI_REG_INTREASON] = ATAPI_INTREASON_IO|ATAPI_INTREASON_COMMAND;
				}
				else
				{
					// indicate data ready: set DRQ and DMA ready, and IO in INTREASON
					atapi_regs[ATAPI_REG_CMDSTATUS] = ATAPI_STAT_DRQ | ATAPI_STAT_SERVDSC;
					atapi_regs[ATAPI_REG_INTREASON] = ATAPI_INTREASON_IO;
				}

				// perform special ATAPI processing of certain commands
				switch (atapi_data[0]&0xff)
				{
					case 0x55:	// MODE SELECT
						atapi_cdata_wait = atapi_data[4]/2;
						break;

					case 0x00:	// BUS RESET / TEST UNIT READY
						atapi_regs[ATAPI_REG_CMDSTATUS] = 0;
						break;

					case 0x45: // PLAY
						atapi_regs[ATAPI_REG_CMDSTATUS] = ATAPI_STAT_BSY;
						timer_adjust( atapi_timer, TIME_IN_CYCLES( ATAPI_CYCLES_PER_SECTOR, 0 ), 0, 0 );
						break;

					case 0xbb: // SET CDROM SPEED
						atapi_regs[ATAPI_REG_CMDSTATUS] = 0;
						break;
				}

				// assert IRQ
				psx_irq_set(0x400);
			}
			else
			{
//              mame_printf_debug("ATAPI: SCSI device returned error!\n");

				atapi_regs[ATAPI_REG_CMDSTATUS] = ATAPI_STAT_DRQ | ATAPI_STAT_CHECK;
				atapi_regs[ATAPI_REG_ERRFEAT] = 0x50;	// sense key = ILLEGAL REQUEST
				atapi_regs[ATAPI_REG_COUNTLOW] = 0;
				atapi_regs[ATAPI_REG_COUNTHIGH] = 0;
			}
		}
	}
	else
	{
		reg = offset<<1;
		if (mem_mask == 0xff00ffff)
		{
			reg += 1;
			data >>= 16;
		}

		switch( reg )
		{
		case ATAPI_REG_DATA:
			verboselog( 1, "atapi_w: data=%02x\n", data );
			break;
		case ATAPI_REG_ERRFEAT:
			verboselog( 1, "atapi_w: errfeat=%02x\n", data );
			break;
		case ATAPI_REG_INTREASON:
			verboselog( 1, "atapi_w: intreason=%02x\n", data );
			break;
		case ATAPI_REG_SAMTAG:
			verboselog( 1, "atapi_w: samtag=%02x\n", data );
			break;
		case ATAPI_REG_COUNTLOW:
			verboselog( 1, "atapi_w: countlow=%02x\n", data );
			break;
		case ATAPI_REG_COUNTHIGH:
			verboselog( 1, "atapi_w: counthigh=%02x\n", data );
			break;
		case ATAPI_REG_DRIVESEL:
			verboselog( 1, "atapi_w: drivesel=%02x\n", data );
			break;
		case ATAPI_REG_CMDSTATUS:
			verboselog( 1, "atapi_w: cmdstatus=%02x\n", data );
			break;
		}

		atapi_regs[reg] = data;
//      mame_printf_debug("ATAPI: reg %d = %x (offset %x mask %x PC=%x)\n", reg, data, offset, mem_mask, activecpu_get_pc());

		if (reg == ATAPI_REG_CMDSTATUS)
		{
//          mame_printf_debug("ATAPI command %x issued! (PC=%x)\n", data, activecpu_get_pc());

			switch (data)
			{
				case 0xa0:	// PACKET
		 			atapi_regs[ATAPI_REG_CMDSTATUS] = ATAPI_STAT_DRQ;
					atapi_regs[ATAPI_REG_INTREASON] = ATAPI_INTREASON_COMMAND;

					atapi_data_ptr = 0;
					atapi_data_len = 0;

					/* we have no data */
					atapi_xferlen = 0;
					atapi_xfermod = 0;

					atapi_cdata_wait = 0;
					break;

				case 0xa1:	// IDENTIFY PACKET DEVICE
		 			atapi_regs[ATAPI_REG_CMDSTATUS] = ATAPI_STAT_DRQ;

					atapi_data_ptr = 0;
					atapi_data_len = 512;

					/* we have no data */
					atapi_xferlen = 0;
					atapi_xfermod = 0;

					memset( atapi_data, 0, atapi_data_len );

					atapi_data[0] = 0x8500;	// ATAPI device, cmd set 5 compliant, DRQ within 3 ms of PACKET command
					atapi_data[49] = 0x0400; // IORDY may be disabled

					atapi_regs[ATAPI_REG_COUNTLOW] = 0;
					atapi_regs[ATAPI_REG_COUNTHIGH] = 2;

					psx_irq_set(0x400);
					break;

				case 0xef:	// SET FEATURES
		 			atapi_regs[ATAPI_REG_CMDSTATUS] = 0;

					atapi_data_ptr = 0;
					atapi_data_len = 0;

					psx_irq_set(0x400);
					break;

				default:
					mame_printf_debug("ATAPI: Unknown IDE command %x\n", data);
					break;
			}
		}
	 }
}

static void atapi_init(void)
{
	memset(atapi_regs, 0, sizeof(atapi_regs));

	atapi_regs[ATAPI_REG_CMDSTATUS] = 0;
	atapi_regs[ATAPI_REG_ERRFEAT] = 1;
	atapi_regs[ATAPI_REG_COUNTLOW] = 0x14;
	atapi_regs[ATAPI_REG_COUNTHIGH] = 0xeb;

	atapi_data_ptr = 0;
	atapi_data_len = 0;
	atapi_cdata_wait = 0;

	atapi_timer = timer_alloc( atapi_xfer_end );
	timer_adjust(atapi_timer, TIME_NEVER, 0, 0);

	// allocate a SCSI CD-ROM device
	atapi_device = SCSI_DEVICE_CDROM;
	atapi_device(SCSIOP_ALLOC_INSTANCE, &atapi_device_data, 0, (UINT8 *)NULL);
}

static WRITE32_HANDLER( atapi_reset_w )
{
	verboselog( 2, "atapi_reset_w( %08x, %08x, %08x )\n", offset, mem_mask, data );

	if (data)
	{
		verboselog( 1, "atapi_reset_w: reset\n" );

//      mame_printf_debug("ATAPI reset\n");

		atapi_regs[ATAPI_REG_CMDSTATUS] = 0;
		atapi_regs[ATAPI_REG_ERRFEAT] = 1;
		atapi_regs[ATAPI_REG_COUNTLOW] = 0x14;
		atapi_regs[ATAPI_REG_COUNTHIGH] = 0xeb;

		atapi_data_ptr = 0;
		atapi_data_len = 0;
		atapi_cdata_wait = 0;

		atapi_xferlen = 0;
		atapi_xfermod = 0;
	}
}

static void cdrom_dma_read( UINT32 n_address, INT32 n_size )
{
	verboselog( 2, "cdrom_dma_read( %08x, %08x )\n", n_address, n_size );
//  mame_printf_debug("DMA read: address %08x size %08x\n", n_address, n_size);
}

static void cdrom_dma_write( UINT32 n_address, INT32 n_size )
{
	verboselog( 2, "cdrom_dma_write( %08x, %08x )\n", n_address, n_size );
//  mame_printf_debug("DMA write: address %08x size %08x\n", n_address, n_size);

	atapi_xferbase = n_address;

	// set a transfer complete timer (Note: CYCLES_PER_SECTOR can't be lower than 2000 or the BIOS ends up "out of order")
	timer_adjust(atapi_timer, TIME_IN_CYCLES((ATAPI_CYCLES_PER_SECTOR * (atapi_xferlen/2048)), 0), 0, 0);
}

static UINT32 m_n_security_control;

static WRITE32_HANDLER( security_w )
{
	COMBINE_DATA( &m_n_security_control );

	verboselog( 2, "security_w( %08x, %08x, %08x )\n", offset, mem_mask, data );

	if( ACCESSING_LSW32 )
	{
		x76f041_sda_write( 0, ( data >> 0 ) & 1 );
		x76f041_scl_write( 0, ( data >> 1 ) & 1 );
		x76f041_cs_write( 0, ( data >> 2 ) & 1 );
		x76f041_rst_write( 0, ( data >> 3 ) & 1 );
	}
}

static READ32_HANDLER( security_r )
{
	UINT32 data = m_n_security_control;
	verboselog( 2, "security_r( %08x, %08x ) %08x\n", offset, mem_mask, data );
	return data;
}

static READ32_HANDLER( flash_r )
{
	UINT32 data = 0;

	if( flash_bank < 0 )
	{
		mame_printf_debug( "%08x: flash_r( %08x, %08x ) no bank selected %08x\n", activecpu_get_pc(), offset, mem_mask, control );
		data = 0xffffffff;
	}
	else
	{
		int adr = offset * 2;

		if( ACCESSING_LSB32 )
		{
			data |= ( intelflash_read( flash_bank + 0, adr + 0 ) & 0xff ) << 0; // 31m/31l/31j/31h
		}
		if( ( mem_mask & 0x0000ff00 ) == 0 )
		{
			data |= ( intelflash_read( flash_bank + 1, adr + 0 ) & 0xff ) << 8; // 27m/27l/27j/27h
		}
		if( ( mem_mask & 0x00ff0000 ) == 0 )
		{
			data |= ( intelflash_read( flash_bank + 0, adr + 1 ) & 0xff ) << 16; // 31m/31l/31j/31h
		}
		if( ACCESSING_MSB32 )
		{
			data |= ( intelflash_read( flash_bank + 1, adr + 1 ) & 0xff ) << 24; // 27m/27l/27j/27h
		}
	}

	verboselog( 2, "flash_r( %08x, %08x, %08x)\n", offset, mem_mask, data );

	return data;
}

static WRITE32_HANDLER( flash_w )
{
	verboselog( 2, "flash_w( %08x, %08x, %08x\n", offset, mem_mask, data );

	if( flash_bank < 0 )
	{
		mame_printf_debug( "%08x: flash_w( %08x, %08x, %08x ) no bank selected %08x\n", activecpu_get_pc(), offset, mem_mask, data, control );
	}
	else
	{
		int adr = offset * 2;

		if( ( mem_mask & 0x000000ff ) == 0 )
		{
			intelflash_write( flash_bank + 0, adr + 0, ( data >> 0 ) & 0xff );
		}
		if( ( mem_mask & 0x0000ff00 ) == 0 )
		{
			intelflash_write( flash_bank + 1, adr + 0, ( data >> 8 ) & 0xff );
		}
		if( ( mem_mask & 0x00ff0000 ) == 0 )
		{
			intelflash_write( flash_bank + 0, adr + 1, ( data >> 16 ) & 0xff );
		}
		if( ( mem_mask & 0xff000000 ) == 0 )
		{
			intelflash_write( flash_bank + 1, adr + 1, ( data >> 24 ) & 0xff );
		}
	}
}

/* Root Counters */

static void *m_p_timer_root[ 3 ];
static UINT16 m_p_n_root_count[ 3 ];
static UINT16 m_p_n_root_mode[ 3 ];
static UINT16 m_p_n_root_target[ 3 ];
static UINT32 m_p_n_root_start[ 3 ];

#define RC_STOP ( 0x01 )
#define RC_RESET ( 0x04 ) /* guess */
#define RC_COUNTTARGET ( 0x08 )
#define RC_IRQTARGET ( 0x10 )
#define RC_IRQOVERFLOW ( 0x20 )
#define RC_REPEAT ( 0x40 )
#define RC_CLC ( 0x100 )
#define RC_DIV ( 0x200 )

static UINT32 psxcpu_gettotalcycles( void )
{
	/* TODO: should return the start of the current tick. */
	return cpunum_gettotalcycles(0) * 2;
}

static int root_divider( int n_counter )
{
	if( n_counter == 0 && ( m_p_n_root_mode[ n_counter ] & RC_CLC ) != 0 )
	{
		/* TODO: pixel clock, probably based on resolution */
		return 5;
	}
	else if( n_counter == 1 && ( m_p_n_root_mode[ n_counter ] & RC_CLC ) != 0 )
	{
		return 2150;
	}
	else if( n_counter == 2 && ( m_p_n_root_mode[ n_counter ] & RC_DIV ) != 0 )
	{
		return 8;
	}
	return 1;
}

static UINT16 root_current( int n_counter )
{
	if( ( m_p_n_root_mode[ n_counter ] & RC_STOP ) != 0 )
	{
		return m_p_n_root_count[ n_counter ];
	}
	else
	{
		UINT32 n_current;
		n_current = psxcpu_gettotalcycles() - m_p_n_root_start[ n_counter ];
		n_current /= root_divider( n_counter );
		n_current += m_p_n_root_count[ n_counter ];
		if( n_current > 0xffff )
		{
			/* TODO: use timer for wrap on 0x10000. */
			m_p_n_root_count[ n_counter ] = n_current;
			m_p_n_root_start[ n_counter ] = psxcpu_gettotalcycles();
		}
		return n_current;
	}
}

static int root_target( int n_counter )
{
	if( ( m_p_n_root_mode[ n_counter ] & RC_COUNTTARGET ) != 0 ||
		( m_p_n_root_mode[ n_counter ] & RC_IRQTARGET ) != 0 )
	{
		return m_p_n_root_target[ n_counter ];
	}
	return 0x10000;
}

static void root_timer_adjust( int n_counter )
{
	if( ( m_p_n_root_mode[ n_counter ] & RC_STOP ) != 0 )
	{
		timer_adjust( m_p_timer_root[ n_counter ], TIME_NEVER, n_counter, 0 );
	}
	else
	{
		int n_duration;

		n_duration = root_target( n_counter ) - root_current( n_counter );
		if( n_duration < 1 )
		{
			n_duration += 0x10000;
		}

		n_duration *= root_divider( n_counter );

		timer_adjust( m_p_timer_root[ n_counter ], TIME_IN_SEC( (double)n_duration / 33868800 ), n_counter, 0 );
	}
}

static void root_finished( int n_counter )
{
//  if( ( m_p_n_root_mode[ n_counter ] & RC_COUNTTARGET ) != 0 )
	{
		/* TODO: wrap should be handled differently as RC_COUNTTARGET & RC_IRQTARGET don't have to be the same. */
		m_p_n_root_count[ n_counter ] = 0;
		m_p_n_root_start[ n_counter ] = psxcpu_gettotalcycles();
	}
	if( ( m_p_n_root_mode[ n_counter ] & RC_REPEAT ) != 0 )
	{
		root_timer_adjust( n_counter );
	}
	if( ( m_p_n_root_mode[ n_counter ] & RC_IRQOVERFLOW ) != 0 ||
		( m_p_n_root_mode[ n_counter ] & RC_IRQTARGET ) != 0 )
	{
		psx_irq_set( 0x10 << n_counter );
	}
}

WRITE32_HANDLER( k573_counter_w )
{
	int n_counter;

	n_counter = offset / 4;

	switch( offset % 4 )
	{
	case 0:
		m_p_n_root_count[ n_counter ] = data;
		m_p_n_root_start[ n_counter ] = psxcpu_gettotalcycles();
		break;
	case 1:
		m_p_n_root_count[ n_counter ] = root_current( n_counter );
		m_p_n_root_start[ n_counter ] = psxcpu_gettotalcycles();
		m_p_n_root_mode[ n_counter ] = data;

		if( ( m_p_n_root_mode[ n_counter ] & RC_RESET ) != 0 )
		{
			/* todo: check this is correct */
			m_p_n_root_count[ n_counter ] = 0;
			m_p_n_root_mode[ n_counter ] &= ~( RC_STOP | RC_RESET );
		}
//      if( ( data & 0xfca6 ) != 0 ||
//          ( ( data & 0x0100 ) != 0 && n_counter != 0 && n_counter != 1 ) ||
//          ( ( data & 0x0200 ) != 0 && n_counter != 2 ) )
//      {
//          printf( "mode %d 0x%04x\n", n_counter, data & 0xfca6 );
//      }
		break;
	case 2:
		m_p_n_root_target[ n_counter ] = data;
		break;
	default:
		return;
	}

	root_timer_adjust( n_counter );
}

READ32_HANDLER( k573_counter_r )
{
	int n_counter;
	UINT32 data;

	n_counter = offset / 4;

	switch( offset % 4 )
	{
	case 0:
		data = root_current( n_counter );
		break;
	case 1:
		data = m_p_n_root_mode[ n_counter ];
		break;
	case 2:
		data = m_p_n_root_target[ n_counter ];
		break;
	default:
		return 0;
	}
	return data;
}

static ADDRESS_MAP_START( konami573_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x003fffff) AM_RAM	AM_SHARE(1) AM_BASE(&g_p_n_psxram) AM_SIZE(&g_n_psxramsize) /* ram */
	AM_RANGE(0x1f000000, 0x1f3fffff) AM_READWRITE( flash_r, flash_w )
	AM_RANGE(0x1f400000, 0x1f40001f) AM_READWRITE( jamma_r, jamma_w )
	AM_RANGE(0x1f480000, 0x1f48000f) AM_READWRITE( atapi_r, atapi_w )	// IDE controller, used mostly in ATAPI mode (only 3 pure IDE commands seen so far)
	AM_RANGE(0x1f500000, 0x1f500003) AM_READWRITE( control_r, control_w )	// Konami can't make a game without a "control" register.
	AM_RANGE(0x1f560000, 0x1f560003) AM_WRITE( atapi_reset_w )
	AM_RANGE(0x1f5c0000, 0x1f5c0003) AM_WRITENOP 				// watchdog?
	AM_RANGE(0x1f620000, 0x1f623fff) AM_READWRITE( timekeeper_0_32le_lsb16_r, timekeeper_0_32le_lsb16_w )
	AM_RANGE(0x1f680000, 0x1f68001f) AM_READWRITE(mb89371_r, mb89371_w)
	AM_RANGE(0x1f6a0000, 0x1f6a0003) AM_READWRITE( security_r, security_w )
	AM_RANGE(0x1f800000, 0x1f8003ff) AM_RAM /* scratchpad */
	AM_RANGE(0x1f801000, 0x1f801007) AM_WRITENOP
	AM_RANGE(0x1f801008, 0x1f80100b) AM_RAM /* ?? */
	AM_RANGE(0x1f80100c, 0x1f80102f) AM_WRITENOP
	AM_RANGE(0x1f801010, 0x1f801013) AM_READNOP
	AM_RANGE(0x1f801014, 0x1f801017) AM_READ(psx_spu_delay_r)
	AM_RANGE(0x1f801040, 0x1f80105f) AM_READWRITE(psx_sio_r, psx_sio_w)
	AM_RANGE(0x1f801060, 0x1f80106f) AM_WRITENOP
	AM_RANGE(0x1f801070, 0x1f801077) AM_READWRITE(psx_irq_r, psx_irq_w)
	AM_RANGE(0x1f801080, 0x1f8010ff) AM_READWRITE(psx_dma_r, psx_dma_w)
	AM_RANGE(0x1f801100, 0x1f80112f) AM_READWRITE(k573_counter_r, k573_counter_w)
	AM_RANGE(0x1f801810, 0x1f801817) AM_READWRITE(psx_gpu_r, psx_gpu_w)
	AM_RANGE(0x1f801820, 0x1f801827) AM_READWRITE(psx_mdec_r, psx_mdec_w)
	AM_RANGE(0x1f801c00, 0x1f801dff) AM_READWRITE(psx_spu_r, psx_spu_w)
	AM_RANGE(0x1f802020, 0x1f802033) AM_RAM /* ?? */
	AM_RANGE(0x1f802040, 0x1f802043) AM_WRITENOP
	AM_RANGE(0x1fc00000, 0x1fc7ffff) AM_ROM AM_SHARE(2) AM_REGION(REGION_USER1, 0) /* bios */
	AM_RANGE(0x80000000, 0x803fffff) AM_RAM AM_SHARE(1) /* ram mirror */
	AM_RANGE(0x9fc00000, 0x9fc7ffff) AM_ROM AM_SHARE(2) /* bios mirror */
	AM_RANGE(0xa0000000, 0xa03fffff) AM_RAM AM_SHARE(1) /* ram mirror */
	AM_RANGE(0xbfc00000, 0xbfc7ffff) AM_ROM AM_SHARE(2) /* bios mirror */
	AM_RANGE(0xfffe0130, 0xfffe0133) AM_WRITENOP
ADDRESS_MAP_END


static void flash_init( void )
{
	int i;
	int chip;
	int size;
	unsigned char *data;
	static struct
	{
		int *start;
		int region;
		int chips;
		int type;
		int size;
	}
	flash_init[] =
	{
		{ &onboard_flash_start, REGION_USER3,  8, FLASH_FUJITSU_29F016A, 0x200000 },
		{ &pccard1_flash_start, REGION_USER4, 16, FLASH_FUJITSU_29F016A, 0x200000 },
		{ &pccard2_flash_start, REGION_USER5, 16, FLASH_FUJITSU_29F016A, 0x200000 },
		{ &pccard3_flash_start, REGION_USER6, 16, FLASH_FUJITSU_29F016A, 0x200000 },
		{ &pccard4_flash_start, REGION_USER7, 16, FLASH_FUJITSU_29F016A, 0x200000 },
		{ NULL, 0, 0, 0, 0 },
	};

	flash_chips = 0;

	i = 0;
	while( flash_init[ i ].start != NULL )
	{
		data = memory_region( flash_init[ i ].region );
		if( data != NULL )
		{
			size = 0;
			*( flash_init[ i ].start ) = flash_chips;
			for( chip = 0; chip < flash_init[ i ].chips; chip++ )
			{
				intelflash_init( flash_chips, flash_init[ i ].type, data + size );
				size += flash_init[ i ].size;
				flash_chips++;
			}
			if( size != memory_region_length( flash_init[ i ].region ) )
			{
				fatalerror( "flash_init %d incorrect region length\n", i );
			}
		}
		else
		{
			*( flash_init[ i ].start ) = -1;
		}
		i++;
	}
}

static double analogue_inputs_callback( int input )
{
	switch( input )
	{
	case ADC083X_CH0:
		return (double) ( readinputportbytag_safe( "analog0", 0 ) * 5 ) / 255;
	case ADC083X_CH1:
		return (double) ( readinputportbytag_safe( "analog1", 0 ) * 5 ) / 255;
	case ADC083X_CH2:
		return (double) ( readinputportbytag_safe( "analog2", 0 ) * 5 ) / 255;
	case ADC083X_CH3:
		return (double) ( readinputportbytag_safe( "analog3", 0 ) * 5 ) / 255;
	case ADC083X_AGND:
		return 0;
	case ADC083X_VREF:
		return 5;
	}
	return 0;
}

void *atapi_get_device(void)
{
	void *ret;
	atapi_device(SCSIOP_GET_DEVICE, atapi_device_data, 0, (UINT8 *)&ret);
	return ret;
}

static DRIVER_INIT( konami573 )
{
	int i;

	psx_driver_init();
	atapi_init();
	psx_dma_install_read_handler(5, cdrom_dma_read);
	psx_dma_install_write_handler(5, cdrom_dma_write);

	for (i = 0; i < 3; i++)
	{
		m_p_timer_root[i] = timer_alloc(root_finished);
	}

	timekeeper_init( 0, TIMEKEEPER_M48T58, NULL );
	x76f041_init( 0, memory_region( REGION_USER2 ) );
	adc083x_init( 0, ADC0834, analogue_inputs_callback );
	flash_init();
}

static MACHINE_RESET( konami573 )
{
	psx_machine_init();

	/* security cart */
	psx_sio_input( 1, PSX_SIO_IN_DSR, PSX_SIO_IN_DSR );

	cdda_set_cdrom(0, atapi_get_device());

	flash_bank = -1;
}

static struct PSXSPUinterface konami573_psxspu_interface =
{
	&g_p_n_psxram,
	psx_irq_set,
	psx_dma_install_read_handler,
	psx_dma_install_write_handler
};

INTERRUPT_GEN( sys573_vblank )
{
	if( strcmp( Machine->gamedrv->name, "ddr2ndl" ) == 0 )
	{
		/* patch out security-plate error */

		/* 8001f850: jal $8003221c */
		if( g_p_n_psxram[ 0x1f850 / 4 ] == 0x0c00c887 )
		{
			/* 8001f850: j $8001f888 */
			g_p_n_psxram[ 0x1f850 / 4 ] = 0x08007e22;
		}
	}
	psx_vblank();
}

/*
GE765-PWB(B)A

todo:
  find out what offset 4 is
  fix reel type detection
  find adc0834 SARS

*/

static READ32_HANDLER( ge765pwbba_r )
{
	UINT32 data = 0;

	switch( offset )
	{
	case 0x26:
		uPD4701_y_add( 0, readinputportbytag_safe( "uPD4701_y", 0 ) );
		uPD4701_switches_set( 0, readinputportbytag_safe( "uPD4701_switches", 0 ) );

		uPD4701_cs_w( 0, 0 );
		uPD4701_xy_w( 0, 1 );

		if( ( mem_mask & 0x000000ff ) != 0x000000ff )
		{
			uPD4701_ul_w( 0, 0 );
			data |= uPD4701_d_r( 0 ) << 0;
		}

		if( ( mem_mask & 0x00ff0000 ) != 0x00ff0000 )
		{
			uPD4701_ul_w( 0, 1 );
			data |= uPD4701_d_r( 0 ) << 16;
		}

		uPD4701_cs_w( 0, 1 );
		break;

	default:
		verboselog( 0, "ge765pwbba_r: unhandled offset %08x %08x\n", offset, mem_mask );
		break;
	}

	verboselog( 2, "ge765pwbba_r( %08x, %08x ) %08x\n", offset, mem_mask, data );
	return data;
}

static WRITE32_HANDLER( ge765pwbba_w )
{
	switch( offset )
	{
	case 0x04:
		break;

	case 0x20:
		if( ( mem_mask & 0x000000ff ) != 0x000000ff )
		{
			output_set_value( "motor", data & 0xff );
		}
		break;

	case 0x22:
		if( ( mem_mask & 0x000000ff ) != 0x000000ff )
		{
			output_set_value( "brake", data & 0xff );
		}
		break;

	case 0x28:
		if( ( mem_mask & 0x000000ff ) != 0x000000ff )
		{
			uPD4701_resety_w( 0, 1 );
			uPD4701_resety_w( 0, 0 );
		}
		break;

	default:
		verboselog( 0, "ge765pwbba_w: unhandled offset %08x %08x %08x\n", offset, mem_mask, data );
		break;
	}

	verboselog( 2, "ge765pwbba_w( %08x, %08x, %08x )\n", offset, mem_mask, data );
}

static DRIVER_INIT( ge765pwbba )
{
	init_konami573( machine );

	uPD4701_init( 0 );

	memory_install_read32_handler ( 0, ADDRESS_SPACE_PROGRAM, 0x1f640000, 0x1f6400ff, 0, 0, ge765pwbba_r );
	memory_install_write32_handler( 0, ADDRESS_SPACE_PROGRAM, 0x1f640000, 0x1f6400ff, 0, 0, ge765pwbba_w );
}

/*
GX700-PWB(F)
*/

static UINT8 gx700pwbf_output_data[ 4 ];
static void (*gx700pwfbf_output_callback)( int offset, int data );

static READ32_HANDLER( gx700pwbf_io_r )
{
	UINT32 data = 0;
	switch( offset )
	{
	case 0x20:
		/* result not used? */
		break;

	case 0x22:
		/* result not used? */
		break;

	case 0x24:
		/* result not used? */
		break;

	case 0x26:
		/* result not used? */
		break;

	default:
//      printf( "gx700pwbf_io_r( %08x, %08x ) %08x\n", offset, mem_mask, data );
		break;
	}

	verboselog( 2, "gx700pwbf_io_r( %08x, %08x ) %08x\n", offset, mem_mask, data );

	return data;
}

static void gx700pwbf_output( int offset, UINT8 data )
{
	int i;
	static int shift[] = { 7, 6, 1, 0, 5, 4, 3, 2 };
	for( i = 0; i < 8; i++ )
	{
		int oldbit = ( gx700pwbf_output_data[ offset ] >> shift[ i ] ) & 1;
		int newbit = ( data >> shift[ i ] ) & 1;
		if( oldbit != newbit )
		{
			gx700pwfbf_output_callback( ( offset * 8 ) + i, newbit );
		}
	}
	gx700pwbf_output_data[ offset ] = data;
}

static WRITE32_HANDLER( gx700pwbf_io_w )
{
	verboselog( 2, "gx700pwbf_io_w( %08x, %08x, %08x )\n", offset, mem_mask, data );

	switch( offset )
	{
	case 0x20:

		if( ACCESSING_LSW32 )
		{
			gx700pwbf_output( 0, data & 0xff );
		}
		break;

	case 0x22:
		if( ACCESSING_LSW32 )
		{
			gx700pwbf_output( 1, data & 0xff );
		}
		break;

	case 0x24:
		if( ACCESSING_LSW32 )
		{
			gx700pwbf_output( 2, data & 0xff );
		}
		break;

	case 0x26:
		if( ACCESSING_LSW32 )
		{
			gx700pwbf_output( 3, data & 0xff );
		}
		break;

	default:
//      printf( "gx700pwbf_io_w( %08x, %08x, %08x )\n", offset, mem_mask, data );
		break;
	}
}

static void gx700pwfbf_init( void (*output_callback)( int offset, int data ) )
{
	memset( gx700pwbf_output_data, 0, sizeof( gx700pwbf_output_data ) );

	gx700pwfbf_output_callback = output_callback;

	memory_install_read32_handler ( 0, ADDRESS_SPACE_PROGRAM, 0x1f640000, 0x1f6400ff, 0, 0, gx700pwbf_io_r );
	memory_install_write32_handler( 0, ADDRESS_SPACE_PROGRAM, 0x1f640000, 0x1f6400ff, 0, 0, gx700pwbf_io_w );
}

/*

DDR Stage

TODO:
  find out the pcb id

*/

#define DDR_STAGE_IDLE ( 0 )
#define DDR_STAGE_INIT ( 1 )

static struct
{
	int DO;
	int shift;
	int state;
	int bit;
} stage[ 2 ];

static int mask[] =
{
	0, 6, 2, 4,
	0, 4, 0, 4,
	0, 4, 0, 4,
	0, 4, 0, 4,
	0, 4, 0, 4,
	0, 4, 0, 6
};

static void ddr_stage_do_w( int offset, int data )
{
	stage[ offset ].DO = data;
}

static void ddr_stage_clk_w( int offset, int data )
{
	if( data )
	{
		stage[ offset ].shift = ( stage[ offset ].shift >> 1 ) | ( stage[ offset ].DO << 12 );

		switch( stage[ offset ].state )
		{
		case DDR_STAGE_IDLE:
			if( stage[ offset ].shift == 0xc90 )
			{
				stage[ offset ].state = DDR_STAGE_INIT;
				stage[ offset ].bit = 0;
				stage_mask = 0xfffff9f9;
			}
			break;

		case DDR_STAGE_INIT:
			stage[ offset ].bit++;
			if( stage[ offset ].bit < 22 )
			{
				int a = ( ( ( ( ~0x06 ) | mask[ stage[ 0 ].bit ] ) & 0xff ) << 8 );
				int b = ( ( ( ( ~0x06 ) | mask[ stage[ 1 ].bit ] ) & 0xff ) << 0 );

				stage_mask = 0xffff0000 | a | b;
			}
			else
			{
				stage[ offset ].bit = 0;
				stage[ offset ].state = DDR_STAGE_IDLE;

				stage_mask = 0xffffffff;
			}
			break;
		}

		verboselog( 2, "stage: %dp data d0=%d shift=%08x bit=%d stage_mask=%08x\n", offset + 1, stage[ offset ].DO, stage[ offset ].shift, stage[ offset ].bit, stage_mask );
	}
}

static void ddr_output_callback( int offset, int data )
{
	switch( offset )
	{
	case 0:
		output_set_value( "foot 1p up", !data );
		break;

	case 1:
		output_set_value( "foot 1p left", !data );
		break;

	case 2:
		output_set_value( "foot 1p right", !data );
		break;

	case 3:
		output_set_value( "foot 1p down", !data );
		break;

	case 4:
		ddr_stage_do_w( 0, data );
		break;

	case 7:
		ddr_stage_clk_w( 0, data );
		break;

	case 8:
		output_set_value( "foot 2p up", !data );
		break;

	case 9:
		output_set_value( "foot 2p left", !data );
		break;

	case 10:
		output_set_value( "foot 2p right", !data );
		break;

	case 11:
		output_set_value( "foot 2p down", !data );
		break;

	case 12:
		ddr_stage_do_w( 1, data );
		break;

	case 15:
		ddr_stage_clk_w( 1, data );
		break;

	case 17:
		output_set_led_value( 0, !data ); // start 1
		break;

	case 18:
		output_set_led_value( 1, !data ); // start 2
		break;

	case 20:
		output_set_value( "body right low", !data );
		break;

	case 21:
		output_set_value( "body left low", !data );
		break;

	case 22:
		output_set_value( "body left high", !data );
		break;

	case 23:
		output_set_value( "body right high", !data );
		break;

	case 30:
		output_set_value( "speaker", !data );
		break;

	default:
//      printf( "%d=%d\n", offset, data );
		break;
	}
}

static DRIVER_INIT( ddr )
{
	init_konami573( machine );

	gx700pwfbf_init( ddr_output_callback );
}

/*

Guitar Freaks

todo:
  find out what offset 4 is
  find out the pcb id

*/

static READ32_HANDLER( gtrfrks_io_r )
{
	UINT32 data = 0;
	switch( offset )
	{
	case 0:
		break;

	default:
		verboselog( 0, "gtrfrks_io_r: unhandled offset %08x, %08x\n", offset, mem_mask );
		break;
	}

	verboselog( 2, "gtrfrks_io_r( %08x, %08x ) %08x\n", data );
	return data;
}

static WRITE32_HANDLER( gtrfrks_io_w )
{
	verboselog( 2, "gtrfrks_io_w( %08x, %08x ) %08x\n", data );

	switch( offset )
	{
	case 0:
		output_set_value( "spot left", !( ( data >> 7 ) & 1 ) );
		output_set_value( "spot right", !( ( data >> 6 ) & 1 ) );
		output_set_led_value( 0, !( ( data >> 5 ) & 1 ) ); // start left
		output_set_led_value( 1, !( ( data >> 4 ) & 1 ) ); // start right
		break;

	case 4:
		break;

	default:
		verboselog( 0, "gtrfrks_io_w: unhandled offset %08x, %08x\n", offset, mem_mask );
		break;
	}
}

static DRIVER_INIT( gtrfrks )
{
	init_konami573( machine );

	memory_install_read32_handler ( 0, ADDRESS_SPACE_PROGRAM, 0x1f600000, 0x1f6000ff, 0, 0, gtrfrks_io_r );
	memory_install_write32_handler( 0, ADDRESS_SPACE_PROGRAM, 0x1f600000, 0x1f6000ff, 0, 0, gtrfrks_io_w );
}

static MACHINE_DRIVER_START( konami573 )
	/* basic machine hardware */
	MDRV_CPU_ADD( PSXCPU, 33868800 / 2 ) /* 33MHz ?? */
	MDRV_CPU_PROGRAM_MAP( konami573_map, 0 )
	MDRV_CPU_VBLANK_INT( sys573_vblank, 1 )

	MDRV_SCREEN_REFRESH_RATE( 60 )
	MDRV_SCREEN_VBLANK_TIME(TIME_IN_USEC( 0 ))

	MDRV_MACHINE_RESET( konami573 )
	MDRV_NVRAM_HANDLER( konami573 )

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES( VIDEO_TYPE_RASTER )
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE( 1024, 1024 )
	MDRV_SCREEN_VISIBLE_AREA( 0, 639, 0, 479 )
	MDRV_PALETTE_LENGTH( 65536 )

	MDRV_PALETTE_INIT( psx )
	MDRV_VIDEO_START( psx_type2 )
	MDRV_VIDEO_UPDATE( psx )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD( PSXSPU, 0 )
	MDRV_SOUND_CONFIG( konami573_psxspu_interface )
	MDRV_SOUND_ROUTE( 0, "left", 1.0 )
	MDRV_SOUND_ROUTE( 1, "right", 1.0 )

	MDRV_SOUND_ADD( CDDA, 0 )
	MDRV_SOUND_ROUTE( 0, "left", 1.0 )
	MDRV_SOUND_ROUTE( 1, "right", 1.0 )
MACHINE_DRIVER_END

INPUT_PORTS_START( konami573 )
	PORT_START_TAG("IN0")
	PORT_BIT( 0xffffffff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN1")
	PORT_DIPNAME( 0x00000001, 0x00000001, "Unused 2" )
	PORT_DIPSETTING(          0x00000001, DEF_STR( Off ) )
	PORT_DIPSETTING(          0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00000002, 0x00000002, "Screen Flip" )
	PORT_DIPSETTING(          0x00000002, DEF_STR( Normal ) )
	PORT_DIPSETTING(          0x00000000, "V-Flip" )
	PORT_DIPNAME( 0x00000004, 0x00000004, "Unused 1")
	PORT_DIPSETTING(          0x00000004, DEF_STR( Off ) )
	PORT_DIPSETTING(          0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00000008, 0x00000008, "Start Up Device" )
	PORT_DIPSETTING(          0x00000008, "CD-ROM Drive" )
	PORT_DIPSETTING(          0x00000000, "Flash ROM" )
	PORT_BIT( 0x000000f0, IP_ACTIVE_HIGH, IPT_SPECIAL ) /* 0xc0 */
	PORT_BIT( 0x00000100, IP_ACTIVE_HIGH, IPT_SPECIAL )
	PORT_BIT( 0x00000200, IP_ACTIVE_HIGH, IPT_SPECIAL )
//  PORT_BIT( 0x00000400, IP_ACTIVE_LOW, IPT_UNKNOWN )
//  PORT_BIT( 0x00000800, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x00001000, 0x00001000, "Network?" )
	PORT_DIPSETTING(          0x00001000, DEF_STR( Off ) )
	PORT_DIPSETTING(          0x00000000, DEF_STR( On ) )
//  PORT_BIT( 0x00002000, IP_ACTIVE_LOW, IPT_UNKNOWN )
//  PORT_BIT( 0x00004000, IP_ACTIVE_LOW, IPT_UNKNOWN )
//  PORT_BIT( 0x00008000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x00010000, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* adc0834 d0 */
//  PORT_BIT( 0x00020000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x00040000, IP_ACTIVE_HIGH, IPT_SPECIAL ) /* x76f041 sda */
//  PORT_BIT( 0x00080000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x00100000, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* skip hang at startup */
	PORT_BIT( 0x00200000, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* skip hang at startup */
//  PORT_BIT( 0x00400000, IP_ACTIVE_LOW, IPT_UNKNOWN )
//  PORT_BIT( 0x00800000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x01000000, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02000000, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04000000, IP_ACTIVE_HIGH, IPT_SPECIAL ) /* PCCARD 1 */
	PORT_BIT( 0x08000000, IP_ACTIVE_HIGH, IPT_SPECIAL ) /* PCCARD 2 */
	PORT_BIT( 0x10000000, IP_ACTIVE_LOW, IPT_SERVICE1 )
//  PORT_BIT( 0x20000000, IP_ACTIVE_LOW, IPT_UNKNOWN )
//  PORT_BIT( 0x40000000, IP_ACTIVE_LOW, IPT_UNKNOWN )
//  PORT_BIT( 0x80000000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN2")
	PORT_BIT( 0xffff0000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x00000100, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x00000200, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x00000400, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x00000800, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x00001000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1) /* skip init? */
	PORT_BIT( 0x00002000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x00004000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x00008000, IP_ACTIVE_LOW, IPT_START1 ) /* skip init? */
	PORT_BIT( 0x00000001, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x00000002, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x00000004, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x00000008, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x00000010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2) /* skip init? */
	PORT_BIT( 0x00000020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x00000040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x00000080, IP_ACTIVE_LOW, IPT_START2 ) /* skip init? */

	PORT_START_TAG("IN3")
	PORT_BIT( 0x00000100, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1)
	PORT_BIT( 0x00000200, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_PLAYER(1)
	PORT_BIT( 0x00000400, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("Test Switch") PORT_CODE(KEYCODE_F2)
	PORT_BIT( 0x00000800, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_PLAYER(1)
	PORT_BIT( 0x0b000000, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* skip e-Amusement error */
//  PORT_BIT( 0xf4fff0ff, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( fbaitbc )
	PORT_INCLUDE( konami573 )

	PORT_MODIFY("IN3")

	PORT_START_TAG( "uPD4701_y" )
	PORT_BIT( 0x0fff, 0, IPT_MOUSE_Y ) PORT_MINMAX( 0, 0xfff ) PORT_SENSITIVITY( 15 ) PORT_KEYDELTA( 8 ) PORT_RESET

	PORT_START_TAG( "uPD4701_switches" )
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNUSED ) PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON7 ) PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_UNUSED ) PORT_PLAYER(1)
INPUT_PORTS_END

INPUT_PORTS_START( fbaitmc )
	PORT_INCLUDE( fbaitbc )

	PORT_START_TAG( "analog0" )
	PORT_BIT( 0xff, 0x80, IPT_AD_STICK_Y ) PORT_MINMAX(0x20,0xdf) PORT_SENSITIVITY(30) PORT_KEYDELTA(30) PORT_PLAYER(1) PORT_REVERSE

	PORT_START_TAG( "analog1" )
	PORT_BIT( 0xff, 0x80, IPT_AD_STICK_X ) PORT_MINMAX(0x20,0xdf) PORT_SENSITIVITY(30) PORT_KEYDELTA(30) PORT_PLAYER(1)
INPUT_PORTS_END

INPUT_PORTS_START( ddr )
	PORT_INCLUDE( konami573 )

	PORT_MODIFY("IN2")
	PORT_BIT( 0x00000100, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_16WAY PORT_PLAYER(1)
	PORT_BIT( 0x00000200, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_16WAY PORT_PLAYER(1) /* serial? */
	PORT_BIT( 0x00000400, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_16WAY PORT_PLAYER(1)    /* serial? */
	PORT_BIT( 0x00000800, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_16WAY PORT_PLAYER(1)
	PORT_BIT( 0x00000001, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_16WAY PORT_PLAYER(2)
	PORT_BIT( 0x00000002, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_16WAY PORT_PLAYER(2) /* serial? */
	PORT_BIT( 0x00000004, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_16WAY PORT_PLAYER(2)    /* serial? */
	PORT_BIT( 0x00000008, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_16WAY PORT_PLAYER(2)
INPUT_PORTS_END

INPUT_PORTS_START( gtrfrks )
	PORT_INCLUDE( konami573 )

	PORT_MODIFY("IN1")
	PORT_BIT( 0x10000000, IP_ACTIVE_LOW, IPT_UNUSED ) /* SERVICE1 */

	PORT_MODIFY("IN2")
	PORT_BIT( 0x00000100, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1) PORT_NAME("P1 Effect 1")
	PORT_BIT( 0x00000200, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_PLAYER(1) PORT_NAME("P1 Effect 2")
	PORT_BIT( 0x00000400, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_PLAYER(1) PORT_NAME("P1 Pick")
	PORT_BIT( 0x00000800, IP_ACTIVE_LOW, IPT_BUTTON7 ) PORT_PLAYER(1) PORT_NAME("P1 Wailing")
	PORT_BIT( 0x00001000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1) PORT_NAME("P1 Button R")
	PORT_BIT( 0x00002000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1) PORT_NAME("P1 Button G")
	PORT_BIT( 0x00004000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1) PORT_NAME("P1 Button B")
	PORT_BIT( 0x00008000, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x00000001, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2) PORT_NAME("P2 Effect 1")
	PORT_BIT( 0x00000002, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_PLAYER(2) PORT_NAME("P2 Effect 2")
	PORT_BIT( 0x00000004, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_PLAYER(2) PORT_NAME("P2 Pick")
	PORT_BIT( 0x00000008, IP_ACTIVE_LOW, IPT_BUTTON7 ) PORT_PLAYER(2) PORT_NAME("P2 Wailing")
	PORT_BIT( 0x00000010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2) PORT_NAME("P2 Button R")
	PORT_BIT( 0x00000020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2) PORT_NAME("P2 Button G")
	PORT_BIT( 0x00000040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2) PORT_NAME("P2 Button B")
	PORT_BIT( 0x00000080, IP_ACTIVE_LOW, IPT_START2 )

	PORT_MODIFY("IN3")
	PORT_BIT( 0x00000100, IP_ACTIVE_LOW, IPT_SERVICE1 ) /* P1 BUTTON4 */
	PORT_BIT( 0x00000200, IP_ACTIVE_LOW, IPT_UNUSED ) /* P1 BUTTON5 */
	PORT_BIT( 0x00000800, IP_ACTIVE_LOW, IPT_UNUSED ) /* P1 BUTTON6 */
INPUT_PORTS_END

#define SYS573_BIOS_A ROM_LOAD( "700a01.bin",   0x0000000, 0x080000, CRC(11812ef8) )

// BIOS
ROM_START( sys573 )
	ROM_REGION32_LE( 0x080000, REGION_USER1, 0 )
	SYS573_BIOS_A

	ROM_REGION( 0x0000224, REGION_USER2, 0 ) /* security cart */
	ROM_FILL( 0x0000000, 0x0000224, 0x00 )

	ROM_REGION( 0x1000000, REGION_USER3, 0 ) /* onboard flash */
	ROM_FILL( 0x0000000, 0x1000000, 0xff )
ROM_END

// Games
ROM_START( bassangl )
	ROM_REGION32_LE( 0x080000, REGION_USER1, 0 )
	SYS573_BIOS_A

	ROM_REGION( 0x0000224, REGION_USER2, 0 ) /* security cart */
	ROM_LOAD( "ge765ja.u1", 0x000000, 0x000224, BAD_DUMP CRC(ee1b32a7) SHA1(c0f6b14b054f5a95ce474e794a3e0ca78faac681) )

	ROM_REGION( 0x1000000, REGION_USER3, 0 ) /* onboard flash */
	ROM_FILL( 0x0000000, 0x1000000, 0xff )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "765jaa02", 0, MD5(11693b1234458c238ed613ef37f71245) SHA1(d820f8166b7d5ffcf41e7a70c8c4c4d1c207c1bd) )
ROM_END

ROM_START( darkhleg )
	ROM_REGION32_LE( 0x080000, REGION_USER1, 0 )
	SYS573_BIOS_A

	ROM_REGION( 0x0000224, REGION_USER2, 0 ) /* security cart */
	ROM_LOAD( "gx706ja.u1", 0x000000, 0x000224, BAD_DUMP CRC(72b42574) SHA1(79dc959f0ce95ccb9ac0dbf0a72aec973e91bc56) )

	ROM_REGION( 0x1000000, REGION_USER3, 0 ) /* onboard flash */
	ROM_FILL( 0x0000000, 0x1000000, 0xff )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "706jaa02", 0, MD5(4f096051df039b0d104d4c0fff5dadb8) SHA1(4c8d976096c2da6d01804a44957daf9b50103c90) )
ROM_END

ROM_START( dstage )
	ROM_REGION32_LE( 0x080000, REGION_USER1, 0 )
	ROM_LOAD( "700a01.bin",   0x0000000, 0x080000, CRC(11812ef8) )

	ROM_REGION( 0x0000224, REGION_USER2, 0 ) /* security cart */
	ROM_LOAD( "gn845ea.u1",   0x000000, 0x000224, BAD_DUMP CRC(db643af7) SHA1(881221da640b883302e657b906ea0a4e74555679) )

	ROM_REGION( 0x1000000, REGION_USER3, 0 ) /* onboard flash */
	ROM_FILL( 0x0000000, 0x1000000, 0xff )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "845ea", 0, BAD_DUMP MD5(32d52ee2b37559d7413788c87085f37c) SHA1(e82610e1a34fba144499f9ee892ac882d1e96853) )
ROM_END

ROM_START( ddru )
	ROM_REGION32_LE( 0x080000, REGION_USER1, 0 )
	ROM_LOAD( "700a01.bin",   0x0000000, 0x080000, CRC(11812ef8) )

	ROM_REGION( 0x0000224, REGION_USER2, 0 ) /* security cart */
	ROM_LOAD( "gn845ua.u1",   0x000000, 0x000224, BAD_DUMP CRC(c9e7fced) SHA1(aac4dde100091bc64d397f53484a0ffbf68b8101) )

	ROM_REGION( 0x1000000, REGION_USER3, 0 ) /* onboard flash */
	ROM_FILL( 0x0000000, 0x1000000, 0xff )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "845uaa02", 0, MD5(32d52ee2b37559d7413788c87085f37c) SHA1(e82610e1a34fba144499f9ee892ac882d1e96853) )
ROM_END

ROM_START( ddrj )
	ROM_REGION32_LE( 0x080000, REGION_USER1, 0 )
	ROM_LOAD( "700a01.bin",   0x0000000, 0x080000, CRC(11812ef8) )

	ROM_REGION( 0x0000224, REGION_USER2, 0 ) /* security cart */
	ROM_LOAD( "gc845jb.u1",   0x000000, 0x000224, BAD_DUMP CRC(a16f42b8) SHA1(da4f1eb3eb2b28cb3a0bc74bb9b9945970f56ac2) )

	ROM_REGION( 0x1000000, REGION_USER3, 0 ) /* onboard flash */
	ROM_FILL( 0x0000000, 0x1000000, 0xff )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "845jba02", 0, MD5(314f64301c3429312770ecdeb975d285) SHA1(8ebd9a68bbea3d9947a95d896347b0fea2145e4a) )
ROM_END

ROM_START( ddra )
	ROM_REGION32_LE( 0x080000, REGION_USER1, 0 )
	ROM_LOAD( "700a01.bin",   0x0000000, 0x080000, CRC(11812ef8) )

	ROM_REGION( 0x0000224, REGION_USER2, 0 ) /* security cart */
	ROM_LOAD( "gn845aa.u1",   0x000000, 0x000224, BAD_DUMP CRC(327c4851) SHA1(f0939224af706fd103a67aae9c96518c1db90ac9) )

	ROM_REGION( 0x1000000, REGION_USER3, 0 ) /* onboard flash */
	ROM_FILL( 0x0000000, 0x1000000, 0xff )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "845aaa02", 0, MD5(2ab58fc647d35673861788a78df2afba) SHA1(fe2d18cdab7a3088f7c876ce531d64a2f3ae9294) )
ROM_END

ROM_START( ddr2nd )
	ROM_REGION32_LE( 0x080000, REGION_USER1, 0 )
	ROM_LOAD( "700a01.bin",   0x0000000, 0x080000, CRC(11812ef8) )

	ROM_REGION( 0x0000224, REGION_USER2, 0 ) /* security cart */
	ROM_LOAD( "gn895jaa.u1",  0x000000, 0x000224, BAD_DUMP CRC(363f427e) SHA1(adec886a07b9bd91f142f286b04fc6582205f037) )

	ROM_REGION( 0x1000000, REGION_USER3, 0 ) /* onboard flash */
	ROM_FILL( 0x0000000, 0x1000000, 0xff )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "895ja", 0, BAD_DUMP MD5(2013c3aac0d7fdbae10eadf1aaa8a174) SHA1(03e64fc507d42d3fd6fec288893ec065e4313b00) )
ROM_END

ROM_START( ddr2ndl )
	ROM_REGION32_LE( 0x080000, REGION_USER1, 0 )
	ROM_LOAD( "700a01.bin",   0x0000000, 0x080000, CRC(11812ef8) )

	ROM_REGION( 0x0000224, REGION_USER2, 0 ) /* security cart */
	ROM_LOAD( "ge885jaa.u1",  0x000000, 0x000224, BAD_DUMP CRC(cbc984c5) SHA1(6c0cd78a41000999b4ffbd9fb3707738b50a9b50) )

	ROM_REGION( 0x1000000, REGION_USER3, 0 ) /* onboard flash */
	ROM_FILL( 0x0000000, 0x1000000, 0xff )

	ROM_REGION( 0x2000000, REGION_USER4, 0 ) /* PCCARD1 */
	ROM_FILL( 0x0000000, 0x2000000, 0xff )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "885jaa02", 0, MD5(696e39fa7113f61181875bffca13a1b4) SHA1(ece1d34a3bdbe07b608429abe30802bc7327a94a) )
ROM_END

ROM_START( fbait2bc )
	ROM_REGION32_LE( 0x080000, REGION_USER1, 0 )
	SYS573_BIOS_A

	ROM_REGION( 0x0000224, REGION_USER2, 0 ) /* security cart */
	ROM_LOAD( "gc865ua.u1", 0x000000, 0x000224, BAD_DUMP CRC(ea8f0b4b) SHA1(363b1ea1a520b239ba8bca867366bbe8a9977a43) )

	ROM_REGION( 0x1000000, REGION_USER3, 0 ) /* onboard flash */
	ROM_FILL( 0x0000000, 0x1000000, 0xff )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "865uab02", 0, MD5(5a253a58417539f9b0cb9726311f73d5) SHA1(86ccbaac30e9e2d7d0ad6ae65a4f53f606f50525) )
ROM_END

ROM_START( fbaitbc )
	ROM_REGION32_LE( 0x080000, REGION_USER1, 0 )
	SYS573_BIOS_A

	ROM_REGION( 0x0000224, REGION_USER2, 0 ) /* security cart */
	ROM_LOAD( "ge765ua.u1", 0x000000, 0x000224, BAD_DUMP CRC(588748c6) SHA1(ea1ead61e0dcb324ef7b6106cae00bcf6702d6c4) )

	ROM_REGION( 0x1000000, REGION_USER3, 0 ) /* onboard flash */
	ROM_FILL( 0x0000000, 0x1000000, 0xff )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "765uab02", 0, MD5(56d8a23bb592932631f8f81b9797fce6) SHA1(dda131b8655e3c4394e50749fe3a1e468f9df353) )
ROM_END

ROM_START( fbaitmc )
	ROM_REGION32_LE( 0x080000, REGION_USER1, 0 )
	SYS573_BIOS_A

	ROM_REGION( 0x0000224, REGION_USER2, 0 ) /* security cart */
	ROM_LOAD( "gx889ea.u1", 0x000000, 0x000224, BAD_DUMP CRC(753ad84e) SHA1(e024cefaaee7c9945ccc1f9a3d896b8560adce2e) )

	ROM_REGION( 0x1000000, REGION_USER3, 0 ) /* onboard flash */
	ROM_FILL( 0x0000000, 0x1000000, 0xff )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "889ea", 0, BAD_DUMP MD5(43eae52edd38019f0836897ea8def527) SHA1(2e6937c265c222ac2cea50fbf32201ade425ee30) )
ROM_END

ROM_START( fbaitmca )
	ROM_REGION32_LE( 0x080000, REGION_USER1, 0 )
	SYS573_BIOS_A

	ROM_REGION( 0x0000224, REGION_USER2, 0 ) /* security cart */
	ROM_LOAD( "gx889aa.u1", 0x000000, 0x000224, BAD_DUMP CRC(9c22aae8) SHA1(c107b0bf7fa76708f2d4f6aaf2cf27b3858378a3) )

	ROM_REGION( 0x1000000, REGION_USER3, 0 ) /* onboard flash */
	ROM_FILL( 0x0000000, 0x1000000, 0xff )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "889aa", 0, BAD_DUMP MD5(43eae52edd38019f0836897ea8def527) SHA1(2e6937c265c222ac2cea50fbf32201ade425ee30) )
ROM_END

ROM_START( fbaitmcj )
	ROM_REGION32_LE( 0x080000, REGION_USER1, 0 )
	SYS573_BIOS_A

	ROM_REGION( 0x0000224, REGION_USER2, 0 ) /* security cart */
	ROM_LOAD( "gx889ja.u1", 0x000000, 0x000224, BAD_DUMP CRC(6278603c) SHA1(d6b59e270cfe4016e12565aedec8a4f0702e1a6f) )

	ROM_REGION( 0x1000000, REGION_USER3, 0 ) /* onboard flash */
	ROM_FILL( 0x0000000, 0x1000000, 0xff )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "889ja", 0, BAD_DUMP MD5(43eae52edd38019f0836897ea8def527) SHA1(2e6937c265c222ac2cea50fbf32201ade425ee30) )
ROM_END

ROM_START( fbaitmcu )
	ROM_REGION32_LE( 0x080000, REGION_USER1, 0 )
	SYS573_BIOS_A

	ROM_REGION( 0x0000224, REGION_USER2, 0 ) /* security cart */
	ROM_LOAD( "gx889ua.u1", 0x000000, 0x000224, BAD_DUMP CRC(67b91e54) SHA1(4d94bfab08e2bf6e34ee606dd3c4e345d8e5d158) )

	ROM_REGION( 0x1000000, REGION_USER3, 0 ) /* onboard flash */
	ROM_FILL( 0x0000000, 0x1000000, 0xff )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "889ua", 0, BAD_DUMP MD5(43eae52edd38019f0836897ea8def527) SHA1(2e6937c265c222ac2cea50fbf32201ade425ee30) )
ROM_END

ROM_START( gtrfrks )
	ROM_REGION32_LE( 0x080000, REGION_USER1, 0 )
	SYS573_BIOS_A

	ROM_REGION( 0x0000224, REGION_USER2, 0 ) /* security cart */
	ROM_LOAD( "gq886eac.u1",  0x000000, 0x000224, BAD_DUMP CRC(06bd6c4f) SHA1(61930e467ad135e2f31393ff5af981ed52f3bef9) )

	ROM_REGION( 0x1000000, REGION_USER3, 0 ) /* onboard flash */
	ROM_FILL( 0x0000000, 0x1000000, 0xff )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "886ea", 0, BAD_DUMP MD5(b8b39a6e48867fdad640bd256273bdfc) SHA1(2277c6268b8327be8d7636d4812920e5d3b353cd) )
ROM_END

ROM_START( gtrfrksu )
	ROM_REGION32_LE( 0x080000, REGION_USER1, 0 )
	SYS573_BIOS_A

	ROM_REGION( 0x0000224, REGION_USER2, 0 ) /* security cart */
	ROM_LOAD( "gq886uac.u1",  0x000000, 0x000224, BAD_DUMP CRC(143eaa55) SHA1(51a4fa3693f1cb1646a8986003f9b6cc1ae8b630) )

	ROM_REGION( 0x1000000, REGION_USER3, 0 ) /* onboard flash */
	ROM_FILL( 0x0000000, 0x1000000, 0xff )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "886ua", 0, BAD_DUMP MD5(b8b39a6e48867fdad640bd256273bdfc) SHA1(2277c6268b8327be8d7636d4812920e5d3b353cd) )
ROM_END

ROM_START( gtrfrksj )
	ROM_REGION32_LE( 0x080000, REGION_USER1, 0 )
	SYS573_BIOS_A

	ROM_REGION( 0x0000224, REGION_USER2, 0 ) /* security cart */
	ROM_LOAD( "gq886jac.u1",  0x000000, 0x000224, BAD_DUMP CRC(11ffd43d) SHA1(27f4f4d782604379254fb98c3c57e547aa4b321f) )

	ROM_REGION( 0x1000000, REGION_USER3, 0 ) /* onboard flash */
	ROM_FILL( 0x0000000, 0x1000000, 0xff )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "886ja", 0, BAD_DUMP MD5(b8b39a6e48867fdad640bd256273bdfc) SHA1(2277c6268b8327be8d7636d4812920e5d3b353cd) )
ROM_END

ROM_START( gtrfrksa )
	ROM_REGION32_LE( 0x080000, REGION_USER1, 0 )
	SYS573_BIOS_A

	ROM_REGION( 0x0000224, REGION_USER2, 0 ) /* security cart */
	ROM_LOAD( "gq886aac.u1",  0x000000, 0x000224, BAD_DUMP CRC(efa51ee9) SHA1(3374d936de69c287e0161bc526546441c2943555) )

	ROM_REGION( 0x1000000, REGION_USER3, 0 ) /* onboard flash */
	ROM_FILL( 0x0000000, 0x1000000, 0xff )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "886aa", 0, BAD_DUMP MD5(b8b39a6e48867fdad640bd256273bdfc) SHA1(2277c6268b8327be8d7636d4812920e5d3b353cd) )
ROM_END

ROM_START( konam80a )
	ROM_REGION32_LE( 0x080000, REGION_USER1, 0 )
	SYS573_BIOS_A

	ROM_REGION( 0x0000224, REGION_USER2, 0 ) /* security cart */
	ROM_LOAD( "gc826aa.u1", 0x000000, 0x000224, BAD_DUMP CRC(9b38b959) SHA1(6b4fca340a9b1c2ae21ad3903c1ac1e39ab08b1a) )

	ROM_REGION( 0x1000000, REGION_USER3, 0 ) /* onboard flash */
	ROM_FILL( 0x0000000, 0x1000000, 0xff )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "826aaa01", 0, BAD_DUMP MD5(456f683c5d47dd73cfb73ce80b8a7351) SHA1(452c94088ffefe42e61c978b48d425e7094a5af6) )
ROM_END

ROM_START( konam80j )
	ROM_REGION32_LE( 0x080000, REGION_USER1, 0 )
	SYS573_BIOS_A

	ROM_REGION( 0x0000224, REGION_USER2, 0 ) /* security cart */
	ROM_LOAD( "gc826ja.u1", 0x000000, 0x000224, BAD_DUMP CRC(e9e861e8) SHA1(45841db0b42d096213d9539a8d076d39391dca6d) )

	ROM_REGION( 0x1000000, REGION_USER3, 0 ) /* onboard flash */
	ROM_FILL( 0x0000000, 0x1000000, 0xff )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "826jaa01", 0, MD5(456f683c5d47dd73cfb73ce80b8a7351) SHA1(452c94088ffefe42e61c978b48d425e7094a5af6) )
ROM_END

ROM_START( konam80k )
	ROM_REGION32_LE( 0x080000, REGION_USER1, 0 )
	SYS573_BIOS_A

	ROM_REGION( 0x0000224, REGION_USER2, 0 ) /* security cart */
	ROM_LOAD( "gc826ka.u1", 0x000000, 0x000224, BAD_DUMP CRC(d41f7e38) SHA1(73e2bb132e23be72e72ea5b0686ccad28e47574a) )

	ROM_REGION( 0x1000000, REGION_USER3, 0 ) /* onboard flash */
	ROM_FILL( 0x0000000, 0x1000000, 0xff )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "826kaa01", 0, BAD_DUMP MD5(456f683c5d47dd73cfb73ce80b8a7351) SHA1(452c94088ffefe42e61c978b48d425e7094a5af6) )
ROM_END

ROM_START( konam80s )
	ROM_REGION32_LE( 0x080000, REGION_USER1, 0 )
	SYS573_BIOS_A

	ROM_REGION( 0x0000224, REGION_USER2, 0 ) /* security cart */
	ROM_LOAD( "gc826ea.u1", 0x000000, 0x000224, BAD_DUMP CRC(6ce4c619) SHA1(d2be08c213c0a74e30b7ebdd93946374cc64457f) )

	ROM_REGION( 0x1000000, REGION_USER3, 0 ) /* onboard flash */
	ROM_FILL( 0x0000000, 0x1000000, 0xff )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "826eaa01", 0, BAD_DUMP MD5(456f683c5d47dd73cfb73ce80b8a7351) SHA1(452c94088ffefe42e61c978b48d425e7094a5af6) )
ROM_END

ROM_START( konam80u )
	ROM_REGION32_LE( 0x080000, REGION_USER1, 0 )
	SYS573_BIOS_A

	ROM_REGION( 0x0000224, REGION_USER2, 0 ) /* security cart */
	ROM_LOAD( "gc826ua.u1", 0x000000, 0x000224, BAD_DUMP CRC(0577379b) SHA1(3988a2a5ef1f1d5981c4767cbed05b73351be903) )

	ROM_REGION( 0x1000000, REGION_USER3, 0 ) /* onboard flash */
	ROM_FILL( 0x0000000, 0x1000000, 0xff )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "826uaa01", 0, MD5(456f683c5d47dd73cfb73ce80b8a7351) SHA1(452c94088ffefe42e61c978b48d425e7094a5af6) )
ROM_END

ROM_START( pbballex )
	ROM_REGION32_LE( 0x080000, REGION_USER1, 0 )
	SYS573_BIOS_A

	ROM_REGION( 0x0000224, REGION_USER2, 0 ) /* security cart */
	ROM_LOAD( "gx802ja.u1", 0x000000, 0x000224, BAD_DUMP CRC(ea8bdda3) SHA1(780034ab08871631ef0e3e9b779ca89e016c26a8) )

	ROM_REGION( 0x1000000, REGION_USER3, 0 ) /* onboard flash */
	ROM_FILL( 0x0000000, 0x1000000, 0xff )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "802jab02", 0, MD5(52bb53327ba48f87dcb030d5e50fe94f) SHA1(67ddce1ad7e436c18e08d5a8c77f3259dbf30572) )
ROM_END

// System 573 BIOS (we're missing the later version that boots up with a pseudo-GUI)
GAME( 1998, sys573, 0, konami573, konami573, konami573, ROT0, "Konami", "System 573 BIOS", NOT_A_DRIVER )

// Standard System 573 games
GAME( 1998, darkhleg, sys573,   konami573, konami573, konami573,  ROT0, "Konami", "Dark Horse Legend (GX706 VER. JAA)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1998, fbaitbc,  sys573,   konami573, fbaitbc,   ge765pwbba, ROT0, "Konami", "Fisherman's Bait - A Bass Challenge (GE765 VER. UAB)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1998, bassangl, fbaitbc,  konami573, fbaitbc,   ge765pwbba, ROT0, "Konami", "Bass Angler (GE765 VER. JAA)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1998, pbballex, sys573,   konami573, konami573, konami573,  ROT0, "Konami", "Powerful Pro Baseball EX (GX802 VER. JAB)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1998, konam80s, sys573,   konami573, konami573, konami573,  ROT90, "Konami", "Konami 80's AC Special (GC826 VER. EAA)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1998, konam80u, konam80s, konami573, konami573, konami573,  ROT90, "Konami", "Konami 80's AC Special (GC826 VER. UAA)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1998, konam80j, konam80s, konami573, konami573, konami573,  ROT90, "Konami", "Konami 80's Gallery (GC826 VER. JAA)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1998, konam80a, konam80s, konami573, konami573, konami573,  ROT90, "Konami", "Konami 80's AC Special (GC826 VER. AAA)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1998, konam80k, konam80s, konami573, konami573, konami573,  ROT90, "Konami", "Konami 80's AC Special (GC826 VER. KAA)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1999, dstage,   sys573,   konami573, ddr,       ddr,        ROT0, "Konami", "Dancing Stage (GN845 VER. EAA)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1999, ddru,     dstage,   konami573, ddr,       ddr,        ROT0, "Konami", "Dance Dance Revolution (GN845 VER. UAA)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1998, ddrj,     dstage,   konami573, ddr,       ddr,        ROT0, "Konami", "Dance Dance Revolution (GC845 VER. JBA)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1999, ddra,     dstage,   konami573, ddr,       ddr,        ROT0, "Konami", "Dance Dance Revolution (GN845 VER. AAA)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1998, fbait2bc, sys573,   konami573, fbaitbc,   ge765pwbba, ROT0, "Konami", "Fisherman's Bait 2 - A Bass Challenge (GE865 VER. UAB)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1999, ddr2ndl,  sys573,   konami573, ddr,       ddr,        ROT0, "Konami", "Dance Dance Revolution 2nd Mix Link (GE885 VER. JAA)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1999, gtrfrks,  sys573,   konami573, gtrfrks,   gtrfrks,    ROT0, "Konami", "Guitar Freaks (GQ886 VER. EAC)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1999, gtrfrksu, gtrfrks,  konami573, gtrfrks,   gtrfrks,    ROT0, "Konami", "Guitar Freaks (GQ886 VER. UAC)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1999, gtrfrksj, gtrfrks,  konami573, gtrfrks,   gtrfrks,    ROT0, "Konami", "Guitar Freaks (GQ886 VER. JAC)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1999, gtrfrksa, gtrfrks,  konami573, gtrfrks,   gtrfrks,    ROT0, "Konami", "Guitar Freaks (GQ886 VER. AAC)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1999, fbaitmc,  sys573,   konami573, fbaitmc,   ge765pwbba, ROT0, "Konami", "Fisherman's Bait - Marlin Challenge (GX889 VER. EA)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1999, fbaitmcu, fbaitmc,  konami573, fbaitmc,   ge765pwbba, ROT0, "Konami", "Fisherman's Bait - Marlin Challenge (GX889 VER. UA)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1999, fbaitmcj, fbaitmc,  konami573, fbaitmc,   ge765pwbba, ROT0, "Konami", "Fisherman's Bait - Marlin Challenge (GX889 VER. JA)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1999, fbaitmca, fbaitmc,  konami573, fbaitmc,   ge765pwbba, ROT0, "Konami", "Fisherman's Bait - Marlin Challenge (GX889 VER. AA)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1999, ddr2nd,   sys573,   konami573, ddr,       ddr,        ROT0, "Konami", "Dance Dance Revolution 2nd Mix (GN895 VER. JAA)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )

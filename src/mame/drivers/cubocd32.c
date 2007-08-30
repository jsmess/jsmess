/* Cubo CD32 (Original hardware by Commodore, additional hardware and games by CD Express, Milan, Italy)

   This is basically a CD32 (Amiga 68020, AGA based games system) hacked up to run arcade games.
   see http://ninjaw.ifrance.com/cd32/cubo/ for a brief overview.

   Several of the games have Audio tracks, therefore the CRC / SHA1 information you get when
   reading your own CDs may not match those in the driver.  There is currently no 100% accurate
   way to rip the audio data with full sub-track and offset information.

   CD32 Hardware Specs (from Wikipedia, http://en.wikipedia.org/wiki/Amiga_CD32)
    * Main Processor: Motorola 68EC020 at 14.3 MHz
    * System Memory: 2 MB Chip RAM
    * 1 MB ROM with Kickstart ROM 3.1 and integrated cdfs.filesystem
    * 1KB of FlashROM for game saves
    * Graphics/Chipset: AGA Chipset
    * Akiko chip, which handles CD-ROM and can do Chunky to Planar conversion
    * Proprietary (MKE) CD-ROM drive at 2x speed
    * Expansion socket for MPEG cartridge, as well as 3rd party devices such as the SX-1 and SX32 expansion packs.
    * 4 8-bit audio channels (2 for left, 2 for right)
    * Gamepad, Serial port, 2 Gameports, Interfaces for keyboard


   ToDo:

   Everything - This is a skeleton driver.
      Add full AGA (68020 based systems, Amiga 1200 / CD32) support to Amiga driver
      Add CD Controller emulation for CD32.
      ... work from there


   Known Games:
   Title                | rev. | year
   ----------------------------------------------
   Candy Puzzle         |  1.0 | 1995
   Harem Challenge      |      | 1995
   Laser Quiz           |      | 1995
   Laser Quiz 2 "Italy" |  1.0 | 1995
   Laser Strixx         |      | 1995
   Magic Premium        |  1.1 | 1996
   Laser Quiz France    |  1.0 | 1995
   Odeon Twister        |      | 199x
   Odeon Twister 2      |202.19| 1999


*/

#include "driver.h"
#include "sound/custom.h"
#include "includes/amiga.h"
#include "cdrom.h"
#include "sound/cdda.h"

/*

    Akiko custom chip emulation

The Akiko chip has:
- built in 1KB NVRAM
- chunky to planar converter
- custom CDROM controller

CDROM Commands:
    len does not include checksum byte,
    resp len does include checksum byte

    x0: len = 0x01, resp len = 0x01
    x1: len = 0x02, resp len = 0x03
    x2: len = 0x01, resp len = 0x03 - Pause CD Audio
    x3: len = 0x01, resp len = 0x03 - Unpause CD Audio
    x4: len = 0x0C, resp len = 0x03 - CD Audio Play/Read CD Data/Seek
    x5: len = 0x02, resp len = 0x10 - Read TOC
    x6: len = 0x01, resp len = 0x10 - Get Next TOC entry
    x7: len = 0x01, resp len = 0x15 - Door Status
    x8: len = 0x04, resp len = 0x03
    x9: len = 0x01, resp len = 0x01
    xA: len = 0x00, resp len = 0x03 - Media Status: Response = 0A NN CK where NN = 0: no disk and NN = 1: disk inserted
    xB: len = 0x00, resp len = 0x01
    xC: len = 0x00, resp len = 0x01
    xD: len = 0x00, resp len = 0x01
    xE: len = 0x00, resp len = 0x01
    xF: len = 0x00, resp len = 0x01
*/

#define LOG_AKIKO		0
#define LOG_AKIKO_I2C	0

enum {
	I2C_WAIT = 0,
	I2C_DEVICEADDR,
	I2C_WORDADDR,
	I2C_DATA
};

#define	NVRAM_SIZE 1024
/* max size of one write request */
#define	NVRAM_PAGE_SIZE	16

struct akiko_def
{
	/* chunky to planar converter */
	UINT32	c2p_input_buffer[8];
	UINT32	c2p_output_buffer[8];
	UINT32	c2p_input_index;
	UINT32	c2p_output_index;

	/* i2c bus */
	int		i2c_state;
	int		i2c_bitcounter;
	int		i2c_direction;
	int		i2c_sda_dir_nvram;
	int		i2c_scl_out;
	int		i2c_scl_in;
	int		i2c_scl_dir;
	int		i2c_oscl;
	int		i2c_sda_out;
	int		i2c_sda_in;
	int		i2c_sda_dir;
	int		i2c_osda;

	/* nvram */
	UINT8	nvram[NVRAM_SIZE];
	UINT8	nvram_writetmp[NVRAM_PAGE_SIZE];
	int		nvram_address;
	int		nvram_writeaddr;
	int		nvram_rw;
	UINT8	nvram_byte;

	/* cdrom */
	UINT32	cdrom_status[2];
	UINT32	cdrom_address[2];
	UINT8	cdrom_cmd_start;
	UINT8	cdrom_cmd_end;
	UINT8	cdrom_cmd_resp;
	cdrom_file *cdrom;
} akiko;

void init_akiko(void)
{
	akiko.c2p_input_index = 0;
	akiko.c2p_output_index = 0;

	akiko.i2c_state = I2C_WAIT;
	akiko.i2c_bitcounter = -1;
	akiko.i2c_direction = -1;
	akiko.i2c_sda_dir_nvram = 0;
	akiko.i2c_scl_out = 0;
	akiko.i2c_scl_in = 0;
	akiko.i2c_scl_dir = 0;
	akiko.i2c_oscl = 0;
	akiko.i2c_sda_out = 0;
	akiko.i2c_sda_in = 0;
	akiko.i2c_sda_dir = 0;
	akiko.i2c_osda = 0;

	akiko.nvram_address = 0;
	akiko.nvram_writeaddr = 0;
	akiko.nvram_rw = 0;
	akiko.nvram_byte = 0;
	memset( akiko.nvram, 0, NVRAM_SIZE );

	akiko.cdrom_status[0] = akiko.cdrom_status[1] = 0;
	akiko.cdrom_address[0] = akiko.cdrom_address[1] = 0;
	akiko.cdrom_cmd_start = akiko.cdrom_cmd_end = akiko.cdrom_cmd_resp = 0;
	akiko.cdrom = cdrom_open(get_disk_handle(0));
}

void akiko_c2p_write(UINT32 data)
{
	akiko.c2p_input_buffer[akiko.c2p_input_index] = data;
	akiko.c2p_input_index++;
	akiko.c2p_input_index &= 7;
	akiko.c2p_output_index = 0;
}

UINT32 akiko_c2p_read(void)
{
	UINT32 val;

	if ( akiko.c2p_output_index == 0 )
	{
		int i;

		for ( i = 0; i < 8; i++ )
			akiko.c2p_output_buffer[i] = 0;

		for (i = 0; i < 8 * 32; i++) {
			if (akiko.c2p_input_buffer[7 - (i >> 5)] & (1 << (i & 31)))
				akiko.c2p_output_buffer[i & 7] |= 1 << (i >> 3);
		}
	}
	akiko.c2p_input_index = 0;
	val = akiko.c2p_output_buffer[akiko.c2p_output_index];
	akiko.c2p_output_index++;
	akiko.c2p_output_index &= 7;
	return val;
}

static void akiko_i2c( void )
{
    akiko.i2c_sda_in = 1;

    if ( !akiko.i2c_sda_dir_nvram && akiko.i2c_scl_out && akiko.i2c_oscl )
    {
		if ( !akiko.i2c_sda_out && akiko.i2c_osda )	/* START-condition? */
		{
	    	akiko.i2c_state = I2C_DEVICEADDR;
	    	akiko.i2c_bitcounter = 0;
	    	akiko.i2c_direction = -1;
#if LOG_AKIKO_I2C
	    	logerror("START\n");
#endif
	    	return;
		}
		else if ( akiko.i2c_sda_out && !akiko.i2c_osda ) /* STOP-condition? */
		{
	    	akiko.i2c_state = I2C_WAIT;
	    	akiko.i2c_bitcounter = -1;
#if LOG_AKIKO_I2C
	    	logerror("STOP\n");
#endif
	    	if ( akiko.i2c_direction > 0 )
	    	{
				memcpy( akiko.nvram + ( akiko.nvram_address & ~(NVRAM_PAGE_SIZE - 1) ), akiko.nvram_writetmp, NVRAM_PAGE_SIZE );
				akiko.i2c_direction = -1;
#if LOG_AKIKO_I2C
				{
					int i;

					logerror("NVRAM write address %04X:", akiko.nvram_address & ~(NVRAM_PAGE_SIZE - 1));
					for (i = 0; i < NVRAM_PAGE_SIZE; i++)
		    			logerror("%02X", akiko.nvram_writetmp[i]);
					logerror("\n");
				}
#endif
	    	}
	    	return;
		}
    }

    if ( akiko.i2c_bitcounter >= 0 )
    {
		if ( akiko.i2c_direction )
		{
	    	/* Amiga -> NVRAM */
	    	if ( akiko.i2c_scl_out && !akiko.i2c_oscl )
	    	{
				if ( akiko.i2c_bitcounter == 8 )
				{
				    akiko.i2c_sda_in = 0; /* ACK */

				    if ( akiko.i2c_direction > 0 )
				    {
						akiko.nvram_writetmp[akiko.nvram_writeaddr++] = akiko.nvram_byte;
						akiko.nvram_writeaddr &= (NVRAM_PAGE_SIZE - 1);
						akiko.i2c_bitcounter = 0;
		    		}
		    		else
		    		{
		    			akiko.i2c_bitcounter = -1;
		    		}
		    	}
		    	else
		    	{
				    akiko.nvram_byte <<= 1;
				    akiko.nvram_byte |= akiko.i2c_sda_out;
				    akiko.i2c_bitcounter++;
				}
	    	}
		}
		else
		{
			/* NVRAM -> Amiga */
			if ( akiko.i2c_scl_out && !akiko.i2c_oscl && akiko.i2c_bitcounter < 8 )
			{
				if ( akiko.i2c_bitcounter == 0 )
					akiko.nvram_byte = akiko.nvram[akiko.nvram_address];

				akiko.i2c_sda_dir_nvram = 1;
				akiko.i2c_sda_in = (akiko.nvram_byte & 0x80) ? 1 : 0;
				akiko.nvram_byte <<= 1;
				akiko.i2c_bitcounter++;

				if ( akiko.i2c_bitcounter == 8 )
				{
#if LOG_AKIKO_I2C
		    		logerror("NVRAM sent byte %02X address %04X\n", akiko.nvram[akiko.nvram_address], akiko.nvram_address);
#endif
		    		akiko.nvram_address++;
		    		akiko.nvram_address &= NVRAM_SIZE - 1;
		    		akiko.i2c_sda_dir_nvram = 0;
				}
	    	}

	    	if ( !akiko.i2c_sda_out && akiko.i2c_sda_dir && !akiko.i2c_scl_out ) /* ACK from Amiga */
				akiko.i2c_bitcounter = 0;
		}

		if ( akiko.i2c_bitcounter >= 0 )
			return;
    }

    switch( akiko.i2c_state )
    {
		case I2C_DEVICEADDR:
			if ( ( akiko.nvram_byte & 0xf0 ) != 0xa0 )
			{
#if LOG_AKIKO_I2C
				logerror("WARNING: I2C_DEVICEADDR: device address != 0xA0\n");
#endif
				akiko.i2c_state = I2C_WAIT;
	    		return;
			}

			akiko.nvram_rw = (akiko.nvram_byte & 1) ? 0 : 1;

			if ( akiko.nvram_rw )
			{
				/* 2 high address bits, only fetched if WRITE = 1 */
				akiko.nvram_address &= 0xff;
				akiko.nvram_address |= ((akiko.nvram_byte >> 1) & 3) << 8;
	    		akiko.i2c_state = I2C_WORDADDR;
	    		akiko.i2c_direction = -1;
	    	}
	    	else
	    	{
	    		akiko.i2c_state = I2C_DATA;
	    		akiko.i2c_direction = 0;
	    		akiko.i2c_sda_dir_nvram = 1;
			}

			akiko.i2c_bitcounter = 0;
#if LOG_AKIKO_I2C
			logerror("I2C_DEVICEADDR: rw %d, address %02Xxx\n", akiko.nvram_rw, akiko.nvram_address >> 8);
#endif
		break;

		case I2C_WORDADDR:
			akiko.nvram_address &= 0x300;
			akiko.nvram_address |= akiko.nvram_byte;

#if LOG_AKIKO_I2C
			logerror("I2C_WORDADDR: address %04X\n", akiko.nvram_address);
#endif
			if ( akiko.i2c_direction < 0 )
			{
				memcpy( akiko.nvram_writetmp, akiko.nvram + (akiko.nvram_address & ~(NVRAM_PAGE_SIZE - 1)), NVRAM_PAGE_SIZE);
	    		akiko.nvram_writeaddr = akiko.nvram_address & (NVRAM_PAGE_SIZE - 1);
			}

			akiko.i2c_state = I2C_DATA;
			akiko.i2c_bitcounter = 0;
			akiko.i2c_direction = 1;
		break;
    }
}

void akiko_nvram_write(UINT32 data)
{
	int		sda;

	akiko.i2c_oscl = akiko.i2c_scl_out;
	akiko.i2c_scl_out = BIT(data,31);
	akiko.i2c_osda = akiko.i2c_sda_out;
	akiko.i2c_sda_out = BIT(data,30);
	akiko.i2c_scl_dir = BIT(data,15);
	akiko.i2c_sda_dir = BIT(data,14);

	sda = akiko.i2c_sda_out;
    if ( akiko.i2c_oscl != akiko.i2c_scl_out || akiko.i2c_osda != sda )
    {
    	akiko_i2c();
    	akiko.i2c_oscl = akiko.i2c_scl_out;
    	akiko.i2c_osda = sda;
    }
}

UINT32 akiko_nvram_read(void)
{
	UINT32	v = 0;

	if ( !akiko.i2c_scl_dir )
	    v |= akiko.i2c_scl_in ? (1<<31) : 0x00;
	else
	    v |= akiko.i2c_scl_out ? (1<<31) : 0x00;

	if ( !akiko.i2c_sda_dir )
	    v |= akiko.i2c_sda_in ? (1<<30) : 0x00;
	else
	    v |= akiko.i2c_sda_out ? (1<<30) : 0x00;

	v |= akiko.i2c_scl_dir ? (1<<15) : 0x00;
	v |= akiko.i2c_sda_dir ? (1<<14) : 0x00;

    return v;
}

static const char* akiko_reg_names[] =
{
	/*0*/	"ID",
	/*1*/	"CDROM STATUS 1",
	/*2*/	"CDROM_STATUS 2",
	/*3*/	"???",
	/*4*/	"CDROM ADDRESS 1",
	/*5*/	"CDROM ADDRESS 2",
	/*6*/	"CDROM COMMAND 1",
	/*7*/	"CDROM COMMAND 2",
	/*8*/	"CDROM READMASK",
	/*9*/	"CDROM LONGMASK",
	/*A*/	"???",
	/*B*/	"???",
	/*C*/	"NVRAM",
	/*D*/	"???",
	/*E*/	"C2P"
};

const char* get_akiko_reg_name(int reg)
{
	if (reg < 0xf )
	{
		return akiko_reg_names[reg];
	}
	else
	{
		return "???";
	}
}

static void akiko_setup_response( int len, int r0, UINT8 *r1 )
{
	int		resp_addr = akiko.cdrom_address[1];
	UINT8	resp_csum = 0xff;
	UINT8	resp_buffer[32];
	int		i;

	memset( resp_buffer, 0, sizeof( resp_buffer ) );

	resp_buffer[0] = r0;

	for( i = 0; i < len; i++ )
	{
		resp_buffer[i+1] = r1[i];
		resp_csum -= resp_buffer[i];
	}

	resp_buffer[len] = resp_csum;
	len++;

	for( i = 0; i < len; i++ )
	{
		program_write_byte( resp_addr + ((akiko.cdrom_cmd_resp + i) & 0xff), resp_buffer[i] );
	}

	akiko.cdrom_cmd_resp = (akiko.cdrom_cmd_resp+len) & 0xff;

	akiko.cdrom_status[0] |= 0x10000000; /* new data available */
}

static TIMER_CALLBACK( akiko_cd_delayed_cmd )
{
	UINT8	resp[32];

	memset( resp, 0, sizeof( resp ) );

	if ( param == 0x05 )
	{
		logerror( "AKIKO: Completing Command %d\n", param );

		if ( akiko.cdrom == NULL || cdrom_get_last_track(akiko.cdrom) == 0 )
		{
			resp[0] = 0x80;
			akiko_setup_response( 2, param, resp );
		}
		else
		{
			int		addrctrl = cdrom_get_adr_control( akiko.cdrom, 0 );
			UINT32	trackstart = lba_to_msf(cdrom_get_track_start( akiko.cdrom, 0 ));

			resp[0] = 0x00;
			resp[1] = ((addrctrl & 0x0f) << 4) | ((addrctrl & 0xf0) >> 4);
			/* 2-3: track number in BCD format */
			/* 4-7: ??? */
			resp[8] = (trackstart >> 16) & 0xff;
			resp[9] = (trackstart >> 8) & 0xff;
			resp[10] = trackstart & 0xff;

			akiko_setup_response( 15, param, resp );
		}
	}
}

static void akiko_update_cdrom( void )
{
	UINT8	resp[32];

	if ( akiko.cdrom_status[0] & 0x10000000 )
		return;

	while ( akiko.cdrom_cmd_start < akiko.cdrom_cmd_end )
	{
		int cmd_addr = akiko.cdrom_address[1] + 0x200 + akiko.cdrom_cmd_start;
		int cmd = program_read_byte( cmd_addr );

		cmd &= 0x0f;

		logerror( "CDROM command: %02X\n", cmd );

		memset( resp, 0, sizeof( resp ) );

		if ( cmd == 0x05 ) /* read toc */
		{
			akiko.cdrom_cmd_start = (akiko.cdrom_cmd_start+3) & 0xff;

			mame_timer_set( MAME_TIME_IN_MSEC(500), cmd, akiko_cd_delayed_cmd );

			break;
		}
		else if ( cmd == 0x07 )	/* check door status */
		{
			resp[0] = 0x01;

			akiko.cdrom_cmd_start = (akiko.cdrom_cmd_start+2) & 0xff;

			if ( akiko.cdrom == NULL || cdrom_get_last_track(akiko.cdrom) == 0 )
				resp[0] = 0x80;

			akiko_setup_response( 20, cmd, resp );
			break;
		}
		else
		{
			break;
		}
	}
}

READ32_HANDLER(amiga_akiko32_r)
{
	UINT32		retval;
	if ( offset < (0x30/4) )
	{
		if ( LOG_AKIKO ) logerror( "Reading AKIKO reg %0x [%s] at PC=%06x\n", offset, get_akiko_reg_name(offset), activecpu_get_pc() );
	}

	switch( offset )
	{
		case 0x00/4:	/* ID */
			init_akiko();
			return 0x0000cafe;

		case 0x04/4:	/* CDROM STATUS 1 */
			return akiko.cdrom_status[0];

		case 0x08/4:	/* CDROM STATUS 2 */
			return akiko.cdrom_status[1];

		case 0x10/4:	/* CDROM ADDRESS 1 */
			return akiko.cdrom_address[0];

		case 0x14/4:	/* CDROM ADDRESS 2 */
			return akiko.cdrom_address[1];

		case 0x18/4:	/* CDROM COMMAND 1 */
			akiko_update_cdrom();
			retval = akiko.cdrom_cmd_start;
			retval <<= 8;
			retval |= akiko.cdrom_cmd_resp;
			retval <<= 8;
			return retval;

		case 0x1C/4:	/* CDROM COMMAND 2 */
			akiko_update_cdrom();
			retval = akiko.cdrom_cmd_end;
			retval <<= 16;
			return retval;

		case 0x30/4:	/* NVRAM */
			return akiko_nvram_read();

		case 0x38/4:	/* C2P */
			return akiko_c2p_read();

		default:
			break;
	}

	return 0;
}

WRITE32_HANDLER(amiga_akiko32_w)
{
	if ( offset < (0x30/4) )
	{
		if ( LOG_AKIKO ) logerror( "Writing AKIKO reg %0x [%s] with %08x at PC=%06x\n", offset, get_akiko_reg_name(offset), data, activecpu_get_pc() );
	}

	switch( offset )
	{
		case 0x04/4:	/* CDROM STATUS 1 */
			akiko.cdrom_status[0] = data;
			break;

		case 0x08/4:	/* CDROM STATUS 2 */
			akiko.cdrom_status[1] = data;
			akiko.cdrom_status[0] &= data;
			break;

		case 0x10/4:	/* CDROM ADDRESS 1 */
			akiko.cdrom_address[0] = data;
			break;

		case 0x14/4:	/* CDROM ADDRESS 2 */
			akiko.cdrom_address[1] = data;
			break;

		case 0x18/4:	/* CDROM COMMAND 1 */
			if ( ( mem_mask & 0x00ff0000 ) == 0 )
				akiko.cdrom_cmd_start = ( data >> 16 ) & 0xff;

			if ( ( mem_mask & 0x0000ff00 ) == 0 )
				akiko.cdrom_cmd_resp = ( data >> 8 ) & 0xff;

			akiko_update_cdrom();
			break;

		case 0x1C/4:	/* CDROM COMMAND 2 */
			if ( ( mem_mask & 0x00ff0000 ) == 0 )
				akiko.cdrom_cmd_end = ( data >> 16 ) & 0xff;

			akiko_update_cdrom();
			break;

		case 0x30/4:
			akiko_nvram_write(data);
			break;

		case 0x38/4:
			akiko_c2p_write(data);
			break;

		default:
			break;
	}
}

static READ32_HANDLER(amiga_cia32_r)
{
	UINT32 retval = 0;

#if 0
	if ( (mem_mask & 0xffff) != 0xffff)
		retval |= (amiga_cia_r(offset*2, mem_mask & 0xffff) );

	if ( ((mem_mask >> 16) & 0xffff) != 0xffff)
		retval |= (amiga_cia_r(offset*2 + 1, mem_mask >> 16) << 16);
#else
	if ( ((mem_mask >> 16) & 0xffff) != 0xffff)
	{
		retval |= amiga_cia_r(offset*2, (mem_mask >> 16) & 0xffff);
		retval <<= 16;
	}

	if ( (mem_mask & 0xffff) != 0xffff )
		retval |= amiga_cia_r(offset*2 + 1, mem_mask & 0xffff);

	return retval;
#endif

	return retval;
}

static WRITE32_HANDLER(amiga_cia32_w)
{
//  amiga_cia_w(offset*2, data >> 16, mem_mask >> 16 );
	if ( ((mem_mask >> 16) & 0xffff) != 0xffff)
		amiga_cia_w(offset*2, data >> 16, mem_mask >> 16 );

	if ( (mem_mask & 0xffff) != 0xffff )
		amiga_cia_w(offset*2 + 1, data & 0xffff, mem_mask & 0xffff );
}

static READ32_HANDLER(amiga_custom32_r)
{
	UINT32 retval = 0;

	if ( ((mem_mask >> 16) & 0xffff) != 0xffff)
	{
		retval |= amiga_custom_r(offset*2, (mem_mask >> 16) & 0xffff);
		retval <<= 16;
	}

	if ( (mem_mask & 0xffff) != 0xffff )
		retval |= amiga_custom_r(offset*2 + 1, mem_mask & 0xffff);

	return retval;
}

static WRITE32_HANDLER(amiga_custom32_w)
{
	if ( ((mem_mask >> 16) & 0xffff) != 0xffff)
		amiga_custom_w(offset*2, data >> 16, mem_mask >> 16 );

	if ( (mem_mask & 0xffff) != 0xffff )
		amiga_custom_w(offset*2 + 1, data & 0xffff, mem_mask & 0xffff );
}

/*************************************
 *
 *  CIA-A port A access:
 *
 *  PA7 = game port 1, pin 6 (fire)
 *  PA6 = game port 0, pin 6 (fire)
 *  PA5 = /RDY (disk ready)
 *  PA4 = /TK0 (disk track 00)
 *  PA3 = /WPRO (disk write protect)
 *  PA2 = /CHNG (disk change)
 *  PA1 = /LED (LED, 0=bright / audio filter control)
 *  PA0 = OVL (ROM/RAM overlay bit)
 *
 *************************************/

static UINT8 cd32_cia_0_porta_r(void)
{
	return readinputportbytag("CIA0PORTA");
}

static void cd32_cia_0_porta_w(UINT8 data)
{
	/* switch banks as appropriate */
	memory_set_bank(1, data & 1);

	/* swap the write handlers between ROM and bank 1 based on the bit */
	if ((data & 1) == 0)
		/* overlay disabled, map RAM on 0x000000 */
		memory_install_write32_handler(0, ADDRESS_SPACE_PROGRAM, 0x000000, 0x1fffff, 0, 0, MWA32_BANK1);

	else
		/* overlay enabled, map Amiga system ROM on 0x000000 */
		memory_install_write32_handler(0, ADDRESS_SPACE_PROGRAM, 0x000000, 0x1fffff, 0, 0, MWA32_ROM);

	/* bit 2 = Power Led on Amiga */
	set_led_status(0, (data & 2) ? 0 : 1);
}

/*************************************
 *
 *  CIA-A port B access:
 *
 *  PB7 = parallel data 7
 *  PB6 = parallel data 6
 *  PB5 = parallel data 5
 *  PB4 = parallel data 4
 *  PB3 = parallel data 3
 *  PB2 = parallel data 2
 *  PB1 = parallel data 1
 *  PB0 = parallel data 0
 *
 *************************************/

static UINT8 cd32_cia_0_portb_r(void)
{
	/* parallel port */
	logerror("%06x:CIA0_portb_r\n", activecpu_get_pc());
	return 0xff;
}

static void cd32_cia_0_portb_w(UINT8 data)
{
	/* parallel port */
	logerror("%06x:CIA0_portb_w(%02x)\n", activecpu_get_pc(), data);
}

static NVRAM_HANDLER( cd32 )
{
	if (read_or_write)
		/* save the SRAM settings */
		mame_fwrite(file, akiko.nvram, NVRAM_SIZE);
	else
	{
		/* load the SRAM settings */
		if (file)
			mame_fread(file, akiko.nvram, NVRAM_SIZE);
		else
			memset(akiko.nvram, 0, NVRAM_SIZE);
	}
}

static ADDRESS_MAP_START( cd32_map, ADDRESS_SPACE_PROGRAM, 32 )
	ADDRESS_MAP_FLAGS( AMEF_UNMAP(1) )
	AM_RANGE(0x000000, 0x1fffff) AM_RAMBANK(1) AM_BASE(&amiga_chip_ram32) AM_SIZE(&amiga_chip_ram_size)
	AM_RANGE(0xb80000, 0xb8003f) AM_READWRITE(amiga_akiko32_r, amiga_akiko32_w)
	AM_RANGE(0xbfd000, 0xbfefff) AM_READWRITE(amiga_cia32_r, amiga_cia32_w)
	AM_RANGE(0xc00000, 0xdfffff) AM_READWRITE(amiga_custom32_r, amiga_custom32_w) AM_BASE((UINT32**)&amiga_custom_regs)
	AM_RANGE(0xe00000, 0xe7ffff) AM_ROM AM_REGION(REGION_USER1, 0x80000)	/* CD32 Extended ROM */
	AM_RANGE(0xa00000, 0xf7ffff) AM_NOP
	AM_RANGE(0xf80000, 0xffffff) AM_ROM AM_REGION(REGION_USER1, 0x0)		/* Kickstart */
ADDRESS_MAP_END



INPUT_PORTS_START( cd32 )
	PORT_START_TAG("CIA0PORTA")
	PORT_BIT( 0x3f, IP_ACTIVE_LOW, IPT_SPECIAL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL

	PORT_START_TAG("CIA0PORTB")
	PORT_DIPNAME( 0x01, 0x01, "DSW1 1" )
	PORT_DIPSETTING(    0x01, "Reset" )
	PORT_DIPSETTING(    0x00, "Set" )
	PORT_SERVICE_NO_TOGGLE( 0x02, IP_ACTIVE_LOW )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START_TAG("JOY0DAT")
	PORT_BIT( 0x0303, IP_ACTIVE_HIGH, IPT_SPECIAL ) PORT_CUSTOM(amiga_joystick_convert, "P1JOY")
	PORT_BIT( 0xfcfc, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START_TAG("JOY1DAT")
	PORT_BIT( 0x0303, IP_ACTIVE_HIGH, IPT_SPECIAL ) PORT_CUSTOM(amiga_joystick_convert, "P2JOY")
	PORT_BIT( 0xfcfc, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START_TAG("POTGO")
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_COCKTAIL
	PORT_BIT( 0xaaff, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START_TAG("P1JOY")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)

	PORT_START_TAG("P2JOY")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)

	PORT_START_TAG("COINS")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
INPUT_PORTS_END

/*************************************
 *
 *  Sound definitions
 *
 *************************************/

static struct CustomSound_interface amiga_custom_interface =
{
	amiga_sh_start
};


static MACHINE_DRIVER_START( cd32 )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68EC020, AMIGA_68EC020_NTSC_CLOCK) /* 14.3 Mhz */
	MDRV_CPU_PROGRAM_MAP(cd32_map,0)
	MDRV_CPU_VBLANK_INT(amiga_scanline_callback, 262)

	MDRV_SCREEN_REFRESH_RATE(59.997)
	MDRV_SCREEN_VBLANK_TIME(USEC_TO_SUBSECONDS(0))

	MDRV_MACHINE_RESET(amiga)
	MDRV_NVRAM_HANDLER(cd32)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(512*2, 262)
	MDRV_SCREEN_VISIBLE_AREA((129-8)*2, (449+8-1)*2, 44-8, 244+8-1)
	MDRV_PALETTE_LENGTH(4096)
	MDRV_PALETTE_INIT(amiga)

	MDRV_VIDEO_START(amiga)
	MDRV_VIDEO_UPDATE(generic_bitmapped)

	/* sound hardware */
    MDRV_SPEAKER_STANDARD_STEREO("left", "right")

    MDRV_SOUND_ADD(CUSTOM, 3579545)
    MDRV_SOUND_CONFIG(amiga_custom_interface)
    MDRV_SOUND_ROUTE(0, "left", 0.50)
    MDRV_SOUND_ROUTE(1, "right", 0.50)
    MDRV_SOUND_ROUTE(2, "right", 0.50)
    MDRV_SOUND_ROUTE(3, "left", 0.50)
MACHINE_DRIVER_END



#define ROM_LOAD16_WORD_BIOS(bios,name,offset,length,hash)     ROMX_LOAD(name, offset, length, hash, ROM_BIOS(bios+1))

#define CD32_BIOS \
	ROM_REGION32_BE(0x100000, REGION_USER1, 0 ) \
	ROM_SYSTEM_BIOS(0, "cd32", "Kickstart v3.1 rev 40.60 with CD32 Extended-ROM" ) \
	ROM_LOAD16_WORD_BIOS(0, "391640-03.u6a", 0x000000, 0x100000, CRC(d3837ae4) SHA1(06807db3181637455f4d46582d9972afec8956d9) ) \


ROM_START( cd32 )
	CD32_BIOS
ROM_END

ROM_START( cndypuzl )
	CD32_BIOS

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "cndypuzl", 0, SHA1(21093753a1875dc4fb97f23232ed3d8776b48c06) MD5(dcb6cdd7d81d5468c1290a3baf4265cb) )
ROM_END

ROM_START( haremchl )
	CD32_BIOS

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "haremchl", 0, SHA1(4d5df2b64b376e8d0574100110f3471d3190765c) MD5(00adbd944c05747e9445446306f904be) )
ROM_END

ROM_START( lsrquiz )
	CD32_BIOS

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "lsrquiz", 0, SHA1(4250c94ab77504104005229b28f24cfabe7c9e48) MD5(12a94f573fe5d218db510166b86fdda5) )
ROM_END

ROM_START( lsrquiz2 )
	CD32_BIOS

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "lsrquiz2", 0, SHA1(ea92df0e53bf36bb86d99ad19fca21c6129e61d7) MD5(df63c32aca815f6c97889e08c10b77bc) )
ROM_END

ROM_START( mgprem11 )
	CD32_BIOS

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "mgprem11", 0, SHA1(a8a32d10148ba968b57b8186fdf4d4cd378fb0d5) MD5(e0e4d00c6f981c19a1d20d5e7090b0db) )
ROM_END

ROM_START( lasstixx )
	CD32_BIOS

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "lasstixx", 0, SHA1(29c2525d43a696da54648caffac9952cec85fd37) MD5(6242dd8a3c0b15ef9eafb930b7a7e87f) )
ROM_END

/***************************************************************************************************/

static DRIVER_INIT( cd32 )
{
	static const amiga_machine_interface cubocd32_intf =
	{
		AGA_CHIP_RAM_MASK,
		cd32_cia_0_porta_r, cd32_cia_0_portb_r,
		cd32_cia_0_porta_w, cd32_cia_0_portb_w,
		NULL, NULL,
		NULL, NULL,
		NULL, NULL, NULL,
		NULL, NULL, NULL,
		NULL, NULL,
		NULL,
		FLAGS_AGA_CHIPSET
	};

	/* configure our Amiga setup */
	amiga_machine_config(&cubocd32_intf);

	/* set up memory */
	memory_configure_bank(1, 0, 1, amiga_chip_ram32, 0);
	memory_configure_bank(1, 1, 1, memory_region(REGION_USER1), 0);
}

/***************************************************************************************************/

/* BIOS */
GAMEB( 1993, cd32, 0, cd32, cd32, cd32, cd32,   ROT0, "Commodore", "Amiga CD32 Bios", GAME_IS_BIOS_ROOT )

GAMEB( 1995, cndypuzl, cd32, cd32, cd32, cd32, cd32,	   ROT0, "CD Express", "Candy Puzzle (v1.0)", GAME_NOT_WORKING|GAME_NO_SOUND )
GAMEB( 1995, haremchl, cd32, cd32, cd32, cd32, cd32,	   ROT0, "CD Express", "Harem Challenge", GAME_NOT_WORKING|GAME_NO_SOUND )
GAMEB( 1995, lsrquiz,  cd32, cd32, cd32, cd32, cd32,	   ROT0, "CD Express", "Laser Quiz", GAME_NOT_WORKING|GAME_NO_SOUND )
GAMEB( 1995, lsrquiz2, cd32, cd32, cd32, cd32, cd32,	   ROT0, "CD Express", "Laser Quiz '2' Italy (v1.0)", GAME_NOT_WORKING|GAME_NO_SOUND )
GAMEB( 1996, mgprem11, cd32, cd32, cd32, cd32, cd32,	   ROT0, "CD Express", "Magic Premium (v1.1)", GAME_NOT_WORKING|GAME_NO_SOUND )
GAMEB( 1995, lasstixx, cd32, cd32, cd32, cd32, cd32,	   ROT0, "CD Express", "Laser Strixx", GAME_NOT_WORKING|GAME_NO_SOUND )

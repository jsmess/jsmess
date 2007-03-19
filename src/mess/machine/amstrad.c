/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

Amstrad hardware consists of:

- General Instruments AY-3-8912 (audio and keyboard scanning)
- Intel 8255PPI (keyboard, access to AY-3-8912, cassette etc)
- Z80A CPU
- 765 FDC (disc drive interface)
- "Gate Array" (custom chip by Amstrad controlling colour, mode,
rom/ram selection


***************************************************************************/

#include <stdarg.h>
#include "driver.h"
#include "cpu/z80/z80.h"
#include "video/m6845.h"
#include "includes/amstrad.h"
//#include "systems/i8255.h"
#include "machine/8255ppi.h"
#include "machine/nec765.h"
#include "devices/dsk.h"
#include "devices/cassette.h"
#include "sound/ay8910.h"
#include "image.h"

void amstrad_setup_machine(void)
{
	amstrad_reset_machine();
}

/* load CPCEMU style snapshots */
void amstrad_handle_snapshot(unsigned char *pSnapshot)
{
	int RegData;
	int i;

	/* init Z80 */
	RegData = (pSnapshot[0x011] & 0x0ff) | ((pSnapshot[0x012] & 0x0ff)<<8);
	cpunum_set_reg(0,Z80_AF, RegData);

	RegData = (pSnapshot[0x013] & 0x0ff) | ((pSnapshot[0x014] & 0x0ff)<<8);
	cpunum_set_reg(0,Z80_BC, RegData);

	RegData = (pSnapshot[0x015] & 0x0ff) | ((pSnapshot[0x016] & 0x0ff)<<8);
	cpunum_set_reg(0,Z80_DE, RegData);

	RegData = (pSnapshot[0x017] & 0x0ff) | ((pSnapshot[0x018] & 0x0ff)<<8);
	cpunum_set_reg(0,Z80_HL, RegData);

	RegData = (pSnapshot[0x019] & 0x0ff) ;
	cpunum_set_reg(0,Z80_R, RegData);

	RegData = (pSnapshot[0x01a] & 0x0ff);
	cpunum_set_reg(0,Z80_I, RegData);

	if ((pSnapshot[0x01b] & 1)==1)
	{
		cpunum_set_reg(0,Z80_IFF1, 1);
	}
	else
	{
		cpunum_set_reg(0,Z80_IFF1, 0);
	}

	if ((pSnapshot[0x01c] & 1)==1)
	{
		cpunum_set_reg(0,Z80_IFF2, 1);
	}
	else
	{
		cpunum_set_reg(0,Z80_IFF2, 0);
	}

	RegData = (pSnapshot[0x01d] & 0x0ff) | ((pSnapshot[0x01e] & 0x0ff)<<8);
	cpunum_set_reg(0,Z80_IX, RegData);

	RegData = (pSnapshot[0x01f] & 0x0ff) | ((pSnapshot[0x020] & 0x0ff)<<8);
	cpunum_set_reg(0,Z80_IY, RegData);

	RegData = (pSnapshot[0x021] & 0x0ff) | ((pSnapshot[0x022] & 0x0ff)<<8);
	cpunum_set_reg(0,Z80_SP, RegData);
	cpunum_set_reg(0,REG_SP,RegData);

	RegData = (pSnapshot[0x023] & 0x0ff) | ((pSnapshot[0x024] & 0x0ff)<<8);

	cpunum_set_reg(0,Z80_PC, RegData);
//	cpunum_set_reg(0,REG_SP,RegData);

	RegData = (pSnapshot[0x025] & 0x0ff);
	cpunum_set_reg(0,Z80_IM, RegData);

	RegData = (pSnapshot[0x026] & 0x0ff) | ((pSnapshot[0x027] & 0x0ff)<<8);
	cpunum_set_reg(0,Z80_AF2, RegData);

	RegData = (pSnapshot[0x028] & 0x0ff) | ((pSnapshot[0x029] & 0x0ff)<<8);
	cpunum_set_reg(0,Z80_BC2, RegData);

	RegData = (pSnapshot[0x02a] & 0x0ff) | ((pSnapshot[0x02b] & 0x0ff)<<8);
	cpunum_set_reg(0,Z80_DE2, RegData);

	RegData = (pSnapshot[0x02c] & 0x0ff) | ((pSnapshot[0x02d] & 0x0ff)<<8);
	cpunum_set_reg(0,Z80_HL2, RegData);

	/* init GA */
	for (i=0; i<17; i++)
	{
		amstrad_GateArray_write(i);

		amstrad_GateArray_write(((pSnapshot[0x02f + i] & 0x01f) | 0x040));
	}

	amstrad_GateArray_write(pSnapshot[0x02e] & 0x01f);

	amstrad_GateArray_write(((pSnapshot[0x040] & 0x03f) | 0x080));

	AmstradCPC_PALWrite(((pSnapshot[0x041] & 0x03f) | 0x0c0));

	/* init CRTC */
	for (i=0; i<18; i++)
	{
                crtc6845_address_w(0,i);
                crtc6845_register_w(0, pSnapshot[0x043+i] & 0x0ff);
	}

    crtc6845_address_w(0,i);

	/* upper rom selection */
	AmstradCPC_SetUpperRom(pSnapshot[0x055]);

	/* PPI */
	ppi8255_w(0,3,pSnapshot[0x059] & 0x0ff);

	ppi8255_w(0,0,pSnapshot[0x056] & 0x0ff);
	ppi8255_w(0,1,pSnapshot[0x057] & 0x0ff);
	ppi8255_w(0,2,pSnapshot[0x058] & 0x0ff);

	/* PSG */
	for (i=0; i<16; i++)
	{
		AY8910_control_port_0_w(0,i);

		AY8910_write_port_0_w(0,pSnapshot[0x05b + i] & 0x0ff);
	}

	AY8910_control_port_0_w(0,pSnapshot[0x05a]);

	{
		int MemSize;
		int MemorySize;

		MemSize = (pSnapshot[0x06b] & 0x0ff) | ((pSnapshot[0x06c] & 0x0ff)<<8);

		if (MemSize==128)
		{
			MemorySize = 128*1024;
		}
		else
		{
			MemorySize = 64*1024;
		}

		memcpy(mess_ram, &pSnapshot[0x0100], MemorySize);
	}
	amstrad_rethinkMemory();
}

/* load snapshot */
SNAPSHOT_LOAD(amstrad)
{
	UINT8 *snapshot;

	/* get file size */
	if (snapshot_size < 8)
		return INIT_FAIL;

	snapshot = malloc(snapshot_size);
	if (!snapshot)
		return INIT_FAIL;

	/* read whole file */
	image_fread(image, snapshot, snapshot_size);

	if (memcmp(snapshot, "MV - SNA", 8))
	{
		free(snapshot);
		return INIT_FAIL;
	}

	amstrad_handle_snapshot(snapshot);
	free(snapshot);
	return INIT_PASS;
}

DEVICE_LOAD(amstrad_plus_cartridge)
{
	// load CPC Plus / GX4000 cartridge image
	// Format is RIFF: RIFF header chunk contains "AMS!"
	// Chunks should be 16k, but may vary
	// Chunks labeled 'cb00' represent Cartridge block 0, and is loaded to &0000-&3fff
	//                'cb01' represent Cartridge block 1, and is loaded to &4000-&7fff
	//                ... and so on.

	unsigned char header[12];  // RIFF chunk
	char chunkid[4];  // chunk ID (4 character code - cb00, cb01, cb02... upto cb31 (max 512kB), other chunks are ignored)
	char chunklen[4];  // chunk length (always little-endian)
	int chunksize;  // chunk length, calcaulated from the above
	int ramblock;  // 16k RAM block chunk is to be loaded in to
	int result;
	unsigned int bytes_to_read;  // total bytes to read, as mame_feof doesn't react to EOF without trying to go past it.
	unsigned char* mem = memory_region(REGION_CPU1);

	logerror("CPR: loading RIFF format CPC cartridge file\n");
	// load RIFF chunk
	result = image_fread(image,header,12);
	if(result != 12)
	{
		logerror("CPR: failed to read from cart image\n");
		return INIT_FAIL;
	}
	if(strncmp(header,"RIFF",4) != 0)
	{
		logerror("CPR: not an RIFF format file - header is '%s'\n",header);
		return INIT_FAIL;
	}
	if(strncmp((char*)(header+8),"AMS!",4) != 0)
	{
		logerror("CPR: not an Amstrad CPC cartridge image\n");
		return INIT_FAIL;
	}

	bytes_to_read = header[4] + (header[5] << 8) + (header[6] << 16)+ (header[7] << 24);
	bytes_to_read -= 4;  // account for AMS! header
	logerror("CPR: Data to read: %i bytes\n",bytes_to_read);
	// read some chunks
	while(bytes_to_read > 0)
	{
		result = image_fread(image,chunkid,4);
		if(result != 4)
		{
			logerror("CPR: failed to read from cart image\n");
			return INIT_FAIL;
		}
		bytes_to_read -= result;
		result = image_fread(image,chunklen,4);
		if(result != 4)
		{
			logerror("CPR: failed to read from cart image\n");
			return INIT_FAIL;
		}
		bytes_to_read -= result;
		// calculate little-endian value, just to be sure
		chunksize = chunklen[0] + (chunklen[1] << 8) + (chunklen[2] << 16) + (chunklen[3] << 24);

		if(strncmp(chunkid,"cb",2) == 0)
		{
			// load chunk into RAM
			// find out what block this is
			ramblock = (chunkid[2] - 0x30) * 10;
			ramblock += chunkid[3] - 0x30;
			logerror("CPR: Loading chunk into RAM block %i ['%4s']\n",ramblock,chunkid);
			if(ramblock >= 0 && ramblock < 32)
			{
				// clear RAM block
				memset(mem+(0x4000*ramblock),0,0x4000);

				// load block into ROM area
				if(chunksize > 16384)
					chunksize = 16384;
				result = image_fread(image,mem+(0x4000*ramblock),chunksize);
				if(result != chunksize)
				{
					logerror("CPR: Read %i-byte chunk, expected %i bytes\n",result,chunksize);
					return INIT_FAIL;
				}
				bytes_to_read -= chunksize;
				logerror("CPR: Loaded %i-byte chunk into RAM block %i\n",result,ramblock);
			}
		}
		else
		{
			logerror("CPR: Unknown chunk '%4s', skipping %i bytes\n",chunkid,chunksize);
			if(chunksize != 0)
			{
				image_fseek(image,chunksize,SEEK_CUR);
				bytes_to_read -= chunksize;
			}
		}
	}
	

	return INIT_PASS;
}


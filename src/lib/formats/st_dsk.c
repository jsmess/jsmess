/*********************************************************************

    formats/st_dsk.c

    All usual Atari ST formats

*********************************************************************/

#include "emu.h"
#include "formats/st_dsk.h"

st_format::st_format()
{
}

const char *st_format::name() const
{
	return "st";
}

const char *st_format::description() const
{
	return "Atari ST floppy disk image";
}

const char *st_format::extensions() const
{
	return "st";
}

bool st_format::supports_save() const
{
	return false;
}

void st_format::find_size(io_generic *io, int &track_count, int &head_count, int &sector_count)
{
	int size = io_generic_size(io);
	for(track_count=80; track_count <= 82; track_count++)
		for(head_count=1; head_count <= 2; head_count++)
			for(sector_count=9; sector_count <= 11; sector_count++)
				if(size == 512*track_count*head_count*sector_count)
					return;
	track_count = head_count = sector_count = 0;
}

int st_format::identify(io_generic *io)
{
	int track_count, head_count, sector_count;
	find_size(io, track_count, head_count, sector_count);

	if(track_count)
		return 50;
	return 0;
}

bool st_format::load(io_generic *io, floppy_image *image)
{
	int track_count, head_count, sector_count;
	find_size(io, track_count, head_count, sector_count);

	UINT8 sectdata[11*512];
	desc_s sectors[11];
	for(int i=1; i<=sector_count; i++) {
		sectors[i].data = sectdata + 512*(i-1);
		sectors[i].size = 512;
	}

	int track_size = sector_count*512;
	for(int track=0; track < track_count; track++) {
		for(int side=0; side < head_count; side++) {
			io_generic_read(io, sectdata, (track*head_count + side)*track_size, track_size);
			generate_track(atari_st_fcp_get_desc(track, side, head_count, sector_count),
						   track, side, sectors, sector_count+1, 100000, image);
		}
	}

	return TRUE;
}

msa_format::msa_format()
{
}

const char *msa_format::name() const
{
	return "msa";
}

const char *msa_format::description() const
{
	return "Atari MSA floppy disk image";
}

const char *msa_format::extensions() const
{
	return "msa";
}

bool msa_format::supports_save() const
{
	return false;
}

void msa_format::read_header(io_generic *io, UINT16 &sign, UINT16 &sect, UINT16 &head, UINT16 &strack, UINT16 &etrack)
{
	UINT8 h[10];
	io_generic_read(io, h, 0, 10);
	sign = (h[0] << 8) | h[1];
	sect = (h[2] << 8) | h[3];
	head = (h[4] << 8) | h[5];
	strack = (h[6] << 8) | h[7];
	etrack = (h[8] << 8) | h[9];
}

bool msa_format::uncompress(UINT8 *buffer, int csize, int usize)
{
	UINT8 sectdata[11*512];
	int src=0, dst=0;
	while(src<csize && dst<usize) {
		unsigned char c = buffer[src++];
		if(c == 0xe5) {
			if(csize-src < 3)
				return false;
			c = buffer[src++];
			int count = (buffer[src] << 8) | buffer[src+1];
			src += 2;
			if(usize-dst < count)
				return false;
			for(int i=0; i<count; i++)
				sectdata[dst++] = c;
		} else
			sectdata[dst++] = c;
	}

	if(src != csize || dst != usize)
		return false;
	memcpy(buffer, sectdata, usize);
	return true;
}

int msa_format::identify(io_generic *io)
{
	UINT16 sign, sect, head, strack, etrack;
	read_header(io, sign, sect, head, strack, etrack);

	if(sign == 0x0e0f &&
	   (sect >= 9 && sect <= 11) &&
	   (head == 0 || head == 1) &&
	   strack <= etrack &&
	   etrack < 82)
		return 100;
	return 0;
}

bool msa_format::load(io_generic *io, floppy_image *image)
{
	UINT16 sign, sect, head, strack, etrack;
	read_header(io, sign, sect, head, strack, etrack);

	UINT8 sectdata[11*512];
	desc_s sectors[11];
	for(int i=1; i<=sect; i++) {
		sectors[i].data = sectdata + 512*(i-1);
		sectors[i].size = 512;
	}

	int pos = 10;
	int track_size = sect*512;

	for(int track=strack; track <= etrack; track++) {
		for(int side=0; side <= head; side++) {
			UINT8 th[2];
			io_generic_read(io, th, pos, 2);
			pos += 2;
			int tsize = (th[0] << 8) | th[1];
			io_generic_read(io, sectdata, pos, tsize);
			pos += tsize;
			if(tsize < track_size) {
				if(!uncompress(sectdata, tsize, track_size))
					return false;
			}
			generate_track(atari_st_fcp_get_desc(track, side, head+1, sect),
						   track, side, sectors, sect+1, 100000, image);
		}
	}

	return true;
}

const floppy_format_type FLOPPY_ST_FORMAT = &floppy_image_format_creator<st_format>;
const floppy_format_type FLOPPY_MSA_FORMAT = &floppy_image_format_creator<msa_format>;

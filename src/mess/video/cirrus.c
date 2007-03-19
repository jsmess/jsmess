/***************************************************************************

	vidhrdw/cirrus.c

	Cirrus SVGA card emulation (preliminary)

	Cirrus has the following additional registers that are not present in
	conventional VGA:

	SEQ 06h:		Unlock Cirrus registers; write 12h to unlock registers,
					and	read 12h back to confirm Cirrus presence.
	SEQ 07h
		bit 3-1:	Pixel depth
						0x00	8 bpp
						0x02	16 bpp (double vert clock)
						0x04	24 bpp
						0x06	16 bpp
						0x08	32 bpp
		bit 0:		VGA/SVGA (0=VGA, 1=SVGA)
	SEQ 0Fh
		bit 7:		Bankswitch enable
		bits 4-3:	Memory size
						0x00	256K
						0x08	512K
						0x10	1M
						0x18	2M
	SEQ 12h:		Hardware Cursor



					
	GC 09h:			Set 64k bank (bits 3-0 only)
	GC 20h:			Blit Width (bits 7-0)
	GC 21h:			Blit Width (bits 12-8)
	GC 22h:			Blit Height (bits 7-0)
	GC 23h:			Blit Height (bits 12-8)
	GC 24h:			Blit Destination Pitch (bits 7-0)
	GC 25h:			Blit Destination Pitch (bits 12-8)
	GC 26h:			Blit Source Pitch (bits 7-0)
	GC 27h:			Blit Source Pitch (bits 12-8)
	GC 28h:			Blit Destination Address (bits 7-0)
	GC 29h:			Blit Destination Address (bits 15-8)
	GC 2Ah:			Blit Destination Address (bits 21-16)
	GC 2Ch:			Blit Source Address (bits 7-0)
	GC 2Dh:			Blit Source Address (bits 15-8)
	GC 2Eh:			Blit Source Address (bits 21-16)
	GC 2Fh:			Blit Write Mask
	GC 30h:			Blit Mode
	GC 31h:			Blit Status
						bit 7 - Autostart
						bit 4 - FIFO Used
						bit 2 - Blit Reset
						bit 1 - Blit Started
						bit 0 - Blit Busy
	GC 32h:			Raster Operation
	GC 33h:			Blit Mode Extension
	GC 34h:			Blit Transparent Color (bits 7-0)
	GC 35h:			Blit Transparent Color (bits 15-8)
	GC 38h:			Blit Transparent Color Mask (bits 7-0)
	GC 39h:			Blit Transparent Color Mask (bits 15-8)

***************************************************************************/

#include "cirrus.h"
#include "memconv.h"

#define LOG_PCIACCESS	0



static void cirrus_update_8bpp(mame_bitmap *bitmap, struct crtc6845 *crtc)
{
	UINT16 *line;
	const UINT8 *vram;
	int y, x;

	vram = (const UINT8 *) pc_vga_memory();

	for (y = 0; y < 480; y++)
	{
		line = BITMAP_ADDR16(bitmap, y, 0);

		for (x = 0; x < 640; x++)
			*line++ = *vram++;
	}
}



static void cirrus_update_16bpp(mame_bitmap *bitmap, struct crtc6845 *crtc)
{
	fatalerror("NYI");
}



static void cirrus_update_24bpp(mame_bitmap *bitmap, struct crtc6845 *crtc)
{
	fatalerror("NYI");
}



static void cirrus_update_32bpp(mame_bitmap *bitmap, struct crtc6845 *crtc)
{
	fatalerror("NYI");
}



static pc_video_update_proc cirrus_choosevideomode(const UINT8 *sequencer,
	const UINT8 *crtc, const UINT8 *gc, int *width, int *height)
{
	pc_video_update_proc proc = NULL;

	if ((sequencer[0x06] == 0x12) && (sequencer[0x07] & 0x01))
	{
		*width = 640;
		*height = 480;

		switch(sequencer[0x07] & 0x0E)
		{
			case 0x00:	proc = cirrus_update_8bpp; break;
			case 0x02:	proc = cirrus_update_16bpp; break;
			case 0x04:	proc = cirrus_update_24bpp; break;
			case 0x06:	proc = cirrus_update_16bpp; break;
			case 0x08:	proc = cirrus_update_32bpp; break;
		}
	}
	return proc;
}



const struct pc_svga_interface cirrus_svga_interface =
{
	2 * 1024 * 1024,	/* 2 megs vram */
	8,					/* 8 sequencer registers */
	10,					/* 10 gc registers */
	25,					/* 25 crtc registers */
	cirrus_choosevideomode
};



/*************************************
 *
 *	PCI card interface
 *
 *************************************/

static UINT32 cirrus5430_pci_read(int function, int offset, UINT32 mem_mask)
{
	UINT32 result = 0;

	if (function == 0)
	{
		switch(offset)
		{
			case 0x00:	/* vendor/device ID */
				result = 0x00A01013;
				break;

			case 0x08:
				result = 0x03000000;
				break;

			case 0x10:
				result = 0xD0000000;
				break;

			default:
				result = 0;
				break;
		}
	}

	if (LOG_PCIACCESS)
		logerror("cirrus5430_pci_read(): function=%d offset=0x%02X result=0x%04X\n", function, offset, result);
	return result;
}



static void cirrus5430_pci_write(int function, int offset, UINT32 data, UINT32 mem_mask)
{
	if (LOG_PCIACCESS)
		logerror("cirrus5430_pci_write(): function=%d offset=0x%02X data=0x%04X\n", function, offset, data);
}



const struct pci_device_info cirrus5430_callbacks =
{
	cirrus5430_pci_read,
	cirrus5430_pci_write
};



/*************************************
 *
 *	Ports
 *
 *************************************/

WRITE8_HANDLER( cirrus_42E8_w )
{
	if (data & 0x80)
		pc_vga_reset();
}



WRITE64_HANDLER( cirrus_64be_42E8_w ) { write64be_with_write8_handler(cirrus_42E8_w, offset, data, mem_mask); }


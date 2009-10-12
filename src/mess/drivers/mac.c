/****************************************************************************

	drivers/mac.c
	Macintosh family emulation
 
	Nate Woods, Raphael Nabet, R. Belmont
 
 
	    0x000000 - 0x3fffff     RAM/ROM (switches based on overlay)
	    0x400000 - 0x4fffff     ROM
 	    0x580000 - 0x5fffff     5380 NCR/Symbios SCSI peripherals chip (Mac Plus only)
 	    0x600000 - 0x6fffff     RAM
	    0x800000 - 0x9fffff     Zilog 8530 SCC (Serial Control Chip) Read
	    0xa00000 - 0xbfffff     Zilog 8530 SCC (Serial Control Chip) Write
	    0xc00000 - 0xdfffff     IWM (Integrated Woz Machine; floppy)
	    0xe80000 - 0xefffff     Rockwell 6522 VIA
	    0xf00000 - 0xffffef     ??? (the ROM appears to be accessing here)
	    0xfffff0 - 0xffffff     Auto Vector
	
	
	Interrupts:
	    M68K:
	        Level 1 from VIA
	        Level 2 from SCC
	        Level 4 : Interrupt switch (not implemented)
 
	    VIA:
	        CA1 from VBLANK
	        CA2 from 1 Hz clock (RTC)
	        CB1 from Keyboard Clock
	        CB2 from Keyboard Data
	        SR  from Keyboard Data Ready
	
	    SCC:
	        PB_EXT  from mouse Y circuitry
	        PA_EXT  from mouse X circuitry

****************************************************************************/

#include "driver.h"
#include "cpu/m68000/m68000.h"
#include "includes/mac.h"
#include "machine/6522via.h"
#include "machine/ncr5380.h"
#include "machine/applefdc.h"
#include "devices/sonydriv.h"
#include "devices/harddriv.h"
#include "formats/ap_dsk35.h"

UINT32 *se30_vram;

/*
	Apple Sound Chip

	Base is normally IOBase + 0x14000
	First 0x800 bytes is buffer RAM

	Verified to be only 8 bits wide by Apple documentation.

	Registers:
	0x800: CONTROL
	0x801: ENABLE
	0x802: MODE
	0x806: VOLUME
	0x807: CHAN
*/

static UINT8 mac_asc_regs[0x2000];

static READ8_HANDLER(mac_asc_r)
{
//	logerror("ASC: Read @ %x (PC %x)\n", offset, cpu_get_pc(cputag_get_cpu(space->machine, "maincpu")));

	return mac_asc_regs[offset];
}

static WRITE8_HANDLER(mac_asc_w)
{
//	logerror("ASC: %02x to %x (PC %x)\n", data, offset, cpu_get_pc(cputag_get_cpu(space->machine, "maincpu")));

	mac_asc_regs[offset] = data;
}

static READ32_HANDLER(mac_swim_r)
{
	return 0x17171717;
}

/*
	RasterOps ColorBoard 264: fixed resolution 640x480 NuBus video card, 1/4/8/16/24 bit color
	1.5? MB of VRAM (tests up to 0x1fffff), Bt473 RAMDAC, and two custom gate arrays.
*/

static UINT32 *cb264_vram;
static UINT32 cb264_mode = 0; 	// 0 = true color, 2 = 1bpp monochrome, others unknown
static UINT32 cb264_palette[256];

static VIDEO_START( cb264 )
{
}

static VIDEO_UPDATE( cb264 )
{
	UINT32 *scanline, *base;
	int x, y;

	if (input_code_pressed(screen->machine, KEYCODE_G))
	{
		FILE *f;

		f = fopen("vram.bin", "wb");
		fwrite(cb264_vram, 0x300000, 1, f);
		fclose(f);
		printf("Dumped CB264 VRAM\n");
	}

	if (cb264_mode == 0)
	{
		for (y = 0; y < 480; y++)
		{
			scanline = BITMAP_ADDR32(bitmap, y, 0);
			base = &cb264_vram[y * 1024];
			for (x = 0; x < 640; x++)
			{
				*scanline++ = *base++;
			}
		}
	}
	else if (cb264_mode == 2)
	{
		UINT8 *vram8 = (UINT8 *)cb264_vram; //, *base8;
		UINT8 pixels;

		for (y = 0; y < 480; y++)
		{
			scanline = BITMAP_ADDR32(bitmap, y, 0);
			for (x = 0; x < 640; x+=8)
			{
				pixels = vram8[(y * 1024) + ((x/8)^3)];

				*scanline++ = cb264_palette[(pixels>>7)^1];
				*scanline++ = cb264_palette[((pixels>>6)&1)^1];
				*scanline++ = cb264_palette[((pixels>>5)&1)^1];
				*scanline++ = cb264_palette[((pixels>>4)&1)^1];
				*scanline++ = cb264_palette[((pixels>>3)&1)^1];
				*scanline++ = cb264_palette[((pixels>>2)&1)^1];
				*scanline++ = cb264_palette[((pixels>>1)&1)^1];
				*scanline++ = cb264_palette[(pixels&1)^1];
			}
		}
	}
	else
	{
		fatalerror("cb264: unknown video mode %d\n", cb264_mode);
	}

	return 0;
}

static WRITE32_HANDLER( cb264_mode_w )
{
	cb264_mode = data;
}

static UINT32 cb264_toggle = 0;
static READ32_HANDLER( cb264_status_r )
{
	cb264_toggle ^= 1;
	return cb264_toggle;	// probably bit 0 is vblank
}

static UINT32 cb264_colors[3], cb264_count, cb264_clutoffs;
static WRITE32_HANDLER( cb264_ramdac_w )
{
	if (!offset)
	{
		cb264_clutoffs = data>>24;
		cb264_count = 0;
	}
	else
	{
		cb264_colors[cb264_count++] = data>>24;

		if (cb264_count == 3)
		{
			cb264_count = 0;
			cb264_clutoffs++;
			palette_set_color(space->machine, cb264_clutoffs, MAKE_RGB(cb264_colors[0], cb264_colors[1], cb264_colors[2]));
			cb264_palette[cb264_clutoffs] = MAKE_RGB(cb264_colors[0], cb264_colors[1], cb264_colors[2]);
		}
	}
}

// IIci/IIsi RAM-Based Video (RBV)

// 512x384x1 framebuffer at fb008000
// 640x480x1 framebuffer at fee00000

static UINT32 *rbv_vram;

static UINT32 rbv_colors[3], rbv_count, rbv_clutoffs;
static UINT32 rbv_palette[256];

static VIDEO_UPDATE( macrbv )
{
	UINT32 *scanline;
	int x, y;
	UINT8 *vram8 = (UINT8 *)rbv_vram;
	UINT8 pixels;

	for (y = 0; y < 480; y++)
	{
		scanline = BITMAP_ADDR32(bitmap, y, 0);
		for (x = 0; x < 640; x+=8)
		{
			pixels = vram8[(y * 1024) + ((x/8)^3)];

			*scanline++ = rbv_palette[(pixels>>7)^1];
			*scanline++ = rbv_palette[((pixels>>6)&1)^1];
			*scanline++ = rbv_palette[((pixels>>5)&1)^1];
			*scanline++ = rbv_palette[((pixels>>4)&1)^1];
			*scanline++ = rbv_palette[((pixels>>3)&1)^1];
			*scanline++ = rbv_palette[((pixels>>2)&1)^1];
			*scanline++ = rbv_palette[((pixels>>1)&1)^1];
			*scanline++ = rbv_palette[(pixels&1)^1];
		}
	}

	return 0;
}

static WRITE32_HANDLER( rbv_ramdac_w )
{
	if (!offset)
	{
		rbv_clutoffs = data>>24;
		rbv_count = 0;
	}
	else
	{
		rbv_colors[rbv_count++] = data>>24;

		if (rbv_count == 3)
		{
			rbv_count = 0;
			rbv_clutoffs++;
			palette_set_color(space->machine, rbv_clutoffs, MAKE_RGB(rbv_colors[0], rbv_colors[1], rbv_colors[2]));
			rbv_palette[rbv_clutoffs] = MAKE_RGB(rbv_colors[0], rbv_colors[1], rbv_colors[2]);
		}
	}
}


static UINT32 rbv_toggle = 0;
READ16_HANDLER ( mac_rbv_r )
{
	int data;
	const device_config *via_1 = devtag_get_device(space->machine, "via6522_1");

//	logerror("rbv_r: %x, mask %x\n", offset, mem_mask);

	if (offset == 1)
	{
		rbv_toggle ^= 0x4040;
		return rbv_toggle;
	}

	if (offset == 0x0010)
	{
		logerror("RBV: Read monitor type (PC=%x)\n", cpu_get_pc(space->cpu));

		data = (6<<3) | 3;	// 13" RGB, 8 bits per pixel

		return (data & 0xff) | (data << 8);
	}

	offset >>= 8;
	offset &= 0x0f;

	data = via_r(via_1, offset);

	return (data & 0xff) | (data << 8);
}

WRITE16_HANDLER ( mac_rbv_w )
{
	const device_config *via_1 = devtag_get_device(space->machine, "via6522_1");

//	logerror("rbv_w: %x to offset %x, mask %x\n", data, offset, mem_mask);

	offset >>= 8;
	offset &= 0x0f;

	if (ACCESSING_BITS_8_15)
		via_w(via_1, offset, (data >> 8) & 0xff);
}

/***************************************************************************
    ADDRESS MAPS
***************************************************************************/

static ADDRESS_MAP_START(mac512ke_map, ADDRESS_SPACE_PROGRAM, 16)
	AM_RANGE(0x800000, 0x9fffff) AM_READ(mac_scc_r)
	AM_RANGE(0xa00000, 0xbfffff) AM_WRITE(mac_scc_w)
	AM_RANGE(0xc00000, 0xdfffff) AM_READWRITE(mac_iwm_r, mac_iwm_w)
	AM_RANGE(0xe80000, 0xefffff) AM_READWRITE(mac_via_r, mac_via_w)
	AM_RANGE(0xfffff0, 0xffffff) AM_READWRITE(mac_autovector_r, mac_autovector_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START(macplus_map, ADDRESS_SPACE_PROGRAM, 16)
	AM_RANGE(0x580000, 0x5fffff) AM_READWRITE(macplus_scsi_r, macplus_scsi_w)
	AM_RANGE(0x800000, 0x9fffff) AM_READ(mac_scc_r)
	AM_RANGE(0xa00000, 0xbfffff) AM_WRITE(mac_scc_w)
	AM_RANGE(0xc00000, 0xdfffff) AM_READWRITE(mac_iwm_r, mac_iwm_w)
	AM_RANGE(0xe80000, 0xefffff) AM_READWRITE(mac_via_r, mac_via_w)
	AM_RANGE(0xfffff0, 0xffffff) AM_READWRITE(mac_autovector_r, mac_autovector_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START(maclc_map, ADDRESS_SPACE_PROGRAM, 32)
	AM_RANGE(0x00a00000, 0x00a7ffff) AM_ROM AM_REGION("user1", 0)	// ROM (in 32-bit mode)

	AM_RANGE(0x50f00000, 0x50f01fff) AM_READWRITE16(mac_via_r, mac_via_w, 0xffffffff)
	AM_RANGE(0x50f04000, 0x50f05fff) AM_READWRITE16(mac_scc_r, mac_scc_2_w, 0xffffffff)
	// 50f06000-7fff = SCSI handshake

	// 50f10000-1fff = SCSI
	// 50f12000-3fff = SCSI DMA
	AM_RANGE(0x50f14000, 0x50f15fff) AM_READ8(mac_asc_r, 0xffffffff) AM_WRITE8(mac_asc_w, 0xffffffff)
	AM_RANGE(0x50f16000, 0x50f17fff) AM_READ(mac_swim_r) AM_WRITENOP
	// 50f18000-9fff = PWMs

	// 50f24000-5fff = VDAC (palette)
	AM_RANGE(0x50f24000, 0x50f25fff) AM_RAM
	AM_RANGE(0x50f26000, 0x50f27fff) AM_READWRITE16(mac_via2_r, mac_via2_w, 0xffffffff)	// VIA2 (RBV)

	AM_RANGE(0xfeff8000, 0xfeffffff) AM_ROM AM_REGION("user1", 0x78000)
ADDRESS_MAP_END

static ADDRESS_MAP_START(macii_map, ADDRESS_SPACE_PROGRAM, 32)
	AM_RANGE(0x40000000, 0x4003ffff) AM_ROM AM_REGION("user1", 0) AM_MIRROR(0x0ffc0000)

	AM_RANGE(0x50f00000, 0x50f01fff) AM_READWRITE16(mac_via_r, mac_via_w, 0xffffffff)
	AM_RANGE(0x50f02000, 0x50f03fff) AM_READWRITE16(mac_via2_r, mac_via2_w, 0xffffffff)
	AM_RANGE(0x50f04000, 0x50f05fff) AM_READWRITE16(mac_scc_r, mac_scc_2_w, 0xffffffff)

	AM_RANGE(0x50f10000, 0x50f11fff) AM_READWRITE16(macplus_scsi_r, macii_scsi_w, 0xffffffff)
	AM_RANGE(0x50f12060, 0x50f12063) AM_READ(macii_scsi_drq_r)
	AM_RANGE(0x50f14000, 0x50f15fff) AM_READ8(mac_asc_r, 0xffffffff) AM_WRITE8(mac_asc_w, 0xffffffff)
	AM_RANGE(0x50f16000, 0x50f17fff) AM_READ(mac_swim_r) AM_WRITENOP

	AM_RANGE(0x50f40000, 0x50f41fff) AM_READWRITE16(mac_via_r, mac_via_w, 0xffffffff)	// mirror

	// RasterOps 264 640x480 fixed-res color video card (8, 16, or 24 bit)
	AM_RANGE(0xfe000000, 0xfe1fffff) AM_RAM	AM_BASE(&cb264_vram) // supposed to be 1.5 megs of VRAM, but every other word?
	AM_RANGE(0xfeff6018, 0xfeff601b) AM_WRITE( cb264_mode_w )
	AM_RANGE(0xfeff6034, 0xfeff6037) AM_READ( cb264_status_r )
	AM_RANGE(0xfeff7000, 0xfeff7007) AM_WRITE( cb264_ramdac_w )
	AM_RANGE(0xfeff8000, 0xfeffffff) AM_ROM AM_REGION("rops264", 0)
ADDRESS_MAP_END

static ADDRESS_MAP_START(maciici_map, ADDRESS_SPACE_PROGRAM, 32)
	AM_RANGE(0x40000000, 0x4007ffff) AM_ROM AM_REGION("user1", 0) AM_MIRROR(0x0ff80000)

	// MMU remaps I/O without the F
	AM_RANGE(0x50000000, 0x50001fff) AM_READWRITE16(mac_via_r, mac_via_w, 0xffffffff)
	AM_RANGE(0x50004000, 0x50005fff) AM_READWRITE16(mac_scc_r, mac_scc_2_w, 0xffffffff)

	AM_RANGE(0x50010000, 0x50011fff) AM_READWRITE16(macplus_scsi_r, macii_scsi_w, 0xffffffff)
	AM_RANGE(0x50012060, 0x50012063) AM_READ(macii_scsi_drq_r)
	AM_RANGE(0x50014000, 0x50015fff) AM_READ8(mac_asc_r, 0xffffffff) AM_WRITE8(mac_asc_w, 0xffffffff)
	AM_RANGE(0x50016000, 0x50017fff) AM_READ(mac_swim_r) AM_WRITENOP

	AM_RANGE(0x50024000, 0x50024007) AM_WRITE( rbv_ramdac_w )
	AM_RANGE(0x50026000, 0x50027fff) AM_READWRITE16(mac_rbv_r, mac_rbv_w, 0xffffffff)

	AM_RANGE(0x50040000, 0x50041fff) AM_READWRITE16(mac_via_r, mac_via_w, 0xffffffff)	// mirror

	// upper (need to use AM_MIRROR for this)

	AM_RANGE(0x50f00000, 0x50f01fff) AM_READWRITE16(mac_via_r, mac_via_w, 0xffffffff)
	AM_RANGE(0x50f04000, 0x50f05fff) AM_READWRITE16(mac_scc_r, mac_scc_2_w, 0xffffffff)

	AM_RANGE(0x50f10000, 0x50f11fff) AM_READWRITE16(macplus_scsi_r, macii_scsi_w, 0xffffffff)
	AM_RANGE(0x50f12060, 0x50f12063) AM_READ(macii_scsi_drq_r)
	AM_RANGE(0x50f14000, 0x50f15fff) AM_READ8(mac_asc_r, 0xffffffff) AM_WRITE8(mac_asc_w, 0xffffffff)
	AM_RANGE(0x50f16000, 0x50f17fff) AM_READ(mac_swim_r) AM_WRITENOP

	// 50f24000 = VDAC
	AM_RANGE(0x50f26000, 0x50f27fff) AM_READWRITE16(mac_rbv_r, mac_rbv_w, 0xffffffff)
	// 51000000-51ffffff = RBV (framebuffer?)

	AM_RANGE(0x50f40000, 0x50f41fff) AM_READWRITE16(mac_via_r, mac_via_w, 0xffffffff)	// mirror

	// mirror video declaration ROM
	AM_RANGE(0xfee00000, 0xfee7ffff) AM_RAM AM_BASE(&rbv_vram)
	AM_RANGE(0xfeff8000, 0xfeffffff) AM_ROM AM_REGION("user1", 0x78000)
ADDRESS_MAP_END

static ADDRESS_MAP_START(macse30_map, ADDRESS_SPACE_PROGRAM, 32)
	AM_RANGE(0x40000000, 0x4003ffff) AM_ROM AM_REGION("user1", 0) AM_MIRROR(0x0ffc0000)

	AM_RANGE(0x50f00000, 0x50f01fff) AM_READWRITE16(mac_via_r, mac_via_w, 0xffffffff)
	AM_RANGE(0x50f02000, 0x50f03fff) AM_READWRITE16(mac_via2_r, mac_via2_w, 0xffffffff)
	AM_RANGE(0x50f04000, 0x50f05fff) AM_READWRITE16(mac_scc_r, mac_scc_2_w, 0xffffffff)

	AM_RANGE(0x50f10000, 0x50f11fff) AM_READWRITE16(macplus_scsi_r, macii_scsi_w, 0xffffffff)
	AM_RANGE(0x50f12060, 0x50f12063) AM_READ(macii_scsi_drq_r)
	AM_RANGE(0x50f14000, 0x50f15fff) AM_READ8(mac_asc_r, 0xffffffff) AM_WRITE8(mac_asc_w, 0xffffffff)
	AM_RANGE(0x50f16000, 0x50f17fff) AM_READ(mac_swim_r) AM_WRITENOP

	AM_RANGE(0x50f40000, 0x50f41fff) AM_READWRITE16(mac_via_r, mac_via_w, 0xffffffff)	// mirror

	AM_RANGE(0xfe000000, 0xfe00ffff) AM_RAM	AM_BASE(&se30_vram)
	AM_RANGE(0xfeffe000, 0xfeffffff) AM_ROM AM_REGION("se30vrom", 0x0)
ADDRESS_MAP_END

static ADDRESS_MAP_START(macclas2_map, ADDRESS_SPACE_PROGRAM, 32)
	AM_RANGE(0x40000000, 0x4007ffff) AM_ROM AM_REGION("user1", 0) AM_MIRROR(0x0ff80000)

	AM_RANGE(0x50f00000, 0x50f01fff) AM_READWRITE16(mac_via_r, mac_via_w, 0xffffffff)
	AM_RANGE(0x50f02000, 0x50f03fff) AM_READWRITE16(mac_via2_r, mac_via2_w, 0xffffffff)
	AM_RANGE(0x50f04000, 0x50f05fff) AM_READWRITE16(mac_scc_r, mac_scc_2_w, 0xffffffff)

	AM_RANGE(0x50f10000, 0x50f11fff) AM_READWRITE16(macplus_scsi_r, macii_scsi_w, 0xffffffff)
	AM_RANGE(0x50f12060, 0x50f12063) AM_READ(macii_scsi_drq_r)
	AM_RANGE(0x50f14000, 0x50f15fff) AM_READ8(mac_asc_r, 0xffffffff) AM_WRITE8(mac_asc_w, 0xffffffff)
	AM_RANGE(0x50f16000, 0x50f17fff) AM_READ(mac_swim_r) AM_WRITENOP

	AM_RANGE(0x50f40000, 0x50f41fff) AM_READWRITE16(mac_via_r, mac_via_w, 0xffffffff)	// mirror

	// mirror video declaration ROM
	AM_RANGE(0xfeff8000, 0xfeffffff) AM_ROM AM_REGION("user1", 0x78000)
ADDRESS_MAP_END

VIDEO_START( maclc )
{
}

VIDEO_UPDATE( maclc )
{
	return 0;
}


/***************************************************************************
    DEVICE CONFIG
***************************************************************************/

static const applefdc_interface mac_iwm_interface =
{
	sony_set_lines,
	mac_fdc_set_enable_lines,

	sony_read_data,
	sony_write_data,
	sony_read_status
};



/***************************************************************************
    MACHINE DRIVERS
***************************************************************************/
static const floppy_config mac128512_floppy_config = //SONY_FLOPPY_ALLOW400K
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_DRIVE_DS_80,
	FLOPPY_OPTIONS_NAME(apple35_mac),
	DO_NOT_KEEP_GEOMETRY
};


static const floppy_config mac_floppy_config = //SONY_FLOPPY_ALLOW400K | SONY_FLOPPY_ALLOW800K
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_DRIVE_DS_80,
	FLOPPY_OPTIONS_NAME(apple35_mac),
	DO_NOT_KEEP_GEOMETRY
};

static MACHINE_DRIVER_START( mac512ke )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", M68000, 7833600)        /* 7.8336 MHz */
	MDRV_CPU_PROGRAM_MAP(mac512ke_map)
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(60.15)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(1260))
	MDRV_QUANTUM_TIME(HZ(60))

	MDRV_MACHINE_RESET( mac )

    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(MAC_H_TOTAL, MAC_V_TOTAL)
	MDRV_SCREEN_VISIBLE_AREA(0, MAC_H_VIS-1, 0, MAC_V_VIS-1)
	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(mac)

	MDRV_VIDEO_START(mac)
	MDRV_VIDEO_UPDATE(mac)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("custom", MAC_SOUND, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	/* nvram */
	MDRV_NVRAM_HANDLER(mac)

	/* devices */
	MDRV_IWM_ADD("fdc", mac_iwm_interface)
	MDRV_FLOPPY_SONY_2_DRIVES_ADD(mac128512_floppy_config)
	
	MDRV_SCC8530_ADD("scc")
	MDRV_SCC8530_ACK(mac_scc_ack)
	MDRV_VIA6522_ADD("via6522_0", 1000000, mac_via6522_intf)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( macplus )
	MDRV_IMPORT_FROM( mac512ke )
	MDRV_CPU_MODIFY( "maincpu" )
	MDRV_CPU_PROGRAM_MAP(macplus_map)

	MDRV_MACHINE_START(macscsi)

	MDRV_HARDDISK_ADD( "harddisk1" )
	MDRV_HARDDISK_ADD( "harddisk2" )
	
	MDRV_FLOPPY_SONY_2_DRIVES_MODIFY(mac_floppy_config)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( macse )
	MDRV_IMPORT_FROM( macplus )

	MDRV_DEVICE_REMOVE("via6522_0")
	MDRV_VIA6522_ADD("via6522_0", 1000000, mac_via6522_adb_intf)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( mac2fdhd )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", M68020_68851, 7833600*2)
	MDRV_CPU_PROGRAM_MAP(macii_map)
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(60.15)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(1260))
	MDRV_QUANTUM_TIME(HZ(60))

	MDRV_MACHINE_START(macscsi)
	MDRV_MACHINE_RESET( mac )

        /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_RGB32)
	MDRV_SCREEN_SIZE(1024, 768)
	MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
	MDRV_PALETTE_LENGTH(256)

	MDRV_VIDEO_START(cb264)
	MDRV_VIDEO_UPDATE(cb264)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("lspeaker", "rspeaker")

	/* nvram */
	MDRV_NVRAM_HANDLER(mac)

	/* devices */
	MDRV_IWM_ADD("fdc", mac_iwm_interface)
	MDRV_FLOPPY_SONY_2_DRIVES_ADD(mac_floppy_config)
	
	MDRV_SCC8530_ADD("scc")
	MDRV_SCC8530_ACK(mac_scc_ack)
	MDRV_VIA6522_ADD("via6522_0", 1000000, mac_via6522_intf)

	MDRV_DEVICE_REMOVE("via6522_0")
	MDRV_VIA6522_ADD("via6522_0", 1000000, mac_via6522_adb_intf)
	MDRV_VIA6522_ADD("via6522_1", 1000000, mac_via6522_2_intf)

	MDRV_HARDDISK_ADD( "harddisk1" )
	MDRV_HARDDISK_ADD( "harddisk2" )
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( maclc )
	MDRV_IMPORT_FROM( mac2fdhd )

	MDRV_CPU_REPLACE("maincpu", M68020, 7833600*2)
	MDRV_CPU_PROGRAM_MAP(maclc_map)

	MDRV_PALETTE_LENGTH(256)

	MDRV_VIDEO_START(maclc)
	MDRV_VIDEO_UPDATE(maclc)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( maciix )
	MDRV_IMPORT_FROM( mac2fdhd )

	MDRV_CPU_REPLACE("maincpu", M68030, 7833600*2)
	MDRV_CPU_PROGRAM_MAP(macii_map)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( macse30 )
	MDRV_CPU_ADD("maincpu", M68030, 7833600*2)
	MDRV_CPU_PROGRAM_MAP(macse30_map)

	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(60.15)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(1260))
	MDRV_QUANTUM_TIME(HZ(60))

	MDRV_MACHINE_START(macscsi)
	MDRV_MACHINE_RESET( mac )

        /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(MAC_H_TOTAL, MAC_V_TOTAL)
	MDRV_SCREEN_VISIBLE_AREA(0, MAC_H_VIS-1, 0, MAC_V_VIS-1)
	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(mac)

	MDRV_VIDEO_START(mac)
	MDRV_VIDEO_UPDATE(macse30)
				 
	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("lspeaker", "rspeaker")

	/* nvram */
	MDRV_NVRAM_HANDLER(mac)

	/* devices */
	MDRV_IWM_ADD("fdc", mac_iwm_interface)
	MDRV_FLOPPY_SONY_2_DRIVES_ADD(mac_floppy_config)
	
	MDRV_SCC8530_ADD("scc")
	MDRV_SCC8530_ACK(mac_scc_ack)
	MDRV_VIA6522_ADD("via6522_0", 1000000, mac_via6522_intf)

	MDRV_DEVICE_REMOVE("via6522_0")
	MDRV_VIA6522_ADD("via6522_0", 1000000, mac_via6522_adb_intf)
	MDRV_VIA6522_ADD("via6522_1", 1000000, mac_via6522_2_intf)

	MDRV_HARDDISK_ADD( "harddisk1" )
	MDRV_HARDDISK_ADD( "harddisk2" )
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( macclas2 )
	MDRV_IMPORT_FROM( macse30 )

	MDRV_CPU_REPLACE("maincpu", M68030, 7833600*2)
	MDRV_CPU_PROGRAM_MAP(macclas2_map)

	MDRV_VIDEO_START(maclc)
	MDRV_VIDEO_UPDATE(maclc)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( maciici )
	MDRV_IMPORT_FROM( mac2fdhd )

	MDRV_CPU_REPLACE("maincpu", M68030, 7833600*2)
	MDRV_CPU_PROGRAM_MAP(maciici_map)

	MDRV_PALETTE_LENGTH(256)

	MDRV_VIDEO_START(maclc)
	MDRV_VIDEO_UPDATE(macrbv)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( maciisi )
	MDRV_IMPORT_FROM( mac2fdhd )

	MDRV_CPU_REPLACE("maincpu", M68030, 25000000)
	MDRV_CPU_PROGRAM_MAP(maciici_map)

	MDRV_PALETTE_LENGTH(256)

	MDRV_VIDEO_START(maclc)
	MDRV_VIDEO_UPDATE(macrbv)
MACHINE_DRIVER_END

static INPUT_PORTS_START( macplus )
	PORT_START("MOUSE0") /* Mouse - button */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_NAME("Mouse Button") PORT_CODE(MOUSECODE_BUTTON1)

	PORT_START("MOUSE1") /* Mouse - X AXIS */
	PORT_BIT( 0xff, 0x00, IPT_MOUSE_X) PORT_SENSITIVITY(100) PORT_KEYDELTA(0) PORT_PLAYER(1)

	PORT_START("MOUSE2") /* Mouse - Y AXIS */
	PORT_BIT( 0xff, 0x00, IPT_MOUSE_Y) PORT_SENSITIVITY(100) PORT_KEYDELTA(0) PORT_PLAYER(1)

	/* R Nabet 000531 : pseudo-input ports with keyboard layout */
	/* we only define US layout for keyboard - international layout is different! */
	/* note : 16 bits at most per port! */

	/* main keyboard pad */

	PORT_START("KEY0")
	PORT_BIT(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_A) 			PORT_CHAR('a') PORT_CHAR('A')
	PORT_BIT(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_S) 			PORT_CHAR('s') PORT_CHAR('S')
	PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_D) 			PORT_CHAR('d') PORT_CHAR('D')
	PORT_BIT(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_F) 			PORT_CHAR('f') PORT_CHAR('F')
	PORT_BIT(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_H) 			PORT_CHAR('h') PORT_CHAR('H')
	PORT_BIT(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_G) 			PORT_CHAR('g') PORT_CHAR('G')
	PORT_BIT(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_Z) 			PORT_CHAR('z') PORT_CHAR('Z')
	PORT_BIT(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_X) 			PORT_CHAR('x') PORT_CHAR('X')
	PORT_BIT(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_C) 			PORT_CHAR('c') PORT_CHAR('C')
	PORT_BIT(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_V) 			PORT_CHAR('v') PORT_CHAR('V')
	PORT_BIT(0x0400, IP_ACTIVE_HIGH, IPT_UNUSED)	/* extra key on ISO : */
	PORT_BIT(0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_B) 			PORT_CHAR('b') PORT_CHAR('B')
	PORT_BIT(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_Q) 			PORT_CHAR('q') PORT_CHAR('Q')
	PORT_BIT(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_W) 			PORT_CHAR('w') PORT_CHAR('W')
	PORT_BIT(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_E) 			PORT_CHAR('e') PORT_CHAR('E')
	PORT_BIT(0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_R) 			PORT_CHAR('r') PORT_CHAR('R')

	PORT_START("KEY1")
	PORT_BIT(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_Y) 			PORT_CHAR('y') PORT_CHAR('Y')
	PORT_BIT(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_T) 			PORT_CHAR('t') PORT_CHAR('T')
	PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_1) 			PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_2) 			PORT_CHAR('2') PORT_CHAR('@')
	PORT_BIT(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_3) 			PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_4) 			PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_6) 			PORT_CHAR('6') PORT_CHAR('^')
	PORT_BIT(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_5) 			PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_EQUALS)		PORT_CHAR('=') PORT_CHAR('+')
	PORT_BIT(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_9) 			PORT_CHAR('9') PORT_CHAR('(')
	PORT_BIT(0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_7) 			PORT_CHAR('7') PORT_CHAR('&')
	PORT_BIT(0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_MINUS)			PORT_CHAR('-') PORT_CHAR('_')
	PORT_BIT(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_8) 			PORT_CHAR('8') PORT_CHAR('*')
	PORT_BIT(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_0) 			PORT_CHAR('0') PORT_CHAR(')')
	PORT_BIT(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_CLOSEBRACE)	PORT_CHAR(']') PORT_CHAR('}')
	PORT_BIT(0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_O) 			PORT_CHAR('o') PORT_CHAR('O')

	PORT_START("KEY2")
	PORT_BIT(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_U)				PORT_CHAR('u') PORT_CHAR('U')
	PORT_BIT(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_OPENBRACE)		PORT_CHAR('[') PORT_CHAR('{')
	PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_I) 			PORT_CHAR('i') PORT_CHAR('I')
	PORT_BIT(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_P) 			PORT_CHAR('p') PORT_CHAR('P')
	PORT_BIT(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Return") PORT_CODE(KEYCODE_ENTER) PORT_CHAR('\r')
	PORT_BIT(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_L) 			PORT_CHAR('l') PORT_CHAR('L')
	PORT_BIT(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_J) 			PORT_CHAR('j') PORT_CHAR('J')
	PORT_BIT(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_QUOTE)			PORT_CHAR('\'') PORT_CHAR('"')
	PORT_BIT(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_K) 			PORT_CHAR('k') PORT_CHAR('K')
	PORT_BIT(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_COLON)			PORT_CHAR(';') PORT_CHAR(':')
	PORT_BIT(0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_BACKSLASH)		PORT_CHAR('\\') PORT_CHAR('|')
	PORT_BIT(0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_COMMA) 		PORT_CHAR(',') PORT_CHAR('<')
	PORT_BIT(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_SLASH)			PORT_CHAR('/') PORT_CHAR('?')
	PORT_BIT(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_N) 			PORT_CHAR('n') PORT_CHAR('N')
	PORT_BIT(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_M) 			PORT_CHAR('m') PORT_CHAR('M')
	PORT_BIT(0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_STOP)			PORT_CHAR('.') PORT_CHAR('>')

	PORT_START("KEY3")
	PORT_BIT(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_TAB) 			PORT_CHAR('\t')
	PORT_BIT(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_SPACE)			PORT_CHAR(' ')
	PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_TILDE)			PORT_CHAR('`') PORT_CHAR('~')
	PORT_BIT(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_BACKSPACE)		PORT_CHAR(8)
	PORT_BIT(0x0010, IP_ACTIVE_HIGH, IPT_UNUSED)	/* keyboard Enter : */
	PORT_BIT(0x0020, IP_ACTIVE_HIGH, IPT_UNUSED)	/* escape: */
	PORT_BIT(0x0040, IP_ACTIVE_HIGH, IPT_UNUSED)	/* ??? */
	PORT_BIT(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Command") PORT_CODE(KEYCODE_LCONTROL)
	PORT_BIT(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Shift") PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Caps Lock") PORT_CODE(KEYCODE_CAPSLOCK) PORT_TOGGLE
	PORT_BIT(0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Option") PORT_CODE(KEYCODE_LALT) PORT_CHAR(UCHAR_SHIFT_2)
	PORT_BIT(0x0800, IP_ACTIVE_HIGH, IPT_UNUSED)	/* Control: */
	PORT_BIT(0x1000, IP_ACTIVE_HIGH, IPT_UNUSED)	/* keypad pseudo-keycode */
	PORT_BIT(0xE000, IP_ACTIVE_HIGH, IPT_UNUSED)	/* ??? */

	/* keypad */
	PORT_START("KEY4")
	PORT_BIT(0x0001, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_DEL_PAD)			PORT_CHAR(UCHAR_MAMEKEY(DEL_PAD))
	PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_ASTERISK)			PORT_CHAR(UCHAR_MAMEKEY(ASTERISK))
	PORT_BIT(0x0038, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_PLUS_PAD)			PORT_CHAR(UCHAR_MAMEKEY(PLUS_PAD))
	PORT_BIT(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Keypad Clear") PORT_CODE(/*KEYCODE_NUMLOCK*/KEYCODE_DEL) PORT_CHAR(UCHAR_MAMEKEY(DEL))
	PORT_BIT(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Keypad =") PORT_CODE(/*CODE_OTHER*/KEYCODE_NUMLOCK) PORT_CHAR(UCHAR_MAMEKEY(NUMLOCK))
	PORT_BIT(0x0E00, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_ENTER_PAD)			PORT_CHAR(UCHAR_MAMEKEY(ENTER_PAD))
	PORT_BIT(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_SLASH_PAD)			PORT_CHAR(UCHAR_MAMEKEY(SLASH_PAD))
	PORT_BIT(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_MINUS_PAD)			PORT_CHAR(UCHAR_MAMEKEY(MINUS_PAD))
	PORT_BIT(0x8000, IP_ACTIVE_HIGH, IPT_UNUSED)

	PORT_START("KEY5")
	PORT_BIT(0x0001, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x0002, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_0_PAD) 			PORT_CHAR(UCHAR_MAMEKEY(0_PAD))
	PORT_BIT(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_1_PAD) 			PORT_CHAR(UCHAR_MAMEKEY(1_PAD))
	PORT_BIT(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_2_PAD) 			PORT_CHAR(UCHAR_MAMEKEY(2_PAD))
	PORT_BIT(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_3_PAD) 			PORT_CHAR(UCHAR_MAMEKEY(3_PAD))
	PORT_BIT(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_4_PAD) 			PORT_CHAR(UCHAR_MAMEKEY(4_PAD))
	PORT_BIT(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_5_PAD) 			PORT_CHAR(UCHAR_MAMEKEY(5_PAD))
	PORT_BIT(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_6_PAD) 			PORT_CHAR(UCHAR_MAMEKEY(6_PAD))
	PORT_BIT(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_7_PAD) 			PORT_CHAR(UCHAR_MAMEKEY(7_PAD))
	PORT_BIT(0x0400, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_8_PAD) 			PORT_CHAR(UCHAR_MAMEKEY(8_PAD))
	PORT_BIT(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_9_PAD) 			PORT_CHAR(UCHAR_MAMEKEY(9_PAD))
	PORT_BIT(0xE000, IP_ACTIVE_HIGH, IPT_UNUSED)

	/* Arrow keys */
	PORT_START("KEY6")
	PORT_BIT(0x0003, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Right Arrow") PORT_CODE(KEYCODE_RIGHT)	PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_BIT(0x0038, IP_ACTIVE_HIGH, IPT_UNUSED	)
	PORT_BIT(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Left Arrow") PORT_CODE(KEYCODE_LEFT)		PORT_CHAR(UCHAR_MAMEKEY(LEFT))
	PORT_BIT(0x0080, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Down Arrow") PORT_CODE(KEYCODE_DOWN)		PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	PORT_BIT(0x1E00, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Up Arrow") PORT_CODE(KEYCODE_UP)			PORT_CHAR(UCHAR_MAMEKEY(UP))
	PORT_BIT(0xC000, IP_ACTIVE_HIGH, IPT_UNUSED)
INPUT_PORTS_END



/***************************************************************************

  Game driver(s)

  The Mac driver uses a convention of placing the BIOS in "user1"

***************************************************************************/

ROM_START( mac128k )
	ROM_REGION16_BE(0x20000, "user1", 0)
	ROM_LOAD16_WORD( "mac128k.rom",  0x00000, 0x10000, CRC(6d0c8a28) SHA1(9d86c883aa09f7ef5f086d9e32330ef85f1bc93b) )
ROM_END

ROM_START( mac512k )
	ROM_REGION16_BE(0x20000, "user1", 0)
	ROM_LOAD16_WORD( "mac512k.rom",  0x00000, 0x10000, CRC(cf759e0d) SHA1(5b1ced181b74cecd3834c49c2a4aa1d7ffe944d7) )
ROM_END

ROM_START( mac512ke )
	ROM_REGION16_BE(0x20000, "user1", 0)
	ROM_LOAD16_WORD( "macplus.rom",  0x00000, 0x20000, CRC(b2102e8e) SHA1(7d2f808a045aa3a1b242764f0e2c7d13e288bf1f))
ROM_END


ROM_START( macplus )
	ROM_REGION16_BE(0x20000, "user1", 0)
	ROM_LOAD16_WORD( "macplus.rom",  0x00000, 0x20000, CRC(b2102e8e) SHA1(7d2f808a045aa3a1b242764f0e2c7d13e288bf1f))
ROM_END


ROM_START( macse )
	ROM_REGION16_BE(0x40000, "user1", 0)
	ROM_LOAD16_WORD( "macse.rom",  0x00000, 0x40000, CRC(0f7ff80c) SHA1(58532b7d0d49659fd5228ac334a1b094f0241968))
ROM_END

ROM_START( macclasc )
	ROM_REGION16_BE(0x40000, "user1", 0)
	ROM_LOAD16_WORD( "classic.rom",  0x00000, 0x40000, CRC(b14ddcde) SHA1(f710e73e8e0f99d9d0e9e79e71f67a6c3648bf06) )
ROM_END

ROM_START( maclc )
	ROM_REGION32_BE(0x80000, "user1", 0)
        ROM_LOAD("350eacf0.rom", 0x000000, 0x080000, CRC(71681726) SHA1(6bef5853ae736f3f06c2b4e79772f65910c3b7d4))
ROM_END

ROM_START( mac2fdhd )	// same ROM for II FDHD, IIx, IIcx, and SE/30
	ROM_REGION32_BE(0x40000, "user1", 0)
        ROM_LOAD( "97221136.rom", 0x000000, 0x040000, CRC(ce3b966f) SHA1(753b94351d94c369616c2c87b19d568dc5e2764e) )

	// RasterOps "ColorBoard 264" NuBus video card
	ROM_REGION32_BE(0x8000, "rops264", 0)
        ROM_LOAD32_BYTE( "264-1914.bin", 0x000003, 0x002000, CRC(d5fbd5ad) SHA1(98d35ed3fb0bca4a9bee1cdb2af0d3f22b379386) )
        ROM_LOAD32_BYTE( "264-1915.bin", 0x000002, 0x002000, CRC(26c19ee5) SHA1(2b2853d04cc6b0258e85eccd23ebfd4f4f63a084) )
ROM_END

ROM_START( maciix )
	ROM_REGION32_BE(0x40000, "user1", 0)
        ROM_LOAD( "97221136.rom", 0x000000, 0x040000, CRC(ce3b966f) SHA1(753b94351d94c369616c2c87b19d568dc5e2764e) )

	// RasterOps "ColorBoard 264" NuBus video card
	ROM_REGION32_BE(0x8000, "rops264", 0)
        ROM_LOAD32_BYTE( "264-1914.bin", 0x000003, 0x002000, CRC(d5fbd5ad) SHA1(98d35ed3fb0bca4a9bee1cdb2af0d3f22b379386) )
        ROM_LOAD32_BYTE( "264-1915.bin", 0x000002, 0x002000, CRC(26c19ee5) SHA1(2b2853d04cc6b0258e85eccd23ebfd4f4f63a084) )
ROM_END

ROM_START( maciicx )
	ROM_REGION32_BE(0x40000, "user1", 0)
        ROM_LOAD( "97221136.rom", 0x000000, 0x040000, CRC(ce3b966f) SHA1(753b94351d94c369616c2c87b19d568dc5e2764e) )

	// RasterOps "ColorBoard 264" NuBus video card
	ROM_REGION32_BE(0x8000, "rops264", 0)
        ROM_LOAD32_BYTE( "264-1914.bin", 0x000003, 0x002000, CRC(d5fbd5ad) SHA1(98d35ed3fb0bca4a9bee1cdb2af0d3f22b379386) )
        ROM_LOAD32_BYTE( "264-1915.bin", 0x000002, 0x002000, CRC(26c19ee5) SHA1(2b2853d04cc6b0258e85eccd23ebfd4f4f63a084) )
ROM_END

ROM_START( macse30 )
	ROM_REGION32_BE(0x40000, "user1", 0)
        ROM_LOAD( "97221136.rom", 0x000000, 0x040000, CRC(ce3b966f) SHA1(753b94351d94c369616c2c87b19d568dc5e2764e) )

	ROM_REGION32_BE(0x2000, "se30vrom", 0)
	ROM_LOAD( "se30vrom.uk6", 0x000000, 0x002000, CRC(b74c3463) SHA1(584201cc67d9452b2488f7aaaf91619ed8ce8f03) )
ROM_END

ROM_START( maciici )
	ROM_REGION32_BE(0x80000, "user1", 0)
        ROM_LOAD( "368cadfe.rom", 0x000000, 0x080000, CRC(46adbf74) SHA1(b54f9d2ed16b63c49ed55adbe4685ebe73eb6e80) )
ROM_END

ROM_START( maciisi )
	ROM_REGION32_BE(0x80000, "user1", 0)
        ROM_LOAD( "36b7fb6c.rom", 0x000000, 0x080000, CRC(f304d973) SHA1(f923de4125aae810796527ff6e25364cf1d54eec) )
ROM_END

ROM_START( macclas2 )
	ROM_REGION32_BE(0x80000, "user1", 0)
        ROM_LOAD( "3193670e.rom", 0x000000, 0x080000, CRC(96d2e1fd) SHA1(50df69c1b6e805e12a405dc610bc2a1471b2eac2) )
ROM_END

ROM_START( maclc2 )
	ROM_REGION32_BE(0x80000, "user1", 0)
	ROM_LOAD( "35c28f5f.rom", 0x000000, 0x080000, CRC(a92145b3) SHA1(d5786182b62a8ffeeb9fd3f80b5511dba70318a0) )
ROM_END

static SYSTEM_CONFIG_START(mac128k)
	CONFIG_RAM_DEFAULT(0x020000)
SYSTEM_CONFIG_END

static SYSTEM_CONFIG_START(mac512k)
	CONFIG_RAM_DEFAULT(0x080000)
SYSTEM_CONFIG_END

static SYSTEM_CONFIG_START(macplus)
	CONFIG_RAM			(0x080000)
	CONFIG_RAM_DEFAULT	(0x100000)
	CONFIG_RAM			(0x200000)
	CONFIG_RAM			(0x280000)
	CONFIG_RAM			(0x400000)
SYSTEM_CONFIG_END

static SYSTEM_CONFIG_START(macse)
	CONFIG_RAM_DEFAULT	(0x100000)
	CONFIG_RAM			(0x200000)
	CONFIG_RAM			(0x280000)
	CONFIG_RAM			(0x400000)
SYSTEM_CONFIG_END

static SYSTEM_CONFIG_START(maclc)
	CONFIG_RAM_DEFAULT	(0x200000)	// 2 MB RAM default
	CONFIG_RAM			(0x400000)
	CONFIG_RAM			(0x600000)
	CONFIG_RAM			(0x800000)
	CONFIG_RAM			(0xa00000)	// up to 10 MB
SYSTEM_CONFIG_END

static SYSTEM_CONFIG_START(macclas2)
	CONFIG_RAM_DEFAULT		(0x400000)	// 4MB default
	CONFIG_RAM			(0x600000)
	CONFIG_RAM			(0x800000)
	CONFIG_RAM			(0xa00000)	// up to 10 MB
SYSTEM_CONFIG_END


/*    YEAR  NAME      PARENT    COMPAT  MACHINE   INPUT     INIT	 CONFIG		COMPANY			FULLNAME */
COMP( 1984, mac128k,  0, 	0,	mac512ke, macplus,  mac128k512k, mac128k,	"Apple Computer",	"Macintosh 128k",  GAME_NOT_WORKING )
COMP( 1984, mac512k,  mac128k,  0,	mac512ke, macplus,  mac128k512k, mac512k,	"Apple Computer",	"Macintosh 512k",  GAME_NOT_WORKING )
COMP( 1986, mac512ke, macplus,  0,	mac512ke, macplus,  mac512ke,	 mac512k,	"Apple Computer",	"Macintosh 512ke", 0 )
COMP( 1986, macplus,  0,	0,	macplus,  macplus,  macplus,	 macplus,	"Apple Computer",	"Macintosh Plus",  0 )
COMP( 1987, macse,    0,	0,	macse,    macplus,  macse,	 macse,		"Apple Computer",	"Macintosh SE",  0 )
COMP( 1988, mac2fdhd, 0,	0,	mac2fdhd, macplus,  maciifdhd,	 maclc,		"Apple Computer",	"Macintosh II (FDHD)",  GAME_NOT_WORKING )
COMP( 1988, maciix,   mac2fdhd, 0,	maciix,   macplus,  maciix,	 maclc,		"Apple Computer",	"Macintosh IIx",  GAME_NOT_WORKING )
COMP( 1989, macse30,  mac2fdhd, 0,	macse30,  macplus,  macse30,	 maclc,		"Apple Computer",	"Macintosh SE/30",  GAME_NOT_WORKING )
COMP( 1989, maciicx,  mac2fdhd, 0,	maciix,   macplus,  maciicx,	 maclc,		"Apple Computer",	"Macintosh IIcx",  GAME_NOT_WORKING )
COMP( 1989, maciici,  0,	0,	maciici,  macplus,  maciici,	 macclas2, 	"Apple Computer",	"Macintosh IIci",  GAME_NOT_WORKING )
COMP( 1990, macclasc, 0,	0,	macse,    macplus,  macclassic,	 macse,		"Apple Computer",	"Macintosh Classic",  GAME_NOT_WORKING )
COMP( 1990, maclc,    0,	0,	maclc,    macplus,  maclc,	 maclc,		"Apple Computer",	"Macintosh LC",  GAME_NOT_WORKING )
COMP( 1990, maciisi,  0,	0,	maciisi,  macplus,  maciici,	 macclas2,	"Apple Computer",	"Macintosh IIsi",  GAME_NOT_WORKING )
COMP( 1991, macclas2, 0,	0,	macclas2, macplus,  macclassic2, macclas2,    	"Apple Computer",	"Macintosh Classic II",  GAME_NOT_WORKING )
COMP( 1991, maclc2,   0,	0,	maclc,    macplus,  maclc2,	 macclas2,      "Apple Computer",	"Macintosh LC II",  GAME_NOT_WORKING )

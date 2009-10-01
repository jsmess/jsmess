/*

    Fujitsu FM-Towns

    Japanese computer system released in 1989.

    CPU:  AMD 80386SX(DX?) (80387 available as add-on?)
    Sound:  Yamaha YM3438
            Ricoh RF5c68
    Video:  VGA + some custom extra video hardware
            320x200 - 640x480
            16 - 32768 colours from a possible palette of between 4096 and
              16.7m colours (depending on video mode)
            1024 sprites (16x16)


    Fujitsu FM-Towns Marty

    Japanese console, based on the FM-Towns computer, using an AMD 80386SX CPU,
    released in 1993


    16/5/09:  Skeleton driver.

    Issues: BIOS requires 386 protected mode.

*/

/* I/O port map (incomplete, could well be incorrect too)
 *
 * 0x0000   : Master 8259 PIC
 * 0x0002   : Master 8259 PIC
 * 0x0010   : Slave 8259 PIC
 * 0x0012   : Slave 8259 PIC
 * 0x0020 RW: bit 0 = soft reset (read/write), bit 6 = power off (write), bit 7 = NMI vector protect
 * 0x0022  W: bit 7 = power off (write)
 * 0x0025 R : returns 0x00? (read)
 * 0x0026 R : timer?
 * 0x0028 RW: bit 0 = NMI mask (read/write)
 * 0x0030 R : Machine ID (low)
 * 0x0031 R : Machine ID (high)
 * 0x0032 RW: bit 7 = RESET, bit 6 = CLK (serial?)
 * 0x0040   : 8253 PIT counter 0
 * 0x0042   : 8253 PIT counter 1
 * 0x0044   : 8253 PIT counter 2
 * 0x0046   : 8253 PIT mode port
 * 0x0060   : 8253 PIT ???
 * 0x006c RW: returns 0x00? (read) timer? (write)
 * 0x00a0-af: DMA controller 1 (uPD71071)
 * 0x00b0-bf: DMA controller 2 (uPD71071)
 * 0x0200-0f: Floppy controller (unknown)
 * 0x0400   : Video / CRTC (unknown)
 * 0x0404   : Video / CRTC (unknown)
 * 0x0440-5f: Video / CRTC (unknown)
 * 0x0480 RW: bit 1 = some sort of RAM switch?
 * 0x04c0-cf: CD-ROM controller (unknown? SCSI?)
 * 0x04d5   : Sound mute
 * 0x04d8   : YM3438 control port A / status
 * 0x04da   : YM3438 data port A / status
 * 0x04dc   : YM3438 control port B / status
 * 0x04de   : YM3438 data port B / status
 * 0x04e0-e3: volume ports
 * 0x04e9-ec: IRQ masks
 * 0x04f0-f8: RF5c68 registers
 * 0x05e8 R : RAM size in MB
 * 0x05ec RW: bit 0 = compatibility mode?
 * 0x0600 RW: Keyboard data port (8042)
 * 0x0602   : Keyboard control port (8042)
 * 0x0604   : (8042)
 * 0x3000 - 0x3fff : CMOS RAM
 * 0xfd90-a0: CRTC / Video
 * 0xff81: CRTC / Video - returns value in RAM location 0xcff81?
 */

#include "driver.h"
#include "cpu/i386/i386.h"
#include "sound/2612intf.h"
#include "sound/rf5c68.h"
#include "machine/pit8253.h"
#include "machine/pic8259.h"

static UINT8 ftimer;
static UINT8 nmi_mask;
static UINT8 compat_mode;
static UINT8 towns_system_port;
static UINT32* towns_vram;
static UINT32* towns_cmos;
static UINT32* towns_gfxvram;
static UINT32* towns_txtvram;

static READ8_HANDLER(towns_system_r)
{
	switch(offset)
	{
		case 0x00:
			logerror("SYS: port 0x20 read\n");
			return 0x00;
		case 0x05:
			logerror("SYS: port 0x25 read\n");
			return 0x00;
		case 0x06:
			logerror("SYS: (0x26) timer read\n");
			ftimer -= 0x13;
			return ftimer;
		case 0x08:
			logerror("SYS: (0x28) NMI mask read\n");
			return nmi_mask & 0x01;
		case 0x10:
			logerror("SYS: (0x30) Machine ID read\n");
			return 0x01;
		case 0x11:
			logerror("SYS: (0x31) Machine ID read\n");
			return 0x01;
		case 0x12:
			logerror("SYS: (0x32) Serial(?) read\n");
			return 0x00;
		default:
			logerror("SYS: Unknown system port read (0x%02x)\n",offset);
			return 0x00;
	}
}

static WRITE8_HANDLER(towns_system_w)
{
	switch(offset)
	{
		case 0x00:
			logerror("SYS: port 0x20 write %02x\n",data);
			break;
		case 0x02:
			logerror("SYS: (0x22) power port write %02x\n",data);
			break;
		case 0x08:
			logerror("SYS: (0x28) NMI mask write %02x\n",data);
			nmi_mask = data & 0x01;
			break;
		case 0x12:
			logerror("SYS: (0x32) Serial(?) write %02x\n",data);
			break;
		default:
			logerror("SYS: Unknown system port write 0x%02x (0x%02x)\n",data,offset);
	}
}

static READ8_HANDLER(towns_sys6c_r)
{
	logerror("SYS: (0x6c) Timer? read\n");
	return 0x00;
}

static WRITE8_HANDLER(towns_sys6c_w)
{
	logerror("SYS: (0x6c) write to timer (0x%02x)\n",data);
	ftimer -= 0x54;
}

static READ8_HANDLER(towns_dma1_r)
{
	logerror("DMA#1: read register %i\n",offset);
	return 0x00;
}

static WRITE8_HANDLER(towns_dma1_w)
{
	logerror("DMA#1: wrote 0x%02x to register %i\n",data,offset);
}

static READ8_HANDLER(towns_dma2_r)
{
	logerror("DMA#2: read register %i\n",offset);
	return 0x00;
}

static WRITE8_HANDLER(towns_dma2_w)
{
	logerror("DMA#2: wrote 0x%02x to register %i\n",data,offset);
}

static READ8_HANDLER(towns_floppy_r)
{
	logerror("PCM: read register %i\n",offset);
	return 0x00;
}

static WRITE8_HANDLER(towns_floppy_w)
{
	logerror("PCM: wrote 0x%02x to register %i\n",data,offset);
}

static READ8_HANDLER(towns_video_440_r)
{
	logerror("VID: read port %04x\n",offset+0x440);
	return 0x00;
}

static WRITE8_HANDLER(towns_video_440_w)
{
	logerror("VID: wrote 0x%02x to port %04x\n",data,offset+0x440);
}

static READ8_HANDLER(towns_video_5c8_r)
{
	logerror("VID: read port %04x\n",offset+0x5c8);
	return 0x00;
}

static WRITE8_HANDLER(towns_video_5c8_w)
{
	logerror("VID: wrote 0x%02x to port %04x\n",data,offset+0x5c8);
}

static READ8_HANDLER(towns_video_fd90_r)
{
	logerror("VID: read port %04x\n",offset+0xfd90);
	return 0x00;
}

static WRITE8_HANDLER(towns_video_fd90_w)
{
	logerror("VID: wrote 0x%02x to port %04x\n",data,offset+0xfd90);
}

static READ8_HANDLER(towns_video_ff81_r)
{
	logerror("VID: read port ff81\n");
	return 0x00;
}

static WRITE8_HANDLER(towns_video_ff81_w)
{
	logerror("VID: wrote 0x%02x to port ff81\n",data);
}

static READ8_HANDLER(towns_keyboard_r)
{
	logerror("KB: read offset %02x\n",offset);
	return 0x00;
}

static WRITE8_HANDLER(towns_keyboard_w)
{
	logerror("KB: wrote 0x%02x to offset %02x\n",data,offset);
}

static READ8_HANDLER(towns_port60_r)
{
	UINT8 val = 0x00;
	if ( pit8253_get_output(devtag_get_device(space->machine, "pit"), 2 ) )
		val |= 0x20;
	else
		val &= ~0x20;

	logerror("PIT: port 0x60 read, returning 0x%02x\n",val);
	return val;
}

static WRITE8_HANDLER(towns_port60_w)
{
	pit8253_gate_w(devtag_get_device(space->machine, "pit"), 2, data & 1);
	logerror("PIT: wrote 0x%02x to port 0x60\n",data);
}

static READ32_HANDLER(towns_sys5e8_r)
{
	switch(offset)
	{
		case 0x00:
			if(ACCESSING_BITS_0_7)
			{
				logerror("SYS: read RAM size port\n");
				return 0x06;  // 6MB is standard for the Marty
			}
			break;
		case 0x01:
			if(ACCESSING_BITS_0_7)
			{
				logerror("SYS: read port 5ec\n");
				return compat_mode & 0x01;
			}
			break;
	}
	return 0x00;
}

static WRITE32_HANDLER(towns_sys5e8_w)
{
	switch(offset)
	{
		case 0x00:
			if(ACCESSING_BITS_0_7)
			{
				logerror("SYS: wrote 0x%02x to port 5e8\n",data);
			}
			break;
		case 0x01:
			if(ACCESSING_BITS_0_7)
			{
				logerror("SYS: wrote 0x%02x to port 5ec\n",data);
				compat_mode = data & 0x01;
			}
			break;
	}
}

static READ32_HANDLER(towns_padport_r)
{
	if(ACCESSING_BITS_0_7)
		return 0x7f;

	return 0x00;
}

static READ8_HANDLER( towns_cmos8_r )
{
	UINT8* cmos = (UINT8*)towns_cmos;

	return cmos[offset];
}

static WRITE8_HANDLER( towns_cmos8_w )
{
	UINT8* cmos = (UINT8*)towns_cmos;

	cmos[offset] = data;
}

static READ32_HANDLER( towns_cmos_r )
{
	return towns_cmos[offset];
}

static WRITE32_HANDLER( towns_cmos_w )
{
	COMBINE_DATA(towns_cmos+offset);
}

static READ8_HANDLER( towns_sys480_r )
{
	if(towns_system_port & 0x02)
		return 0x02;
	else
		return 0x00;
}

static WRITE8_HANDLER( towns_sys480_w )
{
	towns_system_port = data;
}

static ADDRESS_MAP_START(towns_mem, ADDRESS_SPACE_PROGRAM, 32)
  // memory map based on FM-Towns/Bochs (Bochs modified to emulate the FM-Towns)
  // may not be (and probably is not) correct
  AM_RANGE(0x00000000, 0x000bffff) AM_RAM
  AM_RANGE(0x000c0000, 0x000c7fff) AM_RAM AM_BASE(&towns_gfxvram)  // GVRAM
  AM_RANGE(0x000c8000, 0x000cffff) AM_RAM AM_BASE(&towns_txtvram) // TVRAM
//  AM_RANGE(0x000ca000, 0x000cafff) AM_RAM AM_BASE(&towns_txtvram) // TVRAM
  AM_RANGE(0x000d0000, 0x000d7fff) AM_RAM
  AM_RANGE(0x000d8000, 0x000d9fff) AM_READWRITE(towns_cmos_r,towns_cmos_w) // CMOS? RAM
  AM_RANGE(0x000da000, 0x000effff) AM_RAM
  AM_RANGE(0x000f0000, 0x000f7fff) AM_RAM
  AM_RANGE(0x000f8000, 0x000fffff) AM_RAM AM_REGION("user",0x238000)  // BOOT (SYSTEM) ROM
  AM_RANGE(0x00100000, 0x005fffff) AM_RAM  // some extra RAM - seems to be needed to boot
  AM_RANGE(0x80000000, 0x8003ffff) AM_RAM AM_MIRROR(0x1c0000) AM_BASE(&towns_vram) // VRAM
  AM_RANGE(0x81000000, 0x8101ffff) AM_RAM  // Sprite RAM
  AM_RANGE(0xc2000000, 0xc207ffff) AM_ROM AM_REGION("user",0x000000)  // OS ROM
  AM_RANGE(0xc2080000, 0xc20fffff) AM_ROM AM_REGION("user",0x100000)  // DIC ROM
  AM_RANGE(0xc2100000, 0xc213ffff) AM_ROM AM_REGION("user",0x180000)  // FONT ROM
  AM_RANGE(0xc2140000, 0xc2141fff) AM_READWRITE(towns_cmos_r,towns_cmos_w) // CMOS (mirror?)
  AM_RANGE(0xc2200000, 0xc2200fff) AM_NOP  // WAVE RAM
  AM_RANGE(0xfffc0000, 0xffffffff) AM_ROM AM_REGION("user",0x200000)  // SYSTEM ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( towns_io , ADDRESS_SPACE_IO, 32)
  // I/O ports derived from FM Towns/Bochs, these are specific to the FM Towns
  // Some common PC ports are likely to also be used
  // System ports
  AM_RANGE(0x0000,0x0003) AM_DEVREADWRITE8("pic8259_master", pic8259_r, pic8259_w, 0x00ff00ff)
  AM_RANGE(0x0010,0x0013) AM_DEVREADWRITE8("pic8259_slave", pic8259_r, pic8259_w, 0x00ff00ff)
  AM_RANGE(0x0020,0x0033) AM_READWRITE8(towns_system_r,towns_system_w, 0xffffffff)
  AM_RANGE(0x0040,0x0047) AM_DEVREADWRITE8("pit",pit8253_r, pit8253_w, 0x00ff00ff)
  AM_RANGE(0x0060,0x0063) AM_READWRITE8(towns_port60_r, towns_port60_w, 0x000000ff)
  AM_RANGE(0x006c,0x006f) AM_READWRITE8(towns_sys6c_r,towns_sys6c_w, 0x000000ff)
  // DMA controllers (uPD71071?)
  AM_RANGE(0x00a0,0x00af) AM_READWRITE8(towns_dma1_r, towns_dma1_w, 0xffffffff)
  AM_RANGE(0x00b0,0x00bf) AM_READWRITE8(towns_dma2_r, towns_dma2_w, 0xffffffff)
  // Floppy controller
  AM_RANGE(0x0200,0x020f) AM_READWRITE8(towns_floppy_r, towns_floppy_w, 0xffffffff)
  // CRTC / Video
  AM_RANGE(0x0400,0x0403) AM_NOP  // R/O (0x400)
  AM_RANGE(0x0404,0x0407) AM_NOP  // R/W (0x404)
  AM_RANGE(0x0440,0x045f) AM_READWRITE8(towns_video_440_r, towns_video_440_w, 0xffffffff)
  // System port
  AM_RANGE(0x0480,0x0483) AM_READWRITE8(towns_sys480_r,towns_sys480_w,0xff000000)  // R/W (0x480)
  // CD-ROM
  AM_RANGE(0x04c0,0x04cf) AM_NOP
  // Joystick / Mouse ports(?)
  AM_RANGE(0x04d0,0x04d3) AM_READ(towns_padport_r)
  // Sound (YM3438 [FM], RF5c68 [PCM])
  AM_RANGE(0x04d4,0x04d7) AM_NOP  // R/W  -- (0x4d5) mute?
  AM_RANGE(0x04d8,0x04df) AM_DEVREADWRITE8("fm",ym3438_r,ym3438_w,0x00ff00ff)
  AM_RANGE(0x04e0,0x04e3) AM_NOP  // R/W  -- volume ports
  AM_RANGE(0x04e8,0x04ef) AM_NOP  // R/O  -- (0x4e9) FM IRQ flag (bit 0), PCM IRQ flag (bit 3)
                                  // (0x4ea) PCM IRQ mask
                                  // R/W  -- (0x4eb) PCM IRQ flag
                                  // W/O  -- (0x4ec) LED control
  AM_RANGE(0x04f0,0x04fb) AM_DEVWRITE8("pcm",rf5c68_w,0xffffffff)
  // CRTC / Video
  AM_RANGE(0x05c8,0x05cb) AM_READWRITE8(towns_video_5c8_r, towns_video_5c8_w, 0xffffffff)
  // System ports
  AM_RANGE(0x05e8,0x05ef) AM_READWRITE(towns_sys5e8_r, towns_sys5e8_w)
  // Keyboard (8042 MCU)
  AM_RANGE(0x0600,0x0607) AM_READWRITE8(towns_keyboard_r, towns_keyboard_w,0x00ff00ff)
  // CMOS
  AM_RANGE(0x3000,0x3fff) AM_READWRITE8(towns_cmos8_r, towns_cmos8_w,0x00ff00ff)
  // CRTC / Video (again)
  AM_RANGE(0xfd90,0xfda3) AM_READWRITE8(towns_video_fd90_r, towns_video_fd90_w, 0xffffffff)
  AM_RANGE(0xff80,0xff83) AM_READWRITE8(towns_video_ff81_r, towns_video_ff81_w, 0x0000ff00)

ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( towns )
INPUT_PORTS_END

static DRIVER_INIT( towns )
{
	towns_vram = auto_alloc_array(machine,UINT32,0x20000);
	towns_cmos = auto_alloc_array(machine,UINT32,0x2000/(sizeof(UINT32)));
	towns_gfxvram = auto_alloc_array(machine,UINT32,0x8000/(sizeof(UINT32)));
	towns_txtvram = auto_alloc_array(machine,UINT32,0x8000/(sizeof(UINT32)));
}

static MACHINE_RESET( towns )
{
	ftimer = 0x00;
	nmi_mask = 0x00;
	compat_mode = 0x00;
}

static VIDEO_START( towns )
{
}

static VIDEO_UPDATE( towns )
{
	int x,y;
	int pixel = 0;

	for(y=0;y<480;y++)
	{
		for(x=0;x<640;x++)
		{
			*BITMAP_ADDR32(bitmap,y,x) = towns_vram[pixel++];
			if(pixel >= 0x20000)
				pixel = 0;
		}
	}

    return 0;
}

static const struct pit8253_config towns_pit8253_config =
{
	{
		{
			307200,
			NULL
		},
		{
			307200,
			NULL
		},
		{
			307200,
			NULL
		}
	}
};

static const struct pic8259_interface towns_pic8259_master_config =
{
	NULL
};


static const struct pic8259_interface towns_pic8259_slave_config =
{
	NULL
};

static MACHINE_DRIVER_START( towns )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",I386, 16000000)
    MDRV_CPU_PROGRAM_MAP(towns_mem)
    MDRV_CPU_IO_MAP(towns_io)

    MDRV_MACHINE_RESET(towns)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(60)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_RGB32)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)

    /* sound hardware */
    MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("fm", YM3438, 53693100 / 7) // actual clock speed unknown
//  MDRV_SOUND_CONFIG(ym3438_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
	MDRV_SOUND_ADD("pcm", RF5C68, 2150000)  // actual clock speed unknown
//  MDRV_SOUND_CONFIG(rf5c68_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

    MDRV_PIT8253_ADD("pit",towns_pit8253_config)

	MDRV_PIC8259_ADD( "pic8259_master", towns_pic8259_master_config )

	MDRV_PIC8259_ADD( "pic8259_slave", towns_pic8259_slave_config )

    MDRV_VIDEO_START(towns)
    MDRV_VIDEO_UPDATE(towns)
MACHINE_DRIVER_END

/* ROM definitions */
ROM_START( fmtowns )
  ROM_REGION32_LE( 0x280000, "user", 0)
	ROM_LOAD("fmt_dos.rom",  0x000000, 0x080000, CRC(112872ee) SHA1(57fd146478226f7f215caf63154c763a6d52165e) )
	ROM_LOAD("fmt_f20.rom",  0x080000, 0x080000, CRC(9f55a20c) SHA1(1920711cb66340bb741a760de187de2f76040b8c) )
	ROM_LOAD("fmt_dic.rom",  0x100000, 0x080000, CRC(82d1daa2) SHA1(7564020dba71deee27184824b84dbbbb7c72aa4e) )
	ROM_LOAD("fmt_fnt.rom",  0x180000, 0x040000, CRC(dd6fd544) SHA1(a216482ea3162f348fcf77fea78e0b2e4288091a) )
	ROM_LOAD("fmt_sys.rom",  0x200000, 0x040000, CRC(afe4ebcf) SHA1(4cd51de4fca9bd7a3d91d09ad636fa6b47a41df5) )
ROM_END

ROM_START( fmtownsa )
  ROM_REGION32_LE( 0x280000, "user", 0)
	ROM_LOAD("fmt_dos_a.rom",  0x000000, 0x080000, CRC(22270e9f) SHA1(a7e97b25ff72b14121146137db8b45d6c66af2ae) )
	ROM_LOAD("fmt_f20_a.rom",  0x080000, 0x080000, CRC(75660aac) SHA1(6a521e1d2a632c26e53b83d2cc4b0edecfc1e68c) )
	ROM_LOAD("fmt_dic_a.rom",  0x100000, 0x080000, CRC(74b1d152) SHA1(f63602a1bd67c2ad63122bfb4ffdaf483510f6a8) )
	ROM_LOAD("fmt_fnt_a.rom",  0x180000, 0x040000, CRC(0108a090) SHA1(1b5dd9d342a96b8e64070a22c3a158ca419894e1) )
	ROM_LOAD("fmt_sys_a.rom",  0x200000, 0x040000, CRC(92f3fa67) SHA1(be21404098b23465d24c4201a81c96ac01aff7ab) )
ROM_END

ROM_START( fmtmarty )
  ROM_REGION32_LE( 0x400000, "user", 0)
	ROM_LOAD("mrom.m36",  0x000000, 0x200000, CRC(9c0c060c) SHA1(5721c5f9657c570638352fa9acac57fa8d0b94bd) )
	ROM_LOAD("mrom.m37",  0x200000, 0x200000, CRC(fb66bb56) SHA1(e273b5fa618373bdf7536495cd53c8aac1cce9a5) )
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT  MACHINE     INPUT    INIT    CONFIG COMPANY      FULLNAME            FLAGS */
COMP( 1989, fmtowns,  0,    0, 		towns, 		towns, 	 towns,  	 0,  	"Fujitsu",   "FM-Towns",		 GAME_NOT_WORKING)
COMP( 1989, fmtownsa, fmtowns, 0, 	towns, 		towns, 	 towns,  	 0,  	"Fujitsu",   "FM-Towns (alternate)", GAME_NOT_WORKING)
CONS( 1993, fmtmarty, 0,    0, 		towns, 		towns, 	 towns,  	 0,  	"Fujitsu",   "FM-Towns Marty",	 GAME_NOT_WORKING)


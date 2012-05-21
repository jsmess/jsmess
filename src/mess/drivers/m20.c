/*

 Olivetti M20 skeleton driver, by incog (19/05/2009)

Needs a proper Z8001 CPU core, check also

ftp://ftp.groessler.org/pub/chris/olivetti_m20/misc/bios/rom.s

---

APB notes:

0xfc903 checks for the string TEST at 0x3f4-0x3f6, does an int 0xfe if so, unknown purpose

Error codes:
Triangle    Test CPU registers and instructions
Square      Test ROM
4 vertical lines    Test CPU call and trap instructions
Diamond     Test system RAM
EC0     8255 parallel interface IC test failed
EC1     6845 CRT controller IC test failed
EC2     1797 floppy disk controller chip failed
EC3     8253 timer IC failed
EC4     8251 keyboard interface failed
EC5     8251 keyboard test failed
EC6     8259 PIC IC test failed
EK0     Keyboard did not respond
EK1     Keyboard responds, but self test failed
ED1     Disk drive 1 test failed
ED0     Disk drive 0 test failed
EI0     Non-vectored interrupt error
EI1     Vectored interrupt error

*************************************************************************************************/


#include "emu.h"
#include "cpu/z8000/z8000.h"
#include "cpu/i86/i86.h"
#include "video/mc6845.h"
#include "machine/wd17xx.h"
#include "machine/i8251.h"
#include "machine/i8255.h"
#include "machine/pit8253.h"
#include "machine/pic8259.h"
#include "imagedev/flopdrv.h"
#include "formats/basicdsk.h"

class m20_state : public driver_device
{
public:
	m20_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag) ,
        m_maincpu(*this, "maincpu"),
        m_kbdi8251(*this, "i8251_1"),
        m_ttyi8251(*this, "i8251_2"),
        m_i8255(*this, "ppi8255"),
        m_i8259(*this, "i8259"),
		m_p_videoram(*this, "p_videoram"){ }

    required_device<device_t> m_maincpu;
    required_device<i8251_device> m_kbdi8251;
    required_device<i8251_device> m_ttyi8251;
    required_device<i8255_device> m_i8255;
    required_device<device_t> m_i8259;
	required_shared_ptr<UINT16> m_p_videoram;

    virtual void machine_reset();

    DECLARE_READ16_MEMBER(m20_i8259_r);
    DECLARE_WRITE16_MEMBER(m20_i8259_w);
    DECLARE_WRITE_LINE_MEMBER(pic_irq_line_w);
    DECLARE_WRITE_LINE_MEMBER(tty_clock_tick_w);
    DECLARE_WRITE_LINE_MEMBER(kbd_clock_tick_w);
	DECLARE_READ_LINE_MEMBER(kbd_rx);
	DECLARE_WRITE_LINE_MEMBER(kbd_tx);
};


#define MAIN_CLOCK 4000000 /* 4 MHz */
#define PIXEL_CLOCK XTAL_4_433619MHz


static VIDEO_START( m20 )
{
}

static SCREEN_UPDATE_RGB32( m20 )
{
	m20_state *state = screen.machine().driver_data<m20_state>();
	int x,y,i;
	UINT8 pen;
	UINT32 count;

	bitmap.fill(get_black_pen(screen.machine()), cliprect);

	count = (0);

	for(y=0; y<256; y++)
	{
		for(x=0; x<256; x+=16)
		{
			for (i = 0; i < 16; i++)
			{
				pen = (state->m_p_videoram[count]) >> (15 - i) & 1;

				if (screen.visible_area().contains(x + i, y))
					bitmap.pix32(y, x + i) = screen.machine().pens[pen];
			}

			count++;
		}
	}
	return 0;
}

// these two i8251 callbacks will ask for/send 1 bit at a time.
READ_LINE_MEMBER(m20_state::kbd_rx)
{
//    printf("read keyboard\n");
    return 0x00;
}

// this should send 0-0-0-0-0-0-1-1 after the BIOS writes 0x03 to the data port.
// it comes back 0-1-1-0-0-0-0-0-0-1-1 instead, with 3 leading extraneous bits.  why?
WRITE_LINE_MEMBER(m20_state::kbd_tx)
{
    printf("%d to keyboard\n", state);
}

READ16_MEMBER(m20_state::m20_i8259_r)
{
    return pic8259_r(m_i8259, offset)<<1;
}

WRITE16_MEMBER(m20_state::m20_i8259_w)
{
    pic8259_w(m_i8259, offset, (data>>1));
}

WRITE_LINE_MEMBER( m20_state::pic_irq_line_w )
{
    if (state)
    {
        // PIC raised IRQ line
    }
    else
    {
        // PIC lowered IRQ line
    }
}

WRITE_LINE_MEMBER( m20_state::tty_clock_tick_w )
{
    m_ttyi8251->transmit_clock();
    m_ttyi8251->receive_clock();
}

WRITE_LINE_MEMBER( m20_state::kbd_clock_tick_w )
{
    m_kbdi8251->transmit_clock();
    m_kbdi8251->receive_clock();
}

/* from the M20 hardware reference manual:
   M20 memory is configured according to the following scheme:
   Segment   Contents
         0   PCOS kernel
         1   Basic interpreter and PCOS utilities
         2   PCOS variables, Basic stock and tables, user memory
         3   Screen bitmap
         4   Diagnostics and Bootstrap
*/
static ADDRESS_MAP_START(m20_mem, AS_PROGRAM, 16, m20_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x00000, 0x01fff ) AM_RAM AM_SHARE("mainram")
    AM_RANGE( 0x02000, 0x0ffff ) AM_RAM
    AM_RANGE( 0x10000, 0x1ffff ) AM_RAM
    AM_RANGE( 0x20000, 0x2ffff ) AM_RAM
	AM_RANGE( 0x30000, 0x33fff ) AM_RAM AM_SHARE("p_videoram")//base vram
	AM_RANGE( 0x40000, 0x41fff ) AM_ROM AM_REGION("maincpu", 0x10000)
    AM_RANGE( 0x44000, 0x4bfff ) AM_RAM
    AM_RANGE( 0x50000, 0x5bfff ) AM_RAM
    AM_RANGE( 0x60000, 0x67fff ) AM_RAM
    AM_RANGE( 0x80000, 0x8ffff ) AM_RAM
    AM_RANGE( 0x90000, 0x9ffff ) AM_RAM
    AM_RANGE( 0xa0000, 0xaffff ) AM_RAM
    AM_RANGE( 0xb0000, 0xb3fff ) AM_RAM
    AM_RANGE( 0xc0000, 0xc3fff ) AM_RAM
//  AM_RANGE( 0x34000, 0x37fff ) AM_RAM //extra vram for bitmap mode
//  AM_RANGE( 0x20000, 0x2???? ) //work RAM?
ADDRESS_MAP_END

static ADDRESS_MAP_START(m20_io, AS_IO, 16, m20_state)
	ADDRESS_MAP_UNMAP_HIGH

	AM_RANGE(0x00, 0x01) AM_DEVWRITE8_LEGACY("wd177x", wd17xx_command_w, 0x00ff)
	AM_RANGE(0x02, 0x03) AM_DEVREADWRITE8_LEGACY("wd177x", wd17xx_track_r, wd17xx_track_w, 0x00ff)
	AM_RANGE(0x04, 0x05) AM_DEVREADWRITE8_LEGACY("wd177x", wd17xx_sector_r, wd17xx_sector_w, 0x00ff)
	AM_RANGE(0x06, 0x07) AM_DEVREADWRITE8_LEGACY("wd177x", wd17xx_data_r, wd17xx_data_w, 0x00ff)

	AM_RANGE(0x60, 0x61) AM_DEVWRITE8("crtc", mc6845_device, address_w, 0x00ff)
	AM_RANGE(0x62, 0x63) AM_DEVREADWRITE8("crtc", mc6845_device, register_r, register_w, 0x00ff)

	AM_RANGE(0x80, 0x87) AM_DEVREADWRITE8("ppi8255", i8255_device, read, write, 0x00ff)

    AM_RANGE(0xa0, 0xa1) AM_DEVREADWRITE8("i8251_1", i8251_device, data_r, data_w, 0x00ff)
    AM_RANGE(0xa2, 0xa3) AM_DEVREADWRITE8("i8251_1", i8251_device, status_r, control_w, 0x00ff)
    AM_RANGE(0xc0, 0xc1) AM_DEVREADWRITE8("i8251_2", i8251_device, data_r, data_w, 0x00ff)
    AM_RANGE(0xc2, 0xc3) AM_DEVREADWRITE8("i8251_2", i8251_device, status_r, control_w, 0x00ff)

	AM_RANGE(0x120, 0x127) AM_DEVREADWRITE8_LEGACY("pit8253", pit8253_r, pit8253_w, 0x00ff)

	AM_RANGE(0x140, 0x143) AM_READWRITE(m20_i8259_r, m20_i8259_w)
//  AM_RANGE(0x140, 0x143) AM_DEVREADWRITE8_LEGACY("i8259", pic8259_r, pic8259_w, 0xffff)

	// 0x21?? / 0x21? - fdc ... seems to control the screen colors???
ADDRESS_MAP_END

#if 0
static ADDRESS_MAP_START(m20_apb_mem, AS_PROGRAM, 16, m20_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x00000, 0x007ff ) AM_RAM
	AM_RANGE( 0xf0000, 0xf7fff ) AM_RAM //mirrored?
	AM_RANGE( 0xfc000, 0xfffff ) AM_ROM AM_REGION("apb_bios",0)
ADDRESS_MAP_END

static ADDRESS_MAP_START(m20_apb_io, AS_IO, 16, m20_state)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff) // may not be needed
	//0x4060 crtc address
	//0x4062 crtc data
ADDRESS_MAP_END
#endif

static INPUT_PORTS_START( m20 )
INPUT_PORTS_END

static DRIVER_INIT( m20 )
{
}

void m20_state::machine_reset()
{
	UINT8 *ROM = machine().root_device().memregion("maincpu")->base();
	UINT8 *RAM = (UINT8 *)machine().root_device().memshare("mainram")->ptr();

    ROM += 0x10000; // don't know why they load at an offset, but let's go with it

    memcpy(RAM, ROM, 256);  // should be more than sufficient to boot
    m_maincpu->reset();     // reset the CPU to ensure it picks up the new vector
}

static const mc6845_interface mc6845_intf =
{
	"screen",	/* screen we are acting on */
	8,			/* number of pixels per video memory address */
	NULL,		/* before pixel update callback */
	NULL,		/* row update callback */
	NULL,		/* after pixel update callback */
	DEVCB_NULL,	/* callback for display state changes */
	DEVCB_NULL,	/* callback for cursor state changes */
	DEVCB_NULL,	/* HSYNC callback */
	DEVCB_NULL,	/* VSYNC callback */
	NULL		/* update address callback */
};

static I8255A_INTERFACE( ppi_interface )
{
	DEVCB_NULL,     // port A read
	DEVCB_NULL,     // port A write
    DEVCB_NULL,     // port B read
	DEVCB_NULL,     // port B write
    DEVCB_NULL,     // port C read
    DEVCB_NULL      // port C write
};

static const i8251_interface kbd_i8251_intf =
{
	DEVCB_DRIVER_LINE_MEMBER(m20_state, kbd_rx),
	DEVCB_DRIVER_LINE_MEMBER(m20_state, kbd_tx),
	DEVCB_NULL,         // dsr
	DEVCB_NULL,         // dtr
	DEVCB_NULL,         // rts
	DEVCB_NULL,         // rx ready
	DEVCB_NULL,         // tx ready
	DEVCB_NULL,         // tx empty
	DEVCB_NULL          // syndet
};

static const i8251_interface tty_i8251_intf =
{
	DEVCB_NULL,         // rxd in
	DEVCB_NULL,         // txd out
	DEVCB_NULL,         // dsr
	DEVCB_NULL,         // dtr
	DEVCB_NULL,         // rts
	DEVCB_NULL,         // rx ready
	DEVCB_NULL,         // tx ready
	DEVCB_NULL,         // tx empty
	DEVCB_NULL          // syndet
};

const wd17xx_interface m20_wd17xx_interface =
{
	DEVCB_NULL,
	DEVCB_NULL, //DEVCB_LINE(bbc_wd177x_intrq_w),
	DEVCB_NULL, //DEVCB_LINE(bbc_wd177x_drq_w),
	{FLOPPY_0, FLOPPY_1, NULL, NULL}
};

static LEGACY_FLOPPY_OPTIONS_START(m20)
	LEGACY_FLOPPY_OPTION(m20, "img", "M20 disk image", basicdsk_identify_default, basicdsk_construct_default, NULL,
		HEADS([2])
		TRACKS([80])
		SECTORS([16])
		SECTOR_LENGTH([256])
		FIRST_SECTOR_ID([0]))
LEGACY_FLOPPY_OPTIONS_END

static const floppy_interface m20_floppy_interface =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_STANDARD_5_25_DSHD,
	LEGACY_FLOPPY_OPTIONS_NAME(m20),
	NULL,
	NULL
};

const struct pit8253_config pit8253_intf =
{
	{
		{
			1000000,
			DEVCB_NULL,
            DEVCB_DRIVER_LINE_MEMBER(m20_state, tty_clock_tick_w)
		},
		{
			1000000,
			DEVCB_NULL,
            DEVCB_DRIVER_LINE_MEMBER(m20_state, kbd_clock_tick_w)
		},
		{
			1000000,
			DEVCB_NULL,
			DEVCB_NULL
		}
	}
};

const struct pic8259_interface pic_intf =
{
	DEVCB_DRIVER_LINE_MEMBER(m20_state, pic_irq_line_w),
    DEVCB_LINE_VCC, // we're the only 8259, so we're the master
	DEVCB_NULL
};

static MACHINE_CONFIG_START( m20, m20_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", Z8001, MAIN_CLOCK)
	MCFG_CPU_PROGRAM_MAP(m20_mem)
	MCFG_CPU_IO_MAP(m20_io)

#if 0
	MCFG_CPU_ADD("apb", I8086, MAIN_CLOCK)
	MCFG_CPU_PROGRAM_MAP(m20_apb_mem)
	MCFG_CPU_IO_MAP(m20_apb_io)
	MCFG_DEVICE_DISABLE()
#endif

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(60)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_SIZE(512, 256)
	MCFG_SCREEN_VISIBLE_AREA(0, 512-1, 0, 256-1)
	MCFG_VIDEO_START(m20)
	MCFG_SCREEN_UPDATE_STATIC(m20)
	MCFG_PALETTE_LENGTH(4)

	/* Devices */
	MCFG_WD1770_ADD("wd177x", m20_wd17xx_interface )
	MCFG_MC6845_ADD("crtc", MC6845, PIXEL_CLOCK/8, mc6845_intf)	/* hand tuned to get ~50 fps */
	MCFG_I8255A_ADD("ppi8255",  ppi_interface)
	MCFG_I8251_ADD("i8251_1", kbd_i8251_intf)
	MCFG_I8251_ADD("i8251_2", tty_i8251_intf)
	MCFG_PIT8253_ADD("pit8253", pit8253_intf)
	MCFG_PIC8259_ADD("i8259", pic_intf)
	MCFG_LEGACY_FLOPPY_2_DRIVES_ADD(m20_floppy_interface)
MACHINE_CONFIG_END

ROM_START(m20)
	ROM_REGION(0x12000,"maincpu", 0)
	ROM_SYSTEM_BIOS( 0, "m20", "M20 1.0" )
	ROMX_LOAD("m20.bin", 0x10000, 0x2000, CRC(5c93d931) SHA1(d51025e087a94c55529d7ee8fd18ff4c46d93230), ROM_BIOS(1))
	ROM_SYSTEM_BIOS( 1, "m20-20d", "M20 2.0d" )
	ROMX_LOAD("m20-20d.bin", 0x10000, 0x2000, CRC(cbe265a6) SHA1(c7cb9d9900b7b5014fcf1ceb2e45a66a91c564d0), ROM_BIOS(2))
	ROM_SYSTEM_BIOS( 2, "m20-20f", "M20 2.0f" )
	ROMX_LOAD("m20-20f.bin", 0x10000, 0x2000, CRC(db7198d8) SHA1(149d8513867081d31c73c2965dabb36d5f308041), ROM_BIOS(3))

	ROM_REGION(0x4000,"apb_bios", 0) // Processor board with 8086
	ROM_LOAD( "apb-1086-2.0.bin", 0x0000, 0x4000, CRC(8c05be93) SHA1(2bb424afd874cc6562e9642780eaac2391308053))
ROM_END

ROM_START(m40)
	ROM_REGION(0x14000,"maincpu", 0)
	ROM_SYSTEM_BIOS( 0, "m40-81", "M40 15.dec.81" )
	ROMX_LOAD( "m40rom-15-dec-81", 0x0000, 0x2000, CRC(e8e7df84) SHA1(e86018043bf5a23ff63434f9beef7ce2972d8153), ROM_BIOS(1))
	ROM_SYSTEM_BIOS( 1, "m40-82", "M40 17.dec.82" )
	ROMX_LOAD( "m40rom-17-dec-82", 0x0000, 0x2000, CRC(cf55681c) SHA1(fe4ae14a6751fef5d7bde49439286f1da3689437), ROM_BIOS(2))
	ROM_SYSTEM_BIOS( 2, "m40-41", "M40 4.1" )
	ROMX_LOAD( "m40rom-4.1", 0x0000, 0x2000, CRC(cf55681c) SHA1(fe4ae14a6751fef5d7bde49439286f1da3689437), ROM_BIOS(3))
	ROM_SYSTEM_BIOS( 3, "m40-60", "M40 6.0" )
	ROMX_LOAD( "m40rom-6.0", 0x0000, 0x4000, CRC(8114ebec) SHA1(4e2c65b95718c77a87dbee0288f323bd1c8837a3), ROM_BIOS(4))

	ROM_REGION(0x4000, "apb_bios", ROMREGION_ERASEFF) // Processor board with 8086
ROM_END

/*    YEAR  NAME   PARENT  COMPAT  MACHINE INPUT   INIT COMPANY     FULLNAME        FLAGS */
COMP( 1981, m20,   0,      0,      m20,    m20,    m20,	"Olivetti", "Olivetti L1 M20", GAME_NOT_WORKING | GAME_NO_SOUND)
COMP( 1981, m40,   m20,    0,      m20,    m20,    m20, "Olivetti", "Olivetti L1 M40", GAME_NOT_WORKING | GAME_NO_SOUND)

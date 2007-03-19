/******************************************************************
 *
 *  Fairchild Channel F driver
 *
 *  Juergen Buchmueller
 *  Frank Palazzolo
 *  Sean Riddle
 *
 ******************************************************************/

#include "driver.h"
#include "image.h"
#include "video/generic.h"
#include "includes/channelf.h"
#include "devices/cartslot.h"
#include "sound/custom.h"

#ifndef VERBOSE
#define VERBOSE 0
#endif


#if VERBOSE
#define LOG(x)	logerror x
#else
#define LOG(x)	/* x */
#endif

/* The F8 has latches on its port pins
 * These mimic's their behavior
 * [0]=port0, [1]=port1, [2]=port4, [3]=port5
 *
 * Note: this should really be moved into f8.c,
 * but it's complicated by possible peripheral usage.
 *
 * If the read/write operation is going external to the F3850
 * (or F3853, etc.), then the latching applies.  However, relaying the
 * port read/writes from the F3850 to a peripheral like the F3853
 * should not be latched in this way. (See mk1 driver)
 *
 * The f8 cannot determine how its ports are mapped at runtime,
 * so it can't easily decide to latch or not.
 *
 * ...so it stays here for now.
 */

static UINT8 latch[6];	/* SKR - inc by 2 for 2102 ports */

static UINT8 port_read_with_latch(UINT8 ext, UINT8 latch_state)
{
	return (~ext | latch_state);
}

static  READ8_HANDLER( channelf_port_0_r )
{
	return port_read_with_latch(readinputport(0),latch[0]);
}

static  READ8_HANDLER( channelf_port_1_r )
{
	UINT8 ext_value;

	if ((latch[0] & 0x40) == 0)
	{
		ext_value = readinputport(1);
	}
	else
	{
		ext_value = 0xc0 | readinputport(1);
	}
	return port_read_with_latch(ext_value,latch[1]);
}

static  READ8_HANDLER( channelf_port_4_r )
{
	UINT8 ext_value;

	if ((latch[0] & 0x40) == 0)
	{
		ext_value = readinputport(2);
	}
	else
	{
		ext_value = 0xff;
	}
	return port_read_with_latch(ext_value,latch[2]);
}

static  READ8_HANDLER( channelf_port_5_r )
{
	return port_read_with_latch(0xff,latch[3]);
}

struct {	/* SKR - 2102 RAM chip on carts 10 and 18 I/O ports */
	UINT8 d; 			/* data bit:inverted logic, but reading/writing cancel out */
	UINT8 r_w; 			/* inverted logic: 0 means read, 1 means write */
	UINT8 a[10]; 		/* addr bits: inverted logic, but reading/writing cancel out */
	UINT16 addr; 		/* calculated addr from addr bits */
	UINT8 ram[1024];	/* RAM array */
} r2102;

static  READ8_HANDLER( channelf_2102A_r )	/* SKR */
{
	UINT8 pdata;

	if(r2102.r_w==0) {
		r2102.addr=(r2102.a[0]&1)+((r2102.a[1]<<1)&2)+((r2102.a[2]<<2)&4)+((r2102.a[3]<<3)&8)+((r2102.a[4]<<4)&16)+((r2102.a[5]<<5)&32)+((r2102.a[6]<<6)&64)+((r2102.a[7]<<7)&128)+((r2102.a[8]<<8)&256)+((r2102.a[9]<<9)&512);
		r2102.d=r2102.ram[r2102.addr]&1;
		pdata=latch[4]&0x7f;
		pdata|=(r2102.d<<7);
		LOG(("rhA: addr=%d, d=%d, r_w=%d, ram[%d]=%d,  a[9]=%d, a[8]=%d, a[7]=%d, a[6]=%d, a[5]=%d, a[4]=%d, a[3]=%d, a[2]=%d, a[1]=%d, a[0]=%d\n",r2102.addr,r2102.d,r2102.r_w,r2102.addr,r2102.ram[r2102.addr],r2102.a[9],r2102.a[8],r2102.a[7],r2102.a[6],r2102.a[5],r2102.a[4],r2102.a[3],r2102.a[2],r2102.a[1],r2102.a[0]));
		return port_read_with_latch(0xff,pdata);
	} else
		LOG(("rhA: r_w=%d\n",r2102.r_w));
		return port_read_with_latch(0xff,latch[4]);
}

static  READ8_HANDLER( channelf_2102B_r )  /* SKR */
{
	LOG(("rhB\n"));
	return port_read_with_latch(0xff,latch[5]);
}

static WRITE8_HANDLER( channelf_port_0_w )
{
	int offs;

	latch[0] = data;

    if (data & 0x20)
	{
		offs = channelf_row_reg*128+channelf_col_reg;
		if (videoram[offs] != channelf_val_reg)
			videoram[offs] = channelf_val_reg;
	}
}

static WRITE8_HANDLER( channelf_port_1_w )
{
	latch[1] = data;
    channelf_val_reg = ((data ^ 0xff) >> 6) & 0x03;
}

static WRITE8_HANDLER( channelf_port_4_w )
{
    latch[2] = data;
    channelf_col_reg = (data | 0x80) ^ 0xff;
}

static WRITE8_HANDLER( channelf_port_5_w )
{
    latch[3] = data;
	channelf_sound_w((data>>6)&3);
    channelf_row_reg = (data | 0xc0) ^ 0xff;
}

static WRITE8_HANDLER( channelf_2102A_w )  /* SKR */
{
	latch[4]=data;
	r2102.a[2]=(data>>2)&1;
	r2102.a[3]=(data>>1)&1;
	r2102.r_w=data&1;
	r2102.addr=(r2102.a[0]&1)+((r2102.a[1]<<1)&2)+((r2102.a[2]<<2)&4)+((r2102.a[3]<<3)&8)+((r2102.a[4]<<4)&16)+((r2102.a[5]<<5)&32)+((r2102.a[6]<<6)&64)+((r2102.a[7]<<7)&128)+((r2102.a[8]<<8)&256)+((r2102.a[9]<<9)&512);
	r2102.d=(data>>3)&1;
	if(r2102.r_w==1)
		r2102.ram[r2102.addr]=r2102.d;
	LOG(("whA: data=%d, addr=%d, d=%d, r_w=%d, ram[%d]=%d\n",data,r2102.addr,r2102.d,r2102.r_w,r2102.addr,r2102.ram[r2102.addr]));
}

static WRITE8_HANDLER( channelf_2102B_w )  /* SKR */
{
	latch[5]=data;
	r2102.a[9]=(data>>7)&1;
	r2102.a[8]=(data>>6)&1;
	r2102.a[7]=(data>>5)&1;
	r2102.a[1]=(data>>4)&1;
	r2102.a[6]=(data>>3)&1;
	r2102.a[5]=(data>>2)&1;
	r2102.a[4]=(data>>1)&1;
	r2102.a[0]=data&1;
	LOG(("whB: data=%d, a[9]=%d,a[8]=%d,a[0]=%d\n",data,r2102.a[9],r2102.a[8],r2102.a[0]));
}

static ADDRESS_MAP_START( channelf_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_ROM
	AM_RANGE(0x0800, 0x27ff) AM_ROM /* Cartridge Data */
	AM_RANGE(0x2800, 0x2fff) AM_RAM /* Schach RAM */
ADDRESS_MAP_END

static ADDRESS_MAP_START( channelf_io, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x00, 0x00) AM_READWRITE(channelf_port_0_r, channelf_port_0_w) /* Front panel switches */
	AM_RANGE(0x01, 0x01) AM_READWRITE(channelf_port_1_r, channelf_port_1_w) /* Right controller     */
	AM_RANGE(0x04, 0x04) AM_READWRITE(channelf_port_4_r, channelf_port_4_w) /* Left controller      */
	AM_RANGE(0x05, 0x05) AM_READWRITE(channelf_port_5_r, channelf_port_5_w)

	AM_RANGE(0x20, 0x20) AM_READWRITE(channelf_2102A_r, channelf_2102A_w) /* SKR 2102 control and addr for cart 18 */
	AM_RANGE(0x21, 0x21) AM_READWRITE(channelf_2102B_r, channelf_2102B_w) /* SKR 2102 addr */
	AM_RANGE(0x24, 0x24) AM_READWRITE(channelf_2102A_r, channelf_2102A_w) /* SKR 2102 control and addr for cart 10 */
	AM_RANGE(0x25, 0x25) AM_READWRITE(channelf_2102B_r, channelf_2102B_w) /* SKR 2102 addr */
ADDRESS_MAP_END



INPUT_PORTS_START( channelf )
	PORT_START /* Front panel buttons */
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_START )	/* START (1) */
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON5 )	/* HOLD  (2) */
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON6 )	/* MODE  (3) */
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON7 )	/* TIME  (4) */
	PORT_BIT ( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START /* Right controller */
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT) PORT_PLAYER(1)
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT) PORT_PLAYER(1)
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN) PORT_PLAYER(1)
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP) PORT_PLAYER(1)
	PORT_BIT ( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON4) /* C-CLOCKWISE */ PORT_PLAYER(1)
	PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON3) /* CLOCKWISE   */ PORT_PLAYER(1)
	PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON2) /* PULL UP     */ PORT_PLAYER(1)
	PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1) /* PUSH DOWN   */ PORT_PLAYER(1)

	PORT_START /* Left controller */
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT) PORT_PLAYER(2)
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT) PORT_PLAYER(2)
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN) PORT_PLAYER(2)
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP) PORT_PLAYER(2)
	PORT_BIT ( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON4) /* C-CLOCKWISE */ PORT_PLAYER(2)
	PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON3) /* CLOCKWISE   */ PORT_PLAYER(2)
	PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON2) /* PULL UP     */ PORT_PLAYER(2)
	PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1) /* PUSH DOWN   */ PORT_PLAYER(2)

INPUT_PORTS_END

static struct CustomSound_interface channelf_sound_interface =
{
	channelf_sh_custom_start
};


static MACHINE_DRIVER_START( channelf )
	/* basic machine hardware */
	MDRV_CPU_ADD(F8, 3579545/2)        /* Colorburst/2 */
	MDRV_CPU_PROGRAM_MAP(channelf_map, 0)
	MDRV_CPU_IO_MAP(channelf_io, 0)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(128, 64)
	MDRV_SCREEN_VISIBLE_AREA(4, 112 - 7, 4, 64 - 3)
	MDRV_PALETTE_LENGTH(8)
	MDRV_COLORTABLE_LENGTH(0)
	MDRV_PALETTE_INIT( channelf )

	MDRV_VIDEO_START( channelf )
	MDRV_VIDEO_UPDATE( channelf )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(CUSTOM, 0)
	MDRV_SOUND_CONFIG(channelf_sound_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
MACHINE_DRIVER_END

SYSTEM_BIOS_START( channelf )
	SYSTEM_BIOS_ADD( 0, "sl90025", "Luxor Video Entertainment System" )
	SYSTEM_BIOS_ADD( 1, "sl31253", "Channel F" )
SYSTEM_BIOS_END

ROM_START( channelf )
	ROM_REGION(0x10000,REGION_CPU1,0)
	ROMX_LOAD("sl90025.rom",  0x0000, 0x0400, CRC(015c1e38) SHA1(759e2ed31fbde4a2d8daf8b9f3e0dffebc90dae2), ROM_BIOS(1))
	ROMX_LOAD("sl31253.rom",  0x0000, 0x0400, CRC(04694ed9) SHA1(81193965a374d77b99b4743d317824b53c3e3c78), ROM_BIOS(2))
	ROM_LOAD("sl31254.rom",   0x0400, 0x0400, CRC(9c047ba3) SHA1(8f70d1b74483ba3a37e86cf16c849d601a8c3d2c))
	ROM_CART_LOAD(0, "bin\0", 0x0800, 0x2000, ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

SYSTEM_CONFIG_START( channelf )
	CONFIG_DEVICE(cartslot_device_getinfo)
SYSTEM_CONFIG_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

/*     YEAR  NAME      PARENT    BIOS      COMPAT    MACHINE   INPUT     INIT      CONFIG    COMPANY      FULLNAME */
CONSB( 1976, channelf, 0,        channelf, 0,        channelf, channelf, 0,        channelf, "Fairchild", "Channel F" , 0)



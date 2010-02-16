/***************************************************************************

    Micronics 1000

    http://www.philpem.me.uk/elec/micronic/
    http://members.lycos.co.uk/leeedavison/z80/micronic/index.html

****************************************************************************/

/*

    KBD_R:  EQU 00h     ; key matrix read port
    KBD_W:  EQU 02h     ; key matrix write port
    LCD_D:  EQU 03h     ; LCD data port
    Port_04:    EQU 04h     ; IRQ hardware mask
                        ; .... ...0 = keyboard interrupt enable
                        ; .... ..0. = RTC interrupt enable
                        ; .... .0.. = IR port interrupt enable
                        ; .... 0... = main battery interrupt enable
                        ; ...0 .... = backup battery interrupt enable
    Port_05:    EQU 05h     ; interrupt flag byte
                        ; .... ...1 = keyboard interrupt
                        ; .... ..1. = RTC interrupt
                        ; .... .1.. = IR port interrupt ??
                        ; .... 1... = main battery interrupt
                        ; ...1 .... = backup battery interrupt
    Port_07:    EQU 07h     ;
                        ; .... ...x
                        ; .... ..x.
    RTC_A:  EQU 08h     ; RTC address port
    LCD_C:  EQU 23h     ; LCD command port
    RTC_D:  EQU 28h     ; RTC data port
    Port_2A:    EQU 2Ah     ;
                        ; .... ...x
                        ; .... ..x.
                        ; ...x ....
                        ; ..x. ....
    Port_2B:    EQU 2Bh     ; .... xxxx = beep tone
                        ; .... 0000 = off
                        ; .... 0001 = 0.25mS = 4.000kHz
                        ; .... 0010 = 0.50mS = 2.000kHz
                        ; .... 0011 = 0.75mS = 1.333kHz
                        ; .... 0100 = 1.00mS = 1.000kHz
                        ; .... 0101 = 1.25mS = 0.800kHz
                        ; .... 0110 = 1.50mS = 0.667kHz
                        ; .... 0111 = 1.75mS = 0.571kHz
                        ; .... 1000 = 2.00mS = 0.500kHz
                        ; .... 1001 = 2.25mS = 0.444kHz
                        ; .... 1010 = 2.50mS = 0.400kHz
                        ; .... 1011 = 2.75mS = 0.364kHz
                        ; .... 1100 = 3.00mS = 0.333kHz
                        ; .... 1101 = 3.25mS = 0.308kHz
                        ; .... 1110 = 3.50mS = 0.286kHz
                        ; .... 1111 = 3.75mS = 0.267kHz
    Port_2C:    EQU 2Ch     ;
                        ; .... ...x V24_ADAPTER IR port clock
                        ; .... ..x. V24_ADAPTER IR port data
                        ; ...1 .... = backlight on
                        ; ..xx ..xx
    Port_2D:    EQU 2Dh     ;
                        ; .... ...x
                        ; .... ..x.
    Port_33:    EQU 33h     ;
    Port_46:    EQU 46h     ; LCD contrast port
    MEM_P:  EQU 47h     ; memory page
    Port_48:    EQU 48h     ;
                        ; .... ...x
                        ; .... ..x.
    Port_49:    EQU 49h     ;
                        ; .... ...x
                        ; .... ..x.
    Port_4A:    EQU 4Ah     ; end IR port output
                        ; .... ...x
                        ; .... ..x.
                        ; ...x ....
                        ; ..x. ....
                        ; .x.. ....
                        ; x... ....
    Port_4B:    EQU 4Bh     ; IR port status byte
                        ; .... ...1 RX buffer full
                        ; ...x ....
                        ; .x.. ....
                        ; 1... .... TX buffer empty
    Port_4C:    EQU 4Ch     ;
                        ; .... ...x
                        ; x... ....
    Port_4D:    EQU 4Dh     ; IR transmit byte
    Port_4E:    EQU 4Eh     ; IR receive byte
    Port_4F:    EQU 4Fh     ;
                        ; .... ...x
                        ; .... ..x.
                        ; .... .x..
                        ; .... x...
                        ; ...x ....

*/

/*

    TODO:

    - HD61830 text mode
    - everything else

*/

#include "emu.h"
#include "includes/micronic.h"
#include "cpu/z80/z80.h"
#include "video/hd61830.h"
#include "machine/mc146818.h"

static ADDRESS_MAP_START(micronic_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START(micronic_io, ADDRESS_SPACE_IO, 8)
	AM_RANGE(0x03, 0x03) AM_DEVREADWRITE(HD61830_TAG, hd61830_data_r, hd61830_data_w)
	AM_RANGE(0x23, 0x23) AM_DEVREADWRITE(HD61830_TAG, hd61830_status_r, hd61830_control_w)
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( micronic )
INPUT_PORTS_END

static PALETTE_INIT( micronic )
{
	palette_set_color(machine, 0, MAKE_RGB(138, 146, 148));
	palette_set_color(machine, 1, MAKE_RGB(92, 83, 88));
}

static VIDEO_START( micronic )
{
}

static VIDEO_UPDATE( micronic )
{
	micronic_state *state = (micronic_state *)screen->machine->driver_data;

	hd61830_update(state->hd61830, bitmap, cliprect);

	return 0;
}

static MACHINE_START( micronic )
{
	micronic_state *state = (micronic_state *)machine->driver_data;

	/* find devices */
	state->hd61830 = devtag_get_device(machine, HD61830_TAG);

	/* register for state saving */
//  state_save_register_global(machine, state->);
}

static MACHINE_DRIVER_START( micronic )
	MDRV_DRIVER_DATA(micronic_state)

	/* basic machine hardware */
    MDRV_CPU_ADD(Z80_TAG, Z80, 4000000)
    MDRV_CPU_PROGRAM_MAP(micronic_mem)
    MDRV_CPU_IO_MAP(micronic_io)

    MDRV_MACHINE_START(micronic)

    /* video hardware */
	MDRV_SCREEN_ADD(SCREEN_TAG, LCD)
	MDRV_SCREEN_REFRESH_RATE(80)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(160, 64)
	MDRV_SCREEN_VISIBLE_AREA(0, 160-1, 0, 64-1)

	MDRV_DEFAULT_LAYOUT(layout_lcd)

	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(micronic)

    MDRV_VIDEO_START(micronic)
    MDRV_VIDEO_UPDATE(micronic)

	MDRV_HD61830_ADD(HD61830_TAG, XTAL_4_9152MHz/2/2, SCREEN_TAG)
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( micronic )
    ROM_REGION( 0x18000, Z80_TAG, 0 )
	ROM_LOAD( "micron1.bin", 0x0000, 0x8000, CRC(5632c8b7) SHA1(d1c9cf691848e9125f9ea352e4ffa41c288f3e29))
	ROM_LOAD( "micron2.bin", 0x8000, 0x8000, CRC(dc8e7341) SHA1(927dddb3914a50bb051256d126a047a29eff7c65))
	ROM_LOAD( "monitor2.bin", 0x10000, 0x8000, CRC(c6ae2bbf) SHA1(1f2e3a3d4720a8e1bb38b37f4ab9e0e32676d030))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( 198?, micronic,  0,       0,	micronic,	micronic,	 0,  "Victor Micronic",   "Micronic 1000",		GAME_NOT_WORKING | GAME_NO_SOUND)

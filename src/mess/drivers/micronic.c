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

    06/2010 (Sandro Ronco)

    - ROM/RAM banking
    - keypad input
    - Periodic IRQ (RTC-146818)
    - NVRAM

    TODO:

    - HD61830 text mode
    - IR I/O port

    NOTE:
    The display shows "TESTING..." for about 2 min before showing the information screen

*/

#include "emu.h"
#include "includes/micronic.h"
#include "cpu/z80/z80.h"
#include "video/hd61830.h"
#include "machine/mc146818.h"
#include "machine/ram.h"
#include "sound/beep.h"
#include "rendlay.h"

static UINT8 keypad_r (running_machine &machine)
{
	micronic_state *state = machine.driver_data<micronic_state>();
	static const char *const bitnames[] = {"BIT0", "BIT1", "BIT2", "BIT3", "BIT4", "BIT5"};
	UINT8 port, bit, data = 0;

	for (bit = 0; bit < 8; bit++)
		if (state->m_kp_matrix & (1 << bit))
			for (port = 0; port < 6; port++)
				data |= input_port_read(machine, bitnames[port]) & (0x01 << bit) ? 0x01 << port : 0x00;
	return data;
}

static READ8_HANDLER( kp_r )
{
	return  keypad_r(space->machine());
}

static READ8_HANDLER( status_flag_r )
{
	micronic_state *state = space->machine().driver_data<micronic_state>();
	return  state->m_status_flag;
}

static WRITE8_HANDLER( status_flag_w )
{
	micronic_state *state = space->machine().driver_data<micronic_state>();
	state->m_status_flag = data;
}

static WRITE8_HANDLER( kp_matrix_w )
{
	micronic_state *state = space->machine().driver_data<micronic_state>();
	state->m_kp_matrix = data;
}

static WRITE8_HANDLER( beep_w )
{
	micronic_state *state = space->machine().driver_data<micronic_state>();
	UINT16 frequency = 0;

	beep_set_state(state->m_speaker, (data) ? 1 : 0);

	switch(data)
	{
		case 0x00:		frequency = 0;		break;
		case 0x01:		frequency = 4000;	break;
		case 0x02:		frequency = 2000;	break;
		case 0x03:		frequency = 1333;	break;
		case 0x04:		frequency = 1000;	break;
		case 0x05:		frequency = 800;	break;
		case 0x06:		frequency = 667;	break;
		case 0x07:		frequency = 571;	break;
		case 0x08:		frequency = 500;	break;
		case 0x09:		frequency = 444;	break;
		case 0x0a:		frequency = 400;	break;
		case 0x0b:		frequency = 364;	break;
		case 0x0c:		frequency = 333;	break;
		case 0x0d:		frequency = 308;	break;
		case 0x0e:		frequency = 286;	break;
		case 0x0f:		frequency = 267;	break;
	}

	beep_set_frequency(state->m_speaker, frequency);
}

static READ8_HANDLER( irq_flag_r )
{
	return (input_port_read(space->machine(), "BACKBATTERY")<<4) | (input_port_read(space->machine(), "MAINBATTERY")<<3) | (keypad_r(space->machine()) ? 0 : 1);
}

static WRITE8_HANDLER( bank_select_w )
{
	if (data < 2)
	{
		memory_set_bank(space->machine(), "bank1", data);
		space->machine().device(Z80_TAG)->memory().space(AS_PROGRAM)->unmap_write(0x0000, 0x7fff);
	}
	else
	{
		memory_set_bank(space->machine(), "bank1", (data < 9) ? data : 8);
		space->machine().device(Z80_TAG)->memory().space(AS_PROGRAM)->install_write_bank(0x0000, 0x7fff, "bank1");
	}
}

static WRITE8_HANDLER( lcd_contrast_w )
{
	micronic_state *state = space->machine().driver_data<micronic_state>();

	state->m_lcd_contrast = data;
}


/***************************************************************************
    RTC-146818
***************************************************************************/

static void set_146818_periodic_irq(running_machine &machine, UINT8 data)
{
	micronic_state *state = machine.driver_data<micronic_state>();

	attotime timer_per = attotime::from_seconds(0);

	switch(data & 0xf)
	{
		case 0:		timer_per = attotime::from_nsec(0);				break;
		case 1:		timer_per = attotime::from_nsec((data & 0x20) ? 30517 : 3906250);	break;
		case 2:		timer_per = attotime::from_nsec((data & 0x20) ? 61035 : 7812500);	break;
		case 3:		timer_per = attotime::from_nsec(122070);			break;
		case 4:		timer_per = attotime::from_nsec(244141);			break;
		case 5:		timer_per = attotime::from_nsec(488281);			break;
		case 6:		timer_per = attotime::from_nsec(976562);			break;
		case 7:		timer_per = attotime::from_nsec(1953125);			break;
		case 8:		timer_per = attotime::from_nsec(3906250);			break;
		case 9:		timer_per = attotime::from_nsec(7812500);			break;
		case 10:	timer_per = attotime::from_usec(15625);			break;
		case 11:	timer_per = attotime::from_usec(31250);			break;
		case 12:	timer_per = attotime::from_usec(62500);			break;
		case 13:	timer_per = attotime::from_usec(62500);			break;
		case 14:	timer_per = attotime::from_msec(250);				break;
		case 15:	timer_per = attotime::from_msec(500);				break;
	}

	state->m_rtc_periodic_irq->adjust(timer_per, 0, timer_per);
}

static WRITE8_HANDLER( rtc_address_w )
{
	micronic_state *state = space->machine().driver_data<micronic_state>();
	state->m_rtc_address = data;
	mc146818_device *rtc = space->machine().device<mc146818_device>("rtc");
	rtc->write(*space, 0, data);
}

static READ8_HANDLER( rtc_data_r )
{
	micronic_state *state = space->machine().driver_data<micronic_state>();
	UINT8 data = 0;
	mc146818_device *rtc = space->machine().device<mc146818_device>("rtc");
	switch(state->m_rtc_address & 0x3f)
	{
		case 0x0c:
			data = state->m_irq_flags | rtc->read(*space, 1);
			state->m_irq_flags = 0;
			break;
		default:
			data = rtc->read(*space, 1);
	}

	return data;
}

static WRITE8_HANDLER( rtc_data_w )
{
	micronic_state *state = space->machine().driver_data<micronic_state>();
	mc146818_device *rtc = space->machine().device<mc146818_device>("rtc");
	rtc->write(*space, 1, data);

	switch(state->m_rtc_address & 0x3f)
	{
		case 0x0a:
			set_146818_periodic_irq(space->machine(), data);

			break;
		case 0x0b:
			state->m_periodic_irq = (data & 0x40) ? 1 : 0;
	}
}

static TIMER_CALLBACK( rtc_periodic_irq )
{
	micronic_state *state = machine.driver_data<micronic_state>();

	if (state->m_periodic_irq)
		cputag_set_input_line(machine, Z80_TAG, 0, HOLD_LINE);

	state->m_irq_flags =  (state->m_periodic_irq<<7) | 0x40;
}

/***************************************************************************
    Machine
***************************************************************************/

static ADDRESS_MAP_START(micronic_mem, AS_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x7fff) AM_RAMBANK("bank1")
	AM_RANGE(0x8000, 0xffff) AM_RAM AM_BASE_MEMBER(micronic_state, m_ram)
ADDRESS_MAP_END

static ADDRESS_MAP_START(micronic_io, AS_IO, 8)
	ADDRESS_MAP_GLOBAL_MASK (0xff)

	/* keypad */
	AM_RANGE(0x00, 0x00) AM_READ(kp_r)
	AM_RANGE(0x02, 0x02) AM_WRITE(kp_matrix_w)

	/* hd61830 */
	AM_RANGE(0x03, 0x03) AM_DEVREADWRITE_MODERN(HD61830_TAG, hd61830_device, data_r, data_w)
	AM_RANGE(0x23, 0x23) AM_DEVREADWRITE_MODERN(HD61830_TAG, hd61830_device, status_r, control_w)

	/* rtc-146818 */
	AM_RANGE(0x08, 0x08) AM_WRITE(rtc_address_w)
	AM_RANGE(0x28, 0x28) AM_READWRITE(rtc_data_r, rtc_data_w)

	/* sound */
	AM_RANGE(0x2b, 0x2b) AM_WRITE(beep_w)

	/* basic machine */
	AM_RANGE(0x05, 0x05) AM_READ(irq_flag_r)
	AM_RANGE(0x47, 0x47) AM_WRITE(bank_select_w)
	AM_RANGE(0x46, 0x46) AM_WRITE(lcd_contrast_w)
	AM_RANGE(0x48, 0x48) AM_WRITE(status_flag_w)
	AM_RANGE(0x49, 0x49) AM_READ(status_flag_r)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( micronic )

	PORT_START("MAINBATTERY")
		PORT_CONFNAME( 0x01, 0x01, "Main Battery Status" )
		PORT_CONFSETTING( 0x01, DEF_STR( Normal ) )
		PORT_CONFSETTING( 0x00, "Low Battery" )

	PORT_START("BACKBATTERY")
		PORT_CONFNAME( 0x01, 0x01, "Backup Battery Status" )
		PORT_CONFSETTING( 0x01, DEF_STR( Normal ) )
		PORT_CONFSETTING( 0x00, "Low Battery" )

	PORT_START("BIT0")
		PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("MODE") PORT_CODE(KEYCODE_EQUALS)
		PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("A (") PORT_CODE(KEYCODE_A)
		PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("B )") PORT_CODE(KEYCODE_B)
		PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("2nd") PORT_CODE(KEYCODE_LSHIFT)
		PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("U 1") PORT_CODE(KEYCODE_1)
		PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("BACKSPACE") PORT_CODE(KEYCODE_BACKSPACE)

	PORT_START("BIT1")
		PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("C") PORT_CODE(KEYCODE_C)
		PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("D DEL") PORT_CODE(KEYCODE_D)
		PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("E #") PORT_CODE(KEYCODE_E)
		PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("F &") PORT_CODE(KEYCODE_F)
		PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("V 2") PORT_CODE(KEYCODE_2)
		PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("SPACE 0") PORT_CODE(KEYCODE_0)

	PORT_START("BIT2")
		PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("G +") PORT_CODE(KEYCODE_G)
		PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("H /") PORT_CODE(KEYCODE_H)
		PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("I ,") PORT_CODE(KEYCODE_I)
		PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("J ?") PORT_CODE(KEYCODE_J)
		PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("W 3") PORT_CODE(KEYCODE_3)
		PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("NO") PORT_CODE(KEYCODE_PGUP)

	PORT_START("BIT3")
		PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("K -") PORT_CODE(KEYCODE_K)
		PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("L *") PORT_CODE(KEYCODE_L)
		PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("M .") PORT_CODE(KEYCODE_M)
		PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("N Z") PORT_CODE(KEYCODE_Z)
		PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("ENTER") PORT_CODE(KEYCODE_ENTER)
		PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("YES") PORT_CODE(KEYCODE_PGDN)

	PORT_START("BIT4")
		PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("O 7") PORT_CODE(KEYCODE_7)
		PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("P 8") PORT_CODE(KEYCODE_8)
		PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("Q 9") PORT_CODE(KEYCODE_9)
		PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("DEPT") PORT_CODE(KEYCODE_R)

	PORT_START("BIT5")
		PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("R 4") PORT_CODE(KEYCODE_4)
		PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("S 5") PORT_CODE(KEYCODE_5)
		PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("T 6") PORT_CODE(KEYCODE_6)
		PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("END") PORT_CODE(KEYCODE_END)
INPUT_PORTS_END

static NVRAM_HANDLER( micronic )
{
	micronic_state *state = machine.driver_data<micronic_state>();
	UINT8 reg_a = 0, reg_b = 0;

	if (read_or_write)
	{
		file->write(state->m_ram, 0x8000);
		file->write(ram_get_ptr(machine.device(RAM_TAG)), 224*1024);
	}
	else
	{
		if (file)
		{
			file->read(state->m_ram, 0x8000);
			file->read(ram_get_ptr(machine.device(RAM_TAG)), 224*1024);

			/* reload register A and B for restore the periodic irq state */
			file->seek(0x4000a, SEEK_SET);
			file->read(&reg_a, 0x01);
			file->read(&reg_b, 0x01);

			set_146818_periodic_irq(machine, reg_a);
			state->m_periodic_irq = (reg_b & 0x40) ? 1 : 0;
			state->m_status_flag = 0x01;
		}
		else
			state->m_status_flag = 0;
	}
}

static PALETTE_INIT( micronic )
{
	palette_set_color(machine, 0, MAKE_RGB(138, 146, 148));
	palette_set_color(machine, 1, MAKE_RGB(92, 83, 88));
}

static VIDEO_START( micronic )
{
}

static SCREEN_UPDATE( micronic )
{
	micronic_state *state = screen->machine().driver_data<micronic_state>();

	state->m_hd61830->update_screen(bitmap, cliprect);

	return 0;
}

static HD61830_INTERFACE( lcdc_intf )
{
	SCREEN_TAG,
	DEVCB_NULL
};

static MACHINE_START( micronic )
{
	micronic_state *state = machine.driver_data<micronic_state>();

	/* find devices */
	state->m_hd61830 = machine.device<hd61830_device>(HD61830_TAG);
	state->m_speaker = machine.device("beep");

	/* ROM banks */
	memory_configure_bank(machine, "bank1", 0x00, 0x02, machine.region(Z80_TAG)->base(), 0x10000);

	/* RAM banks */
	memory_configure_bank(machine, "bank1", 0x02, 0x07, ram_get_ptr(machine.device(RAM_TAG)), 0x8000);

	state->m_rtc_periodic_irq = machine.scheduler().timer_alloc(FUNC(rtc_periodic_irq));
	/* register for state saving */
//  state_save_register_global(machine, state->);
}

static MACHINE_RESET( micronic )
{
	memory_set_bank(machine, "bank1", 0);
	machine.device(Z80_TAG)->memory().space(AS_PROGRAM)->unmap_write(0x0000, 0x7fff);
}

static MACHINE_CONFIG_START( micronic, micronic_state )
	/* basic machine hardware */
    MCFG_CPU_ADD(Z80_TAG, Z80, XTAL_3_579545MHz)
    MCFG_CPU_PROGRAM_MAP(micronic_mem)
    MCFG_CPU_IO_MAP(micronic_io)

    MCFG_MACHINE_START(micronic)
	MCFG_MACHINE_RESET(micronic)

    /* video hardware */
	MCFG_SCREEN_ADD(SCREEN_TAG, LCD)
	MCFG_SCREEN_REFRESH_RATE(80)
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_SIZE(120, 64)	//6x20, 8x8
	MCFG_SCREEN_VISIBLE_AREA(0, 120-1, 0, 64-1)
    MCFG_SCREEN_UPDATE(micronic)

	MCFG_DEFAULT_LAYOUT(layout_lcd)

	MCFG_PALETTE_LENGTH(2)
	MCFG_PALETTE_INIT(micronic)

    MCFG_VIDEO_START(micronic)

	MCFG_HD61830_ADD(HD61830_TAG, XTAL_4_9152MHz/2/2, lcdc_intf)

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO( "mono" )
	MCFG_SOUND_ADD( "beep", BEEP, 0 )
	MCFG_SOUND_ROUTE( ALL_OUTPUTS, "mono", 1.00 )

	/* ram banks */
	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("224K")

	MCFG_NVRAM_HANDLER(micronic)

	MCFG_MC146818_ADD( "rtc", MC146818_IGNORE_CENTURY )
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( micronic )
    ROM_REGION( 0x18000, Z80_TAG, 0 )
	ROM_SYSTEM_BIOS(0, "v228", "Micronic 1000")
	ROMX_LOAD( "micron1.bin", 0x0000, 0x8000, CRC(5632c8b7) SHA1(d1c9cf691848e9125f9ea352e4ffa41c288f3e29), ROM_BIOS(1))
	ROMX_LOAD( "micron2.bin", 0x10000, 0x8000, CRC(dc8e7341) SHA1(927dddb3914a50bb051256d126a047a29eff7c65), ROM_BIOS(1))
	ROM_SYSTEM_BIOS(1, "test", "Micronic 1000 LCD monitor")
	ROMX_LOAD( "monitor2.bin", 0x0000, 0x8000, CRC(c6ae2bbf) SHA1(1f2e3a3d4720a8e1bb38b37f4ab9e0e32676d030), ROM_BIOS(2))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( 198?, micronic,  0,       0,	micronic,	micronic,	 0,  "Victor Micronic",   "Micronic 1000",		GAME_NOT_WORKING | GAME_NO_SOUND)

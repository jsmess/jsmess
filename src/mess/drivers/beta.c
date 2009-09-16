/***************************************************************************
   
    Beta Computer

	http://retro.hansotten.nl/index.php?page=beta-computer

    17/07/2009 Skeleton driver.

****************************************************************************/

#include "driver.h"
#include "includes/beta.h"
#include "cpu/m6502/m6502.h"
#include "machine/6532riot.h"
#include "sound/speaker.h"
#include "beta.lh"

/* Memory Maps */

static ADDRESS_MAP_START(beta_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x007f) AM_MIRROR(0x7f00) AM_RAM // 6532 RAM
	AM_RANGE(0x0080, 0x00ff) AM_MIRROR(0x7f00) AM_DEVREADWRITE(M6532_TAG, riot6532_r, riot6532_w)
	AM_RANGE(0x8000, 0x87ff) AM_MIRROR(0x7800) AM_ROM
ADDRESS_MAP_END

/* Input Ports */

static INPUT_PORTS_START( beta )
	PORT_START("Q6")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_A) PORT_CHAR('A')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1) PORT_CHAR('1')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_4) PORT_CHAR('4')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7) PORT_CHAR('7')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("GO") PORT_CODE(KEYCODE_G)
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED ) 
	
	PORT_START("Q7")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_0) PORT_CHAR('0')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2) PORT_CHAR('2')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_5) PORT_CHAR('5')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_8) PORT_CHAR('8')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_M) PORT_CHAR('M')
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED ) 
	
	PORT_START("Q8")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_B) PORT_CHAR('B')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3) PORT_CHAR('3')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6) PORT_CHAR('6')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_9) PORT_CHAR('9')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("DA") PORT_CODE(KEYCODE_F2)
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED ) 
	
	PORT_START("Q9")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_C) PORT_CHAR('C')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_D) PORT_CHAR('D')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_E) PORT_CHAR('E')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F) PORT_CHAR('F')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("AD") PORT_CODE(KEYCODE_F1)
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED ) 
INPUT_PORTS_END

/* M6532 Interface */

static void update_display(beta_state *state)
{
	if (state->ls145_p < 6)
	{
		output_set_digit_value(state->ls145_p, state->segment);
		logerror("update display %u with %02x\n", state->ls145_p, state->segment);
	}
}

static UINT8 beta_riot_a_r(const device_config *device, UINT8 olddata)
{
	/*

		bit		description

		PA0		2716 D0, segment D, key bit 0
		PA1		2716 D1, segment E, key bit 1
		PA2		2716 D2, segment C, key bit 2
		PA3		2716 D3, segment G, key bit 3
		PA4		2716 D4, segment F, key bit 4
		PA5		2716 D5, segment B
		PA6		2716 D6, segment A
		PA7		2716 D7

	*/

	beta_state *state = device->machine->driver_data;

	UINT8 data = 0xff; // TODO: read 2716 data

	switch (state->ls145_p)
	{
	case 6: data &= input_port_read(device->machine, "Q6"); break;
	case 7: data &= input_port_read(device->machine, "Q7"); break;
	case 8: data &= input_port_read(device->machine, "Q8"); break;
	case 9: data &= input_port_read(device->machine, "Q9"); break;
	}

	return data;
}

static void beta_riot_a_w(const device_config *device, UINT8 data, UINT8 olddata)
{
	/*

		bit		description

		PA0		2716 D0, segment D, key bit 0
		PA1		2716 D1, segment E, key bit 1
		PA2		2716 D2, segment C, key bit 2
		PA3		2716 D3, segment G, key bit 3
		PA4		2716 D4, segment F, key bit 4
		PA5		2716 D5, segment B
		PA6		2716 D6, segment A
		PA7		2716 D7

	*/

	beta_state *state = device->machine->driver_data;

	logerror("PA %02x\n", data);

	state->segment = BITSWAP8(data, 7, 3, 4, 1, 0, 2, 5, 6) & 0x7f;
	update_display(state);
}

static UINT8 beta_riot_b_r(const device_config *device, UINT8 olddata)
{
	return 0;
}

static void beta_riot_b_w(const device_config *device, UINT8 data, UINT8 olddata)
{
	/*

		bit		description

		PB0		74LS145 P0
		PB1		74LS145 P1
		PB2		74LS145 P2
		PB3		74LS145 P3, 74LS164 D
		PB4		loudspeaker, data led
		PB5		address led, 74LS164 CP
		PB6		2716 _OE
		PB7		2716 _CE

	*/

	beta_state *state = device->machine->driver_data;

	logerror("PB %02x\n", data);

	/* display */
	state->ls145_p = data & 0x0f;
	update_display(state);

	/* speaker */
	speaker_level_w(state->speaker, !BIT(data, 4));

	/* address led */
	output_set_led_value(0, BIT(data, 5));

	/* data led */
	output_set_led_value(1, !BIT(data, 5));

	/* EPROM address */
	if (!state->ls164_cp && BIT(data, 5))
	{
		state->eprom_addr <<= 1;
		state->eprom_addr |= BIT(data, 3);
		state->ls164_cp = BIT(data, 5);
		logerror("EPROM address %04x\n", state->eprom_addr & 0x7ff);
	}
}

static void beta_riot_irq(const device_config *device, int state)
{
	//cputag_set_input_line(device->machine, M6502_TAG, M6502_IRQ_LINE, state ? HOLD_LINE : CLEAR_LINE);
}

static const riot6532_interface beta_riot_interface =
{
	beta_riot_a_r,
	beta_riot_b_r,
	beta_riot_a_w,
	beta_riot_b_w,
	beta_riot_irq
};

/* Machine Initialization */

static MACHINE_START( beta ) 
{
	beta_state *state = machine->driver_data;

	/* find devices */
	state->speaker = devtag_get_device(machine, SPEAKER_TAG);
}

/* Machine Driver */

static MACHINE_DRIVER_START( beta )
	MDRV_DRIVER_DATA(beta_state)

	/* basic machine hardware */
    MDRV_CPU_ADD(M6502_TAG, M6502, XTAL_1MHz)
    MDRV_CPU_PROGRAM_MAP(beta_mem)

    MDRV_MACHINE_START(beta)
	
    /* video hardware */
	MDRV_DEFAULT_LAYOUT( layout_beta )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(SPEAKER_TAG, SPEAKER, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	/* devices */
	MDRV_RIOT6532_ADD(M6532_TAG, XTAL_1MHz, beta_riot_interface)    
MACHINE_DRIVER_END

/* ROMs */

ROM_START( beta )
    ROM_REGION( 0x10000, M6502_TAG, 0 )
  	ROM_LOAD( "beta.rom", 0x8000, 0x0800, CRC(d42fdb17) SHA1(595225a0cd43dd76c46b2aff6c0f27d5991cc4f0))

	ROM_REGION( 0x800, "2716", ROMREGION_ERASEFF )
ROM_END

/* System Configuration */

static SYSTEM_CONFIG_START( beta )
	CONFIG_RAM_DEFAULT( 1 * 1024 )
SYSTEM_CONFIG_END

/* System Drivers */

/*    YEAR	NAME	PARENT	COMPAT	MACHINE	INPUT	INIT	CONFIG	COMPANY					FULLNAME				FLAGS */
COMP( 1984, beta,  0,       0, 	beta, 	beta, 	 0,  	  beta,  	 "Pitronics",   "Beta Computer",		GAME_NOT_WORKING)

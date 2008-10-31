#include "driver.h"
#include "conkort.h"

typedef struct _conkort_t conkort_t;
struct _conkort_t
{
	int cpunum;						/* CPU index of the  */
};

INLINE conkort_t *get_safe_token(const device_config *device)
{
	assert(device != NULL);
	assert(device->token != NULL);

	return (conkort_t *)device->token;
}

READ8_HANDLER( conkort_abcbus_data_r )
{
	return 0;
}

WRITE8_HANDLER( conkort_abcbus_data_w )
{
}

READ8_HANDLER( conkort_abcbus_status_r )
{
	return 0;
}

WRITE8_HANDLER( conkort_abcbus_channel_w )
{
}

WRITE8_HANDLER( conkort_abcbus_command_w )
{
}

READ8_HANDLER( conkort_abcbus_reset_r )
{
	return 0;
}

/* Memory Maps */

static ADDRESS_MAP_START( conkort_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_ROM
	AM_RANGE(0x2000, 0x3fff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( conkort_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0xff)
ADDRESS_MAP_END

/* Input Ports */

INPUT_PORTS_START( conkort )
	PORT_START("SW1")
	PORT_BIT( 0x0f, IP_ACTIVE_HIGH, IPT_UNUSED ) PORT_CONDITION("SW3", 0x7f, PORTCOND_EQUALS, 0x2e)
	PORT_DIPNAME( 0x05, 0x00, "Drive 0" ) PORT_DIPLOCATION("SW1:1,3") PORT_CONDITION("SW3", 0x7f, PORTCOND_EQUALS, 0x2d)
	PORT_DIPSETTING(    0x00, "SS SD" ) PORT_CONDITION("SW3", 0x7f, PORTCOND_EQUALS, 0x2d)
	PORT_DIPSETTING(    0x04, "SS DD" ) PORT_CONDITION("SW3", 0x7f, PORTCOND_EQUALS, 0x2d)
	PORT_DIPSETTING(    0x01, "DS SD" ) PORT_CONDITION("SW3", 0x7f, PORTCOND_EQUALS, 0x2d)
	PORT_DIPSETTING(    0x05, "DS DD" ) PORT_CONDITION("SW3", 0x7f, PORTCOND_EQUALS, 0x2d)
	PORT_DIPNAME( 0x0a, 0x00, "Drive 1" ) PORT_DIPLOCATION("SW1:2,4") PORT_CONDITION("SW3", 0x7f, PORTCOND_EQUALS, 0x2d)
	PORT_DIPSETTING(    0x00, "SS SD" ) PORT_CONDITION("SW3", 0x7f, PORTCOND_EQUALS, 0x2d)
	PORT_DIPSETTING(    0x08, "SS DD" ) PORT_CONDITION("SW3", 0x7f, PORTCOND_EQUALS, 0x2d)
	PORT_DIPSETTING(    0x02, "DS SD" ) PORT_CONDITION("SW3", 0x7f, PORTCOND_EQUALS, 0x2d)
	PORT_DIPSETTING(    0x0a, "DS DD" ) PORT_CONDITION("SW3", 0x7f, PORTCOND_EQUALS, 0x2d)
	PORT_DIPNAME( 0x05, 0x01, "Drive 0" ) PORT_DIPLOCATION("SW1:1,3") PORT_CONDITION("SW3", 0x7f, PORTCOND_EQUALS, 0x2c)
	PORT_DIPSETTING(    0x00, "SS DD 80" ) PORT_CONDITION("SW3", 0x7f, PORTCOND_EQUALS, 0x2c)
	PORT_DIPSETTING(    0x04, "SS DD 40" ) PORT_CONDITION("SW3", 0x7f, PORTCOND_EQUALS, 0x2c)
	PORT_DIPSETTING(    0x01, "DS DD 80" ) PORT_CONDITION("SW3", 0x7f, PORTCOND_EQUALS, 0x2c)
	PORT_DIPSETTING(    0x05, "DS DD 40" ) PORT_CONDITION("SW3", 0x7f, PORTCOND_EQUALS, 0x2c)
	PORT_DIPNAME( 0x0a, 0x02, "Drive 1" ) PORT_DIPLOCATION("SW1:2,4") PORT_CONDITION("SW3", 0x7f, PORTCOND_EQUALS, 0x2c)
	PORT_DIPSETTING(    0x00, "SS DD 80" ) PORT_CONDITION("SW3", 0x7f, PORTCOND_EQUALS, 0x2c)
	PORT_DIPSETTING(    0x08, "SS DD 40" ) PORT_CONDITION("SW3", 0x7f, PORTCOND_EQUALS, 0x2c)
	PORT_DIPSETTING(    0x02, "DS DD 80" ) PORT_CONDITION("SW3", 0x7f, PORTCOND_EQUALS, 0x2c)
	PORT_DIPSETTING(    0x0a, "DS DD 40" ) PORT_CONDITION("SW3", 0x7f, PORTCOND_EQUALS, 0x2c)

	PORT_START("SW2")
	PORT_DIPNAME( 0x0f, 0x08, "Disk Drive" ) PORT_DIPLOCATION("SW2:1,2,3,4")
	PORT_DIPSETTING(    0x08, "BASF 6106/08 (ABC 830, 190 9206-16)" ) PORT_CONDITION("SW3", 0x7f, PORTCOND_EQUALS, 0x2d)
	PORT_DIPSETTING(    0x09, "MPI 51 (ABC 830, 190 9206-16)" ) PORT_CONDITION("SW3", 0x7f, PORTCOND_EQUALS, 0x2d)
	PORT_DIPSETTING(    0x04, "BASF 6118 (ABC 832, 190 9711-16)" ) PORT_CONDITION("SW3", 0x7f, PORTCOND_EQUALS, 0x2c)
	PORT_DIPSETTING(    0x03, "Micropolis 1015F (ABC 832, 190 9711-15)" ) PORT_CONDITION("SW3", 0x7f, PORTCOND_EQUALS, 0x2c)
	PORT_DIPSETTING(    0x05, "Micropolis 1115F (ABC 832, 190 9711-17)" ) PORT_CONDITION("SW3", 0x7f, PORTCOND_EQUALS, 0x2c)
	PORT_DIPSETTING(    0x01, "TEAC FD55F (ABC 834, 230 7802-01)" ) PORT_CONDITION("SW3", 0x7f, PORTCOND_EQUALS, 0x2c)
	PORT_DIPSETTING(    0x02, "BASF 6138 (ABC 850, 230 8440-15)" ) PORT_CONDITION("SW3", 0x7f, PORTCOND_EQUALS, 0x2c)
	PORT_DIPSETTING(    0x0e, "BASF 6105" ) PORT_CONDITION("SW3", 0x7f, PORTCOND_EQUALS, 0x2e)
	PORT_DIPSETTING(    0x0f, "BASF 6106 (ABC 838, 230 8838-15)" ) PORT_CONDITION("SW3", 0x7f, PORTCOND_EQUALS, 0x2e)

	PORT_START("SW3")
	PORT_DIPNAME( 0x7f, 0x2d, "Disk Drive" ) PORT_DIPLOCATION("SW3:1,2,3,4,5,6,7")
	PORT_DIPSETTING(    0x2c, "ABC 832/834/850" )
	PORT_DIPSETTING(    0x2d, "ABC 830" )
	PORT_DIPSETTING(    0x2e, "ABC 838" )
INPUT_PORTS_END

/* Machine Driver */

MACHINE_DRIVER_START( conkort )
	MDRV_CPU_ADD("main", Z80, 4000000)
	MDRV_CPU_PROGRAM_MAP(conkort_map, 0)
	MDRV_CPU_IO_MAP(conkort_io_map, 0)
MACHINE_DRIVER_END

/* ROMs */

ROM_START( conkort )
	ROM_REGION( 0x10000, "main", 0 )
	ROM_LOAD( "64 90318-07.bin", 0x0000, 0x2000, CRC(06ae1fe8) SHA1(ad1d9d0c192539af70cb95223263915a09693ef8) ) // PROM v1.07, Art N/O 6490318-07. Luxor Styrkort Art. N/O 55 21046-41. Date 1985-07-03
	ROM_LOAD( "fast108.bin",	 0x0000, 0x2000, CRC(229764cb) SHA1(a2e2f6f49c31b827efc62f894de9a770b65d109d) ) // Luxor v1.08
	ROM_LOAD( "fast207.bin",	 0x0000, 0x2000, CRC(86622f52) SHA1(61ad271de53152c1640c0b364fce46d1b0b4c7e2) ) // DIAB v2.07
ROM_END

/* Device Interface */

static DEVICE_START( conkort )
{
	conkort_t *conkort = device->token;
	char unique_tag[30];
	astring *tempstring = astring_alloc();

	/* validate arguments */

	assert(device->machine != NULL);
	assert(device->tag != NULL);
	assert(strlen(device->tag) < 20);

	/* find our CPU */

	astring_printf(tempstring, "%s:%s", device->tag, "main");
	conkort->cpunum = mame_find_cpu_index(device->machine, astring_c(tempstring));
	astring_free(tempstring);

	/* register for state saving */

	state_save_combine_module_and_tag(unique_tag, "conkort", device->tag);

	return DEVICE_START_OK;
}

static DEVICE_SET_INFO( conkort )
{
	switch (state)
	{
		/* no parameters to set */
	}
}

DEVICE_GET_INFO( conkort )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;								break;
		case DEVINFO_INT_CLASS:							info->i = DEVICE_CLASS_PERIPHERAL;			break;
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(conkort_t);				break;

		/* --- the following bits of info are returned as pointers --- */
		case DEVINFO_PTR_ROM_REGION:					info->romregion = rom_conkort;				break;
		case DEVINFO_PTR_MACHINE_CONFIG:				info->machine_config = machine_config_conkort; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_SET_INFO:						info->set_info = DEVICE_SET_INFO_NAME(conkort); break;
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(conkort);	break;
		case DEVINFO_FCT_STOP:							/* Nothing */								break;
		case DEVINFO_FCT_RESET:							/* Nothing */								break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							info->s = "Luxor Conkort 55-21046";			break;
		case DEVINFO_STR_FAMILY:						info->s = "Luxor ABC";						break;
		case DEVINFO_STR_VERSION:						info->s = "1.0";							break;
		case DEVINFO_STR_SOURCE_FILE:					info->s = __FILE__;							break;
		case DEVINFO_STR_CREDITS:						info->s = "Copyright the MESS Team"; 		break;
	}
}

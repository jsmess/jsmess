/****************************************************************************

    drivers/palm.c
    Palm (MC68328) emulation

    Driver by MooglyGuy

    Additional bug fixing by R. Belmont

****************************************************************************/

#include "driver.h"
#include "cpu/m68000/m68000.h"
#include "includes/mc68328.h"
#include "sound/dac.h"
#include "debugger.h"


static CPU_DISASSEMBLE(palm_dasm_override);

static UINT8 port_f_latch;
static UINT16 spim_data;

/***************************************************************************
    MACHINE HARDWARE
***************************************************************************/

static INPUT_CHANGED( pen_check )
{
    UINT8 button = input_port_read(field->port->machine, "PENB");
    const device_config *mc68328_device = devtag_get_device(field->port->machine, "dragonball");
    if(button)
    {
        mc68328_set_penirq_line(mc68328_device, 1);
    }
    else
    {
        mc68328_set_penirq_line(mc68328_device, 0);
    }
}

static INPUT_CHANGED( button_check )
{
    UINT8 button_state = input_port_read(field->port->machine, "PORTD");
    const device_config *mc68328_device = devtag_get_device(field->port->machine, "dragonball");

    mc68328_set_port_d_lines(mc68328_device, button_state, (int)(FPTR)param);
}

static WRITE8_DEVICE_HANDLER( palm_port_f_out )
{
    port_f_latch = data;
}

static READ8_DEVICE_HANDLER( palm_port_c_in )
{
    return 0x10;
}

static READ8_DEVICE_HANDLER( palm_port_f_in )
{
    return port_f_latch;
}

static WRITE16_DEVICE_HANDLER( palm_spim_out )
{
    spim_data = data;
}

static READ16_DEVICE_HANDLER( palm_spim_in )
{
    return spim_data;
}

static void palm_spim_exchange( const device_config *device )
{
    UINT8 x = input_port_read(device->machine, "PENX");
    UINT8 y = input_port_read(device->machine, "PENY");

    switch( port_f_latch & 0x0f )
    {
        case 0x06:
            spim_data = (0xff - x) * 2;
            break;

        case 0x09:
            spim_data = (0xff - y) * 2;
            break;
    }
}

static MACHINE_START( palm )
{
    const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);
    memory_install_read16_handler (space, 0x000000, mess_ram_size - 1, mess_ram_size - 1, 0, (read16_space_func)1);
    memory_install_write16_handler(space, 0x000000, mess_ram_size - 1, mess_ram_size - 1, 0, (write16_space_func)1);
    memory_set_bankptr(machine, 1, mess_ram);

    state_save_register_global(machine, port_f_latch);
    state_save_register_global(machine, spim_data);
}

static MACHINE_RESET( palm )
{
    // Copy boot ROM
    UINT8* bios = memory_region(machine, "bios");
    memset(mess_ram, 0, mess_ram_size);
    memcpy(mess_ram, bios, 0x20000);

    device_reset(cputag_get_cpu(machine, "maincpu"));
}


/***************************************************************************
    ADDRESS MAPS
***************************************************************************/

static ADDRESS_MAP_START(palm_map, ADDRESS_SPACE_PROGRAM, 16)
    AM_RANGE(0xc00000, 0xe07fff) AM_ROM AM_REGION("bios", 0)
    AM_RANGE(0xfff000, 0xffffff) AM_DEVREADWRITE(MC68328_TAG, mc68328_r, mc68328_w)
ADDRESS_MAP_END


/***************************************************************************
    AUDIO HARDWARE
***************************************************************************/

static WRITE8_DEVICE_HANDLER( palm_dac_transition )
{
    dac_data_w( devtag_get_device(device->machine, "dac"), 0x7f * data );
}


/***************************************************************************
    MACHINE DRIVERS
***************************************************************************/

static DRIVER_INIT( palm )
{
    debug_cpu_set_dasm_override(cputag_get_cpu(machine, "maincpu"), CPU_DISASSEMBLE_NAME(palm_dasm_override));
}

static const mc68328_interface palm_dragonball_iface =
{
    0,

    NULL,                   // Port A Output
    NULL,                   // Port B Output
    NULL,                   // Port C Output
    NULL,                   // Port D Output
    NULL,                   // Port E Output
    palm_port_f_out,        // Port F Output
    NULL,                   // Port G Output
    NULL,                   // Port J Output
    NULL,                   // Port K Output
    NULL,                   // Port M Output

    NULL,                   // Port A Input
    NULL,                   // Port B Input
    palm_port_c_in,         // Port C Input
    NULL,                   // Port D Input
    NULL,                   // Port E Input
    palm_port_f_in,         // Port F Input
    NULL,                   // Port G Input
    NULL,                   // Port J Input
    NULL,                   // Port K Input
    NULL,                   // Port M Input

    palm_dac_transition,

    palm_spim_out,
    palm_spim_in,
    palm_spim_exchange
};


static MACHINE_DRIVER_START( palm )

    /* basic machine hardware */
    MDRV_CPU_ADD( "maincpu", M68000, 32768*506 )        /* 16.580608 MHz */
    MDRV_CPU_PROGRAM_MAP( palm_map)
    MDRV_SCREEN_ADD( "screen", RASTER )
    MDRV_SCREEN_REFRESH_RATE( 60 )
    MDRV_SCREEN_VBLANK_TIME( ATTOSECONDS_IN_USEC(1260) )
    MDRV_QUANTUM_TIME( HZ(60) )

    MDRV_MACHINE_START( palm )
    MDRV_MACHINE_RESET( palm )

    /* video hardware */
    MDRV_VIDEO_ATTRIBUTES( VIDEO_UPDATE_BEFORE_VBLANK )
    MDRV_SCREEN_FORMAT( BITMAP_FORMAT_INDEXED16 )
    MDRV_SCREEN_SIZE( 160, 220 )
    MDRV_SCREEN_VISIBLE_AREA( 0, 159, 0, 219 )
    MDRV_PALETTE_LENGTH( 2 )
    MDRV_PALETTE_INIT( mc68328 )
    MDRV_DEFAULT_LAYOUT(layout_lcd)

    MDRV_VIDEO_START( mc68328 )
    MDRV_VIDEO_UPDATE( mc68328 )

    /* audio hardware */
    MDRV_SPEAKER_STANDARD_MONO("mono")
    MDRV_SOUND_ADD("dac", DAC, 0)
    MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

    MDRV_MC68328_ADD( palm_dragonball_iface )

MACHINE_DRIVER_END

static INPUT_PORTS_START( palm )
    PORT_START( "PENX" )
    PORT_BIT( 0xff, 0x50, IPT_LIGHTGUN_X ) PORT_NAME("Pen X") PORT_MINMAX(0, 0xa0) PORT_SENSITIVITY(50) PORT_CROSSHAIR(X, 1.0, 0.0, 0)

    PORT_START( "PENY" )
    PORT_BIT( 0xff, 0x50, IPT_LIGHTGUN_Y ) PORT_NAME("Pen Y") PORT_MINMAX(0, 0xa0) PORT_SENSITIVITY(50) PORT_CROSSHAIR(Y, 1.0, 0.0, 0)

    PORT_START( "PENB" )
    PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_NAME("Pen Button") PORT_CODE(MOUSECODE_BUTTON1) PORT_CHANGED(pen_check, NULL)

    PORT_START( "PORTD" )
    PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_NAME("Power") PORT_CODE(KEYCODE_D)   PORT_CHANGED(button_check, (void*)0)
    PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_NAME("Up") PORT_CODE(KEYCODE_Y)      PORT_CHANGED(button_check, (void*)1)
    PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON4 ) PORT_NAME("Down") PORT_CODE(KEYCODE_H)    PORT_CHANGED(button_check, (void*)2)
    PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON5 ) PORT_NAME("Button 1") PORT_CODE(KEYCODE_F)   PORT_CHANGED(button_check, (void*)3)
    PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON6 ) PORT_NAME("Button 2") PORT_CODE(KEYCODE_G)   PORT_CHANGED(button_check, (void*)4)
    PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON7 ) PORT_NAME("Button 3") PORT_CODE(KEYCODE_J)   PORT_CHANGED(button_check, (void*)5)
    PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON8 ) PORT_NAME("Button 4") PORT_CODE(KEYCODE_K)   PORT_CHANGED(button_check, (void*)6)
    PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

#define PALM_68328_BIOS \
    ROM_REGION16_BE( 0x208000, "bios", 0 )  \
    ROM_SYSTEM_BIOS( 0, "1.0e", "Palm OS 1.0 (English)" )   \
    ROMX_LOAD( "palmos10-en.rom", 0x000000, 0x080000, CRC(82030062) SHA1(00d85c6a0588133cc4651555e9605a61fc1901fc), ROM_GROUPWORD | ROM_BIOS(1) ) \
    ROM_SYSTEM_BIOS( 1, "2.0eper", "Palm OS 2.0 Personal (English)" ) \
    ROMX_LOAD( "palmos20-en-pers.rom", 0x000000, 0x100000, CRC(40ea8baa) SHA1(8e26e213de42da1317c375fb1f394bb945b9d178), ROM_GROUPWORD | ROM_BIOS(2) ) \
    ROM_SYSTEM_BIOS( 2, "2.0epro", "Palm OS 2.0 Professional (English)" ) \
    ROMX_LOAD( "palmos20-en-pro.rom", 0x000000, 0x100000, CRC(baa5b36a) SHA1(535bd9548365d300f85f514f318460443a021476), ROM_GROUPWORD | ROM_BIOS(3) ) \
    ROM_SYSTEM_BIOS( 3, "2.0eprod", "Palm OS 2.0 Professional (English, Debug)" ) \
    ROMX_LOAD( "palmis20-en-pro-dbg.rom", 0x000000, 0x100000, CRC(0d1d3a3b) SHA1(f18a80baa306d4d46b490589ee9a2a5091f6081c), ROM_GROUPWORD | ROM_BIOS(4) ) \
    ROM_SYSTEM_BIOS( 4, "3.0e", "Palm OS 3.0 (English)" ) \
    ROMX_LOAD( "palmos30-en.rom", 0x008000, 0x200000, CRC(6f461f3d) SHA1(7fbf592b4dc8c222be510f6cfda21d48ebe22413), ROM_GROUPWORD | ROM_BIOS(5) ) \
    ROM_RELOAD(0x000000, 0x004000)	\
    ROM_SYSTEM_BIOS( 5, "3.0ed", "Palm OS 3.0 (English, Debug)" ) \
    ROMX_LOAD( "palmos30-en-dbg.rom", 0x008000, 0x200000, CRC(4deda226) SHA1(1c67d6fee2b6a4acd51cda6ef3490305730357ad), ROM_GROUPWORD | ROM_BIOS(6) ) \
    ROM_RELOAD(0x000000, 0x004000)	\
    ROM_SYSTEM_BIOS( 6, "3.0g", "Palm OS 3.0 (German)" ) \
    ROMX_LOAD( "palmos30-de.rom", 0x008000, 0x200000, CRC(b991d6c3) SHA1(73e7539517b0d931e9fa99d6f6914ad46fb857b4), ROM_GROUPWORD | ROM_BIOS(7) ) \
    ROM_RELOAD(0x000000, 0x004000)	\
    ROM_SYSTEM_BIOS( 7, "3.0f", "Palm OS 3.0 (French)" ) \
    ROMX_LOAD( "palmos30-fr.rom", 0x008000, 0x200000, CRC(a2a9ff6c) SHA1(7cb119f896017e76e4680510bee96207d9d28e44), ROM_GROUPWORD | ROM_BIOS(8) ) \
    ROM_RELOAD(0x000000, 0x004000)	\
    ROM_SYSTEM_BIOS( 8, "3.0s", "Palm OS 3.0 (Spanish)" ) \
    ROMX_LOAD( "palmos30-sp.rom", 0x008000, 0x200000, CRC(63a595be) SHA1(f6e03a2fedf0cbe6228613f50f8e8717e797877d), ROM_GROUPWORD | ROM_BIOS(9) ) \
    ROM_RELOAD(0x000000, 0x004000)	\
    ROM_SYSTEM_BIOS( 9, "3.3e", "Palm OS 3.3 (English)" ) \
    ROMX_LOAD( "palmos33-en-iii.rom", 0x008000, 0x200000, CRC(1eae0253) SHA1(e4626f1d33eca8368284d906b2152dcd28b71bbd), ROM_GROUPWORD | ROM_BIOS(10) ) \
    ROM_RELOAD(0x000000, 0x004000)	\
    ROM_SYSTEM_BIOS( 10, "3.3f", "Palm OS 3.3 (French)" ) \
    ROMX_LOAD( "palmos33-fr-iii.rom", 0x008000, 0x200000, CRC(d7894f5f) SHA1(c7c90df814d4f97958194e0bc28c595e967a4529), ROM_GROUPWORD | ROM_BIOS(11) ) \
    ROM_RELOAD(0x000000, 0x004000)	\
    ROM_SYSTEM_BIOS( 11, "3.3g", "Palm OS 3.3 (German)" ) \
    ROMX_LOAD( "palmos33-de-iii.rom", 0x008000, 0x200000, CRC(a5a99c45) SHA1(209b0154942dab80b56d5e6e68fa20b9eb75f5fe), ROM_GROUPWORD | ROM_BIOS(12) ) \
    ROM_RELOAD(0x000000, 0x004000)

ROM_START( pilot1k )
    PALM_68328_BIOS
    ROM_DEFAULT_BIOS( "1.0e" )
ROM_END

ROM_START( pilot5k )
    PALM_68328_BIOS
    ROM_DEFAULT_BIOS( "1.0e" )
ROM_END

ROM_START( palmpers )
    PALM_68328_BIOS
    ROM_DEFAULT_BIOS( "2.0eper" )
ROM_END

ROM_START( palmpro )
    PALM_68328_BIOS
    ROM_DEFAULT_BIOS( "2.0epro" )
ROM_END

ROM_START( palmiii )
    PALM_68328_BIOS
    ROM_DEFAULT_BIOS( "3.0e" )
ROM_END

ROM_START( palmv )
	ROM_REGION16_BE( 0x208000, "bios", 0 )
    ROM_SYSTEM_BIOS( 0, "3.1e", "Palm OS 3.1 (English)" )
    ROMX_LOAD( "palmv31-en.rom", 0x008000, 0x200000, CRC(4656b2ae) SHA1(ec66a93441fbccfd8e0c946baa5d79c478c83e85), ROM_GROUPWORD | ROM_BIOS(1) )
    ROM_RELOAD(0x000000, 0x004000)
    ROM_SYSTEM_BIOS( 1, "3.1g", "Palm OS 3.1 (German)" )
    ROMX_LOAD( "palmv31-de.rom", 0x008000, 0x200000, CRC(a9631dcf) SHA1(63b44d4d3fc2f2196c96d3b9b95da526df0fac77), ROM_GROUPWORD | ROM_BIOS(2) )
    ROM_RELOAD(0x000000, 0x004000)
    ROM_SYSTEM_BIOS( 2, "3.1f", "Palm OS 3.1 (French)" )
    ROMX_LOAD( "palmv31-fr.rom", 0x008000, 0x200000, CRC(0d933a1c) SHA1(d0454f1159705d0886f8a68e1b8a5e96d2ca48f6), ROM_GROUPWORD | ROM_BIOS(3) )
    ROM_RELOAD(0x000000, 0x004000)
    ROM_SYSTEM_BIOS( 3, "3.1s", "Palm OS 3.1 (Spanish)" )
    ROMX_LOAD( "palmv31-sp.rom", 0x008000, 0x200000, CRC(cc46ca1f) SHA1(93bc78ca84d34916d7e122b745adec1068230fcd), ROM_GROUPWORD | ROM_BIOS(4) )
    ROM_RELOAD(0x000000, 0x004000)
    ROM_SYSTEM_BIOS( 4, "3.1j", "Palm OS 3.1 (Japanese)" )
	ROMX_LOAD( "palmv31-jp.rom", 0x008000, 0x200000, CRC(c786db12) SHA1(4975ff2af76892370c5d4d7d6fa87a84480e79d6), ROM_GROUPWORD | ROM_BIOS(5) )
    ROM_RELOAD(0x000000, 0x004000)
	ROM_SYSTEM_BIOS( 5, "3.1e2", "Palm OS 3.1 (English) v2" )
    ROMX_LOAD( "palmv31-en-2.rom", 0x008000, 0x200000, CRC(caced2bd) SHA1(95970080601f72a77a4c338203ed8809fab17abf), ROM_GROUPWORD | ROM_BIOS(6) )
	ROM_RELOAD(0x000000, 0x004000)
    ROM_DEFAULT_BIOS( "3.1e2" )
ROM_END

ROM_START( palmvx )
	ROM_REGION16_BE( 0x208000, "bios", 0 )
    ROM_SYSTEM_BIOS( 0, "3.3e", "Palm OS 3.3 (English)" )
    ROMX_LOAD( "palmvx33-en.rom", 0x000000, 0x200000, CRC(3fc0cc6d) SHA1(6873d5fa99ac372f9587c769940c9b3ac1745a0a), ROM_GROUPWORD | ROM_BIOS(1) )
    ROM_SYSTEM_BIOS( 1, "4.0e", "Palm OS 4.0 (English)" )
    ROMX_LOAD( "palmvx40-en.rom", 0x000000, 0x200000, CRC(488e4638) SHA1(10a10fc8617743ebd5df19c1e99ca040ac1da4f5), ROM_GROUPWORD | ROM_BIOS(2) )
    ROM_SYSTEM_BIOS( 2, "4.1e", "Palm OS 4.1 (English)" )
    ROMX_LOAD( "palmvx41-en.rom", 0x000000, 0x200000, CRC(e59f4dff) SHA1(5e3000db318eeb8cd1f4d9729d0c9ebca560fa4a), ROM_GROUPWORD | ROM_BIOS(3) )
    ROM_DEFAULT_BIOS( "4.1e" )
ROM_END

ROM_START( palmiiic )
	ROM_REGION16_BE( 0x208000, "bios", 0 )
	ROM_SYSTEM_BIOS( 0, "3.5eb", "Palm OS 3.5 (English) beta" )
	ROMX_LOAD( "palmiiic350-en-beta.rom", 0x008000, 0x200000, CRC(d58521a4) SHA1(508742ea1e078737666abd4283cf5e6985401c9e), ROM_GROUPWORD | ROM_BIOS(1) )
	ROM_RELOAD(0x000000, 0x004000)
	ROM_SYSTEM_BIOS( 1, "3.5eb", "Palm OS 3.5 (Chinese)" )
    ROMX_LOAD( "palmiiic350-ch.rom", 0x008000, 0x200000, CRC(a9779f3a) SHA1(1541102cd5234665233072afe8f0e052134a5334), ROM_GROUPWORD | ROM_BIOS(2) )
    ROM_RELOAD(0x000000, 0x004000)
    ROM_SYSTEM_BIOS( 2, "4.0e", "Palm OS 4.0 (English)" )
    ROMX_LOAD( "palmiiic40-en.rom", 0x008000, 0x200000, CRC(6b2a5ad2) SHA1(54321dcaedcc80de57a819cfd599d8d1b2e26eeb), ROM_GROUPWORD | ROM_BIOS(3) )
    ROM_RELOAD(0x000000, 0x004000)
    ROM_DEFAULT_BIOS( "4.0e" )
ROM_END

ROM_START( palmm100 )
	ROM_REGION16_BE( 0x208000, "bios", 0 )
    ROM_SYSTEM_BIOS( 0, "3.51e", "Palm OS 3.5.1 (English)" )
    ROMX_LOAD( "palmm100-351-en.rom", 0x008000, 0x200000, CRC(ae8dda60) SHA1(c46248d6f05cb2f4337985610cedfbdc12ac47cf), ROM_GROUPWORD | ROM_BIOS(1) )
    ROM_RELOAD(0x000000, 0x004000)
    ROM_DEFAULT_BIOS( "3.51e" )
ROM_END

ROM_START( palmm130 )
	ROM_REGION16_BE( 0x408000, "bios", 0 )
    ROM_SYSTEM_BIOS( 0, "4.0e", "Palm OS 4.0 (English)" )
    ROMX_LOAD( "palmm130-40-en.rom", 0x008000, 0x400000, CRC(58046b7e) SHA1(986057010d62d5881fba4dede2aba0d4d5008b16), ROM_GROUPWORD | ROM_BIOS(1) )
    ROM_RELOAD(0x000000, 0x004000)
    ROM_DEFAULT_BIOS( "4.0e" )
ROM_END

ROM_START( palmm505 )
	ROM_REGION16_BE( 0x408000, "bios", 0 )
    ROM_SYSTEM_BIOS( 0, "4.0e", "Palm OS 4.0 (English)" )
    ROMX_LOAD( "palmos40-en-m505.rom", 0x008000, 0x400000, CRC(822a4679) SHA1(a4f5e9f7edb1926647ea07969200c5c5e1521bdf), ROM_GROUPWORD | ROM_BIOS(1) )
    ROM_RELOAD(0x000000, 0x004000)
    ROM_SYSTEM_BIOS( 1, "4.1e", "Palm OS 4.1 (English)" )
    ROMX_LOAD( "palmos41-en-m505.rom", 0x008000, 0x400000, CRC(d248202a) SHA1(65e1bd08b244c589b4cd10fe573e0376aba90e5f), ROM_GROUPWORD | ROM_BIOS(2) )
    ROM_RELOAD(0x000000, 0x004000)
    ROM_DEFAULT_BIOS( "4.1e" )
ROM_END

ROM_START( palmm515 )
	ROM_REGION16_BE( 0x408000, "bios", 0 )
    ROM_SYSTEM_BIOS( 0, "4.1e", "Palm OS 4.1 (English)" )
    ROMX_LOAD( "palmos41-en-m515.rom", 0x008000, 0x400000, CRC(6e143436) SHA1(a0767ea26cc493a3f687525d173903fef89f1acb), ROM_GROUPWORD | ROM_BIOS(1) )
    ROM_RELOAD(0x000000, 0x004000)
    ROM_DEFAULT_BIOS( "4.0e" )
ROM_END

ROM_START( visor )
	ROM_REGION16_BE( 0x208000, "bios", 0 )
    ROM_SYSTEM_BIOS( 0, "3.52e", "Palm OS 3.5.2 (English)" )
    ROMX_LOAD( "visor-352-en.rom", 0x008000, 0x200000, CRC(c9e55271) SHA1(749e9142f4480114c5e0d7f21ea354df7273ac5b), ROM_GROUPWORD | ROM_BIOS(1) )
    ROM_RELOAD(0x000000, 0x004000)
    ROM_DEFAULT_BIOS( "3.52e" )
ROM_END

ROM_START( spt1500 )
	ROM_REGION16_BE( 0x208000, "bios", 0 )
    ROM_SYSTEM_BIOS( 0, "4.1pim", "Version 4.1 (pim)" )
  	ROMX_LOAD( "spt1500v41-pim.rom",      0x008000, 0x200000, CRC(29e50eaf) SHA1(3e920887bdf74f8f83935977b02f22d5217723eb), ROM_GROUPWORD | ROM_BIOS(1) )
  	ROM_RELOAD(0x000000, 0x004000)
  	ROM_SYSTEM_BIOS( 1, "4.1pim", "Version 4.1 (pimnoft)" )
  	ROMX_LOAD( "spt1500v41-pimnoft.rom",  0x008000, 0x200000, CRC(4b44f284) SHA1(4412e946444706628b94d2303b02580817e1d370), ROM_GROUPWORD | ROM_BIOS(2) )
  	ROM_RELOAD(0x000000, 0x004000)
  	ROM_SYSTEM_BIOS( 2, "4.1pim", "Version 4.1 (nopimnoft)" )
  	ROMX_LOAD( "spt1500v41-nopimnoft.rom",0x008000, 0x200000, CRC(4ba19190) SHA1(d713c1390b82eb4e5fbb39aa10433757c5c49e02), ROM_GROUPWORD | ROM_BIOS(3) )
  	ROM_RELOAD(0x000000, 0x004000)
ROM_END

ROM_START( spt1700 )
	ROM_REGION16_BE( 0x208000, "bios", 0 )
	ROM_SYSTEM_BIOS( 0, "1.03pim", "Version 1.03 (pim)" )
	ROMX_LOAD( "spt1700v103-pim.rom",     0x008000, 0x200000, CRC(9df4ee50) SHA1(243a19796f15219cbd73e116f7dfb236b3d238cd), ROM_GROUPWORD | ROM_BIOS(1) )
	ROM_RELOAD(0x000000, 0x004000)
ROM_END

ROM_START( spt1740 )
	ROM_REGION16_BE( 0x208000, "bios", 0 )
	ROM_SYSTEM_BIOS( 0, "1.03pim", "Version 1.03 (pim)" )
	ROMX_LOAD( "spt1740v103-pim.rom",     0x008000, 0x200000, CRC(c29f341c) SHA1(b56d7f8a0c15b1105972e24ed52c846b5e27b195), ROM_GROUPWORD | ROM_BIOS(1) )
	ROM_RELOAD(0x000000, 0x004000)
	ROM_SYSTEM_BIOS( 1, "1.03pim", "Version 1.03 (pimnoft)" )
	ROMX_LOAD( "spt1740v103-pimnoft.rom", 0x008000, 0x200000, CRC(b2d49d5c) SHA1(c133dc021b6797cdb93b666c5b315b00b5bb0917), ROM_GROUPWORD | ROM_BIOS(2) )
	ROM_RELOAD(0x000000, 0x004000)
	ROM_SYSTEM_BIOS( 2, "1.03pim", "Version 1.03 (nopim)" )
	ROMX_LOAD( "spt1740v103-nopim.rom",   0x008000, 0x200000, CRC(8ea7e652) SHA1(2a4b5d6a426e627b3cb82c47109cfe2497eba29a), ROM_GROUPWORD | ROM_BIOS(3) )
	ROM_RELOAD(0x000000, 0x004000)
ROM_END

static SYSTEM_CONFIG_START( pilot1k )
    CONFIG_RAM_DEFAULT  (0x020000)      // 128k
    CONFIG_RAM      (0x080000)      // 512k
    CONFIG_RAM      (0x100000)      // 1M
    CONFIG_RAM      (0x200000)      // 2M
    CONFIG_RAM      (0x400000)      // 4M
    CONFIG_RAM      (0x800000)      // 8M
SYSTEM_CONFIG_END

static SYSTEM_CONFIG_START( pilot5k )
    CONFIG_RAM_DEFAULT      (0x080000)      // 512k
    CONFIG_RAM      (0x100000)      // 1M
    CONFIG_RAM      (0x200000)      // 2M
    CONFIG_RAM      (0x400000)      // 4M
    CONFIG_RAM      (0x800000)      // 8M
SYSTEM_CONFIG_END

static SYSTEM_CONFIG_START( palmpro )
    CONFIG_RAM_DEFAULT      (0x100000)      // 1M
    CONFIG_RAM      (0x200000)      // 2M
    CONFIG_RAM      (0x400000)      // 4M
    CONFIG_RAM      (0x800000)      // 8M
SYSTEM_CONFIG_END

static SYSTEM_CONFIG_START( palmiii )
    CONFIG_RAM_DEFAULT  (0x200000)      // 2M
    CONFIG_RAM      (0x400000)      // 4M
    CONFIG_RAM      (0x800000)      // 8M
SYSTEM_CONFIG_END

static SYSTEM_CONFIG_START( palmv )
    CONFIG_RAM_DEFAULT  (0x200000)      // 2M
    CONFIG_RAM      (0x400000)      // 4M
    CONFIG_RAM      (0x800000)      // 8M
SYSTEM_CONFIG_END

static SYSTEM_CONFIG_START( palmvx )
    CONFIG_RAM_DEFAULT  (0x800000)      // 8M
SYSTEM_CONFIG_END

/*    YEAR  NAME      PARENT    COMPAT   MACHINE   INPUT     INIT      CONFIG    COMPANY FULLNAME */
COMP( 1996, pilot1k,  0,        0,       palm,     palm,     palm,     pilot1k,  "U.S. Robotics", "Pilot 1000", GAME_SUPPORTS_SAVE | GAME_NO_SOUND )
COMP( 1996, pilot5k,  pilot1k,  0,       palm,     palm,     palm,     pilot5k,  "U.S. Robotics", "Pilot 5000", GAME_SUPPORTS_SAVE | GAME_NO_SOUND )
COMP( 1997, palmpers, pilot1k,  0,       palm,     palm,     palm,     pilot5k,  "U.S. Robotics", "Palm Pilot Personal", GAME_SUPPORTS_SAVE | GAME_NO_SOUND )
COMP( 1997, palmpro,  pilot1k,  0,       palm,     palm,     palm,     palmpro,  "U.S. Robotics", "Palm Pilot Pro", GAME_SUPPORTS_SAVE | GAME_NO_SOUND )
COMP( 1998, palmiii,  pilot1k,  0,       palm,     palm,     palm,     palmiii,  "3Com", "Palm III", GAME_SUPPORTS_SAVE | GAME_NO_SOUND )
COMP( 1998, palmiiic, pilot1k,  0,       palm,     palm,     palm,     palmiii,  "Palm Inc.", "Palm IIIc", GAME_NOT_WORKING )
COMP( 2000, palmm100, pilot1k,  0,       palm,     palm,     palm,     palmiii,  "Palm Inc.", "Palm m100", GAME_NOT_WORKING )
COMP( 2000, palmm130, pilot1k,  0,       palm,     palm,     palm,     palmiii,  "Palm Inc.", "Palm m130", GAME_NOT_WORKING )
COMP( 2001, palmm505, pilot1k,  0,       palm,     palm,     palm,     palmiii,  "Palm Inc.", "Palm m505", GAME_NOT_WORKING )
COMP( 2001, palmm515, pilot1k,  0,       palm,     palm,     palm,     palmiii,  "Palm Inc.", "Palm m515", GAME_NOT_WORKING )
COMP( 1999, palmv,    pilot1k,  0,       palm,     palm,     palm,     palmv,  	 "3Com", "Palm V", GAME_NOT_WORKING )
COMP( 1999, palmvx,   pilot1k,  0,       palm,     palm,     palm,     palmvx,   "Palm Inc.", "Palm Vx", GAME_NOT_WORKING )
COMP( 2001, visor,    pilot1k,  0,       palm,     palm,     palm,     palmvx,   "Handspring", "Visor Edge", GAME_NOT_WORKING )
COMP( 19??, spt1500,  pilot1k,  0,       palm,     palm,     palm,     palmvx,   "Symbol", "SPT 1500", GAME_NOT_WORKING )
COMP( 19??, spt1700,  pilot1k,  0,       palm,     palm,     palm,     palmvx,   "Symbol", "SPT 1700", GAME_NOT_WORKING )
COMP( 19??, spt1740,  pilot1k,  0,       palm,     palm,     palm,     palmvx,   "Symbol", "SPT 1740", GAME_NOT_WORKING )
static const char *lookup_trap(UINT16 opcode)
{
    static const struct
    {
        const char *name;
        UINT16 trap;
    } traps[] =
    {
        { "sysTrapMemInit", 0xA000 },
        { "sysTrapMemInitHeapTable", 0xA001 },
        { "sysTrapMemStoreInit", 0xA002 },
        { "sysTrapMemCardFormat", 0xA003 },
        { "sysTrapMemCardInfo", 0xA004 },
        { "sysTrapMemStoreInfo", 0xA005 },
        { "sysTrapMemStoreSetInfo", 0xA006 },
        { "sysTrapMemNumHeaps", 0xA007 },
        { "sysTrapMemNumRAMHeaps", 0xA008 },
        { "sysTrapMemHeapID", 0xA009 },
        { "sysTrapMemHeapPtr", 0xA00A },
        { "sysTrapMemHeapFreeBytes", 0xA00B },
        { "sysTrapMemHeapSize", 0xA00C },
        { "sysTrapMemHeapFlags", 0xA00D },
        { "sysTrapMemHeapCompact", 0xA00E },
        { "sysTrapMemHeapInit", 0xA00F },
        { "sysTrapMemHeapFreeByOwnerID", 0xA010 },
        { "sysTrapMemChunkNew", 0xA011 },
        { "sysTrapMemChunkFree", 0xA012 },
        { "sysTrapMemPtrNew", 0xA013 },
        { "sysTrapMemPtrRecoverHandle", 0xA014 },
        { "sysTrapMemPtrFlags", 0xA015 },
        { "sysTrapMemPtrSize", 0xA016 },
        { "sysTrapMemPtrOwner", 0xA017 },
        { "sysTrapMemPtrHeapID", 0xA018 },
        { "sysTrapMemPtrCardNo", 0xA019 },
        { "sysTrapMemPtrToLocalID", 0xA01A },
        { "sysTrapMemPtrSetOwner", 0xA01B },
        { "sysTrapMemPtrResize", 0xA01C },
        { "sysTrapMemPtrResetLock", 0xA01D },
        { "sysTrapMemHandleNew", 0xA01E },
        { "sysTrapMemHandleLockCount", 0xA01F },
        { "sysTrapMemHandleToLocalID", 0xA020 },
        { "sysTrapMemHandleLock", 0xA021 },
        { "sysTrapMemHandleUnlock", 0xA022 },
        { "sysTrapMemLocalIDToGlobal", 0xA023 },
        { "sysTrapMemLocalIDKind", 0xA024 },
        { "sysTrapMemLocalIDToPtr", 0xA025 },
        { "sysTrapMemMove", 0xA026 },
        { "sysTrapMemSet", 0xA027 },
        { "sysTrapMemStoreSearch", 0xA028 },
        { "sysTrapSysReserved10Trap1", 0xA029 },
        { "sysTrapMemKernelInit", 0xA02A },
        { "sysTrapMemHandleFree", 0xA02B },
        { "sysTrapMemHandleFlags", 0xA02C },
        { "sysTrapMemHandleSize", 0xA02D },
        { "sysTrapMemHandleOwner", 0xA02E },
        { "sysTrapMemHandleHeapID", 0xA02F },
        { "sysTrapMemHandleDataStorage", 0xA030 },
        { "sysTrapMemHandleCardNo", 0xA031 },
        { "sysTrapMemHandleSetOwner", 0xA032 },
        { "sysTrapMemHandleResize", 0xA033 },
        { "sysTrapMemHandleResetLock", 0xA034 },
        { "sysTrapMemPtrUnlock", 0xA035 },
        { "sysTrapMemLocalIDToLockedPtr", 0xA036 },
        { "sysTrapMemSetDebugMode", 0xA037 },
        { "sysTrapMemHeapScramble", 0xA038 },
        { "sysTrapMemHeapCheck", 0xA039 },
        { "sysTrapMemNumCards", 0xA03A },
        { "sysTrapMemDebugMode", 0xA03B },
        { "sysTrapMemSemaphoreReserve", 0xA03C },
        { "sysTrapMemSemaphoreRelease", 0xA03D },
        { "sysTrapMemHeapDynamic", 0xA03E },
        { "sysTrapMemNVParams", 0xA03F },
        { "sysTrapDmInit", 0xA040 },
        { "sysTrapDmCreateDatabase", 0xA041 },
        { "sysTrapDmDeleteDatabase", 0xA042 },
        { "sysTrapDmNumDatabases", 0xA043 },
        { "sysTrapDmGetDatabase", 0xA044 },
        { "sysTrapDmFindDatabase", 0xA045 },
        { "sysTrapDmDatabaseInfo", 0xA046 },
        { "sysTrapDmSetDatabaseInfo", 0xA047 },
        { "sysTrapDmDatabaseSize", 0xA048 },
        { "sysTrapDmOpenDatabase", 0xA049 },
        { "sysTrapDmCloseDatabase", 0xA04A },
        { "sysTrapDmNextOpenDatabase", 0xA04B },
        { "sysTrapDmOpenDatabaseInfo", 0xA04C },
        { "sysTrapDmResetRecordStates", 0xA04D },
        { "sysTrapDmGetLastErr", 0xA04E },
        { "sysTrapDmNumRecords", 0xA04F },
        { "sysTrapDmRecordInfo", 0xA050 },
        { "sysTrapDmSetRecordInfo", 0xA051 },
        { "sysTrapDmAttachRecord", 0xA052 },
        { "sysTrapDmDetachRecord", 0xA053 },
        { "sysTrapDmMoveRecord", 0xA054 },
        { "sysTrapDmNewRecord", 0xA055 },
        { "sysTrapDmRemoveRecord", 0xA056 },
        { "sysTrapDmDeleteRecord", 0xA057 },
        { "sysTrapDmArchiveRecord", 0xA058 },
        { "sysTrapDmNewHandle", 0xA059 },
        { "sysTrapDmRemoveSecretRecords", 0xA05A },
        { "sysTrapDmQueryRecord", 0xA05B },
        { "sysTrapDmGetRecord", 0xA05C },
        { "sysTrapDmResizeRecord", 0xA05D },
        { "sysTrapDmReleaseRecord", 0xA05E },
        { "sysTrapDmGetResource", 0xA05F },
        { "sysTrapDmGet1Resource", 0xA060 },
        { "sysTrapDmReleaseResource", 0xA061 },
        { "sysTrapDmResizeResource", 0xA062 },
        { "sysTrapDmNextOpenResDatabase", 0xA063 },
        { "sysTrapDmFindResourceType", 0xA064 },
        { "sysTrapDmFindResource", 0xA065 },
        { "sysTrapDmSearchResource", 0xA066 },
        { "sysTrapDmNumResources", 0xA067 },
        { "sysTrapDmResourceInfo", 0xA068 },
        { "sysTrapDmSetResourceInfo", 0xA069 },
        { "sysTrapDmAttachResource", 0xA06A },
        { "sysTrapDmDetachResource", 0xA06B },
        { "sysTrapDmNewResource", 0xA06C },
        { "sysTrapDmRemoveResource", 0xA06D },
        { "sysTrapDmGetResourceIndex", 0xA06E },
        { "sysTrapDmQuickSort", 0xA06F },
        { "sysTrapDmQueryNextInCategory", 0xA070 },
        { "sysTrapDmNumRecordsInCategory", 0xA071 },
        { "sysTrapDmPositionInCategory", 0xA072 },
        { "sysTrapDmSeekRecordInCategory", 0xA073 },
        { "sysTrapDmMoveCategory", 0xA074 },
        { "sysTrapDmOpenDatabaseByTypeCreator", 0xA075 },
        { "sysTrapDmWrite", 0xA076 },
        { "sysTrapDmStrCopy", 0xA077 },
        { "sysTrapDmGetNextDatabaseByTypeCreator", 0xA078 },
        { "sysTrapDmWriteCheck", 0xA079 },
        { "sysTrapDmMoveOpenDBContext", 0xA07A },
        { "sysTrapDmFindRecordByID", 0xA07B },
        { "sysTrapDmGetAppInfoID", 0xA07C },
        { "sysTrapDmFindSortPositionV10", 0xA07D },
        { "sysTrapDmSet", 0xA07E },
        { "sysTrapDmCreateDatabaseFromImage", 0xA07F },
        { "sysTrapDbgSrcMessage", 0xA080 },
        { "sysTrapDbgMessage", 0xA081 },
        { "sysTrapDbgGetMessage", 0xA082 },
        { "sysTrapDbgCommSettings", 0xA083 },
        { "sysTrapErrDisplayFileLineMsg", 0xA084 },
        { "sysTrapErrSetJump", 0xA085 },
        { "sysTrapErrLongJump", 0xA086 },
        { "sysTrapErrThrow", 0xA087 },
        { "sysTrapErrExceptionList", 0xA088 },
        { "sysTrapSysBroadcastActionCode", 0xA089 },
        { "sysTrapSysUnimplemented", 0xA08A },
        { "sysTrapSysColdBoot", 0xA08B },
        { "sysTrapSysReset", 0xA08C },
        { "sysTrapSysDoze", 0xA08D },
        { "sysTrapSysAppLaunch", 0xA08E },
        { "sysTrapSysAppStartup", 0xA08F },
        { "sysTrapSysAppExit", 0xA090 },
        { "sysTrapSysSetA5", 0xA091 },
        { "sysTrapSysSetTrapAddress", 0xA092 },
        { "sysTrapSysGetTrapAddress", 0xA093 },
        { "sysTrapSysTranslateKernelErr", 0xA094 },
        { "sysTrapSysSemaphoreCreate", 0xA095 },
        { "sysTrapSysSemaphoreDelete", 0xA096 },
        { "sysTrapSysSemaphoreWait", 0xA097 },
        { "sysTrapSysSemaphoreSignal", 0xA098 },
        { "sysTrapSysTimerCreate", 0xA099 },
        { "sysTrapSysTimerWrite", 0xA09A },
        { "sysTrapSysTaskCreate", 0xA09B },
        { "sysTrapSysTaskDelete", 0xA09C },
        { "sysTrapSysTaskTrigger", 0xA09D },
        { "sysTrapSysTaskID", 0xA09E },
        { "sysTrapSysTaskUserInfoPtr", 0xA09F },
        { "sysTrapSysTaskDelay", 0xA0A0 },
        { "sysTrapSysTaskSetTermProc", 0xA0A1 },
        { "sysTrapSysUILaunch", 0xA0A2 },
        { "sysTrapSysNewOwnerID", 0xA0A3 },
        { "sysTrapSysSemaphoreSet", 0xA0A4 },
        { "sysTrapSysDisableInts", 0xA0A5 },
        { "sysTrapSysRestoreStatus", 0xA0A6 },
        { "sysTrapSysUIAppSwitch", 0xA0A7 },
        { "sysTrapSysCurAppInfoPV20", 0xA0A8 },
        { "sysTrapSysHandleEvent", 0xA0A9 },
        { "sysTrapSysInit", 0xA0AA },
        { "sysTrapSysQSort", 0xA0AB },
        { "sysTrapSysCurAppDatabase", 0xA0AC },
        { "sysTrapSysFatalAlert", 0xA0AD },
        { "sysTrapSysResSemaphoreCreate", 0xA0AE },
        { "sysTrapSysResSemaphoreDelete", 0xA0AF },
        { "sysTrapSysResSemaphoreReserve", 0xA0B0 },
        { "sysTrapSysResSemaphoreRelease", 0xA0B1 },
        { "sysTrapSysSleep", 0xA0B2 },
        { "sysTrapSysKeyboardDialogV10", 0xA0B3 },
        { "sysTrapSysAppLauncherDialog", 0xA0B4 },
        { "sysTrapSysSetPerformance", 0xA0B5 },
        { "sysTrapSysBatteryInfoV20", 0xA0B6 },
        { "sysTrapSysLibInstall", 0xA0B7 },
        { "sysTrapSysLibRemove", 0xA0B8 },
        { "sysTrapSysLibTblEntry", 0xA0B9 },
        { "sysTrapSysLibFind", 0xA0BA },
        { "sysTrapSysBatteryDialog", 0xA0BB },
        { "sysTrapSysCopyStringResource", 0xA0BC },
        { "sysTrapSysKernelInfo", 0xA0BD },
        { "sysTrapSysLaunchConsole", 0xA0BE },
        { "sysTrapSysTimerDelete", 0xA0BF },
        { "sysTrapSysSetAutoOffTime", 0xA0C0 },
        { "sysTrapSysFormPointerArrayToStrings", 0xA0C1 },
        { "sysTrapSysRandom", 0xA0C2 },
        { "sysTrapSysTaskSwitching", 0xA0C3 },
        { "sysTrapSysTimerRead", 0xA0C4 },
        { "sysTrapStrCopy", 0xA0C5 },
        { "sysTrapStrCat", 0xA0C6 },
        { "sysTrapStrLen", 0xA0C7 },
        { "sysTrapStrCompare", 0xA0C8 },
        { "sysTrapStrIToA", 0xA0C9 },
        { "sysTrapStrCaselessCompare", 0xA0CA },
        { "sysTrapStrIToH", 0xA0CB },
        { "sysTrapStrChr", 0xA0CC },
        { "sysTrapStrStr", 0xA0CD },
        { "sysTrapStrAToI", 0xA0CE },
        { "sysTrapStrToLower", 0xA0CF },
        { "sysTrapSerReceiveISP", 0xA0D0 },
        { "sysTrapSlkOpen", 0xA0D1 },
        { "sysTrapSlkClose", 0xA0D2 },
        { "sysTrapSlkOpenSocket", 0xA0D3 },
        { "sysTrapSlkCloseSocket", 0xA0D4 },
        { "sysTrapSlkSocketRefNum", 0xA0D5 },
        { "sysTrapSlkSocketSetTimeout", 0xA0D6 },
        { "sysTrapSlkFlushSocket", 0xA0D7 },
        { "sysTrapSlkSetSocketListener", 0xA0D8 },
        { "sysTrapSlkSendPacket", 0xA0D9 },
        { "sysTrapSlkReceivePacket", 0xA0DA },
        { "sysTrapSlkSysPktDefaultResponse", 0xA0DB },
        { "sysTrapSlkProcessRPC", 0xA0DC },
        { "sysTrapConPutS", 0xA0DD },
        { "sysTrapConGetS", 0xA0DE },
        { "sysTrapFplInit", 0xA0DF },
        { "sysTrapFplFree", 0xA0E0 },
        { "sysTrapFplFToA", 0xA0E1 },
        { "sysTrapFplAToF", 0xA0E2 },
        { "sysTrapFplBase10Info", 0xA0E3 },
        { "sysTrapFplLongToFloat", 0xA0E4 },
        { "sysTrapFplFloatToLong", 0xA0E5 },
        { "sysTrapFplFloatToULong", 0xA0E6 },
        { "sysTrapFplMul", 0xA0E7 },
        { "sysTrapFplAdd", 0xA0E8 },
        { "sysTrapFplSub", 0xA0E9 },
        { "sysTrapFplDiv", 0xA0EA },
        { "sysTrapWinScreenInit", 0xA0EB },
        { "sysTrapScrCopyRectangle", 0xA0EC },
        { "sysTrapScrDrawChars", 0xA0ED },
        { "sysTrapScrLineRoutine", 0xA0EE },
        { "sysTrapScrRectangleRoutine", 0xA0EF },
        { "sysTrapScrScreenInfo", 0xA0F0 },
        { "sysTrapScrDrawNotify", 0xA0F1 },
        { "sysTrapScrSendUpdateArea", 0xA0F2 },
        { "sysTrapScrCompressScanLine", 0xA0F3 },
        { "sysTrapScrDeCompressScanLine", 0xA0F4 },
        { "sysTrapTimGetSeconds", 0xA0F5 },
        { "sysTrapTimSetSeconds", 0xA0F6 },
        { "sysTrapTimGetTicks", 0xA0F7 },
        { "sysTrapTimInit", 0xA0F8 },
        { "sysTrapTimSetAlarm", 0xA0F9 },
        { "sysTrapTimGetAlarm", 0xA0FA },
        { "sysTrapTimHandleInterrupt", 0xA0FB },
        { "sysTrapTimSecondsToDateTime", 0xA0FC },
        { "sysTrapTimDateTimeToSeconds", 0xA0FD },
        { "sysTrapTimAdjust", 0xA0FE },
        { "sysTrapTimSleep", 0xA0FF },
        { "sysTrapTimWake", 0xA100 },
        { "sysTrapCategoryCreateListV10", 0xA101 },
        { "sysTrapCategoryFreeListV10", 0xA102 },
        { "sysTrapCategoryFind", 0xA103 },
        { "sysTrapCategoryGetName", 0xA104 },
        { "sysTrapCategoryEditV10", 0xA105 },
        { "sysTrapCategorySelectV10", 0xA106 },
        { "sysTrapCategoryGetNext", 0xA107 },
        { "sysTrapCategorySetTriggerLabel", 0xA108 },
        { "sysTrapCategoryTruncateName", 0xA109 },
        { "sysTrapClipboardAddItem", 0xA10A },
        { "sysTrapClipboardCheckIfItemExist", 0xA10B },
        { "sysTrapClipboardGetItem", 0xA10C },
        { "sysTrapCtlDrawControl", 0xA10D },
        { "sysTrapCtlEraseControl", 0xA10E },
        { "sysTrapCtlHideControl", 0xA10F },
        { "sysTrapCtlShowControl", 0xA110 },
        { "sysTrapCtlGetValue", 0xA111 },
        { "sysTrapCtlSetValue", 0xA112 },
        { "sysTrapCtlGetLabel", 0xA113 },
        { "sysTrapCtlSetLabel", 0xA114 },
        { "sysTrapCtlHandleEvent", 0xA115 },
        { "sysTrapCtlHitControl", 0xA116 },
        { "sysTrapCtlSetEnabled", 0xA117 },
        { "sysTrapCtlSetUsable", 0xA118 },
        { "sysTrapCtlEnabled", 0xA119 },
        { "sysTrapEvtInitialize", 0xA11A },
        { "sysTrapEvtAddEventToQueue", 0xA11B },
        { "sysTrapEvtCopyEvent", 0xA11C },
        { "sysTrapEvtGetEvent", 0xA11D },
        { "sysTrapEvtGetPen", 0xA11E },
        { "sysTrapEvtSysInit", 0xA11F },
        { "sysTrapEvtGetSysEvent", 0xA120 },
        { "sysTrapEvtProcessSoftKeyStroke", 0xA121 },
        { "sysTrapEvtGetPenBtnList", 0xA122 },
        { "sysTrapEvtSetPenQueuePtr", 0xA123 },
        { "sysTrapEvtPenQueueSize", 0xA124 },
        { "sysTrapEvtFlushPenQueue", 0xA125 },
        { "sysTrapEvtEnqueuePenPoint", 0xA126 },
        { "sysTrapEvtDequeuePenStrokeInfo", 0xA127 },
        { "sysTrapEvtDequeuePenPoint", 0xA128 },
        { "sysTrapEvtFlushNextPenStroke", 0xA129 },
        { "sysTrapEvtSetKeyQueuePtr", 0xA12A },
        { "sysTrapEvtKeyQueueSize", 0xA12B },
        { "sysTrapEvtFlushKeyQueue", 0xA12C },
        { "sysTrapEvtEnqueueKey", 0xA12D },
        { "sysTrapEvtDequeueKeyEvent", 0xA12E },
        { "sysTrapEvtWakeup", 0xA12F },
        { "sysTrapEvtResetAutoOffTimer", 0xA130 },
        { "sysTrapEvtKeyQueueEmpty", 0xA131 },
        { "sysTrapEvtEnableGraffiti", 0xA132 },
        { "sysTrapFldCopy", 0xA133 },
        { "sysTrapFldCut", 0xA134 },
        { "sysTrapFldDrawField", 0xA135 },
        { "sysTrapFldEraseField", 0xA136 },
        { "sysTrapFldFreeMemory", 0xA137 },
        { "sysTrapFldGetBounds", 0xA138 },
        { "sysTrapFldGetTextPtr", 0xA139 },
        { "sysTrapFldGetSelection", 0xA13A },
        { "sysTrapFldHandleEvent", 0xA13B },
        { "sysTrapFldPaste", 0xA13C },
        { "sysTrapFldRecalculateField", 0xA13D },
        { "sysTrapFldSetBounds", 0xA13E },
        { "sysTrapFldSetText", 0xA13F },
        { "sysTrapFldGetFont", 0xA140 },
        { "sysTrapFldSetFont", 0xA141 },
        { "sysTrapFldSetSelection", 0xA142 },
        { "sysTrapFldGrabFocus", 0xA143 },
        { "sysTrapFldReleaseFocus", 0xA144 },
        { "sysTrapFldGetInsPtPosition", 0xA145 },
        { "sysTrapFldSetInsPtPosition", 0xA146 },
        { "sysTrapFldSetScrollPosition", 0xA147 },
        { "sysTrapFldGetScrollPosition", 0xA148 },
        { "sysTrapFldGetTextHeight", 0xA149 },
        { "sysTrapFldGetTextAllocatedSize", 0xA14A },
        { "sysTrapFldGetTextLength", 0xA14B },
        { "sysTrapFldScrollField", 0xA14C },
        { "sysTrapFldScrollable", 0xA14D },
        { "sysTrapFldGetVisibleLines", 0xA14E },
        { "sysTrapFldGetAttributes", 0xA14F },
        { "sysTrapFldSetAttributes", 0xA150 },
        { "sysTrapFldSendChangeNotification", 0xA151 },
        { "sysTrapFldCalcFieldHeight", 0xA152 },
        { "sysTrapFldGetTextHandle", 0xA153 },
        { "sysTrapFldCompactText", 0xA154 },
        { "sysTrapFldDirty", 0xA155 },
        { "sysTrapFldWordWrap", 0xA156 },
        { "sysTrapFldSetTextAllocatedSize", 0xA157 },
        { "sysTrapFldSetTextHandle", 0xA158 },
        { "sysTrapFldSetTextPtr", 0xA159 },
        { "sysTrapFldGetMaxChars", 0xA15A },
        { "sysTrapFldSetMaxChars", 0xA15B },
        { "sysTrapFldSetUsable", 0xA15C },
        { "sysTrapFldInsert", 0xA15D },
        { "sysTrapFldDelete", 0xA15E },
        { "sysTrapFldUndo", 0xA15F },
        { "sysTrapFldSetDirty", 0xA160 },
        { "sysTrapFldSendHeightChangeNotification", 0xA161 },
        { "sysTrapFldMakeFullyVisible", 0xA162 },
        { "sysTrapFntGetFont", 0xA163 },
        { "sysTrapFntSetFont", 0xA164 },
        { "sysTrapFntGetFontPtr", 0xA165 },
        { "sysTrapFntBaseLine", 0xA166 },
        { "sysTrapFntCharHeight", 0xA167 },
        { "sysTrapFntLineHeight", 0xA168 },
        { "sysTrapFntAverageCharWidth", 0xA169 },
        { "sysTrapFntCharWidth", 0xA16A },
        { "sysTrapFntCharsWidth", 0xA16B },
        { "sysTrapFntDescenderHeight", 0xA16C },
        { "sysTrapFntCharsInWidth", 0xA16D },
        { "sysTrapFntLineWidth", 0xA16E },
        { "sysTrapFrmInitForm", 0xA16F },
        { "sysTrapFrmDeleteForm", 0xA170 },
        { "sysTrapFrmDrawForm", 0xA171 },
        { "sysTrapFrmEraseForm", 0xA172 },
        { "sysTrapFrmGetActiveForm", 0xA173 },
        { "sysTrapFrmSetActiveForm", 0xA174 },
        { "sysTrapFrmGetActiveFormID", 0xA175 },
        { "sysTrapFrmGetUserModifiedState", 0xA176 },
        { "sysTrapFrmSetNotUserModified", 0xA177 },
        { "sysTrapFrmGetFocus", 0xA178 },
        { "sysTrapFrmSetFocus", 0xA179 },
        { "sysTrapFrmHandleEvent", 0xA17A },
        { "sysTrapFrmGetFormBounds", 0xA17B },
        { "sysTrapFrmGetWindowHandle", 0xA17C },
        { "sysTrapFrmGetFormId", 0xA17D },
        { "sysTrapFrmGetFormPtr", 0xA17E },
        { "sysTrapFrmGetNumberOfObjects", 0xA17F },
        { "sysTrapFrmGetObjectIndex", 0xA180 },
        { "sysTrapFrmGetObjectId", 0xA181 },
        { "sysTrapFrmGetObjectType", 0xA182 },
        { "sysTrapFrmGetObjectPtr", 0xA183 },
        { "sysTrapFrmHideObject", 0xA184 },
        { "sysTrapFrmShowObject", 0xA185 },
        { "sysTrapFrmGetObjectPosition", 0xA186 },
        { "sysTrapFrmSetObjectPosition", 0xA187 },
        { "sysTrapFrmGetControlValue", 0xA188 },
        { "sysTrapFrmSetControlValue", 0xA189 },
        { "sysTrapFrmGetControlGroupSelection", 0xA18A },
        { "sysTrapFrmSetControlGroupSelection", 0xA18B },
        { "sysTrapFrmCopyLabel", 0xA18C },
        { "sysTrapFrmSetLabel", 0xA18D },
        { "sysTrapFrmGetLabel", 0xA18E },
        { "sysTrapFrmSetCategoryLabel", 0xA18F },
        { "sysTrapFrmGetTitle", 0xA190 },
        { "sysTrapFrmSetTitle", 0xA191 },
        { "sysTrapFrmAlert", 0xA192 },
        { "sysTrapFrmDoDialog", 0xA193 },
        { "sysTrapFrmCustomAlert", 0xA194 },
        { "sysTrapFrmHelp", 0xA195 },
        { "sysTrapFrmUpdateScrollers", 0xA196 },
        { "sysTrapFrmGetFirstForm", 0xA197 },
        { "sysTrapFrmVisible", 0xA198 },
        { "sysTrapFrmGetObjectBounds", 0xA199 },
        { "sysTrapFrmCopyTitle", 0xA19A },
        { "sysTrapFrmGotoForm", 0xA19B },
        { "sysTrapFrmPopupForm", 0xA19C },
        { "sysTrapFrmUpdateForm", 0xA19D },
        { "sysTrapFrmReturnToForm", 0xA19E },
        { "sysTrapFrmSetEventHandler", 0xA19F },
        { "sysTrapFrmDispatchEvent", 0xA1A0 },
        { "sysTrapFrmCloseAllForms", 0xA1A1 },
        { "sysTrapFrmSaveAllForms", 0xA1A2 },
        { "sysTrapFrmGetGadgetData", 0xA1A3 },
        { "sysTrapFrmSetGadgetData", 0xA1A4 },
        { "sysTrapFrmSetCategoryTrigger", 0xA1A5 },
        { "sysTrapUIInitialize", 0xA1A6 },
        { "sysTrapUIReset", 0xA1A7 },
        { "sysTrapInsPtInitialize", 0xA1A8 },
        { "sysTrapInsPtSetLocation", 0xA1A9 },
        { "sysTrapInsPtGetLocation", 0xA1AA },
        { "sysTrapInsPtEnable", 0xA1AB },
        { "sysTrapInsPtEnabled", 0xA1AC },
        { "sysTrapInsPtSetHeight", 0xA1AD },
        { "sysTrapInsPtGetHeight", 0xA1AE },
        { "sysTrapInsPtCheckBlink", 0xA1AF },
        { "sysTrapLstSetDrawFunction", 0xA1B0 },
        { "sysTrapLstDrawList", 0xA1B1 },
        { "sysTrapLstEraseList", 0xA1B2 },
        { "sysTrapLstGetSelection", 0xA1B3 },
        { "sysTrapLstGetSelectionText", 0xA1B4 },
        { "sysTrapLstHandleEvent", 0xA1B5 },
        { "sysTrapLstSetHeight", 0xA1B6 },
        { "sysTrapLstSetSelection", 0xA1B7 },
        { "sysTrapLstSetListChoices", 0xA1B8 },
        { "sysTrapLstMakeItemVisible", 0xA1B9 },
        { "sysTrapLstGetNumberOfItems", 0xA1BA },
        { "sysTrapLstPopupList", 0xA1BB },
        { "sysTrapLstSetPosition", 0xA1BC },
        { "sysTrapMenuInit", 0xA1BD },
        { "sysTrapMenuDispose", 0xA1BE },
        { "sysTrapMenuHandleEvent", 0xA1BF },
        { "sysTrapMenuDrawMenu", 0xA1C0 },
        { "sysTrapMenuEraseStatus", 0xA1C1 },
        { "sysTrapMenuGetActiveMenu", 0xA1C2 },
        { "sysTrapMenuSetActiveMenu", 0xA1C3 },
        { "sysTrapRctSetRectangle", 0xA1C4 },
        { "sysTrapRctCopyRectangle", 0xA1C5 },
        { "sysTrapRctInsetRectangle", 0xA1C6 },
        { "sysTrapRctOffsetRectangle", 0xA1C7 },
        { "sysTrapRctPtInRectangle", 0xA1C8 },
        { "sysTrapRctGetIntersection", 0xA1C9 },
        { "sysTrapTblDrawTable", 0xA1CA },
        { "sysTrapTblEraseTable", 0xA1CB },
        { "sysTrapTblHandleEvent", 0xA1CC },
        { "sysTrapTblGetItemBounds", 0xA1CD },
        { "sysTrapTblSelectItem", 0xA1CE },
        { "sysTrapTblGetItemInt", 0xA1CF },
        { "sysTrapTblSetItemInt", 0xA1D0 },
        { "sysTrapTblSetItemStyle", 0xA1D1 },
        { "sysTrapTblUnhighlightSelection", 0xA1D2 },
        { "sysTrapTblSetRowUsable", 0xA1D3 },
        { "sysTrapTblGetNumberOfRows", 0xA1D4 },
        { "sysTrapTblSetCustomDrawProcedure", 0xA1D5 },
        { "sysTrapTblSetRowSelectable", 0xA1D6 },
        { "sysTrapTblRowSelectable", 0xA1D7 },
        { "sysTrapTblSetLoadDataProcedure", 0xA1D8 },
        { "sysTrapTblSetSaveDataProcedure", 0xA1D9 },
        { "sysTrapTblGetBounds", 0xA1DA },
        { "sysTrapTblSetRowHeight", 0xA1DB },
        { "sysTrapTblGetColumnWidth", 0xA1DC },
        { "sysTrapTblGetRowID", 0xA1DD },
        { "sysTrapTblSetRowID", 0xA1DE },
        { "sysTrapTblMarkRowInvalid", 0xA1DF },
        { "sysTrapTblMarkTableInvalid", 0xA1E0 },
        { "sysTrapTblGetSelection", 0xA1E1 },
        { "sysTrapTblInsertRow", 0xA1E2 },
        { "sysTrapTblRemoveRow", 0xA1E3 },
        { "sysTrapTblRowInvalid", 0xA1E4 },
        { "sysTrapTblRedrawTable", 0xA1E5 },
        { "sysTrapTblRowUsable", 0xA1E6 },
        { "sysTrapTblReleaseFocus", 0xA1E7 },
        { "sysTrapTblEditing", 0xA1E8 },
        { "sysTrapTblGetCurrentField", 0xA1E9 },
        { "sysTrapTblSetColumnUsable", 0xA1EA },
        { "sysTrapTblGetRowHeight", 0xA1EB },
        { "sysTrapTblSetColumnWidth", 0xA1EC },
        { "sysTrapTblGrabFocus", 0xA1ED },
        { "sysTrapTblSetItemPtr", 0xA1EE },
        { "sysTrapTblFindRowID", 0xA1EF },
        { "sysTrapTblGetLastUsableRow", 0xA1F0 },
        { "sysTrapTblGetColumnSpacing", 0xA1F1 },
        { "sysTrapTblFindRowData", 0xA1F2 },
        { "sysTrapTblGetRowData", 0xA1F3 },
        { "sysTrapTblSetRowData", 0xA1F4 },
        { "sysTrapTblSetColumnSpacing", 0xA1F5 },
        { "sysTrapWinCreateWindow", 0xA1F6 },
        { "sysTrapWinCreateOffscreenWindow", 0xA1F7 },
        { "sysTrapWinDeleteWindow", 0xA1F8 },
        { "sysTrapWinInitializeWindow", 0xA1F9 },
        { "sysTrapWinAddWindow", 0xA1FA },
        { "sysTrapWinRemoveWindow", 0xA1FB },
        { "sysTrapWinSetActiveWindow", 0xA1FC },
        { "sysTrapWinSetDrawWindow", 0xA1FD },
        { "sysTrapWinGetDrawWindow", 0xA1FE },
        { "sysTrapWinGetActiveWindow", 0xA1FF },
        { "sysTrapWinGetDisplayWindow", 0xA200 },
        { "sysTrapWinGetFirstWindow", 0xA201 },
        { "sysTrapWinEnableWindow", 0xA202 },
        { "sysTrapWinDisableWindow", 0xA203 },
        { "sysTrapWinGetWindowFrameRect", 0xA204 },
        { "sysTrapWinDrawWindowFrame", 0xA205 },
        { "sysTrapWinEraseWindow", 0xA206 },
        { "sysTrapWinSaveBits", 0xA207 },
        { "sysTrapWinRestoreBits", 0xA208 },
        { "sysTrapWinCopyRectangle", 0xA209 },
        { "sysTrapWinScrollRectangle", 0xA20A },
        { "sysTrapWinGetDisplayExtent", 0xA20B },
        { "sysTrapWinGetWindowExtent", 0xA20C },
        { "sysTrapWinDisplayToWindowPt", 0xA20D },
        { "sysTrapWinWindowToDisplayPt", 0xA20E },
        { "sysTrapWinGetClip", 0xA20F },
        { "sysTrapWinSetClip", 0xA210 },
        { "sysTrapWinResetClip", 0xA211 },
        { "sysTrapWinClipRectangle", 0xA212 },
        { "sysTrapWinDrawLine", 0xA213 },
        { "sysTrapWinDrawGrayLine", 0xA214 },
        { "sysTrapWinEraseLine", 0xA215 },
        { "sysTrapWinInvertLine", 0xA216 },
        { "sysTrapWinFillLine", 0xA217 },
        { "sysTrapWinDrawRectangle", 0xA218 },
        { "sysTrapWinEraseRectangle", 0xA219 },
        { "sysTrapWinInvertRectangle", 0xA21A },
        { "sysTrapWinDrawRectangleFrame", 0xA21B },
        { "sysTrapWinDrawGrayRectangleFrame", 0xA21C },
        { "sysTrapWinEraseRectangleFrame", 0xA21D },
        { "sysTrapWinInvertRectangleFrame", 0xA21E },
        { "sysTrapWinGetFramesRectangle", 0xA21F },
        { "sysTrapWinDrawChars", 0xA220 },
        { "sysTrapWinEraseChars", 0xA221 },
        { "sysTrapWinInvertChars", 0xA222 },
        { "sysTrapWinGetPattern", 0xA223 },
        { "sysTrapWinSetPattern", 0xA224 },
        { "sysTrapWinSetUnderlineMode", 0xA225 },
        { "sysTrapWinDrawBitmap", 0xA226 },
        { "sysTrapWinModal", 0xA227 },
        { "sysTrapWinGetDrawWindowBounds", 0xA228 },
        { "sysTrapWinFillRectangle", 0xA229 },
        { "sysTrapWinDrawInvertedChars", 0xA22A },
        { "sysTrapPrefOpenPreferenceDBV10", 0xA22B },
        { "sysTrapPrefGetPreferences", 0xA22C },
        { "sysTrapPrefSetPreferences", 0xA22D },
        { "sysTrapPrefGetAppPreferencesV10", 0xA22E },
        { "sysTrapPrefSetAppPreferencesV10", 0xA22F },
        { "sysTrapSndInit", 0xA230 },
        { "sysTrapSndSetDefaultVolume", 0xA231 },
        { "sysTrapSndGetDefaultVolume", 0xA232 },
        { "sysTrapSndDoCmd", 0xA233 },
        { "sysTrapSndPlaySystemSound", 0xA234 },
        { "sysTrapAlmInit", 0xA235 },
        { "sysTrapAlmCancelAll", 0xA236 },
        { "sysTrapAlmAlarmCallback", 0xA237 },
        { "sysTrapAlmSetAlarm", 0xA238 },
        { "sysTrapAlmGetAlarm", 0xA239 },
        { "sysTrapAlmDisplayAlarm", 0xA23A },
        { "sysTrapAlmEnableNotification", 0xA23B },
        { "sysTrapHwrGetRAMMapping", 0xA23C },
        { "sysTrapHwrMemWritable", 0xA23D },
        { "sysTrapHwrMemReadable", 0xA23E },
        { "sysTrapHwrDoze", 0xA23F },
        { "sysTrapHwrSleep", 0xA240 },
        { "sysTrapHwrWake", 0xA241 },
        { "sysTrapHwrSetSystemClock", 0xA242 },
        { "sysTrapHwrSetCPUDutyCycle", 0xA243 },
        { "sysTrapHwrDisplayInit", 0xA244 },
        { "sysTrapHwrDisplaySleep", 0xA245 },
        { "sysTrapHwrTimerInit", 0xA246 },
        { "sysTrapHwrCursorV33", 0xA247 },
        { "sysTrapHwrBatteryLevel", 0xA248 },
        { "sysTrapHwrDelay", 0xA249 },
        { "sysTrapHwrEnableDataWrites", 0xA24A },
        { "sysTrapHwrDisableDataWrites", 0xA24B },
        { "sysTrapHwrLCDBaseAddrV33", 0xA24C },
        { "sysTrapHwrDisplayDrawBootScreen", 0xA24D },
        { "sysTrapHwrTimerSleep", 0xA24E },
        { "sysTrapHwrTimerWake", 0xA24F },
        { "sysTrapHwrDisplayWake", 0xA250 },
        { "sysTrapHwrIRQ1Handler", 0xA251 },
        { "sysTrapHwrIRQ2Handler", 0xA252 },
        { "sysTrapHwrIRQ3Handler", 0xA253 },
        { "sysTrapHwrIRQ4Handler", 0xA254 },
        { "sysTrapHwrIRQ5Handler", 0xA255 },
        { "sysTrapHwrIRQ6Handler", 0xA256 },
        { "sysTrapHwrDockSignals", 0xA257 },
        { "sysTrapHwrPluggedIn", 0xA258 },
        { "sysTrapCrc16CalcBlock", 0xA259 },
        { "sysTrapSelectDayV10", 0xA25A },
        { "sysTrapSelectTimeV33", 0xA25B },
        { "sysTrapDayDrawDaySelector", 0xA25C },
        { "sysTrapDayHandleEvent", 0xA25D },
        { "sysTrapDayDrawDays", 0xA25E },
        { "sysTrapDayOfWeek", 0xA25F },
        { "sysTrapDaysInMonth", 0xA260 },
        { "sysTrapDayOfMonth", 0xA261 },
        { "sysTrapDateDaysToDate", 0xA262 },
        { "sysTrapDateToDays", 0xA263 },
        { "sysTrapDateAdjust", 0xA264 },
        { "sysTrapDateSecondsToDate", 0xA265 },
        { "sysTrapDateToAscii", 0xA266 },
        { "sysTrapDateToDOWDMFormat", 0xA267 },
        { "sysTrapTimeToAscii", 0xA268 },
        { "sysTrapFind", 0xA269 },
        { "sysTrapFindStrInStr", 0xA26A },
        { "sysTrapFindSaveMatch", 0xA26B },
        { "sysTrapFindGetLineBounds", 0xA26C },
        { "sysTrapFindDrawHeader", 0xA26D },
        { "sysTrapPenOpen", 0xA26E },
        { "sysTrapPenClose", 0xA26F },
        { "sysTrapPenGetRawPen", 0xA270 },
        { "sysTrapPenCalibrate", 0xA271 },
        { "sysTrapPenRawToScreen", 0xA272 },
        { "sysTrapPenScreenToRaw", 0xA273 },
        { "sysTrapPenResetCalibration", 0xA274 },
        { "sysTrapPenSleep", 0xA275 },
        { "sysTrapPenWake", 0xA276 },
        { "sysTrapResLoadForm", 0xA277 },
        { "sysTrapResLoadMenu", 0xA278 },
        { "sysTrapFtrInit", 0xA279 },
        { "sysTrapFtrUnregister", 0xA27A },
        { "sysTrapFtrGet", 0xA27B },
        { "sysTrapFtrSet", 0xA27C },
        { "sysTrapFtrGetByIndex", 0xA27D },
        { "sysTrapGrfInit", 0xA27E },
        { "sysTrapGrfFree", 0xA27F },
        { "sysTrapGrfGetState", 0xA280 },
        { "sysTrapGrfSetState", 0xA281 },
        { "sysTrapGrfFlushPoints", 0xA282 },
        { "sysTrapGrfAddPoint", 0xA283 },
        { "sysTrapGrfInitState", 0xA284 },
        { "sysTrapGrfCleanState", 0xA285 },
        { "sysTrapGrfMatch", 0xA286 },
        { "sysTrapGrfGetMacro", 0xA287 },
        { "sysTrapGrfFilterPoints", 0xA288 },
        { "sysTrapGrfGetNumPoints", 0xA289 },
        { "sysTrapGrfGetPoint", 0xA28A },
        { "sysTrapGrfFindBranch", 0xA28B },
        { "sysTrapGrfMatchGlyph", 0xA28C },
        { "sysTrapGrfGetGlyphMapping", 0xA28D },
        { "sysTrapGrfGetMacroName", 0xA28E },
        { "sysTrapGrfDeleteMacro", 0xA28F },
        { "sysTrapGrfAddMacro", 0xA290 },
        { "sysTrapGrfGetAndExpandMacro", 0xA291 },
        { "sysTrapGrfProcessStroke", 0xA292 },
        { "sysTrapGrfFieldChange", 0xA293 },
        { "sysTrapGetCharSortValue", 0xA294 },
        { "sysTrapGetCharAttr", 0xA295 },
        { "sysTrapGetCharCaselessValue", 0xA296 },
        { "sysTrapPwdExists", 0xA297 },
        { "sysTrapPwdVerify", 0xA298 },
        { "sysTrapPwdSet", 0xA299 },
        { "sysTrapPwdRemove", 0xA29A },
        { "sysTrapGsiInitialize", 0xA29B },
        { "sysTrapGsiSetLocation", 0xA29C },
        { "sysTrapGsiEnable", 0xA29D },
        { "sysTrapGsiEnabled", 0xA29E },
        { "sysTrapGsiSetShiftState", 0xA29F },
        { "sysTrapKeyInit", 0xA2A0 },
        { "sysTrapKeyHandleInterrupt", 0xA2A1 },
        { "sysTrapKeyCurrentState", 0xA2A2 },
        { "sysTrapKeyResetDoubleTap", 0xA2A3 },
        { "sysTrapKeyRates", 0xA2A4 },
        { "sysTrapKeySleep", 0xA2A5 },
        { "sysTrapKeyWake", 0xA2A6 },
        { "sysTrapDlkControl", 0xA2A7 },
        { "sysTrapDlkStartServer", 0xA2A8 },
        { "sysTrapDlkGetSyncInfo", 0xA2A9 },
        { "sysTrapDlkSetLogEntry", 0xA2AA },
        { "sysTrapIntlDispatch", 0xA2AB },
        { "sysTrapSysLibLoad", 0xA2AC },
        { "sysTrapSndPlaySmf", 0xA2AD },
        { "sysTrapSndCreateMidiList", 0xA2AE },
        { "sysTrapAbtShowAbout", 0xA2AF },
        { "sysTrapMdmDial", 0xA2B0 },
        { "sysTrapMdmHangUp", 0xA2B1 },
        { "sysTrapDmSearchRecord", 0xA2B2 },
        { "sysTrapSysInsertionSort", 0xA2B3 },
        { "sysTrapDmInsertionSort", 0xA2B4 },
        { "sysTrapLstSetTopItem", 0xA2B5 },
        { "sysTrapSclSetScrollBar", 0xA2B6 },
        { "sysTrapSclDrawScrollBar", 0xA2B7 },
        { "sysTrapSclHandleEvent", 0xA2B8 },
        { "sysTrapSysMailboxCreate", 0xA2B9 },
        { "sysTrapSysMailboxDelete", 0xA2BA },
        { "sysTrapSysMailboxFlush", 0xA2BB },
        { "sysTrapSysMailboxSend", 0xA2BC },
        { "sysTrapSysMailboxWait", 0xA2BD },
        { "sysTrapSysTaskWait", 0xA2BE },
        { "sysTrapSysTaskWake", 0xA2BF },
        { "sysTrapSysTaskWaitClr", 0xA2C0 },
        { "sysTrapSysTaskSuspend", 0xA2C1 },
        { "sysTrapSysTaskResume", 0xA2C2 },
        { "sysTrapCategoryCreateList", 0xA2C3 },
        { "sysTrapCategoryFreeList", 0xA2C4 },
        { "sysTrapCategoryEditV20", 0xA2C5 },
        { "sysTrapCategorySelect", 0xA2C6 },
        { "sysTrapDmDeleteCategory", 0xA2C7 },
        { "sysTrapSysEvGroupCreate", 0xA2C8 },
        { "sysTrapSysEvGroupSignal", 0xA2C9 },
        { "sysTrapSysEvGroupRead", 0xA2CA },
        { "sysTrapSysEvGroupWait", 0xA2CB },
        { "sysTrapEvtEventAvail", 0xA2CC },
        { "sysTrapEvtSysEventAvail", 0xA2CD },
        { "sysTrapStrNCopy", 0xA2CE },
        { "sysTrapKeySetMask", 0xA2CF },
        { "sysTrapSelectDay", 0xA2D0 },
        { "sysTrapPrefGetPreference", 0xA2D1 },
        { "sysTrapPrefSetPreference", 0xA2D2 },
        { "sysTrapPrefGetAppPreferences", 0xA2D3 },
        { "sysTrapPrefSetAppPreferences", 0xA2D4 },
        { "sysTrapFrmPointInTitle", 0xA2D5 },
        { "sysTrapStrNCat", 0xA2D6 },
        { "sysTrapMemCmp", 0xA2D7 },
        { "sysTrapTblSetColumnEditIndicator", 0xA2D8 },
        { "sysTrapFntWordWrap", 0xA2D9 },
        { "sysTrapFldGetScrollValues", 0xA2DA },
        { "sysTrapSysCreateDataBaseList", 0xA2DB },
        { "sysTrapSysCreatePanelList", 0xA2DC },
        { "sysTrapDlkDispatchRequest", 0xA2DD },
        { "sysTrapStrPrintF", 0xA2DE },
        { "sysTrapStrVPrintF", 0xA2DF },
        { "sysTrapPrefOpenPreferenceDB", 0xA2E0 },
        { "sysTrapSysGraffitiReferenceDialog", 0xA2E1 },
        { "sysTrapSysKeyboardDialog", 0xA2E2 },
        { "sysTrapFntWordWrapReverseNLines", 0xA2E3 },
        { "sysTrapFntGetScrollValues", 0xA2E4 },
        { "sysTrapTblSetRowStaticHeight", 0xA2E5 },
        { "sysTrapTblHasScrollBar", 0xA2E6 },
        { "sysTrapSclGetScrollBar", 0xA2E7 },
        { "sysTrapFldGetNumberOfBlankLines", 0xA2E8 },
        { "sysTrapSysTicksPerSecond", 0xA2E9 },
        { "sysTrapHwrBacklightV33", 0xA2EA },
        { "sysTrapDmDatabaseProtect", 0xA2EB },
        { "sysTrapTblSetBounds", 0xA2EC },
        { "sysTrapStrNCompare", 0xA2ED },
        { "sysTrapStrNCaselessCompare", 0xA2EE },
        { "sysTrapPhoneNumberLookup", 0xA2EF },
        { "sysTrapFrmSetMenu", 0xA2F0 },
        { "sysTrapEncDigestMD5", 0xA2F1 },
        { "sysTrapDmFindSortPosition", 0xA2F2 },
        { "sysTrapSysBinarySearch", 0xA2F3 },
        { "sysTrapSysErrString", 0xA2F4 },
        { "sysTrapSysStringByIndex", 0xA2F5 },
        { "sysTrapEvtAddUniqueEventToQueue", 0xA2F6 },
        { "sysTrapStrLocalizeNumber", 0xA2F7 },
        { "sysTrapStrDelocalizeNumber", 0xA2F8 },
        { "sysTrapLocGetNumberSeparators", 0xA2F9 },
        { "sysTrapMenuSetActiveMenuRscID", 0xA2FA },
        { "sysTrapLstScrollList", 0xA2FB },
        { "sysTrapCategoryInitialize", 0xA2FC },
        { "sysTrapEncDigestMD4", 0xA2FD },
        { "sysTrapEncDES", 0xA2FE },
        { "sysTrapLstGetVisibleItems", 0xA2FF },
        { "sysTrapWinSetBounds", 0xA300 },
        { "sysTrapCategorySetName", 0xA301 },
        { "sysTrapFldSetInsertionPoint", 0xA302 },
        { "sysTrapFrmSetObjectBounds", 0xA303 },
        { "sysTrapWinSetColors", 0xA304 },
        { "sysTrapFlpDispatch", 0xA305 },
        { "sysTrapFlpEmDispatch", 0xA306 },
        { "sysTrapExgInit", 0xA307 },
        { "sysTrapExgConnect", 0xA308 },
        { "sysTrapExgPut", 0xA309 },
        { "sysTrapExgGet", 0xA30A },
        { "sysTrapExgAccept", 0xA30B },
        { "sysTrapExgDisconnect", 0xA30C },
        { "sysTrapExgSend", 0xA30D },
        { "sysTrapExgReceive", 0xA30E },
        { "sysTrapExgRegisterData", 0xA30F },
        { "sysTrapExgNotifyReceiveV35", 0xA310 },
        { "sysTrapSysReserved30Trap2", 0xA311 },
        { "sysTrapPrgStartDialogV31", 0xA312 },
        { "sysTrapPrgStopDialog", 0xA313 },
        { "sysTrapPrgUpdateDialog", 0xA314 },
        { "sysTrapPrgHandleEvent", 0xA315 },
        { "sysTrapImcReadFieldNoSemicolon", 0xA316 },
        { "sysTrapImcReadFieldQuotablePrintable", 0xA317 },
        { "sysTrapImcReadPropertyParameter", 0xA318 },
        { "sysTrapImcSkipAllPropertyParameters", 0xA319 },
        { "sysTrapImcReadWhiteSpace", 0xA31A },
        { "sysTrapImcWriteQuotedPrintable", 0xA31B },
        { "sysTrapImcWriteNoSemicolon", 0xA31C },
        { "sysTrapImcStringIsAscii", 0xA31D },
        { "sysTrapTblGetItemFont", 0xA31E },
        { "sysTrapTblSetItemFont", 0xA31F },
        { "sysTrapFontSelect", 0xA320 },
        { "sysTrapFntDefineFont", 0xA321 },
        { "sysTrapCategoryEdit", 0xA322 },
        { "sysTrapSysGetOSVersionString", 0xA323 },
        { "sysTrapSysBatteryInfo", 0xA324 },
        { "sysTrapSysUIBusy", 0xA325 },
        { "sysTrapWinValidateHandle", 0xA326 },
        { "sysTrapFrmValidatePtr", 0xA327 },
        { "sysTrapCtlValidatePointer", 0xA328 },
        { "sysTrapWinMoveWindowAddr", 0xA329 },
        { "sysTrapFrmAddSpaceForObject", 0xA32A },
        { "sysTrapFrmNewForm", 0xA32B },
        { "sysTrapCtlNewControl", 0xA32C },
        { "sysTrapFldNewField", 0xA32D },
        { "sysTrapLstNewList", 0xA32E },
        { "sysTrapFrmNewLabel", 0xA32F },
        { "sysTrapFrmNewBitmap", 0xA330 },
        { "sysTrapFrmNewGadget", 0xA331 },
        { "sysTrapFileOpen", 0xA332 },
        { "sysTrapFileClose", 0xA333 },
        { "sysTrapFileDelete", 0xA334 },
        { "sysTrapFileReadLow", 0xA335 },
        { "sysTrapFileWrite", 0xA336 },
        { "sysTrapFileSeek", 0xA337 },
        { "sysTrapFileTell", 0xA338 },
        { "sysTrapFileTruncate", 0xA339 },
        { "sysTrapFileControl", 0xA33A },
        { "sysTrapFrmActiveState", 0xA33B },
        { "sysTrapSysGetAppInfo", 0xA33C },
        { "sysTrapSysGetStackInfo", 0xA33D },
        { "sysTrapWinScreenMode", 0xA33E },
        { "sysTrapHwrLCDGetDepthV33", 0xA33F },
        { "sysTrapHwrGetROMToken", 0xA340 },
        { "sysTrapDbgControl", 0xA341 },
        { "sysTrapExgDBRead", 0xA342 },
        { "sysTrapExgDBWrite", 0xA343 },
        { "sysTrapHostControl", 0xA344 },
        { "sysTrapFrmRemoveObject", 0xA345 },
        { "sysTrapSysReserved30Trap1", 0xA346 },
        { "sysTrapExpansionDispatch", 0xA347 },
        { "sysTrapFileSystemDispatch", 0xA348 },
        { "sysTrapOEMDispatch", 0xA349 },
        { "sysTrapHwrLCDContrastV33", 0xA34A },
        { "sysTrapSysLCDContrast", 0xA34B },
        { "sysTrapUIContrastAdjust", 0xA34C },
        { "sysTrapHwrDockStatus", 0xA34D },
        { "sysTrapFntWidthToOffset", 0xA34E },
        { "sysTrapSelectOneTime", 0xA34F },
        { "sysTrapWinDrawChar", 0xA350 },
        { "sysTrapWinDrawTruncChars", 0xA351 },
        { "sysTrapSysNotifyInit", 0xA352 },
        { "sysTrapSysNotifyRegister", 0xA353 },
        { "sysTrapSysNotifyUnregister", 0xA354 },
        { "sysTrapSysNotifyBroadcast", 0xA355 },
        { "sysTrapSysNotifyBroadcastDeferred", 0xA356 },
        { "sysTrapSysNotifyDatabaseAdded", 0xA357 },
        { "sysTrapSysNotifyDatabaseRemoved", 0xA358 },
        { "sysTrapSysWantEvent", 0xA359 },
        { "sysTrapFtrPtrNew", 0xA35A },
        { "sysTrapFtrPtrFree", 0xA35B },
        { "sysTrapFtrPtrResize", 0xA35C },
        { "sysTrapSysReserved31Trap1", 0xA35D },
        { "sysTrapHwrNVPrefSet", 0xA35E },
        { "sysTrapHwrNVPrefGet", 0xA35F },
        { "sysTrapFlashInit", 0xA360 },
        { "sysTrapFlashCompress", 0xA361 },
        { "sysTrapFlashErase", 0xA362 },
        { "sysTrapFlashProgram", 0xA363 },
        { "sysTrapAlmTimeChange", 0xA364 },
        { "sysTrapErrAlertCustom", 0xA365 },
        { "sysTrapPrgStartDialog", 0xA366 },
        { "sysTrapSerialDispatch", 0xA367 },
        { "sysTrapHwrBattery", 0xA368 },
        { "sysTrapDmGetDatabaseLockState", 0xA369 },
        { "sysTrapCncGetProfileList", 0xA36A },
        { "sysTrapCncGetProfileInfo", 0xA36B },
        { "sysTrapCncAddProfile", 0xA36C },
        { "sysTrapCncDeleteProfile", 0xA36D },
        { "sysTrapSndPlaySmfResource", 0xA36E },
        { "sysTrapMemPtrDataStorage", 0xA36F },
        { "sysTrapClipboardAppendItem", 0xA370 },
        { "sysTrapWiCmdV32", 0xA371 },
        { "sysTrapHwrDisplayAttributes", 0xA372 },
        { "sysTrapHwrDisplayDoze", 0xA373 },
        { "sysTrapHwrDisplayPalette", 0xA374 },
        { "sysTrapBltFindIndexes", 0xA375 },
        { "sysTrapBmpGetBits", 0xA376 },
        { "sysTrapBltCopyRectangle", 0xA377 },
        { "sysTrapBltDrawChars", 0xA378 },
        { "sysTrapBltLineRoutine", 0xA379 },
        { "sysTrapBltRectangleRoutine", 0xA37A },
        { "sysTrapScrCompress", 0xA37B },
        { "sysTrapScrDecompress", 0xA37C },
        { "sysTrapSysLCDBrightness", 0xA37D },
        { "sysTrapWinPaintChar", 0xA37E },
        { "sysTrapWinPaintChars", 0xA37F },
        { "sysTrapWinPaintBitmap", 0xA380 },
        { "sysTrapWinGetPixel", 0xA381 },
        { "sysTrapWinPaintPixel", 0xA382 },
        { "sysTrapWinDrawPixel", 0xA383 },
        { "sysTrapWinErasePixel", 0xA384 },
        { "sysTrapWinInvertPixel", 0xA385 },
        { "sysTrapWinPaintPixels", 0xA386 },
        { "sysTrapWinPaintLines", 0xA387 },
        { "sysTrapWinPaintLine", 0xA388 },
        { "sysTrapWinPaintRectangle", 0xA389 },
        { "sysTrapWinPaintRectangleFrame", 0xA38A },
        { "sysTrapWinPaintPolygon", 0xA38B },
        { "sysTrapWinDrawPolygon", 0xA38C },
        { "sysTrapWinErasePolygon", 0xA38D },
        { "sysTrapWinInvertPolygon", 0xA38E },
        { "sysTrapWinFillPolygon", 0xA38F },
        { "sysTrapWinPaintArc", 0xA390 },
        { "sysTrapWinDrawArc", 0xA391 },
        { "sysTrapWinEraseArc", 0xA392 },
        { "sysTrapWinInvertArc", 0xA393 },
        { "sysTrapWinFillArc", 0xA394 },
        { "sysTrapWinPushDrawState", 0xA395 },
        { "sysTrapWinPopDrawState", 0xA396 },
        { "sysTrapWinSetDrawMode", 0xA397 },
        { "sysTrapWinSetForeColor", 0xA398 },
        { "sysTrapWinSetBackColor", 0xA399 },
        { "sysTrapWinSetTextColor", 0xA39A },
        { "sysTrapWinGetPatternType", 0xA39B },
        { "sysTrapWinSetPatternType", 0xA39C },
        { "sysTrapWinPalette", 0xA39D },
        { "sysTrapWinRGBToIndex", 0xA39E },
        { "sysTrapWinIndexToRGB", 0xA39F },
        { "sysTrapWinScreenLock", 0xA3A0 },
        { "sysTrapWinScreenUnlock", 0xA3A1 },
        { "sysTrapWinGetBitmap", 0xA3A2 },
        { "sysTrapUIColorInit", 0xA3A3 },
        { "sysTrapUIColorGetTableEntryIndex", 0xA3A4 },
        { "sysTrapUIColorGetTableEntryRGB", 0xA3A5 },
        { "sysTrapUIColorSetTableEntry", 0xA3A6 },
        { "sysTrapUIColorPushTable", 0xA3A7 },
        { "sysTrapUIColorPopTable", 0xA3A8 },
        { "sysTrapCtlNewGraphicControl", 0xA3A9 },
        { "sysTrapTblGetItemPtr", 0xA3AA },
        { "sysTrapUIBrightnessAdjust", 0xA3AB },
        { "sysTrapUIPickColor", 0xA3AC },
        { "sysTrapEvtSetAutoOffTimer", 0xA3AD },
        { "sysTrapTsmDispatch", 0xA3AE },
        { "sysTrapOmDispatch", 0xA3AF },
        { "sysTrapDmOpenDBNoOverlay", 0xA3B0 },
        { "sysTrapDmOpenDBWithLocale", 0xA3B1 },
        { "sysTrapResLoadConstant", 0xA3B2 },
        { "sysTrapHwrPreDebugInit", 0xA3B3 },
        { "sysTrapHwrResetNMI", 0xA3B4 },
        { "sysTrapHwrResetPWM", 0xA3B5 },
        { "sysTrapKeyBootKeys", 0xA3B6 },
        { "sysTrapDbgSerDrvOpen", 0xA3B7 },
        { "sysTrapDbgSerDrvClose", 0xA3B8 },
        { "sysTrapDbgSerDrvControl", 0xA3B9 },
        { "sysTrapDbgSerDrvStatus", 0xA3BA },
        { "sysTrapDbgSerDrvWriteChar", 0xA3BB },
        { "sysTrapDbgSerDrvReadChar", 0xA3BC },
        { "sysTrapHwrPostDebugInit", 0xA3BD },
        { "sysTrapHwrIdentifyFeatures", 0xA3BE },
        { "sysTrapHwrModelSpecificInit", 0xA3BF },
        { "sysTrapHwrModelInitStage2", 0xA3C0 },
        { "sysTrapHwrInterruptsInit", 0xA3C1 },
        { "sysTrapHwrSoundOn", 0xA3C2 },
        { "sysTrapHwrSoundOff", 0xA3C3 },
        { "sysTrapSysKernelClockTick", 0xA3C4 },
        { "sysTrapMenuEraseMenu", 0xA3C5 },
        { "sysTrapSelectTime", 0xA3C6 },
        { "sysTrapMenuCmdBarAddButton", 0xA3C7 },
        { "sysTrapMenuCmdBarGetButtonData", 0xA3C8 },
        { "sysTrapMenuCmdBarDisplay", 0xA3C9 },
        { "sysTrapHwrGetSilkscreenID", 0xA3CA },
        { "sysTrapEvtGetSilkscreenAreaList", 0xA3CB },
        { "sysTrapSysFatalAlertInit", 0xA3CC },
        { "sysTrapDateTemplateToAscii", 0xA3CD },
        { "sysTrapSecVerifyPW", 0xA3CE },
        { "sysTrapSecSelectViewStatus", 0xA3CF },
        { "sysTrapTblSetColumnMasked", 0xA3D0 },
        { "sysTrapTblSetRowMasked", 0xA3D1 },
        { "sysTrapTblRowMasked", 0xA3D2 },
        { "sysTrapFrmCustomResponseAlert", 0xA3D3 },
        { "sysTrapFrmNewGsi", 0xA3D4 },
        { "sysTrapMenuShowItem", 0xA3D5 },
        { "sysTrapMenuHideItem", 0xA3D6 },
        { "sysTrapMenuAddItem", 0xA3D7 },
        { "sysTrapFrmSetGadgetHandler", 0xA3D8 },
        { "sysTrapCtlSetGraphics", 0xA3D9 },
        { "sysTrapCtlGetSliderValues", 0xA3DA },
        { "sysTrapCtlSetSliderValues", 0xA3DB },
        { "sysTrapCtlNewSliderControl", 0xA3DC },
        { "sysTrapBmpCreate", 0xA3DD },
        { "sysTrapBmpDelete", 0xA3DE },
        { "sysTrapBmpCompress", 0xA3DF },
        { "sysTrapBmpGetColortable", 0xA3E0 },
        { "sysTrapBmpSize", 0xA3E1 },
        { "sysTrapBmpBitsSize", 0xA3E2 },
        { "sysTrapBmpColortableSize", 0xA3E3 },
        { "sysTrapWinCreateBitmapWindow", 0xA3E4 },
        { "sysTrapEvtSetNullEventTick", 0xA3E5 },
        { "sysTrapExgDoDialog", 0xA3E6 },
        { "sysTrapSysUICleanup", 0xA3E7 },
        { "sysTrapWinSetForeColorRGB", 0xA3E8 },
        { "sysTrapWinSetBackColorRGB", 0xA3E9 },
        { "sysTrapWinSetTextColorRGB", 0xA3EA },
        { "sysTrapWinGetPixelRGB", 0xA3EB },
        { "sysTrapHighDensityDispatch", 0xA3EC },
        { "sysTrapSysReserved40Trap2", 0xA3ED },
        { "sysTrapSysReserved40Trap3", 0xA3EE },
        { "sysTrapSysReserved40Trap4", 0xA3EF },
        { "sysTrapCncMgrDispatch", 0xA3F0 },
        { "sysTrapSysNotifyBroadcastFromInterrupt", 0xA3F1 },
        { "sysTrapEvtWakeupWithoutNilEvent", 0xA3F2 },
        { "sysTrapStrCompareAscii", 0xA3F3 },
        { "sysTrapAccessorDispatch", 0xA3F4 },
        { "sysTrapBltGetPixel", 0xA3F5 },
        { "sysTrapBltPaintPixel", 0xA3F6 },
        { "sysTrapScrScreenInit", 0xA3F7 },
        { "sysTrapScrUpdateScreenBitmap", 0xA3F8 },
        { "sysTrapScrPalette", 0xA3F9 },
        { "sysTrapScrGetColortable", 0xA3FA },
        { "sysTrapScrGetGrayPat", 0xA3FB },
        { "sysTrapScrScreenLock", 0xA3FC },
        { "sysTrapScrScreenUnlock", 0xA3FD },
        { "sysTrapFntPrvGetFontList", 0xA3FE },
        { "sysTrapExgRegisterDatatype", 0xA3FF },
        { "sysTrapExgNotifyReceive", 0xA400 },
        { "sysTrapExgNotifyGoto", 0xA401 },
        { "sysTrapExgRequest", 0xA402 },
        { "sysTrapExgSetDefaultApplication", 0xA403 },
        { "sysTrapExgGetDefaultApplication", 0xA404 },
        { "sysTrapExgGetTargetApplication", 0xA405 },
        { "sysTrapExgGetRegisteredApplications", 0xA406 },
        { "sysTrapExgGetRegisteredTypes", 0xA407 },
        { "sysTrapExgNotifyPreview", 0xA408 },
        { "sysTrapExgControl", 0xA409 },
        { "sysTrapLmDispatch", 0xA40A },
        { "sysTrapMemGetRomNVParams", 0xA40B },
        { "sysTrapFntWCharWidth", 0xA40C },
        { "sysTrapDmFindDatabaseWithTypeCreator", 0xA40D },
        { "sysTrapSelectTimeZone", 0xA40E },
        { "sysTrapTimeZoneToAscii", 0xA40F },
        { "sysTrapStrNCompareAscii", 0xA410 },
        { "sysTrapTimTimeZoneToUTC", 0xA411 },
        { "sysTrapTimUTCToTimeZone", 0xA412 },
        { "sysTrapPhoneNumberLookupCustom", 0xA413 },
        { "sysTrapHwrDebugSelect", 0xA414 },
        { "sysTrapBltRoundedRectangle", 0xA415 },
        { "sysTrapBltRoundedRectangleFill", 0xA416 },
        { "sysTrapWinPrvInitCanvas", 0xA417 },
        { "sysTrapHwrCalcDynamicHeapSize", 0xA418 },
        { "sysTrapHwrDebuggerEnter", 0xA419 },
        { "sysTrapHwrDebuggerExit", 0xA41A },
        { "sysTrapLstGetTopItem", 0xA41B },
        { "sysTrapHwrModelInitStage3", 0xA41C },
        { "sysTrapAttnIndicatorAllow", 0xA41D },
        { "sysTrapAttnIndicatorAllowed", 0xA41E },
        { "sysTrapAttnIndicatorEnable", 0xA41F },
        { "sysTrapAttnIndicatorEnabled", 0xA420 },
        { "sysTrapAttnIndicatorSetBlinkPattern", 0xA421 },
        { "sysTrapAttnIndicatorGetBlinkPattern", 0xA422 },
        { "sysTrapAttnIndicatorTicksTillNextBlink", 0xA423 },
        { "sysTrapAttnIndicatorCheckBlink", 0xA424 },
        { "sysTrapAttnInitialize", 0xA425 },
        { "sysTrapAttnGetAttention", 0xA426 },
        { "sysTrapAttnUpdate", 0xA427 },
        { "sysTrapAttnForgetIt", 0xA428 },
        { "sysTrapAttnGetCounts", 0xA429 },
        { "sysTrapAttnListOpen", 0xA42A },
        { "sysTrapAttnHandleEvent", 0xA42B },
        { "sysTrapAttnEffectOfEvent", 0xA42C },
        { "sysTrapAttnIterate", 0xA42D },
        { "sysTrapAttnDoSpecialEffects", 0xA42E },
        { "sysTrapAttnDoEmergencySpecialEffects", 0xA42F },
        { "sysTrapAttnAllowClose", 0xA430 },
        { "sysTrapAttnReopen", 0xA431 },
        { "sysTrapAttnEnableNotification", 0xA432 },
        { "sysTrapHwrLEDAttributes", 0xA433 },
        { "sysTrapHwrVibrateAttributes", 0xA434 },
        { "sysTrapSecGetPwdHint", 0xA435 },
        { "sysTrapSecSetPwdHint", 0xA436 },
        { "sysTrapHwrFlashWrite", 0xA437 },
        { "sysTrapKeyboardStatusNew", 0xA438 },
        { "sysTrapKeyboardStatusFree", 0xA439 },
        { "sysTrapKbdSetLayout", 0xA43A },
        { "sysTrapKbdGetLayout", 0xA43B },
        { "sysTrapKbdSetPosition", 0xA43C },
        { "sysTrapKbdGetPosition", 0xA43D },
        { "sysTrapKbdSetShiftState", 0xA43E },
        { "sysTrapKbdGetShiftState", 0xA43F },
        { "sysTrapKbdDraw", 0xA440 },
        { "sysTrapKbdErase", 0xA441 },
        { "sysTrapKbdHandleEvent", 0xA442 },
        { "sysTrapOEMDispatch2", 0xA443 },
        { "sysTrapHwrCustom", 0xA444 },
        { "sysTrapFrmGetActiveField", 0xA445 },
        { "sysTrapSndPlaySmfIrregardless", 0xA446 },
        { "sysTrapSndPlaySmfResourceIrregardless", 0xA447 },
        { "sysTrapSndInterruptSmfIrregardless", 0xA448 },
        { "sysTrapUdaMgrDispatch", 0xA449 },
        { "sysTrapPalmPrivate1", 0xA44A },
        { "sysTrapPalmPrivate2", 0xA44B },
        { "sysTrapPalmPrivate3", 0xA44C },
        { "sysTrapPalmPrivate4", 0xA44D },
        { "sysTrapBmpGetDimensions", 0xA44E },
        { "sysTrapBmpGetBitDepth", 0xA44F },
        { "sysTrapBmpGetNextBitmap", 0xA450 },
        { "sysTrapTblGetNumberOfColumns", 0xA451 },
        { "sysTrapTblGetTopRow", 0xA452 },
        { "sysTrapTblSetSelection", 0xA453 },
        { "sysTrapFrmGetObjectIndexFromPtr", 0xA454 },
        { "sysTrapBmpGetSizes", 0xA455 },
        { "sysTrapWinGetBounds", 0xA456 },
        { "sysTrapBltPaintPixels", 0xA457 },
        { "sysTrapFldSetMaxVisibleLines", 0xA458 },
        { "sysTrapScrDefaultPaletteState", 0xA459 },
        { "sysTrapPceNativeCall", 0xA45A },
        { "sysTrapSndStreamCreate", 0xA45B },
        { "sysTrapSndStreamDelete", 0xA45C },
        { "sysTrapSndStreamStart", 0xA45D },
        { "sysTrapSndStreamPause", 0xA45E },
        { "sysTrapSndStreamStop", 0xA45F },
        { "sysTrapSndStreamSetVolume", 0xA460 },
        { "sysTrapSndStreamGetVolume", 0xA461 },
        { "sysTrapSndPlayResource", 0xA462 },
        { "sysTrapSndStreamSetPan", 0xA463 },
        { "sysTrapSndStreamGetPan", 0xA464 },
        { "sysTrapMultimediaDispatch", 0xA465 },
        { "sysTrapSndStreamCreateExtended", 0xa466 },
        { "sysTrapSndStreamDeviceControl", 0xa467 },
        { "sysTrapBmpCreateVersion3", 0xA468 },
        { "sysTrapECFixedMul", 0xA469 },
        { "sysTrapECFixedDiv", 0xA46A },
        { "sysTrapHALDrawGetSupportedDensity", 0xA46B },
        { "sysTrapHALRedrawInputArea", 0xA46C },
        { "sysTrapGrfBeginStroke", 0xA46D },
        { "sysTrapBmpPrvConvertBitmap", 0xA46E },
        { "sysTrapSysReservedTrap5", 0xA46F },
        { "sysTrapPinsDispatch", 0xA470 },
        { "sysTrapSysReservedTrap1", 0xA471 },
        { "sysTrapSysReservedTrap2", 0xA472 },
        { "sysTrapSysReservedTrap3", 0xA473 },
        { "sysTrapSysReservedTrap4", 0xA474 },
        { "sysTrapLastTrapNumber", 0xA475 }
    };

    int i;

    for (i = 0; i < (sizeof(traps) / sizeof(traps[0])); i++)
    {
        if (traps[i].trap == opcode)
            return traps[i].name;
    }
    return NULL;
}

static CPU_DISASSEMBLE(palm_dasm_override)
{
    UINT16 opcode;
    unsigned result = 0;
    const char *trap;

    opcode = *((UINT16 *) oprom);
    opcode = ((opcode >> 8) & 0x00ff) | ((opcode << 8) & 0xff00);
    if (opcode == 0x4E4F)
    {
        UINT16 callnum = *((UINT16 *) (oprom + 2));
        callnum = ((callnum >> 8) & 0x00ff) | ((callnum << 8) & 0xff00);
        trap = lookup_trap(callnum);
        result = 2;
        if (trap)
        {
            strcpy(buffer, trap);
        }
        else
        {
            sprintf(buffer, "trap    #$f");
        }
    }
    else if ((opcode & 0xF000) == 0xA000)
    {
        sprintf(buffer, "Call Index: %04x", opcode);
        result = 2;
    }
    return result;
}

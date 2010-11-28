/******************************************************************************
*
*  Votrax Type 'N Talk Driver
*  By Jonathan Gevaryahu AKA Lord Nightmare
*  with loads of help (dumping, code disassembly, schematics, and other
*  documentation) from Kevin 'kevtris' Horton
*
*  The Votrax TNT was sold from some time in early 1981 until at least 1983

******************************************************************************/

/* Core includes */
#include "emu.h"
#include "cpu/m6800/m6800.h"
#include "votrtnt.lh"

/* Components */
#include "sound/votrax.h"
#include "machine/6850acia.h"


class votrtnt_state : public driver_device
{
public:
	votrtnt_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

};


/* Devices */


/******************************************************************************
 Address Maps
******************************************************************************/

static ADDRESS_MAP_START(6802_mem, ADDRESS_SPACE_PROGRAM, 8)
    ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0x7fff)
    AM_RANGE(0x0000, 0x07ff) AM_RAM AM_MIRROR(0x9800)/* RAM, 2114*2 (0x400 bytes) mirrored 4x */
	AM_RANGE(0x2000, 0x2000) AM_NOP AM_MIRROR(0x9ffe)//AM_DEVREADWRITE("acia_0",aciastat_r,aciactrl_w) AM_MIRROR(0x9ffe)/* 6850 ACIA */
	AM_RANGE(0x2001, 0x2001) AM_NOP AM_MIRROR(0x9ffe)//AM_DEVREADWRITE("acia_0",aciadata_r,aciadata_w) AM_MIRROR(0x9ffe)/* 6850 ACIA */
	AM_RANGE(0x4000, 0x4000) AM_NOP AM_MIRROR(0x9fff)/* low 6 bits write to 6 bit input of sc-01-a; high 2 bits are ignored (but by adding a buffer chip could be made to control the inflection bits of the sc-01-a which are normally grouned on the tnt) */
	AM_RANGE(0x6000, 0x6fff) AM_ROM AM_MIRROR(0x9000)/* ROM in potted block */
ADDRESS_MAP_END


/******************************************************************************
 Input Ports
******************************************************************************/

static INPUT_PORTS_START(votrtnt)
	PORT_START("DSW1") /* not connected to cpu, each switch is connected directly to the output of a 4040 counter dividing the cpu m1? clock to feed the 6850 ACIA. Setting more than one switch on is a bad idea. see tnt_schematic.jpg */
	PORT_DIPNAME( 0xFF, 0x00, "Baud Rate" )	PORT_DIPLOCATION("SW1:1,2,3,4,5,6,7,8")
	PORT_DIPSETTING(    0x01, "75" )
	PORT_DIPSETTING(    0x02, "150" )
	PORT_DIPSETTING(    0x04, "300" )
	PORT_DIPSETTING(    0x08, "600" )
	PORT_DIPSETTING(    0x10, "1200" )
	PORT_DIPSETTING(    0x20, "2400" )
	PORT_DIPSETTING(    0x40, "4800" )
	PORT_DIPSETTING(    0x80, "9600" )
INPUT_PORTS_END


/******************************************************************************
 Machine Drivers
******************************************************************************/

static MACHINE_CONFIG_START( votrtnt, votrtnt_state )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu", M6802, XTAL_2_4576MHz)  /* 2.4576MHz XTAL, verified; divided by 4 inside the m6802*/
    MDRV_CPU_PROGRAM_MAP(6802_mem)
    MDRV_QUANTUM_TIME(HZ(60))

    /* video hardware */
    MDRV_DEFAULT_LAYOUT(layout_votrtnt)

    /* serial hardware */
    //MDRV_ACIA6850_ADD("acia_0", acia_intf)

    /* sound hardware */
	//MDRV_SPEAKER_STANDARD_MONO("mono")
	//MDRV_SOUND_ADD("sc-01-a", VOTRAX, 1700000) /* 1.70 MHz? needs verify */
	//MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	/* printer */
	//MDRV_PRINTER_ADD("printer")

MACHINE_CONFIG_END



/******************************************************************************
 ROM Definitions
******************************************************************************/

ROM_START(votrtnt)
    ROM_REGION(0x10000, "maincpu", 0)
	ROM_LOAD("cn49752n.bin", 0x6000, 0x1000, CRC(a44e1af3) SHA1(af83b9e84f44c126b24ee754a22e34ca992a8d3d)) /* 2332 mask rom inside potted brick */

ROM_END



/******************************************************************************
 Stuff that belongs in machine/votrtnt.c
******************************************************************************/
/*
static ACIA6850_INTERFACE( acia_intf )
{
    9600, // rx clock, actually based on dipswitches
    9600, // tx clock, actually based on dipswitches
    DEVCB_NULL, // rx callback
    DEVCB_NULL, // tx callback
    DEVCB_NULL,
    DEVCB_NULL,
    DEVCB_NULL,
    DEVCB_NULL
};*/

/******************************************************************************
 Drivers
******************************************************************************/

/*    YEAR  NAME        PARENT      COMPAT  MACHINE     INPUT   INIT      COMPANY                     FULLNAME                            FLAGS */
COMP( 1980, votrtnt,   0,          0,      votrtnt,   votrtnt, 0,     "Votrax",        "Type 'N Talk",                        GAME_NOT_WORKING | GAME_NO_SOUND)


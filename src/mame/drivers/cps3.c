/*

Capcom CP SYSTEM III Hardware Overview
Capcom, 1996-1999

From late 1996 to 1999 Capcom developed another hardware platform to rival the CPS2 System and called
it CP SYSTEM III. Only 6 games were produced....
                                                                                        CD            Security
Game                                                 Date   S/W Rev        CD Part#     Label         Cart Part#
----------------------------------------------------------------------------------------------------------------
JoJo no Kimyouna Bouken / JoJo's Venture             1998   *              CAP-JJK000   CAP-JJK-140   JJK98c00F
JoJo no Kimyouna Bouken  Miraie no Isan              1999   *              CAP-JJM000   CAP-JJM-110   JJM99900F
/ JoJo's Bizarre Adventure Heritage for the Future          *
Street Fighter III New Generation                    1997   970204 JAPAN   CAP-SF3000   CAP-SF3-3     SF397200F
Street Fighter III 2nd Impact Giant Attack           1997   970930 JAPAN   CAP-3GA000   CAP-3GA-1     3GA97a00F
Street Fighter III 3rd Strike Fight for the Future   1999   *              CAP-33S000   CAP-33S-1     33S99400F
Warzard / Red Earth                                  1996   *              CAP-WZD000   CAP-WZD-5     WZD96a00F

* - only two of the games are tested working, so only two S/W Revision dates are known.

The CP SYSTEM III comprises a main board with several custom ASICs, custom 72-pin SIMMs for program
and graphics storage (the same SIMMs are also used in some CPS2 titles), SCSI CDROM and CDROM disc,
and a plug-in security cart containing a boot ROM, an NVRAM and another custom ASIC containing vital
decryption information held by a [suicide] battery.

Not much is known about the actual CPU used in this system due to the extensive use of encryption,
and the volatile nature of the security information, although it is thought to use two Hitachi SH-2
CPUs in a master and slave configuration.

A possible theory about how the cart works is that the flashROM in the cart probably contains a program
which is executed by the custom Capcom IC inside the cart to boot the game. The code in the flashROM is
encrypted, but is likely decrypted using a key probably held in the FM1208S NVRAM. However, if the
custom IC detects any tampering (generally things such as voltage fluctuation or voltage dropping), it
immediately erases the NVRAM (and thus the key) which effectively kills the security cart dead.
It is also possible that the custom Capcom IC may contain some internal code to initiate the boot process
which is battery-backed as well. This is all speculation, of course.

The main board uses the familiar Capcom SIMM modules to hold the data from the CDROM so that the life of
the CD drive is maximized. The SIMMs don't contain RAM, but instead TSOP48 surface mounted flashROMs
that can be updated with different games on bootup using a built-in software updating system.
The SIMMs that hold the program code are located in positions 1 & 2 and are 64MBit.
The SIMMs that hold the graphics are located in positions 3, 4, 5, 6 & 7 and are 128MBit.
The data in the SIMMs is not decrypted, it is merely taken directly from the CDROM and shuffled slightly.
The SIMMs hold the entire contents of the CDROM.

To swap games requires the security cart for the game, it's CDROM disc and the correctly populated type
and number of SIMMs on the main board.
On first power-up after switching the cart and CD, you're presented with a screen asking if you want to
re-program the SIMMs with the new game. Pressing player 1 button 2 cancels it. Pressing player 1 button 1
allows it to proceed whereby you wait about 25 minutes then the game boots up almost immediately. On
subsequent power-ups, the game boots immediately.
If the CDROM is not present in the drive on a normal bootup, a message tells you to insert the CDROM.
Then you press button 1 to continue and the game boots immediately.
Note that not all of the SIMMs are populated on the PCB for each game. Some games have more, some less,
depending on game requirements, so flash times can vary per game. See the table below for details.

                                                     |----------- Required SIMM Locations & Types -----------|
Game                                                 1       2       3        4        5         6         7
--------------------------------------------------------------------------------------------------------------
JoJo's Venture                                       64MBit  64MBit  128MBit  128MBit  32MBit    -         -
JoJo's Bizarre Adventure                             64MBit  64MBit  128MBit  128MBit  128MBit   -         -
Street Fighter III New Generation                    64MBit  -       128MBit  128MBit  32MBit*   -         -
Street Fighter III 2nd Impact Giant Attack           64MBit  64MBit  128MBit  128MBit  128MBit   -         -
Street Fighter III 3rd Strike Fight for the Future   64MBit  64MBit  128MBit  128MBit  128MBit   128MBit   -
Warzard / Red Earth                                  64MBit  -       128MBit  128MBit  32MBit*   -         -

                                                     Notes:
                                                           - denotes not populated
                                                           * 32MBit SIMMs have only 2 FlashROMs populated on them.
                                                             128MBit SIMMs can also be used.
                                                           No game uses a SIMM at 7
                                                           See main board diagram below for SIMM locations.

Due to the built-in upgradability of the hardware, and the higher frame-rates the hardware seems to have,
it appears Capcom had big plans for this system and possibly intended to create many games on it, as they
did with CPS2. Unfortunately for Capcom, CP SYSTEM III was an absolute flop in the arcades so those plans
were cancelled. Possible reasons include...
- The games were essentially just 2D, and already there were many 3D games coming out onto the market that
  interested operators more than this.
- The cost of the system was quite expensive when compared to other games on the market.
- It is rumoured that the system was difficult to program for developers.
- These PCBs were not popular with operators because the security carts are extremely static-sensitive and most
  of them failed due to the decryption information being zapped by simple handling of the PCBs or by touching
  the security cart edge connector underneath the PCB while the security cart was plugged in, or by power
  fluctuations while flashing the SIMMs. You will know if your cart has been zapped because on bootup, you get
  a screen full of garbage coloured pixels instead of the game booting up, or just a black or single-coloured
  screen. You should also not touch the inside of the security cart or open it up because it will immediately
  be zapped when you touch it!


PCB Layouts
-----------

CAPCOM
CP SYSTEMIII
95682A-4 (older rev 95682A-3)
   |----------------------------------------------------------------------|
  |= J1             HM514260     |------------|      |  |  |  |  |        |
   |                             |CAPCOM      |      |  |  |  |  |        |
  |= J2     TA8201  TC5118160    |DL-2729 PPU |      |  |  |  |  |        |
   |                             |(QFP304)    |      |  |  |  |  |        |
|--|          VOL   TC5118160    |            |      |  |  |  |  |        |
|    LM833N                      |            |      S  S  S  S  S        |
|    LM833N         TC5118160    |------------|      I  I  I  I  I        |
|           TDA1306T                      |--------| M  M  M  M  M        |
|                   TC5118160  60MHz      |CAPCOM  | M  M  M  M  M       |-|
|                              42.9545MHz |DL-3329 | 7  6  5  4  3       | |
|           LM385                         |SSU     | |  |  |  |  |       | |
|J                         KM681002       |--------| |  |  |  |  |       | |
|A                         KM681002  62256 |-------| |  |  |  |  |       | |
|M                                         |DL3529 | |  |  |  |  |       | |
|M          MC44200FU                      |GLL2   | |  |  |  |  |       | |
|A                              3.6864MHz  |-------|                  CN6| |
|                                                             |  |       | |
|                               |--------|   |-|              |  |       | |
|                               |CAPCOM  |   | |   |-------|  |  |       | |
|        TD62064                |DL-2929 |   | |   |CAPCOM |  |  |       | |
|                               |IOU     |   | |   |DL-3429|  |  |       | |
|        TD62064                |--------|   | |   |GLL1   |  S  S       | |
|--|                            *HA16103FPJ  | |   |-------|  I  I       |-|
   |                                         | |CN5           M  M        |
   |                                         | |   |-------|  M  M        |
  |-|                        93C46           | |   |CAPCOM |  2  1        |
  | |      PS2501                            | |   |DL-2829|  |  | |-----||
  | |CN1                                     | |   |CCU    |  |  | |AMD  ||
  | |      PS2501                            | |   |-------|  |  | |33C93||
  |-|                                        |-|              |  | |-----||
   |   SW1                                         HM514260   |  |        |
   |----------------------------------------------------------------------|
Notes:
      TA8201     - Toshiba TA8201 18W BTL x 2-Channel Audio Power Amplifier
      PS2501     - NEC PS2501 High Isolation Voltage Single Transistor Type Multi Photocoupler (DIP16)
      TDA1306T   - Philips TDA1306T Noise Shaping Filter DAC (SOIC24)
      MC44200FU  - Motorola MC44200FU Triple 8-bit Video DAC (QFP44)
      LM833N     - ST Microelectronics LM833N Low Noise Audio Dual Op-Amp (DIP8)
      TD62064    - Toshiba TD62064AP NPN 50V 1.5A Quad Darlington Driver (DIP16)
      HA16103FPJ - Hitachi HA16103FPJ Watchdog Timer (SOIC20)
                   *Note this IC is not populated on the rev -4 board
      93C46      - National Semiconductor NM93C46A 128bytes x8 Serial EEPROM (SOIC8)
                   Note this IC is covered by a plastic housing on the PCB. The chip is just a normal
                   (unsecured) EEPROM so why it was covered is not known.
      LM385      - National Semiconductor LM385 Adjustable Micropower Voltage Reference Diode (SOIC8)
      33C93      - AMD 33C93A-16 SCSI Controller (PLCC44)
      KM681002   - Samsung Electronics KM681002 128k x8 SRAM (SOJ32)
      62256      - 8k x8 SRAM (SOJ28)
      HM514260   - Hitachi HM514260CJ7 1M x16 DRAM (SOJ42)
      TC5118160  - Toshiba TC5118160BJ-60 256k x16 DRAM (SOJ42)
      SW1        - Push-button Test Switch
      VOL        - Master Volume Potentiometer
      J1/J2      - Optional RCA Left/Right Audio Out Connectors
      CN1        - 34-Pin Capcom Kick Button Harness Connector
      CN5        - Security Cartridge Slot
      CN6        - 4-Pin Power Connector and 50-pin SCSI Data Cable Connector
                   CDROM Drive is a CR504-KCM 4X SCSI drive manufactured By Panasonic / Matsushita
      SIMM 1-2   - 72-Pin SIMM Connector, holds single sided SIMMs containing 4x Fujitsu 29F016A
                   surface mounted TSOP48 FlashROMs
      SIMM 3-7   - 72-Pin SIMM Connector, holds double sided SIMMs containing 8x Fujitsu 29F016A
                   surface mounted TSOP48 FlashROMs

                   SIMM Layout -
                          |----------------------------------------------------|
                          |                                                    |
                          |   |-------|   |-------|   |-------|   |-------|    |
                          |   |Flash_A|   |Flash_B|   |Flash_C|   |Flash_D|    |
                          |   |-------|   |-------|   |-------|   |-------|    |
                          |-                                                   |
                           |-------------------------/\------------------------|
                           Notes:
                                  For SIMMs 1-2, Flash_A & Flash_C and regular pinout (Fujitsu 29F016A-90PFTN)
                                  Flash_B & Flash_D are reverse pinout (Fujitsu 29F016A-90PFTR)
                                  and are mounted upside down also so that pin1 lines up with
                                  the normal pinout of FlashROMs A & C.
                                  For SIMMs 3-7, the 8 FlashROMs are populated on both sides using a similar layout.

      Capcom Custom ASICs -
                           DL-2729 PPU SD10-505   (QFP304)
                           DL-2829 CCU SD07-1514  (QFP208)
                           DL-2929 IOU SD08-1513  (I/O controller, QFP208)
                           DL-3329 SSU SD04-1536  (QFP144)
                           DL-3429 GLL1 SD06-1537 (QFP144)
                           DL-3529 GLL2 SD11-1755 (QFP80)


Connector Pinouts
-----------------

                       JAMMA Connector                                       Extra Button Connector
                       ---------------                                       ----------------------
                    PART SIDE    SOLDER SIDE                                       TOP    BOTTOM
                ----------------------------                               --------------------------
                      GND  01    A  GND                                        GND  01    02  GND
                      GND  02    B  GND                                        +5V  03    04  +5V
                      +5V  03    C  +5V                                       +12V  05    06  +12V
                      +5V  04    D  +5V                                             07    08
                       NC  05    E  NC                           Player 2 Button 4  09    10
                     +12V  06    F  +12V                                            11    12
                           07    H                                                  13    14
           Coin Counter 1  08    J  NC                           Player 1 Button 4  15    16
             Coin Lockout  09    K  Coin Lockout                 Player 1 Button 5  17    18
               Speaker (+) 10    L  Speaker (-)                  Player 1 Button 6  19    20
                       NC  11    M  NC                           Player 2 Button 5  21    22
                Video Red  12    N  Video Green                  Player 2 Button 6  23    24
               Video Blue  13    P  Video Composite Sync                            25    26
             Video Ground  14    R  Service Switch                                  27    28
                     Test  15    S  NC                                 Volume Down  29    30  Volume UP
                   Coin A  16    T  Coin B                                     GND  31    32  GND
           Player 1 Start  17    U  Player 2 Start                             GND  33    34  GND
              Player 1 Up  18    V  Player 2 Up
            Player 1 Down  19    W  Player 2 Down
            Player 1 Left  20    X  Player 2 Left
           Player 1 Right  21    Y  Player 2 Right
        Player 1 Button 1  22    Z  Player 2 Button 1
        Player 1 Button 2  23    a  Player 2 Button 2
        Player 1 Button 3  24    b  Player 2 Button 3
                       NC  25    c  NC
                       NC  26    d  NC
                      GND  27    e  GND
                      GND  28    f  GND


Security Cartridge PCB Layout
-----------------------------

CAPCOM 95682B-3 TORNADO
|------------------------------------------------|
|      BATTERY                                   |
|                          |-------|             |
|                          |CAPCOM |   29F400    |
|                          |DL-3229|   *28F400   |
|                          |SCU    |     *FM1208S|
| 74HC00                   |-------|             |
|               6.25MHz                    74F00 |
|---|     |-|                             |------|
    |     | |                             |
    |-----| |-----------------------------|
Notes:
      74F00        - 74F00 Quad 2-Input NAND Gate (SOIC14)
      74HC00       - Philips 74HC00N Quad 2-Input NAND Gate (DIP14)
      29F400       - Fujitsu 29F400TA-90PFTN 512k x8 FlashROM (TSOP48)
      Custom ASIC  - CAPCOM DL-3229 SCU (QFP144)
      FM1208S      - RAMTRON FM1208S 4k (512bytes x8) Nonvolatile Ferroelectric RAM (SOIC24)
      28F400       - 28F400 SOP44 FlashROM (not populated)
      *            - These components located on the other side of the PCB

*/

/* load extracted cd content? */
#define LOAD_CD_CONTENT 1

#include "driver.h"

VIDEO_START(cps3)
{
	return 0;
}

VIDEO_UPDATE(cps3)
{

	return 0;
}

static ADDRESS_MAP_START( cps3_map, ADDRESS_SPACE_PROGRAM, 32 )
ADDRESS_MAP_END

INPUT_PORTS_START( cps3 )
INPUT_PORTS_END

static MACHINE_DRIVER_START( cps3 )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", SH2, 20000000) // ??
	MDRV_CPU_PROGRAM_MAP(cps3_map,0)

	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(64*8, 64*8)
	MDRV_SCREEN_VISIBLE_AREA(0, 64*8-1, 0, 64*8-1)
	MDRV_PALETTE_LENGTH(0x1000)

	MDRV_VIDEO_START(cps3)
	MDRV_VIDEO_UPDATE(cps3)
MACHINE_DRIVER_END

static DRIVER_INIT (nocpu)
{
	cpunum_set_input_line(0, INPUT_LINE_RESET, ASSERT_LINE);
}


ROM_START( sfiii )
	ROM_REGION( 0x080000, REGION_USER1, 0 ) /* bios region */
	ROM_LOAD( "29f400.u2", 0x000000, 0x080000, CRC(74205250) SHA1(c3e83ace7121d32da729162662ec6b5285a31211) )

	#ifdef LOAD_CD_CONTENT
	/* Note: These regions contains the rom data extracted from the cd.
             This is done only to make analysis easier. Once correct
             emulation is possible this region will be removed and the
             roms will be loaded into flashram from the CD by the system */
	ROM_REGION( 0x800000, REGION_USER4, 0 ) /* cd content region */
	ROM_LOAD( "10",  0x0000000, 0x800000, CRC(e896dc27) SHA1(47623820c64b72e69417afcafaacdd2c318cde1c) )
	ROM_REGION( 0x2400000, REGION_USER5, 0 ) /* cd content region */
	ROM_LOAD( "30",  0x0000000, 0x800000, CRC(98c2d07c) SHA1(604ce4a16170847c10bc233a47a47a119ce170f7) )
	ROM_LOAD( "31",  0x0800000, 0x800000, CRC(7115a396) SHA1(b60a74259e3c223138e66e68a3f6457694a0c639) )
	ROM_LOAD( "40",  0x1000000, 0x800000, CRC(839f0972) SHA1(844e43fcc157b7c774044408bfe918c49e174cdb) )
	ROM_LOAD( "41",  0x1800000, 0x800000, CRC(8a8b252c) SHA1(9ead4028a212c689d7a25746fbd656dca6f938e8) )
	ROM_LOAD( "50",  0x2000000, 0x400000, CRC(58933dc2) SHA1(1f1723be676a817237e96b6e20263b935c59daae) )
	/* 90,91,92,93 are bmp images, not extracted */
	#endif

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "sf3000", 0, MD5(cdc5c5423bd8c053de7cdd927dc60da7) SHA1(cc72c9eb2096f4d51f2cf6df18f29fd79d05067c) )
ROM_END

ROM_START( sfiii2 )
	ROM_REGION( 0x080000, REGION_USER1, 0 ) /* bios region */
	ROM_LOAD( "29f400.u2", 0x000000, 0x080000, CRC(faea0a3e) SHA1(a03cd63bcf52e4d57f7a598c8bc8e243694624ec) )

	#ifdef LOAD_CD_CONTENT
	/* Note: These regions contains the rom data extracted from the cd.
             This is done only to make analysis easier. Once correct
             emulation is possible this region will be removed and the
             roms will be loaded into flashram from the CD by the system */
	ROM_REGION( 0x1000000, REGION_USER4, 0 ) /* cd content region */
	ROM_LOAD( "10",  0x0000000, 0x800000, CRC(682b014a) SHA1(abd5785f4b7c89584d6d1cf6fb61a77d7224f81f) )
	ROM_LOAD( "20",  0x0800000, 0x800000, CRC(38090460) SHA1(aaade89b8ccdc9154f97442ca35703ec538fe8be) )
	ROM_REGION( 0x3000000, REGION_USER5, 0 ) /* cd content region */
	ROM_LOAD( "30",  0x0000000, 0x800000, CRC(77c197c0) SHA1(944381161462e65de7ae63a656658f3fbe44727a) )
	ROM_LOAD( "31",  0x0800000, 0x800000, CRC(7470a6f2) SHA1(850b2e20afe8a5a1f0d212d9abe002cb5cf14d22) )
	ROM_LOAD( "40",  0x1000000, 0x800000, CRC(01a85ced) SHA1(802df3274d5f767636b2785606e0558f6d3b9f13) )
	ROM_LOAD( "41",  0x1800000, 0x800000, CRC(fb346d74) SHA1(57570f101a170aa7e83e84e4b7cbdc63a11a486a) )
	ROM_LOAD( "50",  0x2000000, 0x800000, CRC(32f79449) SHA1(44b1f2a640ab4abc23ff47e0edd87fbd0b345c06) )
	ROM_LOAD( "51",  0x2800000, 0x800000, CRC(1102b8eb) SHA1(c7dd2ee3a3214c6ec47a03fe3e8c941775d57f76) )
	/* 90,91,92,93 are bmp images, not extracted */
	#endif

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "3ga000", 0, MD5(941c7e8d0838db9880ea7bf169ad310d) SHA1(76e9fdef020c4b85a10aa8828a63e67c7dca22bd) )
ROM_END

ROM_START( sfiii3 )
	ROM_REGION( 0x080000, REGION_USER1, 0 ) /* bios region */
	/* why was this loading the sfiii.zip bios?? mistake? or does it work with both? */
//  ROM_LOAD( "29f400.u2", 0x000000, 0x080000, CRC(74205250) SHA1(c3e83ace7121d32da729162662ec6b5285a31211) ) // uuh, this is the sfiii bios...
	/* this one is from a usa version of sf3 3rd strike */
	ROM_LOAD( "sf33usa.bin", 0x000000, 0x080000, CRC(ecc545c1) SHA1(e39083820aae914fd8b80c9765129bedb745ceba) )

	#ifdef LOAD_CD_CONTENT
	/* Note: These regions contains the rom data extracted from the cd.
             This is done only to make analysis easier. Once correct
             emulation is possible this region will be removed and the
             roms will be loaded into flashram from the CD by the system */
	ROM_REGION( 0x1000000, REGION_USER4, 0 ) /* cd content region */
	ROM_LOAD( "10",  0x0000000, 0x800000, CRC(77233d39) SHA1(59c3f890fdc33a7d8dc91e5f9c4e7b7019acfb00) )
	ROM_LOAD( "20",  0x0800000, 0x800000, CRC(5ca8faba) SHA1(71c12638ae7fa38b362d68c3ccb4bb3ccd67f0e9) )
	ROM_REGION( 0x4000000, REGION_USER5, 0 ) /* cd content region */
	ROM_LOAD( "30",  0x0000000, 0x800000, CRC(b37cf960) SHA1(60310f95e4ecedee85846c08ccba71e286cda73b) )
	ROM_LOAD( "31",  0x0800000, 0x800000, CRC(450ec982) SHA1(1cb3626b8479997c4f1b29c41c81cac038fac31b) )
	ROM_LOAD( "40",  0x1000000, 0x800000, CRC(632c965f) SHA1(9a46b759f5dee35411fd6446c2457c084a6dfcd8) )
	ROM_LOAD( "41",  0x1800000, 0x800000, CRC(7a4c5f33) SHA1(f33cdfe247c7caf9d3d394499712f72ca930705e) )
	ROM_LOAD( "50",  0x2000000, 0x800000, CRC(8562358e) SHA1(8ed78f6b106659a3e4d94f38f8a354efcbdf3aa7) )
	ROM_LOAD( "51",  0x2800000, 0x800000, CRC(7baf234b) SHA1(38feb45d6315d771de5f9ae965119cb25bae2a1e) )
	ROM_LOAD( "60",  0x3000000, 0x800000, CRC(bc9487b7) SHA1(bc2cd2d3551cc20aa231bba425ff721570735eba) )
	ROM_LOAD( "61",  0x3800000, 0x800000, CRC(b813a1b1) SHA1(16de0ee3dfd6bf33d07b0ff2e797ebe2cfe6589e) )
	/* 92,93 are bmp images, not extracted */
	#endif

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "33s000", 0, MD5(f159ad85cc94ced3ddb9ef5e92173a9f) SHA1(47c7ae0f2dc47c7d28bdf66d378a3aaba4c99c75) )
ROM_END

ROM_START( warzard )
	ROM_REGION( 0x080000, REGION_USER1, 0 ) /* bios region */
	ROM_LOAD( "29f400.u2", 0x000000, 0x080000, CRC(f8e2f0c6) SHA1(93d6a986f44c211fff014e55681eca4d2a2774d6) )

	#ifdef LOAD_CD_CONTENT
	/* Note: These regions contains the rom data extracted from the cd.
             This is done only to make analysis easier. Once correct
             emulation is possible this region will be removed and the
             roms will be loaded into flashram from the CD by the system */
	ROM_REGION( 0x800000, REGION_USER4, 0 ) /* cd content region */
	ROM_LOAD( "10",  0x000000, 0x800000, CRC(68188016) SHA1(93aaac08cb5566c33aabc16457085b0a36048019) )
	ROM_REGION( 0x2400000, REGION_USER5, 0 ) /* cd content region */
	ROM_LOAD( "30",  0x0000000, 0x800000, CRC(074cab4d) SHA1(4cb6cc9cce3b1a932b07058a5d723b3effa23fcf) )
	ROM_LOAD( "31",  0x0800000, 0x800000, CRC(14e2cad4) SHA1(9958a4e315e4476e4791a6219b93495413c7b751) )
	ROM_LOAD( "40",  0x1000000, 0x800000, CRC(72d98890) SHA1(f40926c52cb7a71b0ef0027a0ea38bbc7e8b31b0) )
	ROM_LOAD( "41",  0x1800000, 0x800000, CRC(88ccb33c) SHA1(1e7af35d186d0b4e45b6c27458ddb9cfddd7c9bc) )
	ROM_LOAD( "50",  0x2000000, 0x400000, CRC(2f5b44bd) SHA1(7ffdbed5b6899b7e31414a0828e04543d07435e4) )
	/* 90,91,92,93 are bmp images, not extracted */
	#endif

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "wzd000", 0, MD5(028ff12a4ce34118dd0091e87c8cdd08) SHA1(6d4e6b7fff4ff3f04e349479fa5a1cbe63e673b8) )
ROM_END

ROM_START( jojo )
	ROM_REGION( 0x080000, REGION_USER1, 0 ) /* bios region */
	ROM_LOAD( "29f400.u2", 0x000000, 0x080000, CRC(02778f60) SHA1(a167f9ebe030592a0cdb0c6a3c75835c6a43be4c) )

	#ifdef LOAD_CD_CONTENT
	/* Note: These regions contains the rom data extracted from the cd.
             This is done only to make analysis easier. Once correct
             emulation is possible this region will be removed and the
             roms will be loaded into flashram from the CD by the system */
	ROM_REGION( 0x1000000, REGION_USER4, 0 ) /* cd content region */
	ROM_LOAD( "10",  0x0000000, 0x800000, CRC(e40dc123) SHA1(517e7006349b5a8fd6c30910362583f48d009355) )
	ROM_LOAD( "20",  0x0800000, 0x800000, CRC(0571e37c) SHA1(1aa28ef6ea1b606a55d0766480b3ee156f0bca5a) )
	ROM_REGION( 0x2400000, REGION_USER5, 0 ) /* cd content region */
	ROM_LOAD( "30",  0x0000000, 0x800000, CRC(1d99181b) SHA1(25c216de16cefac2d5892039ad23d07848a457e6) )
	ROM_LOAD( "31",  0x0800000, 0x800000, CRC(6889fbda) SHA1(53a51b993d319d81a604cdf70b224955eacb617e) )
	ROM_LOAD( "40",  0x1000000, 0x800000, CRC(8069f9de) SHA1(7862ee104a2e9034910dd592687b40ebe75fa9ce) )
	ROM_LOAD( "41",  0x1800000, 0x800000, CRC(9c426823) SHA1(1839dccc7943a44063e8cb2376cd566b24e8b797) )
	ROM_LOAD( "50",  0x2000000, 0x400000, CRC(1c749cc7) SHA1(23df741360476d8035c68247e645278fbab53b59) )
	/* 92,93 are bmp images, not extracted */
	#endif

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "jjk000", 0, MD5(05440ecf90e836207a27a99c817a3328) SHA1(d5a11315ac21e573ffe78e63602ec2cb420f361f) )
ROM_END


ROM_START( jojoba )
	ROM_REGION( 0x080000, REGION_USER1, 0 ) /* bios region */
	// this was from a version which doesn't require the cd to run
	ROM_LOAD( "29f400.u2", 0x000000, 0x080000, CRC(4dab19f5) SHA1(ba07190e7662937fc267f07285c51e99a45c061e) )

	#ifdef LOAD_CD_CONTENT
	/* Note: These regions contains the rom data extracted from the cd.
             This is done only to make analysis easier. Once correct
             emulation is possible this region will be removed and the
             roms will be loaded into flashram from the CD by the system */
	ROM_REGION( 0x1000000, REGION_USER4, 0 ) /* cd content region */
	ROM_LOAD( "10",  0x000000, 0x800000, CRC(6e2490f6) SHA1(75cbf1e39ad6362a21c937c827e492d927b7cf39) )
	ROM_LOAD( "20",  0x800000, 0x800000, CRC(1293892b) SHA1(b1beafac1a9c4b6d0640658af8a3eb359e76eb25) )
	ROM_REGION( 0x3000000, REGION_USER5, 0 ) /* cd content region */
	ROM_LOAD( "30",  0x0000000, 0x800000, CRC(d25c5005) SHA1(93a19a14783d604bb42feffbe23eb370d11281e8) )
	ROM_LOAD( "31",  0x0800000, 0x800000, CRC(51bb3dba) SHA1(39e95a05882909820b3efa6a3b457b8574012638) )
	ROM_LOAD( "40",  0x1000000, 0x800000, CRC(94dc26d4) SHA1(5ae2815142972f322886eea4885baf2b82563ab1) )
	ROM_LOAD( "41",  0x1800000, 0x800000, CRC(1c53ee62) SHA1(e096bf3cb6fbc3d45955787b8f3213abcd76d120) )
	ROM_LOAD( "50",  0x2000000, 0x800000, CRC(36e416ed) SHA1(58d0e95cc13f39bc171165468ce72f4f17b8d8d6) )
	ROM_LOAD( "51",  0x2800000, 0x800000, CRC(eedf19ca) SHA1(a7660bf9ff87911afb4f83b64456245059986830) )
	/* 92,93 are bmp images, not extracted */
	#endif

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "jjm000", 0, MD5(bf6b90334bf1f6bd8dbfed737face2d6) SHA1(688520bb83ccbf4b31c3bfe26bd0cc8292a8c558) )
ROM_END

ROM_START( jojobaa )
	ROM_REGION( 0x080000, REGION_USER1, 0 ) /* bios region */
	ROM_LOAD( "jojobacart.bin",  0x000000, 0x080000,  CRC(3085478c) SHA1(055eab1fc42816f370a44b17fd7e87ffcb10e8b7) )

	#ifdef LOAD_CD_CONTENT
	/* Note: These regions contains the rom data extracted from the cd.
             This is done only to make analysis easier. Once correct
             emulation is possible this region will be removed and the
             roms will be loaded into flashram from the CD by the system */
	ROM_REGION( 0x1000000, REGION_USER4, 0 ) /* cd content region */
	ROM_LOAD( "10",  0x000000, 0x800000, CRC(6e2490f6) SHA1(75cbf1e39ad6362a21c937c827e492d927b7cf39) )
	ROM_LOAD( "20",  0x800000, 0x800000, CRC(1293892b) SHA1(b1beafac1a9c4b6d0640658af8a3eb359e76eb25) )
	ROM_REGION( 0x3000000, REGION_USER5, 0 ) /* cd content region */
	ROM_LOAD( "30",  0x0000000, 0x800000, CRC(d25c5005) SHA1(93a19a14783d604bb42feffbe23eb370d11281e8) )
	ROM_LOAD( "31",  0x0800000, 0x800000, CRC(51bb3dba) SHA1(39e95a05882909820b3efa6a3b457b8574012638) )
	ROM_LOAD( "40",  0x1000000, 0x800000, CRC(94dc26d4) SHA1(5ae2815142972f322886eea4885baf2b82563ab1) )
	ROM_LOAD( "41",  0x1800000, 0x800000, CRC(1c53ee62) SHA1(e096bf3cb6fbc3d45955787b8f3213abcd76d120) )
	ROM_LOAD( "50",  0x2000000, 0x800000, CRC(36e416ed) SHA1(58d0e95cc13f39bc171165468ce72f4f17b8d8d6) )
	ROM_LOAD( "51",  0x2800000, 0x800000, CRC(eedf19ca) SHA1(a7660bf9ff87911afb4f83b64456245059986830) )
	/* 92,93 are bmp images, not extracted */
	#endif

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "jjm000", 0, MD5(bf6b90334bf1f6bd8dbfed737face2d6) SHA1(688520bb83ccbf4b31c3bfe26bd0cc8292a8c558) )
ROM_END




GAME( 1997, sfiii,   0,        cps3, cps3, nocpu, ROT0,   "Capcom", "Street Fighter III: New Generation", GAME_NOT_WORKING | GAME_NO_SOUND )
GAME( 1998, sfiii2,  0,        cps3, cps3, nocpu, ROT0,   "Capcom", "Street Fighter III 2nd Impact: Giant Attack", GAME_NOT_WORKING | GAME_NO_SOUND )
GAME( 1999, sfiii3,  0,        cps3, cps3, nocpu, ROT0,   "Capcom", "Street Fighter III 3rd Strike: Fight for the Future", GAME_NOT_WORKING | GAME_NO_SOUND )
GAME( 1996, warzard, 0,        cps3, cps3, nocpu, ROT0,   "Capcom", "Warzard / Red Earth", GAME_NOT_WORKING | GAME_NO_SOUND )
GAME( 1998, jojo,    0,        cps3, cps3, nocpu, ROT0,   "Capcom", "JoJo's Venture / JoJo no Kimyouna Bouken", GAME_NOT_WORKING | GAME_NO_SOUND )
GAME( 1999, jojoba,  0,        cps3, cps3, nocpu, ROT0,   "Capcom", "JoJo's Bizarre Adventure: Heritage for the Future / JoJo no Kimyouna Bouken: Miraie no Isan", GAME_NOT_WORKING | GAME_NO_SOUND )
GAME( 1999, jojobaa, jojoba,   cps3, cps3, nocpu, ROT0,   "Capcom", "JoJo's Bizarre Adventure: Heritage for the Future / JoJo no Kimyouna Bouken: Miraie no Isan (alt)", GAME_NOT_WORKING | GAME_NO_SOUND )

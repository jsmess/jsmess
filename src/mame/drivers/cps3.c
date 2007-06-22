/*

CPS3 Driver (preliminary)

Decryption by Andreas Naive

Driver by David Haywood
 with help from Tomasz Slanina and ElSemi

Sound emulation by Philip Bennett



Emulation Notes:

  sets are region hacked for now, to make the test modes readable, don't take the regions
  you see on bootup as being correct.

ToDo: (in order or priority?)

Convert to custom rendering.  There are just too many colours to use MAME's palettes, and the ram based tiles
make tilemap management more complex than it needs to be.  Currently we're having to overwrite palettes to
display the text layer.

SFIII2 seems to crash sometimes, it's probably reading past the end of memory somewhere

Fix sprite attributes, palette select (seems to be per sprite and per cell), y flip, zoom

Fix backgrounds - palettes, zooming, linescroll, see 'write custom rendering'

Emulate CD-Controller (not too important right now, games can be run as no-cd)  Note, due to the
flash roms region being split over 4 interleaved regions this will require a bit of code to copy
anything written to the flash roms into an area we can map opbase to.

update to use Nicola's newer decryption function

---

Capcom CP SYSTEM III Hardware Overview
Capcom, 1996-1999

From late 1996 to 1999 Capcom developed another hardware platform to rival the CPS2 System and called
it CP SYSTEM III. Only 6 games were produced....

Game                                                 Date   S/W Rev        CD Part#     Label         Cart Part#
----------------------------------------------------------------------------------------------------------------
JoJo no Kimyouna Bouken / JoJo's Venture             1998   98/12/02       CAP-JJK000   CAP-JJK-140   JJK98c00F
                                                            99/01/08                    CAP-JJK-160
                                                            99/01/28 **
JoJo no Kimyouna Bouken  Miraie no Isan
/ JoJo's Bizarre Adventure Heritage for the Future   1999   99/09/13       CAP-JJM000   CAP-JJM-110   JJM99900F
                                                            99/09/27 **

Street Fighter III New Generation                    1997   97/02/04       CAP-SF3000   CAP-SF3-3     SF397200F

Street Fighter III 2nd Impact Giant Attack           1997   97/09/30       CAP-3GA000   CAP-3GA-1     3GA97a00F

Street Fighter III 3rd Strike Fight for the Future   1999   99/05/12       CAP-33S000   CAP-33S-1     33S99400F
                                                            99/06/08                    CAP-33S-2

Warzard / Red Earth                                  1996   96/11/21       CAP-WZD000   CAP-WZD-5     WZD96a00F

** NOT DUMPED but known to exist

The Game Region / No CD Flags / Development flags etc. are controlled by a byte in the bios roms.  The CDs
contain revisions of the game code and are independant of the region.

The CP SYSTEM III comprises a main board with several custom ASICs, custom 72-pin SIMMs for program
and graphics storage (the same SIMMs are also used in some CPS2 titles), SCSI CDROM and CDROM disc,
and a plug-in security cart containing a boot ROM, an NVRAM and another custom ASIC containing vital
decryption information held by a [suicide] battery.

Not much is known about the actual CPU used in this system due to the extensive use of encryption,
and the volatile nature of the security information. There appears to be a custom Hitachi SH-2
CPU on the mainboard and there has been confirmed to be one in the cart. Tests were done by decrypting
the BIOS and code and running it on the PCB. It is known that neither of these CPU's will run standard
(i.e. unencrypted) SH2 code.

The security cart is thought to work like this....  the flashROM in the cart contains a program BIOS which is
decrypted by the CPU in the cart (the CPU has built-in decryption) then executed by that CPU to boot the BIOS
code.  Even though the code in the flashROM is encrypted, the cart can run it even if it is dead/suicided because
it has been discovered that the BIOS contains a hidden security menu allowing the cart to be loaded with the
security data. This proves the cart runs the BIOS even if it is dead. The special security menu is not
normally available but is likely accessed with a special key/button combination which is unknown ATM.
The cart contains a FM1208S NVRAM which appears to either be unused or holds game settings. Because the CPU
in the cart is always powered by a battery, it has stealth capability that allows it to continually monitor
the situation. If the custom CPU detects any tampering (generally things such as voltage fluctuation or
voltage dropping or even removal of the cart with the power on), it immediately erases the SRAM inside the
CPU (and thus the key) which effectively kills the security cart dead. This also suggests that the custom
Capcom CPU contains some additional internal code to initiate the boot process which is battery-backed
as well. It is known (from decapping it) that the CPU in the security cart does contain an amount of static
RAM for data storage and a SH2 core.

The main board uses the familiar Capcom SIMM modules to hold the data from the CDROM so that the life of
the CD drive is maximized. The SIMMs don't contain RAM, but instead TSOP48 surface mounted flashROMs
that can be updated with different games on bootup using a built-in software updating system.
The SIMMs that hold the program code are located in positions 1 & 2 and are 64MBit.
The SIMMs that hold the graphics are located in positions 3, 4, 5, 6 & 7 and are 128MBit.
The data in the SIMMs is not decrypted, it is merely taken directly from the CDROM and shuffled slightly
then programmed to the flashROMs. The SIMMs hold the entire contents of the CDROM.

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
  screen. You should also not touch the inside of the security cart because it will be immediately zapped
  when you touch it! The PCB can detect the presence of the security cart and if it is removed on a working game,
  the game will freeze immediately and it will also erase the security cart battery-backed data.


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
      TDA1306T   - Philips TDA1306T Noise Shaping Filter DAC (SOIC24). The clock (on pin 12) measures
                   14.3181667MHz (42.9545/3)
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
                           DL-2729 PPU SD10-505   (QFP304). Decapping reveals this is the main graphics chip.
                           DL-2829 CCU SD07-1514  (QFP208). Decapping reveals this to be a custom Toshiba ASIC.
                           DL-2929 IOU SD08-1513  (QFP208). This is the I/O controller.
                           DL-3329 SSU SD04-1536  (QFP144). This is might be the main CPU. It appears to be a SH2
                                                            variant with built-in encryption. It is clocked at
                                                            21.47725MHz (42.9545/2)
                           DL-3429 GLL1 SD06-1537 (QFP144). Unknown, possibly a DMA or bus controller.
                           DL-3529 GLL2 SD11-1755 (QFP80).  This might be the sound chip (it has 32k SRAM connected to it).


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

CAPCOM 95682B-3 TORNADE
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
      Custom ASIC  - CAPCOM DL-3229 SCU (QFP144). Decapping reveals this is a Hitachi HD6417099 SH2 variant
                     with built-in encryption, clocked at 6.250MHz
      FM1208S      - RAMTRON FM1208S 4k (512bytes x8) Nonvolatile Ferroelectric RAM (SOIC24)
      28F400       - 28F400 SOP44 FlashROM (not populated)
      *            - These components located on the other side of the PCB
      The battery powers the CPU only. A small board containing some transistors is wired to the 74HC00
      to switch the CPU from battery power to main power to increase the life of the battery.

*/

#include "driver.h"
#include "machine/intelfsh.h"
#include "cpu/sh2/sh2.h"
#include "sound/custom.h"
#include "cps3.h"
/* load extracted cd content? */
#define LOAD_CD_CONTENT 1
#define DEBUG_PRINTF 0

UINT32* decrypted_bios;
UINT32* decrypted_gamerom;
UINT32 cram_gfxflash_bank;
UINT32* cps3_newtile_ram;

UINT32*tilemap20_regs_base;
UINT32*tilemap30_regs_base;
UINT32*tilemap40_regs_base;
UINT32*tilemap50_regs_base;

UINT32* cps3_0xc0000000_ram;
UINT32* cps3_0xc0000000_ram_decrypted;

UINT32* cps3_char_ram;

UINT32* cps3_spriteram;
UINT32* cps3_eeprom;
UINT32* cps3_dstram;

UINT32 cps3_ss_pal_base = 0;
UINT32* cps3_colourram;
UINT32 cps3_unk_vidregs[0x20/4];

static offs_t cps3_opbase_handler(offs_t address);

/* Encryption */

static UINT32 cps3_key1, cps3_key2, cps3_key3, cps3_key4, cps3_key5, cps3_isSpecial;

static UINT16 rotate_left(UINT16 value, int n)
{
   int aux = value>>(16-n);
   return ((value<<n)|aux)%0x10000;
}

static UINT16 rotxor(UINT16 val, UINT16 xor)
{
	UINT16 res;
	res = val + rotate_left(val,2);
	res = rotate_left(res,4) ^ (res & xor);
	return res;
}

static UINT32 cps3_mask(UINT32 address, UINT16 key1, UINT16 key2, UINT16 key3, UINT16 key4, UINT16 key5 )
{
   UINT16 aux, aux2;

   aux2 = address ^ key1;

   aux = (aux2 + rotate_left(aux2,2));

   aux = rotate_left(aux,4)^(aux & (address^key2));

   aux ^= key3;
   aux ^= (address>>16);

   aux2 = aux ^ key4;
   aux2 = rotate_left(aux2, 2) + aux2;

   aux = rotate_left(aux2,4) ^ (aux2 & aux);

   aux ^= address;
   aux ^= key5;

   return aux|(aux<<16);
}
struct game_keys2
{
	const char *name;             /* game driver name */
	const UINT16 keys[5];
	int isSpecial;
};

static const struct game_keys2 keys_table2[] =
{
	// name         key1       key2
	{ "jojo",     { 0xc11c,0xd86e, 0xfcef, 0x0130, 0x2791 },0 },
	{ "jojoalt",  { 0xc11c,0xd86e, 0xfcef, 0x0130, 0x2791 },0 },
	{ "jojoba",   { 0xc11c,0xd86e, 0xdfcf, 0x0302, 0x2791 },0 },
	{ "jojoba",   { 0xc11c,0xd86e, 0xdfcf, 0x0302, 0x2791 },0 },
	{ "sfiii",    { 0xfac1,0x689b, 0xb602, 0xfc03, 0x9764 },0 },
	{ "sfiii2",   { 0xffff,0xffff, 0xffff, 0x0000, 0x0000 },1 },
	{ "sfiii3",   { 0xcd4b,0x54ca, 0x56b9, 0x0c12, 0xab35 },0 },
	{ "sfiii3a",  { 0xcd4b,0x54ca, 0x56b9, 0x0c12, 0xab35 },0 },
	{ "warzard",  { 0xf54e,0x4d62, 0xc0ba, 0xa175, 0xb29d },0 },
	{ 0 }	// end of table
};

void cps3_decrypt_bios(void)
{
	int i;
	UINT32 *coderegion = (UINT32*)memory_region(REGION_USER1);

	decrypted_bios = (UINT32*)memory_region(REGION_USER1);

	for (i=0;i<0x80000;i+=4)
	{
		UINT32 dword = coderegion[i/4];
		UINT32 xormask = cps3_mask(i, cps3_key1, cps3_key2, cps3_key3, cps3_key4, cps3_key5);

		/* a bit of a hack, don't decrypt the FLASH commands which are transfered by SH2 DMA */
		if (((i<0x1ff00) || (i>0x1ff6b)) && (i<0x20000) )
		{
			decrypted_bios[i/4] = dword ^ xormask;
		}
		else
		{
			decrypted_bios[i/4] = dword;
		}
	}

	/* Dump to file */
	{
		FILE *fp;
		const char *gamename = Machine->gamedrv->name;
		char filename[256];
		sprintf(filename, "%s_bios.dump", gamename);

		fp=fopen(filename, "w+b");
		if (fp)
		{
			fwrite(decrypted_bios, 0x080000, 1, fp);
			fclose(fp);
		}
	}
}

void cps3_decrypt_gamerom(void)
{
	int i;
	UINT32 *coderegion = (UINT32*)memory_region(REGION_USER4);

	decrypted_gamerom = auto_malloc(0x1000000);

	for (i=0;i<0x1000000;i+=4)
	{
		UINT32 dword = coderegion[i/4];
		UINT32 xormask = cps3_mask(i+0x6000000, cps3_key1, cps3_key2, cps3_key3, cps3_key4, cps3_key5);
		decrypted_gamerom[i/4] = dword ^ xormask;
	}

	/* Dump to file */
	{
		FILE *fp;
		const char *gamename = Machine->gamedrv->name;
		char filename[256];

		sprintf(filename, "%s_gamerom.dump", gamename);

		fp=fopen(filename, "w+b");
		if (fp)
		{
			fwrite(decrypted_gamerom, 0x1000000, 1, fp);
			fclose(fp);
		}
	}
}


DRIVER_INIT( cps3crpt )
{
	const char *gamename = machine->gamedrv->name;
	const struct game_keys2 *k = &keys_table2[0];
	int i;

	cps3_key1 = -1;
	cps3_key2 = -1;
	cps3_key3 = -1;
	cps3_key4 = -1;
	cps3_key5 = -1;
	cps3_isSpecial = -1;

	while (k->name)
	{
		if (strcmp(k->name, gamename) == 0)
		{
			// we have a proper key set the global variables to it (so that we can decrypt code in ram etc.)
			cps3_key1 = k->keys[0];
			cps3_key2 = k->keys[1];
			cps3_key3 = k->keys[2];
			cps3_key4 = k->keys[3];
			cps3_key5 = k->keys[4];
			cps3_isSpecial = k->isSpecial;
			break;
		}
		++k;
	}

	cps3_decrypt_bios();
	cps3_decrypt_gamerom();

	cps3_0xc0000000_ram_decrypted = auto_malloc(0x400);
	memory_set_opbase_handler(0, cps3_opbase_handler);

	// flash roms

	for (i=0;i<48;i++)
		intelflash_init( i, FLASH_FUJITSU_29F016A, NULL );

	cps3_eeprom = auto_malloc(0x400);



}


static const gfx_layout cps3_tiles16x16_layout =
{
	16,16,
	0x8000,
	6,
	{ 2,3,4,5,6,7 },
	{ 3*8,2*8,1*8,0*8,7*8,6*8,5*8,4*8,
	  11*8,10*8,9*8,8*8,15*8,14*8,13*8,12*8 },
	{ 0*128,1*128,2*128,3*128,4*128,5*128,6*128,7*128,8*128,9*128,10*128,11*128,12*128,13*128,14*128,15*128},
	8*256
};



static const gfx_layout cps3_tiles8x8_layout =
{
	8,8,
	0x400,
	4,
	{ /*8,9,10,11,*/ 0,1,2,3 },
	{ 20,16,4,0,52,48,36,32 },

	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64 },
	64*8
};

UINT32* cps3_ss_ram;
UINT8* cps3_ss_ram_dirty;
int cps3_ss_ram_is_dirty;

UINT8* cps3_char_ram_dirty;
int cps3_char_ram_is_dirty;


void decode_ssram(void)
{
	if (cps3_ss_ram_dirty)
	{
		int i;
		for (i=0;i<0x400;i++)
		{
			if (cps3_ss_ram_dirty[i])
			{
				decodechar(Machine->gfx[0], i, (UINT8*)cps3_ss_ram, &cps3_tiles8x8_layout);
				cps3_ss_ram_dirty[i] = 0;
			}
		}

		cps3_ss_ram_is_dirty = 0;
	}

	return;
}

void decode_charram(void)
{
	if (cps3_char_ram_is_dirty)
	{
		int i;
		for (i=0;i<0x8000;i++)
		{
			if (cps3_char_ram_dirty[i])
			{
				decodechar(Machine->gfx[1], i, (UINT8*)cps3_char_ram, &cps3_tiles16x16_layout);
				cps3_char_ram_dirty[i] = 0;
			}
		}

		cps3_char_ram_is_dirty = 0;
	}

	return;
}

VIDEO_START(cps3)
{
	cps3_ss_ram       = auto_malloc(0x10000);
	cps3_ss_ram_dirty = auto_malloc(0x400);
	cps3_ss_ram_is_dirty = 1;
	memset(cps3_ss_ram, 0x13, 0x10000);
	memset(cps3_ss_ram_dirty, 1, 0x400);
	state_save_register_global_pointer(cps3_ss_ram, 0x10000/4);

	cps3_char_ram = auto_malloc(0x800000);
	cps3_char_ram_dirty = auto_malloc(0x800000/256);
	cps3_char_ram_is_dirty = 1;
	memset(cps3_char_ram, 0x13, 0x800000);
	memset(cps3_char_ram_dirty, 1, 0x8000);
	state_save_register_global_pointer(cps3_char_ram, 0x800000 /4);

	/* create the char set (gfx will then be updated dynamically from RAM) */
	machine->gfx[0] = allocgfx(&cps3_tiles8x8_layout);
	machine->gfx[0]->colortable   = machine->pens;
	machine->gfx[0]->total_colors = machine->drv->total_colors / 16;

	decode_ssram();

	/* create the char set (gfx will then be updated dynamically from RAM) */
	machine->gfx[1] = allocgfx(&cps3_tiles16x16_layout);
	machine->gfx[1]->colortable   = machine->pens;
	machine->gfx[1]->total_colors = machine->drv->total_colors / 64;

	decode_charram();


	cps3_dstram = auto_malloc(0x100000);
		memset(cps3_dstram, 0x00, 0x100000);

	state_save_register_global_pointer(cps3_dstram, 0x100000/4);

	cps3_newtile_ram       = auto_malloc(0x800000);
	memset(cps3_newtile_ram, 0x00, 0x800000);
	state_save_register_global_pointer(cps3_newtile_ram, 0x800000/4);


}

void cps3_draw_bglayer(UINT32*layerregs,  mame_bitmap *bitmap, const rectangle *cliprect )
{
	/* bg layer */
	int scrollx;
	int scrolly;
	int zoom;
	int enable;
	int mapbase;
	int linebase;

	int x,y,count;


	//popmessage("%08x %08x %08x", layerregs[0], layerregs[1], layerregs[2]);
	scrollx =  (layerregs[0]&0xffff0000)>>16;
	scrolly =  (layerregs[0]&0x0000ffff)>>0;
	zoom    =  (layerregs[1]&0xffff0000)>>16;
	enable  =  (layerregs[1]&0x00008000)>>15;
	                       //0x00007fff) ?
	mapbase =  (layerregs[2]&0x007f0000)>>16;
	linebase=  (layerregs[2]&0x7f000000)>>24;

	mapbase = mapbase << 10;

	if (enable)
	{
		count = 0;
		for (y=0;y<64;y++)
		{
			for (x=0;x<64;x++)
			{
				UINT32 dat = cps3_spriteram[mapbase+count];
				int tileno = (dat & 0xffff0000)>>17;
				int yflip  = (dat & 0x00000800)>>11;
				int xflip  = (dat & 0x00001000)>>12;
				int colour = (dat & 0x000001ff)>>0;
//              colour +=0x100;
				drawgfx(bitmap, Machine->gfx[1],tileno,colour,xflip,yflip,(x*16)-scrollx,(y*16)-scrolly,cliprect,TRANSPARENCY_PEN,0);
				count++;
			}
		}
	}
}

VIDEO_UPDATE(cps3)
{
	int y,x, count;
	int offset;

	for (offset=0;offset<0x8000;offset++)
	{
		int r,g,b;
		UINT32 dat;
		int palwritebase = 0x000;
	//  int palreadbase = (cps3_ss_pal_base << 8);
		int palreadbase = 0x0000;

	//  if (offset == 0) if(DEBUG_PRINTF) printf("palreadbase %08x\n", palreadbase*4);

		dat = (cps3_colourram[(palreadbase+offset)&0xffff] & 0xffff0000)>>16;
		r = (dat >> 0) & 0x1f;
		g = (dat >> 5) & 0x1f;
		b = (dat >> 10) & 0x1f;
		palette_set_color(Machine,palwritebase+offset*2,MAKE_RGB(r<<3,g<<3,b<<3));

		dat = (cps3_colourram[(palreadbase+offset)&0xffff] & 0x0000ffff)>>0;
		r = (dat >> 0) & 0x1f;
		g = (dat >> 5) & 0x1f;
		b = (dat >> 10) & 0x1f;
		palette_set_color(Machine,palwritebase+offset*2+1,MAKE_RGB(r<<3,g<<3,b<<3));
	}

	// copy palette for text layer  - NOTE THIS OVERWRITES THE ABOVE, we don't have enough palette entries in mame...
	// use custom drawing instead
	for (offset=0;offset<0x100;offset++)
	{
		int r,g,b;
		UINT32 dat;
		int palwritebase = 0x000;
		int palreadbase = (cps3_ss_pal_base << 8);

		dat = (cps3_colourram[(palreadbase+offset)&0xffff] & 0xffff0000)>>16;
		r = (dat >> 0) & 0x1f;
		g = (dat >> 5) & 0x1f;
		b = (dat >> 10) & 0x1f;
		palette_set_color(Machine,palwritebase+offset*2,MAKE_RGB(r<<3,g<<3,b<<3));

		dat = (cps3_colourram[(palreadbase+offset)&0xffff] & 0x0000ffff)>>0;
		r = (dat >> 0) & 0x1f;
		g = (dat >> 5) & 0x1f;
		b = (dat >> 10) & 0x1f;
		palette_set_color(Machine,palwritebase+offset*2+1,MAKE_RGB(r<<3,g<<3,b<<3));
	}



	decode_ssram();
	decode_charram();

	fillbitmap(bitmap,get_black_pen(machine),cliprect);

	cps3_draw_bglayer(tilemap20_regs_base, bitmap, cliprect);
	cps3_draw_bglayer(tilemap30_regs_base, bitmap, cliprect);
	cps3_draw_bglayer(tilemap40_regs_base, bitmap, cliprect);
	cps3_draw_bglayer(tilemap50_regs_base, bitmap, cliprect);

	popmessage("%08x %08x %08x %08x %08x %08x %08x %08x",
	cps3_unk_vidregs[0],cps3_unk_vidregs[1],cps3_unk_vidregs[2],cps3_unk_vidregs[3],
	cps3_unk_vidregs[4],cps3_unk_vidregs[5],cps3_unk_vidregs[6],cps3_unk_vidregs[7]);


	/* Sprites */
	{
		int i;

		for (i=0x00000/4;i<0x2000/4;i+=4)
		{
			int xpos =  	(cps3_spriteram[i+1]&0x03ff0000)>>16;
			int ypos =  	cps3_spriteram[i+1]&0x000003ff;
			int j;
			int gscroll =      (cps3_spriteram[i+0]&0x70000000)>>28;
			UINT8  length =    (cps3_spriteram[i+0]&0x00ff0000)>>16; // how many entries in the sprite table
			UINT32 start  =    (cps3_spriteram[i+0]&0x00007ff0)>>4;
			int global_xflip = (cps3_spriteram[i+2]&0x10000000)>>28;


			int gscrollx = (cps3_unk_vidregs[gscroll]&0x03ff0000)>>16;
			int gscrolly = (cps3_unk_vidregs[gscroll]&0x000003ff)>>0;
			start = (start * 0x100) >> 2;

			if ((cps3_spriteram[i+0]&0xf0000000) == 0x80000000)
				break;

			for (j=0;j<(length)*4;j+=4)
			{

				UINT32 value1 =  	(cps3_spriteram[start+j+0]);
				UINT32 value2 =  	(cps3_spriteram[start+j+1]);
				UINT32 value3 =  	(cps3_spriteram[start+j+2]);


				//UINT8* srcdata = (UINT8*)cps3_char_ram;
				//UINT32 sourceoffset = (value1 >>14)&0x7fffff;
				int count;

				UINT32 tileno = (value1&0xfffe0000)>>17;

				int xpos2 = (value2 & 0x03ff0000)>>16;
				int ypos2 = (value2 & 0x000003ff)>>0;
				int pal =  (value1  & 0x000001ff);
				int flipx = (value1 & 0x00001000)>>12;
				int flipy = (value1 & 0x00000800)>>11;


				/* these are the sizes to actually draw */
				int ysizedraw2 = ((value3 & 0x7f000000)>>24);
				int xsizedraw2 = ((value3 & 0x007f0000)>>16);
				int xx,yy;

				int tilestable[4] = { 1,1,2,4 };
				int ysize2 = ((value3 & 0x0000000c)>>2);
				int xsize2 = ((value3 & 0x00000003)>>0);
				UINT32 xinc,yinc;

				ysize2 = tilestable[ysize2];
				xsize2 = tilestable[xsize2];

				xinc = ((xsizedraw2+1)<<16) / ((xsize2*0x10));
				yinc = ((ysizedraw2+1)<<16) / ((ysize2*0x10));

				xsize2-=1;
				ysize2-=1;

				flipx ^= global_xflip;

				xpos2+=((xsizedraw2+1)/2);
				ypos2+=((ysizedraw2+1)/2);

				if (!flipx) xpos2-= xsize2*((16*xinc)>>16);

				{
					count = 0;
					for (xx=0;xx<xsize2+1;xx++)
					{
						for (yy=0;yy<ysize2+1;yy++)
						{
							int current_xpos;
							int current_ypos;

							if (!flipx) current_xpos = (xpos+xpos2+xx* ((16*xinc)>>16)  );
							else current_xpos = (xpos+xpos2-xx*((16*xinc)>>16));
							//current_xpos +=  rand()&0x3ff;
							current_xpos += gscrollx;
							current_xpos &=0x3ff;
							if (current_xpos&0x200) current_xpos-=0x400;



							current_ypos = (ypos+ypos2-yy*((16*yinc)>>16));
							current_ypos += gscrolly;
							current_ypos = 0x3ff-current_ypos;
							current_ypos -= 24;
							current_ypos &=0x3ff;

							if (current_ypos&0x200) current_ypos-=0x400;




							//drawgfx(bitmap, Machine->gfx[1],tileno+count,pal,0^flipx,0^flipy,current_xpos,current_ypos,cliprect,TRANSPARENCY_PEN,0);
							//drawgfxzoom
							drawgfxzoom(bitmap, Machine->gfx[1],tileno+count,pal,0^flipx,0^flipy,current_xpos,current_ypos,cliprect,TRANSPARENCY_PEN,0,xinc,yinc);


							count++;

						}
					}
				}
//              */

			//  printf("cell %08x %08x %08x\n",value1, value2, value3);
			}
		}
	}


	// fg layer
	{
		count = 0;
		for (y=0;y<64;y++)
		{
			for (x=0;x<64;x++)
			{
				UINT32 data = cps3_ss_ram[count];
				UINT32 tile = (data >> 16) & 0x1ff;
				int pal = (data&0x003f) >> 1;
				int flipx = (data & 0x0080) >> 7;
				int flipy = (data & 0x0040) >> 6;

				tile+=0x200;
				drawgfx(bitmap, Machine->gfx[0],tile,pal,flipx,flipy,x*8,y*8,cliprect,TRANSPARENCY_PEN,0);
				count++;
			}
		}
	}



	return 0;
}

#if 0
// do custom drawing function here
void custom_draw_fg_layer(  mame_bitmap *bitmap, const rectangle *cliprect }
{
	/* Draw the FG layer directly */
	int line;

	for (line=0;line<224;line++)
	{
		int ytile;

		ytile     = line/8;
		ytileline = line%8;

		for (x=0;x<48;x++)
		{
			UINT32 data = cps3_ss_ram[ytile*64+x];
			UINT32 tile = (data >> 16) & 0x1ff;
			int pal = (data&0x003f) >> 1;
			int flipx = (data & 0x0080) >> 7;
			int flipy = (data & 0x0040) >> 6;


		}
	}
}
#endif

READ32_HANDLER( cps3_ssram_r )
{
	return cps3_ss_ram[offset];
}

WRITE32_HANDLER( cps3_ssram_w )
{
	COMBINE_DATA(&cps3_ss_ram[offset]);
	if (offset>0x8000/4)
	{
		cps3_ss_ram_dirty[offset/16] = 1;
		cps3_ss_ram_is_dirty = 1;
	}
}

WRITE32_HANDLER( cps3_0xc0000000_ram_w )
{
	COMBINE_DATA( &cps3_0xc0000000_ram[offset] );
	// store a decrypted copy
	cps3_0xc0000000_ram_decrypted[offset] = cps3_0xc0000000_ram[offset]^cps3_mask(offset*4+0xc0000000, cps3_key1, cps3_key2, cps3_key3, cps3_key4, cps3_key5);
}

static offs_t cps3_opbase_handler(offs_t address)
{
//  if(DEBUG_PRINTF) printf("address %04x\n",address);

	/* BIOS ROM */
	if (address < 0x80000)
	{
		opcode_base = opcode_arg_base = memory_region(REGION_USER1);
		return ~0;
	}
	/* RAM */
	else if (address >= 0x06000000 && address <= 0x06ffffff)
	{
		opcode_base = (UINT8*)decrypted_gamerom-0x06000000;
		opcode_arg_base = (UINT8*)decrypted_gamerom-0x06000000;

		if (cps3_isSpecial) opcode_arg_base = (UINT8*) memory_region(REGION_USER4)-0x06000000;


		return ~0;
	}
	else if (address >= 0xc0000000 && address <= 0xc00003ff)
	{
//      opcode_base = (void*)cps3_0xc0000000_ram_decrypted;
		opcode_base = (UINT8*)cps3_0xc0000000_ram_decrypted-0xc0000000;
		opcode_arg_base = (UINT8*)cps3_0xc0000000_ram-0xc0000000;
		return ~0;
	}

	/* anything else falls through */
	return address;
}

UINT32 cram_bank = 0;

WRITE32_HANDLER( cram_bank_w )
{
	if (ACCESSING_LSB)
	{
		// this seems to be related to accesses to the 0x04100000 region
		if (cram_bank != data)
		{
			cram_bank = data;
		//if(data&0xfffffff0)
		//bank_w 00000000, ffff0000
		//bank_w 00000001, ffff0000
		//bank_w 00000002, ffff0000
		//bank_w 00000003, ffff0000
		//bank_w 00000004, ffff0000
		//bank_w 00000005, ffff0000
		//bank_w 00000006, ffff0000
		//bank_w 00000007, ffff0000
		// during CHARACTER RAM test..
			if(DEBUG_PRINTF) printf("bank_w %08x, %08x\n",data,mem_mask);

		}
	}
	else
	{
		if(DEBUG_PRINTF) printf("bank_w LSB32 %08x, %08x\n",data,mem_mask);

	}
}

READ32_HANDLER( cram_data_r )
{
	UINT32 fulloffset = (((cram_bank&0x7)*0x100000)/4) + offset;
	return cps3_char_ram[fulloffset];
}

WRITE32_HANDLER( cram_data_w )
{
	UINT32 fulloffset = (((cram_bank&0x7)*0x100000)/4) + offset;
	COMBINE_DATA(&cps3_char_ram[fulloffset]);
	cps3_char_ram_dirty[fulloffset/0x40] = 1;
	cps3_char_ram_is_dirty = 1;
}

/* FLASH ROM ACCESS */

READ32_HANDLER( cps3_gfxflash_r )
{
	UINT32 result = 0;

	if(DEBUG_PRINTF) printf("gfxflash_r\n");

	if (!(mem_mask & 0xff000000))	// GFX Flash 1
	{
		logerror("read GFX flash chip 1 addr %02x\n", (offset<<1));
		result |= intelflash_read(8, (offset<<1) ) << 24;
	}
	if (!(mem_mask & 0x00ff0000))	// GFX Flash 2
	{
		logerror("read GFX flash chip 1 addr %02x\n", (offset<<1));
		result |= intelflash_read(9, (offset<<1) ) << 16;
	}
	if (!(mem_mask & 0x0000ff00))	// GFX Flash 1
	{
		logerror("read GFX flash chip 1 addr %02x\n", (offset<<1)+1);
		result |= intelflash_read(8, (offset<<1)+0x1 ) << 8;
	}
	if (!(mem_mask & 0x000000ff))	// GFX Flash 2
	{
		logerror("read GFX flash chip 1 addr %02x\n", (offset<<1)+1);
		result |= intelflash_read(9, (offset<<1)+0x1 ) << 0;
	}

	printf("read GFX flash chips addr %02x returning %08x mem_mask %08x crambank %08x gfxbank %08x\n", offset*2, result,mem_mask,  cram_bank, cram_gfxflash_bank  );

	return result;
}

WRITE32_HANDLER( cps3_gfxflash_w )
{

	int command;

	if(DEBUG_PRINTF) printf("cps3_gfxflash_w %08x %08x %08x\n", offset *2, data, mem_mask);


	if (!(mem_mask & 0xff000000))	// GFX Flash 1
	{
		command = (data >> 24) & 0xff;
		logerror("write to GFX flash chip 1 addr %02x cmd %02x\n", (offset<<1), command);
		intelflash_write(8, (offset<<1), command);
	}
	if (!(mem_mask & 0x00ff0000))	// GFX Flash 2
	{
		command = (data >> 16) & 0xff;
		logerror("write to GFX flash chip 2 addr %02x cmd %02x\n", (offset<<1), command);
		intelflash_write(9, (offset<<1), command);
	}
	if (!(mem_mask & 0x0000ff00))	// GFX Flash 1
	{
		command = (data >> 8) & 0xff;
		logerror("write to GFX flash chip 1 addr %02x cmd %02x\n", (offset<<1)+1, command);
		intelflash_write(8, (offset<<1)+0x1, command);
	}
	if (!(mem_mask & 0x000000ff))	// GFX Flash 2
	{
		command = (data >> 0) & 0xff;
		logerror("write to GFX flash chip 2 addr %02x cmd %02x\n", (offset<<1)+1, command);
		intelflash_write(9, (offset<<1)+0x1, command);
	}
}



UINT32 cps3_flashmain_r(int base, UINT32 offset, UINT32 mem_mask)
{
	UINT32 result = 0;

	if (!(mem_mask & 0xff000000))	// Flash 1
	{
//      logerror("read flash chip %d addr %02x\n", base+0, offset*4 );
		result |= (intelflash_read(base+0, offset)<<24);
	}
	if (!(mem_mask & 0x00ff0000))	// Flash 1
	{
//      logerror("read flash chip %d addr %02x\n", base+1, offset*4 );
		result |= (intelflash_read(base+1, offset)<<16);
	}
	if (!(mem_mask & 0x0000ff00))	// Flash 1
	{
//      logerror("read flash chip %d addr %02x\n", base+2, offset*4 );
		result |= (intelflash_read(base+2, offset)<<8);
	}
	if (!(mem_mask & 0x000000ff))	// Flash 1
	{
//      logerror("read flash chip %d addr %02x\n", base+3, offset*4 );
		result |= (intelflash_read(base+3, offset)<<0);
	}

	if (base==4) logerror("read flash chips addr %02x returning %08x\n", offset*4, result );

	return result;
}

READ32_HANDLER( cps3_flash1_r )
{
	return cps3_flashmain_r(0, offset,mem_mask);
}

READ32_HANDLER( cps3_flash2_r )
{
	return cps3_flashmain_r(4, offset,mem_mask);
}

void cps3_flashmain_w(int base, UINT32 offset, UINT32 data, UINT32 mem_mask)
{
	int command;
	if (!(mem_mask & 0xff000000))	// Flash 1
	{
		command = (data >> 24) & 0xff;
		logerror("write to flash chip %d addr %02x cmd %02x\n", base+0, offset, command);
		intelflash_write(base+0, offset, command);
	}
	if (!(mem_mask & 0x00ff0000))	// Flash 2
	{
		command = (data >> 16) & 0xff;
		logerror("write to flash chip %d addr %02x cmd %02x\n", base+1, offset, command);
		intelflash_write(base+1, offset, command);
	}
	if (!(mem_mask & 0x0000ff00))	// Flash 2
	{
		command = (data >> 8) & 0xff;
		logerror("write to flash chip %d addr %02x cmd %02x\n", base+2, offset, command);
		intelflash_write(base+2, offset, command);
	}
	if (!(mem_mask & 0x000000ff))	// Flash 2
	{
		command = (data >> 0) & 0xff;
		logerror("write to flash chip %d addr %02x cmd %02x\n", base+3, offset, command);
		intelflash_write(base+3, offset, command);
	}
}
WRITE32_HANDLER( cps3_flash1_w )
{
	cps3_flashmain_w(0, offset,data,mem_mask);
}

WRITE32_HANDLER( cps3_flash2_w )
{
	cps3_flashmain_w(4, offset,data,mem_mask);
}

WRITE32_HANDLER( cram_gfxflash_bank_w )
{
	if (ACCESSING_MSB32)
	{
		//if(DEBUG_PRINTF) printf("cram_gfxflash_bank_w MSB32 %08x\n",data);
		cram_gfxflash_bank = data;
	}

	if (ACCESSING_LSB32)
	{
		if(DEBUG_PRINTF) printf("cram_gfxflash_bank_LSB_w LSB32 %08x\n",data);
	}
}

READ32_HANDLER( cps3_io1_r )
{
	//logerror("%06x io0_r\n", activecpu_get_pc());
	return readinputport(0);
}

READ32_HANDLER( cps3_io2_r )
{
	//logerror("%06x io1_r\n", activecpu_get_pc());
	return readinputport(1);
}

// this seems to be dma active flags, and maybe vblank... not if it is anything else
READ32_HANDLER( cps3_vbl_r )
{
	//return mame_rand(Machine)^ (mame_rand(Machine)<<16);
	return 0x00000000;

	if (video_screen_get_vblank(0))
		return 0xfff0ffff /*| (rand()<<16)*/;
	else
		return 0xffffffff;
}

READ32_HANDLER( cps3_unk_io_r )
{///  warzard will crash before booting if you return anything here
	//return 0x00000000;
	return 0xffffffff;//mame_rand(Machine);
}

READ32_HANDLER( cps3_40C0000_r )
{
	return 0x00000000;//0xffffffff;
	//return mame_rand(Machine);
	//return 0xffffffff;
}

READ32_HANDLER( cps3_40C0004_r )
{
	return 0x00000000;//0xffffffff;
//  return mame_rand(Machine);
//  return 0xffffffff;
}

/* EEPROM access is a little odd, I think it accesses eeprom through some kind of
   additional interface, as these writes aren't normal for the type of eeprom we have */
UINT16 cps3_current_eeprom_read;

READ32_HANDLER( cps3_eeprom_r )
{
	int addr = offset*4;

	if (addr>=0x100 && addr<=0x17f)
	{
		if (ACCESSING_MSB32) cps3_current_eeprom_read = (cps3_eeprom[offset-0x100/4] & 0xffff0000)>>16;
		else cps3_current_eeprom_read = (cps3_eeprom[offset-0x100/4] & 0x0000ffff)>>0;
		// read word to latch...
		return 0x00000000;
	}
	else if (addr == 0x200)
	{
		// busy flag / read data..
		if (ACCESSING_MSB32) return 0;
		else
		{
			//if(DEBUG_PRINTF) printf("reading %04x from eeprom\n", cps3_current_eeprom_read);
			return cps3_current_eeprom_read;
		}
	}
	else
	{
	//  if(DEBUG_PRINTF) printf("unk read eeprom addr %04x, mask %08x\n", addr, mem_mask);
		return 0x00000000;
	}
	return 0x00000000;
}

WRITE32_HANDLER( cps3_eeprom_w )
{
	int addr = offset*4;

	if (addr>=0x080 && addr<=0x0ff)
	{
		offset -= 0x80/4;
		COMBINE_DATA(&cps3_eeprom[offset]);
		// write word to storage

	}
	else if (addr>=0x180 && addr<=0x1ff)
	{
		// always 00000000 ? incrememnt access?
	}
	else
	{
	//  if(DEBUG_PRINTF) printf("unk write eeprom addr %04x, data %08x, mask %08x\n", addr, data, mem_mask);
	}

}

READ32_HANDLER( cps3_cdrom_r )
{
//  return mame_rand(Machine);
	return 0x00000000;
//  return 0xffffffff;
}

WRITE32_HANDLER( cps3_ss_pal_base_w )
{
	 if(DEBUG_PRINTF) printf ("cps3_ss_pal_base_w %08x %08x\n", data, mem_mask);

	if(ACCESSING_MSB32)
	{
		cps3_ss_pal_base = (data & 0x00ff0000)>>16;

		if (data & 0xff000000) printf("cps3_ss_pal_base MSB32 upper bits used %04x \n", data);
	}
	else
	{
		printf("cps3_ss_pal_base LSB32 used %04x \n", data);
	}
}

UINT32*tilemap20_regs_base;
UINT32*tilemap30_regs_base;
UINT32*tilemap40_regs_base;
UINT32*tilemap50_regs_base;

//<ElSemi> +0 X  +2 Y +4 unknown +6 enable (&0x8000) +8 low part tilemap base, high part linescroll base
//<ElSemi> (a word each)

UINT32 paldma_source;
UINT32 paldma_realsource;
UINT32 paldma_dest;
UINT32 paldma_fade;
UINT32 paldma_other2;
UINT32 paldma_length;

WRITE32_HANDLER( cps3_palettedma_w )
{
	if (offset==0)
	{
		COMBINE_DATA(&paldma_source);
		paldma_realsource = (paldma_source<<1)-0x400000;
	}
	else if (offset==1)
	{
		COMBINE_DATA(&paldma_dest);
	}
	else if (offset==2)
	{
		COMBINE_DATA(&paldma_fade);
	}
	else if (offset==3)
	{
		COMBINE_DATA(&paldma_other2);

		if (ACCESSING_MSB32)
		{
			paldma_length = (data & 0xffff0000)>>16;
		}
		if (ACCESSING_LSB32)
		{
			if (data & 0x0002)
			{
				int i;
			//  if(DEBUG_PRINTF) printf("CPS3 pal dma start %08x (real: %08x) dest %08x fade %08x other2 %08x (length %04x)\n", paldma_source, paldma_realsource, paldma_dest, paldma_fade, paldma_other2, paldma_length);

				for (i=0;i<paldma_length;i++)
				{
					UINT16* src = (UINT16*)memory_region(REGION_USER5);
					UINT16* dst = (UINT16*)cps3_colourram;

					dst[paldma_dest+i] = src[((paldma_realsource>>1)+i)^1];
				}


				cpunum_set_input_line(0,10, ASSERT_LINE);


			}
		}
	}

}

UINT32 chardma_source;
UINT32 chardma_other;

UINT8* current_table;
UINT32 current_table_address = -1;

int cps3_rle_length = 0;

UINT8 last_normal_byte = 0;


UINT32 process_byte( UINT8 real_byte, UINT32 destination, int max_length )
{
	UINT8* dest       = (UINT8*)cps3_char_ram;

	//printf("process byte for destination %08x\n", destination);

	if (real_byte&0x40)
	{
		int tranfercount = 0;

		//printf("Set RLE Mode\n");
		cps3_rle_length = (real_byte&0x3f)+1;

		//printf("RLE Operation (length %08x\n", cps3_rle_length );

		while (cps3_rle_length)
		{
			dest[(destination+tranfercount)^3] = (last_normal_byte&0x3f);
			cps3_char_ram_dirty[(destination+tranfercount)/0x100] = 1;
			cps3_char_ram_is_dirty = 1;
			//printf("RLE WRite Byte %08x, %02x\n", destination+tranfercount, real_byte);

			tranfercount++;
			cps3_rle_length--;
			max_length--;

	//      if (max_length==0) return max_length; // this is meant to abort the transfer if we exceed dest length,, not working
		}
		return tranfercount;
	}
	else
	{
		//printf("Write Normal Data\n");
		dest[destination^3] = real_byte;
		last_normal_byte = real_byte;
		cps3_char_ram_dirty[destination/0x100] = 1;
		cps3_char_ram_is_dirty = 1;
		return 1;
	}
}


void cps3_do_char_dma( UINT32 real_source, UINT32 real_destination, UINT32 real_length, UINT8* current_table)
{
	UINT8* sourcedata = (UINT8*)memory_region(REGION_USER5);
	int length_remaining;

	last_normal_byte = 0;
	cps3_rle_length = 0;
	length_remaining = real_length;
	while (length_remaining)
	{
		UINT8 current_byte;

		current_byte = sourcedata[real_source^1];
		real_source++;

		if (current_byte & 0x80)
		{
			UINT8 real_byte;
			UINT32 length_processed;
			current_byte &= 0x7f;

			real_byte = sourcedata[(current_table_address+current_byte*2+0)^1];
			length_processed = process_byte( real_byte, real_destination, length_remaining );
			length_remaining-=length_processed; // subtract the number of bytes the operation has taken
			real_destination+=length_processed; // add it onto the destination
			if (length_remaining<=0) return; // if we've expired, exit

			real_byte = sourcedata[(current_table_address+current_byte*2+1)^1];
			length_processed = process_byte( real_byte, real_destination, length_remaining );
			length_remaining-=length_processed; // subtract the number of bytes the operation has taken
			real_destination+=length_processed; // add it onto the destination
			if (length_remaining<=0) return;  // if we've expired, exit
		}
		else
		{
			UINT32 length_processed;
			length_processed = process_byte( current_byte, real_destination, length_remaining );
			length_remaining-=length_processed; // subtract the number of bytes the operation has taken
			real_destination+=length_processed; // add it onto the destination
			if (length_remaining<=0) return;  // if we've expired, exit
		}

//      length_remaining--;
	}

}

void cps3_process_character_dma(UINT32 address)
{
	int i;

	if(DEBUG_PRINTF) printf("charDMA start:\n");

	for (i=0;i<0x1000;i+=3)
	{
		UINT32 dat1 = cps3_char_ram[i+0+(address)];
		UINT32 dat2 = cps3_char_ram[i+1+(address)];
		UINT32 dat3 = cps3_char_ram[i+2+(address)];
		UINT32 real_source      = (dat3<<1)-0x400000;
		UINT32 real_destination =  dat2<<3;
		UINT32 real_length      = (((dat1&0x003fffff)+1)<<3);

		/* 0x01000000 is the end of list marker, 0x13131313 is our default fill */
		if ((dat1==0x01000000) || (dat1==0x13131313)) break;

//      printf("%08x real_source %08x (rom %d offset %08x) real_destination %08x, real_length %08x\n", dat1, real_source, real_source/0x800000, real_source%0x800000, real_destination, real_length);

		if  ( (dat1&0x00c00000) ==0x00800000 )
		{
			/* some kind of setup command, length is ALWAYS 0x7f, values in the table it points to are ALAWYS < 0x80
               and roughly incrementing */
			//if ( real_length != 0x100 )
			//{
			//  if(DEBUG_PRINTF) printf("Command type 0x008xxxxx, length not 0x100\n");
			//}
			//else
			{
				/* We should probably copy this, but a pointer to it is fine for our purposes as the data doesn't change */
				//current_table = memory_region(REGION_USER5)+real_source;
				current_table_address = real_source;
			}
		}
		else if  ( (dat1&0x00c00000) ==0x00400000 )
		{
			/* A Data Table, to be decoded wit the previous table? */
			cps3_do_char_dma( real_source, real_destination, real_length, current_table);
			cpunum_set_input_line(0,10, ASSERT_LINE);

		}
		else
		{
			printf("Unknown DMA List Command Type\b");
		}

	}
}

WRITE32_HANDLER( cps3_characterdma_w )
{
	if(DEBUG_PRINTF) printf("chardma_w %08x %08x %08x\n", offset, data, mem_mask);

	if (offset==0)
	{
		//COMBINE_DATA(&chardma_source);
		if (ACCESSING_LSB32)
		{
			chardma_source = data & 0x0000ffff;
		}
		if (ACCESSING_MSB32)
		{
			if(DEBUG_PRINTF) printf("chardma_w accessing MSB32 of offset 0");
		}
	}
	else if (offset==1)
	{
		COMBINE_DATA(&chardma_other);

		if (ACCESSING_MSB32)
		{
			if ((data>>16) & 0x0040)
			{
				UINT32 list_address;
				list_address = (chardma_source | ((chardma_other&0x003f0000)));

				//printf("chardma_w activated %08x %08x (address = cram %08x)\n", chardma_source, chardma_other, list_address*4 );
				cps3_process_character_dma(list_address);
			}
			else
			{
				if(DEBUG_PRINTF) printf("chardma_w NOT activated %08x %08x\n", chardma_source, chardma_other );
			}

			if ((data>>16) & 0xff80)
				if(DEBUG_PRINTF) printf("chardma_w unknown bits in activate command %08x %08x\n", chardma_source, chardma_other );
		}
		else
		{
			if(DEBUG_PRINTF) printf("chardma_w LSB32 write to activate command %08x %08x\n", chardma_source, chardma_other );
		}
	}
}

WRITE32_HANDLER( cps3_irq10_ack_w )
{
	cpunum_set_input_line(0,10, CLEAR_LINE); return;
}

WRITE32_HANDLER( cps3_irq12_ack_w )
{
	cpunum_set_input_line(0,12, CLEAR_LINE); return;
}

WRITE32_HANDLER( cps3_unk_vidregs_w )
{
	COMBINE_DATA(&cps3_unk_vidregs[offset]);
}

/* there are more unknown writes, but you get the idea */
static ADDRESS_MAP_START( cps3_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x0007ffff) AM_ROM AM_REGION(REGION_USER1, 0) // Bios ROM
	AM_RANGE(0x02000000, 0x0207ffff) AM_RAM // Main RAM

	AM_RANGE(0x03000000, 0x030003ff) AM_RAM // 'FRAM' (SFIII memory test mode ONLY)

	AM_RANGE(0x04000000, 0x0407dfff) AM_RAM AM_BASE(&cps3_spriteram)//AM_WRITE(MWA32_RAM) // Sprite RAM (jojoba tests this size) - doesn't change frame to frame?!

	AM_RANGE(0x04080000, 0x040bffff) AM_RAM AM_BASE(&cps3_colourram)  // Colour RAM (jojoba tests this size) 0x20000 colours?!

	// video registers of some kind probably
	AM_RANGE(0x040C0000, 0x040C0003) AM_READ(cps3_40C0000_r)//?? every frame
	AM_RANGE(0x040C0004, 0x040C0007) AM_READ(cps3_40C0004_r)//AM_READ(cps3_40C0004_r) // warzard reads this!
//  AM_RANGE(0x040C0008, 0x040C000b) AM_WRITE(MWA32_NOP)//??
//  AM_RANGE(0x040C000c, 0x040C000f) AM_READ(cps3_vbl_r)// AM_WRITE(MWA32_NOP)/

	AM_RANGE(0x040C0000, 0x040C001f) AM_WRITE(cps3_unk_vidregs_w)

//  AM_RANGE(0x040C0010, 0x040C0013) AM_WRITE(MWA32_NOP)//??
//  AM_RANGE(0x040C0014, 0x040C0017) AM_WRITE(MWA32_NOP)//??
//  AM_RANGE(0x040C0018, 0x040C001b) AM_WRITE(MWA32_NOP)//??
//  AM_RANGE(0x040C001c, 0x040C001f) AM_WRITE(MWA32_NOP)//??

	AM_RANGE(0x040C0020, 0x040C002b) AM_WRITE(MWA32_RAM) AM_BASE(&tilemap20_regs_base)
	AM_RANGE(0x040C0030, 0x040C003b) AM_WRITE(MWA32_RAM) AM_BASE(&tilemap30_regs_base)
	AM_RANGE(0x040C0040, 0x040C004b) AM_WRITE(MWA32_RAM) AM_BASE(&tilemap40_regs_base)
	AM_RANGE(0x040C0050, 0x040C005b) AM_WRITE(MWA32_RAM) AM_BASE(&tilemap50_regs_base)

	AM_RANGE(0x040C0094, 0x040C009b) AM_WRITE(cps3_characterdma_w)


	AM_RANGE(0x040C00a0, 0x040C00af) AM_WRITE(cps3_palettedma_w)


	AM_RANGE(0x040C0084, 0x040C0087) AM_WRITE(cram_bank_w)
	AM_RANGE(0x040C0088, 0x040C008b) AM_WRITE(cram_gfxflash_bank_w)

	AM_RANGE(0x040e0000, 0x040e02ff) AM_READWRITE(cps3_sound_r, cps3_sound_w)

	AM_RANGE(0x04100000, 0x041fffff) AM_READWRITE(cram_data_r, cram_data_w)
	AM_RANGE(0x04200000, 0x043fffff) AM_READWRITE(cps3_gfxflash_r, cps3_gfxflash_w) // GFX Flash ROMS

	AM_RANGE(0x05000000, 0x05000003) AM_READ( cps3_io1_r )
	AM_RANGE(0x05000004, 0x05000007) AM_READ( cps3_io2_r )

	AM_RANGE(0x05100000, 0x05100003) AM_WRITE( cps3_irq12_ack_w )
	AM_RANGE(0x05110000, 0x05110003) AM_WRITE( cps3_irq10_ack_w )

	AM_RANGE(0x05000008, 0x0500000b) AM_WRITE( MWA32_NOP ) // ?? every frame

	AM_RANGE(0x05000A00, 0x05000A03) AM_READ( cps3_unk_io_r ) // ?? every frame
	AM_RANGE(0x05000A04, 0x05000A07) AM_READ( cps3_unk_io_r ) // ?? every frame
	AM_RANGE(0x05000A08, 0x05000A0b) AM_READ( cps3_unk_io_r ) // ?? every frame
	AM_RANGE(0x05000A0c, 0x05000A0f) AM_READ( cps3_unk_io_r ) // ?? every frame
	AM_RANGE(0x05000A10, 0x05000A13) AM_READ( cps3_unk_io_r ) // ?? every frame
	AM_RANGE(0x05000A14, 0x05000A17) AM_READ( cps3_unk_io_r ) // ?? every frame
	AM_RANGE(0x05000A18, 0x05000A1b) AM_READ( cps3_unk_io_r ) // ?? every frame
	AM_RANGE(0x05000A1c, 0x05000A1f) AM_READ( cps3_unk_io_r ) // ?? every frame

	// ??
	AM_RANGE(0x05001000, 0x05001203) AM_READWRITE( cps3_eeprom_r, cps3_eeprom_w )

	AM_RANGE(0x05040000, 0x0504ffff) AM_RAM AM_READWRITE(cps3_ssram_r,cps3_ssram_w) // 'SS' RAM (Static Screen?) (text tilemap + toles)

/*
    AM_RANGE(0x0505000C, 0x0505000f) AM_WRITE( MWA32_NOP ) // ?? every frame
    AM_RANGE(0x05050010, 0x05050013) AM_WRITE( MWA32_NOP ) // ?? every frame
    AM_RANGE(0x05050020, 0x05050023) AM_WRITE( MWA32_NOP ) // ?? every frame
*/

	AM_RANGE(0x05050024, 0x05050027) AM_WRITE( cps3_ss_pal_base_w )
//  AM_RANGE(0x05050028, 0x0505002b) AM_WRITE( MWA32_NOP ) // ?? every frame

//  AM_RANGE(0x05100000, 0x05100003) AM_WRITE( MWA32_NOP ) // every frame, irq ack?

	AM_RANGE(0x05140000, 0x05140003) AM_READ( cps3_cdrom_r )

	AM_RANGE(0x06000000, 0x067fffff) AM_ROM AM_READWRITE( cps3_flash1_r, cps3_flash1_w ) /* Flash ROMs simm 1 */
	AM_RANGE(0x06800000, 0x06ffffff) AM_ROM AM_READWRITE( cps3_flash2_r, cps3_flash2_w ) /* Flash ROMs simm 2 */

	AM_RANGE(0xc0000000, 0xc00003ff) AM_RAM AM_WRITE( cps3_0xc0000000_ram_w ) AM_BASE(&cps3_0xc0000000_ram) /* Executes code from here */
ADDRESS_MAP_END




INPUT_PORTS_START( cps3 )
	PORT_START
	PORT_BIT( 0x00000001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT( 0x00000002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT( 0x00000004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0x00000008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x00000010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x00000020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x00000040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
    PORT_DIPNAME( 0x00000080, 0x00000080, DEF_STR( Unknown ) )
    PORT_DIPSETTING(          0x00000080, DEF_STR( Off ) )
    PORT_DIPSETTING(          0x00000000, DEF_STR( On ) )
	PORT_BIT( 0x00000100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT( 0x00000200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT( 0x00000400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT( 0x00000800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0x00001000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x00002000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x00004000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
    PORT_DIPNAME( 0x00008000, 0x00008000, DEF_STR( Unknown ) )
    PORT_DIPSETTING(          0x00008000, DEF_STR( Off ) )
    PORT_DIPSETTING(          0x00000000, DEF_STR( On ) )
	PORT_BIT( 0x00010000, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x00020000, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME( DEF_STR( Service_Mode )) PORT_CODE(KEYCODE_F2)
    PORT_DIPNAME( 0x00040000, 0x00040000, DEF_STR( Unknown ) )
    PORT_DIPSETTING(          0x00040000, DEF_STR( Off ) )
    PORT_DIPSETTING(          0x00000000, DEF_STR( On ) )
    PORT_DIPNAME( 0x00080000, 0x00080000, DEF_STR( Unknown ) )
    PORT_DIPSETTING(          0x00080000, DEF_STR( Off ) )
    PORT_DIPSETTING(          0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00100000, 0x00100000, DEF_STR( Unknown ) )
    PORT_DIPSETTING(          0x00100000, DEF_STR( Off ) )
    PORT_DIPSETTING(          0x00000000, DEF_STR( On ) )
    PORT_DIPNAME( 0x00200000, 0x00200000, DEF_STR( Unknown ) )
    PORT_DIPSETTING(          0x00200000, DEF_STR( Off ) )
    PORT_DIPSETTING(          0x00000000, DEF_STR( On ) )
    PORT_DIPNAME( 0x00400000, 0x00400000, DEF_STR( Unknown ) )
    PORT_DIPSETTING(          0x00400000, DEF_STR( Off ) )
    PORT_DIPSETTING(          0x00000000, DEF_STR( On ) )
    PORT_DIPNAME( 0x00800000, 0x00800000, DEF_STR( Unknown ) )
    PORT_DIPSETTING(          0x00800000, DEF_STR( Off ) )
    PORT_DIPSETTING(          0x00000000, DEF_STR( On ) )
	PORT_BIT( 0x01000000, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02000000, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04000000, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_PLAYER(2)
	PORT_DIPNAME( 0x08000000, 0x08000000, DEF_STR( Unknown ) )
    PORT_DIPSETTING(          0x08000000, DEF_STR( Off ) )
    PORT_DIPSETTING(          0x00000000, DEF_STR( On ) )
	PORT_BIT( 0x10000000, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20000000, IP_ACTIVE_LOW, IPT_START2 )
    PORT_DIPNAME( 0x40000000, 0x40000000, DEF_STR( Unknown ) )
    PORT_DIPSETTING(          0x40000000, DEF_STR( Off ) )
    PORT_DIPSETTING(          0x00000000, DEF_STR( On ) )
    PORT_DIPNAME( 0x80000000, 0x80000000, DEF_STR( Unknown ) )
    PORT_DIPSETTING(          0x80000000, DEF_STR( Off ) )
    PORT_DIPSETTING(          0x00000000, DEF_STR( On ) )

	PORT_START
    PORT_DIPNAME( 0x00000001, 0x00000001, "xxx" )
    PORT_DIPSETTING(          0x00000001, DEF_STR( Off ) )
    PORT_DIPSETTING(          0x00000000, DEF_STR( On ) )
    PORT_DIPNAME( 0x00000002, 0x00000002, DEF_STR( Unknown ) )
    PORT_DIPSETTING(          0x00000002, DEF_STR( Off ) )
    PORT_DIPSETTING(          0x00000000, DEF_STR( On ) )
    PORT_DIPNAME( 0x00000004, 0x00000004, DEF_STR( Unknown ) )
    PORT_DIPSETTING(          0x00000004, DEF_STR( Off ) )
    PORT_DIPSETTING(          0x00000000, DEF_STR( On ) )
    PORT_DIPNAME( 0x00000008, 0x00000008, DEF_STR( Unknown ) )
    PORT_DIPSETTING(          0x00000008, DEF_STR( Off ) )
    PORT_DIPSETTING(          0x00000000, DEF_STR( On ) )
    PORT_DIPNAME( 0x00000010, 0x00000010, DEF_STR( Unknown ) )
    PORT_DIPSETTING(          0x00000010, DEF_STR( Off ) )
    PORT_DIPSETTING(          0x00000000, DEF_STR( On ) )
    PORT_DIPNAME( 0x00000020, 0x00000020, DEF_STR( Unknown ) )
    PORT_DIPSETTING(          0x00000020, DEF_STR( Off ) )
    PORT_DIPSETTING(          0x00000000, DEF_STR( On ) )
    PORT_DIPNAME( 0x00000040, 0x00000040, DEF_STR( Unknown ) )
    PORT_DIPSETTING(          0x00000040, DEF_STR( Off ) )
    PORT_DIPSETTING(          0x00000000, DEF_STR( On ) )
    PORT_DIPNAME( 0x00000080, 0x00000080, DEF_STR( Unknown ) )
    PORT_DIPSETTING(          0x00000080, DEF_STR( Off ) )
    PORT_DIPSETTING(          0x00000000, DEF_STR( On ) )
    PORT_DIPNAME( 0x00000100, 0x00000100, DEF_STR( Unknown ) )
    PORT_DIPSETTING(          0x00000100, DEF_STR( Off ) )
    PORT_DIPSETTING(          0x00000000, DEF_STR( On ) )
    PORT_DIPNAME( 0x00000200, 0x00000200, DEF_STR( Unknown ) )
    PORT_DIPSETTING(          0x00000200, DEF_STR( Off ) )
    PORT_DIPSETTING(          0x00000000, DEF_STR( On ) )
    PORT_DIPNAME( 0x00000400, 0x00000400, DEF_STR( Unknown ) )
    PORT_DIPSETTING(          0x00000400, DEF_STR( Off ) )
    PORT_DIPSETTING(          0x00000000, DEF_STR( On ) )
    PORT_DIPNAME( 0x00000800, 0x00000800, DEF_STR( Unknown ) )
    PORT_DIPSETTING(          0x00000800, DEF_STR( Off ) )
    PORT_DIPSETTING(          0x00000000, DEF_STR( On ) )
    PORT_DIPNAME( 0x00001000, 0x00001000, DEF_STR( Unknown ) )
    PORT_DIPSETTING(          0x00001000, DEF_STR( Off ) )
    PORT_DIPSETTING(          0x00000000, DEF_STR( On ) )
    PORT_DIPNAME( 0x00002000, 0x00002000, DEF_STR( Unknown ) )
    PORT_DIPSETTING(          0x00002000, DEF_STR( Off ) )
    PORT_DIPSETTING(          0x00000000, DEF_STR( On ) )
    PORT_DIPNAME( 0x00004000, 0x00004000, DEF_STR( Unknown ) )
    PORT_DIPSETTING(          0x00004000, DEF_STR( Off ) )
    PORT_DIPSETTING(          0x00000000, DEF_STR( On ) )
    PORT_DIPNAME( 0x00008000, 0x00008000, DEF_STR( Unknown ) )
    PORT_DIPSETTING(          0x00008000, DEF_STR( Off ) )
    PORT_DIPSETTING(          0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00010000, 0x00010000, "1" )
    PORT_DIPSETTING(          0x00010000, DEF_STR( Off ) )
    PORT_DIPSETTING(          0x00000000, DEF_STR( On ) )
	PORT_BIT( 0x00020000, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_PLAYER(1)
	PORT_BIT( 0x00040000, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_PLAYER(1)
	PORT_BIT( 0x00080000, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1)
	PORT_BIT( 0x00100000, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2)
	PORT_BIT( 0x00200000, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_PLAYER(2)
    PORT_DIPNAME( 0x00400000, 0x00400000, DEF_STR( Unknown ) )
    PORT_DIPSETTING(          0x00400000, DEF_STR( Off ) )
    PORT_DIPSETTING(          0x00000000, DEF_STR( On ) )
    PORT_DIPNAME( 0x00800000, 0x00800000, DEF_STR( Unknown ) )
    PORT_DIPSETTING(          0x00800000, DEF_STR( Off ) )
    PORT_DIPSETTING(          0x00000000, DEF_STR( On ) )
    PORT_DIPNAME( 0x01000000, 0x01000000, DEF_STR( Unknown ) )
    PORT_DIPSETTING(          0x01000000, DEF_STR( Off ) )
    PORT_DIPSETTING(          0x00000000, DEF_STR( On ) )
    PORT_DIPNAME( 0x02000000, 0x02000000, DEF_STR( Unknown ) )
    PORT_DIPSETTING(          0x02000000, DEF_STR( Off ) )
    PORT_DIPSETTING(          0x00000000, DEF_STR( On ) )
    PORT_DIPNAME( 0x04000000, 0x04000000, DEF_STR( Unknown ) )
    PORT_DIPSETTING(          0x04000000, DEF_STR( Off ) )
    PORT_DIPSETTING(          0x00000000, DEF_STR( On ) )
    PORT_DIPNAME( 0x08000000, 0x08000000, DEF_STR( Unknown ) )
    PORT_DIPSETTING(          0x08000000, DEF_STR( Off ) )
    PORT_DIPSETTING(          0x00000000, DEF_STR( On ) )
    PORT_DIPNAME( 0x10000000, 0x10000000, DEF_STR( Unknown ) )
    PORT_DIPSETTING(          0x10000000, DEF_STR( Off ) )
    PORT_DIPSETTING(          0x00000000, DEF_STR( On ) )
    PORT_DIPNAME( 0x20000000, 0x20000000, DEF_STR( Unknown ) )
    PORT_DIPSETTING(          0x20000000, DEF_STR( Off ) )
    PORT_DIPSETTING(          0x00000000, DEF_STR( On ) )
    PORT_DIPNAME( 0x40000000, 0x40000000, DEF_STR( Unknown ) )
    PORT_DIPSETTING(          0x40000000, DEF_STR( Off ) )
    PORT_DIPSETTING(          0x00000000, DEF_STR( On ) )
    PORT_DIPNAME( 0x80000000, 0x80000000, DEF_STR( Unknown ) )
    PORT_DIPSETTING(          0x80000000, DEF_STR( Off ) )
    PORT_DIPSETTING(          0x00000000, DEF_STR( On ) )
INPUT_PORTS_END

static INTERRUPT_GEN(cps3_interrupt)
{
	static int i = 0;
	i++;
	i%=16;

	if (i==8) { cpunum_set_input_line(0,10, ASSERT_LINE); return ; }


	if (i==14) { cpunum_set_input_line(0,12, ASSERT_LINE); return ; }

}



struct sh2_config sh2cp_conf_slave  = { 1 };


static struct CustomSound_interface custom_interface =
{
	cps3_sh_start
};


mame_timer* fastboot_timer;

static void fastboot_timer_callback(int num)
{
	UINT32 *rom =  (UINT32*)decrypted_gamerom;//memory_region ( REGION_USER4 );
	if (cps3_isSpecial) rom = (UINT32*)memory_region(REGION_USER4);

//  printf("fastboot callback %08x %08x", rom[0], rom[1]);
	cpunum_set_reg(0,SH2_PC, rom[0]);
	cpunum_set_reg(0,SH2_R15, rom[1]);
	cpunum_set_reg(0,SH2_VBR, 0x6000000);
}

MACHINE_RESET(cps3_reset)
{
	fastboot_timer = timer_alloc(fastboot_timer_callback);
//  printf("reset\n");
	timer_adjust(fastboot_timer,  TIME_NOW, 0, 0);
}

#define MASTER_CLOCK	42954500

static MACHINE_DRIVER_START( cps3 )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", SH2, 6250000) // ??
	MDRV_CPU_PROGRAM_MAP(cps3_map,0)
	MDRV_CPU_VBLANK_INT(cps3_interrupt,16)
//  MDRV_CPU_CONFIG(sh2cp_conf_slave)

	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_REAL_60HZ_VBLANK_DURATION)


	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(384*2, 512)
//  MDRV_SCREEN_VISIBLE_AREA(0, 383, 0, 223/*511*/) // you can see debug texts in jojo with 511

	MDRV_MACHINE_RESET(cps3_reset)


	MDRV_SCREEN_VISIBLE_AREA(0, (384*1)-1, 0, 223/*511*/) // you can see debug texts in jojo with 511

	MDRV_PALETTE_LENGTH(0x10000)

	MDRV_VIDEO_START(cps3)
	MDRV_VIDEO_UPDATE(cps3)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(CUSTOM, MASTER_CLOCK / 3)
	MDRV_SOUND_CONFIG(custom_interface)
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(1, "right", 1.0)
MACHINE_DRIVER_END


ROM_START( sfiii )
	ROM_REGION32_BE( 0x080000, REGION_USER1, 0 ) /* bios region */
	ROM_LOAD( "29f400.u2", 0x000000, 0x080000, CRC(74205250) SHA1(c3e83ace7121d32da729162662ec6b5285a31211) )

	#if LOAD_CD_CONTENT
	/* Note: These regions contains the rom data extracted from the cd.
             This is done only to make analysis easier. Once correct
             emulation is possible this region will be removed and the
             roms will be loaded into flashram from the CD by the system */
	ROM_REGION32_BE( 0x1000000, REGION_USER4, 0 ) /* cd content region */
	ROM_LOAD( "10",  0x0000000, 0x800000, CRC(e896dc27) SHA1(47623820c64b72e69417afcafaacdd2c318cde1c) )
	ROM_REGION16_BE( 0x2400000, REGION_USER5, 0 ) /* cd content region */
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
	ROM_REGION32_BE( 0x080000, REGION_USER1, 0 ) /* bios region */
	ROM_LOAD( "29f400.u2", 0x000000, 0x080000, CRC(faea0a3e) SHA1(a03cd63bcf52e4d57f7a598c8bc8e243694624ec) )

	#if LOAD_CD_CONTENT
	/* Note: These regions contains the rom data extracted from the cd.
             This is done only to make analysis easier. Once correct
             emulation is possible this region will be removed and the
             roms will be loaded into flashram from the CD by the system */
	ROM_REGION32_BE( 0x1000000, REGION_USER4, 0 ) /* cd content region */
	ROM_LOAD( "10",  0x0000000, 0x800000, CRC(682b014a) SHA1(abd5785f4b7c89584d6d1cf6fb61a77d7224f81f) )
	ROM_LOAD( "20",  0x0800000, 0x800000, CRC(38090460) SHA1(aaade89b8ccdc9154f97442ca35703ec538fe8be) )
	ROM_REGION16_BE( 0x3000000, REGION_USER5, 0 ) /* cd content region */
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
	ROM_REGION32_BE( 0x080000, REGION_USER1, 0 ) /* bios region */
	ROM_LOAD( "sf33usa.bin", 0x000000, 0x080000, CRC(ecc545c1) SHA1(e39083820aae914fd8b80c9765129bedb745ceba) )

	#if LOAD_CD_CONTENT
	/* Note: These regions contains the rom data extracted from the cd.
             This is done only to make analysis easier. Once correct
             emulation is possible this region will be removed and the
             roms will be loaded into flashram from the CD by the system */
	ROM_REGION32_BE( 0x1000000, REGION_USER4, 0 ) /* cd content region */
	ROM_LOAD( "10",  0x0000000, 0x800000, CRC(77233d39) SHA1(59c3f890fdc33a7d8dc91e5f9c4e7b7019acfb00) )
	ROM_LOAD( "20",  0x0800000, 0x800000, CRC(5ca8faba) SHA1(71c12638ae7fa38b362d68c3ccb4bb3ccd67f0e9) )
	ROM_REGION16_BE( 0x4000000, REGION_USER5, 0 ) /* cd content region */
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

ROM_START( sfiii3a )
	ROM_REGION32_BE( 0x080000, REGION_USER1, 0 ) /* bios region */
	ROM_LOAD( "sf33usa.bin", 0x000000, 0x080000, CRC(ecc545c1) SHA1(e39083820aae914fd8b80c9765129bedb745ceba) )

	#if LOAD_CD_CONTENT
	/* Note: These regions contains the rom data extracted from the cd.
             This is done only to make analysis easier. Once correct
             emulation is possible this region will be removed and the
             roms will be loaded into flashram from the CD by the system */
	ROM_REGION32_BE( 0x1000000, REGION_USER4, 0 ) /* cd content region */
	ROM_LOAD( "10",  0x0000000, 0x800000,  CRC(ba7f76b2) SHA1(6b396596dea009b34af17484919ae37eda53ec65) )
	ROM_LOAD( "20",  0x0800000, 0x800000, CRC(5ca8faba) SHA1(71c12638ae7fa38b362d68c3ccb4bb3ccd67f0e9) )
	ROM_REGION16_BE( 0x4000000, REGION_USER5, 0 ) /* cd content region */
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
	DISK_IMAGE_READONLY( "cap-33s-2", 0, SHA1(24e78b8c66fb1573ffd642ee51607f3b53ed40b7) MD5(cf63f3dbcc2653b95709133fe79c7225) )
ROM_END

ROM_START( warzard )
	ROM_REGION32_BE( 0x080000, REGION_USER1, 0 ) /* bios region */
	ROM_LOAD( "29f400.u2", 0x000000, 0x080000, CRC(f8e2f0c6) SHA1(93d6a986f44c211fff014e55681eca4d2a2774d6) )

	#if LOAD_CD_CONTENT
	/* Note: These regions contains the rom data extracted from the cd.
             This is done only to make analysis easier. Once correct
             emulation is possible this region will be removed and the
             roms will be loaded into flashram from the CD by the system */
	ROM_REGION32_BE( 0x1000000, REGION_USER4, 0 ) /* cd content region */
	ROM_LOAD( "10",  0x000000, 0x800000, CRC(68188016) SHA1(93aaac08cb5566c33aabc16457085b0a36048019) )
	ROM_REGION16_BE( 0x2400000, REGION_USER5, 0 ) /* cd content region */
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
	ROM_REGION32_BE( 0x080000, REGION_USER1, 0 ) /* bios region */
	ROM_LOAD( "29f400.u2", 0x000000, 0x080000, CRC(02778f60) SHA1(a167f9ebe030592a0cdb0c6a3c75835c6a43be4c) )

	#if LOAD_CD_CONTENT
	/* Note: These regions contains the rom data extracted from the cd.
             This is done only to make analysis easier. Once correct
             emulation is possible this region will be removed and the
             roms will be loaded into flashram from the CD by the system */
	ROM_REGION32_BE( 0x1000000, REGION_USER4, 0 ) /* cd content region */
	ROM_LOAD( "10",  0x0000000, 0x800000, CRC(e40dc123) SHA1(517e7006349b5a8fd6c30910362583f48d009355) )
	ROM_LOAD( "20",  0x0800000, 0x800000, CRC(0571e37c) SHA1(1aa28ef6ea1b606a55d0766480b3ee156f0bca5a) )
	ROM_REGION16_BE( 0x2400000, REGION_USER5, 0 ) /* cd content region */
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

ROM_START( jojoalt )
	ROM_REGION32_BE( 0x080000, REGION_USER1, 0 ) /* bios region */
	ROM_LOAD( "29f400.u2", 0x000000, 0x080000, CRC(02778f60) SHA1(a167f9ebe030592a0cdb0c6a3c75835c6a43be4c) )

	#if LOAD_CD_CONTENT
	/* Note: These regions contains the rom data extracted from the cd.
             This is done only to make analysis easier. Once correct
             emulation is possible this region will be removed and the
             roms will be loaded into flashram from the CD by the system */
	ROM_REGION32_BE( 0x1000000, REGION_USER4, 0 ) /* cd content region */
	ROM_LOAD( "10",  0x0000000, 0x800000, CRC(bc612872) SHA1(18930f2906176b54a9b8bec56f06dda3362eb418) )
	ROM_LOAD( "20",  0x0800000, 0x800000, CRC(0e1daddf) SHA1(34bb4e0fb86258095a7b20f60174453195f3735a) )
	ROM_REGION16_BE( 0x2400000, REGION_USER5, 0 ) /* cd content region */
	ROM_LOAD( "30",  0x0000000, 0x800000, CRC(1d99181b) SHA1(25c216de16cefac2d5892039ad23d07848a457e6) )
	ROM_LOAD( "31",  0x0800000, 0x800000, CRC(6889fbda) SHA1(53a51b993d319d81a604cdf70b224955eacb617e) )
	ROM_LOAD( "40",  0x1000000, 0x800000, CRC(8069f9de) SHA1(7862ee104a2e9034910dd592687b40ebe75fa9ce) )
	ROM_LOAD( "41",  0x1800000, 0x800000, CRC(9c426823) SHA1(1839dccc7943a44063e8cb2376cd566b24e8b797) )
	ROM_LOAD( "50",  0x2000000, 0x400000, CRC(1c749cc7) SHA1(23df741360476d8035c68247e645278fbab53b59) )
	/* 92,93 are bmp images, not extracted */
	#endif

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "cap-jjk-160", 0, SHA1(74fb14d838d98c3e10baa08e6f7b2464d840dcf0) MD5(93cc16f11a88c8f5268cb96baebc0a13) )
ROM_END


ROM_START( jojoba )
	ROM_REGION32_BE( 0x080000, REGION_USER1, 0 ) /* bios region */
	// this was from a version which doesn't require the cd to run
	ROM_LOAD( "29f400.u2", 0x000000, 0x080000, CRC(4dab19f5) SHA1(ba07190e7662937fc267f07285c51e99a45c061e) )

	ROM_REGION32_BE( 0x080000, REGION_USER2, ROMREGION_ERASEFF ) /* bios region */


	#if LOAD_CD_CONTENT
	/* Note: These regions contains the rom data extracted from the cd.
             This is done only to make analysis easier. Once correct
             emulation is possible this region will be removed and the
             roms will be loaded into flashram from the CD by the system */
	ROM_REGION32_BE( 0x1000000, REGION_USER4, 0 ) /* cd content region */
	ROM_LOAD( "10",  0x000000, 0x800000, CRC(6e2490f6) SHA1(75cbf1e39ad6362a21c937c827e492d927b7cf39) )
	ROM_LOAD( "20",  0x800000, 0x800000, CRC(1293892b) SHA1(b1beafac1a9c4b6d0640658af8a3eb359e76eb25) )

	ROM_REGION16_BE( 0x3000000, REGION_USER5, 0 ) /* cd content region */
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
	ROM_REGION32_BE( 0x080000, REGION_USER1, 0 ) /* bios region */
	ROM_LOAD( "jojobacart.bin",  0x000000, 0x080000,  CRC(3085478c) SHA1(055eab1fc42816f370a44b17fd7e87ffcb10e8b7) )

	#if LOAD_CD_CONTENT
	/* Note: These regions contains the rom data extracted from the cd.
             This is done only to make analysis easier. Once correct
             emulation is possible this region will be removed and the
             roms will be loaded into flashram from the CD by the system */
	ROM_REGION32_BE( 0x1000000, REGION_USER4, 0 ) /* cd content region */
	ROM_LOAD( "10",  0x000000, 0x800000, CRC(6e2490f6) SHA1(75cbf1e39ad6362a21c937c827e492d927b7cf39) )
	ROM_LOAD( "20",  0x800000, 0x800000, CRC(1293892b) SHA1(b1beafac1a9c4b6d0640658af8a3eb359e76eb25) )
	ROM_REGION16_BE( 0x3000000, REGION_USER5, 0 ) /* cd content region */
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

void precopy_to_flash(void)
{
	int i;
	for (i=0;i<0x800000;i+=4)
	{
		UINT32* romdata = (UINT32*)decrypted_gamerom;
		UINT32 data;
		UINT8* ptr1 = intelflash_getmemptr(0);
		UINT8* ptr2 = intelflash_getmemptr(1);
		UINT8* ptr3 = intelflash_getmemptr(2);
		UINT8* ptr4 = intelflash_getmemptr(3);

		if (cps3_isSpecial) romdata = (UINT32*)memory_region(REGION_USER4);
		data = romdata[i/4];

		ptr1[i/4] = (data & 0xff000000)>>24;
		ptr2[i/4] = (data & 0x00ff0000)>>16;
		ptr3[i/4] = (data & 0x0000ff00)>>8;
		ptr4[i/4] = (data & 0x000000ff)>>0;
	}

	for (i=0;i<0x800000;i+=4)
	{
		UINT32* romdata = (UINT32*)decrypted_gamerom;
		UINT32 data;
		UINT8* ptr1 = intelflash_getmemptr(4);
		UINT8* ptr2 = intelflash_getmemptr(5);
		UINT8* ptr3 = intelflash_getmemptr(6);
		UINT8* ptr4 = intelflash_getmemptr(7);

		if (cps3_isSpecial) romdata = (UINT32*)memory_region(REGION_USER4);

		data = romdata[(0x800000+i)/4];

		ptr1[i/4] = (data & 0xff000000)>>24;
		ptr2[i/4] = (data & 0x00ff0000)>>16;
		ptr3[i/4] = (data & 0x0000ff00)>>8;
		ptr4[i/4] = (data & 0x000000ff)>>0;
	}

	for (i=0;i<0x800000;i+=4)
	{
		UINT32* romdata = (UINT32*)memory_region(REGION_USER5);
		UINT32 data;
		UINT8* ptr1 = intelflash_getmemptr(8);
		UINT8* ptr2 = intelflash_getmemptr(9);

		data = romdata[(0x800000+i)/4];

		ptr1[i/4] = (data & 0xff000000)>>24;
		ptr2[i/4] = (data & 0x00ff0000)>>16;
		ptr1[i/4] = (data & 0x0000ff00)>>8;
		ptr2[i/4] = (data & 0x000000ff)>>0;
	}

}



DRIVER_INIT (jojoba)
{
	UINT32 *rom =  (UINT32*)memory_region ( REGION_USER1 );
	rom[0x1fec8/4]^=0x00000073; // region hack

	driver_init_cps3crpt(machine);

	precopy_to_flash();
}

DRIVER_INIT( warzard )
{
	/* where is NOCD? */
	/* region 04 usa */
	UINT32 *rom =  (UINT32*)memory_region ( REGION_USER1 );
	rom[0x1fed8/4]^=0x00000001; // clear region to 0 (invalid)

	rom[0x1fed8/4]^=0x00000008; // region 8 - ASIA NO CD
//  rom[0x1fedc/4]^=0x01000000; // region 8 - ASIA NO CD

	driver_init_cps3crpt(machine);

	precopy_to_flash();

}

DRIVER_INIT( sfiii3 )
{
	UINT32 *rom =  (UINT32*)memory_region ( REGION_USER1 );

	rom[0x1fec8/4]^=0x00000070; // region hack
	rom[0x1fecc/4]^=0x01000000; //  nocd hack

	driver_init_cps3crpt(machine);

	precopy_to_flash();

}

DRIVER_INIT( sfiii2 )
{
//  UINT32 *rom =  (UINT32*)memory_region ( REGION_USER1 );
//  rom[0x1fec8/4]^=0x00000002; // region hack
//  rom[0x1fecc/4]^=0x01000000; //  nocd hack

	driver_init_cps3crpt(machine);

	precopy_to_flash();
}

DRIVER_INIT( jojo )
{
	// jojo test mode is drawn with 16x16 japanese tiles when set to japan
	// theres aren't emulated yet.

	UINT32 *rom =  (UINT32*)memory_region ( REGION_USER1 );
	rom[0x1fec8/4]^=0x00000072; // region hack // enable dev mode (hold button 1 when entering test mode
	//  rom[0x1fec8/4]^=0x00000004; // region hack
//  rom[0x1fecc/4]^=0x01000000; //  nocd hack

	driver_init_cps3crpt(machine);

	precopy_to_flash();
}

GAME( 1997, sfiii,   0,        cps3, cps3, sfiii3, ROT0,   "Capcom", "Street Fighter III: New Generation (Japan, 970204)", GAME_NOT_WORKING )
GAME( 1997, sfiii2,  0,        cps3, cps3, sfiii2, ROT0,   "Capcom", "Street Fighter III 2nd Impact: Giant Attack (Japan, 970930)", GAME_NOT_WORKING )
GAME( 1999, sfiii3,  0,        cps3, cps3, sfiii3,   ROT0,   "Capcom", "Street Fighter III 3rd Strike: Fight for the Future (USA, 990512)", GAME_NOT_WORKING )
GAME( 1999, sfiii3a, sfiii3,   cps3, cps3, sfiii3,   ROT0,   "Capcom", "Street Fighter III 3rd Strike: Fight for the Future (USA, 990608)", GAME_NOT_WORKING )

GAME( 1998, jojo,    0,        cps3, cps3, jojo,     ROT0,   "Capcom", "JoJo's Venture / JoJo no Kimyouna Bouken (Japan, 981202)", GAME_NOT_WORKING )
GAME( 1998, jojoalt, jojo,     cps3, cps3, jojo,     ROT0,   "Capcom", "JoJo's Venture / JoJo no Kimyouna Bouken (Japan, 990108)", GAME_NOT_WORKING )

GAME( 1999, jojoba,  0,        cps3, cps3, jojoba,   ROT0,   "Capcom", "JoJo's Bizarre Adventure: Heritage for the Future / JoJo no Kimyouna Bouken: Miraie no Isan (Japan, 990913, NCD)", GAME_NOT_WORKING )
GAME( 1999, jojobaa, jojoba,   cps3, cps3, cps3crpt, ROT0,   "Capcom", "JoJo's Bizarre Adventure: Heritage for the Future / JoJo no Kimyouna Bouken: Miraie no Isan (Japan, 990913)", GAME_NOT_WORKING )

GAME( 1996, warzard, 0,        cps3, cps3, warzard,  ROT0,   "Capcom", "Warzard / Red Earth (Japan, 96/11/21)", GAME_NOT_WORKING )

/***************************************************************************

    Tiger Game.com cartridges

***************************************************************************/

#include "driver.h"
#include "softlist.h"
#include "devices/cartslot.h"


#define GAMECOM_ROM_LOAD( set, name, offset, length, hash )	\
SOFTWARE_START( set ) \
	ROM_REGION( 0x200000, CARTRIDGE_REGION_ROM, ROMREGION_ERASEFF ) \
	ROM_LOAD(name, offset, length, hash) \
SOFTWARE_END


GAMECOM_ROM_LOAD( batmanr,  "batman & robin (1997)(tiger electronics).bin",                                              0x000000,  0x200000,	 CRC(b336e530) SHA1(066d5cddce9729044ad598ad3cc3489a834f5197) )
GAMECOM_ROM_LOAD( centiped, "centipede (1999)(hasbro - tiger electronics).bin",                                          0x000000,  0x40000,	 CRC(1a041ff0) SHA1(b3d50e0cca6e2a539fba5883854aaf26424ab382) )
GAMECOM_ROM_LOAD( dukenuke, "duke nukem 3d (1997)(tiger electronics).bin",                                               0x000000,  0x200000,	 CRC(884568d1) SHA1(9aaa1cf3b12f14201079dfbdd4c17fe868972de6) )
GAMECOM_ROM_LOAD( fightmix, "fighters megamix (1998)(sega).bin",                                                         0x000000,  0x200000,	 CRC(061e943f) SHA1(c165e5f4b61341ae0e8a4d141b28e18108942326) )
GAMECOM_ROM_LOAD( frogger,  "frogger (1999)(hasbro - tiger electronics).bin",                                            0x000000,  0x100000,	 CRC(10e4c5cd) SHA1(8224d3e71a3da2f4029bee8b62b4a0c108c054e4) )
GAMECOM_ROM_LOAD( henry,    "henry (1997)(tiger electronics).bin",                                                       0x000000,  0x100000,	 CRC(75d3c8d8) SHA1(b72a03af1dec0c621f9eda89ded3845724bb34ac) )
GAMECOM_ROM_LOAD( indy500,  "indy 500 (1997)(tiger electronics).bin",                                                    0x000000,  0x100000,	 CRC(845ed281) SHA1(117bfa1c831fbcf4a1d507cb70b6e0636c29bbd3) )
GAMECOM_ROM_LOAD( jeopardy, "jeopardy! (1997)(tiger electronics).bin",                                                   0x000000,  0x100000,	 CRC(8c512912) SHA1(c60ac5e59c3cc092506bc47adb60b40a9415bc97) )
GAMECOM_ROM_LOAD( lightout,  "lights out (1997)(tiger electronics).bin",                                                 0x000000,  0x8000,	 CRC(496efa62) SHA1(1d91b616caa2d35f53ec5680aada8f75e2f0ac43) )
GAMECOM_ROM_LOAD( lostwrld, "lost world, the - jurassic park (1997)(tiger electronics).bin",                             0x000000,  0x200000,	 CRC(b422c5f8) SHA1(648badcac6c386d7ff40f917e34bd309e737c027) )
GAMECOM_ROM_LOAD( monopoly, "monopoly (1999)(tiger electronics).bin",                                                    0x000000,  0x200000,	 CRC(d6a65a07) SHA1(8057144f6995eafff6d893383114b48f4c77abc3) )
GAMECOM_ROM_LOAD( mktrilgy, "mortal kombat trilogy (1997)(midway).bin",                                                  0x000000,  0x1c0000,	 CRC(72893a77) SHA1(d8a403a180f4a397098c8a598dd6cd5b45ef377a) )
GAMECOM_ROM_LOAD( quizwiz,  "quiz wiz - cyber trivia (1997)(tiger electronics).bin",                                     0x000000,  0x100000,	 CRC(0e018f6a) SHA1(c9c4435bdcea2b9ddaa2768f28f286abd382608d) )
GAMECOM_ROM_LOAD( re2,      "resident evil 2 (1998)(capcom).bin",                                                        0x000000,  0x1c0000,	 CRC(4f09457d) SHA1(1d4d4ff13c8f7c228f30bddab174d4781b370ec9) )
GAMECOM_ROM_LOAD( scrabble, "scrabble (1999)(hasbro - tiger electronics).bin",                                           0x000000,  0x200000,	 CRC(4d4ad909) SHA1(2735d59964fe7c1d43bd260df86b9dfbc1bfd736) )
GAMECOM_ROM_LOAD( sonicjam, "sonic jam (1998)(sega).bin",                                                                0x000000,  0x200000,	 CRC(8b4a63f9) SHA1(393a98138c8825ed61990e1b15e1967f87068ac6) )
GAMECOM_ROM_LOAD( tcasino,  "tiger casino (1998)(tiger electronics).bin",                                                0x000000,  0x1c0000,	 CRC(ba698142) SHA1(32ce1be96f7f367b8a0adb8cc61a8cca7d6b79dd) )
GAMECOM_ROM_LOAD( wheelfor, "wheel of fortune (1997)(tiger electronics).bin",                                            0x000000,  0x80000,	 CRC(048347a0) SHA1(298c2b87754dd93481feea1d9e01585a1e269770) )
GAMECOM_ROM_LOAD( wheelfo2, "wheel of fortune 2 (1998)(tiger electronics).bin",                                          0x000000,  0x80000,	 CRC(b91e2e32) SHA1(30ddd349b8bdc21ef7ada4772ab1f3a9318ab0c8) )
GAMECOM_ROM_LOAD( arcadecl, "williams arcade classics (1997)(tiger electronics).bin",                                    0x000000,  0x100000,	 CRC(f894065d) SHA1(5e2164fa76546b2795af7b04a766e31784b82eca) )

SOFTWARE_LIST_START( gamecom_cart )
	SOFTWARE( batmanr,  0       , 1997, "Tiger Electronics", "Batman & Robin (Tiger Electronics)", 0, 0 )
	SOFTWARE( centiped, 0       , 1999, "<unknown>", "Centipede (Hasbro - Tiger Electronics)", 0, 0 )
	SOFTWARE( dukenuke, 0       , 1997, "Tiger Electronics", "Duke Nukem 3D (Tiger Electronics)", 0, 0 )
	SOFTWARE( fightmix, 0       , 1998, "<unknown>", "Fighters Megamix (Sega)", 0, 0 )
	SOFTWARE( frogger,  0       , 1999, "<unknown>", "Frogger (Hasbro - Tiger Electronics)", 0, 0 )
	SOFTWARE( henry,    0       , 1997, "Tiger Electronics", "Henry (Tiger Electronics)", 0, 0 )
	SOFTWARE( indy500,  0       , 1997, "Tiger Electronics", "Indy 500 (Tiger Electronics)", 0, 0 )
	SOFTWARE( jeopardy, 0       , 1997, "Tiger Electronics", "Jeopardy! (Tiger Electronics)", 0, 0 )
	SOFTWARE( lightout, 0       , 1997, "<unknown>", "Lights Out (Tiger Electronics)", 0, 0 )
	SOFTWARE( lostwrld, 0       , 1997, "<unknown>", "Lost World, The - Jurassic Park (Tiger Electronics)", 0, 0 )
	SOFTWARE( monopoly, 0       , 1999, "<unknown>", "Monopoly (Tiger Electronics)", 0, 0 )
	SOFTWARE( mktrilgy, 0       , 1997, "<unknown>", "Mortal Kombat Trilogy (Midway)", 0, 0 )
	SOFTWARE( quizwiz,  0       , 1997, "<unknown>", "Quiz Wiz - Cyber Trivia (Tiger Electronics)", 0, 0 )
	SOFTWARE( re2,      0       , 1998, "<unknown>", "Resident Evil 2 (Capcom)", 0, 0 )
	SOFTWARE( scrabble, 0       , 1999, "<unknown>", "Scrabble (Hasbro - Tiger Electronics)", 0, 0 )
	SOFTWARE( sonicjam, 0       , 1998, "<unknown>", "Sonic Jam (Sega)", 0, 0 )
	SOFTWARE( tcasino,  0       , 1998, "<unknown>", "Tiger Casino (Tiger Electronics)", 0, 0 )
	SOFTWARE( wheelfor, 0       , 1997, "<unknown>", "Wheel of Fortune (Tiger Electronics)", 0, 0 )
	SOFTWARE( wheelfo2, 0       , 1998, "<unknown>", "Wheel of Fortune 2 (Tiger Electronics)", 0, 0 )
	SOFTWARE( arcadecl, 0       , 1997, "<unknown>", "Williams Arcade Classics (Tiger Electronics)", 0, 0 )
SOFTWARE_LIST_END


SOFTWARE_LIST( gamecom_cart, "Tiger Game.com cartridges" )

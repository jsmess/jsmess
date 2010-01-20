/***************************************************************************

    V-Tech Creativision cartridges

***************************************************************************/

#include "emu.h"
#include "softlist.h"
#include "devices/cartslot.h"


#define CRVISION_ROM_LOAD( set, name, offset, length, hash )	\
SOFTWARE_START( set ) \
	ROM_REGION(0x10000, CARTRIDGE_REGION_ROM, 0 ) \
	ROM_LOAD(name, offset, length, hash) \
SOFTWARE_END


/*
A couple of comments about the 6k/8k images (thanks to Luca Antignano):
- game code occupies 6k, but there can be different chips inside the actual carts:
  1x8k eprom, 2x4k eproms or 1x2k + 1x4k eproms. In the 1x8k case, the console 
  actually accesses only the first 2k and the final 4k.
There have been found carts where the remaining part is empty, and carts where the lower
2k are repeated (see the Alt dumps below), but the dumps are basically the same and
a Creativision would not see the difference between them.
*/


/* FIXME: cart region has to be mirrored into maincpu in various ways depending on the cart size: check drivers/crvision.c! */

CRVISION_ROM_LOAD( airsea,   "airsea.bin",            0x000000,  0x1000,	 CRC(81a9257d) SHA1(1fbc52f335c0d8bb96578a6ba764f5631c41fd36) )
CRVISION_ROM_LOAD( astropin, "astropib.bin",          0x000000,  0x2000,	 CRC(d03c0603) SHA1(e7cb096d4d16fd8193f7e39c2f73bdf0930c9654) )
CRVISION_ROM_LOAD( autochas, "autochas.bin",          0x000000,  0x2000,	 CRC(bd091ee0) SHA1(369dc9aa55dd2c09376be840f8ebeca450db8b9c) )
CRVISION_ROM_LOAD( basic8ab, "basic82a.bin",          0x000000,  0x3000,	 CRC(4aee923e) SHA1(7523b938d251315ffc54b1cca166790beba8972a) )
CRVISION_ROM_LOAD( basic8bb, "basic82b.bin",          0x000000,  0x3000,	 CRC(1849efd0) SHA1(96027c0b250e252625143211f3ff6ac4e6e0aeff) )
CRVISION_ROM_LOAD( basic8bi, "basic83.bin",           0x000000,  0x3000,	 CRC(10409a1d) SHA1(5226bc0fa08f1046179572ed602ce157959bf92f) )
CRVISION_ROM_LOAD( basicram, "basicram.bin",          0x000000,  0x3000,	 CRC(b8df3b18) SHA1(d878f23143e3362d5065550c2e1bd80d8f7c25c0) )
CRVISION_ROM_LOAD( chopper,  "chopper.bin",           0x000000,  0x4800,	 CRC(afb482ae) SHA1(43dc37755306ac824492e98c9d0e25379ac69672) )
CRVISION_ROM_LOAD( crazych,  "crazych.bin",           0x000000,  0x1000,	 CRC(b1b5bfe5) SHA1(c0eed370267644d142a42de6201f7ac6d275104d) )
CRVISION_ROM_LOAD( crazypuc, "crazypuc.bin",          0x000000,  0x1000,	 CRC(c673be37) SHA1(0762ba98e9a08b7e6063e2a54734becd83df6eb3) )
CRVISION_ROM_LOAD( deepsea1, "deeps6k.bin",           0x000000,  0x1800,	 CRC(28a30e3a) SHA1(30018a2f7328f747ce21e12c4b0da8dda6334e5f) )
CRVISION_ROM_LOAD( deepsea,  "deeps8k.bin",           0x000000,  0x2000,	 CRC(2e0ddd86) SHA1(d25f82aab473d4c7850ab626ae92d1eac69dbb32) )
CRVISION_ROM_LOAD( locomot1, "locom10k.bin",          0x000000,  0x2800,	 CRC(81b552ef) SHA1(4b383260f8b0db5abe5a0c595f52d3dab55a6fb1) )
CRVISION_ROM_LOAD( locomot,  "locom12k.bin",          0x000000,  0x3000,	 CRC(3618b415) SHA1(4aa0db7b236ac512d777cfc811b58d59f9095ab1) )
CRVISION_ROM_LOAD( mousepuz, "mousepuz.bin",          0x000000,  0x2000,	 CRC(e954c46b) SHA1(c3aa8077756a6101009645711687edd59bf90a98) )
CRVISION_ROM_LOAD( musicmak, "musicmak.bin",          0x000000,  0x3000,	 CRC(f8383d33) SHA1(d1eb8c679310b8988ac4d018853b979613508b0e) )
CRVISION_ROM_LOAD( planetd,  "planet6k.bin",          0x000000,  0x1800,	 CRC(b8cb39f7) SHA1(a89c96ff084f2eb220fc626aaae479d4172b769b) )
CRVISION_ROM_LOAD( planetd1, "planet8k.bin",          0x000000,  0x2000,	 CRC(4457c7b3) SHA1(0be7935db55ecf3e70b96e071ecbc489bd22dc73) )
CRVISION_ROM_LOAD( planetd2, "planet defender.bin",   0x000000,  0x2000,	 CRC(4b463c18) SHA1(1fd80a3921f2487d9eee2b6d8bb3955d22857d0e) )
CRVISION_ROM_LOAD( policej,  "policej.bin",           0x000000,  0x2000,	 CRC(db3d50b2) SHA1(0fe6d15c973c73b53c33945e213bc2671ff5d7df) )
CRVISION_ROM_LOAD( soccer,   "soccer.bin",            0x000000,  0x3000,	 CRC(90a7438d) SHA1(d7526d3bcf156a38d35ae439080eb8913d56e51e) )
CRVISION_ROM_LOAD( sonicinv, "sonicinv.bin",          0x000000,  0x1000,	 CRC(767a1f38) SHA1(d77f49bfa951ce0ba505ad4f05f3d0adb697811f) )
CRVISION_ROM_LOAD( stoneage, "stoneage.bin",          0x000000,  0x2000,	 CRC(74365e94) SHA1(dafe57b0ea5da7431bd5b0393bae7cf256c9a562) )
CRVISION_ROM_LOAD( tankatk1, "tankat6k.bin",          0x000000,  0x1800,	 CRC(7e99fb44) SHA1(67b8da0b96ded6b74a78f0f7da21aede9244d379) )
CRVISION_ROM_LOAD( tankatk,  "tankat8k.bin",          0x000000,  0x2000,	 CRC(2621ffca) SHA1(f3c69a848f1246faf6f9558c0badddd3db626e5f) )
CRVISION_ROM_LOAD( tennis1,  "tens6k.bin",            0x000000,  0x1800,	 CRC(1c5ea624) SHA1(88b9fbee783ed966c8c03614779847e0cc8c6692) )
CRVISION_ROM_LOAD( tennis,   "tens8k.bin",            0x000000,  0x2000,	 CRC(8bed8745) SHA1(690a4dcb412e517b172ec5b44c86863ef63b1246) )
CRVISION_ROM_LOAD( tenniscs, "tenscs8k.bin",          0x000000,  0x2000,	 CRC(098d1cbb) SHA1(7ea0c5aa6072e2c9eab711f278fd939986a92602) )
CRVISION_ROM_LOAD( tennisd1, "tensds6k.bin",          0x000000,  0x1800,	 CRC(2e65cfa9) SHA1(16e5d557449a11656d4821051a8aa61dabcb688e) )
CRVISION_ROM_LOAD( tennisd,  "tensds8k.bin",          0x000000,  0x2000,	 CRC(c914c092) SHA1(7b64efddb590332c192f6c4a6accce85a6d402c3) )

// astrop1 differs from astropin only in the 1st byte. this image came from FunnyMu < 0.43, and it might be a bad dump (astropin has been confirmed good)
CRVISION_ROM_LOAD( astropin1, "astro pinball.bin",     0x000000,  0x2000,	 CRC(cf7bdfc2) SHA1(38bcc856d604567dc6453205a8f858bdf2000330) )

// hapmon is a hex editor created by FunnyMu's author before the emulator and perfectly working on the real machine
CRVISION_ROM_LOAD( hapmon,   "hapmon.bin",            0x000000,  0x1000,	 CRC(15d07b96) SHA1(89359dca952bc90644dfd7f546c184a8165faa14) )



// The carts were universal (Japanese dumps turned out to be the same as Asian, Australian and European ones).
SOFTWARE_LIST_START( crvision_cart )
	SOFTWARE( airsea,    0,        1981, "<unknown>", "Air/Sea Attack", 0, 0 )	/* Id: 8008 */
	SOFTWARE( astropin,  0,        1982, "<unknown>", "Astro Pinball", 0, 0 )	/* Id: 8014 */
	SOFTWARE( astropin1, astropin, 1982, "<unknown>", "Astro Pinball (Bad?)", 0, 0 )
	SOFTWARE( autochas,  0,        1981, "<unknown>", "Auto Chase", 0, 0 )	/* Id: 8006 */
	SOFTWARE( basic8ab,  0,        1982, "<unknown>", "CreatiVision Basic (Alt Rev. 2)", 0, 0 )	/* Id: 8011 */
	SOFTWARE( basic8bb,  0,        1982, "<unknown>", "CreatiVision Basic (Alt Rev. 1)", 0, 0 )	/* Id: 8011 */
	SOFTWARE( basic8bi,  0,        1983, "<unknown>", "CreatiVision Basic", 0, 0 )	/* Id: 8011 */
	SOFTWARE( basicram,  0,        1983, "<unknown>", "Rameses Basic ~ FunVision Basic", 0, 0 )	/* Id: RAM V1 - Y-2605 */
	SOFTWARE( chopper,   0,        1983, "<unknown>", "Chopper Rescue", 0, 0 )	/* Id: 8021 */
	SOFTWARE( crazych,   0,        1981, "<unknown>", "Crazy Chicky", 0, 0 )	/* Id: 8001 */
	SOFTWARE( crazypuc,  0,        1981, "<unknown>", "Crazy Pucker ~ Crazy Moonie ~ Crazy Chewy", 0, 0 )	/* Id: 8001 */
	SOFTWARE( deepsea1,  deepsea,  1982, "<unknown>", "Deep Sea Adventure (6k Cart)", 0, 0 )	/* Id: 8013 */
	SOFTWARE( deepsea,   0,        1982, "<unknown>", "Deep Sea Adventure (8k Cart)", 0, 0 )	/* Id: 8013 */
	SOFTWARE( locomot1,  locomot,  1983, "<unknown>", "Locomotive (10k cart)", 0, 0 )	/* Id: 8020 */
	SOFTWARE( locomot,   0,        1983, "<unknown>", "Locomotive (12k cart)", 0, 0 )	/* Id: 8020 */
	SOFTWARE( mousepuz,  0,        1982, "<unknown>", "Mouse Puzzle", 0, 0 )	/* Id: 8015 */
	SOFTWARE( musicmak,  0,        1983, "<unknown>", "Music Maker", 0, 0 )	/* Id: 8016 */
	SOFTWARE( planetd,   0,        1981, "<unknown>", "Planet Defender (6k Cart)", 0, 0 )	/* Id: 8005 */
	SOFTWARE( planetd1,  planetd,  1981, "<unknown>", "Planet Defender (8k Cart)", 0, 0 )	/* Id: 8005 */
	SOFTWARE( planetd2,  planetd,  1981, "<unknown>", "Planet Defender (8k Cart, Alt)", 0, 0 )
	SOFTWARE( policej,   0,        1982, "<unknown>", "Police Jump", 0, 0 )	/* Id: 8009 */
	SOFTWARE( soccer,    0,        1983, "<unknown>", "Soccer", 0, 0 )	/* Id: 8017 */
	SOFTWARE( sonicinv,  0,        1981, "<unknown>", "Sonic Invader", 0, 0 )	/* Id: 8003 */
	SOFTWARE( stoneage,  0,        1984, "<unknown>", "Stone Age", 0, 0 )	/* Id: 80?? */
	SOFTWARE( tankatk1,  tankatk,  1981, "<unknown>", "Tank Attack (6k Cart)", 0, 0 )	/* Id: 8002 */
	SOFTWARE( tankatk,   0,        1981, "<unknown>", "Tank Attack (8k Cart)", 0, 0 )	/* Id: 8002 */
	SOFTWARE( tennis1,   tennis,   1981, "<unknown>", "Tennis (Wimbledon, 6k Cart)", 0, 0 )	/* Id: 8004 */
	SOFTWARE( tennis,    0,        1981, "<unknown>", "Tennis (Wimbledon, 8k Cart)", 0, 0 )	/* Id: 8004 */
	SOFTWARE( tenniscs,  tennis,   1981, "<unknown>", "Tennis (Coca Cola / Sprite, 8k cart)", 0, 0 )	/* Id: 8004 */
	SOFTWARE( tennisd1,  tennis,   19??, "<unknown>", "Tennis (Dick Smith, 6k Cart)", 0, 0 )	/* Id: Y-1620 */
	SOFTWARE( tennisd,   tennis,   19??, "<unknown>", "Tennis (Dick Smith, 8k Cart)", 0, 0 )	/* Id: Y-1620 */

	SOFTWARE( hapmon,    0,        19??, "<unknown>", "Hapmon", 0, 0 )
SOFTWARE_LIST_END


SOFTWARE_LIST( crvision_cart, "V-Tech Creativision cartridges" )

#if 0

// These contain the same data as chopper, but in a different order (which is not handled properly by emulators). I'm listing these here to document checksums.
CRVISION_ROM_LOAD( chopper2, "chopper rescue (alt).rom",       0x000000,  0x4800,	 CRC(48e7e8b8) SHA1(037088f15830fdd43081edddc5d683318d67ee01) )
CRVISION_ROM_LOAD( chopper1, "chopper rescue.rom",             0x000000,  0x4800,	 CRC(14179570) SHA1(fde339c8dbd44b3332e888cd24d3d492cf066b94) )

	SOFTWARE( chopper2, chopper,  19??, "<unknown>", "Chopper Rescue (Alt 2)", 0, 0 )
	SOFTWARE( chopper1, chopper,  19??, "<unknown>", "Chopper Rescue (Alt)", 0, 0 )

#endif

/*****************************************************************************
 *
 * includes/vc1541.h
 *
 * Commodore VC1541 floppy disk drive
 *
 ****************************************************************************/

#ifndef VC1541_H_
#define VC1541_H_

/*----------- defined in machine/vc1541.c -----------*/

/* we currently have preliminary support for 1541 & 1551 only */
enum { 
	type_1541 = 0,
	type_1541ii,
	type_1551,
	type_1570,
	type_1571,
	type_1571cr,
	type_1581,
	type_2031,
	type_2040,
	type_3040,
	type_4040,
	type_1001,
	type_8050,
	type_8250,
};

enum { 
	format_d64 = 0,			/* 1541 image, 35 tracks */
	format_d64_err,			/* 1541 image, 35 tracks + error table */
	format_d64_40t,			/* 1541 image, 40 tracks */
	format_d64_40t_err,		/* 1541 image, 40 tracks + error table */
	format_d67,				/* 2040 image, 35 tracks DOS1 format */
	format_d71,				/* 1571 image, 70 tracks */
	format_d71_err,			/* 1571 image, 70 tracks + error table */
	format_d81,				/* 1581 image, 80 tracks */
	format_d80,				/* 8050 image, 77 tracks */
	format_d82,				/* 8250 image, 154 tracks (read as 77?) */
	format_g64,				/* 1541 image in GCR format */
};


void cbm_drive_config(running_machine *machine, int type, int id, int mode, const char *cputag, int devicenr);
void c1551_config(running_machine *machine, const char *cputag);
void cbm_drive_reset(running_machine *machine);
void c1551_drive_reset(void);

void vc1541_device_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info);
void c1551_device_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info);
void c1571_device_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info);
void c1581_device_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info);
/* the ones below are still not used in any drivers */
void c2031_device_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info);
void c2040_device_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info);
void c8050_device_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info);
void c8250_device_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info);


MACHINE_DRIVER_EXTERN( cpu_vc1540 );
MACHINE_DRIVER_EXTERN( cpu_vc1541 );
MACHINE_DRIVER_EXTERN( cpu_c1571 );
MACHINE_DRIVER_EXTERN( cpu_c1571cr );
MACHINE_DRIVER_EXTERN( cpu_c1581 );
MACHINE_DRIVER_EXTERN( cpu_c2031 );
MACHINE_DRIVER_EXTERN( cpu_c1551 );

// currently not used (hacked drive firmware with more RAM)
MACHINE_DRIVER_EXTERN( cpu_dolphin );


/* IEC interface for c16 with c1551 */

/* To be passed directly to the drivers */
WRITE8_DEVICE_HANDLER( c1551x_write_data );
READ8_DEVICE_HANDLER( c1551x_read_data );
WRITE8_DEVICE_HANDLER( c1551x_write_handshake );
READ8_DEVICE_HANDLER( c1551x_read_handshake );
READ8_DEVICE_HANDLER( c1551x_read_status );


/* Floppy drive ROMs */

/****************************************************************************

Info from zimmers.net

1551 Parts

Location       Part Number     Description
U4             318008-01       23128 ROM DOS 2.6 C000-FFFF
U5                             6116 2K x 8 Static RAM
U6                             HD61J215P 42 pin Gate Array
U7             251829-01       64H157 20 pin Gate Array
U2                             6510T CPU
U3                             6525A TPI
U11            251853-01       R/W Hybrid

Interface Cartridge:
U2                             6523 Tri-Port Interface
U1             251641-03       82S100 PLA

****************************************************************************/

#define C1551_ROM( cpu )	\
	ROM_REGION( 0x10000, cpu, 0 )	\
	ROM_LOAD( "318008-01.u4", 0xc000, 0x4000, CRC(6d16d024) SHA1(fae3c788ad9a6cc2dbdfbcf6c0264b2ca921d55e) )



/****************************************************************************

Info from zimmers.net

1540/1541/1541A/SX-64 Parts

Location       Part Number    Description
                              2016 2K x 8 bit Static RAM (short board)
UA2-UB3                       2114 (4) 1K x 4 bit Static RAM (long board)
               325302-01      2364-130 ROM DOS 2.6 C000-DFFF
               325303-01      2364-131 ROM DOS 2.6 (1540) E000-FFFF
               901229-01      2364-173 ROM DOS 2.6 (1541) E000-FFFF
               901229-03      2364-197 ROM DOS 2.6 rev. 1 E000-FFFF
               901229-05      2364     ROM DOS 2.6 rev. 2 E000-FFFF
               325572-01      64H105 40 pin Gate Array (short board)
                              6502 CPU
                              6522 (2) VIA
drive                         Alps DFB111M25A
drive                         Alps FDM2111-B2
drive                         Newtronics D500

1541B/1541C Parts

Location       Part Number    Description
UA3                           2016 2K x 8 bit Static RAM
UA2            251968-01      23128 ROM DOS 2.6 rev. 3 C000-FFFF
UA2            251968-01      27128 EPROM DOS 2.6 rev. 3 C000-FFFF
UC4            251828-02      64H156 42 pin Gate Array
UC5            251829-01      64H157 20 pin Gate Array
UC2                           6502 CPU
UC1, UC3                      6522 (2) CIA
UD1          * 251853-01      R/W Hybrid
UD1          * 251853-02      R/W Hybrid
drive                         Newtronics D500
  * Not interchangeable.

1541-II Parts

Location       Part Number    Description
U5                            2016-15 2K x 8 bit Static RAM
U12                           SONY CX20185 R/W Amp.
U4             251968-03      16K ROM DOS 2.6 rev. 4 C000-FFFF
U10            251828-01      64H156 40 pin Gate Array
U3                            6502A CPU
U6, U8                        6522 (2) CIA
drive                         Chinon FZ-501M REV A
drive                         Digital System DS-50F
drive                         Newtronics D500
drive                         Safronic DS-50F



Note: according to additional docs at zimmers.net, ROMs in these drive 
were interchangeable: you could burn later revisions on EEPROM and put 
them in older drives and they would work perfectly.

****************************************************************************/

// I'm not 100% sure about UA2/UB3 locations used below for ROM names
// I need a picture to confirm these, since the part listing above is not really clear (without a pic of the board)
#define VC1540_ROM( cpu )	\
	ROM_REGION( 0x10000, cpu, 0 )	\
	ROM_LOAD( "325302-01.ua2", 0xc000, 0x2000, CRC(29ae9752) SHA1(8e0547430135ba462525c224e76356bd3d430f11) )	\
	ROM_LOAD( "325303-01.ub3", 0xe000, 0x2000, CRC(10b39158) SHA1(56dfe79b26f50af4e83fd9604857756d196516b9) )

/*
    rev. 01 - It is believed to be the first revision of the 1541 firmware. The service manual says that this ROM 
        is for North America and Japan only.
    rev. 02 - Second version of the 1541 firmware. The service manual says that this ROM was not available in North 
        America (Japan only?).
    rev. 03 - It is said to be the first version that is usable in Europe (in the service manual?).
    rev. 05 - From an old-style 1541 with short board.
    rev. 06AA - From an old-style 1541 with short board.
*/
#define VC1541_ROM( cpu )	\
	ROM_REGION( 0x10000, cpu, 0 )	\
	ROM_SYSTEM_BIOS( 0, "rev1", "VC-1541 rev. 01" )	\
	ROMX_LOAD( "325302-01.ua2", 0xc000, 0x2000, CRC(29ae9752) SHA1(8e0547430135ba462525c224e76356bd3d430f11), ROM_BIOS(1) )	\
	ROMX_LOAD( "901229-01.ub3", 0xe000, 0x2000, CRC(9a48d3f0) SHA1(7a1054c6156b51c25410caec0f609efb079d3a77), ROM_BIOS(1) )	\
	ROM_SYSTEM_BIOS( 1, "rev2", "VC-1541 rev. 02" )	\
	ROMX_LOAD( "325302-01.ua2", 0xc000, 0x2000, CRC(29ae9752) SHA1(8e0547430135ba462525c224e76356bd3d430f11), ROM_BIOS(2) )	\
	ROMX_LOAD( "901229-02.ub3", 0xe000, 0x2000, CRC(b29bab75) SHA1(91321142e226168b1139c30c83896933f317d000), ROM_BIOS(2) )	\
	ROM_SYSTEM_BIOS( 2, "rev3", "VC-1541 rev. 03" )	\
	ROMX_LOAD( "325302-01.ua2", 0xc000, 0x2000, CRC(29ae9752) SHA1(8e0547430135ba462525c224e76356bd3d430f11), ROM_BIOS(3) )	\
	ROMX_LOAD( "901229-03.ub3", 0xe000, 0x2000, CRC(9126e74a) SHA1(03d17bd745066f1ead801c5183ac1d3af7809744), ROM_BIOS(3) )	\
	ROM_SYSTEM_BIOS( 3, "rev5", "VC-1541 rev. 05" )	\
	ROMX_LOAD( "325302-01.ua2", 0xc000, 0x2000, CRC(29ae9752) SHA1(8e0547430135ba462525c224e76356bd3d430f11), ROM_BIOS(4) )	\
	ROMX_LOAD( "901229-05.ub3", 0xe000, 0x2000, CRC(361c9f37) SHA1(f5d60777440829e46dc91285e662ba072acd2d8b), ROM_BIOS(4) )	\
	ROM_SYSTEM_BIOS( 4, "rev6aa", "VC-1541 rev. 06AA" )	\
	ROMX_LOAD( "325302-01.ua2", 0xc000, 0x2000, CRC(29ae9752) SHA1(8e0547430135ba462525c224e76356bd3d430f11), ROM_BIOS(5) )	\
	ROMX_LOAD( "901229-06aa.ub3", 0xe000, 0x2000, CRC(3a235039) SHA1(c7f94f4f51d6de4cdc21ecbb7e57bb209f0530c0), ROM_BIOS(5) )	\
	ROM_SYSTEM_BIOS( 5, "rev1c", "VC-1541C rev. 01" )	\
	ROMX_LOAD( "251968-01.ua2", 0xc000, 0x4000, CRC(1b3ca08d) SHA1(8e893932de8cce244117fcea4c46b7c39c6a7765), ROM_BIOS(6) )	\
	ROM_SYSTEM_BIOS( 6, "rev2c", "VC-1541C rev. 02" )	\
	ROMX_LOAD( "251968-02.ua2", 0xc000, 0x4000, CRC(2d862d20) SHA1(38a7a489c7bbc8661cf63476bf1eb07b38b1c704), ROM_BIOS(7) )	\
	ROM_SYSTEM_BIOS( 7, "rev3ii", "VC-1541-II" )	\
	ROMX_LOAD( "251968-03.u4", 0xc000, 0x4000, CRC(899fa3c5) SHA1(d3b78c3dbac55f5199f33f3fe0036439811f7fb3), ROM_BIOS(8) )	\
	ROM_SYSTEM_BIOS( 8, "reviin", "VC-1541-II (with Newtronics D500)" )	\
	ROMX_LOAD( "355640-01.u4", 0xc000, 0x4000, CRC(57224cde) SHA1(ab16f56989b27d89babe5f89c5a8cb3da71a82f0), ROM_BIOS(9) )	\



// currently not used (hacked drive firmware with more RAM)
#define DOLPHIN_ROM( cpu )	\
	ROM_REGION( 0x10000, cpu, 0 )	\
	ROM_LOAD( "c1541.rom", 0xa000, 0x6000, CRC(bd8e42b2) SHA1(d6aff55fc70876fa72be45c666b6f42b92689b4d) )



/****************************************************************************

Info from zimmers.net

1570 Parts

Location       Part Number    Description
               315090-01      32K ROM DOS 3.0
drive                         Alps

1571 Parts

Location       Part Number    Description
U11                           WD1770 FDC
U11                           WD1772 FDC
U3                            2016-20 2K x 8 bit Static RAM
U1                            6502A CPU
U4, U9                        65C22A (2) CIA
U20                           6526A CIA
U20                           8520 CIA
U6             251828-01      64H156 40 pin Gate Array
U5             251829-01      64H157 20 pin Gate Array
U7             251853-01      R/W Hybrid
U2             310654-03      23256 ROM DOS 3.0
U2             310654-05      32K ROM DOS 3.0 rev.
drive                         Alps
drive                         Newtronics D502

C128D Parts

Location       Part Number    Description
U103                          2016-20 2K x 8 bit Static RAM
U108           252308-01      MC2871A R/W Amp.
U107           252371-01      MOS5710 FDC and Gate Array
U101                          6502A CPU
U104, U106                    65C22 (2) CIA
U105           251828-01      64H156 40 pin Gate Array
U102           318047-01      23256 ROM DOS 3.1
drive                         Newtronics D502


****************************************************************************/

// no locations available for this, I need a picture of the board
// currently, we add 1570 to 1571, to preserve the firmware. 
// this will be used later, when proper drives will be added to each system
#define C1570_ROM( cpu ) \
	ROM_REGION( 0x10000, cpu, 0) \
	ROM_LOAD( "315090-01.bin", 0x8000, 0x8000, CRC(5a0c7937) SHA1(5fc06dc82ff6840f183bd43a4d9b8a16956b2f56) )


#define C1571_ROM( cpu ) \
	ROM_REGION( 0x10000, cpu, 0 ) \
	ROM_SYSTEM_BIOS( 0, "rev5", "1571 rev. 05" )	\
	ROMX_LOAD( "310654-05.u2", 0x8000, 0x8000, CRC(5755bae3) SHA1(f1be619c106641a685f6609e4d43d6fc9eac1e70), ROM_BIOS(1) )	\
	ROM_SYSTEM_BIOS( 1, "rev3", "1571 rev. 03" )	\
	ROMX_LOAD( "310654-03.u2", 0x8000, 0x8000, CRC(3889b8b8) SHA1(e649ef4419d65829d2afd65e07d31f3ce147d6eb), ROM_BIOS(2) )	\
	/* This should not be here... */	\
	ROM_SYSTEM_BIOS( 2, "1570", "1570" )	\
	ROMX_LOAD( "315090-01.bin", 0x8000, 0x8000, CRC(5a0c7937) SHA1(5fc06dc82ff6840f183bd43a4d9b8a16956b2f56), ROM_BIOS(3) )


#define C1571CR_ROM( cpu ) \
	ROM_REGION( 0x10000, cpu, 0) \
	ROM_LOAD( "318047-01.u102", 0x8000, 0x8000, CRC(f24efcc4) SHA1(14ee7a0fb7e1c59c51fbf781f944387037daa3ee) )


// no locations available for this, I need a picture of the board
/*
    rev. 01 - First revision of 1581 firmware
    rev. 02 - Second revision of 1581 firmware
    beta - Probably a beta version
    1563 - This comes from a prototype unit of the so-called c128d/81 (a c128d with built-in 1581 drive). 
        Despite the label, is a 1581 firmaware
*/
#define C1581_ROM( cpu ) \
	ROM_REGION( 0x10000, cpu, 0 ) \
	ROM_SYSTEM_BIOS( 0, "rev2", "1581 rev. 02" )	\
	ROMX_LOAD( "318045-02.bin", 0x8000, 0x8000, CRC(a9011b84) SHA1(01228eae6f066bd9b7b2b6a7fa3f667e41dad393), ROM_BIOS(1) )	\
	ROM_SYSTEM_BIOS( 1, "rev1", "1581 rev. 01" )	\
	ROMX_LOAD( "318045-01.bin", 0x8000, 0x8000, CRC(113af078) SHA1(3fc088349ab83e8f5948b7670c866a3c954e6164), ROM_BIOS(2) )	\
	ROM_SYSTEM_BIOS( 2, "beta", "1581 beta?" )	\
	ROMX_LOAD( "1581-beta.bin", 0x8000, 0x8000, CRC(ecc223cd) SHA1(a331d0d46ead1f0275b4ca594f87c6694d9d9594), ROM_BIOS(3) )	\
	ROM_SYSTEM_BIOS( 3, "1563", "1563 (from C128D/81)" )	\
	ROMX_LOAD( "1563.bin", 0x8000, 0x8000, CRC(1d184687) SHA1(2c5111a9c15be7b7955f6c8775fea25ec10c0ca0), ROM_BIOS(4) )



/* Older drive - currently these are only added as documentation (no driver uses them yet) */
/* They all use IEEE-448 connector */

/****************************************************************************

Info from zimmers.net

2031 Parts

Location       Part Number    Description
UB2                           2016 2K x 8 bit static RAM (HP)
U3B-U3E                       2114 (4) 1K x 4 bit static RAM (LP)
               901484-03      2364-107 ROM DOS 2.6 C000-DFFF
               901484-05      2364-123 ROM DOS 2.6 E000-FFFF
                              6502 CPU
                              6522 (2) VIA
drive                         Alps DFB111M25A (LP)
drive                         Alps FDM2111-B2 (LP)
drive                         Shugart 390 (HP)

****************************************************************************/

// I'm not 100% sure about U3B/U3E locations used below for ROM names
// I need a picture to confirm these, since the part listing above is not really clear (without a pic of the board)
#define C2031_ROM( cpu )	\
	ROM_REGION( 0x10000, cpu, 0 )	\
	ROM_LOAD( "901484-03.u3b", 0xc000, 0x2000, CRC(ee4b893b) SHA1(54d608f7f07860f24186749f21c96724dd48bc50) )		\
	ROM_LOAD( "901484-05.u3e", 0xe000, 0x2000, CRC(6a629054) SHA1(ec6b75ecfdd4744e5d57979ef6af990444c11ae1) )



/****************************************************************************

Info from zimmers.net

SFD-1001 Parts

Location       Part Number    Description
7G, 9G                        2116 (2) 2K x 8 static RAM

DOS:
1J             901887-01      2364 ROM DOS 2.7 C000-DFFF
3J             901887-02      2364 ROM DOS 2.7 E000-FFFF
3J             901888-01      2364 ROM DOS 2.7 E000-FFFF
2H                            6502 CPU
4J, 6J                        6532 (2) RIOT

Controller:
5C             901467-01      6316-017 ROM GCR
1E                            6502 CPU
5E                            6522 VIA
3E             251232         adapter board
drive                         Matsushita JU-570-2

Adapter Board:
U2             251257-02A     2716 EPROM controller
U1                            6530 RIOT (ROM disabled)


Notes: The DOS 2.7 ROM in SFD-1001 is the same as in 8250 drives. Differences:

CBM 8050 - dual 5" 1/4 floppy drive, disks are written on one side 
		with double density
CBM	8250 / 8250LP - dual 5.25" 1/4 floppy drive, disks are written 
		on both sides. it reads 8050-disks, LP version has smaller case
SFD-1001 - single 5" 1/4 floppy drive, same as CBM 8250LP, but only 
		one drive and 1541-case

****************************************************************************/

#define SFD1001_ROM( cpu )	\
	ROM_REGION( 0x10800, cpu, 0 )	\
	ROM_LOAD( "901887-01.1j", 0xc000, 0x2000, CRC(0073b8b2) )		\
	ROM_LOAD( "901888-01.3j", 0xe000, 0x2000, CRC(de9b6132) )		\
	ROM_LOAD( "251257-02a.u2", 0x10000, 0x800, CRC(b51150de) )	/* Floppy Disc Controller */


#define C8250_ROM( cpu )	\
	ROM_REGION( 0x10000, cpu, 0 )

// still to be sorted out

/****************************************************************************

Info from zimmers.net

2040/3040/4040 Parts

Location       Part Number    Description
UC4-UF5                       2114 (8) 1K x 4 bit static RAM

DOS:
UL1            901468-06      2332-020 ROM DOS 1 E000-EFFF
UH1            901468-07      2332-021 ROM DOS 1 F000-FFFF
UJ1            901468-11      2332 ROM DOS 2 D000-DFFF
UL1            901468-12      2332 ROM DOS 2 E000-EFFF
UH1            901468-13      2332 ROM DOS 2 F000-FFFF
UJ1            901468-14      2332-191 ROM DOS 2 rev. D000-DFFF
UL1            901468-15      2332-192 ROM DOS 2 rev. E000-EFFF
UH1            901468-16      2332-193 ROM DOS 2 rev. F000-FFFF
UN1                           6502 CPU
UC1, UE1                      6532 (2) RIOT

Controller:
UH3                           6504 CPU
UM3                           6522 VIA
UK3            901466-02      6530-028 RIOT DOS 1
UK3            901466-04      6530-034 RIOT DOS 2
UK6            901467-01      6316-017 ROM GCR
drive                         Shugart 390

****************************************************************************/

#define C4040_ROM( cpu )	\
	ROM_REGION( 0x10000, cpu, 0 )	

// still to be sorted out


/****************************************************************************

Info from zimmers.net

9060/9090 Parts

Location  Part Number   Description
5A-5H                   2114 (8) 1K x 4 bit Static RAM

DOS:
7C        300516-002  * 2564 EPROM DOS 3.0 C000-DFFF (rev. B)
7D        300517-002  * 2564 EPROM DOS 3.0 E000-FFFF (rev. B)
7C        300516-001  + 2764 EPROM DOS 3.0 C000-DFFF (rev. B)
7D        300517-001  + 2764 EPROM DOS 3.0 E000-FFFF (rev. B)
7C        300516      * 2564 EPROM DOS 3.0 rev. C000-DFFF (rev. C)
7D        300517      * 2564 EPROM DOS 3.0 rev. E000-FFFF (rev. C)
7C        300516      + 2764 EPROM DOS 3.0 rev. C000-DFFF (rev. C)
7D        300517      + 2764 EPROM DOS 3.0 rev. E000-FFFF (rev. C)
7E                      6502 CPU
7F, 7G                  6532 (2) RIOT
 * 2564 are used on PCB 300012-001
 + 2764 are used on PCB 230012-002

Controller:
4C        300515        2716 EPROM DOS 3.0 (rev. A)
4C        300515-001    2716 EPROM DOS 3.0 (rev. B)
4A                      6502 CPU
4B                      6522 VIA
4D                      6810 128 x 8 bit Static RAM

Winchester Controller:
9D                      Am2910 12 bit Bit/Slice Controller
drive                   Tandon TM602S (9060)
drive                   Tandon TM603S (9090)


Note: The Commodore D9060 and D9090 can use the same firmware. The 
difference between these units is that the jumper J14 on the DOS board 
is open in the D9060, and closed in the D9090 to select a 4-head vs. 
6-head drive. The jumper J13 seems to be unused.

****************************************************************************/

#define D9090_ROM( cpu )	\
	ROM_REGION( 0x10800, cpu, 0 )	\
	ROM_SYSTEM_BIOS( 0, "reva", "D9090 rev. A" )	\
	ROMX_LOAD( "300516-revb.7c", 0xc000, 0x2000, CRC(2d758a14), ROM_BIOS(1) )	/* No Rev A available for this! */	\
	ROMX_LOAD( "300517-reva.7d", 0xe000, 0x2000, CRC(566df630), ROM_BIOS(1) )	\
	ROMX_LOAD( "300515-reva.4c", 0x10000, 0x800, CRC(99e096f7), ROM_BIOS(1) )	/* Floppy Disc Controller Rev. A - label "300515 Rev A" */	\
	ROM_SYSTEM_BIOS( 1, "revb", "D9090 rev. B" )	\
	ROMX_LOAD( "300516-revb.7c", 0xc000, 0x2000, CRC(2d758a14), ROM_BIOS(2) )	\
	ROMX_LOAD( "300517-revb.7d", 0xe000, 0x2000, CRC(f0382bc3), ROM_BIOS(2) )	\
	ROMX_LOAD( "300515-revb.4c", 0x10000, 0x800, CRC(49adf4fb), ROM_BIOS(2) )	/* Floppy Disc Controller Rev. B - label "300515 Rev B" or "300515-001" */	\
	ROM_SYSTEM_BIOS( 2, "revc", "D9090 rev. C" )	\
	ROMX_LOAD( "300516-revc.7c", 0xc000, 0x2000, CRC(d6a3e88f), ROM_BIOS(3) )	\
	ROMX_LOAD( "300517-revc.7d", 0xe000, 0x2000, CRC(2a9ad4ad), ROM_BIOS(3) )	\
	ROMX_LOAD( "300515-revb.4c", 0x10000, 0x800, CRC(49adf4fb), ROM_BIOS(3) )	/* Floppy Disc Controller Rev. B - label "300515 Rev B" or "300515-001" */


/****************************************************************************

	Dual 8" floppy drive. Related to any other drive? What about 8060?

****************************************************************************/

#define C8280_ROM( cpu )	\
	ROM_REGION( 0x10800, cpu, 0 )	\
	ROM_LOAD( "300542-001.bin", 0xc000, 0x2000, CRC(3c6eee1e) )		\
	ROM_LOAD( "300543-001.bin", 0xe000, 0x2000, CRC(f58e665e) )		\
	ROM_LOAD( "300541-001.bin", 0x10000, 0x800, CRC(cb07b2db) )	/* GCR encoding/decoding tables? */



#if 0
	ROM_LOAD("dos2040",  0x?000, 0x2000, CRC(d04c1fbb))

	ROM_LOAD("dos3040",  0x?000, 0x3000, CRC(f4967a7f))

	ROM_LOAD("dos4040",  0x?000, 0x3000, CRC(40e0ebaa))

	ROM_LOAD("dos1001",  0xc000, 0x4000, CRC(87e6a94e))

	/* dolphin vc1541 */
	ROM_LOAD("c1541.rom",  0xa000, 0x6000, CRC(bd8e42b2))

	/* modified drive 0x2000-0x3ffe ram, 0x3fff 6529 */
	ROM_LOAD("1581rom5.bin",  0x8000, 0x8000, CRC(e08801d7))

	ROM_LOAD("",  0xc000, 0x4000, CRC())
#endif

#endif /* VC1541_H_ */

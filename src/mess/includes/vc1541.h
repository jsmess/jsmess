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

DEVICE_IMAGE_LOAD(vc1541);
DEVICE_IMAGE_UNLOAD(vc1541);

int drive_config(int type, int id, int mode, int cpunr, int devicenr);
void drive_reset(void);

void vc1541_device_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info);
void c2031_device_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info);
void c1551_device_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info);
void c1571_device_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info);

MACHINE_DRIVER_EXTERN( cpu_vc1540 );
MACHINE_DRIVER_EXTERN( cpu_vc1541 );
MACHINE_DRIVER_EXTERN( cpu_c2031 );
MACHINE_DRIVER_EXTERN( cpu_dolphin );
MACHINE_DRIVER_EXTERN( cpu_c1551 );
MACHINE_DRIVER_EXTERN( cpu_c1571 );

#define VC1540_ROM(cpu) \
	ROM_REGION(0x10000,cpu,0) \
	ROM_LOAD("325302.01",  0xc000, 0x2000, CRC(29ae9752) SHA1(8e0547430135ba462525c224e76356bd3d430f11)) \
	ROM_LOAD("325303.01",  0xe000, 0x2000, CRC(10b39158) SHA1(56dfe79b26f50af4e83fd9604857756d196516b9))

#if 1
#define C2031_ROM(cpu) \
	ROM_REGION(0x10000,cpu,0) \
	ROM_LOAD("dos2031",  0xc000, 0x4000, CRC(21b80fdf) SHA1(c53e180a96649ceb3f421739e8dc66faba7cba44))
#else
#define C2031_ROM(cpu) \
	ROM_REGION(0x10000,cpu,0) \
	ROM_LOAD("901484.03",  0xc000, 0x2000, CRC(ee4b893b) SHA1(54d608f7f07860f24186749f21c96724dd48bc50)) \
	ROM_LOAD("901484.05",  0xe000, 0x2000, CRC(6a629054) SHA1(ec6b75ecfdd4744e5d57979ef6af990444c11ae1))
#endif

#if 1
#define VC1541_ROM(cpu) \
	ROM_REGION(0x10000,cpu,0) \
	ROM_LOAD("325302.01",  0xc000, 0x2000, CRC(29ae9752) SHA1(8e0547430135ba462525c224e76356bd3d430f11)) \
	ROM_LOAD("901229.05",  0xe000, 0x2000, CRC(361c9f37) SHA1(f5d60777440829e46dc91285e662ba072acd2d8b) )
#else
/* for this I have the documented rom listing in german */
#define VC1541_ROM(cpu) \
	ROM_REGION(0x10000,cpu,0) \
	ROM_LOAD("325302.01",  0xc000, 0x2000, CRC(29ae9752) SHA1(8e0547430135ba462525c224e76356bd3d430f11)) \
	ROM_LOAD("901229.03",  0xe000, 0x2000, CRC(9126e74a) SHA1(03d17bd745066f1ead801c5183ac1d3af7809744))
#endif

#define DOLPHIN_ROM(cpu) \
	ROM_REGION(0x10000,cpu,0) \
	ROM_LOAD("c1541.rom",  0xa000, 0x6000, CRC(bd8e42b2) SHA1(d6aff55fc70876fa72be45c666b6f42b92689b4d))

#define C1551_ROM(cpu) \
	ROM_REGION(0x10000,cpu,0) \
	ROM_LOAD("318008.01",  0xc000, 0x4000, CRC(6d16d024) SHA1(fae3c788ad9a6cc2dbdfbcf6c0264b2ca921d55e))

#define C1570_ROM(cpu) \
	ROM_REGION(0x10000,cpu,0) \
	ROM_LOAD("315090.01",  0x8000, 0x8000, CRC(5a0c7937) SHA1(5fc06dc82ff6840f183bd43a4d9b8a16956b2f56))

#define C1571_ROM(cpu) \
	ROM_REGION(0x10000,cpu,0) \
	ROM_LOAD("310654.03",  0x8000, 0x8000, CRC(3889b8b8) SHA1(e649ef4419d65829d2afd65e07d31f3ce147d6eb))

#if 0
	ROM_LOAD("dos2040",  0x?000, 0x2000, CRC(d04c1fbb))

	ROM_LOAD("dos3040",  0x?000, 0x3000, CRC(f4967a7f))

	ROM_LOAD("dos4040",  0x?000, 0x3000, CRC(40e0ebaa))

	ROM_LOAD("dos1001",  0xc000, 0x4000, CRC(87e6a94e))

	/* vc1541 drive hardware */
	ROM_LOAD("dos2031",  0xc000, 0x4000, CRC(21b80fdf))

	ROM_LOAD("1540-c000.325302-01.bin",  0xc000, 0x2000, CRC(29ae9752))
	ROM_LOAD("1540-e000.325303-01.bin",  0xe000, 0x2000, CRC(10b39158))

	ROM_LOAD("1541-e000.901229-01.bin",  0xe000, 0x2000, CRC(9a48d3f0))
	ROM_LOAD("1541-e000.901229-02.bin",  0xe000, 0x2000, CRC(b29bab75))
	ROM_LOAD("1541-e000.901229-03.bin",  0xe000, 0x2000, CRC(9126e74a))
	ROM_LOAD("1541-e000.901229-05.bin",  0xe000, 0x2000, CRC(361c9f37))

	ROM_LOAD("1541-II.251968-03.bin",  0xe000, 0x2000, CRC(899fa3c5))

	ROM_LOAD("1541C.251968-01.bin",  0xc000, 0x4000, CRC(1b3ca08d))
	ROM_LOAD("1541C.251968-02.bin",  0xc000, 0x4000, CRC(2d862d20))

	ROM_LOAD("dos1541.c0",  0xc000, 0x2000, CRC(5b84bcef))
	ROM_LOAD("dos1541.e0",  0xe000, 0x2000, CRC(2d8c1fde))
	 /* merged gives 0x899fa3c5 */

	 /* 0x29ae9752 and 0x361c9f37 merged */
	ROM_LOAD("vc1541",  0xc000, 0x4000, CRC(57224cde))

	 /* 0x29ae9752 and 0xb29bab75 merged */
	ROM_LOAD("vc1541",  0xc000, 0x4000, CRC(d3a5789c))

	/* dolphin vc1541 */
	ROM_LOAD("c1541.rom",  0xa000, 0x6000, CRC(bd8e42b2))

	ROM_LOAD("1551.318008-01.bin",  0xc000, 0x4000, CRC(6d16d024))

	/* bug fixes introduced bugs for 1541 mode
     jiffydos to have fixed 1571 and working 1541 mode */
	ROM_LOAD("1570-rom.315090-01.bin",  0x8000, 0x8000, CRC(5a0c7937))
	ROM_LOAD("1571-rom.310654-03.bin",  0x8000, 0x8000, CRC(3889b8b8))
	ROM_LOAD("1571-rom.310654-05.bin",  0x8000, 0x8000, CRC(5755bae3))
	ROM_LOAD("1571cr-rom.318047-01.bin",  0x8000, 0x8000, CRC(f24efcc4))

	ROM_LOAD("1581-rom.318045-01.bin",  0x8000, 0x8000, CRC(113af078))
	ROM_LOAD("1581-rom.318045-02.bin",  0x8000, 0x8000, CRC(a9011b84))
	ROM_LOAD("1581-rom.beta.bin",  0x8000, 0x8000, CRC(ecc223cd))

	/* modified drive 0x2000-0x3ffe ram, 0x3fff 6529 */
	ROM_LOAD("1581rom5.bin",  0x8000, 0x8000, CRC(e08801d7))

	ROM_LOAD("",  0xc000, 0x4000, CRC())
#endif


/* IEC interface for c16 with c1551 */

/* To be passed directly to the drivers */
void c1551x_0_write_data (int data);
int c1551x_0_read_data (void);
void c1551x_0_write_handshake (int data);
int c1551x_0_read_handshake (void);
int c1551x_0_read_status (void);


/* serial bus for vic20, c64 & c16 with vc1541 and some printer */

/* To be passed to serial bus emulation */
void vc1541_serial_reset_write(int which,int level);
int vc1541_serial_atn_read(int which);
void vc1541_serial_atn_write(running_machine *machine, int which,int level);
int vc1541_serial_data_read(int which);
void vc1541_serial_data_write(int which,int level);
int vc1541_serial_clock_read(int which);
void vc1541_serial_clock_write(int which,int level);
int vc1541_serial_request_read(int which);
void vc1541_serial_request_write(int which,int level);


#endif /* VC1541_H_ */

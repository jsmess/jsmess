#ifndef _SMS_H_
#define _SMS_H_

#define LOG_REG
#define LOG_PAGING
//#define LOG_CURLINE
#define LOG_COLOR

#define NVRAM_SIZE								(0x8000)
#define CPU_ADDRESSABLE_SIZE			(0x10000)


#define FLAG_GAMEGEAR			0x00010000
#define FLAG_BIOS_0400			0x00020000
#define FLAG_BIOS_2000			0x00040000
#define FLAG_BIOS_FULL			0x00080000
#define FLAG_FM				0x00100000
#define FLAG_REGION_JAPAN		0x00200000

#define IS_GAMEGEAR			( Machine->gamedrv->flags & FLAG_GAMEGEAR )
#define HAS_BIOS_0400			( Machine->gamedrv->flags & FLAG_BIOS_0400 )
#define HAS_BIOS_2000			( Machine->gamedrv->flags & FLAG_BIOS_2000 )
#define HAS_BIOS_FULL			( Machine->gamedrv->flags & FLAG_BIOS_FULL )
#define HAS_BIOS			( Machine->gamedrv->flags & ( FLAG_BIOS_0400 | FLAG_BIOS_2000 | FLAG_BIOS_FULL ) )
#define HAS_FM				( Machine->gamedrv->flags & FLAG_FM )
#define IS_REGION_JAPAN			( Machine->gamedrv->flags & FLAG_REGION_JAPAN )

/* Function prototypes */

WRITE8_HANDLER(sms_cartram_w);
WRITE8_HANDLER(sms_cartram2_w);
WRITE8_HANDLER(sms_fm_detect_w);
 READ8_HANDLER(sms_fm_detect_r);
 READ8_HANDLER(sms_input_port_0_r);
WRITE8_HANDLER(sms_YM2413_register_port_0_w);
WRITE8_HANDLER(sms_YM2413_data_port_0_w);
WRITE8_HANDLER(sms_version_w);
 READ8_HANDLER(sms_version_r);
WRITE8_HANDLER(sms_mapper_w);
 READ8_HANDLER(sms_mapper_r);
WRITE8_HANDLER(sms_bios_w);
WRITE8_HANDLER(gg_sio_w);
 READ8_HANDLER(gg_sio_r);
 READ8_HANDLER(gg_psg_r);
WRITE8_HANDLER(gg_psg_w);
 READ8_HANDLER(gg_input_port_2_r);

void setup_rom(void);

void check_pause_button( void );

DEVICE_INIT( sms_cart );
DEVICE_LOAD( sms_cart );

MACHINE_START(sms);
MACHINE_RESET(sms);

READ8_HANDLER(sms_store_cart_select_r);
WRITE8_HANDLER(sms_store_cart_select_w);
READ8_HANDLER(sms_store_select1);
READ8_HANDLER(sms_store_select2);
READ8_HANDLER(sms_store_control_r);
WRITE8_HANDLER(sms_store_control_w);
void sms_int_callback( int state );
void sms_store_int_callback( int state );

#define IO_EXPANSION				(0x80)	/* Expansion slot enable (1= disabled, 0= enabled) */
#define IO_CARTRIDGE				(0x40)	/* Cartridge slot enable (1= disabled, 0= enabled) */
#define IO_CARD							(0x20)	/* Card slot disabled (1= disabled, 0= enabled) */
#define IO_WORK_RAM					(0x10)	/* Work RAM disabled (1= disabled, 0= enabled) */
#define IO_BIOS_ROM					(0x08)	/* BIOS ROM disabled (1= disabled, 0= enabled) */
#define IO_CHIP							(0x04)	/* I/O chip disabled (1= disabled, 0= enabled) */

#endif /* _SMS_H_ */


/*****************************************************************************
 *
 * includes/sms.h
 *
 ****************************************************************************/

#ifndef SMS_H_
#define SMS_H_

#define LOG_REG
#define LOG_PAGING
#define LOG_COLOR

#define NVRAM_SIZE             (0x08000)
#define CPU_ADDRESSABLE_SIZE   (0x10000)

/*----------- defined in machine/sms.c -----------*/

/* Function prototypes */
WRITE8_HANDLER( sms_cartram_w );
WRITE8_HANDLER( sms_cartram2_w );
WRITE8_HANDLER( sms_fm_detect_w );
READ8_HANDLER( sms_fm_detect_r );
READ8_HANDLER( sms_input_port_0_r );
READ8_HANDLER( sms_input_port_1_r );
WRITE8_HANDLER( sms_ym2413_register_port_0_w );
WRITE8_HANDLER( sms_ym2413_data_port_0_w );
WRITE8_HANDLER( sms_io_control_w );
READ8_HANDLER( sms_count_r );
WRITE8_HANDLER( sms_mapper_w );
READ8_HANDLER( sms_mapper_r );
WRITE8_HANDLER( sms_bios_w );
WRITE8_HANDLER( gg_sio_w );
READ8_HANDLER( gg_sio_r );
READ8_HANDLER( gg_psg_r );
WRITE8_HANDLER( gg_psg_w );
READ8_HANDLER( gg_input_port_2_r );

void sms_pause_callback( running_machine *machine );
void sms_store_int_callback( running_machine *machine, int state );

DEVICE_START( sms_cart );
DEVICE_IMAGE_LOAD( sms_cart );

MACHINE_START( sms );
MACHINE_RESET( sms );

READ8_HANDLER( sms_store_cart_select_r );
WRITE8_HANDLER( sms_store_cart_select_w );
READ8_HANDLER( sms_store_select1 );
READ8_HANDLER( sms_store_select2 );
READ8_HANDLER( sms_store_control_r );
WRITE8_HANDLER( sms_store_control_w );

#define IO_EXPANSION    (0x80)	/* Expansion slot enable (1= disabled, 0= enabled) */
#define IO_CARTRIDGE    (0x40)	/* Cartridge slot enable (1= disabled, 0= enabled) */
#define IO_CARD         (0x20)	/* Card slot disabled (1= disabled, 0= enabled) */
#define IO_WORK_RAM     (0x10)	/* Work RAM disabled (1= disabled, 0= enabled) */
#define IO_BIOS_ROM     (0x08)	/* BIOS ROM disabled (1= disabled, 0= enabled) */
#define IO_CHIP         (0x04)	/* I/O chip disabled (1= disabled, 0= enabled) */


DRIVER_INIT( sg1000m3 );
DRIVER_INIT( sms1 );
DRIVER_INIT( smsj );
DRIVER_INIT( sms2kr );
DRIVER_INIT( smssdisp );
DRIVER_INIT( gamegear );
DRIVER_INIT( gamegeaj );


#endif /* SMS_H_ */

/**********************************************************************

  Copyright (C) Antoine Mine' 2008

   Hewlett Packard HP48 S/SX & G/GX

**********************************************************************/

/***************************************************************************
    MACROS
***************************************************************************/

/* read from I/O memory */
#define HP48_IO_4(x)   (hp48_io[(x)])
#define HP48_IO_8(x)   (hp48_io[(x)] | (hp48_io[(x)+1] << 4))
#define HP48_IO_12(x)  (hp48_io[(x)] | (hp48_io[(x)+1] << 4) | (hp48_io[(x)+2] << 8))
#define HP48_IO_20(x)  (hp48_io[(x)] | (hp48_io[(x)+1] << 4) | (hp48_io[(x)+2] << 8) | \
	               (hp48_io[(x)+3] << 12) | (hp48_io[(x)+4] << 16))


/*----------- defined in machine/hp48.c -----------*/

/***************************************************************************
    GLOBAL VARIABLES & CONSTANTS
***************************************************************************/

/* I/O memory */
extern UINT8 hp48_io[64];



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/


/************************ Saturn's I/O *******************************/

/* memory controller */
void hp48_mem_reset( running_device *device );
void hp48_mem_config( running_device *device, int v );
void hp48_mem_unconfig( running_device *device, int v );
int  hp48_mem_id( running_device *device );

/* CRC computation */
void hp48_mem_crc( running_device *device, int addr, int data );

/* IN/OUT registers */
int  hp48_reg_in( running_device *device );
void hp48_reg_out( running_device *device, int v );

/* keybord interrupt system */
void hp48_rsi( running_device *device );


/***************************** serial ********************************/

extern void hp48_rs232_start_recv_byte( running_machine *machine, UINT8 data );


/****************************** cards ********************************/

struct hp48_port_config;

extern const struct hp48_port_config hp48sx_port1_config;
extern const struct hp48_port_config hp48sx_port2_config;
extern const struct hp48_port_config hp48gx_port1_config;
extern const struct hp48_port_config hp48gx_port2_config;

extern DEVICE_GET_INFO( hp48_port );

#define HP48_PORT DEVICE_GET_INFO_NAME( hp48_port )

#define MDRV_HP48_PORT_ADD(_tag, _intrf) \
	MDRV_DEVICE_ADD(_tag, HP48_PORT, 0) \
	MDRV_DEVICE_CONFIG(_intrf)

/****************************** machine ******************************/

extern DRIVER_INIT( hp48 );

extern MACHINE_START( hp48s  );
extern MACHINE_START( hp48sx );
extern MACHINE_START( hp48g  );
extern MACHINE_START( hp48gx );
extern MACHINE_START( hp48gp );

extern MACHINE_RESET( hp48 );

/*----------- defined in video/hp48.c -----------*/

/****************************** video ********************************/

extern VIDEO_UPDATE ( hp48 );
extern PALETTE_INIT ( hp48 );

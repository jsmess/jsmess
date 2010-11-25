/**********************************************************************

  Copyright (C) Antoine Mine' 2008

   Hewlett Packard HP48 S/SX & G/GX

**********************************************************************/

#ifdef CHARDEV
#include "devices/chardev.h"
#endif

/* model */
typedef enum {
	HP48_S,
	HP48_SX,
	HP48_G,
	HP48_GX,
	HP48_GP,
} hp48_models;

/* memory module configuration */
typedef struct
{
	/* static part */
	UINT32 off_mask;             /* offset bit-mask, indicates the real size */
	read8_space_func read;
	write8_space_func write;
	void* data;                  /* non-NULL for banks */
	int isnop;

	/* configurable part */
	UINT8  state;                /* one of HP48_MODULE_ */
	UINT32 base;                 /* base address */
	UINT32 mask;                 /* often improperly called size, it is an address select mask */

} hp48_module;


/* screen image averaging */
#define HP48_NB_SCREENS 3

class hp48_state : public driver_device
{
public:
	hp48_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 *videoram;
	UINT8 io[64];
	hp48_models model;
	UINT16 out;
	UINT8 kdn;
	hp48_module modules[6];
	UINT32 port_size[2];
	UINT8 port_write[2];
	UINT8* port_data[2];
	UINT32 bank_switch;
	UINT32 io_addr;
	UINT16 crc;
	UINT8 timer1;
	UINT32 timer2;
#ifdef CHARDEV
	chardev* chardev;
#endif
	UINT8 screens[ HP48_NB_SCREENS ][ 64 ][ 144 ];
	int cur_screen;
};


/***************************************************************************
    MACROS
***************************************************************************/

/* read from I/O memory */
#define HP48_IO_4(x)   (state->io[(x)])
#define HP48_IO_8(x)   (state->io[(x)] | (state->io[(x)+1] << 4))
#define HP48_IO_12(x)  (state->io[(x)] | (state->io[(x)+1] << 4) | (state->io[(x)+2] << 8))
#define HP48_IO_20(x)  (state->io[(x)] | (state->io[(x)+1] << 4) | (state->io[(x)+2] << 8) | \
	               (state->io[(x)+3] << 12) | (state->io[(x)+4] << 16))


/*----------- defined in machine/hp48.c -----------*/

/***************************************************************************
    GLOBAL VARIABLES & CONSTANTS
***************************************************************************/

/* I/O memory */



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

DECLARE_LEGACY_IMAGE_DEVICE(HP48_PORT, hp48_port);

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

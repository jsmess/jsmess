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
void hp48_mem_reset( running_machine* machine );
void hp48_mem_config( running_machine* machine, int v );
void hp48_mem_unconfig( running_machine* machine, int v );
int  hp48_mem_id( running_machine* machine );

/* CRC computation */
void hp48_mem_crc( running_machine* machine, int addr, int data );

/* IN/OUT registers */
int  hp48_reg_in( running_machine* machine );
void hp48_reg_out( running_machine* machine, int v );

/* keybord interrupt system */
void hp48_rsi( running_machine* machine );


/***************************** serial ********************************/

extern void hp48_rs232_start_recv_byte( running_machine *machine, UINT8 data );


/****************************** cards ********************************/

struct hp48_port_config;

extern struct hp48_port_config hp48sx_port1_config;
extern struct hp48_port_config hp48sx_port2_config;
extern struct hp48_port_config hp48gx_port1_config;
extern struct hp48_port_config hp48gx_port2_config;

extern DEVICE_GET_INFO( hp48_port );

#define HP48_PORT DEVICE_GET_INFO_NAME( hp48_port )


/****************************** machine ******************************/

extern DRIVER_INIT( hp48 );

extern MACHINE_START( hp48s  );
extern MACHINE_START( hp48sx );
extern MACHINE_START( hp48g  );
extern MACHINE_START( hp48gx );

extern MACHINE_RESET( hp48 );

/*----------- defined in video/hp48.c -----------*/

/****************************** video ********************************/

extern VIDEO_UPDATE ( hp48 );
extern PALETTE_INIT ( hp48 );

/////////////////////////////////////////////////////////////////////
//////   HECTOR HEADER FILE /////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
/*
        Hector 2HR+
        Victor
        Hector 2HR
        Hector HRX
        Hector MX40c
        Hector MX80c
        Hector1
        Interact

        12/05/2009 Skeleton driver - Micko : mmicko@gmail.com
        31/06/2009 Video - Robbbert

        29/10/2009 Update skeleton to functional machine
                          by yo_fr       (jj.stac@aliceadsl.fr)

               => add Keyboard,
               => add color,
               => add cassette,
               => add sn76477 sound and 1bit sound,
               => add joysticks (stick, pot, fire)
               => add BR/HR switching
               => add bank switch for HRX
               => add device MX80c and bank switching for the ROM
       03/01/2010 Update and clean prog  by yo_fr       (jj.stac@aliceadsl.fr)
               => add the port mapping for keyboard

      don't forget to keep some information about these machine see DChector project : http://dchector.free.fr/ made by DanielCoulom
      (and thank's to Daniel!)

    TODO : Add the cartridge function,
           Adjust the one shot and A/D timing (sn76477)
*/

#include "machine/upd765.h"

/* Enum status for high memory bank (c000 - ffff)*/
enum
{
	HECTOR_BANK_PROG = 0,       		/* first BANK is program ram*/
	HECTOR_BANK_VIDEO					/* second BANK is Video ram */
};
/* Status for rom memory bank (0000 - 3fff) in MX machine*/
enum
{
	HECTORMX_BANK_PAGE0 = 0,        	/* first BANK is base rom*/
	HECTORMX_BANK_PAGE1,				/* second BANK is basic rom */
	HECTORMX_BANK_PAGE2					/* 3 BANK is monitrix / assemblex rom */
};

class hec2hrp_state : public driver_device
{
public:
	hec2hrp_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 *videoram;
};


/*----------- defined in machine/hec2hrp.c -----------*/

/* Sound function*/
extern const sn76477_interface hector_sn76477_interface;

/* Protoype of memory Handler*/
extern WRITE8_HANDLER( hector_switch_bank_w );
extern READ8_HANDLER( hector_keyboard_r );
extern WRITE8_HANDLER( hector_keyboard_w );
extern WRITE8_HANDLER( hector_sn_2000_w );
extern WRITE8_HANDLER( hector_sn_2800_w );
extern WRITE8_HANDLER( hector_sn_3000_w );
extern READ8_HANDLER( hector_cassette_r );
extern WRITE8_HANDLER( hector_color_a_w );
extern WRITE8_HANDLER( hector_color_b_w );

extern void hector_init(running_machine *machine);
extern void hec2hrp_reset(running_machine *machine, int hr);
extern void hec2hrx_reset(running_machine *machine, int hr);

extern READ8_HANDLER( hector_mx_io_port_r );
extern WRITE8_HANDLER( hector_mx80_io_port_w );
extern WRITE8_HANDLER( hector_mx40_io_port_w );
extern  READ8_HANDLER( hector_io_8255_r);
extern WRITE8_HANDLER( hector_io_8255_w);

/*----------- defined in video/hec2video.c -----------*/

extern void hector_80c(bitmap_t *bitmap, UINT8 *page, int ymax, int yram) ;
extern void hector_hr(bitmap_t *bitmap, UINT8 *page, int ymax, int yram) ;
VIDEO_START( hec2hrp );
VIDEO_UPDATE( hec2hrp );

/* Global variables used in extern modules*/

/* Status for screen definition*/
extern UINT8 hector_flag_hr;
extern UINT8 hector_flag_80c;

/* Ram video BR :*/
extern UINT8 hector_videoram[0x04000];

/* Color status*/
extern UINT8 hector_color[4];

// state Hector's port
extern UINT8 hector_port_a;
extern UINT8 hector_port_b;
extern UINT8 hector_port_c_h;
extern UINT8 hector_port_c_l;
extern UINT8 hector_port_cmd;

// state disk2 port 
extern UINT8 disk2_data_r_ready; /* =ff when PC2 = true and data in read buffer (disk2_data_read) */
extern UINT8 disk2_data_w_ready; /* =ff when Disk 2 Port 40 had send a data in write buffer (disk2_data_write) */
extern UINT8 disk2_data_read;    /* Data send by Hector to Disk 2 when PC2=true */
extern UINT8 disk2_data_write;   /* Data send by Disk 2 to Hector when Write Port I/O 40 */

// disk2 handling
extern WRITE_LINE_DEVICE_HANDLER( disk2_fdc_interrupt );

extern READ8_HANDLER(  disk2_io30_port_r);
extern WRITE8_HANDLER( disk2_io30_port_w);
extern READ8_HANDLER(  disk2_io40_port_r);
extern WRITE8_HANDLER( disk2_io40_port_w);
extern READ8_HANDLER(  disk2_io50_port_r);
extern WRITE8_HANDLER( disk2_io50_port_w);
extern READ8_HANDLER(  disk2_io60_port_r);
extern WRITE8_HANDLER( disk2_io60_port_w);
extern READ8_HANDLER(  disk2_io61_port_r);
extern WRITE8_HANDLER( disk2_io61_port_w);
extern READ8_HANDLER(  disk2_io70_port_r);
extern WRITE8_HANDLER( disk2_io70_port_w);

extern WRITE8_HANDLER( hector_disk2_w );
extern READ8_HANDLER(  hector_disk2_r );

extern void Init_Timer_DiskII( running_machine *machine);

extern const upd765_interface disk2_upd765_interface;
extern const floppy_config    disk2_floppy_config;

/* Disk II Memory */
extern UINT8 Disk2memory[0x10000];  /* Memory space for Disk II unit*/

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
        Hector 1
        Interact

        12/05/2009 Skeleton driver - Micko : mmicko@gmail.com
        31/06/2009 Video - Robbbert

        29/10/2009 Update skeleton to functional machine
                    by yo_fr            (jj.stac @ aliceadsl.fr)

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
        20/11/2010 : synchronization between uPD765 and Z80 are now OK, CP/M runnig! JJStacino

            don't forget to keep some information about these machine see DChector project : http://dchector.free.fr/ made by DanielCoulom
            (and thank's to Daniel!)

    TODO :  Add the cartridge function,
            Adjust the one shot and A/D timing (sn76477)
*/

#include "machine/upd765.h"

/* Enum status for high memory bank (c000 - ffff)*/
enum
{
	HECTOR_BANK_PROG = 0,				/* first BANK is program ram*/
	HECTOR_BANK_VIDEO					/* second BANK is Video ram */
};
/* Status for rom memory bank (0000 - 3fff) in MX machine*/
enum
{
	HECTORMX_BANK_PAGE0 = 0,			/* first BANK is base rom*/
	HECTORMX_BANK_PAGE1,				/* second BANK is basic rom */
	HECTORMX_BANK_PAGE2					/* 3 BANK is monitrix / assemblex rom */
};
/* Enum status for low memory bank (00000 - 0fff) for DISC II*/
enum
{
	DISCII_BANK_RAM = 0,			/* first BANK is program ram*/
	DISCII_BANK_ROM					/* second BANK is ROM */
};

class hec2hrp_state : public driver_device
{
public:
	hec2hrp_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }
	UINT8 *m_videoram;
	UINT8 *m_hector_videoram;
};

/*----------- defined in machine/hec2hrp.c -----------*/

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

extern void hector_init( running_machine &machine);
extern void hector_reset(running_machine &machine, int hr, int with_D2);
extern void hector_disc2_reset( running_machine &machine);

extern READ8_HANDLER( hector_mx_io_port_r );
extern WRITE8_HANDLER( hector_mx80_io_port_w );
extern WRITE8_HANDLER( hector_mx40_io_port_w );
extern  READ8_HANDLER( hector_io_8255_r);
extern WRITE8_HANDLER( hector_io_8255_w);

/*----------- defined in video/hec2video.c -----------*/

extern void hector_80c(bitmap_t *bitmap, UINT8 *page, int ymax, int yram) ;
extern void hector_hr(bitmap_t *bitmap, UINT8 *page, int ymax, int yram) ;
VIDEO_START( hec2hrp );
SCREEN_UPDATE( hec2hrp );

/* Global variables used in extern modules*/

/* Status for screen definition*/
extern UINT8 hector_flag_hr;
extern UINT8 hector_flag_80c;

/* Ram video BR :*/
extern UINT8 hector_videoram[0x04000];

/* Color status*/
extern UINT8 hector_color[4];

/* Sound function*/
extern const sn76477_interface hector_sn76477_interface;

// state disc2 port
extern UINT8 hector_disc2_data_r_ready; /* =ff when PC2 = true and data in read buffer (hector_disc2_data_read) */
extern UINT8 hector_disc2_data_w_ready; /* =ff when Disc 2 Port 40 had send a data in write buffer (hector_disc2_data_write) */
extern UINT8 hector_disc2_data_read;    /* Data send by Hector to Disc 2 when PC2=true */
extern UINT8 hector_disc2_data_write;   /* Data send by Disc 2 to Hector when Write Port I/O 40 */

// disc2 handling
WRITE_LINE_DEVICE_HANDLER( hector_disk2_fdc_interrupt );
READ8_HANDLER(  hector_disc2_io00_port_r);
WRITE8_HANDLER( hector_disc2_io00_port_w);
READ8_HANDLER(  hector_disc2_io20_port_r);
WRITE8_HANDLER( hector_disc2_io20_port_w);
READ8_HANDLER(  hector_disc2_io30_port_r);
WRITE8_HANDLER( hector_disc2_io30_port_w);
READ8_HANDLER(  hector_disc2_io40_port_r);
WRITE8_HANDLER( hector_disc2_io40_port_w);
READ8_HANDLER(  hector_disc2_io50_port_r);
WRITE8_HANDLER( hector_disc2_io50_port_w);
READ8_HANDLER(  hector_disc2_io61_port_r);
WRITE8_HANDLER( hector_disc2_io61_port_w);
READ8_HANDLER(  hector_disc2_io70_port_r);
WRITE8_HANDLER( hector_disc2_io70_port_w);

extern void hector_disc2_init( running_machine &machine);

extern const upd765_interface hector_disc2_upd765_interface;
extern const floppy_config    hector_disc2_floppy_config;

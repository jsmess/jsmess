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
extern void hector_reset(running_machine *machine, int hr);

extern READ8_HANDLER( hector_mx_io_port_r);
extern WRITE8_HANDLER( hector_mx80_io_port_w);
extern WRITE8_HANDLER( hector_mx40_io_port_w);


/* Prototype of video function*/
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

/* Sound function*/
extern sn76477_interface hector_sn76477_interface;


/* Enum status for high memory bank (c000 - ffff)*/
enum
{
	HECTOR_BANK_PROG = 0,        		/* first BANK is program ram*/
	HECTOR_BANK_VIDEO					/* second BANK is Video ram */
};
/* Status for rom memory bank (0000 - 3fff) in MX machine*/
enum
{
	HECTORMX_BANK_PAGE0 = 0,        	/* first BANK is base rom*/
	HECTORMX_BANK_PAGE1,				/* second BANK is basic rom */
	HECTORMX_BANK_PAGE2			  		/* 3 BANK is monitrix / assemblex rom */
};

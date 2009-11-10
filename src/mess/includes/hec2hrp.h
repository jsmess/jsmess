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
    Importante note : the keyboard function add been piked from 
                      DChector project : http://dchector.free.fr/ made by DanielCoulom
                      (thank's Daniel)
    TODO : Add the cartridge function,
           Adjust the one shot and A/D timing (sn76477)
*/

// Protoype of memory Handler
extern WRITE8_HANDLER( hector_switch_bank_w );
extern void Key(int pccode, int state, running_machine *machine);
extern READ8_HANDLER( hector_keyboard_r );
extern WRITE8_HANDLER( hector_sn_2000_w );
extern WRITE8_HANDLER( hector_sn_2800_w );
extern WRITE8_HANDLER( hector_sn_3000_w );
extern READ8_HANDLER( hector_cassette_r );
extern WRITE8_HANDLER( hector_color_a_w );
extern WRITE8_HANDLER( hector_color_b_w );


extern READ8_HANDLER( hector_mx_io_port_r);
extern WRITE8_HANDLER( hector_mx80_io_port_w);
extern WRITE8_HANDLER( hector_mx40_io_port_w);


// Prototype of video function
extern void Init_Hector_Palette( running_machine  *machine);
extern void hector_80c(bitmap_t *bitmap, UINT8 *page, int ymax, int yram) ;
extern void hector_hr(bitmap_t *bitmap, UINT8 *page, int ymax, int yram) ;

// Global variables used in extern modules

// Status for screen definition
extern UINT8 flag_hr;
extern UINT8 flag_80c;
// Status for CPU clock
extern UINT8 flag_clk;  // 0 5MHz (HR machine) - 1  1.75MHz (BR machine)

// Ram video BR :
extern UINT8 videoramhector[0x04000];

// Color status
extern UINT8 Col0, Col1,Col2,Col3;  // For actual colors

// Sound function
extern void Mise_A_Jour_Etat(int Adresse, int Value );
extern void Init_Value_SN76477_Hector(void);
extern void Update_Sound(const address_space *space, UINT8 data);
extern  sn76477_interface hector_sn76477_interface;


//Keyboard / joystick / cassette
extern UINT8 actions;
extern UINT8 touches[8];
extern UINT8 pot0, pot1;  // State for resistor
extern UINT8 write_cassette; 

//TIMER definition
extern TIMER_CALLBACK( Callback_CK );
extern TIMER_CALLBACK( Callback_keyboard );
extern  emu_timer *keyboard_timer;
extern emu_timer *Cassette_timer;


// Enum status for high memory bank (c000 - ffff)
enum
{
	HECTOR_BANK_PROG = 0,        		/* first BANK is program ram*/
	HECTOR_BANK_VIDEO					/* second BANK is Video ram */
};
// Status for rom memory bank (0000 - 3fff) in MX machine
enum
{
	HECTORMX_BANK_PAGE0 = 0,        	/* first BANK is base rom*/
	HECTORMX_BANK_PAGE1,				/* second BANK is basic rom */
	HECTORMX_BANK_PAGE2			  		/* 3 BANK is monitrix / assemblex rom */
};


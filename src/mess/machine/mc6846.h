/**********************************************************************

  Copyright (C) Antoine Mine' 2006

  Motorola 6846 timer emulation.

**********************************************************************/

#ifndef MC6846
#define MC6846


/* ---------- configuration ------------ */

typedef struct _mc6846_interface mc6846_interface;
struct _mc6846_interface
{
  /* CPU write to the outside through chip */
  write8_handler out_port_func;  /* 8-bit output */
  write8_handler out_cp1_func;   /* 1-bit output */
  write8_handler out_cp2_func;   /* 1-bit output */
  
  /* CPU read from the outside through chip */
  read8_handler in_port_func; /* 8-bit input */
  
  /* asynchronous timer output to outside world */
  write8_handler out_cto_func; /* 1-bit output */
  
  /* timer interrupt */
  void ( * irq_func ) ( int state );

  /* CPU identifier (defines the clock rate) */
  int cpunum;
};


/* ---------- functions ------------ */

extern void mc6846_config ( const mc6846_interface *func );

/* reset by external signal */
extern void mc6846_reset ( void );

/* interface to CPU via address/data bus*/
extern READ8_HANDLER  ( mc6846_r );
extern WRITE8_HANDLER ( mc6846_w );

/* asynchronous write from outside world into interrupt-generating pins */
extern void mc6846_set_input_cp1 ( int data );
extern void mc6846_set_input_cp2 ( int data );

/* polling from outside world */
extern UINT8  mc6846_get_output_port ( void );
extern UINT8  mc6846_get_output_cto ( void );
extern UINT8  mc6846_get_output_cp2 ( void );

/* partial access to internal state */
extern UINT16 mc6846_get_preset ( void ); /* timer interval - 1 in us */

#endif


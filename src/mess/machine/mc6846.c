/**********************************************************************

  Copyright (C) Antoine Mine' 2006

  Motorola 6846 emulation.

  The MC6846 chip provides ROM (2048 bytes), I/O (8-bit directional data port +
  2 control lines) and a programmable timer.
  It may be interfaced with a M6809 cpu.
  It is used in some Thomson computers.

  Not yet implemented:
  - external clock (CTC)
  - latching of port on CP1
  - gate input (CTG)
  - timer comparison modes (frequency and pulse width)
  - CP2 acknowledge modes

**********************************************************************/

#include "driver.h"
#include "timer.h"
#include "state.h"
#include "mc6846.h"

#define VERBOSE 0


/******************* internal chip data structure ******************/

static struct {
  
  const mc6846_interface* iface;
  
  /* registers */
  UINT8    csr;      /* 0,4: combination status register */
  UINT8    pcr;      /* 1:   peripheral control register */
  UINT8    ddr;      /* 2:   data direction register */
  UINT8    pdr;      /* 3:   peripheral data register (last cpu write) */
  UINT8    tcr;      /* 5:   timer control register */

  /* lines */
  UINT8 cp1;         /* 1-bit input */
  UINT8 cp2;         /* 1-bit input/output: last external write */
  UINT8 cp2_cpu;     /* last cpu write */
  UINT8 cto;         /* 1-bit timer output (unmasked) */

  /* internal state */
  UINT8  time_MSB; /* MSB buffer register */
  UINT8  csr0_to_be_cleared;
  UINT8  csr1_to_be_cleared;
  UINT8  csr2_to_be_cleared;
  UINT16 latch;   /* timer latch */
  UINT16 preset;  /* preset value */
  UINT8  timer_started;

  /* timers */
  mame_timer *interval; /* interval programmable timer */
  mame_timer *one_shot; /* 1-us x factor one-shot timer */

} mc6846;



/******************* utilitiy function and macros ********************/

#if VERBOSE
#define LOG(x)	logerror x
#else
#define LOG(x)
#endif

#define PORT							       \
  ((mc6846.pdr & mc6846.ddr) |					       \
   ((mc6846.iface->in_port_func ? mc6846.iface->in_port_func(0) : 0) & \
    ~mc6846.ddr))

#define CTO							\
  ((MODE == 0x30 || (mc6846.tcr & 0x80)) ? mc6846.cto : 0)

#define MODE (mc6846.tcr & 0x38)

#define FACTOR ((mc6846.tcr & 4) ? 8 : 1)


INLINE UINT16 mc6846_counter ( void )
{
  if ( mc6846.timer_started ) {
    mame_time delay = mame_timer_timeleft( mc6846.interval );
    return MAME_TIME_TO_CYCLES( mc6846.iface->cpunum, delay ) / FACTOR;
  }
  else return mc6846.preset;
}

INLINE void mc6846_update_irq( void ) 
{
  static int old_cif;
  int cif = 0;
  /* composite interrupt flag */
  if ( ( (mc6846.csr & 1) && (mc6846.tcr & 0x40) ) ||
       ( (mc6846.csr & 2) && (mc6846.pcr & 1) ) ||
       ( (mc6846.csr & 4) && (mc6846.pcr & 8) && ! (mc6846.pcr & 0x20) ) )
    cif = 1;
  if ( old_cif != cif ) { 
    LOG (( "%f: mc6846 interrupt %i (time=%i cp1=%i cp2=%i)\n", 
	   timer_get_time(), cif, 
	   mc6846.csr & 1, (mc6846.csr >> 1 ) & 1, (mc6846.csr >> 2 ) & 1 ));
    old_cif = cif;
  }
  if ( cif ) {
    mc6846.csr |= 0x80;
    if ( mc6846.iface->irq_func )
      mc6846.iface->irq_func( 1 );
  }
  else {
    mc6846.csr &= ~0x80;
    if ( mc6846.iface->irq_func )
      mc6846.iface->irq_func( 0 );
  }
}

INLINE void mc6846_update_cto ( void )
{
  int cto = CTO;
  static int old_cto;
  if ( cto != old_cto ) {
    LOG (( "%f: mc6846 CTO set to %i\n", timer_get_time(), cto ));
    old_cto = cto;
  }
  if ( mc6846.iface->out_cto_func ) mc6846.iface->out_cto_func( 0, cto );
}

INLINE void mc6846_timer_launch ( void )
{
  int delay = FACTOR * (mc6846.preset+1);
  LOG (( "%f: mc6846 timer launch called, mode=%i, preset=%i (x%i)\n",
	 timer_get_time(), 
	 MODE, mc6846.preset, FACTOR ));
  if ( ! (mc6846.tcr & 2) ) {
    logerror( "mc6846 external clock CTC not implemented\n" );
  }
  
  switch( MODE ) {
    
  case 0x00:
  case 0x10: /* continuous */
    mc6846.cto = 0;
    break;
    
  case 0x20: /* single-shot */
    mc6846.cto = 0;
    timer_reset( mc6846.one_shot, 
		 TIME_IN_CYCLES( FACTOR, mc6846.iface->cpunum ) );
    break;
    
  case 0x30:  /* cascaded single-shot */
    break;
    
  default:
    logerror( "mc6846 timer mode %i not implemented\n", MODE );
    timer_reset( mc6846.interval, TIME_NEVER );
    mc6846.timer_started = 0;
    return;
  }
  
  timer_reset( mc6846.interval, TIME_IN_CYCLES( delay, mc6846.iface->cpunum ) );
  mc6846.timer_started = 1;
  
  mc6846.csr &= ~1;
  mc6846_update_cto();
  mc6846_update_irq();
}



/******************* timer callbacks *********************************/

static void mc6846_timer_expire ( int dummy )
{
  int delay = FACTOR * (mc6846.latch+1);

  LOG (( "%f: mc6846 timer expire called, mode=%i, latch=%i (x%i)\n",
	 timer_get_time(), MODE, mc6846.latch, FACTOR ));

  /* latch => counter */
  mc6846.preset = mc6846.latch;

  if ( ! (mc6846.tcr & 2) )
    logerror( "mc6846 external clock CTC not implemented\n" );

  switch ( MODE ) {
  case 0x00:
  case 0x10: /* continuous */
    mc6846.cto = 1 ^ mc6846.cto;
    break;

  case 0x20: /* single-shot */
    mc6846.cto = 0;
    break;
    
  case 0x30:  /* cascaded single-shot */
    mc6846.cto = ( mc6846.tcr & 0x80 ) ? 1 : 0;
    break;

  default:
    logerror( "mc6846 timer mode %i not implemented\n", MODE );
    timer_reset( mc6846.interval, TIME_NEVER );
    mc6846.timer_started = 0;
    return;
  }

  timer_reset( mc6846.interval, TIME_IN_CYCLES( delay, mc6846.iface->cpunum ) );

  mc6846.csr |= 1;
  mc6846_update_cto();
  mc6846_update_irq();
}

static void mc6846_timer_one_shot ( int dummy )
{
  LOG (( "%f: mc6846 timer one shot called\n", timer_get_time() ));
  /* 1 micro second after one-shot launch, we put cto to high */
  mc6846.cto = 1;
  mc6846_update_cto();
}



/************************** CPU interface ****************************/

READ8_HANDLER ( mc6846_r )
{
  switch ( offset ) {
  case 0:
  case 4:
    LOG (( "$%04x %f: mc6846 CSR read $%02X intr=%i (timer=%i, cp1=%i, cp2=%i)\n",
	   activecpu_get_previouspc(), timer_get_time(), 
	   mc6846.csr, (mc6846.csr >> 7) & 1, 
	   mc6846.csr & 1, (mc6846.csr >> 1) & 1, (mc6846.csr >> 2) & 1 ));
    mc6846.csr0_to_be_cleared = mc6846.csr & 1;
    mc6846.csr1_to_be_cleared = mc6846.csr & 2;
    mc6846.csr2_to_be_cleared = mc6846.csr & 4;
    return mc6846.csr;
    
  case 1: 
    LOG (( "$%04x %f: mc6846 PCR read $%02X\n", 
	   activecpu_get_previouspc(), timer_get_time(), mc6846.pcr ));
    return mc6846.pcr;

  case 2: 
    LOG (( "$%04x %f: mc6846 DDR read $%02X\n", 
	   activecpu_get_previouspc(), timer_get_time(), mc6846.ddr ));
    return mc6846.ddr;

  case 3:
    LOG (( "$%04x %f: mc6846 PORT read $%02X\n", 
	   activecpu_get_previouspc(), timer_get_time(), PORT ));
    if ( ! (mc6846.pcr & 0x80) ) {
      if ( mc6846.csr1_to_be_cleared ) mc6846.csr &= ~2;
      if ( mc6846.csr2_to_be_cleared ) mc6846.csr &= ~4;
      mc6846_update_irq();
      mc6846.csr1_to_be_cleared = 0;
      mc6846.csr2_to_be_cleared = 0;
    }
    return PORT;

  case 5:
    LOG (( "$%04x %f: mc6846 TCR read $%02X\n", 
	   activecpu_get_previouspc(), timer_get_time(), mc6846.tcr ));
    return mc6846.tcr;

  case 6:
    LOG (( "$%04x %f: mc6846 COUNTER hi read $%02X\n", 
	   activecpu_get_previouspc(), timer_get_time(), 
	   mc6846_counter() >> 8 ));
    if ( mc6846.csr0_to_be_cleared ) {
      mc6846.csr &= ~1;
      mc6846_update_irq();
    }
    mc6846.csr0_to_be_cleared = 0;
    return mc6846_counter() >> 8;

  case 7:
    LOG (( "$%04x %f: mc6846 COUNTER low read $%02X\n", 
	   activecpu_get_previouspc(), timer_get_time(), 
	   mc6846_counter() & 0xff ));
    if ( mc6846.csr0_to_be_cleared ) {
      mc6846.csr &= ~1;
      mc6846_update_irq();
    }
    mc6846.csr0_to_be_cleared = 0;
    return mc6846_counter() & 0xff;

 default:
   logerror( "$%04x mc6846 invalid read offset %i\n", 
	     activecpu_get_previouspc(), offset );
  }
  return 0;
}

WRITE8_HANDLER ( mc6846_w )
{
  switch ( offset ) {
  case 0:
  case 4:
    /* CSR is read-only */
    break;

  case 1:
    {
#if VERBOSE
      const char* cp2[8] = { 
	"in,neg-edge", "in,neg-edge,intr", "in,pos-edge", "in,pos-edge,intr", 
	"out,intr-ack", "out,i/o-ack", "out,0", "out,1" 
      };
      const char* cp1[8] = {
	"neg-edge", "neg-edge,intr", "pos-edge", "pos-edge,intr",
 	"latched,neg-edge", "latched,neg-edge,intr", 
	"latcged,pos-edge", "latcged,pos-edge,intr" 
      };
      LOG (( "$%04x %f: mc6846 PCR write $%02X reset=%i cp2=%s cp1=%s\n", 
	     activecpu_get_previouspc(), timer_get_time(), data,
	     (data >> 7) & 1, cp2[ (data >> 3) & 7 ], cp1[ data & 7 ] ));
#endif
    }
    mc6846.pcr = data;
    if ( data & 0x80 ) { /* data reset */
      mc6846.pdr = 0;
      mc6846.ddr = 0;
      mc6846.csr &= ~6;
      mc6846_update_irq();
    }
    if ( data & 4 )
      logerror( "$%04x mc6846 CP1 latching not implemented\n",
		activecpu_get_previouspc() );
    if (data & 0x20) {
      if (data & 0x10) {
	mc6846.cp2_cpu = (data >> 3) & 1;
	if ( mc6846.iface->out_cp2_func )
	  mc6846.iface->out_cp2_func( 0, mc6846.cp2_cpu );
      }
      else
	logerror( "$%04x mc6846 acknowledge not implemented\n",
		  activecpu_get_previouspc() );
    }
    break;

  case 2:
    LOG (( "$%04x %f: mc6846 DDR write $%02X\n", 
	   activecpu_get_previouspc(), timer_get_time(), data ));
    if ( ! (mc6846.pcr & 0x80) ) {
      mc6846.ddr = data;
      if ( mc6846.iface->out_port_func ) 
	mc6846.iface->out_port_func( 0, mc6846.pdr & mc6846.ddr );
    }
    break;

  case 3:
    LOG (( "$%04x %f: mc6846 PORT write $%02X (mask=$%02X)\n",
	   activecpu_get_previouspc(), timer_get_time(), data,mc6846.ddr ));
    if ( ! (mc6846.pcr & 0x80) ) {
      mc6846.pdr = data;
      if ( mc6846.iface->out_port_func ) 
	mc6846.iface->out_port_func( 0, mc6846.pdr & mc6846.ddr );
      if ( mc6846.csr1_to_be_cleared && (mc6846.csr & 2) ) {
	mc6846.csr &= ~2;
	LOG (( "$%04x %f: mc6846 CP1 intr reset\n", 
	       activecpu_get_previouspc(), timer_get_time() ));
     }
      if ( mc6846.csr2_to_be_cleared && (mc6846.csr & 4) ) {
	mc6846.csr &= ~4;
	LOG (( "$%04x %f: mc6846 CP2 intr reset\n", 
	       activecpu_get_previouspc(), timer_get_time() ));
      }
      mc6846.csr1_to_be_cleared = 0;
      mc6846.csr2_to_be_cleared = 0;
      mc6846_update_irq();
    }
    break;

  case 5:
    {
#if VERBOSE
      const char* mode[8] = { 
	"continuous", "cascaded", "continuous", "one-shot", 
	"freq-cmp", "freq-cmp", "pulse-cmp", "pulse-cmp"
      };
      LOG (( "$%04x %f: mc6846 TCR write $%02X reset=%i clock=%s scale=%i mode=%s out=%s\n", 
	     activecpu_get_previouspc(), timer_get_time(), data,
	     (data >> 7) & 1, (data & 0x40) ? "extern" : "sys",
	     (data & 0x40) ? 1 : 8, mode[ (data >> 1) & 7 ],
	     (data & 1) ? "enabled" : "0" ));
#endif
      mc6846.tcr = data;
      if ( mc6846.tcr & 1 ) {
	/* timer preset = initialization without launch */
	mc6846.preset = mc6846.latch;
	mc6846.csr &= ~1;
	if ( MODE != 0x30 ) mc6846.cto = 0;
	mc6846_update_cto();
	timer_reset( mc6846.interval, TIME_NEVER );
	timer_reset( mc6846.one_shot, TIME_NEVER );
	mc6846.timer_started = 0;
      }
      else {
	/* timer launch */
	if ( ! mc6846.timer_started ) mc6846_timer_launch();
      }
      mc6846_update_irq();
    }
    break;
    
  case 6:
    mc6846.time_MSB = data;
    break;

  case 7:
    mc6846.latch = ( ((UINT16) mc6846.time_MSB) << 8 ) + data;
    LOG (( "$%04x %f: mc6846 COUNT write %i\n", 
	   activecpu_get_previouspc(), timer_get_time(), mc6846.latch  ));
    if (!(mc6846.tcr & 0x38)) {
      /* timer initialization */
      mc6846.preset = mc6846.latch;
      mc6846.csr &= ~1;
      mc6846_update_irq();
      mc6846.cto = 0;
      mc6846_update_cto();
      /* launch only if started */
      if (!(mc6846.tcr & 1)) mc6846_timer_launch();
    }
    break;
 
  default:
    logerror( "$%04x mc6846 invalid write offset %i\n", 
	      activecpu_get_previouspc(), offset );
  }
}



/******************** outside world interface ************************/

void mc6846_set_input_cp1 ( int data )
{
  data = (data != 0 );
  if ( data == mc6846.cp1 ) return;
  mc6846.cp1 = data;
  LOG (( "%f: mc6846 input CP1 set to %i\n",  timer_get_time(), data ));
  if (( data &&  (mc6846.pcr & 2)) ||
      (!data && !(mc6846.pcr & 2))) {
    mc6846.csr |= 2;
    mc6846_update_irq();
  }
}

void mc6846_set_input_cp2 ( int data )
{
  data = (data != 0 );
  if ( data == mc6846.cp2 ) return;
  mc6846.cp2 = data;
  LOG (( "%f: mc6846 input CP2 set to %i\n", timer_get_time(), data ));
  if (mc6846.pcr & 0x20) {
    if (( data &&  (mc6846.pcr & 0x10)) ||
	(!data && !(mc6846.pcr & 0x10))) {
      mc6846.csr |= 4;
      mc6846_update_irq();
    }    
  }
}


/************************ accessors **********************************/

UINT8 mc6846_get_output_port ( void )
{
  return PORT;
}

UINT8 mc6846_get_output_cto ( void )
{
  return CTO;
}

UINT8 mc6846_get_output_cp2 ( void )
{
  return mc6846.cp2_cpu;
}

UINT16 mc6846_get_preset ( void )
{
  return mc6846.preset;
}


/************************ reset *****************************/

void mc6846_reset ( void )
{
  LOG (( "mc6846_reset\n" ));
  mc6846.cto   = 0;
  mc6846.csr   = 0;
  mc6846.pcr   = 0x80;
  mc6846.ddr   = 0;
  mc6846.pdr   = 0;
  mc6846.tcr   = 1;
  mc6846.cp1   = 0;
  mc6846.cp2   = 0;
  mc6846.cp2_cpu  = 0;
  mc6846.latch    = 0xffff;
  mc6846.preset   = 0xffff;
  mc6846.time_MSB = 0;
  mc6846.csr0_to_be_cleared = 0;
  mc6846.csr1_to_be_cleared = 0;
  mc6846.csr2_to_be_cleared = 0;
  mc6846.timer_started = 0;
  timer_reset( mc6846.interval, TIME_NEVER );
  timer_reset( mc6846.one_shot, TIME_NEVER );
}



/************************** configuration ****************************/

void mc6846_config ( const mc6846_interface* iface )
{
  mc6846.iface = iface;
  mc6846.interval = mame_timer_alloc( mc6846_timer_expire );
  mc6846.one_shot = mame_timer_alloc( mc6846_timer_one_shot );
  
  state_save_register_item( "mc6846", 0, mc6846.csr );
  state_save_register_item( "mc6846", 0, mc6846.pcr );
  state_save_register_item( "mc6846", 0, mc6846.ddr );
  state_save_register_item( "mc6846", 0, mc6846.pdr );
  state_save_register_item( "mc6846", 0, mc6846.tcr );
  state_save_register_item( "mc6846", 0, mc6846.cp1 );
  state_save_register_item( "mc6846", 0, mc6846.cp2 );
  state_save_register_item( "mc6846", 0, mc6846.cp2_cpu );
  state_save_register_item( "mc6846", 0, mc6846.cto );
  state_save_register_item( "mc6846", 0, mc6846.time_MSB );
  state_save_register_item( "mc6846", 0, mc6846.csr0_to_be_cleared );
  state_save_register_item( "mc6846", 0, mc6846.csr1_to_be_cleared );
  state_save_register_item( "mc6846", 0, mc6846.csr2_to_be_cleared );
  state_save_register_item( "mc6846", 0, mc6846.latch );
  state_save_register_item( "mc6846", 0, mc6846.preset );
  state_save_register_item( "mc6846", 0, mc6846.timer_started );

  mc6846_reset();
}

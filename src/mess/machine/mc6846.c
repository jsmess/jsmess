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
#include "mc6846.h"

#define VERBOSE 0



/******************* internal chip data structure ******************/


typedef struct
{

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
	emu_timer *interval; /* interval programmable timer */
	emu_timer *one_shot; /* 1-us x factor one-shot timer */

} mc6846_t;



/******************* utility function and macros ********************/

#define LOG(x) do { if (VERBOSE) logerror x; } while (0)

#define PORT								\
	((mc6846->pdr & mc6846->ddr) |					\
	 ((mc6846->iface->in_port_func ? mc6846->iface->in_port_func( device, 0 ) : 0) & \
	  ~mc6846->ddr))

#define CTO								\
	((MODE == 0x30 || (mc6846->tcr & 0x80)) ? mc6846->cto : 0)

#define MODE (mc6846->tcr & 0x38)

#define FACTOR ((mc6846->tcr & 4) ? 8 : 1)



INLINE mc6846_t* get_safe_token( const device_config *device ) 
{
	assert( device != NULL );
	assert( device->token != NULL );
	assert( device->type == DEVICE_GET_INFO_NAME( mc6846 ) );
	return (mc6846_t*) device->token;
}


INLINE UINT16 mc6846_counter( const device_config *device )
{
	mc6846_t* mc6846 = get_safe_token( device );
	if ( mc6846->timer_started )
	{
		attotime delay = timer_timeleft( mc6846->interval );
		return attotime_to_ticks( delay, 1000000 ) / FACTOR;
	}
	else
		return mc6846->preset;
}



INLINE void mc6846_update_irq( const device_config *device )
{
	mc6846_t* mc6846 = get_safe_token( device );
	static int old_cif;
	int cif = 0;
	/* composite interrupt flag */
	if ( ( (mc6846->csr & 1) && (mc6846->tcr & 0x40) ) ||
	     ( (mc6846->csr & 2) && (mc6846->pcr & 1) ) ||
	     ( (mc6846->csr & 4) && (mc6846->pcr & 8) && ! (mc6846->pcr & 0x20) ) )
		cif = 1;
	if ( old_cif != cif )
	{
		LOG (( "%f: mc6846 interrupt %i (time=%i cp1=%i cp2=%i)\n",
		       attotime_to_double(timer_get_time(device->machine)), cif,
		       mc6846->csr & 1, (mc6846->csr >> 1 ) & 1, (mc6846->csr >> 2 ) & 1 ));
		old_cif = cif;
	}
	if ( cif )
	{
		mc6846->csr |= 0x80;
		if ( mc6846->iface->irq_func )
			mc6846->iface->irq_func( device, 1 );
	}
	else
	{
		mc6846->csr &= ~0x80;
		if ( mc6846->iface->irq_func )
			mc6846->iface->irq_func( device, 0 );
	}
}



INLINE void mc6846_update_cto ( const device_config *device )
{
	mc6846_t* mc6846 = get_safe_token( device );
	int cto = CTO;
	static int old_cto;
	if ( cto != old_cto )
	{
		LOG (( "%f: mc6846 CTO set to %i\n", attotime_to_double(timer_get_time(device->machine)), cto ));
		old_cto = cto;
	}
	if ( mc6846->iface->out_cto_func )
		mc6846->iface->out_cto_func( device, 0, cto );
}



INLINE void mc6846_timer_launch ( const device_config *device )
{
	mc6846_t* mc6846 = get_safe_token( device );
	int delay = FACTOR * (mc6846->preset+1);
	LOG (( "%f: mc6846 timer launch called, mode=%i, preset=%i (x%i)\n", attotime_to_double(timer_get_time(device->machine)), MODE, mc6846->preset, FACTOR ));

	if ( ! (mc6846->tcr & 2) )
	{
		logerror( "mc6846 external clock CTC not implemented\n" );
	}

	switch( MODE )
	{

	case 0x00:
	case 0x10: /* continuous */
		mc6846->cto = 0;
		break;

	case 0x20: /* single-shot */
		mc6846->cto = 0;
		timer_reset( mc6846->one_shot, ATTOTIME_IN_USEC(FACTOR) );
		break;

	case 0x30:  /* cascaded single-shot */
		break;

	default:
		logerror( "mc6846 timer mode %i not implemented\n", MODE );
		timer_reset( mc6846->interval, attotime_never );
		mc6846->timer_started = 0;
		return;
	}

	timer_reset( mc6846->interval, ATTOTIME_IN_USEC(delay) );
	mc6846->timer_started = 1;

	mc6846->csr &= ~1;
	mc6846_update_cto( device );
	mc6846_update_irq( device );
}



/******************* timer callbacks *********************************/

static TIMER_CALLBACK( mc6846_timer_expire )
{
	const device_config* device = (const device_config*) ptr;
	mc6846_t* mc6846 = get_safe_token( device );
	int delay = FACTOR * (mc6846->latch+1);

	LOG (( "%f: mc6846 timer expire called, mode=%i, latch=%i (x%i)\n", attotime_to_double(timer_get_time(device->machine)), MODE, mc6846->latch, FACTOR ));

	/* latch => counter */
	mc6846->preset = mc6846->latch;

	if ( ! (mc6846->tcr & 2) )
		logerror( "mc6846 external clock CTC not implemented\n" );

	switch ( MODE )
	{
	case 0x00:
	case 0x10: /* continuous */
		mc6846->cto = 1 ^ mc6846->cto;
		break;

	case 0x20: /* single-shot */
		mc6846->cto = 0;
		break;

	case 0x30:  /* cascaded single-shot */
		mc6846->cto = ( mc6846->tcr & 0x80 ) ? 1 : 0;
		break;

	default:
		logerror( "mc6846 timer mode %i not implemented\n", MODE );
		timer_reset( mc6846->interval, attotime_never );
		mc6846->timer_started = 0;
		return;
	}

	timer_reset( mc6846->interval, ATTOTIME_IN_USEC(delay) );

	mc6846->csr |= 1;
	mc6846_update_cto( device );
	mc6846_update_irq( device );
}



static TIMER_CALLBACK( mc6846_timer_one_shot )
{
	const device_config* device = (const device_config*) ptr;
	mc6846_t* mc6846 = get_safe_token( device );
	LOG (( "%f: mc6846 timer one shot called\n", attotime_to_double(timer_get_time(device->machine)) ));

	/* 1 micro second after one-shot launch, we put cto to high */
	mc6846->cto = 1;
	mc6846_update_cto( device );
}



/************************** CPU interface ****************************/


READ8_DEVICE_HANDLER ( mc6846_r )
{
	mc6846_t* mc6846 = get_safe_token( device );
	switch ( offset )
	{
	case 0:
	case 4:
		LOG (( "$%04x %f: mc6846 CSR read $%02X intr=%i (timer=%i, cp1=%i, cp2=%i)\n",
		       cpu_get_previouspc( device->machine->firstcpu ), attotime_to_double(timer_get_time(device->machine)),
		       mc6846->csr, (mc6846->csr >> 7) & 1,
		       mc6846->csr & 1, (mc6846->csr >> 1) & 1, (mc6846->csr >> 2) & 1 ));
		mc6846->csr0_to_be_cleared = mc6846->csr & 1;
		mc6846->csr1_to_be_cleared = mc6846->csr & 2;
		mc6846->csr2_to_be_cleared = mc6846->csr & 4;
		return mc6846->csr;

	case 1:
		LOG (( "$%04x %f: mc6846 PCR read $%02X\n", cpu_get_previouspc( device->machine->firstcpu ), attotime_to_double(timer_get_time(device->machine)), mc6846->pcr ));
		return mc6846->pcr;

	case 2:
		LOG (( "$%04x %f: mc6846 DDR read $%02X\n", cpu_get_previouspc( device->machine->firstcpu ), attotime_to_double(timer_get_time(device->machine)), mc6846->ddr ));
		return mc6846->ddr;

	case 3:
		LOG (( "$%04x %f: mc6846 PORT read $%02X\n", cpu_get_previouspc( device->machine->firstcpu ), attotime_to_double(timer_get_time(device->machine)), PORT ));
		if ( ! (mc6846->pcr & 0x80) )
		{
			if ( mc6846->csr1_to_be_cleared )
				mc6846->csr &= ~2;
			if ( mc6846->csr2_to_be_cleared )
				mc6846->csr &= ~4;
			mc6846_update_irq( device );
			mc6846->csr1_to_be_cleared = 0;
			mc6846->csr2_to_be_cleared = 0;
		}
		return PORT;

	case 5:
		LOG (( "$%04x %f: mc6846 TCR read $%02X\n",cpu_get_previouspc( device->machine->firstcpu ), attotime_to_double(timer_get_time(device->machine)), mc6846->tcr ));
		return mc6846->tcr;

	case 6:
		LOG (( "$%04x %f: mc6846 COUNTER hi read $%02X\n", cpu_get_previouspc( device->machine->firstcpu ), attotime_to_double(timer_get_time(device->machine)), mc6846_counter( device ) >> 8 ));
		if ( mc6846->csr0_to_be_cleared )
		{
			mc6846->csr &= ~1;
			mc6846_update_irq( device );
		}
		mc6846->csr0_to_be_cleared = 0;
		return mc6846_counter( device ) >> 8;

	case 7:
		LOG (( "$%04x %f: mc6846 COUNTER low read $%02X\n", cpu_get_previouspc( device->machine->firstcpu ), attotime_to_double(timer_get_time(device->machine)), mc6846_counter( device ) & 0xff ));
		if ( mc6846->csr0_to_be_cleared )
		{
			mc6846->csr &= ~1;
			mc6846_update_irq( device );
		}
		mc6846->csr0_to_be_cleared = 0;
		return mc6846_counter( device ) & 0xff;

	default:
		logerror( "$%04x mc6846 invalid read offset %i\n", cpu_get_previouspc( device->machine->firstcpu ), offset );
	}
	return 0;
}



WRITE8_DEVICE_HANDLER ( mc6846_w )
{
	mc6846_t* mc6846 = get_safe_token( device );
	switch ( offset )
	{
	case 0:
	case 4:
		/* CSR is read-only */
		break;

	case 1:
	{
		static const char *const cp2[8] =
			{
			"in,neg-edge", "in,neg-edge,intr", "in,pos-edge", "in,pos-edge,intr",
			"out,intr-ack", "out,i/o-ack", "out,0", "out,1"
		};
		static const char *const cp1[8] = {
			"neg-edge", "neg-edge,intr", "pos-edge", "pos-edge,intr",
			"latched,neg-edge", "latched,neg-edge,intr",
			"latcged,pos-edge", "latcged,pos-edge,intr"
		};
		LOG (( "$%04x %f: mc6846 PCR write $%02X reset=%i cp2=%s cp1=%s\n",
		       cpu_get_previouspc( device->machine->firstcpu ), attotime_to_double(timer_get_time(device->machine)), data,
		       (data >> 7) & 1, cp2[ (data >> 3) & 7 ], cp1[ data & 7 ] ));

	}
	mc6846->pcr = data;
	if ( data & 0x80 )
	{      /* data reset */
		mc6846->pdr = 0;
		mc6846->ddr = 0;
		mc6846->csr &= ~6;
		mc6846_update_irq( device );
	}
	if ( data & 4 )
		logerror( "$%04x mc6846 CP1 latching not implemented\n", cpu_get_previouspc( device->machine->firstcpu ) );
	if (data & 0x20)
	{
		if (data & 0x10)
		{
			mc6846->cp2_cpu = (data >> 3) & 1;
			if ( mc6846->iface->out_cp2_func )
				mc6846->iface->out_cp2_func( device, 0, mc6846->cp2_cpu );
		}
		else
			logerror( "$%04x mc6846 acknowledge not implemented\n", cpu_get_previouspc( device->machine->firstcpu ) );
	}
	break;

	case 2:
		LOG (( "$%04x %f: mc6846 DDR write $%02X\n", cpu_get_previouspc( device->machine->firstcpu ), attotime_to_double(timer_get_time(device->machine)), data ));
		if ( ! (mc6846->pcr & 0x80) )
		{
			mc6846->ddr = data;
			if ( mc6846->iface->out_port_func )
				mc6846->iface->out_port_func( device, 0, mc6846->pdr & mc6846->ddr );
		}
		break;

	case 3:
		LOG (( "$%04x %f: mc6846 PORT write $%02X (mask=$%02X)\n", cpu_get_previouspc( device->machine->firstcpu ), attotime_to_double(timer_get_time(device->machine)), data,mc6846->ddr ));
		if ( ! (mc6846->pcr & 0x80) )
		{
			mc6846->pdr = data;
			if ( mc6846->iface->out_port_func )
				mc6846->iface->out_port_func( device, 0, mc6846->pdr & mc6846->ddr );
			if ( mc6846->csr1_to_be_cleared && (mc6846->csr & 2) )
			{
				mc6846->csr &= ~2;
				LOG (( "$%04x %f: mc6846 CP1 intr reset\n", cpu_get_previouspc( device->machine->firstcpu ), attotime_to_double(timer_get_time(device->machine)) ));
			}
			if ( mc6846->csr2_to_be_cleared && (mc6846->csr & 4) )
			{
				mc6846->csr &= ~4;
				LOG (( "$%04x %f: mc6846 CP2 intr reset\n", cpu_get_previouspc( device->machine->firstcpu ), attotime_to_double(timer_get_time(device->machine)) ));
			}
			mc6846->csr1_to_be_cleared = 0;
			mc6846->csr2_to_be_cleared = 0;
			mc6846_update_irq( device );
		}
		break;

	case 5:
	{
		static const char *const mode[8] =
			{
				"continuous", "cascaded", "continuous", "one-shot",
				"freq-cmp", "freq-cmp", "pulse-cmp", "pulse-cmp"
			};
		LOG (( "$%04x %f: mc6846 TCR write $%02X reset=%i clock=%s scale=%i mode=%s out=%s\n",
		       cpu_get_previouspc( device->machine->firstcpu ), attotime_to_double(timer_get_time(device->machine)), data,
		       (data >> 7) & 1, (data & 0x40) ? "extern" : "sys",
		       (data & 0x40) ? 1 : 8, mode[ (data >> 1) & 7 ],
		       (data & 1) ? "enabled" : "0" ));

		mc6846->tcr = data;
		if ( mc6846->tcr & 1 )
		{
			/* timer preset = initialization without launch */
			mc6846->preset = mc6846->latch;
			mc6846->csr &= ~1;
			if ( MODE != 0x30 )
				mc6846->cto = 0;
			mc6846_update_cto( device );
			timer_reset( mc6846->interval, attotime_never );
			timer_reset( mc6846->one_shot, attotime_never );
			mc6846->timer_started = 0;
		}
		else
		{
			/* timer launch */
			if ( ! mc6846->timer_started )
				mc6846_timer_launch( device );
		}
		mc6846_update_irq( device );
	}
	break;

	case 6:
		mc6846->time_MSB = data;
		break;

	case 7:
		mc6846->latch = ( ((UINT16) mc6846->time_MSB) << 8 ) + data;
		LOG (( "$%04x %f: mc6846 COUNT write %i\n", cpu_get_previouspc( device->machine->firstcpu ), attotime_to_double(timer_get_time(device->machine)), mc6846->latch  ));
		if (!(mc6846->tcr & 0x38))
		{
			/* timer initialization */
			mc6846->preset = mc6846->latch;
			mc6846->csr &= ~1;
			mc6846_update_irq( device );
			mc6846->cto = 0;
			mc6846_update_cto( device );
			/* launch only if started */
			if (!(mc6846->tcr & 1))
				mc6846_timer_launch( device );
		}
		break;

	default:
		logerror( "$%04x mc6846 invalid write offset %i\n", cpu_get_previouspc( device->machine->firstcpu ), offset );
	}
}



/******************** outside world interface ************************/



void mc6846_set_input_cp1 ( const device_config *device, int data )
{
	mc6846_t* mc6846 = get_safe_token( device );
	data = (data != 0 );
	if ( data == mc6846->cp1 )
		return;
	mc6846->cp1 = data;
	LOG (( "%f: mc6846 input CP1 set to %i\n",  attotime_to_double(timer_get_time(device->machine)), data ));
	if (( data &&  (mc6846->pcr & 2)) || (!data && !(mc6846->pcr & 2)))
	{
		mc6846->csr |= 2;
		mc6846_update_irq( device );
	}
}

void mc6846_set_input_cp2 ( const device_config *device, int data )
{
	mc6846_t* mc6846 = get_safe_token( device );
	data = (data != 0 );
	if ( data == mc6846->cp2 )
		return;
	mc6846->cp2 = data;
	LOG (( "%f: mc6846 input CP2 set to %i\n", attotime_to_double(timer_get_time(device->machine)), data ));
	if (mc6846->pcr & 0x20)
	{
		if (( data &&  (mc6846->pcr & 0x10)) || (!data && !(mc6846->pcr & 0x10)))
		{
			mc6846->csr |= 4;
			mc6846_update_irq( device );
		}
	}
}



/************************ accessors **********************************/



UINT8 mc6846_get_output_port ( const device_config *device )
{
	mc6846_t* mc6846 = get_safe_token( device );
	return PORT;
}



UINT8 mc6846_get_output_cto ( const device_config *device )
{
	mc6846_t* mc6846 = get_safe_token( device );
	return CTO;
}



UINT8 mc6846_get_output_cp2 ( const device_config *device )
{
	mc6846_t* mc6846 = get_safe_token( device );
	return mc6846->cp2_cpu;
}



UINT16 mc6846_get_preset ( const device_config *device )
{
	mc6846_t* mc6846 = get_safe_token( device );
	return mc6846->preset;
}



/************************ reset *****************************/


static DEVICE_RESET( mc6846 )
{
	mc6846_t* mc6846 = get_safe_token( device );
	LOG (( "mc6846_reset\n" ));
	mc6846->cto   = 0;
	mc6846->csr   = 0;
	mc6846->pcr   = 0x80;
	mc6846->ddr   = 0;
	mc6846->pdr   = 0;
	mc6846->tcr   = 1;
	mc6846->cp1   = 0;
	mc6846->cp2   = 0;
	mc6846->cp2_cpu  = 0;
	mc6846->latch    = 0xffff;
	mc6846->preset   = 0xffff;
	mc6846->time_MSB = 0;
	mc6846->csr0_to_be_cleared = 0;
	mc6846->csr1_to_be_cleared = 0;
	mc6846->csr2_to_be_cleared = 0;
	mc6846->timer_started = 0;
	timer_reset( mc6846->interval, attotime_never );
	timer_reset( mc6846->one_shot, attotime_never );
}


/************************ start *****************************/

static DEVICE_START( mc6846 )
{
	mc6846_t* mc6846 = get_safe_token( device );

	mc6846->iface = device->static_config;
	mc6846->interval = timer_alloc( device->machine, mc6846_timer_expire , (void*) device );
	mc6846->one_shot = timer_alloc( device->machine, mc6846_timer_one_shot , (void*) device );

	state_save_register_item( device->machine, "mc6846", device->tag, 0, mc6846->csr );
	state_save_register_item( device->machine, "mc6846", device->tag, 0, mc6846->pcr );
	state_save_register_item( device->machine, "mc6846", device->tag, 0, mc6846->ddr );
	state_save_register_item( device->machine, "mc6846", device->tag, 0, mc6846->pdr );
	state_save_register_item( device->machine, "mc6846", device->tag, 0, mc6846->tcr );
	state_save_register_item( device->machine, "mc6846", device->tag, 0, mc6846->cp1 );
	state_save_register_item( device->machine, "mc6846", device->tag, 0, mc6846->cp2 );
	state_save_register_item( device->machine, "mc6846", device->tag, 0, mc6846->cp2_cpu );
	state_save_register_item( device->machine, "mc6846", device->tag, 0, mc6846->cto );
	state_save_register_item( device->machine, "mc6846", device->tag, 0, mc6846->time_MSB );
	state_save_register_item( device->machine, "mc6846", device->tag, 0, mc6846->csr0_to_be_cleared );
	state_save_register_item( device->machine, "mc6846", device->tag, 0, mc6846->csr1_to_be_cleared );
	state_save_register_item( device->machine, "mc6846", device->tag, 0, mc6846->csr2_to_be_cleared );
	state_save_register_item( device->machine, "mc6846", device->tag, 0, mc6846->latch );
	state_save_register_item( device->machine, "mc6846", device->tag, 0, mc6846->preset );
	state_save_register_item( device->machine, "mc6846", device->tag, 0, mc6846->timer_started );
}


/************************** configuration ****************************/


DEVICE_GET_INFO( mc6846 ) {
	switch ( state ) {
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:			info->i = sizeof(mc6846_t);			break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:		info->i = 0;					break;
		case DEVINFO_INT_CLASS:				info->i = DEVICE_CLASS_PERIPHERAL;		break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:				info->start = DEVICE_START_NAME(mc6846);	break;
		case DEVINFO_FCT_STOP:				/* nothing */					break;
		case DEVINFO_FCT_RESET:				info->reset = DEVICE_RESET_NAME(mc6846);	break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
   	        case DEVINFO_STR_NAME:				strcpy(info->s, "Motorola MC6846 programmable timer");	break;
		case DEVINFO_STR_FAMILY:			strcpy(info->s, "MC6846");				break;
		case DEVINFO_STR_VERSION:			strcpy(info->s, "1.00");				break;
		case DEVINFO_STR_SOURCE_FILE:			strcpy(info->s, __FILE__);				break;
		case DEVINFO_STR_CREDITS:			strcpy(info->s, "Copyright the MAME and MESS Teams");  break;
	}
}

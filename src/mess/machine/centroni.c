/*
  defines centronics/parallel port printer interface  

  provides a centronics printer simulation (sends output to IO_PRINTER)
*/
#include "includes/centroni.h"
#include "devices/printer.h"

typedef struct {
	CENTRONICS_CONFIG *config;
	UINT8 data;
	UINT8 control;
	double time_;
	void *timer;

	/* These bytes are used in the timer callback. When the timer callback
	is executed, the control value is updated with the new data. The mask
	defines the bits that are changing, and the data is the new data */
	/* The user defined callback in CENTRONICS_CONFIG is called with the 
	change. This allows systems that have interrupts triggered from the centronics
	interface to function correctly. e.g. Amstrad NC100 */
	/* mask of data to set in timer callback */
	UINT8 new_control_mask;
	/* data to set in timer callback */
	UINT8 new_control_data;
} CENTRONICS;

static CENTRONICS cent[3]={
	{ 0,0,0, 0.0 },
	{ 0,0,0, 0.0 },
	{ 0,0,0, 0.0 }
};

static void centronics_timer_callback(int nr);

void centronics_config(int nr, CENTRONICS_CONFIG *config)
{
	CENTRONICS *This=cent+nr;
	This->config=config;
	This->timer = timer_alloc(centronics_timer_callback);
}

void centronics_write_data(int nr, UINT8 data)
{
	CENTRONICS *This=cent+nr;
	This->data=data;
}

UINT8 centronics_read_data(int nr)
{
	CENTRONICS *This=cent+nr;
	return This->data;
}

/* execute user callback in CENTRONICS_CONFIG when state of control from printer
has changed */
static void centronics_timer_callback(int nr)
{
	CENTRONICS *This=cent+nr;

	/* update control state */
	This->control &=~This->new_control_mask;
	This->control |=This->new_control_data;

	/* if callback is specified, call it with the new state of the outputs from the printer */
	if (This->config->handshake_out)
		This->config->handshake_out(nr, This->control, This->new_control_mask);

	/* phase 2: schedule ack end */
	if ( This->control & CENTRONICS_ACKNOWLEDGE )
	{
		This->new_control_mask = CENTRONICS_ACKNOWLEDGE;
		This->new_control_data = 0;
		timer_adjust(This->timer, TIME_IN_USEC(2), nr, 0);
	}
	/* phase 3: end */
	else if ( This->control & CENTRONICS_NOT_BUSY )
	{
		timer_adjust(This->timer, TIME_NEVER, nr, 0);
	}
	/* phase 1: schedule not busy & ack */
	else
	{
		This->new_control_mask = CENTRONICS_NOT_BUSY | CENTRONICS_ACKNOWLEDGE;
		This->new_control_data = CENTRONICS_NOT_BUSY | CENTRONICS_ACKNOWLEDGE;
		timer_adjust(This->timer, TIME_IN_USEC(15), nr, 0);
	}
}

void centronics_write_handshake(int nr, int data, int mask)
{
	CENTRONICS *This=cent+nr;
	
	int neu=(data&mask)|(This->control&(~mask));
	
	if (neu & CENTRONICS_NO_RESET)
	{
	  /* strobe down */
	  if ( (This->control&CENTRONICS_STROBE) && !(neu&CENTRONICS_STROBE) )
	  {
			/* schedule busy */
			This->new_control_mask = CENTRONICS_NOT_BUSY;
			This->new_control_data = 0;
			timer_adjust(This->timer, TIME_IN_USEC(5), nr, 0);

			/* output */
			printer_output(image_from_devtype_and_index(IO_PRINTER, nr), This->data);
		}

	}
	This->control=neu;
}

int centronics_read_handshake(int nr)
{
	CENTRONICS *This=cent+nr;
	UINT8 data=0;

	/* state of busy */
	data |= (This->control & CENTRONICS_NOT_BUSY);

	if (This->config->type == PRINTER_IBM)
	{
		data |= CENTRONICS_ONLINE;
	}
	else
	{
		if (This->control & CENTRONICS_SELECT) 
			data|=CENTRONICS_ONLINE;
	}
	data |= CENTRONICS_NO_ERROR;
	if (!printer_status(image_from_devtype_and_index(IO_PRINTER, nr), 0))
		data |= CENTRONICS_NO_PAPER;

	/* state of acknowledge */
	data|=(This->control & CENTRONICS_ACKNOWLEDGE);


	return data;
}

CENTRONICS_DEVICE CENTRONICS_PRINTER_DEVICE= {
	NULL,
	centronics_write_data,
	centronics_read_handshake,
	centronics_write_handshake
};


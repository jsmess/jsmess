/*****************************************************************************
 *
 * machine/tc8521.h
 *
 * TOSHIBA TC8521 REAL TIME CLOCK
 *
 ****************************************************************************/

#ifndef TC8521_H_
#define TC8525_H_


/*----------- defined in machine/tc8521.c -----------*/

 READ8_HANDLER(tc8521_r);
WRITE8_HANDLER(tc8521_w);

struct tc8521_interface
{
	/* output of alarm */
	void (*alarm_output_callback)(int);
};

void tc8521_init(const struct tc8521_interface *);

void tc8521_load_stream(mame_file *file);
void tc8521_save_stream(mame_file *file);


#endif /* TC8521_H_ */

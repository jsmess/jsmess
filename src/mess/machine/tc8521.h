/*********************************************************************

	tc8521.h

	Toshiba TC8251 Real Time Clock code

*********************************************************************/

#ifndef __TC8521_H__
#define __TC8525_H__


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

struct tc8521_interface
{
	/* output of alarm */
	void (*alarm_output_callback)(int);
};



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

READ8_HANDLER(tc8521_r);
WRITE8_HANDLER(tc8521_w);

void tc8521_init(const struct tc8521_interface *);

void tc8521_load_stream(mame_file *file);
void tc8521_save_stream(mame_file *file);


#endif /* __TC8521_H__ */

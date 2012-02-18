/****************************************************************************

    TI-99 Mechatronic mouse with adapter
    See mecmouse.c for documentation

    Michael Zapf
    October 2010
    January 2012: rewritten as class

*****************************************************************************/

#ifndef __MECMOUSE__
#define __MECMOUSE__

#include "emu.h"

extern const device_type MECMOUSE;

class mecmouse_device : public device_t
{
public:
	mecmouse_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);
	void select(int selnow, int stick1, int stick2);
	int get_values();
	int get_values8(int mode);

protected:
	void device_start(void);
	void device_reset(void);

private:
	void poll(void);
	int m_select;
	int m_read_y;
	int m_x;
	int m_y;
	int m_x_buf;
	int m_y_buf;
	int m_last_mx;
	int m_last_my;
	void device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr);

	emu_timer	*m_poll_timer;
};

#define MCFG_MECMOUSE_ADD(_tag, _clock )	\
	MCFG_DEVICE_ADD(_tag, MECMOUSE, _clock)

#endif

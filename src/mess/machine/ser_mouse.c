/***************************************************************************

    machine/ser_mouse.c

    Code for emulating PC-style serial mouses

***************************************************************************/

#include "machine/ser_mouse.h"


const device_type SERIAL_MOUSE = &device_creator<serial_mouse_device>;

serial_mouse_device::serial_mouse_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
	: device_t(mconfig, SERIAL_MOUSE, "Serial Mouse", tag, owner, clock),
		device_rs232_port_interface(mconfig, *this)
{
}

void serial_mouse_device::device_start()
{
	m_owner = dynamic_cast<rs232_port_device *>(owner());
}

void serial_mouse_device::device_reset()
{
	m_head = m_tail = 0;
	m_timer = timer_alloc();
	m_old_dtr = m_old_rts = 0;
}

/* add data to queue */
void serial_mouse_device::queue_data(int data)
{
	m_queue[m_head] = data;
	m_head++;
	m_head %= 256;
}

/**************************************************************************
 *  Check for mouse moves and buttons. Build delta x/y packets
 **************************************************************************/
void serial_mouse_device::device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr)
{
	static int ox = 0, oy = 0;
	int nx,ny;
	int dx, dy, nb;
	int mbc;

	/* Do not get deltas or send packets if queue is not empty (Prevents drifting) */
	if (m_head==m_tail)
	{
		nx = input_port_read(*this, "ser_mouse_x");

		dx = nx - ox;
		if (dx<=-0x800) dx = nx + 0x1000 - ox; /* Prevent jumping */
		if (dx>=0x800) dx = nx - 0x1000 - ox;
		ox = nx;

		ny = input_port_read(*this, "ser_mouse_y");

		dy = ny - oy;
		if (dy<=-0x800) dy = ny + 0x1000 - oy;
		if (dy>=0x800) dy = ny - 0x1000 - oy;
		oy = ny;

		nb = input_port_read(*this, "ser_mouse_misc");
		if ((nb & 0x80) != 0)
		{
			m_protocol=TYPE_MOUSE_SYSTEMS;
		}
		else
		{
			m_protocol=TYPE_MICROSOFT_MOUSE;
		}
		mbc = nb^m_mb;
		m_mb = nb;

		/* check if there is any delta or mouse buttons changed */
		if ( (dx!=0) || (dy!=0) || (mbc!=0) )
		{
			switch (m_protocol)
			{

			default:
			case TYPE_MICROSOFT_MOUSE:
				{
					/* split deltas into packtes of -128..+127 max */
					do
					{
						UINT8 m0, m1, m2;
						int ddx = (dx < -128) ? -128 : (dx > 127) ? 127 : dx;
						int ddy = (dy < -128) ? -128 : (dy > 127) ? 127 : dy;
						m0 = 0x40 | ((nb << 4) & 0x30) | ((ddx >> 6) & 0x03) | ((ddy >> 4) & 0x0c);
						m1 = ddx & 0x3f;
						m2 = ddy & 0x3f;

						/* KT - changed to use a function */
						queue_data(m0 | 0x40);
						queue_data(m1 & 0x03f);
						queue_data(m2 & 0x03f);
						if ((mbc & 0x04) != 0)  /* If button 3 changed send extra byte */
							queue_data( (nb & 0x04) << 3);

						dx -= ddx;
						dy -= ddy;
					} while( dx || dy );
				}
				break;

				/* mouse systems mouse
                from "PC Mouse information" by Tomi Engdahl */

				/*
                The data is sent in 5 byte packets in following format:
                        D7      D6      D5      D4      D3      D2      D1      D0

                1.      1       0       0       0       0       LB      CB      RB
                2.      X7      X6      X5      X4      X3      X2      X1      X0
                3.      Y7      Y6      Y5      Y4      Y3      Y4      Y1      Y0
                4.      X7'     X6'     X5'     X4'     X3'     X2'     X1'     X0'
                5.      Y7'     Y6'     Y5'     Y4'     Y3'     Y4'     Y1'     Y0'

                LB is left button state (0=pressed, 1=released)
                CB is center button state (0=pressed, 1=released)
                RB is right button state (0=pressed, 1=released)
                X7-X0 movement in X direction since last packet in signed byte
                      format (-128..+127), positive direction right
                Y7-Y0 movement in Y direction since last packet in signed byte
                      format (-128..+127), positive direction up
                X7'-X0' movement in X direction since sending of X7-X0 packet in signed byte
                      format (-128..+127), positive direction right
                Y7'-Y0' movement in Y direction since sending of Y7-Y0 in signed byte
                      format (-128..+127), positive direction up

                The last two bytes in the packet (bytes 4 and 5) contains information about movement data changes which have occurred after data butes 2 and 3 have been sent. */

				case TYPE_MOUSE_SYSTEMS:
				{

					dy =-dy;

					do
					{
						int ddx = (dx < -128) ? -128 : (dx > 127) ? 127 : dx;
						int ddy = (dy < -128) ? -128 : (dy > 127) ? 127 : dy;

						/* KT - changed to use a function */
						queue_data(0x080 | ((((nb & 0x04) >> 1) + ((nb & 0x02) << 1) + (nb & 0x01)) ^ 0x07));
						queue_data(ddx);
						queue_data(ddy);
						/* for now... */
						queue_data(0);
						queue_data(0);
						dx -= ddx;
						dy -= ddy;
					} while( dx || dy );

				}
				break;
			}
		}
	}

	/* Send any data from this scan or any pending data from a previous scan */

	if( m_tail != m_head )
	{
		int data;

		data = m_queue[m_tail];
		m_rdata = data;
		m_owner->out_rx8(data);
		m_tail++;
		m_tail &= 255;
	}
}



/**************************************************************************
 *  Check for mouse control line changes and (de-)install timer
 **************************************************************************/

void serial_mouse_device::check_state()
{
	if((m_dtr == m_old_dtr) && (m_rts == m_old_rts)) return;

	if (m_dtr && m_rts)
	{
		/* RTS toggled */
		if (!m_old_rts && m_rts)
		{
			/* reset mouse */
			m_head = m_tail = m_mb = 0;

			if ((input_port_read(*this, "ser_mouse_misc") & 0x80) == 0 )
			{
				/* Identify as Microsoft 3 Button Mouse */
				m_queue[m_head] = 'M';
				m_head++;
				m_head %= 256;
				m_queue[m_head] = '3';
				m_head++;
				m_head %= 256;
			}
		}
		/* start a timer to scan the mouse input */
		m_timer->adjust(attotime::zero, 0, attotime::from_hz(240));
	}
	else
	{
		m_timer->adjust(attotime::zero);
		m_head = m_tail = 0;
	}
}



/**************************************************************************
 *  Mouse INPUT_PORT declarations
 **************************************************************************/

static INPUT_PORTS_START( ser_mouse )
	PORT_START( "ser_mouse_misc" )
	PORT_CONFNAME( 0x80, 0x80, "Mouse Protocol" )
	PORT_CONFSETTING( 0x80, "Mouse Systems" )
	PORT_CONFSETTING( 0x00, "Microsoft" )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_NAME("Mouse Left Button") PORT_CODE(MOUSECODE_BUTTON1)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON2) PORT_NAME("Mouse Middle Button") PORT_CODE(MOUSECODE_BUTTON3)
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON3) PORT_NAME("Mouse Right Button") PORT_CODE(MOUSECODE_BUTTON2)

	PORT_START( "ser_mouse_x" ) /* Mouse - X AXIS */
	PORT_BIT( 0xfff, 0x00, IPT_MOUSE_X) PORT_SENSITIVITY(100) PORT_KEYDELTA(0) PORT_PLAYER(1)

	PORT_START( "ser_mouse_y" ) /* Mouse - Y AXIS */
	PORT_BIT( 0xfff, 0x00, IPT_MOUSE_Y) PORT_SENSITIVITY(100) PORT_KEYDELTA(0) PORT_PLAYER(1)
INPUT_PORTS_END

ioport_constructor serial_mouse_device::device_input_ports() const
{
	return INPUT_PORTS_NAME(ser_mouse);
}

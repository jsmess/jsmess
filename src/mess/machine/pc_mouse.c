/***************************************************************************

    machine/pc_mouse.c

    Code for emulating PC-style serial mouses

***************************************************************************/

#include "emu.h"
#include "machine/ins8250.h"
#include "includes/pc_mouse.h"


static struct {

	PC_MOUSE_PROTOCOL protocol;

	device_t *ins8250;
	int inputs;

	UINT8 queue[256];
	UINT8 head, tail, mb;

	emu_timer *timer;

} pc_mouse;

static TIMER_CALLBACK(pc_mouse_scan);

void pc_mouse_initialise(running_machine &machine)
{
	pc_mouse.head = pc_mouse.tail = 0;
	pc_mouse.timer = machine.scheduler().timer_alloc(FUNC(pc_mouse_scan));
	pc_mouse.inputs=UART8250_HANDSHAKE_IN_DSR|UART8250_HANDSHAKE_IN_CTS;
	pc_mouse.ins8250 = NULL;
}

void pc_mouse_set_serial_port(device_t *ins8250)
{
	if ( pc_mouse.ins8250 != ins8250 ) {
		pc_mouse.ins8250 = ins8250;
		ins8250_handshake_in(pc_mouse.ins8250, pc_mouse.inputs);
	}
}

/* add data to queue */
static void	pc_mouse_queue_data(int data)
{
	pc_mouse.queue[pc_mouse.head] = data;
	pc_mouse.head++;
	pc_mouse.head %= 256;
}

/**************************************************************************
 *  Check for mouse moves and buttons. Build delta x/y packets
 **************************************************************************/
static TIMER_CALLBACK(pc_mouse_scan)
{
	static int ox = 0, oy = 0;
	int nx,ny;
	int dx, dy, nb;
	int mbc;

	/* Do not get deltas or send packets if queue is not empty (Prevents drifting) */
	if (pc_mouse.head==pc_mouse.tail)
	{
		nx = input_port_read(machine, "pc_mouse_x");

		dx = nx - ox;
		if (dx<=-0x800) dx = nx + 0x1000 - ox; /* Prevent jumping */
		if (dx>=0x800) dx = nx - 0x1000 - ox;
		ox = nx;

		ny = input_port_read(machine, "pc_mouse_y");

		dy = ny - oy;
		if (dy<=-0x800) dy = ny + 0x1000 - oy;
		if (dy>=0x800) dy = ny - 0x1000 - oy;
		oy = ny;

		nb = input_port_read(machine, "pc_mouse_misc");
		if ((nb & 0x80) != 0)
		{
			pc_mouse.protocol=TYPE_MOUSE_SYSTEMS;
		}
		else
		{
			pc_mouse.protocol=TYPE_MICROSOFT_MOUSE;
		}
		mbc = nb^pc_mouse.mb;
		pc_mouse.mb = nb;

		/* check if there is any delta or mouse buttons changed */
		if ( (dx!=0) || (dy!=0) || (mbc!=0) )
		{
			switch (pc_mouse.protocol)
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
						pc_mouse_queue_data(m0 | 0x40);
						pc_mouse_queue_data(m1 & 0x03f);
						pc_mouse_queue_data(m2 & 0x03f);
						if ((mbc & 0x04) != 0)  /* If button 3 changed send extra byte */
						{
						pc_mouse_queue_data( (nb & 0x04) << 3);
						}

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

                The last two bytes in the packet (bytes 4 and 5) contains information about movement data changes which have occured after data butes 2 and 3 have been sent. */

				case TYPE_MOUSE_SYSTEMS:
				{

					dy =-dy;

					do
					{
						int ddx = (dx < -128) ? -128 : (dx > 127) ? 127 : dx;
						int ddy = (dy < -128) ? -128 : (dy > 127) ? 127 : dy;

						/* KT - changed to use a function */
						pc_mouse_queue_data(0x080 | ((((nb & 0x04) >> 1) + ((nb & 0x02) << 1) + (nb & 0x01)) ^ 0x07));
						pc_mouse_queue_data(ddx);
						pc_mouse_queue_data(ddy);
						/* for now... */
						pc_mouse_queue_data(0);
						pc_mouse_queue_data(0);
						dx -= ddx;
						dy -= ddy;
					} while( dx || dy );

				}
				break;
			}
		}
	}

	/* Send any data from this scan or any pending data from a previous scan */

	if( pc_mouse.tail != pc_mouse.head )
	{
		int data;

		data = pc_mouse.queue[pc_mouse.tail];
		ins8250_receive(pc_mouse.ins8250, data);
		pc_mouse.tail++;
		pc_mouse.tail &= 255;
	}
}



/**************************************************************************
 *  Check for mouse control line changes and (de-)install timer
 **************************************************************************/

INS8250_HANDSHAKE_OUT( pc_mouse_handshake_in )
{
    int new_msr = 0x00;

	if (device!=pc_mouse.ins8250) return;

    /* check if mouse port has DTR set */
	if( data & UART8250_HANDSHAKE_OUT_DTR )
		new_msr |= UART8250_HANDSHAKE_IN_DSR;	/* set DSR */

	/* check if mouse port has RTS set */
	if( data & UART8250_HANDSHAKE_OUT_RTS )
		new_msr |= UART8250_HANDSHAKE_IN_CTS;	/* set CTS */

	/* CTS changed state? */
	if (((pc_mouse.inputs^new_msr) & UART8250_HANDSHAKE_IN_CTS)!=0)
	{
		/* CTS just went to 1? */
		if ((new_msr & 0x010)!=0)
		{
			/* set CTS is now 1 */

			/* reset mouse */
			pc_mouse.head = pc_mouse.tail = pc_mouse.mb = 0;

			if ((input_port_read(device->machine(), "pc_mouse_misc") & 0x80) == 0 )
			{
				/* Identify as Microsoft 3 Button Mouse */
				pc_mouse.queue[pc_mouse.head] = 'M';
				pc_mouse.head++;
				pc_mouse.head %= 256;
				pc_mouse.queue[pc_mouse.head] = '3';
				pc_mouse.head++;
				pc_mouse.head %= 256;
			}

			/* start a timer to scan the mouse input */
			pc_mouse.timer->adjust(attotime::zero, 0, attotime::from_hz(240));
		}
		else
		{
			/* CTS just went to 0 */
			pc_mouse.timer->adjust(attotime::zero);
			pc_mouse.head = pc_mouse.tail = 0;
		}
	}

	pc_mouse.inputs=new_msr;
	ins8250_handshake_in(pc_mouse.ins8250, new_msr);
}



/**************************************************************************
 *  Mouse INPUT_PORT declarations
 **************************************************************************/

INPUT_PORTS_START( pc_mouse_mousesystems )
	PORT_START( "pc_mouse_misc" )
	PORT_CONFNAME( 0x80, 0x80, "Mouse Protocol" )
	PORT_CONFSETTING( 0x80, "Mouse Systems" )
	PORT_CONFSETTING( 0x00, "Microsoft" )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_NAME("Mouse Left Button") PORT_CODE(MOUSECODE_BUTTON1)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON2) PORT_NAME("Mouse Middle Button") PORT_CODE(MOUSECODE_BUTTON3)
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON3) PORT_NAME("Mouse Right Button") PORT_CODE(MOUSECODE_BUTTON2)

	PORT_START( "pc_mouse_x" ) /* Mouse - X AXIS */
	PORT_BIT( 0xfff, 0x00, IPT_MOUSE_X) PORT_SENSITIVITY(100) PORT_KEYDELTA(0) PORT_PLAYER(1)

	PORT_START( "pc_mouse_y" ) /* Mouse - Y AXIS */
	PORT_BIT( 0xfff, 0x00, IPT_MOUSE_Y) PORT_SENSITIVITY(100) PORT_KEYDELTA(0) PORT_PLAYER(1)
INPUT_PORTS_END



INPUT_PORTS_START( pc_mouse_microsoft )
	PORT_START( "pc_mouse_misc" )
	PORT_CONFNAME( 0x80, 0x00, "Mouse Protocol" )
	PORT_CONFSETTING( 0x00, "Microsoft" )
	PORT_CONFSETTING( 0x80, "Mouse Systems" )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_NAME("Mouse Left Button") PORT_CODE(MOUSECODE_BUTTON1)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON2) PORT_NAME("Mouse Middle Button") PORT_CODE(MOUSECODE_BUTTON3)
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON3) PORT_NAME("Mouse Right Button") PORT_CODE(MOUSECODE_BUTTON2)

	PORT_START( "pc_mouse_x" ) /* Mouse - X AXIS */
	PORT_BIT( 0xfff, 0x00, IPT_MOUSE_X) PORT_SENSITIVITY(100) PORT_KEYDELTA(0) PORT_PLAYER(1)

	PORT_START( "pc_mouse_y" ) /* Mouse - Y AXIS */
	PORT_BIT( 0xfff, 0x00, IPT_MOUSE_Y) PORT_SENSITIVITY(100) PORT_KEYDELTA(0) PORT_PLAYER(1)
INPUT_PORTS_END



INPUT_PORTS_START( pc_mouse_none )
	PORT_START( "pc_mouse_misc" )      /* IN12 */
	PORT_BIT ( 0xffff, 0x0000, IPT_UNUSED )
	PORT_START( "pc_mouse_x" )      /* IN13 */
	PORT_BIT ( 0xffff, 0x0000, IPT_UNUSED )
	PORT_START( "pc_mouse_y" )      /* IN14 */
	PORT_BIT ( 0xffff, 0x0000, IPT_UNUSED )
INPUT_PORTS_END

#include "emu.h"
#include "includes/intv.h"


/* STIC variables */

READ16_HANDLER( intv_stic_r )
{
	intv_state *state = space->machine().driver_data<intv_state>();
	//logerror("%x = stic_r(%x)\n",0,offset);
	switch (offset)
	{
		case 0x18:
		case 0x19:
		case 0x1a:
		case 0x1b:
		case 0x1c:
		case 0x1d:
		case 0x1e:
		case 0x1f:
            return state->m_collision_registers[offset & 0x07];
		case 0x21:
			state->m_color_stack_mode = 1;
			//logerror("Setting color stack mode\n");
			break;
	}
	return 0;
}

WRITE16_HANDLER( intv_stic_w )
{
	intv_state *state = space->machine().driver_data<intv_state>();
	intv_sprite_type *s;

	//logerror("stic_w(%x) = %x\n",offset,data);
	switch (offset)
	{
		/* X Positions */
		case 0x00:
		case 0x01:
		case 0x02:
		case 0x03:
		case 0x04:
		case 0x05:
		case 0x06:
		case 0x07:
			s =  &state->m_sprite[offset & 0x07];
			s->doublex = !!(data & 0x0400);
			s->visible = !!(data & 0x0200);
			s->coll = !!(data & 0x0100);
			s->xpos = (data & 0xFF);
			break;
		/* Y Positions */
		case 0x08:
		case 0x09:
		case 0x0a:
		case 0x0b:
		case 0x0c:
		case 0x0d:
		case 0x0e:
		case 0x0f:
			s =  &state->m_sprite[offset & 0x07];
			s->yflip = !!(data & 0x0800);
			s->xflip = !!(data & 0x0400);
			s->quady = !!(data & 0x0200);
			s->doubley = !!(data & 0x0100);
			s->doubleyres = !!(data & 0x0080);
			s->ypos = (data & 0x7F);
			break;
		/* Attributes */
		case 0x10:
		case 0x11:
		case 0x12:
		case 0x13:
		case 0x14:
		case 0x15:
		case 0x16:
		case 0x17:
			s =  &state->m_sprite[offset & 0x07];
            s->behind_foreground = !!(data & 0x2000);
            s->grom = !(data & 0x0800);
			s->card = ((data >> 3) & 0xFF);
			s->color = ((data >> 9) & 0x08) | (data & 0x7);
			break;
		/* Collision Detection - TBD */
		case 0x18:
		case 0x19:
		case 0x1a:
		case 0x1b:
		case 0x1c:
		case 0x1d:
		case 0x1e:
		case 0x1f:
            //a MOB's own collision bit is *always* zero, even if a
            //one is poked into it
            data &= (~(1 << (offset & 0x07))) & 0x03FF;
            state->m_collision_registers[offset & 0x07] = data;
			break;
		/* Display enable */
		case 0x20:
			//logerror("***Writing a %x to the STIC handshake\n",data);
			state->m_stic_handshake = 1;
			break;
		/* Graphics Mode */
		case 0x21:
			state->m_color_stack_mode = 0;
			break;
		/* Color Stack */
		case 0x28:
		case 0x29:
		case 0x2a:
		case 0x2b:
			logerror("Setting color_stack[%x] = %x (%x)\n",offset&0x3,data&0xf,cpu_get_pc(&space->device()));
			state->m_color_stack[offset&0x3] = data&0xf;
			break;
		/* Border Color */
		case 0x2c:
			//logerror("***Writing a %x to the border color\n",data);
			state->m_border_color = data & 0xf;
			break;
		/* Framing */
		case 0x30:
			state->m_col_delay = data & 0x7;
			break;
		case 0x31:
			state->m_row_delay = data & 0x7;
			break;
		case 0x32:
			state->m_left_edge_inhibit = (data & 0x01);
			state->m_top_edge_inhibit = (data & 0x02)>>1;
			break;
	}
}

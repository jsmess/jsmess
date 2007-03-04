#include "driver.h"
#include "video/stic.h"
#include "includes/intv.h"

#include "cpuintrf.h"

/* STIC variables */
struct intv_sprite_type intv_sprite[8];
UINT8 intv_sprite_buffers[8][16][128];
int intv_collision_registers[8];
int intv_stic_handshake;
int intv_border_color;
int intv_color_stack[4];
int intv_color_stack_mode = 0; // for now
int intv_color_stack_offset = 0; // for now
int intv_col_delay = 0;  // for now
int intv_row_delay = 0;  // for now
int intv_left_edge_inhibit = 0;  // for now
int intv_top_edge_inhibit = 0;  // for now

READ16_HANDLER( stic_r )
{
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
            return intv_collision_registers[offset & 0x07];
		case 0x21:
			intv_color_stack_mode = 1;
			//logerror("Setting color stack mode\n");
			break;
	}
	return 0;
}

WRITE16_HANDLER( stic_w )
{
	struct intv_sprite_type *s;

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
			s =  &intv_sprite[offset & 0x07];
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
			s =  &intv_sprite[offset & 0x07];
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
			s =  &intv_sprite[offset & 0x07];
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
            intv_collision_registers[offset & 0x07] = data;
			break;
		/* Display enable */
		case 0x20:
			logerror("%g: WRITING TO STIC HANDSHAKE\n",timer_get_time());
			//logerror("***Writing a %x to the STIC handshake\n",data);
			intv_stic_handshake = 1;
			break;
		/* Graphics Mode */
		case 0x21:
			intv_color_stack_mode = 0;
			break;
		/* Color Stack */
		case 0x28:
		case 0x29:
		case 0x2a:
		case 0x2b:
			logerror("Setting color_stack[%x] = %x (%x)\n",offset&0x3,data&0xf,activecpu_get_pc());
			intv_color_stack[offset&0x3] = data&0xf;
			break;
		/* Border Color */
		case 0x2c:
			//logerror("***Writing a %x to the border color\n",data);
			intv_border_color = data & 0xf;
			break;
		/* Framing */
		case 0x30:
			logerror("%g: WRITING %d TO COL DELAY\n",timer_get_time(),data & 0x7);
			intv_col_delay = data & 0x7;
			break;
		case 0x31:
			intv_row_delay = data & 0x7;
			break;
		case 0x32:
			intv_left_edge_inhibit = (data & 0x01);
			intv_top_edge_inhibit = (data & 0x02)>>1;
			break;
	}
}

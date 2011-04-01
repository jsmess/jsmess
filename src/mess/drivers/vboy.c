/***************************************************************************

    Nintendo Virtual Boy

    12/05/2009 Skeleton driver.

    Great info at http://www.goliathindustries.com/vb/
    and http://www.vr32.de/modules/dokuwiki/doku.php?

    TODO:
    - finish V810 irq support;
    - understand irqs on this HW;

****************************************************************************/

#include "emu.h"
#include "cpu/v810/v810.h"
#include "imagedev/cartslot.h"
#include "vboy.lh"

typedef struct _vboy_regs_t vboy_regs_t;
struct _vboy_regs_t
{
	UINT32 lpc, lpc2, lpt, lpr;
	UINT32 khb, klb;
	UINT32 thb, tlb;
	UINT32 tcr, wcr, kcr;
};

typedef struct _vip_regs_t vip_regs_t;
struct _vip_regs_t
{

	UINT16 INTPND;
	UINT16 INTENB;
	UINT16 DPSTTS;
	UINT16 DPCTRL;
	UINT16 BRTA;
	UINT16 BRTB;
	UINT16 BRTC;
	UINT16 REST;
	UINT16 FRMCYC;
	UINT16 CTA;
	UINT16 XPSTTS;
	UINT16 XPCTRL;
	UINT16 VER;
	UINT16 SPT[4];
	UINT16 GPLT[4];
	UINT16 JPLT[4];
	UINT16 BKCOL;
};

class vboy_state : public driver_device
{
public:
	vboy_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT16 *m_font;
	UINT16 *m_bgmap;
	UINT16 *m_l_frame_0;
	UINT16 *m_l_frame_1;
	UINT16 *m_r_frame_0;
	UINT16 *m_r_frame_1;
	UINT16 *m_world;
	UINT16 *m_columntab1;
	UINT16 *m_columntab2;
	UINT16 *m_objects;
	vboy_regs_t m_vboy_regs;
	vip_regs_t m_vip_regs;
	bitmap_t *m_bg_map[16];
	bitmap_t *m_screen_output;
};

static READ32_HANDLER( port_02_read )
{
	vboy_state *state = space->machine().driver_data<vboy_state>();
	UINT32 value = 0x00;

	switch ((offset << 2))
	{
		case 0x10:	// KLB (Keypad Low Byte)
			value = state->m_vboy_regs.klb;	// 0x02 is always 1
			break;
		case 0x14:	// KHB (Keypad High Byte)
			value = state->m_vboy_regs.khb;
			break;
		case 0x20:	// TCR (Timer Control Reg)
			value = state->m_vboy_regs.tcr;
			break;
		case 0x24:	// WCR (Wait State Control Reg)
			value = state->m_vboy_regs.wcr;
			break;
		case 0x28:	// KCR (Keypad Control Reg)
			value = state->m_vboy_regs.kcr;
			break;
		case 0x00:	// LPC (Link Port Control Reg)
		case 0x04:	// LPC2 (Link Port Control Reg)
		case 0x08:	// LPT (Link Port Transmit)
		case 0x0c:	// LPR (Link Port Receive)
		case 0x18:	// TLB (Timer Low Byte)
		case 0x1c:	// THB (Timer High Byte)
		default:
			logerror("Unemulated read: offset %08x\n", 0x02000000 + (offset << 2));
			break;
	}
	return value;
}

static WRITE32_HANDLER( port_02_write )
{
	vboy_state *state = space->machine().driver_data<vboy_state>();

	switch (offset<<2)
	{
		case 0x0c:	// LPR (Link Port Receive)
		case 0x10:	// KLB (Keypad Low Byte)
		case 0x14:	// KHB (Keypad High Byte)
			//logerror("Ilegal write: offset %02x should be only read\n", offset);
			break;
		case 0x20:	// TCR (Timer Control Reg)
			state->m_vboy_regs.kcr = (data | 0xe0) & 0xfd;	// according to docs: bits 5, 6 & 7 are unused and set to 1, bit 1 is read only.
			break;
		case 0x24:	// WCR (Wait State Control Reg)
			state->m_vboy_regs.wcr = data | 0xfc;	// according to docs: bits 2 to 7 are unused and set to 1.
			break;
		case 0x28:	// KCR (Keypad Control Reg)
			if (data & 0x04 )
			{
				state->m_vboy_regs.klb = (data & 0x01) ? 0 : (input_port_read(space->machine(), "INPUT") & 0x00ff);
				state->m_vboy_regs.khb = (data & 0x01) ? 0 : (input_port_read(space->machine(), "INPUT") & 0xff00) >> 8;
			}
			else if (data & 0x20)
			{
				state->m_vboy_regs.klb = input_port_read(space->machine(), "INPUT") & 0x00ff;
				state->m_vboy_regs.khb = (input_port_read(space->machine(), "INPUT") & 0xff00) >> 8;
			}
			state->m_vboy_regs.kcr = (data | 0x48) & 0xfd;	// according to docs: bit 6 & bit 3 are unused and set to 1, bit 1 is read only.
			break;
		case 0x00:	// LPC (Link Port Control Reg)
		case 0x04:	// LPC2 (Link Port Control Reg)
		case 0x08:	// LPT (Link Port Transmit)
		case 0x18:	// TLB (Timer Low Byte)
		case 0x1c:	// THB (Timer High Byte)
		default:
			logerror("Unemulated write: offset %08x, data %04x\n", 0x02000000 + (offset << 2), data);
			break;
	}
}

static READ16_HANDLER( vip_r )
{
	vboy_state *state = space->machine().driver_data<vboy_state>();

	switch(offset << 1) {
		case 0x00:	//INTPND
					return state->m_vip_regs.INTPND & state->m_vip_regs.INTENB;
		case 0x02:	//INTENB
					return state->m_vip_regs.INTENB;
		case 0x04:	//INTCLR
					logerror("Error reading INTCLR\n");
					break;
		case 0x20:	//DPSTTS
					return state->m_vip_regs.DPSTTS;
		case 0x22:	//DPCTRL
					return state->m_vip_regs.DPCTRL;
		case 0x24:	//BRTA
					return state->m_vip_regs.BRTA;
		case 0x26:	//BRTB
					return state->m_vip_regs.BRTB;
		case 0x28:	//BRTC
					return state->m_vip_regs.BRTC;
		case 0x2A:	//REST
					return state->m_vip_regs.REST;
		case 0x2E:	//FRMCYC
					return state->m_vip_regs.FRMCYC;
		case 0x30:	//CTA
					return state->m_vip_regs.CTA;
		case 0x40:	//XPSTTS
					return state->m_vip_regs.XPSTTS;
		case 0x42:	//XPCTRL
					return state->m_vip_regs.XPCTRL;
		case 0x44:	//VER
					return state->m_vip_regs.VER;
		case 0x48:	//SPT0
					return state->m_vip_regs.SPT[0];
		case 0x4A:	//SPT1
					return state->m_vip_regs.SPT[1];
		case 0x4C:	//SPT2
					return state->m_vip_regs.SPT[2];
		case 0x4E:	//SPT3
					return state->m_vip_regs.SPT[3];
		case 0x60:	//GPLT0
					return state->m_vip_regs.GPLT[0];
		case 0x62:	//GPLT1
					return state->m_vip_regs.GPLT[1];
		case 0x64:	//GPLT2
					return state->m_vip_regs.GPLT[2];
		case 0x66:	//GPLT3
					return state->m_vip_regs.GPLT[3];
		case 0x68:	//JPLT0
					return state->m_vip_regs.JPLT[0];
		case 0x6A:	//JPLT1
					return state->m_vip_regs.JPLT[1];
		case 0x6C:	//JPLT2
					return state->m_vip_regs.JPLT[2];
		case 0x6E:	//JPLT3
					return state->m_vip_regs.JPLT[3];
		case 0x70:	//BKCOL
					return state->m_vip_regs.BKCOL;
		default:
					logerror("Unemulated read: addr %08x\n", offset * 2 + 0x0005f800);
					break;
	}
	return 0xffff;
}

static WRITE16_HANDLER( vip_w )
{
	vboy_state *state = space->machine().driver_data<vboy_state>();

	switch(offset << 1) {
		case 0x00:	//INTPND
					logerror("Error writing INTPND\n");
					break;
		case 0x02:	//INTENB
					state->m_vip_regs.INTENB = data;
					//printf("%04x ENB\n",data);
					break;
		case 0x04:	//INTCLR
					state->m_vip_regs.INTPND &= ~data;
					cputag_set_input_line(space->machine(), "maincpu", 4, CLEAR_LINE);
					//printf("%04x ACK\n",data);
					break;
		case 0x20:	//DPSTTS
					logerror("Error writing DPSTTS\n");
					break;
		case 0x22:	//DPCTRL
					state->m_vip_regs.DPCTRL = data;
					break;
		case 0x24:	//BRTA
					state->m_vip_regs.BRTA = data;
					palette_set_color_rgb(space->machine(), 1,(state->m_vip_regs.BRTA) & 0xff,0,0);
					break;
		case 0x26:	//BRTB
					state->m_vip_regs.BRTB = data;
					palette_set_color_rgb(space->machine(), 2,(state->m_vip_regs.BRTA + state->m_vip_regs.BRTB) & 0xff,0,0);
					break;
		case 0x28:	//BRTC
					state->m_vip_regs.BRTC = data;
					palette_set_color_rgb(space->machine(), 3,(state->m_vip_regs.BRTA + state->m_vip_regs.BRTB + state->m_vip_regs.BRTC) & 0xff,0,0);
					break;
		case 0x2A:	//REST
					state->m_vip_regs.REST = data;
					break;
		case 0x2E:	//FRMCYC
					state->m_vip_regs.FRMCYC = data;
					break;
		case 0x30:	//CTA
					state->m_vip_regs.CTA = data;
					break;
		case 0x40:	//XPSTTS
					logerror("Error writing XPSTTS\n");
					break;
		case 0x42:	//XPCTRL
					state->m_vip_regs.XPCTRL = data;
					break;
		case 0x44:	//VER
					state->m_vip_regs.VER = data;
					break;
		case 0x48:	//SPT0
					state->m_vip_regs.SPT[0] = data;
					break;
		case 0x4A:	//SPT1
					state->m_vip_regs.SPT[1] = data;
					break;
		case 0x4C:	//SPT2
					state->m_vip_regs.SPT[2] = data;
					break;
		case 0x4E:	//SPT3
					state->m_vip_regs.SPT[3] = data;
					break;
		case 0x60:	//GPLT0
					state->m_vip_regs.GPLT[0] = data;
					break;
		case 0x62:	//GPLT1
					state->m_vip_regs.GPLT[1] = data;
					break;
		case 0x64:	//GPLT2
					state->m_vip_regs.GPLT[2] = data;
					break;
		case 0x66:	//GPLT3
					state->m_vip_regs.GPLT[3] = data;
					break;
		case 0x68:	//JPLT0
					state->m_vip_regs.JPLT[0] = data;
					break;
		case 0x6A:	//JPLT1
					state->m_vip_regs.JPLT[1] = data;
					break;
		case 0x6C:	//JPLT2
					state->m_vip_regs.JPLT[2] = data;
					break;
		case 0x6E:	//JPLT3
					state->m_vip_regs.JPLT[3] = data;
					break;
		case 0x70:	//BKCOL
					state->m_vip_regs.BKCOL = data;
					break;
		default:
					logerror("Unemulated write: addr %08x, data %04x\n", offset * 2 + 0x0005f800, data);
					break;
	}
}

static WRITE16_HANDLER( vboy_font0_w )
{
	vboy_state *state = space->machine().driver_data<vboy_state>();

	state->m_font[offset] = data | (state->m_font[offset] & (mem_mask ^ 0xffff));
}

static WRITE16_HANDLER( vboy_font1_w )
{
	vboy_state *state = space->machine().driver_data<vboy_state>();

	state->m_font[offset + 0x1000] = data | (state->m_font[offset + 0x1000] & (mem_mask ^ 0xffff));
}

static WRITE16_HANDLER( vboy_font2_w )
{
	vboy_state *state = space->machine().driver_data<vboy_state>();

	state->m_font[offset + 0x2000] = data | (state->m_font[offset + 0x2000] & (mem_mask ^ 0xffff));
}

static WRITE16_HANDLER( vboy_font3_w )
{
	vboy_state *state = space->machine().driver_data<vboy_state>();

	state->m_font[offset + 0x3000] = data | (state->m_font[offset + 0x3000] & (mem_mask ^ 0xffff));
}

static READ16_HANDLER( vboy_font0_r )
{
	vboy_state *state = space->machine().driver_data<vboy_state>();

	return state->m_font[offset];
}

static READ16_HANDLER( vboy_font1_r )
{
	vboy_state *state = space->machine().driver_data<vboy_state>();

	return state->m_font[offset + 0x1000];
}

static READ16_HANDLER( vboy_font2_r )
{
	vboy_state *state = space->machine().driver_data<vboy_state>();

	return state->m_font[offset + 0x2000];
}

static READ16_HANDLER( vboy_font3_r )
{
	vboy_state *state = space->machine().driver_data<vboy_state>();

	return state->m_font[offset + 0x3000];
}

static void put_char(vboy_state *state, bitmap_t *bitmap, int x, int y, UINT16 ch, int flipx, int flipy, int trans, UINT8 pal)
{
	UINT16 code = ch;
	int i, b;

	for(i = 0; i < 8; i++)
	{
		UINT16  data;
		if(flipy==0) {
			 data = state->m_font[code * 8 + i];
		} else {
			 data = state->m_font[code * 8 + (7-i)];
		}
		for (b = 0; b < 8; b++)
		{
			UINT8 dat,col;
			if(flipx==0) {
				dat = ((data >> (b << 1)) & 0x03);
			} else {
				dat = ((data >> ((7-b) << 1)) & 0x03);
			}
			col = (pal >> (dat*2)) & 3;
			// This is how emulator works, but need to check
			if (dat==0) col=0;

			if (!trans || ( trans && col!=0)) {
				*BITMAP_ADDR16(bitmap, (y + i) & 0x1ff, (x + b) & 0x1ff) =  col;
			}
		}
	}
}

static WRITE16_HANDLER( vboy_bgmap_w )
{
	vboy_state *state = space->machine().driver_data<vboy_state>();

	state->m_bgmap[offset] = data | (state->m_bgmap[offset] & (mem_mask ^ 0xffff));
}

static READ16_HANDLER( vboy_bgmap_r )
{
	vboy_state *state = space->machine().driver_data<vboy_state>();

	return state->m_bgmap[offset];
}

static ADDRESS_MAP_START( vboy_mem, AS_PROGRAM, 32 )
	ADDRESS_MAP_GLOBAL_MASK(0x07ffffff)
	AM_RANGE( 0x00000000, 0x00005fff ) AM_RAM AM_BASE_MEMBER(vboy_state,m_l_frame_0) // L frame buffer 0
	AM_RANGE( 0x00006000, 0x00007fff ) AM_READWRITE16(vboy_font0_r, vboy_font0_w, 0xffffffff) // Font 0-511
	AM_RANGE( 0x00008000, 0x0000dfff ) AM_RAM AM_BASE_MEMBER(vboy_state,m_l_frame_1) // L frame buffer 1
	AM_RANGE( 0x0000e000, 0x0000ffff ) AM_READWRITE16(vboy_font1_r, vboy_font1_w, 0xffffffff) // Font 512-1023
	AM_RANGE( 0x00010000, 0x00015fff ) AM_RAM AM_BASE_MEMBER(vboy_state,m_r_frame_0) // R frame buffer 0
	AM_RANGE( 0x00016000, 0x00017fff ) AM_READWRITE16(vboy_font2_r, vboy_font2_w, 0xffffffff) // Font 1024-1535
	AM_RANGE( 0x00018000, 0x0001dfff ) AM_RAM AM_BASE_MEMBER(vboy_state,m_r_frame_1) // R frame buffer 1
	AM_RANGE( 0x0001e000, 0x0001ffff ) AM_READWRITE16(vboy_font3_r, vboy_font3_w, 0xffffffff) // Font 1536-2047

	AM_RANGE( 0x00020000, 0x0003ffff ) AM_READWRITE16(vboy_bgmap_r,vboy_bgmap_w, 0xffffffff) // VIPC memory

	//AM_RANGE( 0x00040000, 0x0005ffff ) AM_RAM // VIPC
	AM_RANGE( 0x0005f800, 0x0005f87f )	AM_READWRITE16(vip_r, vip_w, 0xffffffff)

	AM_RANGE( 0x00078000, 0x00079fff ) AM_READWRITE16(vboy_font0_r, vboy_font0_w, 0xffffffff) // Font 0-511 mirror
	AM_RANGE( 0x0007a000, 0x0007bfff ) AM_READWRITE16(vboy_font1_r, vboy_font1_w, 0xffffffff) // Font 512-1023 mirror
	AM_RANGE( 0x0007c000, 0x0007dfff ) AM_READWRITE16(vboy_font2_r, vboy_font2_w, 0xffffffff) // Font 1024-1535 mirror
	AM_RANGE( 0x0007e000, 0x0007ffff ) AM_READWRITE16(vboy_font3_r, vboy_font3_w, 0xffffffff) // Font 1536-2047 mirror

	AM_RANGE( 0x01000000, 0x010005ff ) AM_RAM // Sound RAM
	AM_RANGE( 0x02000000, 0x0200002b ) AM_MIRROR(0x0ffff00) AM_READWRITE(port_02_read, port_02_write) // Hardware control registers mask 0xff
	//AM_RANGE( 0x04000000, 0x04ffffff ) // Expansion area
	AM_RANGE( 0x05000000, 0x0500ffff ) AM_MIRROR(0x0ff0000) AM_RAM // Main RAM - 64K mask 0xffff
	AM_RANGE( 0x06000000, 0x06003fff ) AM_RAM // Cart RAM - 8K
	AM_RANGE( 0x07000000, 0x071fffff ) AM_MIRROR(0x0e00000) AM_ROM AM_REGION("user1", 0) /* ROM */
ADDRESS_MAP_END

static ADDRESS_MAP_START( vboy_io, AS_IO, 32 )
	ADDRESS_MAP_GLOBAL_MASK(0x07ffffff)
	AM_RANGE( 0x00000000, 0x00005fff ) AM_RAM AM_BASE_MEMBER(vboy_state,m_l_frame_0) // L frame buffer 0
	AM_RANGE( 0x00006000, 0x00007fff ) AM_READWRITE16(vboy_font0_r, vboy_font0_w, 0xffffffff) // Font 0-511
	AM_RANGE( 0x00008000, 0x0000dfff ) AM_RAM AM_BASE_MEMBER(vboy_state,m_l_frame_1) // L frame buffer 1
	AM_RANGE( 0x0000e000, 0x0000ffff ) AM_READWRITE16(vboy_font1_r, vboy_font1_w, 0xffffffff) // Font 512-1023
	AM_RANGE( 0x00010000, 0x00015fff ) AM_RAM AM_BASE_MEMBER(vboy_state,m_r_frame_0) // R frame buffer 0
	AM_RANGE( 0x00016000, 0x00017fff ) AM_READWRITE16(vboy_font2_r, vboy_font2_w, 0xffffffff) // Font 1024-1535
	AM_RANGE( 0x00018000, 0x0001dfff ) AM_RAM AM_BASE_MEMBER(vboy_state,m_r_frame_1) // R frame buffer 1
	AM_RANGE( 0x0001e000, 0x0001ffff ) AM_READWRITE16(vboy_font3_r, vboy_font3_w, 0xffffffff) // Font 1536-2047

	AM_RANGE( 0x00020000, 0x0003ffff ) AM_READWRITE16(vboy_bgmap_r,vboy_bgmap_w, 0xffffffff) // VIPC memory

	//AM_RANGE( 0x00040000, 0x0005ffff ) AM_RAM // VIPC
	AM_RANGE( 0x0005f800, 0x0005f87f )	AM_READWRITE16(vip_r, vip_w, 0xffffffff)

	AM_RANGE( 0x00078000, 0x00079fff ) AM_READWRITE16(vboy_font0_r, vboy_font0_w, 0xffffffff) // Font 0-511 mirror
	AM_RANGE( 0x0007a000, 0x0007bfff ) AM_READWRITE16(vboy_font1_r, vboy_font1_w, 0xffffffff) // Font 512-1023 mirror
	AM_RANGE( 0x0007c000, 0x0007dfff ) AM_READWRITE16(vboy_font2_r, vboy_font2_w, 0xffffffff) // Font 1024-1535 mirror
	AM_RANGE( 0x0007e000, 0x0007ffff ) AM_READWRITE16(vboy_font3_r, vboy_font3_w, 0xffffffff) // Font 1536-2047 mirror

	AM_RANGE( 0x01000000, 0x010005ff ) AM_RAM // Sound RAM
	AM_RANGE( 0x02000000, 0x0200002b ) AM_MIRROR(0x0ffff00) AM_READWRITE(port_02_read, port_02_write) // Hardware control registers mask 0xff
	//AM_RANGE( 0x04000000, 0x04ffffff ) // Expansion area
	AM_RANGE( 0x05000000, 0x0500ffff ) AM_MIRROR(0x0ff0000) AM_RAM // Main RAM - 64K mask 0xffff
	AM_RANGE( 0x06000000, 0x06003fff ) AM_RAM // Cart RAM - 8K
	AM_RANGE( 0x07000000, 0x071fffff ) AM_MIRROR(0x0e00000) AM_ROM AM_REGION("user1", 0) /* ROM */
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( vboy )
	PORT_START("INPUT")
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_DOWN ) PORT_PLAYER(1)
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_SELECT ) PORT_PLAYER(1)
	PORT_BIT( 0x1000, IP_ACTIVE_HIGH, IPT_START ) PORT_PLAYER(1)
	PORT_BIT( 0x0800, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_UP ) PORT_PLAYER(1)
	PORT_BIT( 0x0400, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_DOWN ) PORT_PLAYER(1)
	PORT_BIT( 0x0200, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_UP ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_NAME("L") PORT_PLAYER(1) // Left button on back
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_BUTTON4 ) PORT_NAME("R") PORT_PLAYER(1) // Right button on back
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_NAME("A") PORT_PLAYER(1) // A button (or B?)
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_NAME("B") PORT_PLAYER(1) // B button (or A?)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW,  IPT_UNUSED ) // Always 1
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_UNUSED ) // Battery low
INPUT_PORTS_END


static MACHINE_RESET(vboy)
{
	vboy_state *state = machine.driver_data<vboy_state>();

	/* Initial values taken from Reality Boy, to be verified when emulation improves */
	state->m_vboy_regs.lpc = 0x6d;
	state->m_vboy_regs.lpc2 = 0xff;
	state->m_vboy_regs.lpt = 0x00;
	state->m_vboy_regs.lpr = 0x00;
	state->m_vboy_regs.klb = 0x00;
	state->m_vboy_regs.khb = 0x00;
	state->m_vboy_regs.tlb = 0xff;
	state->m_vboy_regs.thb = 0xff;
	state->m_vboy_regs.tcr = 0xe4;
	state->m_vboy_regs.wcr = 0xfc;
	state->m_vboy_regs.kcr = 0x4c;
}


static VIDEO_START( vboy )
{
	vboy_state *state = machine.driver_data<vboy_state>();
	int i;

	// Allocate memory for temporary screens
	for(i = 0; i < 16; i++)
	{
		state->m_bg_map[i] = auto_bitmap_alloc(machine, 512, 512, BITMAP_FORMAT_INDEXED16);
	}
	state->m_screen_output = auto_bitmap_alloc(machine, 384, 224, BITMAP_FORMAT_INDEXED16);

	state->m_font  = auto_alloc_array(machine, UINT16, 2048 * 8);
	state->m_bgmap = auto_alloc_array(machine, UINT16, 0x20000 >> 1);;
	state->m_objects = state->m_bgmap + (0x1E000 >> 1);
	state->m_columntab1 = state->m_bgmap + (0x1dc00 >> 1);
	state->m_columntab2 = state->m_bgmap + (0x1de00 >> 1);
	state->m_world = state->m_bgmap + (0x1d800 >> 1);
}

static void fill_bg_map(vboy_state *state, int num, bitmap_t *bitmap)
{
	int i, j;

	// Fill background map
	for (i = 0; i < 64; i++)
	{
		for (j = 0; j < 64; j++)
		{
			UINT16 val = state->m_bgmap[j + 64 * i + (num * 0x1000)];
			put_char(state, bitmap, j * 8, i * 8, val & 0x7ff, BIT(val,13), BIT(val,12), 0, state->m_vip_regs.GPLT[(val >> 14) & 3]);
		}
	}
}

static UINT8 display_world(vboy_state *state, int num, bitmap_t *bitmap, UINT8 right)
{
	UINT16 def = state->m_world[num*16];
	INT16 gx  = state->m_world[num*16+1];
	INT16 gp  = state->m_world[num*16+2];
	INT16 gy  = state->m_world[num*16+3];
	INT16 mx  = state->m_world[num*16+4];
	INT16 mp  = state->m_world[num*16+5];
	INT16 my  = state->m_world[num*16+6];
	UINT16 w  = state->m_world[num*16+7];
	UINT16 h = state->m_world[num*16+8];
	UINT16 param_base  = state->m_world[num*16+9] & 0xfff0;
//  UINT16 overplane = state->m_world[num*16+10];
	UINT8 bg_map_num = def & 0x0f;
	INT16 x,y,i;
	UINT8 mode	= (def >> 12) & 3;
	UINT16 *vboy_paramtab;

	vboy_paramtab = state->m_bgmap + param_base;
	if ((mode==0) || (mode==1)) {
		fill_bg_map(state, bg_map_num, state->m_bg_map[bg_map_num]);
		if (BIT(def,15) && (right==0)) {
			// Left screen
			for(y=0;y<=h;y++) {
				for(x=0;x<=w;x++) {
					INT16 y1 = (y+gy);
					INT16 x1 = (x+gx-gp);
					UINT16 pix = 0;
					if (mode==1) {
						x1 += vboy_paramtab[y*2];
					}
					pix = *BITMAP_ADDR16(state->m_bg_map[bg_map_num], (y+my) & 0x1ff, (x+mx-mp) & 0x1ff);
					if (pix!=0) {
						if (y1>=0 && y1<224) {
							if (x1>=0 && x1<384) {
								*BITMAP_ADDR16(bitmap, y1, x1) = pix;
							}
						}
					}
				}
			}
		}
		if (BIT(def,14) && (right==1)) {
			// Right screen
			for(y=0;y<=h;y++) {
				for(x=0;x<=w;x++) {
					INT16 y1 = (y+gy);
					INT16 x1 = (x+gx+gp);
					UINT16 pix = 0;
					if (mode==1) {
						x1 += vboy_paramtab[y*2+1];
					}
					pix = *BITMAP_ADDR16(state->m_bg_map[bg_map_num], (y+my) & 0x1ff, (x+mx+mp) & 0x1ff);
					if (pix!=0) {
						if (y1>=0 && y1<224) {
							if (x1>=0 && x1<384) {
								*BITMAP_ADDR16(bitmap, y1, x1) = pix;
							}
						}
					}
				}
			}
		}
	}
	if(mode==2) {

	}
	if(mode==3) {
		// just for test
		for(i=state->m_vip_regs.SPT[3];i>=state->m_vip_regs.SPT[2];i--) {
			UINT16 start_ndx = i * 4;
			INT16 jx = state->m_objects[start_ndx+0];
			INT16 jp = state->m_objects[start_ndx+1] & 0x3fff;
			INT16 jy = state->m_objects[start_ndx+2] & 0x1ff;
			UINT16 val = state->m_objects[start_ndx+3];

			if (!right) {
				put_char(state, bitmap, (jx-jp) & 0x1ff,jy, val & 0x7ff, BIT(val,13), BIT(val,12), 1, state->m_vip_regs.JPLT[(val>>14) & 3]);
			} else {
				put_char(state, bitmap, (jx+jp) & 0x1ff,jy, val & 0x7ff, BIT(val,13), BIT(val,12), 1, state->m_vip_regs.JPLT[(val>>14) & 3]);
			}
		}
	}
	// Return END world status
	return BIT(def,6);
}

static SCREEN_UPDATE( vboy )
{
	vboy_state *state = screen->machine().driver_data<vboy_state>();
	int i;
	UINT8 right = 0;
	device_t *_3d_right_screen = screen->machine().device("3dright");

	bitmap_fill(state->m_screen_output, cliprect, state->m_vip_regs.BKCOL);

	if (screen == _3d_right_screen) right = 1;

	for(i=31;i>=0;i--) {
		if (display_world(state,i,state->m_screen_output,right)) break;
	}
	copybitmap(bitmap, state->m_screen_output, 0, 0, 0, 0, cliprect);

	state->m_vip_regs.DPSTTS = ((state->m_vip_regs.DPCTRL&0x0302)|0x7c);
	return 0;
}

static TIMER_DEVICE_CALLBACK( video_tick )
{
	vboy_state *state = timer.machine().driver_data<vboy_state>();

	state->m_vip_regs.XPSTTS = (state->m_vip_regs.XPSTTS==0) ? 0x0c : 0x00;
}

static const rgb_t vboy_palette[18] = {
	MAKE_RGB(0x00, 0x00, 0x00), // 0
	MAKE_RGB(0x00, 0x00, 0x00), // 1
	MAKE_RGB(0x00, 0x00, 0x00), // 2
	MAKE_RGB(0x00, 0x00, 0x00)  // 3
};

static PALETTE_INIT( vboy )
{
	palette_set_colors(machine, 0, vboy_palette, ARRAY_LENGTH(vboy_palette));
}

static INTERRUPT_GEN( vboy_interrupt )
{
	vboy_state *state = device->machine().driver_data<vboy_state>();

	if(state->m_vip_regs.INTENB)
	{
		cputag_set_input_line(device->machine(), "maincpu", 4, HOLD_LINE);
		state->m_vip_regs.INTPND |= 0x4000;
	}
}

static MACHINE_CONFIG_START( vboy, vboy_state )

	/* basic machine hardware */
	MCFG_CPU_ADD( "maincpu", V810, XTAL_20MHz )
	MCFG_CPU_PROGRAM_MAP(vboy_mem)
	MCFG_CPU_IO_MAP(vboy_io)
	MCFG_CPU_VBLANK_INT("3dleft", vboy_interrupt)

	MCFG_MACHINE_RESET(vboy)

	MCFG_TIMER_ADD_PERIODIC("video", video_tick, attotime::from_hz(100))

	/* video hardware */
	MCFG_DEFAULT_LAYOUT(layout_vboy)

	MCFG_PALETTE_LENGTH(4)
	MCFG_PALETTE_INIT(vboy)

	MCFG_VIDEO_START(vboy)

	/* Left screen */
	MCFG_SCREEN_ADD("3dleft", RASTER)
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_REFRESH_RATE(50.2)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500))
	MCFG_SCREEN_SIZE(384, 224)
	MCFG_SCREEN_VISIBLE_AREA(0, 384-1, 0, 224-1)
	MCFG_SCREEN_UPDATE(vboy)

	/* Right screen */
	MCFG_SCREEN_ADD("3dright", RASTER)
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_REFRESH_RATE(50.2)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500))
	MCFG_SCREEN_SIZE(384, 224)
	MCFG_SCREEN_VISIBLE_AREA(0, 384-1, 0, 224-1)
	MCFG_SCREEN_UPDATE(vboy)

	/* cartridge */
	MCFG_CARTSLOT_ADD("cart")
	MCFG_CARTSLOT_EXTENSION_LIST("vb,bin")
	MCFG_CARTSLOT_MANDATORY
	MCFG_CARTSLOT_INTERFACE("vboy_cart")

	/* software lists */
	MCFG_SOFTWARE_LIST_ADD("cart_list","vboy")
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( vboy )
	ROM_REGION( 0x200000, "user1", 0 )
	ROM_CART_LOAD("cart", 0x0000, 0x200000, ROM_MIRROR)
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY     FULLNAME       FLAGS */
CONS( 1995, vboy,   0,      0,		 vboy,		vboy,	 0, 	 "Nintendo", "Virtual Boy", GAME_NOT_WORKING | GAME_NO_SOUND)

/****************************************************************************

    drivers/mac.c
    Macintosh family emulation

    Nate Woods, Raphael Nabet, R. Belmont

        0x000000 - 0x3fffff     RAM/ROM (switches based on overlay)
        0x400000 - 0x4fffff     ROM
        0x580000 - 0x5fffff     5380 NCR/Symbios SCSI peripherals chip (Mac Plus only)
        0x600000 - 0x6fffff     RAM
        0x800000 - 0x9fffff     Zilog 8530 SCC (Serial Control Chip) Read
        0xa00000 - 0xbfffff     Zilog 8530 SCC (Serial Control Chip) Write
        0xc00000 - 0xdfffff     IWM (Integrated Woz Machine; floppy)
        0xe80000 - 0xefffff     Rockwell 6522 VIA
        0xf00000 - 0xffffef     ??? (the ROM appears to be accessing here)
        0xfffff0 - 0xffffff     Auto Vector


    Interrupts:
        M68K:
            Level 1 from VIA
            Level 2 from SCC
            Level 4 : Interrupt switch (not implemented)

        VIA:
            CA1 from VBLANK
            CA2 from 1 Hz clock (RTC)
            CB1 from Keyboard Clock
            CB2 from Keyboard Data
            SR  from Keyboard Data Ready

        SCC:
            PB_EXT  from mouse Y circuitry
            PA_EXT  from mouse X circuitry

****************************************************************************/

#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/m68000/m68000.h"
#include "cpu/powerpc/ppc.h"
#include "cpu/m6805/m6805.h"
#include "machine/6522via.h"
#include "machine/ncr5380.h"
#include "machine/applefdc.h"
#include "devices/sonydriv.h"
#include "imagedev/harddriv.h"
#include "formats/ap_dsk35.h"
#include "machine/ram.h"
#include "imagedev/chd_cd.h"
#include "sound/asc.h"
#include "sound/cdda.h"
#include "includes/mac.h"

#define C7M	(7833600)
#define C15M	(C7M*2)
#define C32M	(C15M*2)

// do this here - SCREEN_UPDATE is called each scanline when stepping in the
// debugger, which means you can't escape the VIA2 IRQ handler
//
// RBV/MDU interrupts:
//
// CA1: any slot interrupt = 0x02
// CA2: SCSI interrupt = 0x01
// CB1: ASC interrupt = 0x10

static INTERRUPT_GEN( mac_rbv_vbl )
{
	mac_state *mac = device->machine().driver_data<mac_state>();

	mac->m_rbv_regs[2] &= ~0x40;	// set vblank signal

	if ((mac->m_rbv_regs[0x12] & 0x40) && (mac->m_rbv_ier & 0x2))
	{
		mac->m_rbv_ifr |= 0x82;
		mac->set_via2_interrupt(1);
	}
}

READ32_MEMBER( mac_state::rbv_ramdac_r )
{
	return 0;
}

WRITE32_MEMBER( mac_state::rbv_ramdac_w )
{
	if (!offset)
	{
		m_rbv_clutoffs = data>>24;
		m_rbv_count = 0;
	}
	else
	{
		m_rbv_colors[m_rbv_count++] = data>>24;

		if (m_rbv_count == 3)
		{
			// for portrait display, force monochrome by using the blue channel
			if (m_model != MODEL_MAC_CLASSIC_II)
			{
				// Color Classic has no MONTYPE so the safe read gets us 512x384, which is right
				if (input_port_read_safe(space.machine(), "MONTYPE", 2) == 1)
				{
					palette_set_color(space.machine(), m_rbv_clutoffs, MAKE_RGB(m_rbv_colors[2], m_rbv_colors[2], m_rbv_colors[2]));
					m_rbv_palette[m_rbv_clutoffs] = MAKE_RGB(m_rbv_colors[2], m_rbv_colors[2], m_rbv_colors[2]);
					m_rbv_clutoffs++;
					m_rbv_count = 0;
				}
				else
				{
					palette_set_color(space.machine(), m_rbv_clutoffs, MAKE_RGB(m_rbv_colors[0], m_rbv_colors[1], m_rbv_colors[2]));
					m_rbv_palette[m_rbv_clutoffs] = MAKE_RGB(m_rbv_colors[0], m_rbv_colors[1], m_rbv_colors[2]);
					m_rbv_clutoffs++;
					m_rbv_count = 0;
				}
			}
		}
	}
}

WRITE32_MEMBER( mac_state::ariel_ramdac_w )	// this is for the "Ariel" style RAMDAC
{
	if (mem_mask == 0xff000000)
	{
		m_rbv_clutoffs = data>>24;
		m_rbv_count = 0;
	}
	else if (mem_mask == 0x00ff0000)
	{
		m_rbv_colors[m_rbv_count++] = data>>16;

		if (m_rbv_count == 3)
		{
			// for portrait display, force monochrome by using the blue channel
			if (m_model != MODEL_MAC_CLASSIC_II)
			{
				// Color Classic has no MONTYPE so the safe read gets us 512x384, which is right
				if (input_port_read_safe(space.machine(), "MONTYPE", 2) == 1)
				{
					palette_set_color(space.machine(), m_rbv_clutoffs, MAKE_RGB(m_rbv_colors[2], m_rbv_colors[2], m_rbv_colors[2]));
					m_rbv_palette[m_rbv_clutoffs] = MAKE_RGB(m_rbv_colors[2], m_rbv_colors[2], m_rbv_colors[2]);
					m_rbv_clutoffs++;
					m_rbv_count = 0;
				}
				else
				{
					palette_set_color(space.machine(), m_rbv_clutoffs, MAKE_RGB(m_rbv_colors[0], m_rbv_colors[1], m_rbv_colors[2]));
					m_rbv_palette[m_rbv_clutoffs] = MAKE_RGB(m_rbv_colors[0], m_rbv_colors[1], m_rbv_colors[2]);
					m_rbv_clutoffs++;
					m_rbv_count = 0;
				}
			}
		}
	}
	else if (mem_mask == 0x0000ff00)
	{
		// config reg
//      printf("Ariel: %02x to config\n", (data>>8)&0xff);
	}
	else	// color key reg
	{
	}
}

READ8_MEMBER( mac_state::mac_sonora_vctl_r )
{
	if (offset == 2)
	{
		return (6 << 4);	// 640x480 RGB monitor
	}

	return m_sonora_vctl[offset];
}

WRITE8_MEMBER( mac_state::mac_sonora_vctl_w )
{
//  printf("Sonora: %02x to vctl %x\n", data, offset);
	m_sonora_vctl[offset] = data;
}

READ8_MEMBER ( mac_state::mac_rbv_r )
{
	int data = 0;

	if (offset < 0x100)
	{
		data = m_rbv_regs[offset];

		if (offset == 0x02)
		{
			if (!space.machine().primary_screen->vblank())
			{
				data |= 0x40;
			}
		}

		if (offset == 0x10)
		{
			data &= ~0x38;
			data |= (input_port_read_safe(space.machine(), "MONTYPE", 2)<<3);
//          printf("rbv_r montype: %02x (PC %x)\n", data, cpu_get_pc(space.cpu));
		}

		// bit 7 of these registers always reads as 0 on RBV
		if ((offset == 0x12) || (offset == 0x13))
		{
			data &= ~0x80;
		}
	}
	else
	{
		offset >>= 9;

		switch (offset)
		{
			case 13:	// IFR
//              printf("Read IER = %02x (PC=%x)\n", m_rbv_ier, cpu_get_pc(space.cpu));
				return m_rbv_ifr;
				break;

			case 14:	// IER
//              printf("Read IFR = %02x (PC=%x)\n", m_rbv_ifr, cpu_get_pc(space.cpu));
				return m_rbv_ier;
				break;

			default:
				logerror("rbv_r: Unknown extended RBV VIA register %d access\n", offset);
				break;
		}
	}

//  if (offset != 2) printf("rbv_r: %x = %02x (PC=%x)\n", offset, data, cpu_get_pc(space.cpu));

	return data;
}

WRITE8_MEMBER ( mac_state::mac_rbv_w )
{
	if (offset < 0x100)
	{
//      if (offset == 0x10)
//      printf("rbv_w: %02x to offset %x (PC=%x)\n", data, offset, cpu_get_pc(space.cpu));
		switch (offset)
		{
			case 0x00:
				if (m_model == MODEL_MAC_LC)
				{
					m68k_set_hmmu_enable(m_maincpu, (data & 0x8) ? M68K_HMMU_DISABLE : M68K_HMMU_ENABLE_LC);
				}
				break;

			case 0x01:
				if (((data & 0xc0) != (m_rbv_regs[1] & 0xc0)) && (m_rbv_type == RBV_TYPE_V8))
				{
					m_rbv_regs[1] = data;
					this->v8_resize();
				}
				break;

			case 0x02:
				if ((data & 0x40) && (m_rbv_type == RBV_TYPE_SONORA))
				{
					m_rbv_regs[offset] &= ~0x40;
				}
				else
				{
					m_rbv_regs[offset] = data;
				}
				break;

			case 0x03:
				this->set_via2_interrupt(0);
				m_rbv_regs[offset] = data;
				break;

			case 0x10:
				if (data != 0)
				{
					m_rbv_immed10wr = 1;
				}
				m_rbv_regs[offset] = data;
				break;

			case 0x12:
				if (data & 0x80)	// 1 bits write 1s
				{
					m_rbv_regs[offset] |= data & 0x7f;
				}
				else			// 1 bits write 0s
				{
					m_rbv_regs[offset] &= ~(data & 0x7f);
				}
				break;

			case 0x13:
				if (data & 0x80)	// 1 bits write 1s
				{
					m_rbv_regs[offset] |= data & 0x7f;

					if (data == 0xff) m_rbv_regs[offset] = 0x1f;	// I don't know why this is special, but the IIci ROM's POST demands it
				}
				else			// 1 bits write 0s
				{
					m_rbv_regs[offset] &= ~(data & 0x7f);
				}
//              printf("RBV: 0x13 (%02x) = %02x (PC %x)\n", data, m_rbv_regs[offset], cpu_get_pc(space.cpu));
				break;

			default:
				m_rbv_regs[offset] = data;
				break;
		}
	}
	else
	{
		offset >>= 9;

		switch (offset)
		{
			case 13:	// IFR
//              printf("rbv_w: %02x to IFR\n", data);
				m_rbv_ifr = data;
				this->set_via2_interrupt(0);
				break;

			case 14:	// IER
				if (data & 0x80)	// 1 bits write 1s
				{
					m_rbv_ier |= data & 0x7f;
				}
				else	    // 1 bits write 0s
				{
					m_rbv_ier &= ~(data & 0x7f);
				}

//              printf("rbv_w: %02x to IER => %02x\n", data, m_rbv_ier);
				break;

			default:
				logerror("rbv_w: Unknown extended RBV VIA register %d access\n", offset);
				break;
		}
	}
}

READ32_MEMBER(mac_state::mac_read_id)
{
	logerror("Mac read ID reg @ PC=%x\n", cpu_get_pc(&space.device()));

	switch (m_model)
	{
		case MODEL_MAC_LC_III:
			return 0xa55a0001;	// 25 MHz LC III

		case MODEL_MAC_LC_III_PLUS:
			return 0xa55a0003;	// 33 MHz LC III+

		case MODEL_MAC_POWERMAC_6100:
			return 0xa55a3011;

		case MODEL_MAC_POWERMAC_7100:
			return 0xa55a3012;

		case MODEL_MAC_POWERMAC_8100:
			return 0xa55a3013;

		case MODEL_MAC_PBDUO_210:
			return 0xa55a1004;

		case MODEL_MAC_PBDUO_230:
			return 0xa55a1005;

		case MODEL_MAC_PBDUO_250:
			return 0xa55a1006;

		default:
			return 0;
	}
}

// Portable/PB100 video
static VIDEO_START( mac_prtb )
{
}

static SCREEN_UPDATE( mac_prtb )
{
	return 0;
}

READ16_MEMBER(mac_state::mac_config_r)
{
	return 0xffff;	// not sure what this does
}

/***************************************************************************
    ADDRESS MAPS
***************************************************************************/

static ADDRESS_MAP_START(mac512ke_map, AS_PROGRAM, 16, mac_state )
	AM_RANGE(0x800000, 0x9fffff) AM_READ(mac_scc_r)
	AM_RANGE(0xa00000, 0xbfffff) AM_WRITE(mac_scc_w)
	AM_RANGE(0xc00000, 0xdfffff) AM_READWRITE(mac_iwm_r, mac_iwm_w)
	AM_RANGE(0xe80000, 0xefffff) AM_READWRITE(mac_via_r, mac_via_w)
	AM_RANGE(0xfffff0, 0xffffff) AM_READWRITE(mac_autovector_r, mac_autovector_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START(macplus_map, AS_PROGRAM, 16, mac_state )
	AM_RANGE(0x580000, 0x5fffff) AM_READWRITE(macplus_scsi_r, macplus_scsi_w)
	AM_RANGE(0x800000, 0x9fffff) AM_READ(mac_scc_r)
	AM_RANGE(0xa00000, 0xbfffff) AM_WRITE(mac_scc_w)
	AM_RANGE(0xc00000, 0xdfffff) AM_READWRITE(mac_iwm_r, mac_iwm_w)
	AM_RANGE(0xe80000, 0xefffff) AM_READWRITE(mac_via_r, mac_via_w)
	AM_RANGE(0xfffff0, 0xffffff) AM_READWRITE(mac_autovector_r, mac_autovector_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START(macprtb_map, AS_PROGRAM, 16, mac_state )
	AM_RANGE(0x900000, 0x93ffff) AM_ROM AM_REGION("bootrom", 0) AM_MIRROR(0x0c0000)
	AM_RANGE(0xf60000, 0xf6ffff) AM_READWRITE(mac_iwm_r, mac_iwm_w)
	AM_RANGE(0xf70000, 0xf7ffff) AM_READWRITE(mac_via_r, mac_via_w)
	AM_RANGE(0xf90000, 0xf9ffff) AM_READWRITE(macplus_scsi_r, macplus_scsi_w)
	AM_RANGE(0xfa8000, 0xfaffff) AM_RAM	// VRAM
	AM_RANGE(0xfb0000, 0xfbffff) AM_DEVREADWRITE8("asc", asc_device, read, write, 0xffff)
	AM_RANGE(0xfc0000, 0xfcffff) AM_READ(mac_config_r)
	AM_RANGE(0xfd0000, 0xfdffff) AM_READWRITE(mac_scc_r, mac_scc_2_w)
	AM_RANGE(0xfffff0, 0xffffff) AM_READWRITE(mac_autovector_r, mac_autovector_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START(maclc_map, AS_PROGRAM, 32, mac_state )
	ADDRESS_MAP_GLOBAL_MASK(0x80ffffff)	// V8 uses bit 31 and 23-0 for address decoding only

	AM_RANGE(0xa00000, 0xafffff) AM_ROM AM_REGION("bootrom", 0)	// ROM (in 32-bit mode)

	AM_RANGE(0xf00000, 0xf01fff) AM_READWRITE16(mac_via_r, mac_via_w, 0xffffffff)
	AM_RANGE(0xf04000, 0xf05fff) AM_READWRITE16(mac_scc_r, mac_scc_2_w, 0xffffffff)
	AM_RANGE(0xf06000, 0xf07fff) AM_READWRITE(macii_scsi_drq_r, macii_scsi_drq_w)
	AM_RANGE(0xf10000, 0xf11fff) AM_READWRITE16(macplus_scsi_r, macii_scsi_w, 0xffffffff)
	AM_RANGE(0xf12000, 0xf13fff) AM_READWRITE(macii_scsi_drq_r, macii_scsi_drq_w)
	AM_RANGE(0xf14000, 0xf15fff) AM_DEVREADWRITE8("asc", asc_device, read, write, 0xffffffff)
	AM_RANGE(0xf16000, 0xf17fff) AM_READWRITE16(mac_iwm_r, mac_iwm_w, 0xffffffff)
	AM_RANGE(0xf24000, 0xf24003) AM_READWRITE(rbv_ramdac_r, ariel_ramdac_w)
	AM_RANGE(0xf26000, 0xf27fff) AM_READWRITE8(mac_rbv_r, mac_rbv_w, 0xffffffff)	// VIA2 (V8)
	AM_RANGE(0xf40000, 0xfbffff) AM_RAM AM_BASE(m_rbv_vram)
ADDRESS_MAP_END

static ADDRESS_MAP_START(maclc3_map, AS_PROGRAM, 32, mac_state )
	AM_RANGE(0x40000000, 0x400fffff) AM_ROM AM_REGION("bootrom", 0) AM_MIRROR(0x0ff00000)

	AM_RANGE(0x50000000, 0x50001fff) AM_READWRITE16(mac_via_r, mac_via_w, 0xffffffff) AM_MIRROR(0x00f00000)
	AM_RANGE(0x50004000, 0x50005fff) AM_READWRITE16(mac_scc_r, mac_scc_2_w, 0xffffffff) AM_MIRROR(0x00f00000)
	AM_RANGE(0x50006000, 0x50007fff) AM_READWRITE(macii_scsi_drq_r, macii_scsi_drq_w) AM_MIRROR(0x00f00000)
	AM_RANGE(0x50010000, 0x50011fff) AM_READWRITE16(macplus_scsi_r, macii_scsi_w, 0xffffffff) AM_MIRROR(0x00f00000)
	AM_RANGE(0x50012000, 0x50013fff) AM_READWRITE(macii_scsi_drq_r, macii_scsi_drq_w) AM_MIRROR(0x00f00000)
	AM_RANGE(0x50014000, 0x50015fff) AM_DEVREADWRITE8("asc", asc_device, read, write, 0xffffffff) AM_MIRROR(0x00f00000)
	AM_RANGE(0x50016000, 0x50017fff) AM_READWRITE16(mac_iwm_r, mac_iwm_w, 0xffffffff) AM_MIRROR(0x00f00000)
	AM_RANGE(0x50024000, 0x50025fff) AM_WRITE( ariel_ramdac_w ) AM_MIRROR(0x00f00000)
	AM_RANGE(0x50026000, 0x50027fff) AM_READWRITE8(mac_rbv_r, mac_rbv_w, 0xffffffff) AM_MIRROR(0x00f00000)
	AM_RANGE(0x50028000, 0x50028003) AM_READWRITE8(mac_sonora_vctl_r, mac_sonora_vctl_w, 0xffffffff) AM_MIRROR(0x00f00000)

	AM_RANGE(0x5ffffffc, 0x5fffffff) AM_READ(mac_read_id)

	AM_RANGE(0x60000000, 0x600fffff) AM_RAM AM_MIRROR(0x0ff00000) AM_BASE(m_rbv_vram)
ADDRESS_MAP_END

static ADDRESS_MAP_START(macii_map, AS_PROGRAM, 32, mac_state )
	AM_RANGE(0x40000000, 0x4003ffff) AM_ROM AM_REGION("bootrom", 0) AM_MIRROR(0x0ffc0000)

	// MMU remaps I/O without the F
	AM_RANGE(0x50000000, 0x50001fff) AM_READWRITE16(mac_via_r, mac_via_w, 0xffffffff) AM_MIRROR(0x00f00000)
	AM_RANGE(0x50002000, 0x50003fff) AM_READWRITE16(mac_via2_r, mac_via2_w, 0xffffffff) AM_MIRROR(0x00f00000)
	AM_RANGE(0x50004000, 0x50005fff) AM_READWRITE16(mac_scc_r, mac_scc_2_w, 0xffffffff) AM_MIRROR(0x00f00000)
	AM_RANGE(0x50006000, 0x50006003) AM_WRITE(macii_scsi_drq_w) AM_MIRROR(0x00f00000)
	AM_RANGE(0x50006060, 0x50006063) AM_READ(macii_scsi_drq_r) AM_MIRROR(0x00f00000)
	AM_RANGE(0x50010000, 0x50011fff) AM_READWRITE16(macplus_scsi_r, macii_scsi_w, 0xffffffff) AM_MIRROR(0x00f00000)
	AM_RANGE(0x50012060, 0x50012063) AM_READ(macii_scsi_drq_r) AM_MIRROR(0x00f00000)
	AM_RANGE(0x50014000, 0x50015fff) AM_DEVREADWRITE8("asc", asc_device, read, write, 0xffffffff) AM_MIRROR(0x00f00000)
	AM_RANGE(0x50016000, 0x50017fff) AM_READWRITE16(mac_iwm_r, mac_iwm_w, 0xffffffff) AM_MIRROR(0x00f00000)
	AM_RANGE(0x50040000, 0x50041fff) AM_READWRITE16(mac_via_r, mac_via_w, 0xffffffff) AM_MIRROR(0x00f00000)

	// RasterOps 264 640x480 fixed-res color video card (8, 16, or 24 bit)
	AM_RANGE(0xfe000000, 0xfe1fffff) AM_RAM	AM_BASE(m_cb264_vram) // supposed to be 1.5 megs of VRAM, but every other word?
	AM_RANGE(0xfeff6000, 0xfeff60ff) AM_READWRITE( mac_cb264_r, mac_cb264_w )
	AM_RANGE(0xfeff7000, 0xfeff7fff) AM_WRITE( mac_cb264_ramdac_w )
	AM_RANGE(0xfeff8000, 0xfeffffff) AM_ROM AM_REGION("rops264", 0)
ADDRESS_MAP_END

static ADDRESS_MAP_START(maciici_map, AS_PROGRAM, 32, mac_state )
	AM_RANGE(0x40000000, 0x4007ffff) AM_ROM AM_REGION("bootrom", 0) AM_MIRROR(0x0ff80000)

	AM_RANGE(0x50000000, 0x50001fff) AM_READWRITE16(mac_via_r, mac_via_w, 0xffffffff) AM_MIRROR(0x00f00000)
	AM_RANGE(0x50004000, 0x50005fff) AM_READWRITE16(mac_scc_r, mac_scc_2_w, 0xffffffff) AM_MIRROR(0x00f00000)
	AM_RANGE(0x50006000, 0x50007fff) AM_READWRITE(macii_scsi_drq_r, macii_scsi_drq_w) AM_MIRROR(0x00f00000)
	AM_RANGE(0x50010000, 0x50011fff) AM_READWRITE16(macplus_scsi_r, macii_scsi_w, 0xffffffff) AM_MIRROR(0x00f00000)
	AM_RANGE(0x50012060, 0x50012063) AM_READ(macii_scsi_drq_r) AM_MIRROR(0x00f00000)
	AM_RANGE(0x50014000, 0x50015fff) AM_DEVREADWRITE8("asc", asc_device, read, write, 0xffffffff) AM_MIRROR(0x00f00000)
	AM_RANGE(0x50016000, 0x50017fff) AM_READWRITE16(mac_iwm_r, mac_iwm_w, 0xffffffff) AM_MIRROR(0x00f00000)
	AM_RANGE(0x50024000, 0x50024007) AM_WRITE( rbv_ramdac_w ) AM_MIRROR(0x00f00000)
	AM_RANGE(0x50026000, 0x50027fff) AM_READWRITE8(mac_rbv_r, mac_rbv_w, 0xffffffff) AM_MIRROR(0x00f00000)
	AM_RANGE(0x50040000, 0x50041fff) AM_READWRITE16(mac_via_r, mac_via_w, 0xffffffff) AM_MIRROR(0x00f00000)
ADDRESS_MAP_END

static ADDRESS_MAP_START(macse30_map, AS_PROGRAM, 32, mac_state )
	AM_RANGE(0x40000000, 0x4003ffff) AM_ROM AM_REGION("bootrom", 0) AM_MIRROR(0x0ffc0000)

	AM_RANGE(0x50000000, 0x50001fff) AM_READWRITE16(mac_via_r, mac_via_w, 0xffffffff) AM_MIRROR(0x00f00000)
	AM_RANGE(0x50002000, 0x50003fff) AM_READWRITE16(mac_via2_r, mac_via2_w, 0xffffffff) AM_MIRROR(0x00f00000)
	AM_RANGE(0x50004000, 0x50005fff) AM_READWRITE16(mac_scc_r, mac_scc_2_w, 0xffffffff) AM_MIRROR(0x00f00000)
	AM_RANGE(0x50006000, 0x50007fff) AM_READWRITE(macii_scsi_drq_r, macii_scsi_drq_w) AM_MIRROR(0x00f00000)
	AM_RANGE(0x50010000, 0x50011fff) AM_READWRITE16(macplus_scsi_r, macii_scsi_w, 0xffffffff) AM_MIRROR(0x00f00000)
	AM_RANGE(0x50012060, 0x50012063) AM_READ(macii_scsi_drq_r) AM_MIRROR(0x00f00000)
	AM_RANGE(0x50014000, 0x50015fff) AM_DEVREADWRITE8("asc", asc_device, read, write, 0xffffffff) AM_MIRROR(0x00f00000)
	AM_RANGE(0x50016000, 0x50017fff) AM_READWRITE16(mac_iwm_r, mac_iwm_w, 0xffffffff) AM_MIRROR(0x00f00000)
	AM_RANGE(0x50040000, 0x50041fff) AM_READWRITE16(mac_via_r, mac_via_w, 0xffffffff) AM_MIRROR(0x00f00000)	// mirror

	AM_RANGE(0xfe000000, 0xfe00ffff) AM_RAM	AM_BASE(m_se30_vram)
	AM_RANGE(0xfeffe000, 0xfeffffff) AM_ROM AM_REGION("se30vrom", 0x0)
ADDRESS_MAP_END

static ADDRESS_MAP_START(pwrmac_map, AS_PROGRAM, 64, mac_state )
	AM_RANGE(0x00000000, 0x007fffff) AM_RAM	// 8 MB standard

	AM_RANGE(0x40000000, 0x403fffff) AM_ROM AM_REGION("bootrom", 0) AM_MIRROR(0x0fc00000)

	AM_RANGE(0x50000000, 0x50001fff) AM_READWRITE16(mac_via_r, mac_via_w, U64(0xffffffffffffffff)) AM_MIRROR(0x00f00000)
	AM_RANGE(0x50002000, 0x50003fff) AM_READWRITE16(mac_via2_r, mac_via2_w, U64(0xffffffffffffffff)) AM_MIRROR(0x00f00000)
	AM_RANGE(0x50004000, 0x50005fff) AM_READWRITE16(mac_scc_r, mac_scc_2_w, U64(0xffffffffffffffff)) AM_MIRROR(0x00f00000)
	AM_RANGE(0x50006000, 0x50006007) AM_WRITE32(macii_scsi_drq_w,U64(0xffffffffffffffff)) AM_MIRROR(0x00f00000)
	AM_RANGE(0x50006060, 0x50006067) AM_READ32(macii_scsi_drq_r,U64(0xffffffffffffffff)) AM_MIRROR(0x00f00000)
	AM_RANGE(0x50010000, 0x50011fff) AM_READWRITE16(macplus_scsi_r, macii_scsi_w, U64(0xffffffffffffffff)) AM_MIRROR(0x00f00000)
	AM_RANGE(0x50012060, 0x50012067) AM_READ32(macii_scsi_drq_r,U64(0xffffffffffffffff)) AM_MIRROR(0x00f00000)
	AM_RANGE(0x50016000, 0x50017fff) AM_READWRITE16(mac_iwm_r, mac_iwm_w, U64(0xffffffffffffffff)) AM_MIRROR(0x00f00000)

	AM_RANGE(0x5ffffff8, 0x5fffffff) AM_READ32(mac_read_id, U64(0xffffffffffffffff))

	AM_RANGE(0xffc00000, 0xffffffff) AM_ROM AM_REGION("bootrom", 0)
ADDRESS_MAP_END

/***************************************************************************
    DEVICE CONFIG
***************************************************************************/

static const applefdc_interface mac_iwm_interface =
{
	sony_set_lines,
	mac_fdc_set_enable_lines,

	sony_read_data,
	sony_write_data,
	sony_read_status
};

static const SCSIConfigTable dev_table =
{
	2,                                      /* 2 SCSI devices */
	{
	 { SCSI_ID_6, "harddisk1", SCSI_DEVICE_HARDDISK },  /* SCSI ID 6, using disk1, and it's a harddisk */
	 { SCSI_ID_5, "harddisk2", SCSI_DEVICE_HARDDISK }   /* SCSI ID 5, using disk2, and it's a harddisk */
	}
};

static const struct NCR5380interface macplus_5380intf =
{
	&dev_table,	// SCSI device table
	mac_scsi_irq	// IRQ (unconnected on the Mac Plus)
};

/***************************************************************************
    MACHINE DRIVERS
***************************************************************************/
static const floppy_config mac128512_floppy_config = //SONY_FLOPPY_ALLOW400K
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_STANDARD_3_5_DSHD,
	FLOPPY_OPTIONS_NAME(apple35_mac),
	NULL
};


static const floppy_config mac_floppy_config = //SONY_FLOPPY_ALLOW400K | SONY_FLOPPY_ALLOW800K
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_STANDARD_3_5_DSHD,
	FLOPPY_OPTIONS_NAME(apple35_mac),
	NULL
};

static MACHINE_CONFIG_START( mac512ke, mac_state )

	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", M68000, 7833600)        /* 7.8336 MHz */
	MCFG_CPU_PROGRAM_MAP(mac512ke_map)
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(60.15)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(1260))
	MCFG_QUANTUM_TIME(attotime::from_hz(60))

    /* video hardware */
	MCFG_VIDEO_ATTRIBUTES(VIDEO_UPDATE_BEFORE_VBLANK)
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_SIZE(MAC_H_TOTAL, MAC_V_TOTAL)
	MCFG_SCREEN_VISIBLE_AREA(0, MAC_H_VIS-1, 0, MAC_V_VIS-1)
	MCFG_SCREEN_UPDATE(mac)

	MCFG_PALETTE_LENGTH(2)
	MCFG_PALETTE_INIT(mac)

	MCFG_VIDEO_START(mac)

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD("custom", MAC_SOUND, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	/* nvram */
	MCFG_NVRAM_HANDLER(mac)

	/* devices */
	MCFG_IWM_ADD("fdc", mac_iwm_interface)
	MCFG_FLOPPY_SONY_2_DRIVES_ADD(mac128512_floppy_config)

	MCFG_SCC8530_ADD("scc", 7833600)
	MCFG_SCC8530_IRQ(mac_scc_irq)
	MCFG_VIA6522_ADD("via6522_0", 1000000, mac_via6522_intf)

	/* internal ram */
	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("512K")
MACHINE_CONFIG_END

static MACHINE_CONFIG_DERIVED( mac128k, mac512ke )

	/* internal ram */
	MCFG_RAM_MODIFY(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("128K")
MACHINE_CONFIG_END

static MACHINE_CONFIG_DERIVED( macplus, mac512ke )
	MCFG_CPU_MODIFY( "maincpu" )
	MCFG_CPU_PROGRAM_MAP(macplus_map)

	MCFG_NCR5380_ADD("ncr5380", 7833600, macplus_5380intf)

	MCFG_HARDDISK_ADD( "harddisk1" )
	MCFG_HARDDISK_ADD( "harddisk2" )

	MCFG_FLOPPY_SONY_2_DRIVES_MODIFY(mac_floppy_config)

	/* internal ram */
	MCFG_RAM_MODIFY(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("4M")
	MCFG_RAM_EXTRA_OPTIONS("1M,2M,2560K,4M")
MACHINE_CONFIG_END

static MACHINE_CONFIG_DERIVED( macse, macplus )

	MCFG_DEVICE_REMOVE("via6522_0")
	MCFG_VIA6522_ADD("via6522_0", 1000000, mac_via6522_adb_intf)

	/* internal ram */
	MCFG_RAM_MODIFY(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("4M")
	MCFG_RAM_EXTRA_OPTIONS("2M,2560K,4M")
MACHINE_CONFIG_END

static MACHINE_CONFIG_START( macprtb, mac_state )

	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", M68000, 7833600*2)
	MCFG_CPU_PROGRAM_MAP(macprtb_map)
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(60.15)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(1260))
	MCFG_QUANTUM_TIME(attotime::from_hz(60))

    /* video hardware */
	MCFG_VIDEO_ATTRIBUTES(VIDEO_UPDATE_BEFORE_VBLANK)
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_SIZE(700, 480)
	MCFG_SCREEN_VISIBLE_AREA(0, 639, 0, 399)
	MCFG_SCREEN_UPDATE(mac_prtb)

	MCFG_PALETTE_LENGTH(2)
	MCFG_PALETTE_INIT(mac)

	MCFG_VIDEO_START(mac_prtb)

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_STEREO("lspeaker", "rspeaker")
	MCFG_ASC_ADD("asc", C15M, ASC_TYPE_ASC, mac_asc_irq)
	MCFG_SOUND_ROUTE(0, "lspeaker", 1.0)
	MCFG_SOUND_ROUTE(1, "rspeaker", 1.0)

	/* nvram */
	MCFG_NVRAM_HANDLER(mac)

	/* devices */
	MCFG_NCR5380_ADD("ncr5380", 7833600, macplus_5380intf)

	MCFG_IWM_ADD("fdc", mac_iwm_interface)
	MCFG_FLOPPY_SONY_2_DRIVES_ADD(mac128512_floppy_config)

	MCFG_SCC8530_ADD("scc", 7833600)
	MCFG_SCC8530_IRQ(mac_scc_irq)
	MCFG_VIA6522_ADD("via6522_0", 783360, mac_via6522_intf)

	MCFG_HARDDISK_ADD( "harddisk1" )
	MCFG_HARDDISK_ADD( "harddisk2" )

	/* internal ram */
	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("1M")
	MCFG_RAM_EXTRA_OPTIONS("1M,3M,5M,7M,9M")

MACHINE_CONFIG_END

static MACHINE_CONFIG_START( macii, mac_state )

	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", M68020HMMU, 7833600*2)
	MCFG_CPU_PROGRAM_MAP(macii_map)
	MCFG_CPU_VBLANK_INT("screen", mac_cb264_vbl)

	MCFG_SCREEN_ADD("screen", RASTER)
	// dot clock, htotal, hstart, hend, vtotal, vstart, vend
	MCFG_SCREEN_RAW_PARAMS(25175000, 800, 0, 640, 525, 0, 480)

        /* video hardware */
	MCFG_VIDEO_ATTRIBUTES(VIDEO_UPDATE_BEFORE_VBLANK)
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_RGB32)
	MCFG_SCREEN_SIZE(1024, 768)
	MCFG_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
	MCFG_SCREEN_UPDATE(mac_cb264)

	MCFG_PALETTE_LENGTH(256)

	MCFG_VIDEO_START(mac_cb264)

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_STEREO("lspeaker", "rspeaker")
	MCFG_ASC_ADD("asc", C15M, ASC_TYPE_ASC, mac_asc_irq)
	MCFG_SOUND_ROUTE(0, "lspeaker", 1.0)
	MCFG_SOUND_ROUTE(1, "rspeaker", 1.0)

	/* nvram */
	MCFG_NVRAM_HANDLER(mac)

	/* devices */
	MCFG_NCR5380_ADD("ncr5380", 7833600, macplus_5380intf)

	MCFG_IWM_ADD("fdc", mac_iwm_interface)
	MCFG_FLOPPY_SONY_2_DRIVES_ADD(mac_floppy_config)

	MCFG_SCC8530_ADD("scc", 7833600)
	MCFG_SCC8530_IRQ(mac_scc_irq)

	MCFG_VIA6522_ADD("via6522_0", 783360, mac_via6522_adb_intf)
	MCFG_VIA6522_ADD("via6522_1", 783360, mac_via6522_2_intf)

	MCFG_HARDDISK_ADD( "harddisk1" )
	MCFG_HARDDISK_ADD( "harddisk2" )

	/* internal ram */
	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("2M")
	MCFG_RAM_EXTRA_OPTIONS("8M,32M,64M,96M,128M")
MACHINE_CONFIG_END

static MACHINE_CONFIG_DERIVED( maclc, macii )

	MCFG_CPU_REPLACE("maincpu", M68020HMMU, 7833600*2)
	MCFG_CPU_PROGRAM_MAP(maclc_map)
	MCFG_CPU_VBLANK_INT("screen", mac_rbv_vbl)

	MCFG_PALETTE_LENGTH(256)

	MCFG_VIDEO_START(macv8)
	MCFG_VIDEO_RESET(macrbv)
	
	MCFG_SCREEN_MODIFY("screen")
	MCFG_SCREEN_UPDATE(macrbvvram)

	MCFG_RAM_MODIFY(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("2M")
	MCFG_RAM_EXTRA_OPTIONS("4M,6M,8M,10M")

	MCFG_ASC_REPLACE("asc", C15M, ASC_TYPE_V8, mac_asc_irq)
	MCFG_SOUND_ROUTE(0, "lspeaker", 1.0)
	MCFG_SOUND_ROUTE(1, "rspeaker", 1.0)
MACHINE_CONFIG_END

static MACHINE_CONFIG_DERIVED( maclc2, maclc )

	MCFG_CPU_REPLACE("maincpu", M68030, 7833600*2)
	MCFG_CPU_PROGRAM_MAP(maclc_map)
	MCFG_CPU_VBLANK_INT("screen", mac_rbv_vbl)

	MCFG_RAM_MODIFY(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("4M")
	MCFG_RAM_EXTRA_OPTIONS("6M,8M,10M")
MACHINE_CONFIG_END

static MACHINE_CONFIG_DERIVED( maclc3, maclc )

	MCFG_CPU_REPLACE("maincpu", M68030, 25000000)
	MCFG_CPU_PROGRAM_MAP(maclc3_map)
	MCFG_CPU_VBLANK_INT("screen", mac_rbv_vbl)

	MCFG_VIDEO_START(macsonora)
	MCFG_VIDEO_RESET(macrbv)
	
	MCFG_SCREEN_MODIFY("screen")
	MCFG_SCREEN_UPDATE(macrbvvram)

	MCFG_RAM_MODIFY(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("4M")
	MCFG_RAM_EXTRA_OPTIONS("8M,12M,16M,20M,24M,28M,32M,36M")

	MCFG_ASC_REPLACE("asc", C15M, ASC_TYPE_SONORA, mac_asc_irq)
	MCFG_SOUND_ROUTE(0, "lspeaker", 1.0)
	MCFG_SOUND_ROUTE(1, "rspeaker", 1.0)
MACHINE_CONFIG_END

static MACHINE_CONFIG_DERIVED( maciix, macii )

	MCFG_CPU_REPLACE("maincpu", M68030, 7833600*2)
	MCFG_CPU_PROGRAM_MAP(macii_map)
	MCFG_CPU_VBLANK_INT("screen", mac_cb264_vbl)

	MCFG_RAM_MODIFY(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("2M")
	MCFG_RAM_EXTRA_OPTIONS("8M,32M,64M,96M,128M")
MACHINE_CONFIG_END

static MACHINE_CONFIG_START( macse30, mac_state )

	MCFG_CPU_ADD("maincpu", M68030, 7833600*2)
	MCFG_CPU_PROGRAM_MAP(macse30_map)

	MCFG_QUANTUM_TIME(attotime::from_hz(60))

       /* video hardware */
	MCFG_VIDEO_ATTRIBUTES(VIDEO_UPDATE_BEFORE_VBLANK)
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(60.15)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(1260))
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_SIZE(MAC_H_TOTAL, MAC_V_TOTAL)
	MCFG_SCREEN_VISIBLE_AREA(0, MAC_H_VIS-1, 0, MAC_V_VIS-1)
	MCFG_SCREEN_UPDATE(macse30)

	MCFG_PALETTE_LENGTH(2)
	MCFG_PALETTE_INIT(mac)

	MCFG_VIDEO_START(mac)

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_STEREO("lspeaker", "rspeaker")
	MCFG_ASC_ADD("asc", C15M, ASC_TYPE_ASC, mac_asc_irq)
	MCFG_SOUND_ROUTE(0, "lspeaker", 1.0)
	MCFG_SOUND_ROUTE(1, "rspeaker", 1.0)

	/* nvram */
	MCFG_NVRAM_HANDLER(mac)

	/* devices */
	MCFG_NCR5380_ADD("ncr5380", 7833600, macplus_5380intf)

	MCFG_IWM_ADD("fdc", mac_iwm_interface)
	MCFG_FLOPPY_SONY_2_DRIVES_ADD(mac_floppy_config)

	MCFG_SCC8530_ADD("scc", 7833600)
	MCFG_SCC8530_IRQ(mac_scc_irq)

	MCFG_VIA6522_ADD("via6522_0", 783360, mac_via6522_adb_intf)
	MCFG_VIA6522_ADD("via6522_1", 783360, mac_via6522_2_intf)

	MCFG_HARDDISK_ADD( "harddisk1" )
	MCFG_HARDDISK_ADD( "harddisk2" )

	/* internal ram */
	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("2M")
	MCFG_RAM_EXTRA_OPTIONS("8M,16M,32M,48M,64M,96M,128M")
MACHINE_CONFIG_END

static MACHINE_CONFIG_DERIVED( macclas2, maclc )

	MCFG_CPU_REPLACE("maincpu", M68030, 7833600*2)
	MCFG_CPU_PROGRAM_MAP(maclc_map)

	MCFG_VIDEO_START(macv8)
	MCFG_VIDEO_RESET(maceagle)

	MCFG_SCREEN_MODIFY("screen")
	MCFG_SCREEN_SIZE(MAC_H_TOTAL, MAC_V_TOTAL)
	MCFG_SCREEN_VISIBLE_AREA(0, MAC_H_VIS-1, 0, MAC_V_VIS-1)
	MCFG_SCREEN_UPDATE(macrbv)

	MCFG_ASC_REPLACE("asc", C15M, ASC_TYPE_EAGLE, mac_asc_irq)
	MCFG_SOUND_ROUTE(0, "lspeaker", 1.0)
	MCFG_SOUND_ROUTE(1, "rspeaker", 1.0)

	MCFG_RAM_MODIFY(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("10M")
	MCFG_RAM_EXTRA_OPTIONS("2M,4M,6M,8M,10M")
MACHINE_CONFIG_END

static MACHINE_CONFIG_DERIVED( maciici, macii )

	MCFG_CPU_REPLACE("maincpu", M68030, 25000000)
	MCFG_CPU_PROGRAM_MAP(maciici_map)
	MCFG_CPU_VBLANK_INT("screen", mac_rbv_vbl)

	MCFG_PALETTE_LENGTH(256)

	MCFG_VIDEO_START(macrbv)
	MCFG_VIDEO_RESET(macrbv)

	MCFG_SCREEN_MODIFY("screen")
	MCFG_SCREEN_SIZE(640, 870)
	MCFG_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
	MCFG_SCREEN_UPDATE(macrbv)

	/* internal ram */
	MCFG_RAM_MODIFY(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("2M")
	MCFG_RAM_EXTRA_OPTIONS("4M,8M,16M,32M,48M,64M")
MACHINE_CONFIG_END

static MACHINE_CONFIG_DERIVED( maciisi, macii )

	MCFG_CPU_REPLACE("maincpu", M68030, 20000000)
	MCFG_CPU_PROGRAM_MAP(maciici_map)
	MCFG_CPU_VBLANK_INT("screen", mac_rbv_vbl)

	MCFG_PALETTE_LENGTH(256)

	MCFG_VIDEO_START(macrbv)
	MCFG_VIDEO_RESET(macrbv)

	MCFG_SCREEN_MODIFY("screen")
	MCFG_SCREEN_SIZE(640, 870)
	MCFG_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
	MCFG_SCREEN_UPDATE(macrbv)

	/* internal ram */
	MCFG_RAM_MODIFY(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("2M")
	MCFG_RAM_EXTRA_OPTIONS("4M,8M,16M,32M,48M,64M")
MACHINE_CONFIG_END

static MACHINE_CONFIG_START( pwrmac, mac_state )

	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", PPC601, 66000000)
	MCFG_CPU_PROGRAM_MAP(pwrmac_map)

    /* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	// dot clock, htotal, hstart, hend, vtotal, vstart, vend
	MCFG_SCREEN_RAW_PARAMS(25175000, 800, 0, 640, 525, 0, 480)
	MCFG_VIDEO_ATTRIBUTES(VIDEO_UPDATE_BEFORE_VBLANK)
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_RGB32)
	MCFG_SCREEN_SIZE(1024, 768)
	MCFG_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
	MCFG_SCREEN_UPDATE(macrbv)

	MCFG_PALETTE_LENGTH(256)

	MCFG_VIDEO_START(macsonora)
	MCFG_VIDEO_RESET(macrbv)

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_STEREO("lspeaker", "rspeaker")

	/* nvram */
	MCFG_NVRAM_HANDLER(mac)

	/* devices */
	MCFG_NCR5380_ADD("ncr5380", 7833600, macplus_5380intf)

	MCFG_IWM_ADD("fdc", mac_iwm_interface)
	MCFG_FLOPPY_SONY_2_DRIVES_ADD(mac_floppy_config)

	MCFG_SCC8530_ADD("scc", 7833600)
	MCFG_SCC8530_IRQ(mac_scc_irq)

	MCFG_VIA6522_ADD("via6522_0", 783360, mac_via6522_adb_intf)
	MCFG_VIA6522_ADD("via6522_1", 783360, mac_via6522_2_intf)

	MCFG_HARDDISK_ADD( "harddisk1" )
	MCFG_HARDDISK_ADD( "harddisk2" )

	/* internal ram */
	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("8M")
	MCFG_RAM_EXTRA_OPTIONS("16M,32M,64M,128M")
MACHINE_CONFIG_END

static INPUT_PORTS_START( macplus )
	PORT_START("MOUSE0") /* Mouse - button */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_NAME("Mouse Button") PORT_CODE(MOUSECODE_BUTTON1)

	PORT_START("MOUSE1") /* Mouse - X AXIS */
	PORT_BIT( 0xff, 0x00, IPT_MOUSE_X) PORT_SENSITIVITY(100) PORT_KEYDELTA(0) PORT_PLAYER(1)

	PORT_START("MOUSE2") /* Mouse - Y AXIS */
	PORT_BIT( 0xff, 0x00, IPT_MOUSE_Y) PORT_SENSITIVITY(100) PORT_KEYDELTA(0) PORT_PLAYER(1)

	/* R Nabet 000531 : pseudo-input ports with keyboard layout */
	/* we only define US layout for keyboard - international layout is different! */
	/* note : 16 bits at most per port! */

	/* main keyboard pad */

	PORT_START("KEY0")
	PORT_BIT(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_A) 			PORT_CHAR('a') PORT_CHAR('A')
	PORT_BIT(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_S) 			PORT_CHAR('s') PORT_CHAR('S')
	PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_D) 			PORT_CHAR('d') PORT_CHAR('D')
	PORT_BIT(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_F) 			PORT_CHAR('f') PORT_CHAR('F')
	PORT_BIT(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_H) 			PORT_CHAR('h') PORT_CHAR('H')
	PORT_BIT(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_G) 			PORT_CHAR('g') PORT_CHAR('G')
	PORT_BIT(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_Z) 			PORT_CHAR('z') PORT_CHAR('Z')
	PORT_BIT(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_X) 			PORT_CHAR('x') PORT_CHAR('X')
	PORT_BIT(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_C) 			PORT_CHAR('c') PORT_CHAR('C')
	PORT_BIT(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_V) 			PORT_CHAR('v') PORT_CHAR('V')
	PORT_BIT(0x0400, IP_ACTIVE_HIGH, IPT_UNUSED)	/* extra key on ISO : */
	PORT_BIT(0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_B) 			PORT_CHAR('b') PORT_CHAR('B')
	PORT_BIT(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_Q) 			PORT_CHAR('q') PORT_CHAR('Q')
	PORT_BIT(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_W) 			PORT_CHAR('w') PORT_CHAR('W')
	PORT_BIT(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_E) 			PORT_CHAR('e') PORT_CHAR('E')
	PORT_BIT(0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_R) 			PORT_CHAR('r') PORT_CHAR('R')

	PORT_START("KEY1")
	PORT_BIT(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_Y) 			PORT_CHAR('y') PORT_CHAR('Y')
	PORT_BIT(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_T) 			PORT_CHAR('t') PORT_CHAR('T')
	PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_1) 			PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_2) 			PORT_CHAR('2') PORT_CHAR('@')
	PORT_BIT(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_3) 			PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_4) 			PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_6) 			PORT_CHAR('6') PORT_CHAR('^')
	PORT_BIT(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_5) 			PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_EQUALS)		PORT_CHAR('=') PORT_CHAR('+')
	PORT_BIT(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_9) 			PORT_CHAR('9') PORT_CHAR('(')
	PORT_BIT(0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_7) 			PORT_CHAR('7') PORT_CHAR('&')
	PORT_BIT(0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_MINUS)			PORT_CHAR('-') PORT_CHAR('_')
	PORT_BIT(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_8) 			PORT_CHAR('8') PORT_CHAR('*')
	PORT_BIT(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_0) 			PORT_CHAR('0') PORT_CHAR(')')
	PORT_BIT(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_CLOSEBRACE)	PORT_CHAR(']') PORT_CHAR('}')
	PORT_BIT(0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_O) 			PORT_CHAR('o') PORT_CHAR('O')

	PORT_START("KEY2")
	PORT_BIT(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_U)				PORT_CHAR('u') PORT_CHAR('U')
	PORT_BIT(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_OPENBRACE)		PORT_CHAR('[') PORT_CHAR('{')
	PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_I) 			PORT_CHAR('i') PORT_CHAR('I')
	PORT_BIT(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_P) 			PORT_CHAR('p') PORT_CHAR('P')
	PORT_BIT(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Return") PORT_CODE(KEYCODE_ENTER) PORT_CHAR('\r')
	PORT_BIT(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_L) 			PORT_CHAR('l') PORT_CHAR('L')
	PORT_BIT(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_J) 			PORT_CHAR('j') PORT_CHAR('J')
	PORT_BIT(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_QUOTE)			PORT_CHAR('\'') PORT_CHAR('"')
	PORT_BIT(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_K) 			PORT_CHAR('k') PORT_CHAR('K')
	PORT_BIT(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_COLON)			PORT_CHAR(';') PORT_CHAR(':')
	PORT_BIT(0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_BACKSLASH)		PORT_CHAR('\\') PORT_CHAR('|')
	PORT_BIT(0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_COMMA) 		PORT_CHAR(',') PORT_CHAR('<')
	PORT_BIT(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_SLASH)			PORT_CHAR('/') PORT_CHAR('?')
	PORT_BIT(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_N) 			PORT_CHAR('n') PORT_CHAR('N')
	PORT_BIT(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_M) 			PORT_CHAR('m') PORT_CHAR('M')
	PORT_BIT(0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_STOP)			PORT_CHAR('.') PORT_CHAR('>')

	PORT_START("KEY3")
	PORT_BIT(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_TAB)			PORT_CHAR('\t')
	PORT_BIT(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_SPACE)			PORT_CHAR(' ')
	PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_TILDE)			PORT_CHAR('`') PORT_CHAR('~')
	PORT_BIT(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_BACKSPACE)		PORT_CHAR(8)
	PORT_BIT(0x0010, IP_ACTIVE_HIGH, IPT_UNUSED)	/* keyboard Enter : */
	PORT_BIT(0x0020, IP_ACTIVE_HIGH, IPT_UNUSED)	/* escape: */
	PORT_BIT(0x0040, IP_ACTIVE_HIGH, IPT_UNUSED)	/* ??? */
	PORT_BIT(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Command") PORT_CODE(KEYCODE_LCONTROL)
	PORT_BIT(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Shift") PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Caps Lock") PORT_CODE(KEYCODE_CAPSLOCK) PORT_TOGGLE
	PORT_BIT(0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Option") PORT_CODE(KEYCODE_LALT) PORT_CHAR(UCHAR_SHIFT_2)
	PORT_BIT(0x0800, IP_ACTIVE_HIGH, IPT_UNUSED)	/* Control: */
	PORT_BIT(0x1000, IP_ACTIVE_HIGH, IPT_UNUSED)	/* keypad pseudo-keycode */
	PORT_BIT(0xE000, IP_ACTIVE_HIGH, IPT_UNUSED)	/* ??? */

	/* keypad */
	PORT_START("KEY4")
	PORT_BIT(0x0001, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_DEL_PAD)			PORT_CHAR(UCHAR_MAMEKEY(DEL_PAD))
	PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_ASTERISK)			PORT_CHAR(UCHAR_MAMEKEY(ASTERISK))
	PORT_BIT(0x0038, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_PLUS_PAD)			PORT_CHAR(UCHAR_MAMEKEY(PLUS_PAD))
	PORT_BIT(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Keypad Clear") PORT_CODE(/*KEYCODE_NUMLOCK*/KEYCODE_DEL) PORT_CHAR(UCHAR_MAMEKEY(DEL))
	PORT_BIT(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Keypad =") PORT_CODE(/*CODE_OTHER*/KEYCODE_NUMLOCK) PORT_CHAR(UCHAR_MAMEKEY(NUMLOCK))
	PORT_BIT(0x0E00, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_ENTER_PAD)			PORT_CHAR(UCHAR_MAMEKEY(ENTER_PAD))
	PORT_BIT(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_SLASH_PAD)			PORT_CHAR(UCHAR_MAMEKEY(SLASH_PAD))
	PORT_BIT(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_MINUS_PAD)			PORT_CHAR(UCHAR_MAMEKEY(MINUS_PAD))
	PORT_BIT(0x8000, IP_ACTIVE_HIGH, IPT_UNUSED)

	PORT_START("KEY5")
	PORT_BIT(0x0001, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x0002, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_0_PAD) 			PORT_CHAR(UCHAR_MAMEKEY(0_PAD))
	PORT_BIT(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_1_PAD) 			PORT_CHAR(UCHAR_MAMEKEY(1_PAD))
	PORT_BIT(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_2_PAD) 			PORT_CHAR(UCHAR_MAMEKEY(2_PAD))
	PORT_BIT(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_3_PAD) 			PORT_CHAR(UCHAR_MAMEKEY(3_PAD))
	PORT_BIT(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_4_PAD) 			PORT_CHAR(UCHAR_MAMEKEY(4_PAD))
	PORT_BIT(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_5_PAD) 			PORT_CHAR(UCHAR_MAMEKEY(5_PAD))
	PORT_BIT(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_6_PAD) 			PORT_CHAR(UCHAR_MAMEKEY(6_PAD))
	PORT_BIT(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_7_PAD) 			PORT_CHAR(UCHAR_MAMEKEY(7_PAD))
	PORT_BIT(0x0400, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_8_PAD) 			PORT_CHAR(UCHAR_MAMEKEY(8_PAD))
	PORT_BIT(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_9_PAD) 			PORT_CHAR(UCHAR_MAMEKEY(9_PAD))
	PORT_BIT(0xE000, IP_ACTIVE_HIGH, IPT_UNUSED)

	/* Arrow keys */
	PORT_START("KEY6")
	PORT_BIT(0x0003, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Right Arrow") PORT_CODE(KEYCODE_RIGHT)	PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_BIT(0x0038, IP_ACTIVE_HIGH, IPT_UNUSED	)
	PORT_BIT(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Left Arrow") PORT_CODE(KEYCODE_LEFT)		PORT_CHAR(UCHAR_MAMEKEY(LEFT))
	PORT_BIT(0x0080, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Down Arrow") PORT_CODE(KEYCODE_DOWN)		PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	PORT_BIT(0x1E00, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Up Arrow") PORT_CODE(KEYCODE_UP)			PORT_CHAR(UCHAR_MAMEKEY(UP))
	PORT_BIT(0xC000, IP_ACTIVE_HIGH, IPT_UNUSED)
INPUT_PORTS_END

static INPUT_PORTS_START( macadb )
	PORT_START("MOUSE0") /* Mouse - button */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_NAME("Mouse Button") PORT_CODE(MOUSECODE_BUTTON1)

	PORT_START("MOUSE1") /* Mouse - X AXIS */
	PORT_BIT( 0xff, 0x00, IPT_MOUSE_X) PORT_SENSITIVITY(100) PORT_KEYDELTA(0) PORT_PLAYER(1)

	PORT_START("MOUSE2") /* Mouse - Y AXIS */
	PORT_BIT( 0xff, 0x00, IPT_MOUSE_Y) PORT_SENSITIVITY(100) PORT_KEYDELTA(0) PORT_PLAYER(1)

        /* This handles the standard (not Extended) Apple ADB keyboard, which is similar to the IIgs's */
	/* main keyboard */

	PORT_START("KEY0")
	PORT_BIT(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_A) 			PORT_CHAR('a') PORT_CHAR('A')
	PORT_BIT(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_S) 			PORT_CHAR('s') PORT_CHAR('S')
	PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_D) 			PORT_CHAR('d') PORT_CHAR('D')
	PORT_BIT(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_F) 			PORT_CHAR('f') PORT_CHAR('F')
	PORT_BIT(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_H) 			PORT_CHAR('h') PORT_CHAR('H')
	PORT_BIT(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_G) 			PORT_CHAR('g') PORT_CHAR('G')
	PORT_BIT(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_Z) 			PORT_CHAR('z') PORT_CHAR('Z')
	PORT_BIT(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_X) 			PORT_CHAR('x') PORT_CHAR('X')
	PORT_BIT(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_C) 			PORT_CHAR('c') PORT_CHAR('C')
	PORT_BIT(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_V) 			PORT_CHAR('v') PORT_CHAR('V')
	PORT_BIT(0x0400, IP_ACTIVE_HIGH, IPT_UNUSED)	/* extra key on ISO : */
	PORT_BIT(0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_B) 			PORT_CHAR('b') PORT_CHAR('B')
	PORT_BIT(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_Q) 			PORT_CHAR('q') PORT_CHAR('Q')
	PORT_BIT(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_W) 			PORT_CHAR('w') PORT_CHAR('W')
	PORT_BIT(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_E) 			PORT_CHAR('e') PORT_CHAR('E')
	PORT_BIT(0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_R) 			PORT_CHAR('r') PORT_CHAR('R')

	PORT_START("KEY1")
	PORT_BIT(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_Y) 			PORT_CHAR('y') PORT_CHAR('Y')
	PORT_BIT(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_T) 			PORT_CHAR('t') PORT_CHAR('T')
	PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_1) 			PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_2) 			PORT_CHAR('2') PORT_CHAR('@')
	PORT_BIT(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_3) 			PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_4) 			PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_6) 			PORT_CHAR('6') PORT_CHAR('^')
	PORT_BIT(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_5) 			PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_EQUALS)		PORT_CHAR('=') PORT_CHAR('+')
	PORT_BIT(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_9) 			PORT_CHAR('9') PORT_CHAR('(')
	PORT_BIT(0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_7) 			PORT_CHAR('7') PORT_CHAR('&')
	PORT_BIT(0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_MINUS)			PORT_CHAR('-') PORT_CHAR('_')
	PORT_BIT(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_8) 			PORT_CHAR('8') PORT_CHAR('*')
	PORT_BIT(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_0) 			PORT_CHAR('0') PORT_CHAR(')')
	PORT_BIT(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_CLOSEBRACE)	PORT_CHAR(']') PORT_CHAR('}')
	PORT_BIT(0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_O) 			PORT_CHAR('o') PORT_CHAR('O')

	PORT_START("KEY2")
	PORT_BIT(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_U)				PORT_CHAR('u') PORT_CHAR('U')
	PORT_BIT(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_OPENBRACE)		PORT_CHAR('[') PORT_CHAR('{')
	PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_I) 			PORT_CHAR('i') PORT_CHAR('I')
	PORT_BIT(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_P) 			PORT_CHAR('p') PORT_CHAR('P')
	PORT_BIT(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Return") PORT_CODE(KEYCODE_ENTER) PORT_CHAR('\r')
	PORT_BIT(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_L) 			PORT_CHAR('l') PORT_CHAR('L')
	PORT_BIT(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_J) 			PORT_CHAR('j') PORT_CHAR('J')
	PORT_BIT(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_QUOTE)			PORT_CHAR('\'') PORT_CHAR('"')
	PORT_BIT(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_K) 			PORT_CHAR('k') PORT_CHAR('K')
	PORT_BIT(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_COLON)			PORT_CHAR(';') PORT_CHAR(':')
	PORT_BIT(0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_BACKSLASH)		PORT_CHAR('\\') PORT_CHAR('|')
	PORT_BIT(0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_COMMA) 		PORT_CHAR(',') PORT_CHAR('<')
	PORT_BIT(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_SLASH)			PORT_CHAR('/') PORT_CHAR('?')
	PORT_BIT(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_N) 			PORT_CHAR('n') PORT_CHAR('N')
	PORT_BIT(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_M) 			PORT_CHAR('m') PORT_CHAR('M')
	PORT_BIT(0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_STOP)			PORT_CHAR('.') PORT_CHAR('>')

	PORT_START("KEY3")
	PORT_BIT(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_TAB)			PORT_CHAR('\t')
	PORT_BIT(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_SPACE)			PORT_CHAR(' ')
	PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_TILDE)			PORT_CHAR('`') PORT_CHAR('~')
	PORT_BIT(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_BACKSPACE)		PORT_CHAR(8)
	PORT_BIT(0x0010, IP_ACTIVE_HIGH, IPT_UNUSED)	/* keyboard Enter : */
	PORT_BIT(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Esc")		PORT_CODE(KEYCODE_ESC)		PORT_CHAR(27)
	PORT_BIT(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Control") PORT_CODE(KEYCODE_LCONTROL)
	PORT_BIT(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Command / Open Apple") PORT_CODE(KEYCODE_LALT)
	PORT_BIT(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Shift") PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Caps Lock") PORT_CODE(KEYCODE_CAPSLOCK) PORT_TOGGLE
	PORT_BIT(0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Option / Solid Apple") PORT_CODE(KEYCODE_RALT)
	PORT_BIT(0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Left Arrow") PORT_CODE(KEYCODE_LEFT)		PORT_CHAR(UCHAR_MAMEKEY(LEFT))
	PORT_BIT(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Right Arrow") PORT_CODE(KEYCODE_RIGHT)	PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_BIT(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Down Arrow") PORT_CODE(KEYCODE_DOWN)		PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	PORT_BIT(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Up Arrow") PORT_CODE(KEYCODE_UP)			PORT_CHAR(UCHAR_MAMEKEY(UP))
	PORT_BIT(0x8000, IP_ACTIVE_HIGH, IPT_UNUSED)	/* ??? */

	/* keypad */
	PORT_START("KEY4")
	PORT_BIT(0x0001, IP_ACTIVE_HIGH, IPT_UNUSED)	// 0x40
	PORT_BIT(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_DEL_PAD)			PORT_CHAR(UCHAR_MAMEKEY(DEL_PAD))	// 0x41
	PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_UNUSED)	// 0x42
	PORT_BIT(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_ASTERISK)			PORT_CHAR(UCHAR_MAMEKEY(ASTERISK))	// 0x43
	PORT_BIT(0x0010, IP_ACTIVE_HIGH, IPT_UNUSED)	// 0x44
	PORT_BIT(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_PLUS_PAD)			PORT_CHAR(UCHAR_MAMEKEY(PLUS_PAD)) // 0x45
	PORT_BIT(0x0040, IP_ACTIVE_HIGH, IPT_UNUSED)	// 0x46
	PORT_BIT(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Keypad Clear") PORT_CODE(/*KEYCODE_NUMLOCK*/KEYCODE_DEL) PORT_CHAR(UCHAR_MAMEKEY(DEL))	// 0x47
	PORT_BIT(0x0700, IP_ACTIVE_HIGH, IPT_UNUSED)	// 0x48, 49, 4a
	PORT_BIT(0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_SLASH_PAD)			PORT_CHAR(UCHAR_MAMEKEY(SLASH_PAD))	// 0x4b
	PORT_BIT(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_ENTER_PAD)			PORT_CHAR(UCHAR_MAMEKEY(ENTER_PAD))	// 0x4c
	PORT_BIT(0x2000, IP_ACTIVE_HIGH, IPT_UNUSED)	// 0x4d
	PORT_BIT(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_MINUS_PAD)			PORT_CHAR(UCHAR_MAMEKEY(MINUS_PAD)) // 0x4e
	PORT_BIT(0x8000, IP_ACTIVE_HIGH, IPT_UNUSED)	// 0x4f

	PORT_START("KEY5")
	PORT_BIT(0x0001, IP_ACTIVE_HIGH, IPT_UNUSED)	// 0x50
	PORT_BIT(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Keypad =") PORT_CODE(/*CODE_OTHER*/KEYCODE_NUMLOCK) PORT_CHAR(UCHAR_MAMEKEY(NUMLOCK))	// 0x51
	PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_0_PAD) 			PORT_CHAR(UCHAR_MAMEKEY(0_PAD))	// 0x52
	PORT_BIT(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_1_PAD) 			PORT_CHAR(UCHAR_MAMEKEY(1_PAD)) // 0x53
	PORT_BIT(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_2_PAD) 			PORT_CHAR(UCHAR_MAMEKEY(2_PAD)) // 0x54
	PORT_BIT(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_3_PAD) 			PORT_CHAR(UCHAR_MAMEKEY(3_PAD)) // 0x55
	PORT_BIT(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_4_PAD) 			PORT_CHAR(UCHAR_MAMEKEY(4_PAD)) // 0x56
	PORT_BIT(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_5_PAD) 			PORT_CHAR(UCHAR_MAMEKEY(5_PAD)) // 0x57
	PORT_BIT(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_6_PAD) 			PORT_CHAR(UCHAR_MAMEKEY(6_PAD)) // 0x58
	PORT_BIT(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_7_PAD) 			PORT_CHAR(UCHAR_MAMEKEY(7_PAD))	// 0x59
	PORT_BIT(0x0400, IP_ACTIVE_HIGH, IPT_UNUSED)	// 0x5a
	PORT_BIT(0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_8_PAD) 			PORT_CHAR(UCHAR_MAMEKEY(8_PAD))	// 0x5b
	PORT_BIT(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_9_PAD) 			PORT_CHAR(UCHAR_MAMEKEY(9_PAD))	// 0x5c
	PORT_BIT(0xE000, IP_ACTIVE_HIGH, IPT_UNUSED)
INPUT_PORTS_END

INPUT_PORTS_START( maciici )
	PORT_INCLUDE(macadb)

	PORT_START("MONTYPE")
	PORT_CONFNAME(0x0f, 0x06, "Connected monitor")
	PORT_CONFSETTING( 0x01, "15\" Portrait Display (640x870)")
	PORT_CONFSETTING( 0x02, "12\" RGB (512x384)")
	PORT_CONFSETTING( 0x06, "13\" RGB (640x480)")
INPUT_PORTS_END

/***************************************************************************

  Game driver(s)

  The Mac driver uses a convention of placing the BIOS in "bootrom"

***************************************************************************/

ROM_START( mac128k )
	ROM_REGION16_BE(0x20000, "bootrom", 0)
	ROM_LOAD16_WORD( "mac128k.rom",  0x00000, 0x10000, CRC(6d0c8a28) SHA1(9d86c883aa09f7ef5f086d9e32330ef85f1bc93b) )
ROM_END

ROM_START( mac512k )
	ROM_REGION16_BE(0x20000, "bootrom", 0)
	ROM_LOAD16_WORD( "mac512k.rom",  0x00000, 0x10000, CRC(cf759e0d) SHA1(5b1ced181b74cecd3834c49c2a4aa1d7ffe944d7) )
ROM_END

ROM_START( mac512ke )
	ROM_REGION16_BE(0x20000, "bootrom", 0)
	ROM_LOAD16_WORD( "macplus.rom",  0x00000, 0x20000, CRC(b2102e8e) SHA1(7d2f808a045aa3a1b242764f0e2c7d13e288bf1f))
ROM_END


ROM_START( macplus )
	ROM_REGION16_BE(0x20000, "bootrom", 0)
	ROM_LOAD16_WORD( "macplus.rom",  0x00000, 0x20000, CRC(b2102e8e) SHA1(7d2f808a045aa3a1b242764f0e2c7d13e288bf1f))
ROM_END


ROM_START( macse )
	ROM_REGION16_BE(0x40000, "bootrom", 0)
	ROM_LOAD16_WORD( "macse.rom",  0x00000, 0x40000, CRC(0f7ff80c) SHA1(58532b7d0d49659fd5228ac334a1b094f0241968))
ROM_END

ROM_START( macsefd )
	ROM_REGION16_BE(0x40000, "bootrom", 0)
        ROM_LOAD( "be06e171.rom", 0x000000, 0x040000, CRC(f530cb10) SHA1(d3670a90273d12e53d86d1228c068cb660b8c9d1) )
ROM_END

ROM_START( macclasc )
	ROM_REGION16_BE(0x80000, "bootrom", 0)
        ROM_LOAD( "a49f9914.rom", 0x000000, 0x080000, CRC(510d7d38) SHA1(ccd10904ddc0fb6a1d216b2e9effd5ec6cf5a83d) )
ROM_END

ROM_START( maclc )
	ROM_REGION32_BE(0x100000, "bootrom", 0)
        ROM_LOAD("350eacf0.rom", 0x000000, 0x080000, CRC(71681726) SHA1(6bef5853ae736f3f06c2b4e79772f65910c3b7d4))

	ROM_REGION(0x1100, "egret", 0)
	ROM_LOAD( "341s0851.bin", 0x000000, 0x001100, CRC(ea9ea6e4) SHA1(8b0dae3ec66cdddbf71567365d2c462688aeb571) )
ROM_END

ROM_START( macii )
	ROM_REGION32_BE(0x40000, "bootrom", 0)
	ROM_LOAD( "9779d2c4.rom", 0x000000, 0x040000, CRC(4df6d054) SHA1(db6b504744281369794e26ba71a6e385cf6227fa) )

	// RasterOps "ColorBoard 264" NuBus video card
	ROM_REGION32_BE(0x8000, "rops264", 0)
        ROM_LOAD32_BYTE( "264-1914.bin", 0x000003, 0x002000, CRC(d5fbd5ad) SHA1(98d35ed3fb0bca4a9bee1cdb2af0d3f22b379386) )
        ROM_LOAD32_BYTE( "264-1915.bin", 0x000002, 0x002000, CRC(26c19ee5) SHA1(2b2853d04cc6b0258e85eccd23ebfd4f4f63a084) )
ROM_END

ROM_START( mac2fdhd )	// same ROM for II FDHD, IIx, IIcx, and SE/30
	ROM_REGION32_BE(0x40000, "bootrom", 0)
        ROM_LOAD( "97221136.rom", 0x000000, 0x040000, CRC(ce3b966f) SHA1(753b94351d94c369616c2c87b19d568dc5e2764e) )

	// RasterOps "ColorBoard 264" NuBus video card
	ROM_REGION32_BE(0x8000, "rops264", 0)
        ROM_LOAD32_BYTE( "264-1914.bin", 0x000003, 0x002000, CRC(d5fbd5ad) SHA1(98d35ed3fb0bca4a9bee1cdb2af0d3f22b379386) )
        ROM_LOAD32_BYTE( "264-1915.bin", 0x000002, 0x002000, CRC(26c19ee5) SHA1(2b2853d04cc6b0258e85eccd23ebfd4f4f63a084) )
ROM_END

ROM_START( maciix )
	ROM_REGION32_BE(0x40000, "bootrom", 0)
        ROM_LOAD( "97221136.rom", 0x000000, 0x040000, CRC(ce3b966f) SHA1(753b94351d94c369616c2c87b19d568dc5e2764e) )

	// RasterOps "ColorBoard 264" NuBus video card
	ROM_REGION32_BE(0x8000, "rops264", 0)
        ROM_LOAD32_BYTE( "264-1914.bin", 0x000003, 0x002000, CRC(d5fbd5ad) SHA1(98d35ed3fb0bca4a9bee1cdb2af0d3f22b379386) )
        ROM_LOAD32_BYTE( "264-1915.bin", 0x000002, 0x002000, CRC(26c19ee5) SHA1(2b2853d04cc6b0258e85eccd23ebfd4f4f63a084) )
ROM_END

ROM_START( maciicx )
	ROM_REGION32_BE(0x40000, "bootrom", 0)
        ROM_LOAD( "97221136.rom", 0x000000, 0x040000, CRC(ce3b966f) SHA1(753b94351d94c369616c2c87b19d568dc5e2764e) )

	// RasterOps "ColorBoard 264" NuBus video card
	ROM_REGION32_BE(0x8000, "rops264", 0)
        ROM_LOAD32_BYTE( "264-1914.bin", 0x000003, 0x002000, CRC(d5fbd5ad) SHA1(98d35ed3fb0bca4a9bee1cdb2af0d3f22b379386) )
        ROM_LOAD32_BYTE( "264-1915.bin", 0x000002, 0x002000, CRC(26c19ee5) SHA1(2b2853d04cc6b0258e85eccd23ebfd4f4f63a084) )
ROM_END

ROM_START( macse30 )
	ROM_REGION32_BE(0x40000, "bootrom", 0)
        ROM_LOAD( "97221136.rom", 0x000000, 0x040000, CRC(ce3b966f) SHA1(753b94351d94c369616c2c87b19d568dc5e2764e) )

	ROM_REGION32_BE(0x2000, "se30vrom", 0)
	ROM_LOAD( "se30vrom.uk6", 0x000000, 0x002000, CRC(b74c3463) SHA1(584201cc67d9452b2488f7aaaf91619ed8ce8f03) )
ROM_END

ROM_START( maciici )
	ROM_REGION32_BE(0x80000, "bootrom", 0)
        ROM_LOAD( "368cadfe.rom", 0x000000, 0x080000, CRC(46adbf74) SHA1(b54f9d2ed16b63c49ed55adbe4685ebe73eb6e80) )
ROM_END

ROM_START( maciisi )
	ROM_REGION32_BE(0x80000, "bootrom", 0)
        ROM_LOAD( "36b7fb6c.rom", 0x000000, 0x080000, CRC(f304d973) SHA1(f923de4125aae810796527ff6e25364cf1d54eec) )

	ROM_REGION(0x1100, "egret", 0)
	ROM_LOAD( "341s0851.bin", 0x000000, 0x001100, CRC(ea9ea6e4) SHA1(8b0dae3ec66cdddbf71567365d2c462688aeb571) )
ROM_END

ROM_START( macclas2 )
	ROM_REGION32_BE(0x100000, "bootrom", 0)
        ROM_LOAD( "3193670e.rom", 0x000000, 0x080000, CRC(96d2e1fd) SHA1(50df69c1b6e805e12a405dc610bc2a1471b2eac2) )

	ROM_REGION(0x1100, "egret", 0)
	ROM_LOAD( "341s0851.bin", 0x000000, 0x001100, CRC(ea9ea6e4) SHA1(8b0dae3ec66cdddbf71567365d2c462688aeb571) )
ROM_END

ROM_START( maclc2 )
	ROM_REGION32_BE(0x100000, "bootrom", 0)
        ROM_LOAD32_BYTE( "341-0476_ue2-hh.bin", 0x000000, 0x020000, CRC(0c3b0ce4) SHA1(e4e8c883d7f2e002a3f7b7aefaa3840991e57025) )
        ROM_LOAD32_BYTE( "341-0475_ud2-mh.bin", 0x000001, 0x020000, CRC(7b013595) SHA1(0b82d8fac570270db9774f6254017d28611ae756) )
        ROM_LOAD32_BYTE( "341-0474_uc2-ml.bin", 0x000002, 0x020000, CRC(2ff2f52b) SHA1(876850df61d0233c1dd3c00d48d8d6690186b164) )
        ROM_LOAD32_BYTE( "341-0473_ub2-ll.bin", 0x000003, 0x020000, CRC(8843c37c) SHA1(bb5104110507ca543d106f11c6061245fd90c1a7) )

	ROM_REGION(0x1100, "egret", 0)
	ROM_LOAD( "341s0851.bin", 0x000000, 0x001100, CRC(ea9ea6e4) SHA1(8b0dae3ec66cdddbf71567365d2c462688aeb571) )
ROM_END

ROM_START( maclc3 )
	ROM_REGION32_BE(0x100000, "bootrom", 0)
        ROM_LOAD( "ecbbc41c.rom", 0x000000, 0x100000, CRC(e578f5f3) SHA1(c77df3220c861f37a2c553b6ee9241b202dfdffc) )

	ROM_REGION(0x1100, "egret", 0)
	ROM_LOAD( "341s0851.bin", 0x000000, 0x001100, CRC(ea9ea6e4) SHA1(8b0dae3ec66cdddbf71567365d2c462688aeb571) )
ROM_END

ROM_START( pmac6100 )
	ROM_REGION64_BE(0x400000, "bootrom", 0)
        ROM_LOAD( "9feb69b3.rom", 0x000000, 0x400000, CRC(a43fadbc) SHA1(6fac1c4e920a077c077b03902fef9199d5e8f2c3) )
ROM_END

ROM_START( macprtb )
	ROM_REGION16_BE(0x40000, "bootrom", 0)
        ROM_LOAD16_WORD( "93ca3846.rom", 0x000000, 0x040000, CRC(497348f8) SHA1(79b468b33fc53f11e87e2e4b195aac981bf0c0a6) )
ROM_END

ROM_START( macpb100 )
	ROM_REGION16_BE(0x40000, "bootrom", 0)
        ROM_LOAD16_WORD( "96645f9c.rom", 0x000000, 0x040000, CRC(29ac7ee9) SHA1(7f3acf40b1f63612de2314a2e9fcfeafca0711fc) )
ROM_END

ROM_START( maccclas )
	ROM_REGION32_BE(0x100000, "bootrom", 0)
        ROM_LOAD( "ecd99dc0.rom", 0x000000, 0x100000, CRC(c84c3aa5) SHA1(fd9e852e2d77fe17287ba678709b9334d4d74f1e) )

	ROM_REGION(0x1100, "egret", 0)
	ROM_LOAD( "341s0851.bin", 0x000000, 0x001100, CRC(ea9ea6e4) SHA1(8b0dae3ec66cdddbf71567365d2c462688aeb571) )
ROM_END

/*    YEAR  NAME      PARENT    COMPAT  MACHINE   INPUT     INIT     COMPANY          FULLNAME */
COMP( 1984, mac128k,  0,	0,	mac128k,  macplus,  mac128k512k,      "Apple Computer", "Macintosh 128k",  GAME_NOT_WORKING )
COMP( 1984, mac512k,  mac128k,  0,	mac512ke, macplus,  mac128k512k,  "Apple Computer", "Macintosh 512k",  GAME_NOT_WORKING )
COMP( 1986, mac512ke, macplus,  0,	mac512ke, macplus,  mac512ke,	  "Apple Computer", "Macintosh 512ke", 0 )
COMP( 1986, macplus,  0,	0,	macplus,  macplus,  macplus,	      "Apple Computer", "Macintosh Plus",  0 )
COMP( 1987, macse,    0,	0,	macse,    macadb,   macse,	          "Apple Computer", "Macintosh SE",  0 )
COMP( 1987, macsefd,  0,	0,	macse,    macadb,   macse,	          "Apple Computer", "Macintosh SE (FDHD)",  0 )
COMP( 1987, macii,    0,	0,	macii,    macadb,   macii,	          "Apple Computer", "Macintosh II",  GAME_NOT_WORKING )
COMP( 1988, mac2fdhd, 0,	0,	macii,    macadb,   maciifdhd,	      "Apple Computer", "Macintosh II (FDHD)",  GAME_NOT_WORKING )
COMP( 1988, maciix,   mac2fdhd, 0,	maciix,   macadb,   maciix,	      "Apple Computer", "Macintosh IIx",  0 )
COMP( 1989, macprtb,  0,        0,      macprtb,  macadb,   macprtb,  "Apple Computer", "Macintosh Portable", GAME_NOT_WORKING )
COMP( 1989, macse30,  mac2fdhd, 0,	macse30,  macadb,   macse30,	  "Apple Computer", "Macintosh SE/30",  0 )
COMP( 1989, maciicx,  mac2fdhd, 0,	maciix,   macadb,   maciicx,	  "Apple Computer", "Macintosh IIcx",  0 )
COMP( 1989, maciici,  0,	0,	maciici,  maciici,  maciici,	      "Apple Computer", "Macintosh IIci", 0 )
COMP( 1990, macclasc, 0,	0,	macse,    macadb,   macclassic,	      "Apple Computer", "Macintosh Classic",  GAME_NOT_WORKING )
COMP( 1990, maclc,    0,	0,	maclc,    maciici,  maclc,	          "Apple Computer", "Macintosh LC",  GAME_NOT_WORKING )
COMP( 1990, maciisi,  0,	0,	maciisi,  maciici,  maciisi,	      "Apple Computer", "Macintosh IIsi",  GAME_NOT_WORKING )
COMP( 1991, macpb100, 0,        0,      macprtb,  macadb,   macprtb,  "Apple Computer", "Macintosh PowerBook 100", GAME_NOT_WORKING )
COMP( 1991, macclas2, 0,	0,	macclas2, macadb,   macclassic2,      "Apple Computer", "Macintosh Classic II",  GAME_NOT_WORKING )
COMP( 1991, maclc2,   0,	0,	maclc2,   maciici,  maclc2,	          "Apple Computer", "Macintosh LC II",  GAME_NOT_WORKING )
COMP( 1993, maccclas, 0,        0,      maclc2,   macadb,   maclrcclassic,        "Apple Computer", "Macintosh Color Classic", GAME_NOT_WORKING )
COMP( 1993, maclc3,   0,	0,	maclc3,   maciici,  maclc3,	          "Apple Computer", "Macintosh LC III",  GAME_NOT_WORKING )
COMP( 1994, pmac6100, 0,	0,	pwrmac,   macadb,   macpm6100,	      "Apple Computer", "Power Macintosh 6100",  GAME_NOT_WORKING | GAME_NO_SOUND )


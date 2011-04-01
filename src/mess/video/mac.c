/***************************************************************************

    video/mac.c

    Macintosh video hardware

    Emulates the video hardware for compact Macintosh series (original
    Macintosh (128k, 512k, 512ke), Macintosh Plus, Macintosh SE, Macintosh
    Classic)

***************************************************************************/

#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "sound/asc.h"
#include "includes/mac.h"
#include "machine/ram.h"

PALETTE_INIT( mac )
{
	palette_set_color_rgb(machine, 0, 0xff, 0xff, 0xff);
	palette_set_color_rgb(machine, 1, 0x00, 0x00, 0x00);
}



VIDEO_START( mac )
{
}



#define MAC_MAIN_SCREEN_BUF_OFFSET	0x5900
#define MAC_ALT_SCREEN_BUF_OFFSET	0xD900

SCREEN_UPDATE( mac )
{
	UINT32 video_base;
	const UINT16 *video_ram;
	UINT16 word;
	UINT16 *line;
	int y, x, b;
	mac_state *state = screen->machine().driver_data<mac_state>();

	video_base = ram_get_size(screen->machine().device(RAM_TAG)) - (state->m_screen_buffer ? MAC_MAIN_SCREEN_BUF_OFFSET : MAC_ALT_SCREEN_BUF_OFFSET);
	video_ram = (const UINT16 *) (ram_get_ptr(screen->machine().device(RAM_TAG)) + video_base);

	for (y = 0; y < MAC_V_VIS; y++)
	{
		line = BITMAP_ADDR16(bitmap, y, 0);

		for (x = 0; x < MAC_H_VIS; x += 16)
		{
			word = *(video_ram++);
			for (b = 0; b < 16; b++)
			{
				line[x + b] = (word >> (15 - b)) & 0x0001;
			}
		}
	}
	return 0;
}

SCREEN_UPDATE( macse30 )
{
	UINT32 video_base;
	const UINT16 *video_ram;
	UINT16 word;
	UINT16 *line;
	int y, x, b;
	mac_state *state = screen->machine().driver_data<mac_state>();

	video_base = state->m_screen_buffer ? 0x8000 : 0;
	video_ram = (const UINT16 *) &state->m_se30_vram[video_base/4];

	for (y = 0; y < MAC_V_VIS; y++)
	{
		line = BITMAP_ADDR16(bitmap, y, 0);

		for (x = 0; x < MAC_H_VIS; x += 16)
		{
//          word = *(video_ram++);
			word = video_ram[((y * MAC_H_VIS)/16) + ((x/16)^1)];
			for (b = 0; b < 16; b++)
			{
				line[x + b] = (word >> (15 - b)) & 0x0001;
			}
		}
	}
	return 0;
}

/*
    RasterOps ColorBoard 264: fixed resolution 640x480 NuBus video card, 1/4/8/16/24 bit color
    1.5? MB of VRAM (tests up to 0x1fffff), Bt473 RAMDAC, and two custom gate arrays.

        - 0xfsff6004 is color depth: 0 for 1bpp, 1 for 2bpp, 2 for 4bpp, 3 for 8bpp, 4 for 24bpp (resolution is fixed 640x480)
        - 0xfsff6014 is VBL ack: write 1 to ack
    - 0xfsff603c is VBL disable: write 1 to disable, 0 to enable
*/

VIDEO_START( mac_cb264 )
{
	mac_state *mac = machine.driver_data<mac_state>();

	mac->m_cb264_toggle = 0;
	mac->m_cb264_count = 0;
	mac->m_cb264_clutoffs = 0;
	mac->m_cb264_vbl_disable = 1;
	mac->m_cb264_mode = 0;
}

// we do this here because SCREEN_UPDATE is called constantly when stepping in the debugger,
// which makes it hard to get accurate timings (or escape the VIA2 IRQ handler)
INTERRUPT_GEN( mac_cb264_vbl )
{
	mac_state *mac = device->machine().driver_data<mac_state>();

	if (!mac->m_cb264_vbl_disable)
	{
		mac->nubus_slot_interrupt(0xe, 1);
	}
}

SCREEN_UPDATE( mac_cb264 )
{
	UINT32 *scanline, *base;
	int x, y;
	mac_state *mac = screen->machine().driver_data<mac_state>();

	switch (mac->m_cb264_mode)
	{
		case 0: // 1 bpp
			{
				UINT8 *vram8 = (UINT8 *)mac->m_cb264_vram;
				UINT8 pixels;

				for (y = 0; y < 480; y++)
				{
					scanline = BITMAP_ADDR32(bitmap, y, 0);
					for (x = 0; x < 640/8; x++)
					{
						pixels = vram8[(y * 1024) + (BYTE4_XOR_BE(x))];

						*scanline++ = mac->m_cb264_palette[pixels&0x80];
						*scanline++ = mac->m_cb264_palette[(pixels<<1)&0x80];
						*scanline++ = mac->m_cb264_palette[(pixels<<2)&0x80];
						*scanline++ = mac->m_cb264_palette[(pixels<<3)&0x80];
						*scanline++ = mac->m_cb264_palette[(pixels<<4)&0x80];
						*scanline++ = mac->m_cb264_palette[(pixels<<5)&0x80];
						*scanline++ = mac->m_cb264_palette[(pixels<<6)&0x80];
						*scanline++ = mac->m_cb264_palette[(pixels<<7)&0x80];
					}
				}
			}
			break;

		case 1: // 2 bpp (3f/7f/bf/ff)
			{
				UINT8 *vram8 = (UINT8 *)mac->m_cb264_vram;
				UINT8 pixels;

				for (y = 0; y < 480; y++)
				{
					scanline = BITMAP_ADDR32(bitmap, y, 0);
					for (x = 0; x < 640/4; x++)
					{
						pixels = vram8[(y * 1024) + (BYTE4_XOR_BE(x))];

						*scanline++ = mac->m_cb264_palette[pixels&0xc0];
						*scanline++ = mac->m_cb264_palette[(pixels<<2)&0xc0];
						*scanline++ = mac->m_cb264_palette[(pixels<<4)&0xc0];
						*scanline++ = mac->m_cb264_palette[(pixels<<6)&0xc0];
					}
				}
			}
			break;

		case 2: // 4 bpp
			{
				UINT8 *vram8 = (UINT8 *)mac->m_cb264_vram;
				UINT8 pixels;

				for (y = 0; y < 480; y++)
				{
					scanline = BITMAP_ADDR32(bitmap, y, 0);

					for (x = 0; x < 640/2; x++)
					{
						pixels = vram8[(y * 1024) + (BYTE4_XOR_BE(x))];

						*scanline++ = mac->m_cb264_palette[pixels&0xf0];
						*scanline++ = mac->m_cb264_palette[(pixels<<4)&0xf0];
					}
				}
			}
			break;

		case 3: // 8 bpp
			{
				UINT8 *vram8 = (UINT8 *)mac->m_cb264_vram;
				UINT8 pixels;

				for (y = 0; y < 480; y++)
				{
					scanline = BITMAP_ADDR32(bitmap, y, 0);

					for (x = 0; x < 640; x++)
					{
						pixels = vram8[(y * 1024) + (BYTE4_XOR_BE(x))];
						*scanline++ = mac->m_cb264_palette[pixels];
					}
				}
			}
			break;

		case 4:	// 24 bpp
		case 7: // ???
			for (y = 0; y < 480; y++)
			{
				scanline = BITMAP_ADDR32(bitmap, y, 0);
				base = &mac->m_cb264_vram[y * 1024];
				for (x = 0; x < 640; x++)
				{
					*scanline++ = *base++;
				}
			}
			break;

		default:
			fatalerror("cb264: unknown video mode %d", mac->m_cb264_mode);
			break;

	}

	return 0;
}

READ32_MEMBER( mac_state::mac_cb264_r )
{
	switch (offset)
	{
		case 0x0c/4:
		case 0x28/4:
			break;

		case 0x34/4:
			m_cb264_toggle ^= 1;
			return m_cb264_toggle;	// bit 0 is vblank?

		default:
			logerror("cb264_r: reg %x (mask %x PC %x)\n", offset*4, mem_mask, cpu_get_pc(&space.device()));
			break;
	}

	return 0;
}

WRITE32_MEMBER( mac_state::mac_cb264_w )
{
	switch (offset)
	{
		case 0x4/4:	// 0 = 1 bpp, 1 = 2bpp, 2 = 4bpp, 3 = 8bpp, 4 = 24bpp
			m_cb264_mode = data;
			break;

		case 0x14/4:	// VBL ack
			{
				mac_state *mac = space.machine().driver_data<mac_state>();
				mac->nubus_slot_interrupt(0xe, 0);
			}
			break;

		case 0x3c/4:	// VBL disable
			m_cb264_vbl_disable = data;
			break;

		default:
//          printf("cb264_w: %x to reg %x (mask %x PC %x)\n", data, offset*4, mem_mask, cpu_get_pc(&space->device()));
			break;
	}
}

WRITE32_MEMBER( mac_state::mac_cb264_ramdac_w )
{
	switch (offset)
	{
		case 0:
			m_cb264_clutoffs = data>>24;
			m_cb264_count = 0;
			break;

		case 1:
			m_cb264_colors[m_cb264_count++] = data>>24;

			if (m_cb264_count == 3)
			{
				palette_set_color(space.machine(), m_cb264_clutoffs, MAKE_RGB(m_cb264_colors[0], m_cb264_colors[1], m_cb264_colors[2]));
				m_cb264_palette[m_cb264_clutoffs] = MAKE_RGB(m_cb264_colors[0], m_cb264_colors[1], m_cb264_colors[2]);
				m_cb264_clutoffs++;
				m_cb264_count = 0;
			}
			break;

		default:
//          printf("%x to unknown RAMDAC register @ %x\n", data, offset);
			break;
	}
}

// IIci/IIsi RAM-Based Video (RBV) and children: V8, Eagle, Spice, VASP, Sonora

VIDEO_START( macrbv )
{
}

VIDEO_RESET(maceagle)
{
	mac_state *mac = machine.driver_data<mac_state>();

	mac->m_rbv_montype = 32;
	mac->m_rbv_palette[0xfe] = 0xffffff;
	mac->m_rbv_palette[0xff] = 0;
}

VIDEO_RESET(macrbv)
{
	mac_state *mac = machine.driver_data<mac_state>();
	rectangle visarea;
	int htotal, vtotal;
	double framerate;

	memset(mac->m_rbv_regs, 0, sizeof(mac->m_rbv_regs));

	mac->m_rbv_count = 0;
	mac->m_rbv_clutoffs = 0;
	mac->m_rbv_immed10wr = 0;

	mac->m_rbv_regs[2] = 0xff;

	mac->m_rbv_type = RBV_TYPE_RBV;

	visarea.min_x = 0;
	visarea.min_y = 0;

	mac->m_rbv_montype = input_port_read_safe(machine, "MONTYPE", 2);
	switch (mac->m_rbv_montype)
	{
		case 1:	// 15" portrait display
			visarea.max_x = 640-1;
			visarea.max_y = 870-1;
		    	htotal = 832;
			vtotal = 918;
			framerate = 75.0;
			break;

		case 2: // 12" RGB
			visarea.max_x = 512-1;
			visarea.max_y = 384-1;
		    	htotal = 640;
			vtotal = 407;
			framerate = 60.15;
			break;

		case 6: // 13" RGB
		default:
			visarea.max_x = 640-1;
			visarea.max_y = 480-1;
		    	htotal = 800;
			vtotal = 525;
			framerate = 59.94;
			break;
	}

//      printf("RBV reset: monitor is %dx%d @ %f Hz\n", visarea.max_x+1, visarea.max_y+1, framerate);
	machine.primary_screen->configure(htotal, vtotal, visarea, HZ_TO_ATTOSECONDS(framerate));
}

VIDEO_START( macsonora )
{
	mac_state *mac = machine.driver_data<mac_state>();

	memset(mac->m_rbv_regs, 0, sizeof(mac->m_rbv_regs));

	mac->m_rbv_count = 0;
	mac->m_rbv_clutoffs = 0;
	mac->m_rbv_immed10wr = 0;

	mac->m_rbv_regs[2] = 0xff;
	mac->m_rbv_regs[4] = 0x6;
	mac->m_rbv_regs[5] = 0x3;

	mac->m_sonora_vctl[0] = 0x9f;
	mac->m_sonora_vctl[1] = 0;
	mac->m_sonora_vctl[2] = 0;

	mac->m_rbv_type = RBV_TYPE_SONORA;
}

VIDEO_START( macv8 )
{
	mac_state *mac = machine.driver_data<mac_state>();

	memset(mac->m_rbv_regs, 0, sizeof(mac->m_rbv_regs));

	mac->m_rbv_count = 0;
	mac->m_rbv_clutoffs = 0;
	mac->m_rbv_immed10wr = 0;

	mac->m_rbv_regs[0] = 0x4f;
	mac->m_rbv_regs[1] = 0x06;
	mac->m_rbv_regs[2] = 0xff;

	mac->m_rbv_type = RBV_TYPE_V8;
}

SCREEN_UPDATE( macrbv )
{
	UINT32 *scanline;
	int x, y, hres, vres;
	mac_state *mac = screen->machine().driver_data<mac_state>();
	UINT8 *vram8 = (UINT8 *)ram_get_ptr(screen->machine().device(RAM_TAG));

	switch (mac->m_rbv_montype)
	{
		case 32: // classic II built-in display
			hres = MAC_H_VIS;
			vres = MAC_V_VIS;
			vram8 += 0x1f9a80;	// Classic II apparently doesn't use VRAM?
			break;

		case 1:	// 15" portrait display
			hres = 640;
			vres = 870;
			break;

		case 2: // 12" RGB
			hres = 512;
			vres = 384;
			break;

		case 6: // 13" RGB
		default:
			hres = 640;
			vres = 480;
			break;
	}

	switch (mac->m_rbv_regs[0x10] & 7)
	{
		case 0:	// 1bpp
		{
			UINT8 pixels;

			for (y = 0; y < vres; y++)
			{
				scanline = BITMAP_ADDR32(bitmap, y, 0);
				for (x = 0; x < hres; x+=8)
				{
					pixels = vram8[(y * (hres/8)) + ((x/8)^3)];

					*scanline++ = mac->m_rbv_palette[0xfe|(pixels>>7)];
					*scanline++ = mac->m_rbv_palette[0xfe|((pixels>>6)&1)];
					*scanline++ = mac->m_rbv_palette[0xfe|((pixels>>5)&1)];
					*scanline++ = mac->m_rbv_palette[0xfe|((pixels>>4)&1)];
					*scanline++ = mac->m_rbv_palette[0xfe|((pixels>>3)&1)];
					*scanline++ = mac->m_rbv_palette[0xfe|((pixels>>2)&1)];
					*scanline++ = mac->m_rbv_palette[0xfe|((pixels>>1)&1)];
					*scanline++ = mac->m_rbv_palette[0xfe|(pixels&1)];
				}
			}
		}
		break;

		case 1:	// 2bpp
		{
			UINT8 pixels;

			for (y = 0; y < vres; y++)
			{
				scanline = BITMAP_ADDR32(bitmap, y, 0);
				for (x = 0; x < hres/4; x++)
				{
					pixels = vram8[(y * (hres/4)) + (BYTE4_XOR_BE(x))];

					*scanline++ = mac->m_rbv_palette[0xfc|((pixels>>6)&3)];
					*scanline++ = mac->m_rbv_palette[0xfc|((pixels>>4)&3)];
					*scanline++ = mac->m_rbv_palette[0xfc|((pixels>>2)&3)];
					*scanline++ = mac->m_rbv_palette[0xfc|(pixels&3)];
				}
			}
		}
		break;

		case 2: // 4bpp
		{
			UINT8 pixels;

			for (y = 0; y < vres; y++)
			{
				scanline = BITMAP_ADDR32(bitmap, y, 0);

				for (x = 0; x < hres/2; x++)
				{
					pixels = vram8[(y * (hres/2)) + (BYTE4_XOR_BE(x))];

					*scanline++ = mac->m_rbv_palette[0xf0|(pixels>>4)];
					*scanline++ = mac->m_rbv_palette[0xf0|(pixels&0xf)];
				}
			}
		}
		break;

		case 3: // 8bpp
		{
			UINT8 pixels;

			for (y = 0; y < vres; y++)
			{
				scanline = BITMAP_ADDR32(bitmap, y, 0);

				for (x = 0; x < hres; x++)
				{
					pixels = vram8[(y * hres) + (BYTE4_XOR_BE(x))];
					*scanline++ = mac->m_rbv_palette[pixels];
				}
			}
		}
	}

	return 0;
}

SCREEN_UPDATE( macrbvvram )
{
	UINT32 *scanline;
	int x, y;
	mac_state *mac = screen->machine().driver_data<mac_state>();
	UINT8 mode = 0;

	switch (mac->m_rbv_type)
	{
		case RBV_TYPE_RBV:
		case RBV_TYPE_V8:
			mode = mac->m_rbv_regs[0x10] & 7;
			break;

		case RBV_TYPE_SONORA:
			mode = mac->m_sonora_vctl[1] & 7;

			// forced blank?
			if (mac->m_sonora_vctl[0] & 0x80)
			{
				return 0;
			}
			break;
	}

	switch (mode)
	{
		case 0:	// 1bpp
		{
			UINT8 *vram8 = (UINT8 *)mac->m_rbv_vram;
			UINT8 pixels;


			if (mac->m_rbv_type == RBV_TYPE_SONORA)
			{
				for (y = 0; y < 480; y++)
				{
					scanline = BITMAP_ADDR32(bitmap, y, 0);
					for (x = 0; x < 640; x+=8)
					{
						pixels = vram8[(y * 80) + ((x/8)^3)];

						*scanline++ = mac->m_rbv_palette[0x7f|(pixels&0x80)];
						*scanline++ = mac->m_rbv_palette[0x7f|((pixels<<1)&0x80)];
						*scanline++ = mac->m_rbv_palette[0x7f|((pixels<<2)&0x80)];
						*scanline++ = mac->m_rbv_palette[0x7f|((pixels<<3)&0x80)];
						*scanline++ = mac->m_rbv_palette[0x7f|((pixels<<4)&0x80)];
						*scanline++ = mac->m_rbv_palette[0x7f|((pixels<<5)&0x80)];
						*scanline++ = mac->m_rbv_palette[0x7f|((pixels<<6)&0x80)];
						*scanline++ = mac->m_rbv_palette[0x7f|((pixels<<7)&0x80)];
					}
				}
			}
			else
			{
				for (y = 0; y < 480; y++)
				{
					scanline = BITMAP_ADDR32(bitmap, y, 0);
					for (x = 0; x < 640; x+=8)
					{
						pixels = vram8[(y * 80) + ((x/8)^3)];

						*scanline++ = mac->m_rbv_palette[0xfe|(pixels>>7)];
						*scanline++ = mac->m_rbv_palette[0xfe|((pixels>>6)&1)];
						*scanline++ = mac->m_rbv_palette[0xfe|((pixels>>5)&1)];
						*scanline++ = mac->m_rbv_palette[0xfe|((pixels>>4)&1)];
						*scanline++ = mac->m_rbv_palette[0xfe|((pixels>>3)&1)];
						*scanline++ = mac->m_rbv_palette[0xfe|((pixels>>2)&1)];
						*scanline++ = mac->m_rbv_palette[0xfe|((pixels>>1)&1)];
						*scanline++ = mac->m_rbv_palette[0xfe|(pixels&1)];
					}
				}
			}
		}
		break;

		case 1:	// 2bpp
		{
			UINT8 *vram8 = (UINT8 *)mac->m_rbv_vram;
			UINT8 pixels;

			for (y = 0; y < 480; y++)
			{
				scanline = BITMAP_ADDR32(bitmap, y, 0);
				for (x = 0; x < 640/4; x++)
				{
					pixels = vram8[(y * 160) + (BYTE4_XOR_BE(x))];

					*scanline++ = mac->m_rbv_palette[0xfc|((pixels>>6)&3)];
					*scanline++ = mac->m_rbv_palette[0xfc|((pixels>>4)&3)];
					*scanline++ = mac->m_rbv_palette[0xfc|((pixels>>2)&3)];
					*scanline++ = mac->m_rbv_palette[0xfc|(pixels&3)];
				}
			}
		}
		break;

		case 2: // 4bpp
		{
			UINT8 *vram8 = (UINT8 *)mac->m_rbv_vram;
			UINT8 pixels;

			for (y = 0; y < 480; y++)
			{
				scanline = BITMAP_ADDR32(bitmap, y, 0);

				for (x = 0; x < 640/2; x++)
				{
					pixels = vram8[(y * 320) + (BYTE4_XOR_BE(x))];

					*scanline++ = mac->m_rbv_palette[0xf0|(pixels>>4)];
					*scanline++ = mac->m_rbv_palette[0xf0|(pixels&0xf)];
				}
			}
		}
		break;

		case 3: // 8bpp
		{
			UINT8 *vram8 = (UINT8 *)mac->m_rbv_vram;
			UINT8 pixels;

			for (y = 0; y < 480; y++)
			{
				scanline = BITMAP_ADDR32(bitmap, y, 0);

				for (x = 0; x < 640; x++)
				{
					pixels = vram8[(y * 640) + (BYTE4_XOR_BE(x))];
					*scanline++ = mac->m_rbv_palette[pixels];
				}
			}
		}
	}

	return 0;
}


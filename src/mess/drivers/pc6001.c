/*********************************************************************************

    NEC PC-6001 series
    NEC PC-6600 series

    preliminary driver by Angelo Salese

    TODO:
    - Hook up tape loading, images that are floating around the net are already
      ADC'ed, so they should be easy to implement (but not exactly faithful)
    - cassette handling requires a decap of the MCU. It could be possible to
      do some tight synch between the master CPU and a code simulation,but I think
      it's not worth the effort...
    - many games won't boot, check why;
    - make the later model to work;
    - Currently rewriting the video part without the MC6847 for two reasons:
        A) the later models have a custom video chip in the place of the MC6847,
           so this implementation will be used in the end.
        B) It's easier to me to see what the attribute vram does since I don't
           have any docs atm.

==================================================================================

    PC-6001 (1981-09):

     * CPU: Z80A @ 4 MHz
     * ROM: 16KB + 4KB (chargen) - no kanji
     * RAM: 16KB, it can be expanded to 32KB
     * Text Mode: 32x16 and 2 colors
     * Graphic Modes: 64x48 (9 colors), 128x192 (4 colors), 256x192 (2 colors)
     * Sound: BEEP + PSG - Optional Voice Synth Cart
     * Keyboard: JIS Keyboard with 5 function keys, control key, TAB key,
            HOME/CLR key, INS key, DEL key, GRAPH key, Japanese syllabary
            key, page key, STOP key, and cursor key (4 directions)
     * 1 cartslot, optional floppy drive, optional serial 232 port, 2
            joystick ports


    PC-6001 mkII (1983-07):

     * CPU: Z80A @ 4 MHz
     * ROM: 32KB + 16KB (chargen) + 32KB (kanji) + 16KB (Voice Synth)
     * RAM: 64KB
     * Text Mode: same as PC-6001 with N60-BASIC; 40x20 and 15 colors with
            N60M-BASIC
     * Graphic Modes: same as PC-6001 with N60-BASIC; 80x40 (15 colors),
            160x200 (15 colors), 320x200 (4 colors) with N60M-BASIC
     * Sound: BEEP + PSG
     * Keyboard: JIS Keyboard with 5 function keys, control key, TAB key,
            HOME/CLR key, INS key, DEL key, CAPS key, GRAPH key, Japanese
            syllabary key, page key, mode key, STOP key, and cursor key (4
            directions)
     * 1 cartslot, floppy drive, optional serial 232 port, 2 joystick ports


    PC-6001 mkIISR (1984-12):

     * CPU: Z80A @ 3.58 MHz
     * ROM: 64KB + 16KB (chargen) + 32KB (kanji) + 32KB (Voice Synth)
     * RAM: 64KB
     * Text Mode: same as PC-6001/PC-6001mkII with N60-BASIC; 40x20, 40x25,
            80x20, 80x25 and 15 colors with N66SR-BASIC
     * Graphic Modes: same as PC-6001/PC-6001mkII with N60-BASIC; 80x40 (15 colors),
            320x200 (15 colors), 640x200 (15 colors) with N66SR-BASIC
     * Sound: BEEP + PSG + FM
     * Keyboard: JIS Keyboard with 5 function keys, control key, TAB key,
            HOME/CLR key, INS key, DEL key, CAPS key, GRAPH key, Japanese
            syllabary key, page key, mode key, STOP key, and cursor key (4
            directions)
     * 1 cartslot, floppy drive, optional serial 232 port, 2 joystick ports


    info from http://www.geocities.jp/retro_zzz/machines/nec/6001/spc60.html

==================================================================================

irq vector 00: writes 0x00 to [$fa19]
irq vector 02: (A = 0, B = 0) tests ppi port c, does something with ay ports (plus more?) ;keyboard data ready, no kanji lock, no caps lock
irq vector 04: uart irq
irq vector 06: operates with $fa28, $fa2e, $fd1b ;hblank? joypad irq?
irq vector 08: tests ppi port c, puts port A to $fa1d,puts 0x02 to [$fa19] ;tape data ready
irq vector 0A: writes 0x00 to [$fa19]
irq vector 0C: writes 0x00 to [$fa19]
irq vector 0E: same as 2, (A = 0x03, B = 0x00) ;? any press has the same effect as enter pressed twice, break input?
irq vector 10: same as 2, (A = 0x03, B = 0x00)
irq vector 12: writes 0x10 to [$fa19] ;end of tape reached
irq vector 14: same as 2, (A = 0x00, B = 0x01) ;kanji lock enabled
irq vector 16: tests ppi port c, writes the result to $feca. ;vblank


*********************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"
#include "machine/i8255a.h"
#include "machine/msm8251.h"
#include "video/m6847.h"
#include "sound/ay8910.h"
#include "sound/wave.h"

#include "devices/cassette.h"
#include "devices/cartslot.h"
#include "formats/p6001_cas.h"

static UINT8 *pc6001_ram;
static UINT8 *pc6001_video_ram;
static UINT8 irq_vector = 0x00;
static UINT8 cas_switch,sys_latch;
static UINT32 cas_offset;
static UINT32 cas_maxsize;

//#define CAS_LENGTH 0x1655

static VIDEO_START( pc6001 )
{
	#if 0
	m6847_config cfg;

	memset(&cfg, 0, sizeof(cfg));
	cfg.type = M6847_VERSION_M6847T1_NTSC;
	cfg.get_attributes = pc6001_get_attributes;
	cfg.get_video_ram = pc6001_get_video_ram;
	cfg.get_char_rom = pc6001_get_char_rom;
	m6847_init(machine, &cfg);
	#endif
	pc6001_video_ram = auto_alloc_array(machine, UINT8, 0x2000); //0x400? We'll see...
}

static void draw_bitmap_1bpp(running_machine *machine, bitmap_t *bitmap,const rectangle *cliprect,int attr)
{
	int x,y,xi;
	int fgcol,color;

	fgcol = (attr & 2) ? 7 : 2;

	for(y=0;y<192;y++)
	{
		for(x=0;x<32;x++)
		{
			int tile = pc6001_video_ram[(x+(y*32))+0x200];

			for(xi=0;xi<8;xi++)
			{
				color = ((tile)>>(7-xi) & 1) ? fgcol : 0;

				*BITMAP_ADDR16(bitmap, (y+24), (x*8+xi)+32) = machine->pens[color];
			}
		}
	}
}

static void draw_bitmap_2bpp(running_machine *machine, bitmap_t *bitmap,const rectangle *cliprect, int attr)
{
	int color,x,y,xi,yi;

	int shrink_x = 2*4;
	int shrink_y = (attr & 8) ? 1 : 2;
	int w = (shrink_x == 8) ? 32 : 16;
	int col_bank = ((attr & 2)<<1);

	for(y=0;y<(192/shrink_y);y++)
	{
		for(x=0;x<w;x++)
		{
			int tile = pc6001_video_ram[(x+(y*32))+0x200];

			for(yi=0;yi<shrink_y;yi++)
			{
				for(xi=0;xi<shrink_x;xi++)
				{
					int i;
					i = (shrink_x == 8) ? (xi & 0x06) : (xi & 0x0c)>>1;
					color = ((tile >> i) & 3)+8;
					color+= col_bank;

					*BITMAP_ADDR16(bitmap, ((y*shrink_y+yi)+24), (x*shrink_x+((shrink_x-1)-xi))+32) = machine->pens[color];
				}
			}
		}
	}
}

static void draw_tile_3bpp(running_machine *machine, bitmap_t *bitmap,const rectangle *cliprect,int x,int y,int tile,int attr)
{
	int color,pen,xi,yi;

	if(attr & 0x10) //2x2 squares on a single cell
		pen = (tile & 0x70)>>4;
	else //2x3
		pen = (tile & 0xc0) >> 6 | (attr & 2)<<1;

	for(yi=0;yi<12;yi++)
	{
		for(xi=0;xi<8;xi++)
		{
			int i;
			i = (xi & 4)>>2; //x-axis
			if(attr & 0x10) //2x2
			{
				i+= (yi >= 6) ? 2 : 0; //y-axis
			}
			else //2x3
			{
				i+= (yi & 4)>>1; //y-axis 1
				i+= (yi & 8)>>1; //y-axis 2
			}

			color = ((tile >> i) & 1) ? pen+8 : 0;

			*BITMAP_ADDR16(bitmap, ((y*12+(11-yi))+24), (x*8+(7-xi))+32) = machine->pens[color];
		}
	}
}

static void draw_tile_text(running_machine *machine, bitmap_t *bitmap,const rectangle *cliprect,int x,int y,int tile,int attr,int has_mc6847)
{
	int xi,yi,pen,fgcol,color;
	UINT8 *gfx_data = memory_region(machine, "gfx1");

	for(yi=0;yi<12;yi++)
	{
		for(xi=0;xi<8;xi++)
		{
			pen = gfx_data[(tile*0x10)+yi]>>(7-xi) & 1;

			if(has_mc6847)
			{
				fgcol = (attr & 2) ? 0x12 : 0x10;

				if(attr & 1)
					color = pen ? (fgcol+0) : (fgcol+1);
				else
					color = pen ? (fgcol+1) : (fgcol+0);

			}
			else
			{
				fgcol = (attr & 2) ? 2 : 7;

				if(attr & 1)
					color = pen ? 0 : fgcol;
				else
					color = pen ? fgcol : 0;
			}

			*BITMAP_ADDR16(bitmap, ((y*12+yi)+24), (x*8+xi)+32) = machine->pens[color];
		}
	}
}

static void draw_border(running_machine *machine, bitmap_t *bitmap,const rectangle *cliprect,int attr)
{
	int x,y,color;

	for(y=0;y<240;y++)
	{
		for(x=0;x<320;x++)
		{
			if(attr & 0x80 && (!(attr & 0x10)))
				color = ((attr & 2)<<1) + 8;
			else
				color = 0; //FIXME: other modes not yet checked

			*BITMAP_ADDR16(bitmap, y, x) = machine->pens[color];
		}
	}
}

static void pc6001_screen_draw(running_machine *machine, bitmap_t *bitmap,const rectangle *cliprect, int has_mc6847)
{
	int x,y;
	int tile,attr;

	attr = pc6001_video_ram[0];

	draw_border(machine,bitmap,cliprect,attr);

	if(attr & 0x80) // gfx mode
	{
		if(attr & 0x10) // 256x192x1 mode (FIXME: might be a different trigger)
		{
			draw_bitmap_1bpp(machine,bitmap,cliprect,attr);
		}
		else // 128x192x2 mode
		{
			draw_bitmap_2bpp(machine,bitmap,cliprect,attr);
		}
	}
	else // text mode
	{
		for(y=0;y<16;y++)
		{
			for(x=0;x<32;x++)
			{
				tile = pc6001_video_ram[(x+(y*32))+0x200];
				attr = pc6001_video_ram[(x+(y*32)) & 0x1ff];

				if(attr & 0x40)
				{
					draw_tile_3bpp(machine,bitmap,cliprect,x,y,tile,attr);
				}
				else
				{
					draw_tile_text(machine,bitmap,cliprect,x,y,tile,attr,has_mc6847);
				}
			}
		}
	}
}

static VIDEO_UPDATE( pc6001 )
{
	pc6001_screen_draw(screen->machine,bitmap,cliprect,1);

	return 0;
}

static VIDEO_UPDATE( pc6001m2 )
{
	pc6001_screen_draw(screen->machine,bitmap,cliprect,0);

	return 0;
}

static WRITE8_HANDLER ( pc6001_system_latch_w )
{
	UINT16 startaddr[] = {0xC000, 0xE000, 0x8000, 0xA000 };

	pc6001_video_ram =  pc6001_ram + startaddr[(data >> 1) & 0x03] - 0x8000;

	if((!(sys_latch & 8)) && data & 0x8) //PLAY tape cmd
	{
		cas_switch = 1;
		//cassette_change_state(space->machine->device("cass" ),CASSETTE_MOTOR_ENABLED,CASSETTE_MASK_MOTOR);
		//cassette_change_state(space->machine->device("cass" ),CASSETTE_PLAY,CASSETTE_MASK_UISTATE);
	}
	if((sys_latch & 8) && ((data & 0x8) == 0)) //STOP tape cmd
	{
		cas_switch = 0;
		//cassette_change_state(space->machine->device("cass" ),CASSETTE_MOTOR_DISABLED,CASSETTE_MASK_MOTOR);
		//cassette_change_state(space->machine->device("cass" ),CASSETTE_STOPPED,CASSETTE_MASK_UISTATE);
		//irq_vector = 0x00;
		//cputag_set_input_line(space->machine,"maincpu", 0, ASSERT_LINE);
	}

	sys_latch = data;
	printf("%02x\n",data);
}

#if 0
static ATTR_CONST UINT8 pc6001_get_attributes(running_machine *machine, UINT8 c,int scanline, int pos)
{
	UINT8 result = 0x00;
	UINT8 val = pc6001_video_ram [(scanline / 12) * 0x20 + pos];

	if (val & 0x01) {
		result |= M6847_INV;
	}
	if (val & 0x40)
		result |= M6847_AG | M6847_GM1; //TODO

	result |= M6847_INTEXT; // always use external ROM
	return result;
}

static const UINT8 *pc6001_get_video_ram(running_machine *machine, int scanline)
{
	return pc6001_video_ram +0x0200+ (scanline / 12) * 0x20;
}

static UINT8 pc6001_get_char_rom(running_machine *machine, UINT8 ch, int line)
{
	UINT8 *gfx = memory_region(machine, "gfx1");
	return gfx[ch*16+line];
}
#endif

static UINT8 port_c_8255,cur_keycode;

static READ8_DEVICE_HANDLER(nec_ppi8255_r) {
	if (offset==2) {
		return port_c_8255;
	}
	else if(offset==0)
	{
		UINT8 res;
		res = cur_keycode;
		cur_keycode = 0;
		return res;
	}
	else {
		return i8255a_r(device,offset);
	}
}

static WRITE8_DEVICE_HANDLER(nec_ppi8255_w) {
	if (offset==3) {
		if(data & 1) {
			port_c_8255 |=   1<<((data>>1)&0x07);
		} else {
			port_c_8255 &= ~(1<<((data>>1)&0x07));
		}
		switch(data) {
        	case 0x08: port_c_8255 |= 0x88; break;
        	case 0x09: port_c_8255 &= 0xf7; break;
        	case 0x0c: port_c_8255 |= 0x28; break;
        	case 0x0d: port_c_8255 &= 0xf7; break;
        	default: break;
		}
		port_c_8255 |= 0xa8;

		{
			UINT8 *gfx_data = memory_region(device->machine, "gfx1");
			UINT8 *ext_rom = memory_region(device->machine, "cart_img");

			//printf("%02x\n",data);

			if((data & 0x0f) == 0x05)
				memory_set_bankptr(device->machine, "bank1", &ext_rom[0x2000]);
			if((data & 0x0f) == 0x04)
				memory_set_bankptr(device->machine, "bank1", &gfx_data[0]);
		}
	}
	i8255a_w(device,offset,data);
}

static ADDRESS_MAP_START(pc6001_map, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x3fff) AM_ROM AM_WRITENOP
	AM_RANGE(0x4000, 0x5fff) AM_ROM AM_REGION("cart_img",0)
	AM_RANGE(0x6000, 0x7fff) AM_ROMBANK("bank1")
	AM_RANGE(0x8000, 0xffff) AM_RAM AM_BASE(&pc6001_ram)
ADDRESS_MAP_END

static ADDRESS_MAP_START( pc6001_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x80, 0x80) AM_DEVREADWRITE("uart", msm8251_data_r,msm8251_data_w)
	AM_RANGE(0x81, 0x81) AM_DEVREADWRITE("uart", msm8251_status_r,msm8251_control_w)
	AM_RANGE(0x90, 0x93) AM_DEVREADWRITE("ppi8255", nec_ppi8255_r, nec_ppi8255_w)
	AM_RANGE(0xa0, 0xa0) AM_DEVWRITE("ay8910", ay8910_address_w)
	AM_RANGE(0xa1, 0xa1) AM_DEVWRITE("ay8910", ay8910_data_w)
	AM_RANGE(0xa2, 0xa2) AM_DEVREAD("ay8910", ay8910_r)
	AM_RANGE(0xb0, 0xb0) AM_WRITE(pc6001_system_latch_w)
ADDRESS_MAP_END

/*
    ROM_REGION( 0x28000, "maincpu", ROMREGION_ERASEFF )
    ROM_LOAD( "basicrom.62", 0x10000, 0x8000, CRC(950ac401) SHA1(fbf195ba74a3b0f80b5a756befc96c61c2094182) )
    ROM_LOAD( "voicerom.62", 0x18000, 0x4000, CRC(49b4f917) SHA1(1a2d18f52ef19dc93da3d65f19d3abbd585628af) )
    ROM_LOAD( "cgrom60.62",  0x1c000, 0x2000, CRC(81eb5d95) SHA1(53d8ae9599306ff23bf95208d2f6cc8fed3fc39f) )
    ROM_LOAD( "cgrom60m.62", 0x1e000, 0x2000, CRC(3ce48c33) SHA1(f3b6c63e83a17d80dde63c6e4d86adbc26f84f79) )
    ROM_LOAD( "kanjirom.62", 0x20000, 0x8000, CRC(20c8f3eb) SHA1(4c9f30f0a2ebbe70aa8e697f94eac74d8241cadd) )
*/

#define BASICROM(_v_) \
	0x10000+0x2000*_v_ \

#define VOICEROM(_v_) \
	0x18000+0x2000*_v_ \

#define EXROM(_v_) \
	0x1c000+0x2000*_v_ \

#define TVROM(_v_) \
	0x20000+0x2000*_v_ \

#define WRAM(_v_) \
	0x28000+0x2000*_v_ \

#define EXWRAM(_v_) \
	0x38000+0x2000*_v_ \

static const UINT32 banksw_table_r0[0x10][4] = {
	{ -1,			-1,				-1,				-1 },			//0x00: <invalid setting>
	{ BASICROM(0),	BASICROM(1),	BASICROM(2),	BASICROM(3) },	//0x01: basic rom 0 & 1 / basic rom 2 & 3
	{ TVROM(0),		TVROM(1),		VOICEROM(0),	VOICEROM(1) },	//0x02: tv rom 0 & 1 / voice rom 0 & 1
	{ EXROM(1),		EXROM(1),		EXROM(1),		EXROM(1)	},	//0x03: ex rom 1 & 1 / ex rom 1 & 1
	{ EXROM(0),		EXROM(0),		EXROM(0),		EXROM(0)	},	//0x04: ex rom 0 & 0 / ex rom 0 & 0
	{ TVROM(1),		BASICROM(1),	VOICEROM(0),	BASICROM(3) },	//0x05: tv rom 1 & basic rom 1 / voice rom 0 & basic 3
	{ BASICROM(0),	TVROM(2),		BASICROM(2),	VOICEROM(1) },	//0x06: basic rom 0 & tv rom 2 / basic rom 2 & voice 1
	{ EXROM(0),		EXROM(1),		EXROM(0),		EXROM(1)	},	//0x07: ex rom 0 & ex rom 1 / ex rom 0 & ex rom 1
	{ EXROM(1),		EXROM(0),		EXROM(1),		EXROM(0)	},	//0x08: ex rom 1 & ex rom 0 / ex rom 1 & ex rom 0
	{ EXROM(1),		BASICROM(1),	EXROM(1),		BASICROM(3) },	//0x09: ex rom 1 & basic rom 1 / ex rom 1 & basic 3
	{ BASICROM(0),  EXROM(1),		BASICROM(2),	EXROM(1)	},	//0x0a: basic rom 0 & ex rom 1 / basic rom 2 & ex rom 1
	{ EXROM(0),		TVROM(2),		EXROM(0),		VOICEROM(1) },	//0x0b: ex rom 0 & tv rom 2 / ex rom 0 & voice 1
	{ TVROM(1),		EXROM(0),		VOICEROM(0),	EXROM(0)	},	//0x0c: tv rom 1 & ex rom 0 / voice rom 0 & ex rom 0
	{ WRAM(0),		WRAM(1),		WRAM(2),		WRAM(3)		},	//0x0d: ram 0 & 1 / ram 2 & 3
	{ EXWRAM(0),	EXWRAM(1),		EXWRAM(2),		EXWRAM(3)	},	//0x0e: exram 0 & 1 / exram 2 & 3
	{ -1,			-1,				-1,				-1 },			//0x0f: <invalid setting>
};

static const UINT32 banksw_table_r1[0x10][4] = {
	{ -1,			-1,				-1,				-1 },			//0x00: <invalid setting>
	{ BASICROM(0),	BASICROM(1),	BASICROM(2),	BASICROM(3) },	//0x01: basic rom 0 & 1 / basic rom 2 & 3
	{ TVROM(0),		TVROM(1),		VOICEROM(0),	VOICEROM(1) },	//0x02: tv rom 0 & 1 / voice rom 0 & 1
	{ EXROM(1),		EXROM(1),		EXROM(1),		EXROM(1)	},	//0x03: ex rom 1 & 1 / ex rom 1 & 1
	{ EXROM(0),		EXROM(0),		EXROM(0),		EXROM(0)	},	//0x04: ex rom 0 & 0 / ex rom 0 & 0
	{ TVROM(1),		BASICROM(1),	VOICEROM(0),	BASICROM(3) },	//0x05: tv rom 1 & basic rom 1 / voice rom 0 & basic 3
	{ BASICROM(0),	TVROM(2),		BASICROM(2),	VOICEROM(1) },	//0x06: basic rom 0 & tv rom 2 / basic rom 2 & voice 1
	{ EXROM(0),		EXROM(1),		EXROM(0),		EXROM(1)	},	//0x07: ex rom 0 & ex rom 1 / ex rom 0 & ex rom 1
	{ EXROM(1),		EXROM(0),		EXROM(1),		EXROM(0)	},	//0x08: ex rom 1 & ex rom 0 / ex rom 1 & ex rom 0
	{ EXROM(1),		BASICROM(1),	EXROM(1),		BASICROM(3) },	//0x09: ex rom 1 & basic rom 1 / ex rom 1 & basic 3
	{ BASICROM(0),  EXROM(1),		BASICROM(2),	EXROM(1)	},	//0x0a: basic rom 0 & ex rom 1 / basic rom 2 & ex rom 1
	{ EXROM(0),		TVROM(2),		EXROM(0),		VOICEROM(1) },	//0x0b: ex rom 0 & tv rom 2 / ex rom 0 & voice 1
	{ TVROM(1),		EXROM(0),		VOICEROM(0),	EXROM(0)	},	//0x0c: tv rom 1 & ex rom 0 / voice rom 0 & ex rom 0
	{ WRAM(4),		WRAM(5),		WRAM(6),		WRAM(7)		},	//0x0d: ram 4 & 5 / ram 6 & 7
	{ EXWRAM(4),	EXWRAM(5),		EXWRAM(6),		EXWRAM(7)	},	//0x0e: exram 4 & 5 / exram 6 & 7
	{ -1,			-1,				-1,				-1 },			//0x0f: <invalid setting>
};

static WRITE8_HANDLER( pc6001m2_bank_r0_w )
{
	UINT8 *ROM = memory_region(space->machine, "maincpu");

//  bankaddress = 0x10000 + (0x4000 * ((data & 0x40)>>6));
//  memory_set_bankptr(space->machine, 1, &ROM[bankaddress]);

	printf("%02x BANK\n",data);
	memory_set_bankptr(space->machine, "bank1", &ROM[banksw_table_r0[data & 0xf][0]]);
	memory_set_bankptr(space->machine, "bank2", &ROM[banksw_table_r0[data & 0xf][1]]);
	memory_set_bankptr(space->machine, "bank3", &ROM[banksw_table_r0[(data & 0xf0)>>4][2]]);
	memory_set_bankptr(space->machine, "bank4", &ROM[banksw_table_r0[(data & 0xf0)>>4][3]]);
}

static WRITE8_HANDLER( pc6001m2_bank_r1_w )
{
	UINT8 *ROM = memory_region(space->machine, "maincpu");

//  bankaddress = 0x10000 + (0x4000 * ((data & 0x40)>>6));
//  memory_set_bankptr(space->machine, 1, &ROM[bankaddress]);

	printf("%02x BANK\n",data);
	memory_set_bankptr(space->machine, "bank5", &ROM[banksw_table_r1[data & 0xf][0]]);
	memory_set_bankptr(space->machine, "bank6", &ROM[banksw_table_r1[data & 0xf][1]]);
	memory_set_bankptr(space->machine, "bank7", &ROM[banksw_table_r1[(data & 0xf0)>>4][2]]);
	memory_set_bankptr(space->machine, "bank8", &ROM[banksw_table_r1[(data & 0xf0)>>4][3]]);
}

static WRITE8_HANDLER( work_ram0_w ) { UINT8 *ROM = memory_region(space->machine, "maincpu"); ROM[offset+WRAM(0)] = data; }
static WRITE8_HANDLER( work_ram1_w ) { UINT8 *ROM = memory_region(space->machine, "maincpu"); ROM[offset+WRAM(1)] = data; }
static WRITE8_HANDLER( work_ram2_w ) { UINT8 *ROM = memory_region(space->machine, "maincpu"); ROM[offset+WRAM(2)] = data; }
static WRITE8_HANDLER( work_ram3_w ) { UINT8 *ROM = memory_region(space->machine, "maincpu"); ROM[offset+WRAM(3)] = data; }
static WRITE8_HANDLER( work_ram4_w ) { UINT8 *ROM = memory_region(space->machine, "maincpu"); ROM[offset+WRAM(4)] = data; }
static WRITE8_HANDLER( work_ram5_w ) { UINT8 *ROM = memory_region(space->machine, "maincpu"); ROM[offset+WRAM(5)] = data; }
static WRITE8_HANDLER( work_ram6_w ) { UINT8 *ROM = memory_region(space->machine, "maincpu"); ROM[offset+WRAM(6)] = data; }
static WRITE8_HANDLER( work_ram7_w ) { UINT8 *ROM = memory_region(space->machine, "maincpu"); ROM[offset+WRAM(7)] = data; }


static ADDRESS_MAP_START(pc6001m2_map, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x1fff) AM_ROMBANK("bank1") AM_WRITE(work_ram0_w)
	AM_RANGE(0x2000, 0x3fff) AM_ROMBANK("bank2") AM_WRITE(work_ram1_w)
	AM_RANGE(0x4000, 0x5fff) AM_ROMBANK("bank3") AM_WRITE(work_ram2_w)
	AM_RANGE(0x6000, 0x7fff) AM_ROMBANK("bank4") AM_WRITE(work_ram3_w)
	AM_RANGE(0x8000, 0x9fff) AM_ROMBANK("bank5") AM_WRITE(work_ram4_w)
	AM_RANGE(0xa000, 0xbfff) AM_ROMBANK("bank6") AM_WRITE(work_ram5_w)
	AM_RANGE(0xc000, 0xdfff) AM_ROMBANK("bank7") AM_WRITE(work_ram6_w)
	AM_RANGE(0xe000, 0xffff) AM_ROMBANK("bank8") AM_WRITE(work_ram7_w)
	AM_RANGE(0x8000, 0xffff) AM_RAM AM_BASE(&pc6001_ram)
ADDRESS_MAP_END

static ADDRESS_MAP_START( pc6001m2_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x80, 0x80) AM_DEVREADWRITE("uart", msm8251_data_r,msm8251_data_w)
	AM_RANGE(0x81, 0x81) AM_DEVREADWRITE("uart", msm8251_status_r,msm8251_control_w)
	AM_RANGE(0x90, 0x93) AM_DEVREADWRITE("ppi8255", nec_ppi8255_r, nec_ppi8255_w)
	AM_RANGE(0xa0, 0xa0) AM_DEVWRITE("ay8910", ay8910_address_w)
	AM_RANGE(0xa1, 0xa1) AM_DEVWRITE("ay8910", ay8910_data_w)
	AM_RANGE(0xa2, 0xa2) AM_DEVREAD("ay8910", ay8910_r)
	AM_RANGE(0xb0, 0xb0) AM_WRITE(pc6001_system_latch_w)
	AM_RANGE(0xf0, 0xf0) AM_WRITE(pc6001m2_bank_r0_w)
	AM_RANGE(0xf1, 0xf1) AM_WRITE(pc6001m2_bank_r1_w)
	//0xf2 w bank
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( pc6001 )
	/* TODO: these two are unchecked */
	PORT_START("P1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START("P2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START("key1") //0x00-0x1f
	PORT_BIT(0x00000001,IP_ACTIVE_HIGH,IPT_UNUSED) //0x00 null
	PORT_BIT(0x00000002,IP_ACTIVE_HIGH,IPT_UNUSED) //0x01 soh
	PORT_BIT(0x00000004,IP_ACTIVE_HIGH,IPT_UNUSED) //0x02 stx
	PORT_BIT(0x00000008,IP_ACTIVE_HIGH,IPT_UNUSED) //0x03 etx
	PORT_BIT(0x00000010,IP_ACTIVE_HIGH,IPT_UNUSED) //0x04 etx
	PORT_BIT(0x00000020,IP_ACTIVE_HIGH,IPT_UNUSED) //0x05 eot
	PORT_BIT(0x00000040,IP_ACTIVE_HIGH,IPT_UNUSED) //0x06 enq
	PORT_BIT(0x00000080,IP_ACTIVE_HIGH,IPT_UNUSED) //0x07 ack
	PORT_BIT(0x00000100,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Backspace") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8)
	PORT_BIT(0x00000200,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Tab") PORT_CODE(KEYCODE_TAB) PORT_CHAR(9)
	PORT_BIT(0x00000400,IP_ACTIVE_HIGH,IPT_UNUSED) //0x0a
	PORT_BIT(0x00000800,IP_ACTIVE_HIGH,IPT_UNUSED) //0x0b lf
	PORT_BIT(0x00001000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x0c vt
	PORT_BIT(0x00002000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("RETURN") PORT_CODE(KEYCODE_ENTER) PORT_CHAR(27)
	PORT_BIT(0x00004000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x0e cr
	PORT_BIT(0x00008000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x0f so

	PORT_BIT(0x00010000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x10 si
	PORT_BIT(0x00020000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x11 dle
	PORT_BIT(0x00040000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x12 dc1
	PORT_BIT(0x00080000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x13 dc2
	PORT_BIT(0x00100000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x14 dc3
	PORT_BIT(0x00200000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x15 dc4
	PORT_BIT(0x00400000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x16 nak
	PORT_BIT(0x00800000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x17 syn
	PORT_BIT(0x01000000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x18 etb
	PORT_BIT(0x02000000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x19 cancel
	PORT_BIT(0x04000000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x1a em
	PORT_BIT(0x08000000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x1b sub
	PORT_BIT(0x10000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("ESC") PORT_CODE(KEYCODE_TILDE) PORT_CHAR(27)
	PORT_BIT(0x20000000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x1d fs
	PORT_BIT(0x40000000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x1e gs
	PORT_BIT(0x80000000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x1f us

	PORT_START("key2") //0x20-0x3f
	PORT_BIT(0x00000001,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Space") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')
	PORT_BIT(0x00000002,IP_ACTIVE_HIGH,IPT_UNUSED) //0x21 !
	PORT_BIT(0x00000004,IP_ACTIVE_HIGH,IPT_UNUSED) //0x22 "
	PORT_BIT(0x00000008,IP_ACTIVE_HIGH,IPT_UNUSED) //0x23 #
	PORT_BIT(0x00000010,IP_ACTIVE_HIGH,IPT_UNUSED) //0x24 $
	PORT_BIT(0x00000020,IP_ACTIVE_HIGH,IPT_UNUSED) //0x25 %
	PORT_BIT(0x00000040,IP_ACTIVE_HIGH,IPT_UNUSED) //0x26 &
	PORT_BIT(0x00000080,IP_ACTIVE_HIGH,IPT_UNUSED) //0x27 '
	PORT_BIT(0x00000100,IP_ACTIVE_HIGH,IPT_UNUSED) //0x28 (
	PORT_BIT(0x00000200,IP_ACTIVE_HIGH,IPT_UNUSED) //0x29 )
	PORT_BIT(0x00000400,IP_ACTIVE_HIGH,IPT_UNUSED) //0x2a *
	PORT_BIT(0x00000800,IP_ACTIVE_HIGH,IPT_UNUSED) //0x2b +
	PORT_BIT(0x00001000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x2c ,
	PORT_BIT(0x00002000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("-") PORT_CODE(KEYCODE_MINUS) PORT_CHAR('-')
	PORT_BIT(0x00004000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x2e .
	PORT_BIT(0x00008000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x2f /

	PORT_BIT(0x00010000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("0") PORT_CODE(KEYCODE_0) PORT_CHAR('0')
	PORT_BIT(0x00020000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("1") PORT_CODE(KEYCODE_1) PORT_CHAR('1')
	PORT_BIT(0x00040000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("2") PORT_CODE(KEYCODE_2) PORT_CHAR('2')
	PORT_BIT(0x00080000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("3") PORT_CODE(KEYCODE_3) PORT_CHAR('3')
	PORT_BIT(0x00100000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("4") PORT_CODE(KEYCODE_4) PORT_CHAR('4')
	PORT_BIT(0x00200000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("5") PORT_CODE(KEYCODE_5) PORT_CHAR('5')
	PORT_BIT(0x00400000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("6") PORT_CODE(KEYCODE_6) PORT_CHAR('6')
	PORT_BIT(0x00800000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("7") PORT_CODE(KEYCODE_7) PORT_CHAR('7')
	PORT_BIT(0x01000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("8") PORT_CODE(KEYCODE_8) PORT_CHAR('8')
	PORT_BIT(0x02000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("9") PORT_CODE(KEYCODE_9) PORT_CHAR('9')
	PORT_BIT(0x04000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME(":") PORT_CODE(KEYCODE_QUOTE) PORT_CHAR(':')
	PORT_BIT(0x08000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME(";") PORT_CODE(KEYCODE_COLON) PORT_CHAR(';')
	PORT_BIT(0x10000000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x3c <
	PORT_BIT(0x20000000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x3d =
	PORT_BIT(0x40000000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x3e >
	PORT_BIT(0x80000000,IP_ACTIVE_HIGH,IPT_UNUSED) //0x3f ?

	PORT_START("key3") //0x40-0x5f
	PORT_BIT(0x00000001,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("@") PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR('@')
	PORT_BIT(0x00000002,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("A") PORT_CODE(KEYCODE_A) PORT_CHAR('A')
	PORT_BIT(0x00000004,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("B") PORT_CODE(KEYCODE_B) PORT_CHAR('B')
	PORT_BIT(0x00000008,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("C") PORT_CODE(KEYCODE_C) PORT_CHAR('C')
	PORT_BIT(0x00000010,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("D") PORT_CODE(KEYCODE_D) PORT_CHAR('D')
	PORT_BIT(0x00000020,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("E") PORT_CODE(KEYCODE_E) PORT_CHAR('E')
	PORT_BIT(0x00000040,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("F") PORT_CODE(KEYCODE_F) PORT_CHAR('F')
	PORT_BIT(0x00000080,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("G") PORT_CODE(KEYCODE_G) PORT_CHAR('G')
	PORT_BIT(0x00000100,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("H") PORT_CODE(KEYCODE_H) PORT_CHAR('H')
	PORT_BIT(0x00000200,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("I") PORT_CODE(KEYCODE_I) PORT_CHAR('I')
	PORT_BIT(0x00000400,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("J") PORT_CODE(KEYCODE_J) PORT_CHAR('J')
	PORT_BIT(0x00000800,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("K") PORT_CODE(KEYCODE_K) PORT_CHAR('K')
	PORT_BIT(0x00001000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("L") PORT_CODE(KEYCODE_L) PORT_CHAR('L')
	PORT_BIT(0x00002000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("M") PORT_CODE(KEYCODE_M) PORT_CHAR('M')
	PORT_BIT(0x00004000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("N") PORT_CODE(KEYCODE_N) PORT_CHAR('N')
	PORT_BIT(0x00008000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("O") PORT_CODE(KEYCODE_O) PORT_CHAR('O')
	PORT_BIT(0x00010000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("P") PORT_CODE(KEYCODE_P) PORT_CHAR('P')
	PORT_BIT(0x00020000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Q") PORT_CODE(KEYCODE_Q) PORT_CHAR('Q')
	PORT_BIT(0x00040000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("R") PORT_CODE(KEYCODE_R) PORT_CHAR('R')
	PORT_BIT(0x00080000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("S") PORT_CODE(KEYCODE_S) PORT_CHAR('S')
	PORT_BIT(0x00100000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("T") PORT_CODE(KEYCODE_T) PORT_CHAR('T')
	PORT_BIT(0x00200000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("U") PORT_CODE(KEYCODE_U) PORT_CHAR('U')
	PORT_BIT(0x00400000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("V") PORT_CODE(KEYCODE_V) PORT_CHAR('V')
	PORT_BIT(0x00800000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("W") PORT_CODE(KEYCODE_W) PORT_CHAR('W')
	PORT_BIT(0x01000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("X") PORT_CODE(KEYCODE_X) PORT_CHAR('X')
	PORT_BIT(0x02000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Y") PORT_CODE(KEYCODE_Y) PORT_CHAR('Y')
	PORT_BIT(0x04000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Z") PORT_CODE(KEYCODE_Z) PORT_CHAR('Z')
	PORT_BIT(0x08000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("[") PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR('[')
	PORT_BIT(0x10000000,IP_ACTIVE_HIGH,IPT_UNUSED)
	PORT_BIT(0x20000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("]") PORT_CODE(KEYCODE_BACKSLASH) PORT_CHAR(']')
	PORT_BIT(0x40000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("^") PORT_CODE(KEYCODE_EQUALS) PORT_CHAR('^')
	PORT_BIT(0x80000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("_")

	PORT_START("key_modifiers")
	PORT_BIT(0x00000001,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("CTRL") PORT_CODE(KEYCODE_LCONTROL)
	PORT_BIT(0x00000002,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("SHIFT") PORT_CODE(KEYCODE_LSHIFT)
	PORT_BIT(0x00000004,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("KANA") PORT_CODE(KEYCODE_RCONTROL) PORT_TOGGLE
	PORT_BIT(0x00000008,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("CAPS") PORT_CODE(KEYCODE_CAPSLOCK) PORT_TOGGLE
	PORT_BIT(0x00000010,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("GRPH") PORT_CODE(KEYCODE_LALT)
INPUT_PORTS_END

static INTERRUPT_GEN( pc6001_interrupt )
{
	irq_vector = 0x16;
	cpu_set_input_line(device, 0, ASSERT_LINE);
}

static IRQ_CALLBACK ( pc6001_irq_callback )
{
	cpu_set_input_line(device, 0, CLEAR_LINE);
	return irq_vector;
}

static READ8_DEVICE_HANDLER (pc6001_8255_porta_r )
{
	return 0;
}

static WRITE8_DEVICE_HANDLER (pc6001_8255_porta_w )
{
	if(data != 0x06)
		printf("pc6001_8255_porta_w %02x\n",data);
}

static READ8_DEVICE_HANDLER (pc6001_8255_portb_r )
{
	return 0;
}

static WRITE8_DEVICE_HANDLER (pc6001_8255_portb_w )
{
	//printf("pc6001_8255_portb_w %02x\n",data);
}

static WRITE8_DEVICE_HANDLER (pc6001_8255_portc_w )
{
	//printf("pc6001_8255_portc_w %02x\n",data);
}

static READ8_DEVICE_HANDLER (pc6001_8255_portc_r )
{
	return 0x88;
}



static I8255A_INTERFACE( pc6001_ppi8255_interface )
{
	DEVCB_HANDLER(pc6001_8255_porta_r),
	DEVCB_HANDLER(pc6001_8255_portb_r),
	DEVCB_HANDLER(pc6001_8255_portc_r),
	DEVCB_HANDLER(pc6001_8255_porta_w),
	DEVCB_HANDLER(pc6001_8255_portb_w),
	DEVCB_HANDLER(pc6001_8255_portc_w)
};

static const msm8251_interface pc6001_usart_interface=
{
	NULL,
	NULL,
	NULL
};


static const ay8910_interface pc6001_ay_interface =
{
	AY8910_LEGACY_OUTPUT,
	AY8910_DEFAULT_LOADS,
	DEVCB_INPUT_PORT("P1"),
	DEVCB_INPUT_PORT("P2"),
	DEVCB_NULL,
	DEVCB_NULL
};

static UINT8 check_keyboard_press(running_machine *machine)
{
	const char* portnames[3] = { "key1","key2","key3" };
	int i,port_i,scancode;
	UINT8 shift_pressed,caps_lock;
	scancode = 0;

	shift_pressed = (input_port_read(machine,"key_modifiers") & 2)>>1;
	caps_lock = (input_port_read(machine,"key_modifiers") & 8)>>3;

	for(port_i=0;port_i<3;port_i++)
	{
		for(i=0;i<32;i++)
		{
			if((input_port_read(machine,portnames[port_i])>>i) & 1)
			{
				if((shift_pressed != caps_lock) && scancode >= 0x41 && scancode <= 0x5f)
					scancode+=0x20;

				if(shift_pressed && scancode >= 0x31 && scancode <= 0x39)
					scancode-=0x10;

				return scancode;
			}
			scancode++;
		}
	}

	return 0;
}

static TIMER_CALLBACK(cassette_callback)
{
	if(cas_switch == 1)
	{
		#if 0
		static UINT8 cas_data_i = 0x80,cas_data_poll;
		//cur_keycode = gfx_data[cas_offset++];
		if(cassette_input(machine->device("cass")) > 0.03)
			cas_data_poll|= cas_data_i;
		else
			cas_data_poll&=~cas_data_i;
		if(cas_data_i == 1)
		{
			cur_keycode = cas_data_poll;
			cas_data_i = 0x80;
			/* data ready, poll irq */
			irq_vector = 0x08;
			cputag_set_input_line(machine,"maincpu", 0, ASSERT_LINE);
		}
		else
			cas_data_i>>=1;
		#else
			UINT8 *cas_data = memory_region(machine, "cas");

			cur_keycode = cas_data[cas_offset++];
			popmessage("%04x %04x",cas_offset,cas_maxsize);
			if(cas_offset >= cas_maxsize)
			{
				cas_offset = 0;
				cas_switch = 0;
				irq_vector = 0x12;
				cputag_set_input_line(machine,"maincpu", 0, ASSERT_LINE);
			}
			else
			{
				irq_vector = 0x08;
				cputag_set_input_line(machine,"maincpu", 0, ASSERT_LINE);
			}
		#endif
	}
}

static TIMER_CALLBACK(audio_callback)
{
	if(cas_switch == 0)
	{
		irq_vector = 0x06;
		cputag_set_input_line(machine,"maincpu", 0, ASSERT_LINE);
	}
}

static TIMER_CALLBACK(keyboard_callback)
{
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	UINT32 key1 = input_port_read(machine,"key1");
	UINT32 key2 = input_port_read(machine,"key2");
	UINT32 key3 = input_port_read(machine,"key3");
	static UINT32 old_key1,old_key2,old_key3;

	if(cas_switch == 0)
	{
		if((key1 != old_key1) || (key2 != old_key2) || (key3 != old_key3))
		{
			cur_keycode = check_keyboard_press(space->machine);
			irq_vector = 0x02;
			cputag_set_input_line(machine,"maincpu", 0, ASSERT_LINE);
			old_key1 = key1;
			old_key2 = key2;
			old_key3 = key3;
		}
	}
}

static MACHINE_START(pc6001)
{
	/* TODO: accurate timing on these (especially for the first one) */
	timer_pulse(machine, ATTOTIME_IN_HZ(540), NULL, 0, audio_callback);
	timer_pulse(machine, ATTOTIME_IN_HZ(250), NULL, 0, keyboard_callback);
	timer_pulse(machine, ATTOTIME_IN_HZ(160), NULL, 0, cassette_callback);
}

static MACHINE_RESET(pc6001)
{
	port_c_8255=0;
	//pc6001_video_ram =  pc6001_ram;

	cpu_set_irq_callback(machine->device("maincpu"),pc6001_irq_callback);
	cas_switch = 0;
	cas_offset = 0;
}

static MACHINE_RESET(pc6001m2)
{
	port_c_8255=0;
	//pc6001_video_ram =  pc6001_ram;

	cpu_set_irq_callback(machine->device("maincpu"),pc6001_irq_callback);
	cas_switch = 0;
	cas_offset = 0;

	/* set default bankswitch (basic 0 & 1) */
	{
		UINT8 *ROM = memory_region(machine, "maincpu");
		memory_set_bankptr(machine, "bank1", &ROM[BASICROM(0)]);
		memory_set_bankptr(machine, "bank2", &ROM[BASICROM(1)]);
		// Added next two lines to prevent crash of driver, please update according to neeeds
		memory_set_bankptr(machine, "bank3", &ROM[BASICROM(0)]);
		memory_set_bankptr(machine, "bank4", &ROM[BASICROM(1)]);

		memory_set_bankptr(machine, "bank5", &ROM[WRAM(4)]);
		memory_set_bankptr(machine, "bank6", &ROM[WRAM(5)]);
		memory_set_bankptr(machine, "bank7", &ROM[WRAM(6)]);
		memory_set_bankptr(machine, "bank8", &ROM[WRAM(7)]);
	}
}

static const rgb_t defcolors[] =
{
	MAKE_RGB(0x00, 0xff, 0x00),	/* GREEN */
	MAKE_RGB(0xff, 0xff, 0x00),	/* YELLOW */
	MAKE_RGB(0x00, 0x00, 0xff),	/* BLUE */
	MAKE_RGB(0xff, 0x00, 0x00),	/* RED */
	MAKE_RGB(0xff, 0xff, 0xff),	/* BUFF */
	MAKE_RGB(0x00, 0xff, 0xff),	/* CYAN */
	MAKE_RGB(0xff, 0x00, 0xff),	/* MAGENTA */
	MAKE_RGB(0xff, 0x80, 0x00),	/* ORANGE */

	/* MC6847 specific */
	MAKE_RGB(0x00, 0x40, 0x00),	/* ALPHANUMERIC DARK GREEN */
	MAKE_RGB(0x00, 0xff, 0x00),	/* ALPHANUMERIC BRIGHT GREEN */
	MAKE_RGB(0x40, 0x10, 0x00),	/* ALPHANUMERIC DARK ORANGE */
	MAKE_RGB(0xff, 0xc4, 0x18)	/* ALPHANUMERIC BRIGHT ORANGE */

};

static PALETTE_INIT(pc6001)
{
	int i;

	for(i=0;i<8+4;i++)
		palette_set_color(machine, i+8,defcolors[i]);
}

static const cassette_config pc6001_cassette_config =
{
	pc6001_cassette_formats,
	NULL,
	(cassette_state)(CASSETTE_STOPPED | CASSETTE_MOTOR_DISABLED | CASSETTE_SPEAKER_ENABLED),
	NULL
};

static DEVICE_IMAGE_LOAD( pc6001_cass )
{
	UINT8 *cas = memory_region(image.device().machine, "cas");
	UINT32 size;

	size = image.length();
	if (image.fread( cas, size) != size)
	{
		image.seterror(IMAGE_ERROR_UNSPECIFIED, "Unable to fully read from file");
		return IMAGE_INIT_FAIL;
	}

	cas_maxsize = size;

	return IMAGE_INIT_PASS;
}

static MACHINE_DRIVER_START( pc6001 )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu",Z80, 7987200 / 4)
	MDRV_CPU_PROGRAM_MAP(pc6001_map)
	MDRV_CPU_IO_MAP(pc6001_io)
	MDRV_CPU_VBLANK_INT("screen", pc6001_interrupt)

//  MDRV_CPU_ADD("subcpu", I8049, 7987200)
	MDRV_MACHINE_START(pc6001)
	MDRV_MACHINE_RESET(pc6001)

	/* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_VIDEO_START(pc6001)
	MDRV_VIDEO_UPDATE(pc6001)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
//  MDRV_SCREEN_REFRESH_RATE(M6847_NTSC_FRAMES_PER_SECOND)
//  MDRV_VIDEO_UPDATE(m6847)
//  MDRV_SCREEN_FORMAT(BITMAP_FORMAT_RGB32)
	MDRV_SCREEN_SIZE(320, 25+192+26)
	MDRV_SCREEN_VISIBLE_AREA(0, 319, 0, 239)
	MDRV_PALETTE_LENGTH(16+4)
	MDRV_PALETTE_INIT(pc6001)

	MDRV_I8255A_ADD( "ppi8255", pc6001_ppi8255_interface )
	/* uart */
	MDRV_MSM8251_ADD("uart", pc6001_usart_interface)

	MDRV_CARTSLOT_ADD("cart")
	MDRV_CARTSLOT_EXTENSION_LIST("bin")
	MDRV_CARTSLOT_NOT_MANDATORY

//	MDRV_CASSETTE_ADD("cass",pc6001_cassette_config)
	MDRV_CARTSLOT_ADD("cass")
	MDRV_CARTSLOT_EXTENSION_LIST("cas,p6")
	MDRV_CARTSLOT_NOT_MANDATORY
	MDRV_CARTSLOT_INTERFACE("pc6001_cass")
	MDRV_CARTSLOT_LOAD(pc6001_cass)

	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("ay8910", AY8910, XTAL_4MHz/2)
	MDRV_SOUND_CONFIG(pc6001_ay_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
	MDRV_SOUND_WAVE_ADD("wave","cass")
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.05)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( pc6001m2 )
	MDRV_IMPORT_FROM(pc6001)

	MDRV_MACHINE_RESET(pc6001m2)

	MDRV_VIDEO_UPDATE(pc6001m2)

	/* basic machine hardware */
	MDRV_CPU_MODIFY("maincpu")
	MDRV_CPU_PROGRAM_MAP(pc6001m2_map)
	MDRV_CPU_IO_MAP(pc6001m2_io)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( pc6001sr )
	MDRV_IMPORT_FROM(pc6001)

	MDRV_VIDEO_UPDATE(pc6001m2)

	/* basic machine hardware */
	MDRV_CPU_REPLACE("maincpu", Z80, XTAL_3_579545MHz)
	MDRV_CPU_PROGRAM_MAP(pc6001m2_map)
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( pc6001 )	/* screen = 8000-83FF */
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "basicrom.60", 0x0000, 0x4000, CRC(54c03109) SHA1(c622fefda3cdc2b87a270138f24c05828b5c41d2) )

	ROM_REGION( 0x2000, "gfx1", 0 )
	ROM_LOAD( "cgrom60.60", 0x0000, 0x1000, CRC(b0142d32) SHA1(9570495b10af5b1785802681be94b0ea216a1e26) )
	ROM_RELOAD(             0x1000, 0x1000 )

	ROM_REGION( 0x1000, "mcu", ROMREGION_ERASEFF )
	ROM_LOAD( "i8049", 0x0000, 0x1000, NO_DUMP )

	ROM_REGION( 0x20000, "cas", ROMREGION_ERASEFF )

	ROM_REGION( 0x4000, "cart_img", ROMREGION_ERASE00 )
	ROM_CART_LOAD("cart", 0x0000, 0x3fff, ROM_OPTIONAL | ROM_MIRROR)
ROM_END

ROM_START( pc6001a )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "basicrom.60a", 0x0000, 0x4000, CRC(fa8e88d9) SHA1(c82e30050a837e5c8ffec3e0c8e3702447ffd69c) )

	ROM_REGION( 0x1000, "gfx1", 0 )
	ROM_LOAD( "cgrom60.60a", 0x0000, 0x1000, CRC(49c21d08) SHA1(9454d6e2066abcbd051bad9a29a5ca27b12ec897) )

	ROM_REGION( 0x1000, "mcu", ROMREGION_ERASEFF )
	ROM_LOAD( "i8049", 0x0000, 0x1000, NO_DUMP )

	ROM_REGION( 0x20000, "cas", ROMREGION_ERASEFF )

	ROM_REGION( 0x4000, "cart_img", ROMREGION_ERASE00 )
	ROM_CART_LOAD("cart", 0x0000, 0x3fff, ROM_OPTIONAL | ROM_MIRROR)
ROM_END

ROM_START( pc6001m2 )
	ROM_REGION( 0x48000, "maincpu", ROMREGION_ERASE00 )
	ROM_LOAD( "basicrom.62", 0x10000, 0x8000, CRC(950ac401) SHA1(fbf195ba74a3b0f80b5a756befc96c61c2094182) )
	ROM_LOAD( "voicerom.62", 0x18000, 0x4000, CRC(49b4f917) SHA1(1a2d18f52ef19dc93da3d65f19d3abbd585628af) )
	ROM_LOAD( "cgrom60.62",  0x1c000, 0x2000, CRC(81eb5d95) SHA1(53d8ae9599306ff23bf95208d2f6cc8fed3fc39f) )
	ROM_LOAD( "cgrom60m.62", 0x1e000, 0x2000, CRC(3ce48c33) SHA1(f3b6c63e83a17d80dde63c6e4d86adbc26f84f79) )
	ROM_LOAD( "kanjirom.62", 0x20000, 0x8000, CRC(20c8f3eb) SHA1(4c9f30f0a2ebbe70aa8e697f94eac74d8241cadd) )

	ROM_REGION( 0x1000, "mcu", ROMREGION_ERASEFF )
	ROM_LOAD( "i8049", 0x0000, 0x1000, NO_DUMP )

	ROM_REGION( 0x1000, "gfx1", 0 )
	ROM_COPY( "maincpu", 0x1c000, 0x00000, 0x1000 )

	ROM_REGION( 0x20000, "cas", ROMREGION_ERASEFF )

	ROM_REGION( 0x4000, "cart_img", ROMREGION_ERASE00 )
	ROM_CART_LOAD("cart", 0x0000, 0x3fff, ROM_OPTIONAL | ROM_MIRROR)
ROM_END

ROM_START( pc6001sr )
	ROM_REGION( 0x20000, "maincpu", ROMREGION_ERASEFF )
	/* If you split this into 4x 8000, the first 3 need to load to 0-7FFF (via bankswitch). The 4th part looks like gfx data */
	ROM_LOAD( "systemrom1.64", 0x0000, 0x10000, CRC(b6fc2db2) SHA1(dd48b1eee60aa34780f153359f5da7f590f8dff4) )
	ROM_LOAD( "systemrom2.64", 0x10000, 0x10000, CRC(55a62a1d) SHA1(3a19855d290fd4ac04e6066fe4a80ecd81dc8dd7) )

	ROM_REGION( 0x1000, "mcu", ROMREGION_ERASEFF )
	ROM_LOAD( "i8049", 0x0000, 0x1000, NO_DUMP )

	ROM_REGION( 0x4000, "gfx1", 0 )
	ROM_LOAD( "cgrom68.64", 0x0000, 0x4000, CRC(73bc3256) SHA1(5f80d62a95331dc39b2fb448a380fd10083947eb) )

	ROM_REGION( 0x20000, "cas", ROMREGION_ERASEFF )

	ROM_REGION( 0x4000, "cart_img", ROMREGION_ERASE00 )
	ROM_CART_LOAD("cart", 0x0000, 0x3fff, ROM_OPTIONAL | ROM_MIRROR)
ROM_END

ROM_START( pc6600 )	/* Variant of pc6001m2 */
	ROM_REGION( 0x14000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "basicrom.66", 0x0000, 0x8000, CRC(c0b01772) SHA1(9240bb6b97fe06f5f07b5d65541c4d2f8758cc2a) )
	ROM_LOAD( "voicerom.66", 0x10000, 0x4000, CRC(91d078c1) SHA1(6a93bd7723ef67f461394530a9feee57c8caf7b7) )

	ROM_REGION( 0xc000, "gfx1", 0 )
	ROM_LOAD( "cgrom60.66",  0x0000, 0x2000, CRC(d2434f29) SHA1(a56d76f5cbdbcdb8759abe601eab68f01b0a8fe8) )
	ROM_LOAD( "cgrom66.66",  0x2000, 0x2000, CRC(3ce48c33) SHA1(f3b6c63e83a17d80dde63c6e4d86adbc26f84f79) )
	ROM_LOAD( "kanjirom.66", 0x4000, 0x8000, CRC(20c8f3eb) SHA1(4c9f30f0a2ebbe70aa8e697f94eac74d8241cadd) )

	ROM_REGION( 0x1000, "mcu", ROMREGION_ERASEFF )
	ROM_LOAD( "i8049", 0x0000, 0x1000, NO_DUMP )

	ROM_REGION( 0x20000, "cas", ROMREGION_ERASEFF )

	ROM_REGION( 0x4000, "cart_img", ROMREGION_ERASE00 )
	ROM_CART_LOAD("cart", 0x0000, 0x3fff, ROM_OPTIONAL | ROM_MIRROR)
ROM_END

/* There exists an alternative (incomplete?) dump, consisting of more .68 pieces, but it's been probably created for emulators:
systemrom1.68 = 0x0-0x8000 BASICROM.68 + ??
systemrom2.68 = 0x0-0x2000 ?? + 0x2000-0x4000 SYSROM2.68 + 0x4000-0x8000 VOICEROM.68 + 0x8000-0x10000 KANJIROM.68
cgrom68.68 = CGROM60.68 + CGROM66.68
 */
ROM_START( pc6600sr )	/* Variant of pc6001sr */
	ROM_REGION( 0x20000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "systemrom1.68", 0x0000, 0x10000, CRC(b6fc2db2) SHA1(dd48b1eee60aa34780f153359f5da7f590f8dff4) )
	ROM_LOAD( "systemrom2.68", 0x10000, 0x10000, CRC(55a62a1d) SHA1(3a19855d290fd4ac04e6066fe4a80ecd81dc8dd7) )

	ROM_REGION( 0x4000, "gfx1", 0 )
	ROM_LOAD( "cgrom68.68", 0x0000, 0x4000, CRC(73bc3256) SHA1(5f80d62a95331dc39b2fb448a380fd10083947eb) )

	ROM_REGION( 0x1000, "mcu", ROMREGION_ERASEFF )
	ROM_LOAD( "i8049", 0x0000, 0x1000, NO_DUMP )

	ROM_REGION( 0x20000, "cas", ROMREGION_ERASEFF )

	ROM_REGION( 0x4000, "cart_img", ROMREGION_ERASE00 )
	ROM_CART_LOAD("cart", 0x0000, 0x3fff, ROM_OPTIONAL | ROM_MIRROR)
ROM_END

/*    YEAR  NAME      PARENT   COMPAT MACHINE   INPUT     INIT    COMPANY  FULLNAME          FLAGS */
COMP( 1981, pc6001,   0,       0,     pc6001,   pc6001,   0,      "Nippon Electronic Company",   "PC-6001",       GAME_NOT_WORKING )
COMP( 1981, pc6001a,  pc6001,  0,     pc6001,   pc6001,   0,      "Nippon Electronic Company",   "PC-6001A",      GAME_NOT_WORKING )	// US version of PC-6001
COMP( 1983, pc6001m2, pc6001,  0,     pc6001m2, pc6001,   0,      "Nippon Electronic Company",   "PC-6001mkII",   GAME_NOT_WORKING )
COMP( 1983, pc6600,   pc6001,  0,     pc6001m2, pc6001,   0,      "Nippon Electronic Company",   "PC-6600",       GAME_NOT_WORKING )	// high-end version of PC-6001mkII
COMP( 1984, pc6001sr, pc6001,  0,     pc6001sr, pc6001,   0,      "Nippon Electronic Company",   "PC-6001mkIISR", GAME_NOT_WORKING )
COMP( 1984, pc6600sr, pc6001,  0,     pc6001sr, pc6001,   0,      "Nippon Electronic Company",   "PC-6600SR",     GAME_NOT_WORKING )	// high-end version of PC-6001mkIISR

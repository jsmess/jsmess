/*********************************************************************

    ef9345.c

    Thomson EF9345 video controller emulator code

    This code is based on Daniel Coulom's implementation in DCVG5k
    and DCAlice released by Daniel Coulom under GPL license

    The implementation below is released under the MAME license for use
    in MAME, MESS and derivatives by permission of the author.

*********************************************************************/

#include "emu.h"
#include "ef9345.h"

#define MODE24x40	0
#define MODEVAR40	1
#define MODE8x80	2
#define MODE12x80	3
#define MODE16x40	4

//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************

// devices
const device_type EF9345 = ef9345_device_config::static_alloc_device_config;

// default address map
static ADDRESS_MAP_START( ef9345, 0, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_RAM
ADDRESS_MAP_END

//**************************************************************************
//  device configuration
//**************************************************************************

//-------------------------------------------------
//  ef9345_device_config - constructor
//-------------------------------------------------

ef9345_device_config::ef9345_device_config( const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock ):
	device_config( mconfig, static_alloc_device_config, "ef9345", tag, owner, clock),
	device_config_memory_interface(mconfig, *this),
	m_space_config("videoram", ENDIANNESS_LITTLE, 8, 16, 0, NULL, *ADDRESS_MAP_NAME(ef9345))
{
}


//-------------------------------------------------
//  static_alloc_device_config - allocate a new
//  configuration object
//-------------------------------------------------

device_config *ef9345_device_config::static_alloc_device_config( const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock )
{
	return global_alloc( ef9345_device_config( mconfig, tag, owner, clock ) );
}


//-------------------------------------------------
//  alloc_device - allocate a new device object
//-------------------------------------------------

device_t *ef9345_device_config::alloc_device( running_machine &machine ) const
{
	return auto_alloc( &machine, ef9345_device( machine, *this ) );
}

//-------------------------------------------------
//  memory_space_config - return a description of
//  any address spaces owned by this device
//-------------------------------------------------

const address_space_config *ef9345_device_config::memory_space_config(int spacenum) const
{
	return (spacenum == 0) ? &m_space_config : NULL;
}

//-------------------------------------------------
//  device_config_complete - perform any
//  operations now that the configuration is
//  complete
//-------------------------------------------------

void ef9345_device_config::device_config_complete()
{
	// inherit a copy of the static data
	const ef9345_interface *intf = reinterpret_cast<const ef9345_interface *>(static_config());

	if (intf != NULL)
	{
		*static_cast<ef9345_interface *>(this) = *intf;
	}
	// or initialize to defaults if none provided
	else
	{
		screen_tag = NULL;
	}
}


//**************************************************************************
//  INLINE HELPERS
//**************************************************************************

// calculate the internal RAM offset
inline UINT16 ef9345_device::indexram(UINT8 r)
{
	UINT8 x = registers[r];
	UINT8 y = registers[r - 1];
	if (y < 8)
		y &= 1;
	return ((x&0x3f) | ((x & 0x40) << 6) | ((x & 0x80) << 4) | ((y & 0x1f) << 6) | ((y & 0x20) << 8));
}

// calculate the internal ROM offset
inline UINT16 ef9345_device::indexrom(UINT8 r)
{
	UINT8 x = registers[r];
	UINT8 y = registers[r - 1];
	if (y < 8)
		y &= 1;
	return((x&0x3f)|((x&0x40)<<6)|((x&0x80)<<4)|((y&0x1f)<<6));
}

// increment x
inline void ef9345_device::inc_x(UINT8 r)
{
	UINT8 i = (registers[r] & 0x3f) + 1;
	if (i > 39)
	{
		i -= 40;
		state |= 0x40;
	}
	registers[r] = (registers[r] & 0xc0) | i;
}

// increment y
inline void ef9345_device::inc_y(UINT8 r)
{
	UINT8 i = (registers[r] & 0x1f) + 1;
	if (i > 31)
		i -= 24;
	registers[r] = (registers[r] & 0xe0) | i;
}


//**************************************************************************
//  live device
//**************************************************************************

//-------------------------------------------------
//  ef9345_device - constructor
//-------------------------------------------------

ef9345_device::ef9345_device( running_machine &_machine, const ef9345_device_config &config ) :
    device_t(_machine, config),
	device_memory_interface(_machine, config, *this),
    m_config(config)
{
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void ef9345_device::device_start()
{
	screen = machine->device<screen_device>(m_config.screen_tag);

	assert(screen != NULL);

	m_busy_timer = device_timer_alloc(*this, BUSY_TIMER);
	m_blink_timer = device_timer_alloc(*this, BLINKING_TIMER);

	videoram = space(0);
	charset = region();

	screen_out = auto_bitmap_alloc(machine, 496, screen->height() , BITMAP_FORMAT_INDEXED16);

	timer_adjust_periodic(m_blink_timer, ATTOTIME_IN_MSEC(500), 0, ATTOTIME_IN_MSEC(500));

	init_accented_chars();

	state_save_register_device_item_array(this, 0, border);
	state_save_register_device_item_array(this, 0, registers);
	state_save_register_device_item_array(this, 0, last_dial);
	state_save_register_device_item_array(this, 0, ram_base);
	state_save_register_device_item(this, 0, bf);
	state_save_register_device_item(this, 0, char_mode);
	state_save_register_device_item(this, 0, state);
	state_save_register_device_item(this, 0, TGS);
	state_save_register_device_item(this, 0, MAT);
	state_save_register_device_item(this, 0, PAT);
	state_save_register_device_item(this, 0, DOR);
	state_save_register_device_item(this, 0, ROR);
	state_save_register_device_item(this, 0, block);
	state_save_register_device_item(this, 0, blink);
	state_save_register_device_item(this, 0, latchc0);
	state_save_register_device_item(this, 0, latchm);
	state_save_register_device_item(this, 0, latchi);
	state_save_register_device_item(this, 0, latchu);

	state_save_register_device_item_bitmap(this, 0, screen_out);
}

//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------
void ef9345_device::device_reset()
{
	TGS = MAT = PAT = DOR = ROR = 0;
	state = 0;
	bf = 0;
	block = 0;
	blink = 0;
	latchc0 = 0;
	latchm = 0;
	latchi = 0;
	latchu = 0;
	char_mode = 0;

	memset(last_dial, 0, ARRAY_LENGTH(last_dial) * sizeof(UINT8));
	memset(registers, 0, ARRAY_LENGTH(registers) * sizeof(UINT8));
	memset(border, 0, ARRAY_LENGTH(border) * sizeof(UINT8));
	memset(border, 0, ARRAY_LENGTH(ram_base) * sizeof(UINT16));

	bitmap_fill(screen_out, NULL, 0);

	set_video_mode();
}

//-------------------------------------------------
//  device_timer - handler timer events
//-------------------------------------------------
void ef9345_device::device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr)
{
	switch(id)
	{
		case BUSY_TIMER:
			bf = 0;
			break;

		case BLINKING_TIMER:
			blink = !blink;
			break;
	}
}


// set busy flag and timer to clear it
void ef9345_device::set_busy_flag(int period)
{
	bf = 1;
	timer_adjust_oneshot(m_busy_timer, ATTOTIME_IN_USEC(period), 0);
}

// draw a char in 40 char line mode
void ef9345_device::draw_char_40(UINT8 *c, UINT16 x, UINT16 y)
{
	//verify size limit
	if (y * 10 >= screen->height() || x * 8 >= screen->width())
		return;

	for(int i = 0; i < 10; i++)
		for(int j = 0; j < 8; j++)
				*BITMAP_ADDR16(screen_out, y * 10 + i, x * 8 + j)  = c[8 * i + j] & 0x07;
}

// draw a char in 80 char line mode
void ef9345_device::draw_char_80(UINT8 *c, UINT16 x, UINT16 y)
{
	// verify size limit
	if (y * 10 >= screen->height() || x * 6 >= screen->width())
		return;

	for(int i = 0; i < 10; i++)
		for(int j = 0; j < 6; j++)
				*BITMAP_ADDR16(screen_out, y * 10 + i, x * 6 + j)  = c[6 * i + j] & 0x07;
}


// set then ef9345 mode
void ef9345_device::set_video_mode(void)
{
	char_mode = ((PAT & 0x80) >> 5) | ((TGS & 0xc0) >> 6);
	UINT16 new_width = (char_mode == MODE12x80 || char_mode == MODE8x80) ? 492 : 336;

	if (screen->width() != new_width)
	{
		rectangle visarea = screen->visible_area();
		visarea.max_x = new_width - 1;

		screen->configure(new_width, screen->height(), visarea, screen->frame_period().attoseconds);
	}

	//border color
	memset(border, MAT & 0x07, ARRAY_LENGTH(border));

	//set the base for the videoram charset
	ram_base[0] = ((DOR & 0x07) << 11);
	ram_base[1] = ram_base[0];
	ram_base[2] = ((DOR & 0x30) << 8);
	ram_base[3] = ram_base[2] + 0x0800;

	//address of the current memory block
	block = 0x0800 * ((((ROR & 0xf0) >> 4) | ((ROR & 0x40) >> 5) | ((ROR & 0x20) >> 3)) & 0x0c);
}

// initialize the ef9345 accented chars
void ef9345_device::init_accented_chars(void)
{
	UINT16 i, j;
	for(j = 0; j < 0x10; j++)
		for(i = 0; i < 0x200; i++)
			acc_char[(j << 9) + i] = charset->u8(0x0600 + i);

	for(j = 0; j < 0x200; j += 0x40)
		for(i = 0; i < 4; i++)
		{
			acc_char[0x0200 + j + i +  4] |= 0x1c; //tilde
			acc_char[0x0400 + j + i +  4] |= 0x10; //acute
			acc_char[0x0400 + j + i +  8] |= 0x08; //acute
			acc_char[0x0600 + j + i +  4] |= 0x04; //grave
			acc_char[0x0600 + j + i +  8] |= 0x08; //grave

			acc_char[0x0a00 + j + i +  4] |= 0x1c; //tilde
			acc_char[0x0c00 + j + i +  4] |= 0x10; //acute
			acc_char[0x0c00 + j + i +  8] |= 0x08; //acute
			acc_char[0x0e00 + j + i +  4] |= 0x04; //grave
			acc_char[0x0e00 + j + i +  8] |= 0x08; //grave

			acc_char[0x1200 + j + i +  4] |= 0x08; //point
			acc_char[0x1400 + j + i +  4] |= 0x14; //trema
			acc_char[0x1600 + j + i + 32] |= 0x08; //cedilla
			acc_char[0x1600 + j + i + 36] |= 0x04; //cedilla

			acc_char[0x1a00 + j + i +  4] |= 0x08; //point
			acc_char[0x1c00 + j + i +  4] |= 0x14; //trema
			acc_char[0x1e00 + j + i + 32] |= 0x08; //cedilla
			acc_char[0x1e00 + j + i + 36] |= 0x04; //cedilla
		}
}

// read a char in charset or in videoram
UINT8 ef9345_device::read_char(UINT8 index, UINT16 addr)
{
	if (index < 0x04)
		return charset->u8(0x0800*index + addr);
	else if (index < 0x08)
		return acc_char[0x0800*(index&3) + addr];
	else if (index < 0x0c)
		return videoram->read_byte(ram_base[index-8] + addr);
	else
		return videoram->read_byte(addr);
}

// calculate the dial position of the char
UINT8 ef9345_device::get_dial(UINT8 x, UINT8 attrib)
{
	if (x > 0 && last_dial[x-1] == 1) 		//top right
		last_dial[x] = 2;
	else if (x > 0 && last_dial[x-1] == 5)	//half right
		last_dial[x] = 10;
	else if (last_dial[x] == 1)				//bottom left
		last_dial[x] = 4;
	else if (last_dial[x] == 2)				//bottom right
		last_dial[x] = 8;
	else if (last_dial[x] == 3)				//lower half
		last_dial[x] = 12;
	else if (attrib == 1)					//Left half
		last_dial[x] = 5;
	else if (attrib == 2)					//half high
		last_dial[x] = 3;
	else if (attrib == 3)					//top left
		last_dial[x] = 1;
	else					 				//none
		last_dial[x] = 0;

	return last_dial[x];
}

// zoom the char
void ef9345_device::zoom(UINT8 *pix, UINT16 n)
{
	UINT8 i, j;
	if ((n & 0x0a) == 0)
		for(i = 0; i < 80; i += 8) // 1, 4, 5
			for(j = 7; j > 0; j--)
				pix[i + j] = pix[i + j / 2];

	if ((n & 0x05) == 0)
		for(i = 0; i < 80; i += 8) // 2, 8, 10
			for(j =0 ; j < 7; j++)
				pix[i + j] = pix[i + 4 + j / 2];

	if ((n & 0x0c) == 0)
		for(i = 0; i < 8; i++) // 1, 2, 3
			for(j = 9; j > 0; j--)
				pix[i + 8 * j] = pix[i + 8 * (j / 2)];

	if ((n & 0x03) == 0)
		for(i = 0; i < 8; i++) // 4, 8, 12
			for(j = 0; j < 9; j++)
				pix[i + 8 * j] = pix[i + 40 + 8 * (j / 2)];
}


// calculate the address of the char x,y
UINT16 ef9345_device::indexblock(UINT16 x, UINT16 y)
{
	UINT16 i = x, j;
	j = (y == 0) ? ((TGS & 0x20) >> 5) : ((ROR & 0x1f) + y - 1);

	//right side of a double width character
	if ((TGS & 0x80) == 0 && x > 0)
	{
		if (last_dial[x - 1] == 1) i--;
		if (last_dial[x - 1] == 4) i--;
		if (last_dial[x - 1] == 5) i--;
	}

	return 0x40 * j + i;
}

// draw bichrome character (40 columns)
void ef9345_device::bichrome40(UINT8 type, UINT16 address, UINT8 dial, UINT16 iblock, UINT16 x, UINT16 y, UINT8 c0, UINT8 c1, UINT8 insert, UINT8 flash, UINT8 hided, UINT8 negative, UINT8 underline)
{
	UINT16 i;
	UINT8 pix[80];

	if (flash && PAT & 0x40 && blink)
		c1 = c0;					//flash
	if (hided && PAT & 0x08)
		c1 = c0;        			//hided
	if (negative)					//negative
	{
		i = c1;
		c1 = c0;
		c0 = i;
	}

	if ((PAT & 0x30) == 0x30)
		insert = 0;         		//active area mark
	if (insert == 0)
		c1 += 8;                   	//foreground color
	if ((PAT & 0x30) == 0x00)
		insert = 1;         		//insert mode
	if (insert == 0)
		c0 += 8;                   	//background color

	//draw the cursor
	i = (registers[6] & 0x1f);
	if (i < 8)
		i &= 1;

	if (iblock == 0x40 * i + (registers[7] & 0x3f))	//cursor position
	{
		switch(MAT & 0x70)
		{
		case 0x40:					//00 = fixed complemented
			c0 = (23 - c0) & 15;
			c1 = (23 - c1) & 15;
			break;
		case 0x50:					//01 = fixed underlined
			underline = 1;
			break;
		case 0x60:					//10 = flash complemented
			if (blink)
			{
				c0 = (23 - c0) & 15;
				c1 = (23 - c1) & 15;
			}
			break;
		case 0x70:					//11 = flash underlined
			if (blink)
				underline = 1;
				break;
		}
	}

	// generate the pixel table
	for(i = 0; i < 40; i+=4)
	{
		UINT8 ch = read_char(type, address + i);

		for (UINT8 b=0; b<8; b++)
			pix[i*2 + b] = (ch & (1<<b)) ? c1 : c0;
	}

	//draw the underline
	if (underline)
		memset(&pix[72], c1, 8);

	if (dial > 0)
		zoom(pix, dial);

	//doubles the height of the char
	if (MAT & 0x80)
		zoom(pix, (y & 0x01) ? 0x0c : 0x03);

	draw_char_40(pix, x + 1 , y + 1);
}

// draw quadrichrome character (40 columns)
void ef9345_device::quadrichrome40(UINT8 c, UINT8 b, UINT8 a, UINT16 x, UINT16 y)
{
	//C0-6= character code
	//B0= insert             not yet implemented !!!
	//B1= low resolution
	//B2= subset index (low resolution only)
	//B3-5 = set number
	//A0-6 = 4 color palette

	UINT8 i, j, n, col[8], pix[80];
	UINT8 lowresolution = (b & 0x02) >> 1, ramx, ramy, ramblock;
	UINT16 ramindex;

	//quadrichrome don't suppor double size
	last_dial[x] = 0;

	//initialize the color table
	for(j = 1, n = 0, i = 0; i < 8; i++)
	{
		col[n++] = (a & j) ? i : 7;
		j <<= 1;
	}

	//find block number in ram
	ramblock = 0;
	if (b & 0x20)	ramblock |= 4;      //B5
	if (b & 0x08)	ramblock |= 2;      //B3
	if (b & 0x10)	ramblock |= 1;      //B4

	//find character address in ram
	ramx = c & 0x03;
	ramy =(c & 0x7f) >> 2;
	ramindex = 0x0800 * ramblock + 0x40 * ramy + ramx;
	if (lowresolution) ramindex += 5 * (b & 0x04);

	//fill pixel table
	for(i = 0, j = 0; i < 10; i++)
	{
		UINT8 ch = read_char(0x0c, ramindex + 4 * (i >> lowresolution));
		pix[j] = pix[j + 1] = col[(ch & 0x03) >> 0]; j += 2;
		pix[j] = pix[j + 1] = col[(ch & 0x0c) >> 2]; j += 2;
		pix[j] = pix[j + 1] = col[(ch & 0x30) >> 4]; j += 2;
		pix[j] = pix[j + 1] = col[(ch & 0xc0) >> 6]; j += 2;
	}

	draw_char_40(pix, x + 1, y + 1);
}

// draw bichrome character (80 columns)
void ef9345_device::bichrome80(UINT8 c, UINT8 a, UINT16 x, UINT16 y)
{
	UINT8 c0, c1, pix[60];
	UINT16 i, j, d;
	
	c1 = (a & 1) ? (DOR >> 4) & 7 : DOR & 7; //foreground color = DOR
	c0 =  MAT & 7;                           //background color = MAT

	switch(c & 0x80)
	{
	case 0: //alphanumeric G0 set
		//A0: D = color set
		//A1: U = underline
		//A2: F = flash
		//A3: N = negative
		//C0-6: character code

		if ((a & 4) && (PAT & 0x40) && (blink))
			c1 = c0; 	//flash
		if (a & 8)		//negative
		{
			i = c1;
			c1 = c0;
			c0 = i;
		}

		d = ((c & 0x7f) >> 2) * 0x40 + (c & 0x03);  //char position

		for(i=0, j=0; i < 10; i++)
		{
			UINT8 ch = read_char(0, d + 4 * i);
			for (UINT8 b=0; b<6; b++)
				pix[j++] = (ch & (1<<b)) ? c1 : c0;
		}

		//draw the underline
		if (a & 2)
			memset(&pix[54], c1, 6);

		break;
	default: //dedicated mosaic set
		//A0: D = color set
		//A1-3: 3 blocks de 6 pixels
		//C0-6: 7 blocks de 6 pixels
		pix[ 0] = (c & 0x01) ? c1 : c0;
		pix[ 3] = (c & 0x02) ? c1 : c0;
		pix[12] = (c & 0x04) ? c1 : c0;
		pix[15] = (c & 0x08) ? c1 : c0;
		pix[24] = (c & 0x10) ? c1 : c0;
		pix[27] = (c & 0x20) ? c1 : c0;
		pix[36] = (c & 0x40) ? c1 : c0;
		pix[39] = (a & 0x02) ? c1 : c0;
		pix[48] = (a & 0x04) ? c1 : c0;
		pix[51] = (a & 0x08) ? c1 : c0;

		for(i = 0; i < 60; i += 12)
		{
			pix[i + 6] = pix[i];
			pix[i + 9] = pix[i + 3];
		}

		for(i = 0; i < 60; i += 3)
			pix[i + 2] = pix[i + 1] = pix[i];

		break;
	}

	draw_char_80(pix, x, y);
}

// generate 16 bits 40 columns char
void ef9345_device::makechar_16x40(UINT16 x, UINT16 y)
{
	UINT8 a, b, c0, c1, i, f, m, n, u, type, dial;
	UINT16 address, iblock;

	iblock = (MAT & 0x80 && y > 1) ? indexblock(x, y / 2) : indexblock(x, y);
	a = videoram->read_byte(block + iblock);
	b = videoram->read_byte(block + iblock + 0x0800);

	dial = get_dial(x, (a & 0x80) ? 0 : (((a & 0x20) >> 5) | ((a & 0x10) >> 3)));

	//type and address of the char
	type = ((b & 0x80) >> 4) | ((a & 0x80) >> 6);
	address = ((b & 0x7f) >> 2) * 0x40 + (b & 0x03);

	 //negative space
	if ((b & 0xe0) == 0x80)
	{
		address = 0;
		type = 3;
	}

	//reset attributes latch
	if (x == 0)
		latchm = latchi = latchu = latchc0 = 0;

	if (type == 4)
	{
		latchm = b & 1;
		latchi = (b & 2) >> 1;
		latchu = (b & 4) >> 2;
	}

	if (a & 0x80)
		latchc0 = (a & 0x70) >> 4;

	//char attributes
	c0 = latchc0;							//background
	c1 = a & 0x07;							//foreground
	i = latchi;								//insert mode
	f  = (a & 0x08) >> 3;					//flash
	m = latchm;								//hided
	n  = (a & 0x80) ? 0: ((a & 0x40) >> 6);	//negative
	u = latchu;								//underline

	bichrome40(type, address, dial, iblock, x, y, c0, c1, i, f, m, n, u);
}

// generate 24 bits 40 columns char
void ef9345_device::makechar_24x40(UINT16 x, UINT16 y)
{
	UINT8 a, b, c, c0, c1, i, f, m, n, u, type, dial;
	UINT16 address, iblock;

	iblock = (MAT & 0x80 && y > 1) ? indexblock(x, y / 2) : indexblock(x, y);
	c = videoram->read_byte(block + iblock);
	b = videoram->read_byte(block + iblock + 0x0800);
	a = videoram->read_byte(block + iblock + 0x1000);

	if ((b & 0xc0) == 0xc0)
	{
		quadrichrome40(c, b, a, x, y);
		return;
	}

	dial = get_dial(x, (b & 0x02) + ((b & 0x08) >> 3));

	//type and address of the char
	address = ((c & 0x7f) >> 2) * 0x40 + (c & 0x03);
	type = (b & 0xf0) >> 4;

	//char attributes
	c0 = a & 0x07;					//background
	c1 = (a & 0x70) >> 4;			//foreground
	i = b & 0x01;					//insert
	f = (a & 0x08) >> 3;			//flash
	m = (b & 0x04) >> 2;			//hided
	n = ((a & 0x80) >> 7);			//negative
	u = (((b & 0x60) == 0) || ((b & 0xc0) == 0x40)) ? ((b & 0x10) >> 4) : 0; //underline

	bichrome40(type, address, dial, iblock, x, y, c0, c1, i, f, m, n, u);
}

// generate 12 bits 80 columns char
void ef9345_device::makechar_12x80(UINT16 x, UINT16 y)
{
	UINT16 iblock = indexblock(x, y);
	bichrome80(videoram->read_byte(block + iblock), (videoram->read_byte(block + iblock + 0x1000) >> 4) & 0x0f, 2 * x + 1, y + 1);
	bichrome80(videoram->read_byte(block + iblock + 0x0800), videoram->read_byte(block + iblock + 0x1000) & 0x0f, 2 * x + 2, y + 1);
}

void ef9345_device::draw_border(UINT16 line)
{
	if (char_mode == MODE12x80 || char_mode == MODE8x80)
		for(int i = 0; i < 82; i++)
			draw_char_80(border, i, line);
	else
		for(int i = 0; i < 42; i++)
			draw_char_40(border, i, line);
}

void ef9345_device::makechar(UINT16 x, UINT16 y)
{
	switch (char_mode)
	{
		case MODE24x40:
			makechar_24x40(x, y);
			break;
		case MODEVAR40:
		case MODE8x80:
			logerror("Unemulated EF9345 mode: %02x\n", char_mode);
			break;
		case MODE12x80:
			makechar_12x80(x, y);
			break;
		case MODE16x40:
			makechar_16x40(x, y);
			break;
		default:
			logerror("Unknown EF9345 mode: %02x\n", char_mode);
			break;
	}
}

// Execute EF9345 command
void ef9345_device::ef9345_exec(UINT8 cmd)
{
	state = 0;
	if ((registers[5] & 0x3f) == 39) state |= 0x10; //S4(LXa) set
	if ((registers[7] & 0x3f) == 39) state |= 0x20; //S5(LXm) set

	UINT16 a = indexram(7);

	switch(cmd)
	{
		case 0x00:	//KRF: R1,R2,R3->ram
		case 0x01:	//KRF: R1,R2,R3->ram + increment
			set_busy_flag(4);
			videoram->write_byte(a, registers[1]);
			videoram->write_byte(a + 0x0800, registers[2]);
			videoram->write_byte(a + 0x1000, registers[3]);
			if (cmd&1) inc_x(7);
			break;
		case 0x02:	//KRG: R1,R2->ram
		case 0x03:	//KRG: R1,R2->ram + increment
			set_busy_flag(5.5);
			videoram->write_byte(a, registers[1]);
			videoram->write_byte(a + 0x0800, registers[2]);
			if (cmd&1) inc_x(7);
			break;
		case 0x08:	//KRF: ram->R1,R2,R3
		case 0x09:	//KRF: ram->R1,R2,R3 + increment
			set_busy_flag(7.5);
			registers[1] = videoram->read_byte(a);
			registers[2] = videoram->read_byte(a + 0x0800);
			registers[3] = videoram->read_byte(a + 0x1000);
			if (cmd&1) inc_x(7);
			break;
		case 0x0a:	//KRG: ram->R1,R2
		case 0x0b:	//KRG: ram->R1,R2 + increment
			set_busy_flag(7.5);
			registers[1] = videoram->read_byte(a);
			registers[2] = videoram->read_byte(a + 0x0800);
			if (cmd&1) inc_x(7);
			break;
		case 0x30:	//OCT: R1->RAM, main pointer
		case 0x31:	//OCT: R1->RAM, main pointer + inc
			set_busy_flag(4);
			videoram->write_byte(indexram(7), registers[1]);

			if (cmd&1)
			{
				inc_x(7);
				if ((registers[7] & 0x3f) == 0)
					inc_y(6);
			}
			break;
		case 0x34:	//OCT: R1->RAM, aux pointer
		case 0x35:	//OCT: R1->RAM, aux pointer + inc
			set_busy_flag(4);
			videoram->write_byte(indexram(5), registers[1]);

			if (cmd&1)
				inc_x(5);
			break;
		case 0x38:	//OCT: RAM->R1, main pointer
		case 0x39:	//OCT: RAM->R1, main pointer + inc
			set_busy_flag(4.5);
			registers[1] = videoram->read_byte(indexram(7));

			if (cmd&1)
			{
				inc_x(7);

				if ((registers[7] & 0x3f) == 0)
					inc_y(6);
			}
			break;
		case 0x3c:	//OCT: RAM->R1, aux pointer
		case 0x3d:	//OCT: RAM->R1, aux pointer + inc
			set_busy_flag(4.5);
			registers[1] = videoram->read_byte(indexram(5));

			if (cmd&1)
				inc_x(5);
			break;
		case 0x50:	//KRL: 80 UINT8 - 12 bits write
		case 0x51:	//KRL: 80 UINT8 - 12 bits write + inc
			set_busy_flag(12.5);
			videoram->write_byte(a, registers[1]);
			switch((a / 0x0800) & 1)
			{
				case 0:
				{
					UINT8 tmp_data = videoram->read_byte(a + 0x1000);
					videoram->write_byte(a + 0x1000, (tmp_data & 0x0f) | (registers[3] & 0xf0));
					break;
				}
				case 1:
				{
					UINT8 tmp_data = videoram->read_byte(a + 0x0800);
					videoram->write_byte(a + 0x0800, (tmp_data & 0xf0) | (registers[3] & 0x0f));
					break;
				}
			}
			if (cmd&1)
			{
				if ((registers[7] & 0x80) == 0x00) { registers[7] |= 0x80; return; }
				registers[7] &= 0x80;
				inc_x(7);
			}
			break;
		case 0x58:	//KRL: 80 UINT8 - 12 bits read
		case 0x59:	//KRL: 80 UINT8 - 12 bits read + inc
			set_busy_flag(11.5);
			registers[1] = videoram->read_byte(a);
			switch((a / 0x0800) & 1)
			{
				case 0:
					registers[3] = videoram->read_byte(a + 0x1000);
					break;
				case 1:
					registers[3] = videoram->read_byte(a + 0x0800);
					break;
			}
			if (cmd&1)
			{
				if ((registers[7] & 0x80) == 0x00)
				{
					registers[7] |= 0x80;
					break;;
				}
				registers[7] &= 0x80;
				inc_x(7);
			}
			break;
		case 0x80:	//IND: R1->ROM (impossible ?)
			break;
		case 0x81:	//IND: R1->TGS
		case 0x82:	//IND: R1->MAT
		case 0x83:	//IND: R1->PAT
		case 0x84:	//IND: R1->DOR
		case 0x87:	//IND: R1->ROR
			set_busy_flag(2);
			switch(cmd&7)
			{
				case 1: 	TGS = registers[1]; break;
				case 2: 	MAT = registers[1]; break;
				case 3: 	PAT = registers[1]; break;
				case 4: 	DOR = registers[1]; break;
				case 7: 	ROR = registers[1]; break;
			}
			set_video_mode();
			state &= 0x8f;  //reset S4(LXa), S5(LXm), S6(Al)
			break;
		case 0x88:	//IND: ROM->R1
		case 0x89:	//IND: TGS->R1
		case 0x8a:	//IND: MAT->R1
		case 0x8b:	//IND: PAT->R1
		case 0x8c:	//IND: DOR->R1
		case 0x8f:	//IND: ROR->R1
			set_busy_flag(3.5);
			switch(cmd&7)
			{
				case 0: 	registers[1] = charset->u8(indexrom(7) & 0x1fff);
				case 1: 	registers[1] = TGS; break;
				case 2: 	registers[1] = MAT; break;
				case 3: 	registers[1] = PAT; break;
				case 4: 	registers[1] = DOR; break;
				case 7: 	registers[1] = ROR; break;
			}
			state &= 0x8f;  //reset S4(LXa), S5(LXm), S6(Al)
			break;
		case 0x90:	//NOP: no operation
		case 0x91:	//NOP: no operation
		case 0x95:	//VRM: vertical sync mask reset
		case 0x99:	//VSM: vertical sync mask set
			break;
		case 0xb0:	//INY: increment Y
			set_busy_flag(2);
			inc_y(6);
			state &= 0x8f;  //reset S4(LXa), S5(LXm), S6(Al)
			break;
		case 0xd5:	//MVB: move buffer MP->AP stop
		case 0xd6:	//MVB: move buffer MP->AP nostop
		case 0xd9:	//MVB: move buffer AP->MP stop
		case 0xda:	//MVB: move buffer AP->MP nostop
		case 0xe5:	//MVD: move double buffer MP->AP stop
		case 0xe6:	//MVD: move double buffer MP->AP nostop
		case 0xe9:	//MVD: move double buffer AP->MP stop
		case 0xea:	//MVD: move double buffer AP->MP nostop
		case 0xf5:	//MVT: move triple buffer MP->AP stop
		case 0xf6:	//MVT: move triple buffer MP->AP nostop
		case 0xf9:	//MVT: move triple buffer AP->MP stop
		case 0xfa:	//MVT: move triple buffer AP->MP nostop
		{
			UINT16 i, a1, a2;
			UINT8 n = (cmd>>4) - 0x0c;
			UINT8 r1 = (cmd&0x04) ? 7 : 5;
			UINT8 r2 = (cmd&0x04) ? 5 : 7;
			int busy = 2;

			for(i = 0; i < 1280; i++)
			{
				a1 = indexram(r1); a2 = indexram(r2);
				videoram->write_byte(a2, videoram->read_byte(a1));

				if (n > 1) videoram->write_byte(a2 + 0x0800, videoram->read_byte(a1 + 0x0800));
				if (n > 2) videoram->write_byte(a2 + 0x1000, videoram->read_byte(a1 + 0x1000));

				inc_x(r1);
				inc_x(r2);
				if ((registers[5] & 0x3f) == 0 && (cmd&1))
					break;

				if ((registers[7] & 0x3f) == 0)
				{
					if (cmd&1)
						break;
					else
					inc_y(6);
				}

				busy += 4 * n;
			}
			state &= 0x8f;  //reset S4(LXa), S5(LXm), S6(Al)
			set_busy_flag(busy);
		}
		break;
		case 0x05:	//CLF: Clear page 24 bits
		case 0x07:	//CLG: Clear page 16 bits
		case 0x40:	//KRC: R1 -> ram
		case 0x41:	//KRC: R1 -> ram + inc
		case 0x48:	//KRC: 80 characters - 8 bits
		case 0x49:	//KRC: 80 characters - 8 bits
		default:
			logerror("Unemulated EF9345 cmd: %02x\n", cmd);
	}
}


/**************************************************************
            EF9345 interface
**************************************************************/

void ef9345_device::video_update(bitmap_t *bitmap, const rectangle *cliprect)
{
	copybitmap(bitmap, screen_out, 0, 0, 0, 0, cliprect);
}

void ef9345_device::update_scanline(UINT16 scanline)
{
	UINT16 i;

	if (scanline == 250)
		state &= 0xfb;

	set_busy_flag(104);

	if (char_mode == MODE12x80 || char_mode == MODE8x80)
	{
		draw_char_80(border, 0, (scanline / 10) + 1);
		draw_char_80(border, 81, (scanline / 10) + 1);
	}
	else
	{
		draw_char_40(border, 0, (scanline / 10) + 1);
		draw_char_40(border, 41, (scanline / 10) + 1);
	}

	if (scanline == 0)
	{
		state |= 0x04;
		draw_border(0);
		if (PAT & 1)
			for(i = 0; i < 40; i++)
				makechar(i, (scanline / 10));
		else
			for(i = 0; i < 42; i++)
				draw_char_40(border, i, 1);
	}
	else if (scanline < 120)
	{
		if (PAT & 2)
			for(i = 0; i < 40; i++)
				makechar(i, (scanline / 10));
		else
			draw_border(scanline / 10);
	}
	else if (scanline < 250)
	{
		if (PAT & 4)
			for(i = 0; i < 40; i++)
				makechar(i, (scanline / 10));
		else
			draw_border(scanline / 10);

		if (scanline == 240)
			draw_border(26);
	}
}

READ8_MEMBER( ef9345_device::data_r )
{
	if (offset & 7)
		return registers[offset & 7];

	if (bf)
		state |= 0x80;
	else
		state &= 0x7f;

	return state;
}

WRITE8_MEMBER( ef9345_device::data_w )
{
	registers[offset & 7] = data;

	if (offset & 8)
		ef9345_exec(registers[0] & 0xff);
}

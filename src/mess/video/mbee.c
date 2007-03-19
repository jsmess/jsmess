/***************************************************************************
	microbee.c

    video hardware
	Juergen Buchmueller <pullmoll@t-online.de>, Dec 1999

****************************************************************************/

#include "driver.h"
#include "video/generic.h"
#include "includes/mbee.h"

typedef struct {		 // CRTC 6545
	UINT8 horizontal_total;
    UINT8 horizontal_displayed;
    UINT8 horizontal_sync_pos;
    UINT8 horizontal_length;
    UINT8 vertical_total;
    UINT8 vertical_adjust;
    UINT8 vertical_displayed;
    UINT8 vertical_sync_pos;
    UINT8 crt_mode;
    UINT8 scan_lines;
    UINT8 cursor_top;
    UINT8 cursor_bottom;
    UINT8 screen_address_hi;
    UINT8 screen_address_lo;
    UINT8 cursor_address_hi;
    UINT8 cursor_address_lo;
	UINT8 lpen_hi;
	UINT8 lpen_lo;
	UINT8 transp_hi;
	UINT8 transp_lo;
    UINT8 idx;
	UINT8 cursor_visible;
	UINT8 cursor_phase;
	UINT8 lpen_strobe;
	UINT8 update_strobe;
} CRTC6545;

static CRTC6545 crt;
static int off_x = 0;
static int off_y = 0;
static int framecnt = 0;
static int update_all = 0;
static int m6545_color_bank = 0;
static int m6545_video_bank = 0;
static int mbee_pcg_color_latch = 0;

char mbee_frame_message[128+1];
int mbee_frame_counter;


UINT8 *pcgram;

/* from mess/systems/microbee.c */
extern gfx_layout mbee_charlayout;


WRITE8_HANDLER ( mbee_pcg_color_latch_w )
{
	logerror("mbee pcg_color_latch_w $%02X\n", data);
	mbee_pcg_color_latch = data;
}

 READ8_HANDLER ( mbee_pcg_color_latch_r )
{
	int data = mbee_pcg_color_latch;
	logerror("mbee pcg_color_latch_r $%02X\n", data);
	return data;
}

WRITE8_HANDLER ( mbee_videoram_w )
{
    if( videoram[offset] != data )
	{
		logerror("mbee videoram [$%04X] <- $%02X\n", offset, data);
		videoram[offset] = data;
		colorram[offset] = 2;
		dirtybuffer[offset] = 1;
	}
}

 READ8_HANDLER ( mbee_videoram_r )
{
	int data;
	if( m6545_video_bank & 0x01 )
	{
		data = pcgram[offset];
		logerror("mbee pcgram [$%04X] -> $%02X\n", offset, data);
	}
	else
	{
		data = videoram[offset];
		logerror("mbee videoram [$%04X] -> $%02X\n", offset, data);
    }
    return data;
}

WRITE8_HANDLER ( mbee_pcg_color_w )
{
	if( (m6545_video_bank & 0x01) || (mbee_pcg_color_latch & 0x40) == 0 )
	{
		if( pcgram[0x0800+offset] != data )
        {
            int chr = 0x80 + offset / 16;
			int i;

            logerror("mbee pcgram  [$%04X] <- $%02X\n", offset, data);
            pcgram[0x0800+offset] = data;
            /* decode character graphics again */
            decodechar(Machine->gfx[0], chr, pcgram, &mbee_charlayout);

            /* mark all visible characters with that code dirty */
            for( i = 0; i < videoram_size; i++ )
            {
                if( videoram[i] == chr )
                    dirtybuffer[i] = 1;
            }
        }
    }
	else
	{
		if( colorram[offset] != data )
        {
            logerror("colorram [$%04X] <- $%02X\n", offset, data);
            colorram[offset] = data;
            dirtybuffer[offset] = 1;
        }
	}
}

 READ8_HANDLER ( mbee_pcg_color_r )
{
	int data;

	if( mbee_pcg_color_latch & 0x40 )
        data = colorram[offset];
	else
		data = pcgram[0x0800+offset];
    return data;
}

static int keyboard_matrix_r(int offs)
{
	int port = (offs >> 7) & 7;
	int bit = (offs >> 4) & 7;
	int data = (readinputport(port) >> bit) & 1;
	int extra = readinputport(8);

	if( extra & 0x01 )	/* extra: cursor up */
	{
		if( port == 7 && bit == 1 ) data = 1;	/* Control */
		if( port == 0 && bit == 5 ) data = 1;	/* E */
	}
	if( extra & 0x02 )	/* extra: cursor down */
	{
		if( port == 7 && bit == 1 ) data = 1;	/* Control */
		if( port == 3 && bit == 0 ) data = 1;	/* X */
    }
	if( extra & 0x04 )	/* extra: cursor left */
	{
		if( port == 7 && bit == 1 ) data = 1;	/* Control */
		if( port == 2 && bit == 3 ) data = 1;	/* S */
    }
	if( extra & 0x08 )	/* extra: cursor right */
	{
		if( port == 7 && bit == 1 ) data = 1;	/* Control */
		if( port == 0 && bit == 4 ) data = 1;	/* D */
    }
	if( extra & 0x10 )	/* extra: insert */
	{
		if( port == 7 && bit == 1 ) data = 1;	/* Control */
		if( port == 2 && bit == 6 ) data = 1;	/* V */
    }
    if( data )
	{
		crt.lpen_lo = offs & 0xff;
		crt.lpen_hi = (offs >> 8) & 0x03;
		crt.lpen_strobe = 1;
		logerror("mbee keyboard_matrix_r $%03X (port:%d bit:%d) = %d\n", offs, port, bit, data);
	}
	return data;
}

static void m6545_offset_xy(void)
{
    if( crt.horizontal_sync_pos )
		off_x = crt.horizontal_total - crt.horizontal_sync_pos - 23;
	else
		off_x = -24;

	off_y = (crt.vertical_total - crt.vertical_sync_pos) *
		(crt.scan_lines + 1) + crt.vertical_adjust;

	if( off_y < 0 )
		off_y = 0;

	if( off_y > 128 )
		off_y = 128;

	logerror("6545 offset x:%d  y:%d\n", off_x, off_y);
}

 READ8_HANDLER ( mbee_color_bank_r )
{
	int data = m6545_color_bank;
	logerror("6545 color_bank_r $%02X\n", data);
	return data;
}

WRITE8_HANDLER ( mbee_color_bank_w )
{
	logerror("6545 color_bank_w $%02X\n", data);
	m6545_color_bank = data;
}

 READ8_HANDLER ( mbee_video_bank_r )
{
	int data = m6545_video_bank;
	logerror("6545 video_bank_r $%02X\n", data);
	return data;
}

WRITE8_HANDLER ( mbee_video_bank_w )
{
	logerror("6545 video_bank_w $%02X\n", data);
    m6545_video_bank = data;
}

static void m6545_update_strobe(int param)
{
	int data;
    data = keyboard_matrix_r(param);
    crt.update_strobe = 1;
	if( data )
	{
		logerror("6545 update_strobe_cb $%04X = $%02X\n", param, data);
	}
}

 READ8_HANDLER ( m6545_status_r )
{
	int data = 0, y = cpu_getscanline();

	if( y < Machine->screen[0].visarea.min_y ||
		y > Machine->screen[0].visarea.max_y )
		data |= 0x20;	/* vertical blanking */
	if( crt.lpen_strobe )
		data |= 0x40;	/* lpen register full */
	if( crt.update_strobe )
		data |= 0x80;	/* update strobe has occured */
	logerror("6545 status_r $%02X\n", data);
    return data;
}

 READ8_HANDLER ( m6545_data_r )
{
	int addr, data = 0;

    switch( crt.idx )
	{
/* These are write only on a Rockwell 6545 */
#if 0
    case 0:
		return crt.horizontal_total;
	case 1:
		return crt.horizontal_displayed;
	case 2:
		return crt.horizontal_sync_pos;
	case 3:
		return crt.horizontal_length;
	case 4:
		return crt.vertical_total;
	case 5:
		return crt.vertical_adjust;
	case 6:
		return crt.vertical_displayed;
	case 7:
		return crt.vertical_sync_pos;
	case 8:
		return crt.crt_mode;
	case 9:
		return crt.scan_lines;
	case 10:
		return crt.cursor_top;
	case 11:
		return crt.cursor_bottom;
	case 12:
		return crt.screen_address_hi;
	case 13:
		return crt.screen_address_lo;
#endif
    case 14:
		data = crt.cursor_address_hi;
		break;
	case 15:
		data = crt.cursor_address_lo;
		break;
	case 16:
		logerror("6545 lpen_hi_r $%02X (lpen:%d upd:%d)\n", crt.lpen_hi, crt.lpen_strobe, crt.update_strobe);
		crt.lpen_strobe = 0;
        crt.update_strobe = 0;
		data = crt.lpen_hi;
		break;
	case 17:
		logerror("6545 lpen_lo_r $%02X (lpen:%d upd:%d)\n", crt.lpen_lo, crt.lpen_strobe, crt.update_strobe);
        crt.lpen_strobe = 0;
		crt.update_strobe = 0;
        data = crt.lpen_lo;
		break;
	case 18:
		logerror("6545 transp_hi_r $%02X\n", crt.transp_hi);
		data = crt.transp_hi;
		break;
	case 19:
		logerror("6545 transp_lo_r $%02X\n", crt.transp_lo);
		data = crt.transp_lo;
		break;
	case 31:
		/* shared memory latch */
		addr = crt.transp_hi * 256 + crt.transp_lo;
		logerror("6545 transp_latch $%04X\n", addr);
		m6545_update_strobe(addr);
        break;
    default:
		logerror("6545 read unmapped port $%X\n", crt.idx);
    }
	return data;
}

WRITE8_HANDLER ( m6545_index_w )
{
	crt.idx = data & 0x1f;
}

WRITE8_HANDLER ( m6545_data_w )
{
	int addr;

    switch( crt.idx )
	{
	case 0:
		if( crt.horizontal_total == data )
			break;
		crt.horizontal_total = data;
        logerror("6545 horizontal total        %d\n", data);
        m6545_offset_xy();
		break;
	case 1:
		if( crt.horizontal_displayed == data )
			break;
		crt.horizontal_displayed = data;
		logerror("6545 horizontal displayed    %d\n", data);
        break;
	case 2:
		if( crt.horizontal_sync_pos == data )
			break;
		crt.horizontal_sync_pos = data;
		logerror("6545 horizontal sync pos     %d\n", data);
        m6545_offset_xy();
		break;
	case 3:
		crt.horizontal_length = data;
		logerror("6545 horizontal length       %d\n", data);
        break;
	case 4:
		if( crt.vertical_total == data )
			break;
		crt.vertical_total = data;
		logerror("6545 vertical total          %d\n", data);
        m6545_offset_xy();
		break;
	case 5:
		if( crt.vertical_adjust == data )
			break;
		crt.vertical_adjust = data;
		logerror("6545 vertical adjust         %d\n", data);
        m6545_offset_xy();
		break;
	case 6:
		if( crt.vertical_displayed == data )
			break;
		logerror("6545 vertical displayed      %d\n", data);
        crt.vertical_displayed = data;
		break;
	case 7:
		if( crt.vertical_sync_pos == data )
			break;
		crt.vertical_sync_pos = data;
		logerror("6545 vertical sync pos       %d\n", data);
        m6545_offset_xy();
		break;
	case 8:
		crt.crt_mode = data;

		{
			logerror("6545 mode_w $%02X\n", data);
			logerror("     interlace          %d\n", data & 3);
			logerror("     addr mode          %d\n", (data >> 2) & 1);
			logerror("     refresh RAM        %s\n", ((data >> 3) & 1) ? "transparent" : "shared");
			logerror("     disp enb, skew     %d\n", (data >> 4) & 3);
			logerror("     pin 34             %s\n", ((data >> 6) & 1) ? "update strobe" : "RA4");
			logerror("     update read mode   %s\n", ((data >> 7) & 1) ? "interleaved" : "during h/v-blank");
        }
		break;
	case 9:
		data &= 15;
		if( crt.scan_lines == data )
			break;
		crt.scan_lines = data;
		logerror("6545 scanlines               %d\n", data);
        m6545_offset_xy();
		break;
	case 10:
		if( crt.cursor_top == data )
			break;
		crt.cursor_top = data;
		logerror("6545 cursor top              %d/$%02X\n", data & 31, data);
		addr = 256 * crt.cursor_address_hi + crt.cursor_address_lo;
        dirtybuffer[addr] = 1;
		break;
	case 11:
		if( crt.cursor_bottom == data )
			break;
		crt.cursor_bottom = data;
        logerror("6545 cursor bottom           %d/$%02X\n", data & 31, data);
		addr = 256 * crt.cursor_address_hi + crt.cursor_address_lo;
        dirtybuffer[addr] = 1;
		break;
	case 12:
		data &= 63;
		if( crt.screen_address_hi == data )
			break;
		update_all = 1;
		crt.screen_address_hi = data;
		logerror("6545 screen address hi       $%02X\n", data);
        break;
	case 13:
		if( crt.screen_address_lo == data )
			break;
		update_all = 1;
		crt.screen_address_lo = data;
		logerror("6545 screen address lo       $%02X\n", data);
        break;
	case 14:
		data &= 63;
		if( crt.cursor_address_hi == data )
			break;
		crt.cursor_address_hi = data;
		addr = 256 * crt.cursor_address_hi + crt.cursor_address_lo;
		dirtybuffer[addr] = 1;
		logerror("6545 cursor address hi       $%02X\n", data);
        break;
	case 15:
		if( crt.cursor_address_lo == data )
			break;
		crt.cursor_address_lo = data;
		addr = 256 * crt.cursor_address_hi + crt.cursor_address_lo;
		dirtybuffer[addr] = 1;
		logerror("6545 cursor address lo       $%02X\n", data);
        break;
	case 16:
		/* lpen hi is read only */
		break;
    case 17:
		/* lpen lo is read only */
        break;
    case 18:
		data &= 63;
		if( crt.transp_hi == data )
            break;
		crt.transp_hi = data;
        logerror("6545 transp_hi_w $%02X\n", data);
		break;
    case 19:
		if( crt.transp_lo == data )
            break;
		crt.transp_lo = data;
        logerror("6545 transp_lo_w $%02X\n", data);
		break;
	case 31:
		/* shared memory latch */
		addr = crt.transp_hi * 256 + crt.transp_lo;
        logerror("6545 transp_latch $%04X\n", addr);
		m6545_update_strobe(addr);
        break;
	default:
		logerror("6545 write unmapped port $%X <- $%02X\n", crt.idx, data);
    }
}

VIDEO_START( mbee )
{
    if( video_start_generic(machine) )
		return 1;
	videoram = auto_malloc(0x800);
	colorram = auto_malloc(0x800);
    memset(dirtybuffer, 1, videoram_size);

    return 0;
}

VIDEO_UPDATE( mbee )
{
	int offs, cursor, screen_;
	int full_refresh = 1;

	if( mbee_frame_counter > 0 )
	{
		if( --mbee_frame_counter == 0 )
			full_refresh = 1;
		else
		{
			popmessage("%s", mbee_frame_message);
		}
    }

    if( full_refresh )
	{
		memset(dirtybuffer, 1, videoram_size);
	}

	for( offs = 0x000; offs < 0x380; offs += 0x10 )
		keyboard_matrix_r(offs);

	framecnt++;

	cursor = crt.cursor_address_hi * 256 + crt.cursor_address_lo;
	screen_ = crt.screen_address_hi * 256 + crt.screen_address_lo;
	for( offs = screen_; offs < crt.horizontal_displayed * crt.vertical_displayed + screen_; offs++ )
	{
		if( dirtybuffer[offs] )
		{
			int sx, sy, code, color;
			sy = off_y + ((offs - screen_) / crt.horizontal_displayed) * (crt.scan_lines + 1);
			sx = (off_x + ((offs - screen_) % crt.horizontal_displayed)) * 8;
			code = videoram[offs];
			color = colorram[offs];
			drawgfx( bitmap,Machine->gfx[0],code,color,0,0,sx,sy,
				&Machine->screen[0].visarea,TRANSPARENCY_NONE,0);
			dirtybuffer[offs] = 0;
			if( offs == cursor && (crt.cursor_top & 0x60) != 0x20 )
			{
				if( (crt.cursor_top & 0x60) == 0x60 || (framecnt & 16) == 0 )
				{
					int x, y;
                    for( y = (crt.cursor_top & 31); y <= (crt.cursor_bottom & 31); y++ )
					{
						if( y > crt.scan_lines )
							break;
						for( x = 0; x < 8; x++ )
							plot_pixel(bitmap,sx+x,sy+y, Machine->pens[color]);
					}
					dirtybuffer[offs] = 1;
				}
            }
		}
	}
	return 0;
}




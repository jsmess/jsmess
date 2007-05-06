/**********************************************************************

    Motorola 6845 CRT Controller interface and emulation

    This function emulates the functionality of a single
    crtc6845.

**********************************************************************/

typedef struct _crtc6845_state crtc6845_state;
struct _crtc6845_state
{
	UINT8		address_latch;
	UINT8		horiz_total;
	UINT8		horiz_disp;
	UINT8		horiz_sync_pos;
	UINT8		sync_width;
	UINT8		vert_total;
	UINT8		vert_total_adj;
	UINT8		vert_disp;
	UINT8		vert_sync_pos;
	UINT8		intl_skew;
	UINT8		max_ras_addr;
	UINT8		cursor_start_ras;
	UINT8		cursor_end_ras;
	UINT16		start_addr;
	UINT16		cursor;
	UINT16		light_pen;
	UINT8		page_flip;
};

extern crtc6845_state crtc6845;

void crtc6845_init(void);
READ8_HANDLER( crtc6845_register_r );
WRITE8_HANDLER( crtc6845_address_w );
WRITE8_HANDLER( crtc6845_register_w );

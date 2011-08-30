/****************************************************f***********************

    Amiga floppy disk controller emulation

    - The drive spins at 300 RPM = 1 Rev every 200ms
    - At 2usec per bit, that's 12500 bytes per track, or 6250 words per track
    - At 4usec per bit, that's 6250 bytes per track, or 3125 words per track
    - Each standard amiga sector has 544 words per sector, and 11 sectors per track = 11968 bytes
    - ( 12500(max) - 11968(actual) ) = 532 per track gap bytes

***************************************************************************/


#include "emu.h"
#include "includes/amiga.h"
#include "imagedev/floppy.h"
#include "formats/ami_dsk.h"
#include "formats/hxcmfm_dsk.h"
#include "formats/mfi_dsk.h"
#include "amigafdc.h"
#include "machine/6526cia.h"

static floppy_format_type floppy_formats[] = {
	FLOPPY_ADF_FORMAT, FLOPPY_MFM_FORMAT, FLOPPY_MFI_FORMAT,
	NULL
};

#define MAX_TRACK_BYTES			12500
#define ACTUAL_TRACK_BYTES		11968
#define GAP_TRACK_BYTES			( MAX_TRACK_BYTES - ACTUAL_TRACK_BYTES )
#define ONE_SECTOR_BYTES		(544*2)
#define ONE_REV_TIME			200 /* ms */
#define MAX_WORDS_PER_DMA_CYCLE	32 /* 64 bytes per dma cycle */
#define DISK_DETECT_DELAY		1
#define MAX_TRACKS				160
#define MAX_MFM_TRACK_LEN		16384

#define NUM_DRIVES 2

/* required prototype */
static void setup_fdc_buffer( device_t *device,int drive );

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/
typedef struct {
	floppy_image_device *f;
	int tracklen;
	UINT8 *mfm;
	emu_timer *dma_timer;
	int pos;
	int len;
	offs_t ptr;
} fdc_def;

typedef struct _amiga_fdc_t amiga_fdc_t;
struct _amiga_fdc_t
{
	fdc_def fdc_status[NUM_DRIVES];

	/* signals */
	int fdc_sel;
	int fdc_rdy;
};

/*****************************************************************************
    INLINE FUNCTIONS
*****************************************************************************/
INLINE amiga_fdc_t *get_safe_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == AMIGA_FDC);

	return (amiga_fdc_t *)downcast<legacy_device_base *>(device)->token();
}

static TIMER_CALLBACK(fdc_dma_proc);

static int amiga_floppy_get_drive(device_t *image)
{
	int drive =0;
	if (strcmp(image->tag(), FLOPPY_0) == 0) drive = 0;
	if (strcmp(image->tag(), FLOPPY_1) == 0) drive = 1;
	return drive;
}

static int amiga_load_proc(device_image_interface &image)
{
	amiga_fdc_t *fdc = get_safe_token(image.device().owner());
	int id = amiga_floppy_get_drive(&image.device());
	fdc->fdc_status[id].dma_timer->reset(  );
	fdc->fdc_rdy = 0;

	return IMAGE_INIT_PASS;
}

static void amiga_unload_proc(device_image_interface &image)
{
	amiga_fdc_t *fdc = get_safe_token(image.device().owner());
	int id = amiga_floppy_get_drive(&image.device());
	fdc->fdc_status[id].dma_timer->reset(  );
	fdc->fdc_rdy = 0;
}

/*-------------------------------------------------
    DEVICE_START(amiga_fdc)
-------------------------------------------------*/

static DEVICE_START(amiga_fdc)
{
	amiga_fdc_t *fdc = get_safe_token(device);
	int id;
	for(id=0;id<2;id++)
	{
		fdc->fdc_status[id].dma_timer = device->machine().scheduler().timer_alloc(FUNC(fdc_dma_proc), (void*)device);
		fdc->fdc_status[id].pos = 0;
		fdc->fdc_status[id].len = 0;
		fdc->fdc_status[id].ptr = 0;
		if (id==0) {
			fdc->fdc_status[id].f =  downcast<floppy_image_device *>(device->subdevice(FLOPPY_0));
		} else {
			fdc->fdc_status[id].f =  downcast<floppy_image_device *>(device->subdevice(FLOPPY_1));
		}
		fdc->fdc_status[id].tracklen = MAX_TRACK_BYTES;
		fdc->fdc_status[id].dma_timer->reset(  );
	}
	fdc->fdc_sel = 0x0f;
	fdc->fdc_rdy = 0;

}


/*-------------------------------------------------
    DEVICE_RESET(amiga_fdc)
-------------------------------------------------*/

static DEVICE_RESET(amiga_fdc)
{
}

static int fdc_get_curpos( device_t *device, int drive )
{
	amiga_state *state = device->machine().driver_data<amiga_state>();
	double elapsed;
	int speed;
	int	bytes;
	int pos;
	amiga_fdc_t *fdc = get_safe_token(device);

	elapsed = fdc->fdc_status[drive].f->get_pos();
	speed = ( CUSTOM_REG(REG_ADKCON) & 0x100 ) ? 2 : 4;

	bytes = elapsed /  attotime::from_usec( speed * 8 ).as_double();
	pos = bytes % ( fdc->fdc_status[drive].tracklen );

	return pos;
}

UINT16 amiga_fdc_get_byte (device_t *device)
{
	amiga_state *state = device->machine().driver_data<amiga_state>();
	int pos;
	int i, drive = -1;
	UINT16 ret;
	amiga_fdc_t *fdc = get_safe_token(device);

	ret = ( ( CUSTOM_REG(REG_DSKLEN) >> 1 ) & 0x4000 ) & ( ( CUSTOM_REG(REG_DMACON) << 10 ) & 0x4000 );
	ret |= ( CUSTOM_REG(REG_DSKLEN) >> 1 ) & 0x2000;

	for ( i = 0; i < NUM_DRIVES; i++ ) {
		if ( !( fdc->fdc_sel & ( 1 << i ) ) )
			drive = i;
	}

	if ( drive == -1 )
		return ret;

	setup_fdc_buffer( device,drive );

	pos = fdc_get_curpos( device, drive );
	if ( fdc->fdc_status[drive].mfm[pos] == ( CUSTOM_REG(REG_DSRSYNC) >> 8 ) &&
		 fdc->fdc_status[drive].mfm[pos+1] == ( CUSTOM_REG(REG_DSRSYNC) & 0xff ) )
			ret |= 0x1000;

	if ( pos != fdc->fdc_status[drive].pos ) {
		ret |= 0x8000;
		fdc->fdc_status[drive].pos = pos;
	}

	ret |= fdc->fdc_status[drive].mfm[pos];

	return ret;
}

static TIMER_CALLBACK(fdc_dma_proc)
{
	amiga_state *state = machine.driver_data<amiga_state>();
	int drive = param;
	amiga_fdc_t *fdc = get_safe_token((device_t*)ptr);

	/* if DMA got disabled by the time we got here, stop operations */
	if ( ( CUSTOM_REG(REG_DSKLEN) & 0x8000 ) == 0 )
		goto bail;

	if ( ( CUSTOM_REG(REG_DMACON) & 0x0210 ) == 0 )
		goto bail;

	/* if floppy got ejected, also stop */
	if ( fdc->fdc_status[drive].f->ready_r()==1)
		goto bail;

	setup_fdc_buffer( (device_t*)ptr, drive );

	if ( CUSTOM_REG(REG_DSKLEN) & 0x4000 ) /* disk write case, unsupported yet */
	{
		if ( fdc->fdc_status[drive].len > 0 )
		{
			address_space *space = machine.device("maincpu")->memory().space(AS_PROGRAM);

			logerror("Write to disk unsupported yet\n" );

			amiga_custom_w(space, REG_INTREQ, 0x8000 | INTENA_DSKBLK, 0xffff);
		}
	}
	else
	{
		offs_t offset = fdc->fdc_status[drive].ptr;
		int cur_pos = fdc->fdc_status[drive].pos;
		int len = fdc->fdc_status[drive].len;

		if ( len > MAX_WORDS_PER_DMA_CYCLE ) len = MAX_WORDS_PER_DMA_CYCLE;

		fdc->fdc_status[drive].len -= len;

		while ( len-- )
		{
			int dat = ( fdc->fdc_status[drive].mfm[cur_pos++] ) << 8;

			cur_pos %= ( fdc->fdc_status[drive].tracklen );

			dat |= fdc->fdc_status[drive].mfm[cur_pos++];

			cur_pos %= ( fdc->fdc_status[drive].tracklen );

			(*state->m_chip_ram_w)(state, offset, dat);

			offset += 2;
		}

		fdc->fdc_status[drive].ptr = offset;
		fdc->fdc_status[drive].pos = cur_pos;

		if ( fdc->fdc_status[drive].len <= 0 )
		{
			address_space *space = machine.device("maincpu")->memory().space(AS_PROGRAM);

			amiga_custom_w(space, REG_INTREQ, 0x8000 | INTENA_DSKBLK, 0xffff);
		}
		else
		{
			int time;
			double	len_words = fdc->fdc_status[drive].len;
			if ( len_words > MAX_WORDS_PER_DMA_CYCLE ) len_words = MAX_WORDS_PER_DMA_CYCLE;

			time = len_words * 2;
			time *= ( CUSTOM_REG(REG_ADKCON) & 0x0100 ) ? 2 : 4;
			time *= 8;
			fdc->fdc_status[drive].dma_timer->adjust(attotime::from_usec( time ), drive);
			return;
		}
	}

bail:
	fdc->fdc_status[drive].dma_timer->reset(  );
}

void amiga_fdc_setup_dma( device_t *device ) {
	amiga_state *state = device->machine().driver_data<amiga_state>();
	int i, cur_pos, drive = -1, len_words = 0;
	int time = 0;
	amiga_fdc_t *fdc = get_safe_token(device);

	for ( i = 0; i < NUM_DRIVES; i++ ) {
		if ( !( fdc->fdc_sel & ( 1 << i ) ) )
			drive = i;
	}

	if ( drive == -1 ) {
		address_space *space = device->machine().device("maincpu")->memory().space(AS_PROGRAM);

		logerror("Disk DMA started with no drive selected!\n" );
		amiga_custom_w(space, REG_INTREQ, 0x8000 | INTENA_DSKBLK, 0xffff);
		return;
	}

	if ( ( CUSTOM_REG(REG_DSKLEN) & 0x8000 ) == 0 )
		goto bail;

	if ( ( CUSTOM_REG(REG_DMACON) & 0x0210 ) == 0 )
		goto bail;

	setup_fdc_buffer( device,drive );

	fdc->fdc_status[drive].len = CUSTOM_REG(REG_DSKLEN) & 0x3fff;
	fdc->fdc_status[drive].ptr = CUSTOM_REG_LONG(REG_DSKPTH);
	fdc->fdc_status[drive].pos = cur_pos = fdc_get_curpos( device, drive );

	/* we'll do a set amount at a time */
	len_words = fdc->fdc_status[drive].len;
	if ( len_words > MAX_WORDS_PER_DMA_CYCLE ) len_words = MAX_WORDS_PER_DMA_CYCLE;

	if ( CUSTOM_REG(REG_ADKCON) & 0x0400 ) { /* Wait for sync */
		if ( CUSTOM_REG(REG_DSRSYNC) != 0x4489 ) {
			logerror("Attempting to read a non-standard SYNC\n" );
		}

		i = cur_pos;
		do {
			if ( fdc->fdc_status[drive].mfm[i] == ( CUSTOM_REG(REG_DSRSYNC) >> 8 ) &&
				 fdc->fdc_status[drive].mfm[i+1] == ( CUSTOM_REG(REG_DSRSYNC) & 0xff ) )
					break;

			i++;
			i %= ( fdc->fdc_status[drive].tracklen );
			time++;
		} while( i != cur_pos );

		if ( i == cur_pos && time != 0 ) {
			logerror("SYNC not found on track!\n" );
			return;
		} else {
			fdc->fdc_status[drive].pos = i + 2;
			time += 2;
		}
	}

	time += len_words * 2;
	time *= ( CUSTOM_REG(REG_ADKCON) & 0x0100 ) ? 2 : 4;
	time *= 8;
	fdc->fdc_status[drive].dma_timer->adjust(attotime::from_usec( time ), drive);

	return;

bail:
	fdc->fdc_status[drive].dma_timer->reset(  );
}

static void setup_fdc_buffer( device_t *device,int drive )
{
	amiga_fdc_t *fdc = get_safe_token(device);
	fdc->fdc_status[drive].mfm = fdc->fdc_status[drive].f->get_buffer();
}


static void fdc_setup_leds(device_t *device, int drive ) {

	char portname[12];
	amiga_fdc_t *fdc = get_safe_token(device);
	sprintf(portname, "drive_%d_led", drive);
	int val = fdc->fdc_status[drive].f->ready_r() == 0 ? 0 : 1;
	output_set_value(portname, val);

	if ( drive == 0 )
		set_led_status(device->machine(), 1, val); /* update internal drive led */

	if ( drive == 1 )
		set_led_status(device->machine(), 2, val); /* update external drive led */
}

WRITE8_DEVICE_HANDLER( amiga_fdc_control_w )
{
	int drive;

	amiga_fdc_t *fdc = get_safe_token(device);

	if ( fdc->fdc_sel != ( ( data >> 3 ) & 15 ) )
		fdc->fdc_rdy = 0;
	fdc->fdc_sel = ( data >> 3 ) & 15;

	for ( drive = 0; drive < NUM_DRIVES; drive++ ) {
		if ( !( fdc->fdc_sel & ( 1 << drive ) ) ) {
			fdc->fdc_status[drive].f->ss_w(( data >> 2 ) & 1);
			fdc->fdc_status[drive].f->dir_w(( data >> 1 ) & 1);
			fdc->fdc_status[drive].f->stp_w(data & 1);

			fdc->fdc_status[drive].f->mon_w((( data >> 7 ) & 1)  );
			fdc_setup_leds( device,drive );
		}
    }
}

UINT8  amiga_fdc_status_r (device_t *device)
{
	int drive = -1, ret = 0x3c;
	amiga_fdc_t *fdc = get_safe_token(device);
	for ( drive = 0; drive < NUM_DRIVES; drive++ ) {
		if ( !( fdc->fdc_sel & ( 1 << drive ) ) ) {

			if ( fdc->fdc_status[drive].f->ready_r()==0) {
				if ( fdc->fdc_rdy ) ret &= ~0x20;
				fdc->fdc_rdy = 1;
			} else {
				/* return drive id */
				ret &= ~0x20;
			}

			if ( fdc->fdc_status[drive].f->trk00_r() == 0 )
				ret &= ~0x10;

			if ( fdc->fdc_status[drive].f->wpt_r()==0 )
				ret &= ~0x08;

			if ( fdc->fdc_status[drive].f->dskchg_r() == 0)
				ret &= ~0x04;

			return ret;
		}
	}

	return ret;
}

WRITE_LINE_DEVICE_HANDLER(amiga_index_callback)
{
	/* Issue a index pulse when a disk revolution completes */
	device_t *cia = device->machine().device("cia_1");
	mos6526_flag_w(cia, state);
}

static const floppy_interface amiga_floppy_interface =
{
	DEVCB_LINE(amiga_index_callback),
	floppy_formats,
	NULL,
	NULL,
	amiga_load_proc,
	amiga_unload_proc
};

static MACHINE_CONFIG_FRAGMENT( amiga_fdc )
	MCFG_FLOPPY_2_DRIVES_ADD(amiga_floppy_interface)
MACHINE_CONFIG_END


DEVICE_GET_INFO( amiga_fdc )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;												break;
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(amiga_fdc_t);								break;

		/* --- the following bits of info are returned as pointers --- */
		case DEVINFO_PTR_MACHINE_CONFIG:				info->machine_config = MACHINE_CONFIG_NAME(amiga_fdc);		break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(amiga_fdc);					break;
		case DEVINFO_FCT_STOP:							/* Nothing */												break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(amiga_fdc);					break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Amiga FDC");								break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "Amiga FDC");								break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");										break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);									break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright the MESS Team"); 				break;
	}
}

DEFINE_LEGACY_DEVICE(AMIGA_FDC, amiga_fdc);

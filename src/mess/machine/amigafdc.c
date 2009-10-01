/***************************************************************************

    Amiga floppy disk controller emulation

    - The drive spins at 300 RPM = 1 Rev every 200ms
    - At 2usec per bit, that's 12500 bytes per track, or 6250 words per track
    - At 4usec per bit, that's 6250 bytes per track, or 3125 words per track
    - Each standard amiga sector has 544 words per sector, and 11 sectors per track = 11968 bytes
    - ( 12500(max) - 11968(actual) ) = 532 per track gap bytes

***************************************************************************/


#include "driver.h"
#include "amiga.h"
#include "amigafdc.h"
#include "machine/6526cia.h"


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
static void setup_fdc_buffer( int drive );

typedef struct {
	int motor_on;
	int side;
	int dir;
	int wprot;
	int disk_changed;
	const device_config *f;
	UINT32 extinfo[MAX_TRACKS];
	UINT32 extoffs[MAX_TRACKS];
	int is_ext_image;
	int cyl;
	int tracklen;
	UINT8 mfm[MAX_MFM_TRACK_LEN];
	int	cached;
	emu_timer *rev_timer;
	emu_timer *sync_timer;
	emu_timer *dma_timer;
	int rev_timer_started;
	int pos;
	int len;
	offs_t ptr;
} fdc_def;

static fdc_def fdc_status[NUM_DRIVES];
/* signals */
static int fdc_sel = 0x0f;
static int fdc_dir = 0;
static int fdc_side = 1;
static int fdc_step = 1;
static int fdc_rdy = 0;

static TIMER_CALLBACK(fdc_rev_proc);
static TIMER_CALLBACK(fdc_dma_proc);
static TIMER_CALLBACK(fdc_sync_proc);

static DEVICE_START(amiga_fdc)
{
	int id = image_index_in_device(device);
	fdc_status[id].motor_on = 0;
	fdc_status[id].side = 0;
	fdc_status[id].dir = 0;
	fdc_status[id].wprot = 1;
	fdc_status[id].cyl = 0;
	fdc_status[id].rev_timer = timer_alloc(device->machine, fdc_rev_proc, NULL);
	fdc_status[id].dma_timer = timer_alloc(device->machine, fdc_dma_proc, NULL);
	fdc_status[id].sync_timer = timer_alloc(device->machine, fdc_sync_proc, NULL);
	fdc_status[id].rev_timer_started = 0;
	fdc_status[id].cached = -1;
	fdc_status[id].pos = 0;
	fdc_status[id].len = 0;
	fdc_status[id].ptr = 0;
	fdc_status[id].f = NULL;
	fdc_status[id].is_ext_image = 0;
	fdc_status[id].tracklen = MAX_TRACK_BYTES;
	fdc_status[id].disk_changed = DISK_DETECT_DELAY;

	memset( fdc_status[id].mfm, 0xaa, MAX_MFM_TRACK_LEN );

	fdc_sel = 0x0f;
	fdc_dir = 0;
	fdc_side = 1;
	fdc_step = 1;
	fdc_rdy = 0;
}

static void check_extended_image( int id )
{
	UINT8	header[8], data[4];

	fdc_status[id].is_ext_image = 0;

	if ( image_fseek( fdc_status[id].f, 0, SEEK_SET ) )
	{
		logerror("FDC: image_fseek failed!\n" );
		return;
	}

	image_fread( fdc_status[id].f, &header, sizeof( header ) );

	if ( memcmp( header, "UAE--ADF", sizeof( header ) ) == 0 )
	{
		int		i;

		for( i = 0; i < MAX_TRACKS; i++ )
		{
			image_fread( fdc_status[id].f, &data, sizeof( data ) );

			/* data[0,1] = SYNC - data[2,3] = LEN */

			fdc_status[id].extinfo[i] = data[0];
			fdc_status[id].extinfo[i] <<= 8;
			fdc_status[id].extinfo[i] |= data[1];
			fdc_status[id].extinfo[i] <<= 8;
			fdc_status[id].extinfo[i] |= data[2];
			fdc_status[id].extinfo[i] <<= 8;
			fdc_status[id].extinfo[i] |= data[3];

			if ( i == 0 )
				fdc_status[id].extoffs[i] = sizeof( header ) + ( MAX_TRACKS * sizeof( data ) );
			else
				fdc_status[id].extoffs[i] = fdc_status[id].extoffs[i-1] + ( fdc_status[id].extinfo[i-1] & 0xffff );
		}

		fdc_status[id].is_ext_image = 1;
	}
}

static DEVICE_IMAGE_LOAD(amiga_fdc)
{
	int id = image_index_in_device(image);

	fdc_status[id].disk_changed = DISK_DETECT_DELAY;
	fdc_status[id].f = image;
	fdc_status[id].cached = -1;
	fdc_status[id].motor_on = 0;
	fdc_status[id].rev_timer_started = 0;
	timer_reset( fdc_status[id].rev_timer, attotime_never );
	timer_reset( fdc_status[id].sync_timer, attotime_never );
	timer_reset( fdc_status[id].dma_timer, attotime_never );
	fdc_rdy = 0;

	check_extended_image( id );

	return INIT_PASS;
}

static DEVICE_IMAGE_UNLOAD(amiga_fdc)
{
	int id = image_index_in_device(image);

	fdc_status[id].disk_changed = DISK_DETECT_DELAY;
	fdc_status[id].f = NULL;
	fdc_status[id].cached = -1;
	fdc_status[id].motor_on = 0;
	fdc_status[id].rev_timer_started = 0;
	timer_reset( fdc_status[id].rev_timer, attotime_never );
	timer_reset( fdc_status[id].sync_timer, attotime_never );
	timer_reset( fdc_status[id].dma_timer, attotime_never );
	fdc_rdy = 0;
}

static int fdc_get_curpos( int drive )
{
	double elapsed;
	int speed;
	int	bytes;
	int pos;

	if ( fdc_status[drive].rev_timer_started == 0 ) {
		logerror("Rev timer not started on drive %d, cant get position!\n", drive );
		return 0;
	}

	elapsed = attotime_to_double(timer_timeelapsed( fdc_status[drive].rev_timer ));
	speed = ( CUSTOM_REG(REG_ADKCON) & 0x100 ) ? 2 : 4;

	bytes = elapsed / attotime_to_double( ATTOTIME_IN_USEC( speed * 8 ) );
	pos = bytes % ( fdc_status[drive].tracklen );

	return pos;
}

UINT16 amiga_fdc_get_byte( void ) {
	int pos;
	int i, drive = -1;
	UINT16 ret;

	ret = ( ( CUSTOM_REG(REG_DSKLEN) >> 1 ) & 0x4000 ) & ( ( CUSTOM_REG(REG_DMACON) << 10 ) & 0x4000 );
	ret |= ( CUSTOM_REG(REG_DSKLEN) >> 1 ) & 0x2000;

	for ( i = 0; i < NUM_DRIVES; i++ ) {
		if ( !( fdc_sel & ( 1 << i ) ) )
			drive = i;
	}

	if ( drive == -1 )
		return ret;

	if ( fdc_status[drive].disk_changed )
		return ret;

	setup_fdc_buffer( drive );

	pos = fdc_get_curpos( drive );

	if ( fdc_status[drive].mfm[pos] == ( CUSTOM_REG(REG_DSRSYNC) >> 8 ) &&
		 fdc_status[drive].mfm[pos+1] == ( CUSTOM_REG(REG_DSRSYNC) & 0xff ) )
			ret |= 0x1000;

	if ( pos != fdc_status[drive].pos ) {
		ret |= 0x8000;
		fdc_status[drive].pos = pos;
	}

	ret |= fdc_status[drive].mfm[pos];

	return ret;
}

static TIMER_CALLBACK(fdc_sync_proc)
{
	int drive = param;
	UINT16			sync = CUSTOM_REG(REG_DSRSYNC);
	int				cur_pos;
	int				sector;
	int				time;

	/* if floppy got ejected, stop */
	if ( fdc_status[drive].disk_changed )
		goto bail;

	if ( fdc_status[drive].motor_on == 0 )
		goto bail;

	cur_pos = fdc_get_curpos( drive );

	if ( cur_pos <= ( GAP_TRACK_BYTES + 6 ) )
	{
		sector = 0;
	}
	else
	{
		sector = ( cur_pos - ( GAP_TRACK_BYTES + 6 ) ) / ONE_SECTOR_BYTES;
	}

	setup_fdc_buffer( drive );

	if ( cur_pos < 2 )
		cur_pos = 2;

	if ( fdc_status[drive].mfm[cur_pos-2] == ( ( sync >> 8 ) & 0xff ) &&
		 fdc_status[drive].mfm[cur_pos-1] == ( sync & 0xff ) )
	{
		const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);

		amiga_custom_w(space, REG_INTREQ, 0x8000 | INTENA_DSKSYN, 0xffff);
	}

	if ( sector < 10 )
	{
		/* start the sync timer */
		time = ONE_SECTOR_BYTES;
		time *= ( CUSTOM_REG(REG_ADKCON) & 0x0100 ) ? 2 : 4;
		time *= 8;
		timer_adjust_oneshot(fdc_status[drive].sync_timer, ATTOTIME_IN_USEC( time ), drive);
		return;
	}

bail:
	timer_reset( fdc_status[drive].sync_timer, attotime_never );
}

static TIMER_CALLBACK(fdc_dma_proc)
{
	int drive = param;
	/* if DMA got disabled by the time we got here, stop operations */
	if ( ( CUSTOM_REG(REG_DSKLEN) & 0x8000 ) == 0 )
		goto bail;

	if ( ( CUSTOM_REG(REG_DMACON) & 0x0210 ) == 0 )
		goto bail;

	/* if floppy got ejected, also stop */
	if ( fdc_status[drive].disk_changed )
		goto bail;

	if ( fdc_status[drive].motor_on == 0 )
		goto bail;

	setup_fdc_buffer( drive );

	if ( CUSTOM_REG(REG_DSKLEN) & 0x4000 ) /* disk write case, unsupported yet */
	{
		if ( fdc_status[drive].len > 0 )
		{
			const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);

			logerror("Write to disk unsupported yet\n" );

			amiga_custom_w(space, REG_INTREQ, 0x8000 | INTENA_DSKBLK, 0xffff);
		}
	}
	else
	{
		offs_t offset = fdc_status[drive].ptr;
		int cur_pos = fdc_status[drive].pos;
		int len = fdc_status[drive].len;

		if ( len > MAX_WORDS_PER_DMA_CYCLE ) len = MAX_WORDS_PER_DMA_CYCLE;

		fdc_status[drive].len -= len;

		while ( len-- )
		{
			int dat = ( fdc_status[drive].mfm[cur_pos++] ) << 8;

			cur_pos %= ( fdc_status[drive].tracklen );

			dat |= fdc_status[drive].mfm[cur_pos++];

			cur_pos %= ( fdc_status[drive].tracklen );

			amiga_chip_ram_w(offset, dat);

			offset += 2;
		}

		fdc_status[drive].ptr = offset;
		fdc_status[drive].pos = cur_pos;

		if ( fdc_status[drive].len <= 0 )
		{
			const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);

			amiga_custom_w(space, REG_INTREQ, 0x8000 | INTENA_DSKBLK, 0xffff);
		}
		else
		{
			int time;
			double	len_words = fdc_status[drive].len;
			if ( len_words > MAX_WORDS_PER_DMA_CYCLE ) len_words = MAX_WORDS_PER_DMA_CYCLE;

			time = len_words * 2;
			time *= ( CUSTOM_REG(REG_ADKCON) & 0x0100 ) ? 2 : 4;
			time *= 8;
			timer_adjust_oneshot(fdc_status[drive].dma_timer, ATTOTIME_IN_USEC( time ), drive);
			return;
		}
	}

bail:
	timer_reset( fdc_status[drive].dma_timer, attotime_never );
}

void amiga_fdc_setup_dma( running_machine *machine ) {
	int i, cur_pos, drive = -1, len_words = 0;
	int time = 0;

	for ( i = 0; i < NUM_DRIVES; i++ ) {
		if ( !( fdc_sel & ( 1 << i ) ) )
			drive = i;
	}

	if ( drive == -1 ) {
		const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);

		logerror("Disk DMA started with no drive selected!\n" );
		amiga_custom_w(space, REG_INTREQ, 0x8000 | INTENA_DSKBLK, 0xffff);
		return;
	}

	if ( ( CUSTOM_REG(REG_DSKLEN) & 0x8000 ) == 0 )
		goto bail;

	if ( ( CUSTOM_REG(REG_DMACON) & 0x0210 ) == 0 )
		goto bail;

	if ( fdc_status[drive].disk_changed )
		goto bail;

	setup_fdc_buffer( drive );

	fdc_status[drive].len = CUSTOM_REG(REG_DSKLEN) & 0x3fff;
	fdc_status[drive].ptr = CUSTOM_REG_LONG(REG_DSKPTH);
	fdc_status[drive].pos = cur_pos = fdc_get_curpos( drive );

	/* we'll do a set amount at a time */
	len_words = fdc_status[drive].len;
	if ( len_words > MAX_WORDS_PER_DMA_CYCLE ) len_words = MAX_WORDS_PER_DMA_CYCLE;

	if ( CUSTOM_REG(REG_ADKCON) & 0x0400 ) { /* Wait for sync */
		if ( CUSTOM_REG(REG_DSRSYNC) != 0x4489 ) {
			logerror("Attempting to read a non-standard SYNC\n" );
		}

		i = cur_pos;
		do {
			if ( fdc_status[drive].mfm[i] == ( CUSTOM_REG(REG_DSRSYNC) >> 8 ) &&
				 fdc_status[drive].mfm[i+1] == ( CUSTOM_REG(REG_DSRSYNC) & 0xff ) )
				 	break;

			i++;
			i %= ( fdc_status[drive].tracklen );
			time++;
		} while( i != cur_pos );

		if ( i == cur_pos && time != 0 ) {
			logerror("SYNC not found on track!\n" );
			return;
		} else {
			fdc_status[drive].pos = i + 2;
			time += 2;
		}
	}

	time += len_words * 2;
	time *= ( CUSTOM_REG(REG_ADKCON) & 0x0100 ) ? 2 : 4;
	time *= 8;
	timer_adjust_oneshot(fdc_status[drive].dma_timer, ATTOTIME_IN_USEC( time ), drive);

	return;

bail:
	timer_reset( fdc_status[drive].dma_timer, attotime_never );
}

static void setup_fdc_buffer( int drive )
{
	int		sector, offset, len;
	UINT8	temp_cyl[512*11];

	/* no disk in drive */
	if ( fdc_status[drive].f == NULL ) {
		memset( &fdc_status[drive].mfm[0], 0xaa, 32 );
		fdc_status[drive].disk_changed = DISK_DETECT_DELAY;
		return;
	}

	len = 512*11;

	offset = ( fdc_status[drive].cyl << 1 ) | fdc_side;

	if ( fdc_status[drive].cached == offset )
		return;

#if 0
	popmessage("T:%d S:%d", fdc_status[drive].cyl, fdc_side );
#endif

	if ( fdc_status[drive].is_ext_image )
	{
		len = fdc_status[drive].extinfo[offset] & 0xffff;

		if ( len > ( MAX_MFM_TRACK_LEN - 2 ) )
			len = MAX_MFM_TRACK_LEN - 2;

		if ( image_fseek( fdc_status[drive].f, fdc_status[drive].extoffs[offset], SEEK_SET ) )
		{
			logerror("FDC: image_fseek failed!\n" );
			fdc_status[drive].f = NULL;
			fdc_status[drive].disk_changed = DISK_DETECT_DELAY;
			return;
		}

		/* if SYNC is 0000, then its a regular amiga dos track image */
		if ( ( fdc_status[drive].extinfo[offset] >> 16 ) == 0x0000 )
		{
			image_fread( fdc_status[drive].f, temp_cyl, len );
			/* fall through to regular load */
		}
		else /* otherwise, it's a raw track */
		{
			fdc_status[drive].mfm[0] = ( fdc_status[drive].extinfo[offset] >> 24 ) & 0xff;
			fdc_status[drive].mfm[1] = ( fdc_status[drive].extinfo[offset] >> 16 ) & 0xff;
			image_fread( fdc_status[drive].f, &fdc_status[drive].mfm[2], len );
			fdc_status[drive].tracklen = len + 2;
			fdc_status[drive].cached = offset;
			return;
		}
	}
	else
	{
		if ( image_fseek( fdc_status[drive].f, offset * len, SEEK_SET ) )
		{
			logerror("FDC: image_fseek failed!\n" );
			fdc_status[drive].f = NULL;
			fdc_status[drive].disk_changed = DISK_DETECT_DELAY;
			return;
		}

		image_fread( fdc_status[drive].f, temp_cyl, len );
	}

	fdc_status[drive].tracklen = MAX_TRACK_BYTES;
	memset( &fdc_status[drive].mfm[0], 0xaa, GAP_TRACK_BYTES );

	for ( sector = 0; sector < 11; sector++ ) {
		int x;
	    UINT8 *dest = ( &fdc_status[drive].mfm[(ONE_SECTOR_BYTES*sector)+GAP_TRACK_BYTES] );
	    UINT8 *src = &temp_cyl[sector*512];
		UINT32 tmp;
		UINT32 even, odd;
		UINT32 hck = 0, dck = 0;

		/* Preamble and sync */
		*(dest + 0) = 0xaa;
		*(dest + 1) = 0xaa;
		*(dest + 2) = 0xaa;
		*(dest + 3) = 0xaa;
		*(dest + 4) = 0x44;
		*(dest + 5) = 0x89;
		*(dest + 6) = 0x44;
		*(dest + 7) = 0x89;

		/* Track and sector info */

		tmp = 0xff000000 | (offset<<16) | (sector<<8) | (11 - sector);
		odd = (tmp & 0x55555555) | 0xaaaaaaaa;
		even = ( ( tmp >> 1 ) & 0x55555555 ) | 0xaaaaaaaa;
		*(dest +  8) = (UINT8) ((even & 0xff000000)>>24);
		*(dest +  9) = (UINT8) ((even & 0xff0000)>>16);
		*(dest + 10) = (UINT8) ((even & 0xff00)>>8);
		*(dest + 11) = (UINT8) ((even & 0xff));
		*(dest + 12) = (UINT8) ((odd & 0xff000000)>>24);
		*(dest + 13) = (UINT8) ((odd & 0xff0000)>>16);
		*(dest + 14) = (UINT8) ((odd & 0xff00)>>8);
		*(dest + 15) = (UINT8) ((odd & 0xff));

		/* Fill unused space */

		for (x = 16 ; x < 48; x++)
			*(dest + x) = 0xaa;

		/* Encode data section of sector */

		for (x = 64 ; x < 576; x++)
		{
			tmp = *(src + x - 64);
			odd = (tmp & 0x55);
			even = (tmp>>1) & 0x55;
			*(dest + x) = (UINT8) (even | 0xaa);
			*(dest + x + 512) = (UINT8) (odd | 0xaa);
		}

		/* Calculate checksum for unused space */

		for(x = 8; x < 48; x += 4)
		    hck ^= (((UINT32) *(dest + x))<<24) | (((UINT32) *(dest + x + 1))<<16) |
		    	   (((UINT32) *(dest + x + 2))<<8) | ((UINT32) *(dest + x + 3));

		even = odd = hck;
		odd >>= 1;
		even |= 0xaaaaaaaa;
		odd |= 0xaaaaaaaa;

		*(dest + 48) = (UINT8) ((odd & 0xff000000)>>24);
		*(dest + 49) = (UINT8) ((odd & 0xff0000)>>16);
		*(dest + 50) = (UINT8) ((odd & 0xff00)>>8);
		*(dest + 51) = (UINT8) (odd & 0xff);
		*(dest + 52) = (UINT8) ((even & 0xff000000)>>24);
		*(dest + 53) = (UINT8) ((even & 0xff0000)>>16);
		*(dest + 54) = (UINT8) ((even & 0xff00)>>8);
		*(dest + 55) = (UINT8) (even & 0xff);

		/* Calculate checksum for data section */

		for(x = 64; x < 1088; x += 4)
			dck ^= (((UINT32) *(dest + x))<<24) | (((UINT32) *(dest + x + 1))<<16) |
				   (((UINT32) *(dest + x + 2))<< 8) |  ((UINT32) *(dest + x + 3));
		even = odd = dck;
		odd >>= 1;
		even |= 0xaaaaaaaa;
		odd |= 0xaaaaaaaa;
		*(dest + 56) = (UINT8) ((odd & 0xff000000)>>24);
		*(dest + 57) = (UINT8) ((odd & 0xff0000)>>16);
		*(dest + 58) = (UINT8) ((odd & 0xff00)>>8);
		*(dest + 59) = (UINT8) (odd & 0xff);
		*(dest + 60) = (UINT8) ((even & 0xff000000)>>24);
		*(dest + 61) = (UINT8) ((even & 0xff0000)>>16);
		*(dest + 62) = (UINT8) ((even & 0xff00)>>8);
		*(dest + 63) = (UINT8) (even & 0xff);
	}

	fdc_status[drive].cached = offset;
}

static TIMER_CALLBACK(fdc_rev_proc)
{
	int drive = param;
	int time;
	const device_config *cia;

	/* Issue a index pulse when a disk revolution completes */
	cia = devtag_get_device(machine, "cia_1");
	cia_issue_index(cia);

	timer_adjust_oneshot(fdc_status[drive].rev_timer, ATTOTIME_IN_MSEC( ONE_REV_TIME ), drive);
	fdc_status[drive].rev_timer_started = 1;

	if ( fdc_status[drive].is_ext_image == 0 )
	{
		/* also start the sync timer */
		time = GAP_TRACK_BYTES + 6;
		time *= ( CUSTOM_REG(REG_ADKCON) & 0x0100 ) ? 2 : 4;
		time *= 8;
		timer_adjust_oneshot(fdc_status[drive].sync_timer, ATTOTIME_IN_USEC( time ), drive);
	}
}

static void start_rev_timer( int drive ) {
//  double time;

	if ( fdc_status[drive].rev_timer_started ) {
		logerror("Revolution timer started twice?!\n" );
		return;
	}

	timer_adjust_oneshot(fdc_status[drive].rev_timer, ATTOTIME_IN_MSEC( ONE_REV_TIME ), drive);
	fdc_status[drive].rev_timer_started = 1;
}

static void stop_rev_timer( int drive ) {
	if ( fdc_status[drive].rev_timer_started == 0 ) {
		logerror("Revolution timer never started?!\n" );
		return;
	}

	timer_reset( fdc_status[drive].rev_timer, attotime_never );
	fdc_status[drive].rev_timer_started = 0;
	timer_reset( fdc_status[drive].dma_timer, attotime_never );
	timer_reset( fdc_status[drive].sync_timer, attotime_never );
}

static void fdc_setup_leds( int drive ) {

	char portname[12];
	sprintf(portname, "drive_%d_led", drive);
	output_set_value(portname, fdc_status[drive].motor_on == 0 ? 0 : 1);

	if ( drive == 0 )
		set_led_status( 1, fdc_status[drive].motor_on ); /* update internal drive led */

	if ( drive == 1 )
		set_led_status( 2, fdc_status[drive].motor_on ); /* update external drive led */
}

static void fdc_stepdrive( int drive ) {
	if ( fdc_dir ) {
		if ( fdc_status[drive].cyl )
			fdc_status[drive].cyl--;
	} else {
		if ( fdc_status[drive].cyl < 79 )
			fdc_status[drive].cyl++;
	}

	/* Update disk detection if applicable */
	if ( fdc_status[drive].f != NULL )
	{
		if ( fdc_status[drive].disk_changed > 0 )
			fdc_status[drive].disk_changed--;
	}
}

static void fdc_motor( int drive, int off ) {
	int on = !off;

	if ( ( fdc_status[drive].motor_on == 0 ) && on ) {
		fdc_status[drive].pos = 0;

		start_rev_timer( drive );

	} else {
		if ( fdc_status[drive].motor_on && off )
			stop_rev_timer( drive );
	}

	fdc_status[drive].motor_on = on;
}

WRITE8_DEVICE_HANDLER( amiga_fdc_control_w )
{
	int step_pulse;
	int drive;

	if ( fdc_sel != ( ( data >> 3 ) & 15 ) )
		fdc_rdy = 0;

	fdc_sel = ( data >> 3 ) & 15;
    fdc_side = 1 - ( ( data >> 2 ) & 1 );
	fdc_dir = ( data >> 1 ) & 1;

	step_pulse = data & 1;

    if ( fdc_step != step_pulse ) {
		fdc_step = step_pulse;

    	if ( fdc_step == 0 ) {
		    for ( drive = 0; drive < NUM_DRIVES; drive++ ) {
				if ( !( fdc_sel & ( 1 << drive ) ) )
				    fdc_stepdrive( drive );
			}
		}
	}

	for ( drive = 0; drive < NUM_DRIVES; drive++ ) {
		if ( !( fdc_sel & ( 1 << drive ) ) ) {
			fdc_motor( drive, ( data >> 7 ) & 1 );
			fdc_setup_leds( drive );
		}
    }
}

int amiga_fdc_status_r( void ) {
	int drive = -1, ret = 0x3c;

	for ( drive = 0; drive < NUM_DRIVES; drive++ ) {
		if ( !( fdc_sel & ( 1 << drive ) ) ) {

			if ( fdc_status[drive].motor_on ) {
				if ( fdc_rdy ) ret &= ~0x20;
				fdc_rdy = 1;
			} else {
				/* return drive id */
				ret &= ~0x20;
			}

			if ( fdc_status[drive].cyl == 0 )
				ret &= ~0x10;

			if ( fdc_status[drive].wprot )
				ret &= ~0x08;

			if ( fdc_status[drive].disk_changed )
				ret &= ~0x04;

			return ret;
		}
	}

	return ret;
}



void amiga_floppy_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_TYPE:					info->i = IO_FLOPPY; break;
		case MESS_DEVINFO_INT_COUNT:					info->i = NUM_DRIVES; break;
		case MESS_DEVINFO_INT_READABLE:				info->i = 1; break;
		case MESS_DEVINFO_INT_WRITEABLE:				info->i = 0; break;
		case MESS_DEVINFO_INT_CREATABLE:				info->i = 0; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_START:					info->start = DEVICE_START_NAME(amiga_fdc); break;
		case MESS_DEVINFO_PTR_LOAD:					info->load = DEVICE_IMAGE_LOAD_NAME(amiga_fdc); break;
		case MESS_DEVINFO_PTR_UNLOAD:				info->unload = DEVICE_IMAGE_UNLOAD_NAME(amiga_fdc); break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:		strcpy(info->s = device_temp_str(), "adf"); break;
	}
}

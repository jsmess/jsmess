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
#include "formats/ami_dsk.h"
#include "formats/hxcmfm_dsk.h"
#include "formats/mfi_dsk.h"
#include "amigafdc.h"
#include "machine/6526cia.h"

const device_type AMIGA_FDC = &device_creator<amiga_fdc>;

const floppy_format_type amiga_fdc::floppy_formats[] = {
	FLOPPY_ADF_FORMAT, FLOPPY_MFM_FORMAT, FLOPPY_MFI_FORMAT,
	NULL
};


amiga_fdc::amiga_fdc(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
	device_t(mconfig, AMIGA_FDC, "Amiga FDC", tag, owner, clock)
{
}

int amiga_fdc::get_drive(floppy_image_device *floppy)
{
	for(int id=0; id< NUM_DRIVES; id++)
		if(floppy == fdc_status[id].f)
			return id;
	return -1;
}

int amiga_fdc::load_proc(floppy_image_device *floppy)
{
	int id = get_drive(floppy);
	fdc_status[id].dma_timer->reset();
	fdc_rdy = 0;

	return IMAGE_INIT_PASS;
}

void amiga_fdc::unload_proc(floppy_image_device *floppy)
{
	int id = get_drive(floppy);
	fdc_status[id].dma_timer->reset();
	fdc_rdy = 0;
}

void amiga_fdc::device_start()
{
	for(int id=0; id< NUM_DRIVES; id++)
	{
		fdc_status[id].dma_timer = timer_alloc(id);
		fdc_status[id].pos = 0;
		fdc_status[id].len = 0;
		fdc_status[id].ptr = 0;
		fdc_status[id].f = downcast<floppy_image_device *>(subdevice(id ? FLOPPY_1 : FLOPPY_0));
		fdc_status[id].f->setup_load_cb(floppy_image_device::load_cb(FUNC(amiga_fdc::load_proc), this));
		fdc_status[id].f->setup_unload_cb(floppy_image_device::unload_cb(FUNC(amiga_fdc::unload_proc), this));
		fdc_status[id].f->setup_index_pulse_cb(floppy_image_device::index_pulse_cb(FUNC(amiga_fdc::index_callback), this));
		fdc_status[id].tracklen = MAX_TRACK_BYTES;
	}
	fdc_sel = 0x0f;
	fdc_rdy = 0;
}


void amiga_fdc::device_reset()
{
}

int amiga_fdc::get_curpos(int drive)
{
	amiga_state *state = machine().driver_data<amiga_state>();
	double elapsed;
	int speed;
	int	bytes;
	int pos;

	elapsed = fdc_status[drive].f->get_pos();
	speed = ( CUSTOM_REG(REG_ADKCON) & 0x100 ) ? 2 : 4;

	bytes = elapsed /  attotime::from_usec( speed * 8 ).as_double();
	pos = bytes % ( fdc_status[drive].tracklen );

	return pos;
}

UINT16 amiga_fdc::get_byte()
{
	amiga_state *state = machine().driver_data<amiga_state>();
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

	setup_fdc_buffer(drive);

	pos = get_curpos(drive);
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

void amiga_fdc::device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr)
{

	amiga_state *state = machine().driver_data<amiga_state>();
	int drive = id;

	/* if DMA got disabled by the time we got here, stop operations */
	if ( ( CUSTOM_REG(REG_DSKLEN) & 0x8000 ) == 0 )
		goto bail;

	if ( ( CUSTOM_REG(REG_DMACON) & 0x0210 ) == 0 )
		goto bail;

	/* if floppy got ejected, also stop */
	if ( fdc_status[drive].f->ready_r()==1)
		goto bail;

	setup_fdc_buffer(drive);

	if ( CUSTOM_REG(REG_DSKLEN) & 0x4000 ) /* disk write case, unsupported yet */
	{
		if ( fdc_status[drive].len > 0 )
		{
			address_space *space = machine().device("maincpu")->memory().space(AS_PROGRAM);

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

			(*state->m_chip_ram_w)(state, offset, dat);

			offset += 2;
		}

		fdc_status[drive].ptr = offset;
		fdc_status[drive].pos = cur_pos;

		if ( fdc_status[drive].len <= 0 )
		{
			address_space *space = machine().device("maincpu")->memory().space(AS_PROGRAM);

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
			fdc_status[drive].dma_timer->adjust(attotime::from_usec( time ));
			return;
		}
	}

bail:
	fdc_status[drive].dma_timer->reset();
}

void amiga_fdc::setup_dma()
{
	amiga_state *state = machine().driver_data<amiga_state>();
	int i, cur_pos, drive = -1, len_words = 0;
	int time = 0;

	for ( i = 0; i < NUM_DRIVES; i++ ) {
		if ( !( fdc_sel & ( 1 << i ) ) )
			drive = i;
	}

	if ( drive == -1 ) {
		address_space *space = machine().device("maincpu")->memory().space(AS_PROGRAM);

		logerror("Disk DMA started with no drive selected!\n" );
		amiga_custom_w(space, REG_INTREQ, 0x8000 | INTENA_DSKBLK, 0xffff);
		return;
	}

	if ( ( CUSTOM_REG(REG_DSKLEN) & 0x8000 ) == 0 )
		goto bail;

	if ( ( CUSTOM_REG(REG_DMACON) & 0x0210 ) == 0 )
		goto bail;

	setup_fdc_buffer(drive);

	fdc_status[drive].len = CUSTOM_REG(REG_DSKLEN) & 0x3fff;
	fdc_status[drive].ptr = CUSTOM_REG_LONG(REG_DSKPTH);
	fdc_status[drive].pos = cur_pos = get_curpos(drive);

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
	fdc_status[drive].dma_timer->adjust(attotime::from_usec(time));

	return;

bail:
	fdc_status[drive].dma_timer->reset();
}

void amiga_fdc::setup_fdc_buffer(int drive)
{
	fdc_status[drive].mfm = fdc_status[drive].f->get_buffer();
}


void amiga_fdc::setup_leds(int drive)
{
	char portname[12];
	sprintf(portname, "drive_%d_led", drive);
	int val = fdc_status[drive].f->ready_r() == 0 ? 0 : 1;
	output_set_value(portname, val);

	if ( drive == 0 )
		set_led_status(machine(), 1, val); /* update internal drive led */

	if ( drive == 1 )
		set_led_status(machine(), 2, val); /* update external drive led */
}

WRITE8_MEMBER( amiga_fdc::control_w )
{
	int drive;

	if ( fdc_sel != ( ( data >> 3 ) & 15 ) )
		fdc_rdy = 0;
	fdc_sel = ( data >> 3 ) & 15;

	for ( drive = 0; drive < NUM_DRIVES; drive++ ) {
		if ( !( fdc_sel & ( 1 << drive ) ) ) {
			fdc_status[drive].f->ss_w(( data >> 2 ) & 1);
			fdc_status[drive].f->dir_w(( data >> 1 ) & 1);
			fdc_status[drive].f->stp_w(data & 1);

			fdc_status[drive].f->mon_w((( data >> 7 ) & 1)  );
			setup_leds(drive);
		}
    }
}

UINT8 amiga_fdc::status_r()
{
	int drive = -1, ret = 0x3c;
	for ( drive = 0; drive < NUM_DRIVES; drive++ ) {
		if ( !(fdc_sel & ( 1 << drive ) ) ) {

			if ( fdc_status[drive].f->ready_r()==0) {
				if ( fdc_rdy ) ret &= ~0x20;
				fdc_rdy = 1;
			} else {
				/* return drive id */
				ret &= ~0x20;
			}

			if ( fdc_status[drive].f->trk00_r() == 0 )
				ret &= ~0x10;

			if ( fdc_status[drive].f->wpt_r()==0 )
				ret &= ~0x08;

			if ( fdc_status[drive].f->dskchg_r() == 0)
				ret &= ~0x04;

			return ret;
		}
	}

	return ret;
}

void amiga_fdc::index_callback(floppy_image_device *floppy, int state)
{
	/* Issue a index pulse when a disk revolution completes */
	device_t *cia = machine().device("cia_1");
	mos6526_flag_w(cia, state);
}

const floppy_interface amiga_fdc::amiga_floppy_interface =
{
	DEVCB_NULL,
	floppy_formats,
	NULL,
	NULL,
	NULL,
	NULL
};

static MACHINE_CONFIG_FRAGMENT( amiga_fdc )
	MCFG_FLOPPY_2_DRIVES_ADD(amiga_fdc::amiga_floppy_interface)
MACHINE_CONFIG_END

machine_config_constructor amiga_fdc::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME(amiga_fdc);
}

#ifndef AMIGAFDC_H
#define AMIGAFDC_H

#include "emu.h"
#include "imagedev/floppy.h"

#define MCFG_AMIGA_FDC_ADD(_tag)	\
	MCFG_DEVICE_ADD(_tag,  AMIGA_FDC, 0)

class amiga_fdc : public device_t {
public:
	amiga_fdc(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	DECLARE_WRITE8_MEMBER( control_w );

	UINT8 status_r();
	UINT16 get_byte();
	void setup_dma();

	static const floppy_interface amiga_floppy_interface;

protected:
    virtual void device_start();
    virtual void device_reset();
	virtual void device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr);
	virtual machine_config_constructor device_mconfig_additions() const;

private:
	enum {
		MAX_TRACK_BYTES         = 12500,
		ACTUAL_TRACK_BYTES      = 11968,
		GAP_TRACK_BYTES         = MAX_TRACK_BYTES - ACTUAL_TRACK_BYTES,
		ONE_SECTOR_BYTES        = 544*2,
		ONE_REV_TIME            = 200, /* ms */
		MAX_WORDS_PER_DMA_CYCLE = 32, /* 64 bytes per dma cycle */
		DISK_DETECT_DELAY       = 1,
		MAX_TRACKS              = 160,
		MAX_MFM_TRACK_LEN       = 16384,
		NUM_DRIVES              = 2
	};

	static const floppy_format_type floppy_formats[];

	struct fdc_def {
		floppy_image_device *f;
		int tracklen;
		UINT8 mfm[MAX_MFM_TRACK_LEN];
		emu_timer *dma_timer;
		int pos;
		int len;
		offs_t ptr;
	};

	fdc_def fdc_status[NUM_DRIVES];

	/* signals */
	int fdc_sel;
	int fdc_rdy;

	void setup_fdc_buffer(int drive);
	void setup_leds(int drive);
	int get_drive(floppy_image_device *floppy);
	int get_curpos(int drive);

	int load_proc(floppy_image_device *floppy);
	void unload_proc(floppy_image_device *floppy);
	void index_callback(floppy_image_device *floppy, int state);
	void advance(const UINT32 *trackbuf, UINT32 &cur_cell, UINT32 cell_count, UINT32 time);
	UINT32 get_next_edge(const UINT32 *trackbuf, UINT32 cur_cell, UINT32 cell_count);
};
	
extern const device_type AMIGA_FDC;

#endif /* AMIGAFDC_H */

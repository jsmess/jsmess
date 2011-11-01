#ifndef WD1772_H
#define WD1772_H

#include "emu.h"
#include "imagedev/floppy.h"

#define MCFG_WD1772x_ADD(_tag, _clock)	\
	MCFG_DEVICE_ADD(_tag, WD1772x, _clock)

class wd1772_t : public device_t {
public:
	typedef delegate<void (bool state)> line_cb;

	wd1772_t(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	void dden_w(bool dden);
	void set_floppy(floppy_image_device *floppy);
	void setup_intrq_cb(line_cb cb);
	void setup_drq_cb(line_cb cb);

	void cmd_w(UINT8 val);
	UINT8 status_r();

	void track_w(UINT8 val);
	UINT8 track_r();

	void sector_w(UINT8 val);
	UINT8 sector_r();

	void data_w(UINT8 val);
	UINT8 data_r();

	void gen_w(int reg, UINT8 val);
	UINT8 gen_r(int reg);

	bool intrq_r();
	bool drq_r();

	DECLARE_READ8_MEMBER( read ) { return gen_r(offset);}
	DECLARE_WRITE8_MEMBER( write ) { gen_w(offset,data); }

protected:
	virtual void device_start();
	virtual void device_reset();
	virtual void device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr);

private:
	enum { TM_GEN, TM_CMD, TM_TRACK, TM_SECTOR };

	//  State machine general behaviour:
	//
	//  There are three levels of state.
	//
	//  Main state is associated to (groups of) commands.  They're set
	//  by a *_start() function below, and the associated _continue()
	//  function can then be called at pretty much any time.
	//
	//  Sub state is the state of execution within a command.  The
	//  principle is that the *_start() function selects the initial
	//  substate, then the *_continue() function decides what to do,
	//  possibly changing state.  Eventually it can:
	//  - decide to wait for an event (timer, index)
	//  - end the command with command_end()
	//  - start a live state (see below)
	//
	//  In the first case, it must first switch to a waiting
	//  sub-state, then return.  The waiting sub-state must just
	//  return immediatly when *_continue is called.  Eventually the
	//  event handler function will advance the state machine to
	//  another sub-state, and things will continue synchronously.
	//
	//  On command end it's also supposed to return immediatly.
	//
	//  The last option is to switch to the next sub-state, start a
	//  live state with live_start() then return.  The next sub-state
	//  will only be called once the live state is finished.
	//
	//  Live states change continually depending on the disk contents
	//  until the next externally discernable event is found.  They
	//  are checkpointing, run until an event is found, then they wait
	//  for it.  When an event eventually happen the the changes are
	//  either committed or replayed until the sync event time.
	//
	//  The transition to IDLE is only done on a synced event.  Some
	//  other transitions, such as activating drq, are also done after
	//  syncing without exiting live mode.  Syncing in live mode is
	//  done by calling live_delay() with the state to change to after
	//  syncing.

	enum {
		// General "doing nothing" state
		IDLE,

		// Main states - the commands
		RESTORE,
		SEEK,
		STEP,
		READ_SECTOR,
		READ_TRACK,
		READ_ID,

		// Sub states

		SPINUP,
		SPINUP_WAIT,
		SPINUP_DONE,

		SETTLE_WAIT,
		SETTLE_DONE,

		SEEK_MOVE,
		SEEK_WAIT_STEP_TIME,
		SEEK_WAIT_STEP_TIME_DONE,
		SEEK_WAIT_STABILIZATION_TIME,
		SEEK_WAIT_STABILIZATION_TIME_DONE,
		SEEK_DONE,

		WAIT_INDEX,
		WAIT_INDEX_DONE,

		SCAN_ID,
		SCAN_ID_FAILED,

		SECTOR_READ,
		TRACK_DONE,

		// Live states

		SEARCH_ADDRESS_MARK,
		READ_BLOCK_HEADER,
		READ_ID_BLOCK_TO_LOCAL,
		READ_ID_BLOCK_TO_DMA,
		READ_ID_BLOCK_TO_DMA_BYTE,
		READ_SECTOR_DATA,
		READ_SECTOR_DATA_BYTE,
		READ_TRACK_DATA,
		READ_TRACK_DATA_BYTE,
	};

	struct pll_t {
		UINT16 counter;
		UINT16 increment;
		UINT16 transition_time;
		UINT8 history;
		UINT8 slot;
		UINT8 phase_add, phase_sub, freq_add, freq_sub;
		attotime ctime;

		attotime delays[42];

		void set_clock(attotime period);
		void reset(attotime when);
		int get_next_bit(attotime &tm, floppy_image_device *floppy, attotime limit);
	};

	struct live_info {
		attotime tm;
		int state, next_state;
		UINT16 shift_reg;
		UINT16 crc;
		int bit_counter;
		bool data_separator_phase;
		UINT8 data_reg;
		UINT8 idbuf[6];
		pll_t pll;
	};

	enum {
		S_BUSY = 0x01,
		S_DRQ  = 0x02,
		S_IP   = 0x02,
		S_TR00 = 0x04,
		S_LOST = 0x04,
		S_CRC  = 0x08,
		S_RNF  = 0x10,
		S_SPIN = 0x20,
		S_DDM  = 0x20,
		S_WP   = 0x40,
		S_MON  = 0x80
	};

	const static int step_times[4];

	floppy_image_device *floppy;

	emu_timer *t_gen, *t_cmd, *t_track, *t_sector;

	bool dden, status_type_1, intrq, drq;
	int main_state, sub_state;
	UINT8 command, track, sector, data, status;
	int last_dir;

	int counter, motor_timeout, sector_size;

	int cmd_buffer, track_buffer, sector_buffer;

	live_info cur_live, checkpoint_live;
	line_cb intrq_cb, drq_cb;

	static astring tts(attotime t);
	astring ttsn();

	void delay_cycles(emu_timer *tm, int cycles);

	// Device timer subfunctions
	void do_cmd_w();
	void do_track_w();
	void do_sector_w();
	void do_generic();


	// Main-state handling functions
	void seek_start(int state);
	void seek_continue();

	void read_sector_start();
	void read_sector_continue();

	void read_track_start();
	void read_track_continue();

	void read_id_start();
	void read_id_continue();

	void interrupt_start();

	void general_continue();
	void command_end();


	void spinup();
	void index_callback(floppy_image_device *floppy, int state);

	void live_start(int live_state);
	void live_abort();
	void checkpoint();
	void rollback();
	void live_delay(int state);
	void live_sync();
	void live_run(attotime limit = attotime::never);
	bool read_one_bit(attotime limit);
	void drop_drq();
	void set_drq();
};

extern const device_type WD1772x;

#endif

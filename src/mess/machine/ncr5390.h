#ifndef NCR5390_H
#define NCR5390_H

#include "machine/scsi.h"

#define MCFG_NCR5390_ADD(_tag, _clock, _irq, _drq, _slots)	\
	MCFG_DEVICE_ADD(_tag, NCR5390, _clock)                  \
	downcast<ncr5390_device *>(device)->set_cb(_irq, _drq); \
	downcast<ncr5390_device *>(device)->set_slots(_slots);

class ncr5390_device : public device_t
{
public:
	ncr5390_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);
	void set_cb(line_cb_t irq_cb, line_cb_t drq_cb);
	void set_slots(const SCSIConfigTable &slots);
	
	DECLARE_ADDRESS_MAP(map, 8);

	DECLARE_READ8_MEMBER(tcount_lo_r);
	DECLARE_WRITE8_MEMBER(tcount_lo_w);
	DECLARE_READ8_MEMBER(tcount_hi_r);
	DECLARE_WRITE8_MEMBER(tcount_hi_w);
	DECLARE_READ8_MEMBER(fifo_r);
	DECLARE_WRITE8_MEMBER(fifo_w);
	DECLARE_READ8_MEMBER(command_r);
	DECLARE_WRITE8_MEMBER(command_w);
	DECLARE_READ8_MEMBER(status_r);
	DECLARE_WRITE8_MEMBER(bus_id_w);
	DECLARE_READ8_MEMBER(istatus_r);
	DECLARE_WRITE8_MEMBER(timeout_w);
	DECLARE_READ8_MEMBER(seq_step_r);
	DECLARE_WRITE8_MEMBER(sync_period_w);
	DECLARE_READ8_MEMBER(fifo_flags_r);
	DECLARE_WRITE8_MEMBER(sync_offset_w);
	DECLARE_READ8_MEMBER(conf_r);
	DECLARE_WRITE8_MEMBER(conf_w);
	DECLARE_WRITE8_MEMBER(clock_w);

protected:
	virtual void device_start();
	virtual void device_reset();
    virtual void device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr);

private:
	enum { MODE_D, MODE_T, MODE_I };
	enum { T_SELECT_TIMEOUT };
	enum {
		S_GROSS_ERROR = 0x40,
		S_PARITY      = 0x20,
		S_TC0         = 0x10,
		S_TCC         = 0x08,

		I_SCSI_RESET  = 0x80,
		I_ILLEGAL     = 0x40,
		I_DISCONNECT  = 0x20,
		I_BUS         = 0x10,
		I_FUNCTION    = 0x08,
		I_RESELECTED  = 0x04,
		I_SELECT_ATN  = 0x02,
		I_SELECTED    = 0x01,
	};

	const SCSIConfigTable *slots;
	SCSIInstance *devices[8];

	emu_timer *tm;

	UINT8 command[2], config, status, istatus;
	UINT8 clock_conv, sync_offset, sync_period, bus_id, select_timeout;
	UINT8 fifo[16];
	UINT16 tcount;
	int mode, fifo_pos, command_pos;

	bool irq, drq;

	line_cb_t irq_cb, drq_cb;

	void start_command();
	bool check_valid_command(UINT8 cmd);
	void command_params_size(bool &msg, int &extra);
	int derive_msg_size(UINT8 msg_id);
	void start_command_misc();
	void start_command_disconnected();
	void start_command_target();
	void start_command_initiator();
	void command_pop_and_chain();
	void check_irq();

	void reset_soft();
	void reset_disconnect();

	void delay(int cycles, int param);
};

extern const device_type NCR5390;

#endif

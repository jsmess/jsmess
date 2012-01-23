#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "ncr5390.h"

const device_type NCR5390 = &device_creator<ncr5390_device>;

DEVICE_ADDRESS_MAP_START(map, 8, ncr5390_device)
	AM_RANGE(0x0, 0x0) AM_READWRITE(tcount_lo_r, tcount_lo_w)
	AM_RANGE(0x1, 0x1) AM_READWRITE(tcount_hi_r, tcount_hi_w)
	AM_RANGE(0x2, 0x2) AM_READWRITE(fifo_r, fifo_w)
	AM_RANGE(0x3, 0x3) AM_READWRITE(command_r, command_w)
	AM_RANGE(0x4, 0x4) AM_READWRITE(status_r, bus_id_w)
	AM_RANGE(0x5, 0x5) AM_READWRITE(istatus_r, timeout_w)
	AM_RANGE(0x6, 0x6) AM_READWRITE(seq_step_r, sync_period_w)
	AM_RANGE(0x7, 0x7) AM_READWRITE(fifo_flags_r, sync_offset_w)
	AM_RANGE(0x8, 0x8) AM_READWRITE(conf_r, conf_w)
	AM_RANGE(0x9, 0x9) AM_WRITE(clock_w)
ADDRESS_MAP_END

ncr5390_device::ncr5390_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
        : device_t(mconfig, NCR5390, "5390 SCSI", tag, owner, clock)
{
}

void ncr5390_device::device_start()
{
	save_item(NAME(command));
	save_item(NAME(config));
	save_item(NAME(status));
	save_item(NAME(istatus));
	save_item(NAME(fifo_pos));
	save_item(NAME(fifo));
	save_item(NAME(tcount));
	save_item(NAME(mode));
	save_item(NAME(command_pos));
	save_item(NAME(irq));
	save_item(NAME(drq));
	save_item(NAME(clock_conv));

	tcount = 0;
	config = 0;
	status = 0;
	bus_id = 0;
	select_timeout = 0;
	tm = timer_alloc(0);
	memset(devices, 0, sizeof(devices));
	for(int i=0; i != slots->devs_present; i++)
		SCSIAllocInstance(machine(),
						  slots->devices[i].scsiClass,
						  devices+i,
						  slots->devices[i].diskregion);
}

void ncr5390_device::device_reset()
{
	fifo_pos = 0;
	memset(fifo, 0, sizeof(fifo));

	clock_conv = 2;
	sync_period = 5;
	sync_offset = 0;
	config &= 7;
	status &= 0x97;
	istatus = 0;
	irq = false;
	if(!irq_cb.isnull())
		irq_cb(irq);
	reset_soft();
}

void ncr5390_device::reset_soft()
{
	status &= 0xef;
	drq = false;
	if(!drq_cb.isnull())
		drq_cb(drq);
	reset_disconnect();
}

void ncr5390_device::reset_disconnect()
{
	command_pos = 0;
	memset(command, 0, sizeof(command));
	mode = MODE_D;
}

void ncr5390_device::set_cb(line_cb_t _irq_cb, line_cb_t _drq_cb)
{
	irq_cb = _irq_cb;
	drq_cb = _drq_cb;
}

void ncr5390_device::set_slots(const SCSIConfigTable &_slots)
{
	slots = &_slots;
}

void ncr5390_device::device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr)
{
	switch(param) {
	case T_SELECT_TIMEOUT:
		logerror("ncr5390: select timeout\n");
		istatus |= I_DISCONNECT;
		check_irq();
		break;

	default:
		logerror("ncr5390: device_timer unexpected param %d\n", param);
		exit(0);
	}	
}

void ncr5390_device::delay(int cycles, int param)
{
	if(!clock_conv)
		return;
	cycles *= clock_conv;
	tm->adjust(clocks_to_attotime(cycles), param);
}

READ8_MEMBER(ncr5390_device::tcount_lo_r)
{
	fprintf(stderr, "ncr5390: tcount_lo_r (%08x)\n", cpu_get_pc(&space.device()));
	return tcount;
}

WRITE8_MEMBER(ncr5390_device::tcount_lo_w)
{
	tcount = (tcount & 0xff00) | data;
	fprintf(stderr, "ncr5390: tcount_lo_w %02x (%08x)\n", data, cpu_get_pc(&space.device()));
}

READ8_MEMBER(ncr5390_device::tcount_hi_r)
{
	fprintf(stderr, "ncr5390: tcount_hi_r (%08x)\n", cpu_get_pc(&space.device()));
	return tcount >> 8;
}

WRITE8_MEMBER(ncr5390_device::tcount_hi_w)
{
	tcount = (tcount & 0x00ff) | (data << 8);
	fprintf(stderr, "ncr5390: tcount_hi_w %02x (%08x)\n", data, cpu_get_pc(&space.device()));
}

READ8_MEMBER(ncr5390_device::fifo_r)
{
	UINT8 r;
	if(fifo_pos) {
		r = fifo[0];
		fifo_pos--;
		memmove(fifo, fifo+1, fifo_pos);
	} else
		r = 0;
	fprintf(stderr, "ncr5390: fifo_r %02x (%08x)\n", r, cpu_get_pc(&space.device()));
	return r;
}

WRITE8_MEMBER(ncr5390_device::fifo_w)
{
	fprintf(stderr, "ncr5390: fifo_w %02x (%08x)\n", data, cpu_get_pc(&space.device()));
	if(fifo_pos != 16)
		fifo[fifo_pos++] = data;
}

READ8_MEMBER(ncr5390_device::command_r)
{
	fprintf(stderr, "ncr5390: command_r (%08x)\n", cpu_get_pc(&space.device()));
	return command[0];
}

WRITE8_MEMBER(ncr5390_device::command_w)
{
	fprintf(stderr, "ncr5390: command_w %02x (%08x)\n", data, cpu_get_pc(&space.device()));
	if(command_pos == 2) {
		status |= S_GROSS_ERROR;
		check_irq();
		return;
	}
	command[command_pos++] = data;
	if(command_pos == 1)
		start_command();
}

void ncr5390_device::command_pop_and_chain()
{
	if(command_pos) {
		command_pos--;
		if(command_pos) {
			command[0] = command[1];
			start_command();
		}
	}
}

void ncr5390_device::start_command()
{
	if(!check_valid_command(command[0] & 0x7f)) {
		logerror("ncr5390: invalid command %02x\n", command[0]);
		istatus |= I_ILLEGAL;
		check_irq();
		return;
	}

	bool msg;
	int extra;
	int total_len;
	command_params_size(msg, extra);
	if(fifo_pos < extra) {
		logerror("ncr5390: cmd without extra bytes (fp=%d, extra=%d)\n", fifo_pos, extra);
		exit(0);
	}
	if(msg) {
		if(fifo_pos < extra+1) {
			logerror("ncr5390: cmd without msg header (fp=%d, extra=%d)\n", fifo_pos, extra);
			exit(0);
		}
		int len = derive_msg_size(fifo[extra]);
		if(fifo_pos < extra+len) {
			logerror("ncr5390: cmd without complete msg (fp=%d, extra=%d, len=%d)\n", fifo_pos, extra, len);
			exit(0);
		}
		total_len = len+extra;
	} else
		total_len = extra;

	logerror("ncr5390: fifo_pos = %d, total_len = %d\n", fifo_pos, total_len);

	switch((command[0] >> 4) & 7) {
	case 0: start_command_misc(); break;
	case 4: start_command_disconnected(); break;
	case 2: start_command_target(); break;
	case 1: start_command_initiator(); break;
	}
}

bool ncr5390_device::check_valid_command(UINT8 cmd)
{
	int subcmd = cmd & 15;
	switch((cmd >> 4) & 7) {
	case 0: return subcmd <= 3;
	case 4: return mode == MODE_D && subcmd <= 5;
	case 2: return mode == MODE_T && subcmd <= 13 && subcmd != 6;
	case 1: return mode == MODE_I && (subcmd <= 2 || subcmd == 8 || subcmd == 10);
	}
	return false;
}

void ncr5390_device::command_params_size(bool &msg, int &extra)
{
	msg = false;
	extra = 0;
	int subcmd = command[0] & 15;
	switch((command[0] >> 4) & 7) {
	case 0:
		break;

	case 4:
		extra = subcmd == 0 || subcmd == 2 || subcmd == 3 ? 1 : 0;
		msg = subcmd == 1 || subcmd == 2;
		break;

	case 2:
		extra =	subcmd >= 3 && subcmd <= 5 ? 2 : 0;
		msg = subcmd <= 2;
		break;

	case 1:
		break;
	}
}

int ncr5390_device::derive_msg_size(UINT8 msg_id)
{
	const static int sizes[8] = { 6, 10, 6, 6, 6, 12, 6, 10 };
	return sizes[msg_id >> 5];
}

void ncr5390_device::start_command_misc()
{
	UINT8 subcmd = command[0] & 15;
	switch(subcmd) {
	case 0: // Nop
		command_pop_and_chain();
		break;

	case 1: // Fifo flush
		fifo_pos = 0;
		command_pop_and_chain();
		break;

	case 2: // Device reset
		device_reset();
		break;

	case 3: // SCSI bus reset
		reset_soft();
		break;

	default:
		logerror("ncr5390: start unimplemented misc(%d)\n", subcmd);
		exit(0);
	}
}

void ncr5390_device::start_command_disconnected()
{
	UINT8 subcmd = command[0] & 15;
	switch(subcmd) {
	case 2:
		delay(8192*select_timeout, T_SELECT_TIMEOUT);
		break;

	default:
		logerror("ncr5390: start unimplemented disconnected(%d)\n", subcmd);
		device_reset();
		break;
	}
}

void ncr5390_device::start_command_target()
{
	UINT8 subcmd = command[0] & 15;
	switch(subcmd) {
	default:
		logerror("ncr5390: start unimplemented target(%d)\n", subcmd);
		exit(0);
	}
}

void ncr5390_device::start_command_initiator()
{
	UINT8 subcmd = command[0] & 15;
	switch(subcmd) {
	default:
		logerror("ncr5390: start unimplemented initiator(%d)\n", subcmd);
		exit(0);
	}
}

void ncr5390_device::check_irq()
{
	bool oldirq = irq;
	irq = istatus != 0;
	if(irq != oldirq && !irq_cb.isnull())
		irq_cb(irq);

}

READ8_MEMBER(ncr5390_device::status_r)
{
	fprintf(stderr, "ncr5390: status_r (%08x)\n", cpu_get_pc(&space.device()));
	return status;
}

WRITE8_MEMBER(ncr5390_device::bus_id_w)
{
	bus_id = data & 7;
}

READ8_MEMBER(ncr5390_device::istatus_r)
{
	UINT8 res = istatus;
	status = istatus = 0;
	check_irq();
	if(res)
		command_pop_and_chain();

	fprintf(stderr, "ncr5390: istatus_r (%08x)\n", cpu_get_pc(&space.device()));
	return res;
}

WRITE8_MEMBER(ncr5390_device::timeout_w)
{
	select_timeout = data;
}

READ8_MEMBER(ncr5390_device::seq_step_r)
{
	fprintf(stderr, "ncr5390: seq_step_r (%08x)\n", cpu_get_pc(&space.device()));
	return 0x00;
}

WRITE8_MEMBER(ncr5390_device::sync_period_w)
{
	sync_period = data & 0x1f;
}

READ8_MEMBER(ncr5390_device::fifo_flags_r)
{
	return fifo_pos;
}

WRITE8_MEMBER(ncr5390_device::sync_offset_w)
{
	sync_offset = data & 0x0f;
}

READ8_MEMBER(ncr5390_device::conf_r)
{
	return config;
}

WRITE8_MEMBER(ncr5390_device::conf_w)
{
	config = data;
}

WRITE8_MEMBER(ncr5390_device::clock_w)
{
	clock_conv = data & 0x07;
}


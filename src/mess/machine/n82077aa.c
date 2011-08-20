#define ADDRESS_MAP_MODERN

#include "n82077aa.h"

const device_type N82077AA = &device_creator<n82077aa_device>;

DEVICE_ADDRESS_MAP_START(amap, 8, n82077aa_device)
	AM_RANGE(0x0, 0x0) AM_READ(sra_r)
	AM_RANGE(0x1, 0x1) AM_READ(sra_r)
	AM_RANGE(0x2, 0x2) AM_READWRITE(dor_r, dor_w)
	AM_RANGE(0x3, 0x3) AM_READWRITE(tdr_r, tdr_w)
	AM_RANGE(0x4, 0x4) AM_READWRITE(msr_r, dsr_w)
	AM_RANGE(0x5, 0x5) AM_READWRITE(fifo_r, fifo_w)
	AM_RANGE(0x7, 0x7) AM_READWRITE(dir_r, ccr_w)
ADDRESS_MAP_END

n82077aa_device::n82077aa_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) : device_t(mconfig, N82077AA, "N82077AA", tag, owner, clock)
{
}

void n82077aa_device::set_mode(int _mode)
{
	mode = _mode;
}

void n82077aa_device::device_start()
{
}

void n82077aa_device::device_reset()
{
}

READ8_MEMBER(n82077aa_device::sra_r)
{
	fprintf(stderr, "sra_r (%08x)\n", cpu_get_pc(&space.device()));
	return 0;
}

READ8_MEMBER(n82077aa_device::srb_r)
{
	fprintf(stderr, "sra_r (%08x)\n", cpu_get_pc(&space.device()));
	return 0;
}

READ8_MEMBER(n82077aa_device::dor_r)
{
	fprintf(stderr, "dor_r (%08x)\n", cpu_get_pc(&space.device()));
	return dor;
}

WRITE8_MEMBER(n82077aa_device::dor_w)
{
	UINT8 diff = dor ^ data;
	dor = data;
	fprintf(stderr, "dor_w %02x (%08x)\n", data, cpu_get_pc(&space.device()));
	if(diff & 4)
		device_reset();
}

READ8_MEMBER(n82077aa_device::tdr_r)
{
	fprintf(stderr, "tdr_r (%08x)\n", cpu_get_pc(&space.device()));
	return 0;
}

WRITE8_MEMBER(n82077aa_device::tdr_w)
{
	fprintf(stderr, "tdr_w %02x (%08x)\n", data, cpu_get_pc(&space.device()));
}

READ8_MEMBER(n82077aa_device::msr_r)
{
	if(cpu_get_pc(&space.device()) != 0x10111fa)
		fprintf(stderr, "msr_r (%08x)\n", cpu_get_pc(&space.device()));
	return 0;
}

WRITE8_MEMBER(n82077aa_device::dsr_w)
{
	fprintf(stderr, "dsr_w %02x (%08x)\n", data, cpu_get_pc(&space.device()));
	dsr = data;
}

READ8_MEMBER(n82077aa_device::fifo_r)
{
	fprintf(stderr, "fifo_r (%08x)\n", cpu_get_pc(&space.device()));
	return 0;
}

WRITE8_MEMBER(n82077aa_device::fifo_w)
{
	fprintf(stderr, "fifo_w %02x (%08x)\n", data, cpu_get_pc(&space.device()));
}

READ8_MEMBER(n82077aa_device::dir_r)
{
	fprintf(stderr, "dir_r (%08x)\n", cpu_get_pc(&space.device()));
	return 0x78;
}

WRITE8_MEMBER(n82077aa_device::ccr_w)
{
	fprintf(stderr, "ccr_w %02x (%08x)\n", data, cpu_get_pc(&space.device()));
	ccr = data;
}

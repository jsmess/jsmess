#ifndef __N82077AA_H__
#define __N82077AA_H__

#include "emu.h"

#define MCFG_N82077AA_ADD(_tag, _mode)	\
	MCFG_DEVICE_ADD(_tag, N82077AA, 0)	\
	downcast<n82077aa_device *>(device)->set_mode(_mode);

class n82077aa_device : public device_t {
public:
	enum { MODE_AT, MODE_PS2, MODE_M30 };

	n82077aa_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);
	void set_mode(int mode);

	DECLARE_ADDRESS_MAP(amap, 8);

	DECLARE_READ8_MEMBER (sra_r);
	DECLARE_READ8_MEMBER (srb_r);
	DECLARE_READ8_MEMBER (dor_r);
	DECLARE_WRITE8_MEMBER(dor_w);
	DECLARE_READ8_MEMBER (tdr_r);
	DECLARE_WRITE8_MEMBER(tdr_w);
	DECLARE_READ8_MEMBER (msr_r);
	DECLARE_WRITE8_MEMBER(dsr_w);
	DECLARE_READ8_MEMBER (fifo_r);
	DECLARE_WRITE8_MEMBER(fifo_w);
	DECLARE_READ8_MEMBER (dir_r);
	DECLARE_WRITE8_MEMBER(ccr_w);

protected:
	virtual void device_start();
	virtual void device_reset();

private:
	int mode;

	UINT8 dor, dsr, ccr, msr;
};

extern const device_type N82077AA;

#endif

/*****************************************************************************
 *
 * includes/cococart.h
 *
 ****************************************************************************/

#ifndef COCOCART_H_
#define COCOCART_H_


enum
{
	CARTLINE_CLEAR,
	CARTLINE_ASSERTED,
	CARTLINE_Q
};

#if 0
struct cartridge_callback
{
	void (*setcartline)(int data);
	void (*setbank)(int bank);
};

struct cartridge_slot
{
	void (*init)(const struct cartridge_callback *callbacks);
	void (*term)(void);
	read8_machine_func m_io_r;
	write8_machine_func m_io_w;
	void (*enablesound)(int enable);
};

extern const struct cartridge_slot cartridge_fdc_coco;
extern const struct cartridge_slot cartridge_fdc_dragon;
extern const struct cartridge_slot cartridge_fdc_dragon_delta;
extern const struct cartridge_slot cartridge_standard;
extern const struct cartridge_slot cartridge_banks;
extern const struct cartridge_slot cartridge_banks_mega;
extern const struct cartridge_slot cartridge_Orch90;
#endif


#endif /* COCOCART_H_ */

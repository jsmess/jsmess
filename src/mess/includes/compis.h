/*****************************************************************************
 *
 * includes/compis.h
 *
 * machine driver header
 *
 * Per Ola Ingvarsson
 * Tomas Karlsson
 *
 ****************************************************************************/

#ifndef COMPIS_H_
#define COMPIS_H_

#include "emu.h"
#include "machine/msm8251.h"
#include "machine/upd765.h"


struct mem_state
{
	UINT16	lower;
	UINT16	upper;
	UINT16	middle;
	UINT16	middle_size;
	UINT16	peripheral;
};

struct timer_state
{
	UINT16	control;
	UINT16	maxA;
	UINT16	maxB;
	UINT16	count;
	emu_timer *	int_timer;
	emu_timer *	time_timer;
	UINT8	time_timer_active;
	attotime	last_time;
};

struct dma_state
{
	UINT32	source;
	UINT32	dest;
	UINT16	count;
	UINT16	control;
	UINT8	finished;
	emu_timer *	finish_timer;
};

struct intr_state
{
	UINT8	pending;
	UINT16	ack_mask;
	UINT16	priority_mask;
	UINT16	in_service;
	UINT16	request;
	UINT16	status;
	UINT16	poll_status;
	UINT16	timer;
	UINT16	dma[2];
	UINT16	ext[4];
};

typedef struct
{
	struct timer_state	timer[3];
	struct dma_state	dma[2];
	struct intr_state	intr;
	struct mem_state	mem;
} i186_state;

typedef struct
{
	device_t *pic8259_master;
	device_t *pic8259_slave;
} compis_devices_t;

/* Keyboard */
typedef struct
{
	UINT8 nationality;   /* Character set, keyboard layout (Swedish) */
	UINT8 release_time;  /* Autorepeat release time (0.8)   */
	UINT8 speed;	     /* Transmission speed (14)     */
	UINT8 roll_over;     /* Key roll-over (MKEY)        */
	UINT8 click;	     /* Key click (NO)          */
	UINT8 break_nmi;     /* Keyboard break (NMI)        */
	UINT8 beep_freq;     /* Beep frequency (low)        */
	UINT8 beep_dura;     /* Beep duration (short)       */
	UINT8 password[8];   /* Password            */
	UINT8 owner[16];     /* Owner               */
	UINT8 network_id;    /* Network workstation number (1)  */
	UINT8 boot_order[4]; /* Boot device order (FD HD NW PD) */
	UINT8 key_code;
	UINT8 key_status;
} TYP_COMPIS_KEYBOARD;

/* USART 8251 */
typedef struct
{
	UINT8 status;
	UINT8 bytes_sent;
} TYP_COMPIS_USART;

/* Printer */
typedef struct
{
	UINT8 data;
	UINT8 strobe;
} TYP_COMPIS_PRINTER;


/* Main emulation */
typedef struct
{
	TYP_COMPIS_PRINTER	printer;	/* Printer */
	TYP_COMPIS_USART	usart;		/* USART 8251 */
	TYP_COMPIS_KEYBOARD	keyboard;	/* Keyboard  */
} TYP_COMPIS;


class compis_state : public driver_device
{
public:
	compis_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config),
		  m_hgdc(*this, "upd7220")
 		  { }

	required_device<device_t> m_hgdc;

	virtual void video_start();
	virtual bool screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect);
	UINT8 *m_char_rom;

	i186_state m_i186;
	compis_devices_t m_devices;
	TYP_COMPIS m_compis;
};


/*----------- defined in machine/compis.c -----------*/

extern const i8255a_interface compis_ppi_interface;
extern const struct pit8253_config compis_pit8253_config;
extern const struct pit8253_config compis_pit8254_config;
extern const struct pic8259_interface compis_pic8259_master_config;
extern const struct pic8259_interface compis_pic8259_slave_config;
extern const msm8251_interface compis_usart_interface;
extern const upd765_interface compis_fdc_interface;

DRIVER_INIT(compis);
MACHINE_START(compis);
MACHINE_RESET(compis);
INTERRUPT_GEN(compis_vblank_int);

/* PIT 8254 (80150/80130) */
READ16_DEVICE_HANDLER (compis_osp_pit_r);
WRITE16_DEVICE_HANDLER (compis_osp_pit_w);

/* USART 8251 */
READ16_HANDLER (compis_usart_r);
WRITE16_HANDLER (compis_usart_w);

/* 80186 Internal */
READ16_HANDLER (compis_i186_internal_port_r);
WRITE16_HANDLER (compis_i186_internal_port_w);

/* FDC 8272 */
READ16_HANDLER (compis_fdc_dack_r);
READ8_HANDLER (compis_fdc_r);
WRITE8_HANDLER (compis_fdc_w);


#endif /* COMPIS_H_ */

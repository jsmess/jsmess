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
READ16_HANDLER (compis_fdc_r);
WRITE16_HANDLER (compis_fdc_w);


#endif /* COMPIS_H_ */

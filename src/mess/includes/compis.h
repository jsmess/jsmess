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

#include "driver.h"
#include "machine/msm8251.h"
#include "machine/nec765.h"

/*----------- defined in machine/compis.c -----------*/

extern const ppi8255_interface compis_ppi_interface;
extern const struct pit8253_config compis_pit8253_config;
extern const struct pit8253_config compis_pit8254_config;
extern const struct pic8259_interface compis_pic8259_master_config;
extern const struct pic8259_interface compis_pic8259_slave_config;
extern const msm8251_interface compis_usart_interface;
extern const nec765_interface compis_fdc_interface;

DRIVER_INIT(compis);
MACHINE_RESET(compis);
INTERRUPT_GEN(compis_vblank_int);

/* PPI 8255 */
READ16_DEVICE_HANDLER (compis_ppi_r);
WRITE16_DEVICE_HANDLER (compis_ppi_w);

/* PIT 8253 */
READ16_DEVICE_HANDLER (compis_pit_r);
WRITE16_DEVICE_HANDLER (compis_pit_w);

/* PIC 8259 (80150/80130) */
READ16_DEVICE_HANDLER (compis_osp_pic_r);
WRITE16_DEVICE_HANDLER (compis_osp_pic_w);

/* PIT 8254 (80150/80130) */
READ16_DEVICE_HANDLER (compis_osp_pit_r);
WRITE16_DEVICE_HANDLER (compis_osp_pit_w);

/* USART 8251 */
READ16_HANDLER (compis_usart_r);
WRITE16_HANDLER (compis_usart_w);

/* 80186 Internal */
READ16_HANDLER (i186_internal_port_r);
WRITE16_HANDLER (i186_internal_port_w);

/* FDC 8272 */
READ16_HANDLER (compis_fdc_dack_r);
READ16_HANDLER (compis_fdc_r);
WRITE16_HANDLER (compis_fdc_w);

/* RTC 58174 */
READ16_HANDLER (compis_rtc_r);
WRITE16_HANDLER (compis_rtc_w);


#endif /* COMPIS_H_ */

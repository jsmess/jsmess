/******************************************************************************

	compis.h
	machine driver header

	Per Ola Ingvarsson
	Tomas Karlsson
		
 ******************************************************************************/

DRIVER_INIT(compis);
MACHINE_RESET(compis);
INTERRUPT_GEN(compis_vblank_int);

/* PPI 8255 */
READ8_HANDLER (compis_ppi_r);
WRITE8_HANDLER (compis_ppi_w);

/* PIT 8253 */
READ8_HANDLER (compis_pit_r);
WRITE8_HANDLER (compis_pit_w);

/* PIC 8259 (80150/80130) */
READ8_HANDLER (compis_osp_pic_r);
WRITE8_HANDLER (compis_osp_pic_w);

/* PIT 8254 (80150/80130) */
READ8_HANDLER (compis_osp_pit_r);
WRITE8_HANDLER (compis_osp_pit_w);

/* USART 8251 */
READ8_HANDLER (compis_usart_r);
WRITE8_HANDLER (compis_usart_w);

/* 80186 Internal */
READ8_HANDLER (i186_internal_port_r);
WRITE8_HANDLER (i186_internal_port_w);

/* FDC 8272 */
READ8_HANDLER (compis_fdc_dack_r);
READ8_HANDLER (compis_fdc_r);
WRITE8_HANDLER (compis_fdc_w);

/* RTC 58174 */
READ8_HANDLER (compis_rtc_r);
WRITE8_HANDLER (compis_rtc_w);

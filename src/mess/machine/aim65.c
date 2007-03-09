#include "driver.h"

#include "machine/6821pia.h"
#include "machine/6522via.h"
#include "includes/riot6532.h"
#include "includes/aim65.h"
#include "cpu/m6502/m6502.h"

static UINT8 pia_a, pia_b;

static void *print_timer;
static int printer_level;


static void aim65_pia(void)
{
	if ( !(pia_a&0x80) ) {
		if (!(pia_a&4)) dl1416a_write(0, pia_a&3, pia_b&0x7f, !(pia_b&0x80) );
		if (!(pia_a&8)) dl1416a_write(1, pia_a&3, pia_b&0x7f, !(pia_b&0x80) );
		if (!(pia_a&0x10)) dl1416a_write(2, pia_a&3, pia_b&0x7f, !(pia_b&0x80) );
		if (!(pia_a&0x20)) dl1416a_write(3, pia_a&3, pia_b&0x7f, !(pia_b&0x80) );
		if (!(pia_a&0x40)) dl1416a_write(4, pia_a&3, pia_b&0x7f, !(pia_b&0x80) );
	}
}

static WRITE8_HANDLER( aim65_pia_a_w)
{
	pia_a=data;
	aim65_pia();
}

static WRITE8_HANDLER( aim65_pia_b_w)
{
	pia_b=data;
	aim65_pia();
}

static const pia6821_interface pia=
{
	0,//read8_handler in_a_func,
	0,//read8_handler in_b_func,
	0,//read8_handler in_ca1_func,
	0,//read8_handler in_cb1_func,
	0,//read8_handler in_ca2_func,
	0,//read8_handler in_cb2_func,
	aim65_pia_a_w,
	aim65_pia_b_w,
	0,//write8_handler out_ca2_func,
	0,//write8_handler out_cb2_func,
	0,//void (*irq_a_func)(int state),
	0,//void (*irq_b_func)(int state),
};

static int aim65_riot_b_r(int chip)
{
	int data=0xff, a=riot_0_a_r(0);

	if (!(a&1)) {
		if (KEY_SPACE) data&=~1;
		//right?
		if (KEY_POINT) data&=~4;
		if (KEY_M) data&=~8;
		if (KEY_B) data&=~0x10;
		if (KEY_C) data&=~0x20;
		if (KEY_Z) data&=~0x40;
		//right?
	}
	if (!(a&2)) {
		if (KEY_DEL) data&=~1; //backspace 0x08
		if (KEY_LF) data&=~2; //0x60
		if (KEY_L) data&=~4;
		if (KEY_J) data&=~8;
		if (KEY_G) data&=~0x10;
		if (KEY_D) data&=~0x20;
		if (KEY_A) data&=~0x40;
		//right?
	}
	if (!(a&4)) {
		//right?
		if (KEY_PRINT) data&=~2; // backslash
		if (KEY_P) data&=~4;
		if (KEY_I) data&=~8;
		if (KEY_Y) data&=~0x10;
		if (KEY_R) data&=~0x20;
		if (KEY_W) data&=~0x40;
		if (KEY_ESC) data&=~0x80; //0x1b
	}
	if (!(a&8)) {
		if (KEY_RETURN) data&=~1;
		//right?
		if (KEY_MINUS) data&=~4;
		if (KEY_O) data&=~8;
		if (KEY_U) data&=~0x10;
		if (KEY_T) data&=~0x20;
		if (KEY_E) data&=~0x40;
		if (KEY_Q) data&=~0x80;
	}
	if (!(a&0x10)) {
		if (KEY_CRTL) data&=~1;
		//right?
		if (KEY_COLON) data&=~4;
		if (KEY_9) data&=~8;
		if (KEY_7) data&=~0x10;
		if (KEY_5) data&=~0x20;
		if (KEY_3) data&=~0x40;
		if (KEY_1) data&=~0x80;
	}
	if (!(a&0x20)) {
		if (KEY_LEFT_SHIFT) data&=~1;
		//right?
		if (KEY_0) data&=~4;
		if (KEY_8) data&=~8;
		if (KEY_6) data&=~0x10;
		if (KEY_4) data&=~0x20;
		if (KEY_2) data&=~0x40;
		if (KEY_F3) data&=~0x80; //^ 0x5e
	}
	if (!(a&0x40)) {
		if (KEY_RIGHT_SHIFT) data&=~1;
		if (KEY_DEL) data&=~2; //backspace 0x08
		if (KEY_SEMICOLON) data&=~4;
		if (KEY_K) data&=~8;
		if (KEY_H) data&=~0x10;
		if (KEY_F) data&=~0x20;
		if (KEY_S) data&=~0x40;
		if (KEY_F2) data&=~0x80;
	}
	if (!(a&0x80)) {
		if (KEY_SLASH) data&=~4;
		if (KEY_COMMA) data&=~8;
		if (KEY_N) data&=~0x10;
		if (KEY_V) data&=~0x20;
		if (KEY_X) data&=~0x40;
		if (KEY_F1) data&=~0x80;
	}
	return data;
}

static RIOT_CONFIG riot={
	1000000,
	{ 0, 0 },
	{ aim65_riot_b_r, 0 },
	0
};

/*
  aim65 thermal printer (20 characters)
  10 heat elements (place on 1 line, space between 2 characters(about 14dots))
  (pa0..pa7,pb0,pb1 1 heat element on)

  cb2 0 motor, heat elements on
  cb1 output start!?
  ca1 input

  normally printer 5x7 characters
  (horizontal movement limits not known, normally 2 dots between characters)

  3 dots space between lines?
 */

static void aim65_printer_timer(int param)
{
	via_0_cb1_w(0,printer_level);
	via_0_ca1_w(0,!printer_level);
	printer_level = !printer_level;
	aim65_printer_inc();
}

static WRITE8_HANDLER(aim65_printer_on)
{
	if (!data) {
		aim65_printer_cr();
		timer_adjust(print_timer, 0, 0, .010);
		via_0_cb1_w(0,0);
		printer_level = 1;
	}
	else
		timer_reset(print_timer, TIME_NEVER);
}

static WRITE8_HANDLER(aim65_via0_a_w)
{
	 aim65_printer_data_a(data);

}

static WRITE8_HANDLER(aim65_via0_b_w)
{
	aim65_printer_data_b(data);

}

static  READ8_HANDLER( aim65_via0_b_r)
{
	return readinputport(4);
}

static void aim65_via_irq_func(int state)
{
	if (state)
	{
		cpunum_set_input_line(0,M6502_IRQ_LINE, HOLD_LINE);
	}
	else
	{
		cpunum_set_input_line(0,M6502_IRQ_LINE, CLEAR_LINE);
	}


}

static struct via6522_interface via0={
	0,//read8_handler in_a_func;
	aim65_via0_b_r,//read8_handler in_b_func;
	0,//read8_handler in_ca1_func;
	0,//read8_handler in_cb1_func;
	0,//read8_handler in_ca2_func;
	0,//read8_handler in_cb2_func;
	aim65_via0_a_w,	//write8_handler out_a_func;
	aim65_via0_b_w, //write8_handler out_b_func;
	0,//	write8_handler out_ca1_func;
	0,//	write8_handler out_cb1_func;
	0,//write8_handler out_ca2_func;
	aim65_printer_on,//write8_handler out_cb2_func;
	aim65_via_irq_func //void (*irq_func)(int state);
};

DRIVER_INIT( aim65 )
{
	pia_config(0, PIA_STANDARD_ORDERING, &pia);
	riot_init(0, &riot);
	via_config(0,&via0);
	via_reset();
	via_0_cb1_w(1,1);
	via_0_ca1_w(1,0);

	print_timer = timer_alloc(aim65_printer_timer);

	printer_level = 0;

}













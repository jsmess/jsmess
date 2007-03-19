/*
	ti990.h: header file for ti990.c
*/

void ti990_reset_int(void);
void ti990_set_int_line(int line, int state);
void ti990_set_int2(int state);
void ti990_set_int3(int state);
void ti990_set_int6(int state);
void ti990_set_int7(int state);
void ti990_set_int9(int state);
void ti990_set_int10(int state);
void ti990_set_int13(int state);

void ti990_hold_load(void);

 READ8_HANDLER ( ti990_panel_read );
WRITE8_HANDLER ( ti990_panel_write );

void ti990_line_interrupt(void);
void ti990_ckon_ckof_callback(int state);

void ti990_cpuboard_reset(void);

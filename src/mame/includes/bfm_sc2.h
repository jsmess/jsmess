#ifndef INC_BFMSC2
#define INC_BFMSC2

/*----------- defined in drivers/bfm_sc2.c -----------*/

extern void on_scorpion2_reset(void);
extern void send_to_sc2(int data);
extern int  read_from_sc2(void);
extern void send_to_adder(int data);
extern int  receive_from_adder(void);

extern void Scorpion2_SetSwitchState(int strobe, int data, int state);
extern int Scorpion2_GetSwitchState(int strobe, int data);

extern int  get_adder2_uart_status(void);

extern int adder2_data_from_sc2;	// data available for adder from sc2
extern int adder2_sc2data;			// data
extern int sc2_data_from_adder;		// data available for sc2 from adder
extern int sc2_adderdata;			// data

extern int sc2_show_door;		  // flag <>0, display door state
extern int sc2_door_state;		  // door state to display

#endif

//Any fixes for this driver should be forwarded to AGEMAME HQ (http://www.mameworld.net/agemame/)

#ifndef INC_STEPPERS
#define INC_STEPPERS

#define MAX_STEPPERS		  16			// maximum number of steppers

#define STEPPER_48STEP_REEL    0			// STARPOINT RMXXX reel unit
#define BARCREST_48STEP_REEL   1			// Barcrest bespoke reel unit
#define STEPPER_144STEPS_DICE  2			// STARPOINT 1DCU DICE mechanism - tech sheet available from Re-A or EC on request

void Stepper_init(  int id, int type);		// init a stepper motor

void Stepper_reset_position(int id);		// reset a motor to position 0

int  Stepper_optic_state(   int id);		// read a motor's optics

void Stepper_set_index(int id,int position,int length,int pattern);

int  Stepper_update(int id, UINT8 pattern);	// update a motor

int  Stepper_get_position(int id);			// get current position in half steps

int  Stepper_get_max(int id);				// get maximum position in half steps
#endif

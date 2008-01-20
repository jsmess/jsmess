/*****************************************************************************
 *
 * machine/centroni.h
 *
 * Defines centronics/parallel port printer interface
 * Provides a centronics printer simulation (sends output to IO_PRINTER)
 *
 ****************************************************************************/

#ifndef CENTRONICS_H_
#define CENTRONICS_H_

#ifdef __cplusplus
extern "C" {
#endif


typedef enum {
	PRINTER_CENTRONICS,
	PRINTER_IBM // select line not important
} PRINTER_TYPE;

typedef struct {
	UINT8 (*read_data)(int nr);
	void (*write_data)(int nr, UINT8 data);
	int (*handshake_in)(int nr);
	void (*handshake_out)(int nr, int data, int mask);
} CENTRONICS_DEVICE;


// from printer
#define CENTRONICS_NOT_BUSY 0x80
#define CENTRONICS_ACKNOWLEDGE 0x40
#define CENTRONICS_NO_PAPER 0x20
#define CENTRONICS_ONLINE 0x10
#define CENTRONICS_NO_ERROR 8

// to printer
#define CENTRONICS_SELECT 8
#define CENTRONICS_NO_RESET 4
#define CENTRONICS_AUTOLINEFEED 2
#define CENTRONICS_STROBE 1

typedef struct {
	PRINTER_TYPE type;
	void (*handshake_out)(int n, int data, int mask);
} CENTRONICS_CONFIG;


/*----------- defined in machine/centroni.c -----------*/

void centronics_config(int nr, const CENTRONICS_CONFIG *config);

void centronics_write_data(int nr, UINT8 data);
void centronics_write_handshake(int nr, int data, int mask);
UINT8 centronics_read_data(int nr);
int centronics_read_handshake(int nr);

extern const CENTRONICS_DEVICE CENTRONICS_PRINTER_DEVICE;


#ifdef __cplusplus
}
#endif

#endif /* CENTRONI_H_ */

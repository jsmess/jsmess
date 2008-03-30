/*
	smartmed.h: header file for smartmed.c
*/

DEVICE_START( smartmedia );
DEVICE_IMAGE_LOAD( smartmedia );
DEVICE_IMAGE_UNLOAD( smartmedia );

int smartmedia_machine_init(int id);

int smartmedia_present(int id);
int smartmedia_protected(int id);

UINT8 smartmedia_data_r(int id);
void smartmedia_command_w(int id, UINT8 data);
void smartmedia_address_w(int id, UINT8 data);
void smartmedia_data_w(int id, UINT8 data);

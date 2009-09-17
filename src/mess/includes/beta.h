#ifndef __BETA__
#define __BETA__

#define SCREEN_TAG		"screen"
#define M6502_TAG		"m6502"
#define M6532_TAG		"m6532"
#define EPROM_TAG		"eprom"
#define SPEAKER_TAG		"b237"

typedef struct _beta_state beta_state;
struct _beta_state
{
	/* EPROM state */
	int eprom_oe;
	int eprom_ce;
	UINT16 eprom_addr;
	UINT8 eprom_data;

	/* display state */
	UINT8 ls145_p;
	UINT8 segment;

	/* devices */
	const device_config *speaker;
};

#endif

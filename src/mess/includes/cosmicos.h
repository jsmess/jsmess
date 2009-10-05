#ifndef __COSMICOS__
#define __COSMICOS__

#define CDP1802_TAG		"ic19"
#define DM9368_TAG		"ic10"
#define CASSETTE_TAG	"cassette"

typedef struct _cosmicos_state cosmicos_state;
struct _cosmicos_state
{
	/* display state */
	UINT8 segment;

	/* devices */
	const device_config *dm9368;
	const device_config *cassette;
};

#endif

#ifndef __ATARI_ST__
#define __ATARI_ST__

typedef struct _atarist_state atarist_state;

struct _atarist_state
{
	/* machine state */
	mc68901_t *mfp;
};

#endif

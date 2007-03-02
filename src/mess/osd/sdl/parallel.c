//============================================================
//
//	parallel.c - SDL parallel processing
//
//============================================================

#include <stdlib.h>

#include "parallel.h"
#include "mame.h"

#define SEQUENTIAL	0

//============================================================
//	GLOBAL VARIABLES
//============================================================

int win_task_count = 1;

//============================================================
//	LOCAL VARIABLES
//============================================================

//============================================================
//	win_parallel_init
//============================================================

int win_parallel_init(void)
{
	return 1;
}

//============================================================
//	osd_parallelize
//============================================================

void osd_parallelize(void (*task)(void *param, int task_num, int task_count), void *param, int max_tasks)
{
	int i;

	for (i = 0; i < max_tasks; i++)
		task(param, i, max_tasks);
}

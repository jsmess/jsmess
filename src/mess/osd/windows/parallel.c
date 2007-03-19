//============================================================
//
//	parallel.c - Win32 parallel processing
//
//============================================================

#include <windows.h>
#include <stdlib.h>

#include "parallel.h"
#include "mame.h"

#define SEQUENTIAL	0

struct task_data
{
	HANDLE thread;
	DWORD thread_id;
	HANDLE semaphore;
};

struct call_data
{
	void (*task)(void *param, int task_num, int task_count);
	void *param;
};

static void win_parallel_exit(running_machine *machine);

//============================================================
//	GLOBAL VARIABLES
//============================================================

int win_task_count;

//============================================================
//	LOCAL VARIABLES
//============================================================

static struct task_data *tasks;
static int currently_parallel;

//============================================================
//	SetThreadName (taken from MSDN)
//============================================================

#ifdef _MSC_VER
typedef struct tagTHREADNAME_INFO
{
	DWORD dwType;		// must be 0x1000
	LPCSTR szName;		// pointer to name (in user addr space)
	DWORD dwThreadID;	// thread ID (-1=caller thread)
	DWORD dwFlags;		// reserved for future use, must be zero
} THREADNAME_INFO;

static void SetThreadName(DWORD dwThreadID, LPCSTR szThreadName)
{
	THREADNAME_INFO info;
	info.dwType = 0x1000;
	info.szName = szThreadName;
	info.dwThreadID = dwThreadID;
	info.dwFlags = 0;

	__try
	{
		RaiseException( 0x406D1388, 0, sizeof(info)/sizeof(DWORD), (DWORD*) &info);
	}
	__except(EXCEPTION_CONTINUE_EXECUTION)
	{
	}
}
#endif

//============================================================
//	get_processor_count
//============================================================

INLINE DWORD get_processor_count(void)
{
	SYSTEM_INFO sysinfo;
	GetSystemInfo(&sysinfo);
	return sysinfo.dwNumberOfProcessors;
}

//============================================================
//	task_proc
//============================================================

static DWORD WINAPI task_proc(void *param)
{
	struct task_data *tdata;
	int task_num;
	HANDLE semaphore;
	MSG msg;
	const struct call_data *cd;

	tdata = (struct task_data *) param;
	task_num = tdata - tasks + 1;
	semaphore = tdata->semaphore;

#ifdef _MSC_VER
	{
		char buf[64];
		snprintf(buf, sizeof(buf) / sizeof(buf[0]), "parallelization thread #%d", task_num);
		SetThreadName(-1, buf);
	}
#endif

	while(GetMessage(&msg, NULL, 0, 0))
	{
		if (msg.message == WM_USER)
		{
			cd = (const struct call_data *) msg.lParam;
			cd->task(cd->param, task_num, win_task_count);
			ReleaseSemaphore(semaphore, 1, NULL);
		}
	}
	return 0;
}

//============================================================
//	win_parallel_init
//============================================================

int win_parallel_init(void)
{
	typedef DWORD (WINAPI *set_thread_ideal_processor_proc)(HANDLE hThread, DWORD dwIdealProcessor);

	int i;
	int processor_count;
	HMODULE kernellib;
	set_thread_ideal_processor_proc set_thread_ideal_processor;

	tasks = NULL;
	currently_parallel = 0;
	processor_count = get_processor_count();

	// if we didn't specify the amount of tasks, use the processor count
	if (win_task_count == 0)
		win_task_count = processor_count;

	// are we going to go parallel?
	if (win_task_count > 1)
	{
		tasks = malloc((win_task_count - 1) * sizeof(struct task_data));
		if (!tasks)
			goto error;

		// SetThreadIdealProcessor is only defined in NT/2000/XP; so we have to
		// dynamically link with it
		kernellib = LoadLibrary(TEXT("KERNEL32.DLL"));
		if (kernellib)
		{
			set_thread_ideal_processor = (set_thread_ideal_processor_proc)
				GetProcAddress(kernellib, "SetThreadIdealProcessor");
			FreeLibrary(kernellib);
		}
		else
		{
			set_thread_ideal_processor = NULL;
		}

		memset(tasks, 0, (win_task_count - 1) * sizeof(struct task_data));

		for (i = 0; i < win_task_count-1; i++)
		{
			tasks[i].semaphore = CreateSemaphore(NULL, 0, 1, NULL);
			if (!tasks[i].semaphore)
				goto error;
			tasks[i].thread = CreateThread(NULL, 0, task_proc, (void *) &tasks[i], 0, &tasks[i].thread_id);
			if (!tasks[i].thread)
				goto error;
			if (set_thread_ideal_processor)
				set_thread_ideal_processor(tasks[i].thread, (i+1) % processor_count);
		}
		if (set_thread_ideal_processor)
			set_thread_ideal_processor(GetCurrentThread(), 0);
	}

	add_exit_callback(Machine, win_parallel_exit);
	return 0;

error:
	win_parallel_exit(Machine);
	return -1;
}

//============================================================
//	win_parallel_exit
//============================================================

static void win_parallel_exit(running_machine *machine)
{
	int i;
	HANDLE *threads = NULL;
	int thread_count;

	if (tasks)
	{
		threads = alloca(sizeof(HANDLE) * (win_task_count-1));
		thread_count = 0;

		for (i = 0; i < win_task_count-1; i++)
		{
			if (tasks[i].thread)
			{
				threads[thread_count++] = tasks[i].thread;
				PostThreadMessage(tasks[i].thread_id, WM_QUIT, 0, 0);
			}
			if (tasks[i].semaphore)
			{
				CloseHandle(tasks[i].semaphore);
			}
		}
		WaitForMultipleObjects(thread_count, threads, TRUE, INFINITE);
		free(tasks);
		tasks = NULL;
	}
}

//============================================================
//	osd_parallelize
//============================================================

void osd_parallelize(void (*task)(void *param, int task_num, int task_count), void *param, int max_tasks)
{
	int i;

	// we can't divide this load by more than we have threads
	if (max_tasks > win_task_count)
		max_tasks = win_task_count;

#if SEQUENTIAL
	for (i = 0; i < max_tasks; i++)
		task(param, i, max_tasks);

#else
	if ((max_tasks <= 1) || currently_parallel)
	{
		// either we are not parallel or we are being called reentrantly
		task(param, 0, 1);
	}
	else
	{
		struct call_data cd;
		HANDLE *semaphores;

		currently_parallel = 1;
		semaphores = alloca(sizeof(HANDLE) * (max_tasks-1));

		// set up the call_data parameter block
		cd.task = task;
		cd.param = param;

		// issue a message to each of the threads
		for (i = 0; i < max_tasks-1; i++)
		{
			semaphores[i] = tasks[i].semaphore;
			PostThreadMessage(tasks[i].thread_id, WM_USER, 0, (LPARAM) &cd);
		}

		// perform task #0
		task(param, 0, max_tasks);

		// wait for all the other tasks to complete
		WaitForMultipleObjects(max_tasks-1, semaphores, TRUE, INFINITE);
		currently_parallel = 0;
	}
#endif
}

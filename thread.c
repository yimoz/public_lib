#include "thread.h"

#ifdef WIN32
#include <windows.h>
#include <process.h>
#else
#endif

#include <stdlib.h>

unsigned int __stdcall thread_func(void *data)
{
	if (data)
	{
		pure_thread_p p_thread = (pure_thread_p)data;
		((pure_thread_func)p_thread->func)(p_thread->data);
	}

	_endthreadex(0);
	return 0;
}

pure_thread_p createthread(pure_thread_func pth_func, void *data)
{
	pure_thread_p p_thread = (pure_thread_p)calloc(1, sizeof(pure_thread));
	if (p_thread != 0)
	{
		p_thread->func = pth_func;
		p_thread->data = data;
		p_thread->psh_thread_handle = _beginthreadex(NULL, 0, thread_func, p_thread, 0, 0);
	}

	return p_thread;
}
void deletethread(pure_thread_p thread, unsigned long ul_wait_time)
{
	if (thread == 0)
		return;

	WaitForSingleObject(thread->psh_thread_handle, ul_wait_time);

	CloseHandle(thread->psh_thread_handle);

	free(thread);
}

void deletethread_infinite(pure_thread_p thread)
{
	if (thread == 0)
		return;

	WaitForSingleObject(thread->psh_thread_handle, INFINITE);

	CloseHandle(thread->psh_thread_handle);

	free(thread);
}
#ifndef PURE_THREAD_H_
#define PURE_THREAD_H_

#include "type.h"

typedef void (*pure_thread_func)(void* data);

typedef struct pure_thread_t
{
	pure_thread_handle psh_thread_handle;
	pure_thread_func func;
	void *data;
}pure_thread,*pure_thread_p;


pure_thread_p createthread(pure_thread_func pth_func, void *data);
//if you want to wait forever,use INFINITE as ul_wait_time.
void deletethread(pure_thread_p thread,unsigned long ul_wait_time);
void deletethread_infinite(pure_thread_p thread);


#endif // 

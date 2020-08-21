#ifndef PURE_TYPE_H_
#define PURE_TYPE_H_

#ifdef WIN32

#ifndef _UINTPTR_T_DEFINED
#define _UINTPTR_T_DEFINED
#ifdef _WIN64
typedef unsigned __int64  uintptr_t;
#else
//typedef unsigned int uintptr_t;
typedef void* uintptr_t;
#endif
#endif

typedef void* pure_sys_handle;
typedef uintptr_t pure_thread_handle;
#else

#endif

#define PURE_EVENT_TIMER  1
#define PURE_EVENT_ACCEPT 2
#define PURE_EVENT_RECV   3
#define PURE_EVENT_SEND   4

typedef struct _pure_list_item pure_list_item, *pure_list_item_p;

struct _pure_list_item
{
	void *p_data;
	pure_list_item_p prev;
	pure_list_item_p next;
};

typedef struct _pure_list
{
	pure_list_item_p head;
	pure_list_item_p end;
	int n_len;
}pure_list, *pure_list_p;

/* one file handle one this */
typedef struct _pure_event_context
{
	pure_sys_handle syshandle_file;
	pure_list_p list_event_type;
}pure_event_context, *pure_event_context_p;

typedef struct pure_event_handle_t
{
#ifdef WIN32
	pure_sys_handle syshandle_completeport;      /* sys handle */
	long l_handle;                               /* user handle */
#else
#endif

	int n_thread_run;                            /* thread start signal */
	pure_list_p list_threads;                    /* thread list */
	pure_list_p list_io_context;                 /* event I/O context */
}pure_event_handle, *pure_event_handle_ptr;

#endif // PURE_TYPE_H_

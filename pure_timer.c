#include "pure_timer.h"
#include "thread.h"
#include "list_double.h"
#include "lock.h"

#if (!defined(WIN32) && !defined(_WIN32_WCE))
#include <time.h>
#include <sys/timeb.h>
#include <string>
#include <stdlib.h>
#include <stdio.h>
#else
#include <time.h>
#include <windows.h>
#endif

//�������ɼ�ʱ��ʱ����
static unsigned long ul_timer_handle_total = 1;

typedef struct pure_timer_event_t pure_timer_event_t, pure_timer_event, *pure_timer_event_p;

struct pure_timer_event_t
{
	unsigned long   ul_handle;
	pure_timeval    tt_create;    //����ʱ��
	pure_timeval    tt_start;     //����ʱ��
	pure_timeval    tt_interval;  //���ʱ��
	pure_timeval    tt_lastval;   //���һ�θ���ʱ��
	callback_timer  cbt_function; //��ʱ���ص�����
	void*           p_user_data;  //�ص��û�����
	int             n_delete;     //ɾ����ǣ���Ϊ0ʱ��ʾ�ѱ�ɾ������ʹ��
};

struct pure_timer_t
{
	int n_thread_count;
	int n_count_in_one_thread;
	pure_list_p p_list_thread;     //list of pure_timer_thread_p
	int n_thread_run;
	pure_list_p p_list_event;
	int n_event_count;
	pure_wrlock_p p_wrlock_event;
	
	pure_list_p p_list_event_del;
	pure_wrlock_p p_wrlock_event_del;
	pure_thread_p p_thread_del;
	int n_event_del_run;
};

typedef struct pure_timer_thread_t
{
	pure_thread_p p_thread;     //ʵ���߳�ָ��
	int n_task_count;           //������������
	int n_thread_num;           //�߳����
	pure_timer_p p_timer;       //�߳�������ʱ��
#ifdef WIN32
	HANDLE h_ev; /* ֪ͨ�¼� Event */
#else
#endif
}pure_timer_thread, *pure_timer_thread_p;

unsigned long _gettickcount()
{
#if (!defined(WIN32) && !defined(_WIN32_WCE))
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
#else
	return GetTickCount();
#endif
}

void _pure_list_num_task_thread(int n_start, int n_task_num,pure_list_p p_list)
{
	char szLog[512];
	int nsign = 0;
	unsigned long ul_now = 0;
	pure_list_item_p p_item = p_list->head;
	pure_timer_event_p p_event;

	for (;nsign < (n_start - 1);nsign++)
	{
		if (p_item->next == 0)
			break;
		
		p_item = p_item->next;
	}
	
	for (nsign = 0;nsign < n_task_num;nsign++)
	{
		ul_now = _gettickcount();
		if (p_item == 0)
		{
			break;
		}

		p_event = p_item->p_data;
		if (p_event && p_event->n_delete == 0)
		{
			if (p_event->tt_create.ul_million != 0 && (ul_now - p_event->tt_create.ul_million > p_event->tt_start.ul_million))
			{
				//start
				p_event->tt_create.ul_million = 0;
				if ((callback_timer)p_event->cbt_function)
					((callback_timer)p_event->cbt_function)(p_event->ul_handle, p_event->p_user_data);
					p_event->tt_lastval.ul_million = ul_now;
			}
			else if (ul_now - p_event->tt_lastval.ul_million > p_event->tt_interval.ul_million)
			{
				if ((callback_timer)p_event->cbt_function)
					((callback_timer)p_event->cbt_function)(p_event->ul_handle, p_event->p_user_data);
					p_event->tt_lastval.ul_million = ul_now;
			}
		}
		p_item = p_item->next;
	}
}

void pure_timerthread(void* p_data)
{
	pure_timer_thread_p p_thread = (pure_timer_thread_p)p_data;
	while (p_thread && p_thread->p_timer && p_thread->p_timer->n_thread_run)
	{
		Sleep(10);

		int n_task_count = 0;
		int n_task_run = 0;
		int n_task_start = 0;

		pure_r_lock(p_thread->p_timer->p_wrlock_event);
		n_task_count = pure_list_get_len(p_thread->p_timer->p_list_event);
		pure_wr_unlock(p_thread->p_timer->p_wrlock_event);

		if (n_task_count > p_thread->n_thread_num * p_thread->n_task_count)
		{
			n_task_run = n_task_count / p_thread->p_timer->n_thread_count;
			n_task_start = n_task_run * (p_thread->n_thread_num - 1) + 1;
			if (p_thread->n_thread_num == 1)
			{
				n_task_run += n_task_count % p_thread->p_timer->n_thread_count;
			}

			pure_r_lock(p_thread->p_timer->p_wrlock_event);
			_pure_list_num_task_thread(n_task_start, n_task_run, p_thread->p_timer->p_list_event);
			pure_wr_unlock(p_thread->p_timer->p_wrlock_event);
			Sleep(10);
		}
		else
		{
			ResetEvent(p_thread->h_ev);
			WaitForSingleObject(p_thread->h_ev, INFINITE);
		}
	}
}
pure_timer_event_p _pure_timer_event_list_item_get_byhandle(pure_list_p p_list, unsigned long ul_handle)
{
	pure_list_item_p p_tmp;

	if (p_list == 0)
		return 0;

	p_tmp = p_list->head;

	while (p_tmp)
	{
		if (((pure_timer_event_p)p_tmp->p_data)->ul_handle == ul_handle)
		{
			break;
		}

		p_tmp = p_tmp->next;
	}

	if (p_tmp)
	{
		return p_tmp->p_data;
	}

	return 0;
}

void _pure_timer_del_event(pure_timer_p p_timer, unsigned long ul_timer_event_handle)
{
	pure_timer_event_p p_event = 0;
	int n_event_cout = 0;

	//ɾ������
	pure_w_lock(p_timer->p_wrlock_event);
	p_event = _pure_timer_event_list_item_get_byhandle(p_timer->p_list_event, ul_timer_event_handle);
	if (p_event)
	{
		pure_list_item_del(p_timer->p_list_event, p_event);
		p_timer->n_event_count--;
		n_event_cout = p_timer->n_event_count;
	}
	pure_wr_unlock(p_timer->p_wrlock_event);
}

void _pure_event_del_thread(void* pdata)
{
	pure_timer_t *p_timer = pdata;
	if (pdata)
	{
		while (p_timer->n_event_del_run)
		{
			unsigned long *p_handle = 0;
			pure_r_lock(p_timer->p_wrlock_event_del);
			p_handle = pure_list_get_first(p_timer->p_list_event_del);
			pure_wr_unlock(p_timer->p_wrlock_event_del);
			if (p_handle)
			{
				_pure_timer_del_event(p_timer, (*p_handle));

				pure_w_lock(p_timer->p_wrlock_event_del);
				pure_list_item_del(p_timer->p_list_event_del, p_handle);
				pure_wr_unlock(p_timer->p_wrlock_event_del);

				free(p_handle);
				p_handle = 0;
			}

			Sleep(100);
		}
	}
}

pure_timer_thread_p _pure_timer_thread_list_item_get_bynum(pure_list_p p_list, int n_thread_num)
{
	pure_list_item_p p_tmp;

	if (p_list == 0)
		return 0;

	p_tmp = p_list->head;

	while (p_tmp)
	{
		if (((pure_timer_thread_p)p_tmp->p_data)->n_thread_num == n_thread_num)
		{
			break;
		}

		p_tmp = p_tmp->next;
	}

	return p_tmp->p_data;
}

void _delete_timer_thread(pure_timer_thread_p thread)
{
	if (thread == 0)
		return;

	SetEvent(thread->h_ev);
	deletethread_infinite(thread->p_thread);

	CloseHandle(thread->h_ev);

	free(thread);
}

pure_timer_p pure_timer_create(int n_thread_count, int n_count_in_one_thread)
{
	pure_timer_p p_timer = 0;
	int nsign = 0;

	if (n_thread_count < 2)
		n_thread_count = 2;
	if (n_count_in_one_thread < 10)
		n_count_in_one_thread = 10;

	//��ʼ��&����ֵ��timer
	p_timer = (pure_timer_p)calloc(1, sizeof(pure_timer));
	if (0 == p_timer)
	{
		return 0;
	}

	p_timer->n_count_in_one_thread = n_count_in_one_thread;
	p_timer->n_thread_count = n_thread_count;
	p_timer->p_wrlock_event = pure_wrlock_create();
	if (0 == p_timer->p_wrlock_event)
	{
		free(p_timer);
		return 0;
	}

	p_timer->p_list_event = pure_list_init();
	p_timer->p_list_thread = pure_list_init();
	
	p_timer->p_list_event_del = pure_list_init();
	p_timer->p_wrlock_event_del = pure_wrlock_create();
	p_timer->n_event_del_run = 1;
	p_timer->p_thread_del = createthread(_pure_event_del_thread, p_timer);
	if (p_timer->p_thread_del == 0)
	{
		pure_list_fini(p_timer->p_list_event);
		pure_list_fini(p_timer->p_list_event_del);
		pure_wrlock_free(p_timer->p_wrlock_event);
		pure_wrlock_free(p_timer->p_wrlock_event_del);
		p_timer->n_thread_run = 0;
		pure_list_fini_with_freefunc(p_timer->p_list_thread, _delete_timer_thread);
		free(p_timer);
		return 0;
	}

	//��ʼ���߳�
	p_timer->n_thread_run = 1;
	for (nsign = 0;nsign < n_thread_count;nsign++)
	{
		pure_timer_thread_p p_thread = calloc(1, sizeof(pure_timer_thread));
		if (p_thread)
		{
			p_thread->n_thread_num = nsign + 1;
			p_thread->p_timer = p_timer;
			
			p_thread->h_ev = CreateEvent(NULL, TRUE, FALSE, NULL);
			if (p_thread->h_ev == NULL)
			{
				p_timer->n_event_del_run = 0;
				deletethread_infinite(p_timer->p_thread_del);
				pure_list_fini(p_timer->p_list_event_del);
				pure_list_fini(p_timer->p_list_event);
				pure_wrlock_free(p_timer->p_wrlock_event);
				pure_wrlock_free(p_timer->p_wrlock_event_del);
				p_timer->n_thread_run = 0;
				pure_list_fini_with_freefunc(p_timer->p_list_thread, _delete_timer_thread);
				
				free(p_timer);

				return 0;
			}

			p_thread->p_thread = createthread(pure_timerthread, p_thread);
			pure_list_item_add(p_timer->p_list_thread, p_thread);
		}
	}

	return p_timer;
}

void pure_timer_delete(pure_timer_p p_timer)
{
	if (p_timer == 0)
		return;

	//�ͷ��߳�
	p_timer->n_thread_run = 0;
	pure_list_fini_with_freefunc(p_timer->p_list_thread, _delete_timer_thread);

	p_timer->n_event_del_run = 0;
	deletethread_infinite(p_timer->p_thread_del);

	//�ͷ�event
	pure_w_lock(p_timer->p_wrlock_event_del);
	pure_list_fini(p_timer->p_list_event_del);
	pure_wr_unlock(p_timer->p_wrlock_event_del);

	pure_w_lock(p_timer->p_wrlock_event);
	pure_list_fini(p_timer->p_list_event);
	pure_wr_unlock(p_timer->p_wrlock_event);

	//�ͷ���
	pure_wrlock_free(p_timer->p_wrlock_event);
	pure_wrlock_free(p_timer->p_wrlock_event_del);

	free(p_timer);
}

unsigned long _pure_timer_add_event(pure_timer_p p_timer, pure_timer_conf *p_timer_conf)
{
	pure_timer_event_p p_event = 0;
	int n_event_cout = 0;
	int n_call_thread_num = 0;
	int nsign = 0;

	if (p_timer_conf == 0 || p_timer == 0 || p_timer_conf->cbt_function == 0)
		return -1;

	//�������
	p_event = (pure_timer_event_p)calloc(1, sizeof(pure_timer_event));
	if (p_event == 0)
		return -2;

	p_event->cbt_function = p_timer_conf->cbt_function;
	p_event->p_user_data = p_timer_conf->p_user_data;
	p_event->tt_interval = p_timer_conf->tt_interval;
	p_event->tt_start = p_timer_conf->tt_start;
	p_event->ul_handle = ul_timer_handle_total++;
	p_event->tt_create.ul_million = _gettickcount();

	//����handle���ɻ���
	if (ul_timer_handle_total == 0)
		ul_timer_handle_total = 1;

	{
		pure_w_lock(p_timer->p_wrlock_event);

		pure_list_item_add(p_timer->p_list_event, p_event);
		p_timer->n_event_count++;
		n_event_cout = p_timer->n_event_count;
		pure_wr_unlock(p_timer->p_wrlock_event);
	}

	n_call_thread_num = n_event_cout / p_timer->n_count_in_one_thread;
	if (n_call_thread_num > 0)
	{
		//����10������
		for (nsign = 0; nsign < n_call_thread_num; nsign++)
		{
			pure_timer_thread_p p_timer_thread = _pure_timer_thread_list_item_get_bynum(p_timer->p_list_thread, nsign + 1);
			if (p_timer_thread)
			{
				SetEvent(p_timer_thread->h_ev);
			}
		}
	}
	else
	{
		//<10������
		if (n_event_cout > 0)
		{
			pure_timer_thread_p p_timer_thread = _pure_timer_thread_list_item_get_bynum(p_timer->p_list_thread, 1);
			if (p_timer_thread)
			{
				SetEvent(p_timer_thread->h_ev);
			}
		}
	}

	return p_event->ul_handle;
}

unsigned long pure_timer_add_event(pure_timer_p p_timer, pure_timer_conf *p_timer_conf)
{
	return _pure_timer_add_event(p_timer, p_timer_conf);
}

void pure_timer_del_event(pure_timer_p p_timer, unsigned long ul_timer_event_handle)
{
	if (p_timer)
	{
		unsigned long* p_handle = calloc(1, sizeof(unsigned long));
		*p_handle = ul_timer_event_handle;

		pure_r_lock(p_timer->p_wrlock_event);
		pure_timer_event_p p_event = _pure_timer_event_list_item_get_byhandle(p_timer->p_list_event,ul_timer_event_handle);
		if (p_event)
			p_event->n_delete = 1;
		pure_wr_unlock(p_timer->p_wrlock_event);

		pure_w_lock(p_timer->p_wrlock_event_del);
		pure_list_item_add(p_timer->p_list_event_del, p_handle);
		pure_wr_unlock(p_timer->p_wrlock_event_del);
	}
}

void *pure_timer_get_event_userdata(pure_timer_p p_timer, unsigned long ul_timer_event_handle)
{
	pure_timer_event_p p_event = 0;
	void* p_context = 0;

	pure_r_lock(p_timer->p_wrlock_event);
	p_event = _pure_timer_event_list_item_get_byhandle(p_timer->p_list_event, ul_timer_event_handle);
	if (p_event)
	{
		p_context = p_event->p_user_data;
	}
	pure_wr_unlock(p_timer->p_wrlock_event);

	return p_context;
}

int pure_timer_get_cpu_count()
{
	SYSTEM_INFO si;

	GetSystemInfo(&si);

	return si.dwNumberOfProcessors;
}
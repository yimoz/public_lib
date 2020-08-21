#ifndef PURE_TIMER_H_
#define PURE_TIMER_H_

/*
** ϵͳ������ռ���ں���Դ,pure_timer_t�����̳߳صķ�ʽʵ��
*/

#define TIME_NAME_LEN 64

typedef struct pure_timeval_t        pure_timeval;
typedef struct pure_timer_t          pure_timer_t,pure_timer,*pure_timer_p;
typedef struct pure_timer_conf_t     pure_timer_conf_t,pure_timer_conf, *pure_timer_conf_p;

struct pure_timeval_t
{
	unsigned long ul_seconds;
	unsigned long ul_million;
	unsigned long ul_nano;
};

typedef void (__stdcall *callback_timer)(unsigned long ul_handle, void *p_user_data);

struct pure_timer_conf_t
{
	pure_timeval    tt_start;     //����ʱ��
	pure_timeval    tt_interval;  //���ʱ��
	callback_timer  cbt_function; //��ʱ���ص�����
	void*           p_user_data;  //�ص��û�����
};

/*
* @n_thread_count: �����߳���(default=2)
* @n_count_in_one_thread: һ���߳���ͳ���¼���(default=10)
* return: = 0 failed.
*/
pure_timer_p pure_timer_create(int n_thread_count,int n_count_in_one_thread);
void pure_timer_delete(pure_timer_p p_timer);

/*
* @p_timer: ��ʱ��ָ��
* @p_timer_conf: ��ʱ�����ýṹ��ָ��
* return: > 0 timer handle,<= 0 failed.
*/
unsigned long pure_timer_add_event(pure_timer_p p_timer, pure_timer_conf *p_timer_conf);
void pure_timer_del_event(pure_timer_p p_timer, unsigned long ul_timer_event_handle);

void *pure_timer_get_event_userdata(pure_timer_p p_timer, unsigned long ul_timer_event_handle);

int pure_timer_get_cpu_count();

#endif // PURE_TIMER_H_

#include "lock.h"

#if (!defined(WIN32) && !defined(_WIN32_WCE))
#include <pthread.h>
#else
#ifndef _WIN32_WCE
#include <process.h>
#endif // _WIN32_WCE
#include <windows.h>
#endif

pure_lock_t pure_lock_create()
{
	pure_lock_t ret;
#if (!defined(WIN32) && !defined(_WIN32_WCE))
	ret = (pure_lock_t)malloc(sizeof(pthread_mutex_t));
#else
	ret = (pure_lock_t)malloc(sizeof(CRITICAL_SECTION));
#endif

#if (!defined(WIN32) && !defined(_WIN32_WCE))
	//����ڶ�������Ϊ�գ�������Ĭ��.
	pthread_mutexattr_t pthreadmtxattr;
	pthread_mutexattr_init(&pthreadmtxattr);
	pthread_mutexattr_settype(&pthreadmtxattr, PTHREAD_MUTEX_RECURSIVE_NP);
	pthread_mutex_init((pthread_mutex_t*)ret, &pthreadmtxattr);
#else
	InitializeCriticalSection((CRITICAL_SECTION*)ret);
#endif

	return ret;
}

void pure_lock_free(pure_lock_t lock)
{
#if (!defined(WIN32) && !defined(_WIN32_WCE))
	pthread_mutex_destroy((pthread_mutex_t*)lock);
#else
	DeleteCriticalSection((CRITICAL_SECTION*)lock);
#endif

#if (!defined(WIN32) && !defined(_WIN32_WCE))
	free((pthread_mutex_t*)lock);
#else
	free((CRITICAL_SECTION*)lock);
#endif
}

void pure_lock(pure_lock_t lock)
{
#if (!defined(WIN32) && !defined(_WIN32_WCE))
	pthread_mutex_lock((pthread_mutex_t*)lock);
#else
	EnterCriticalSection((CRITICAL_SECTION*)lock);
#endif
}

void pure_unlock(pure_lock_t lock)
{
#if (!defined(WIN32) && !defined(_WIN32_WCE))
	pthread_mutex_unlock((pthread_mutex_t*)lock);
#else
	LeaveCriticalSection((CRITICAL_SECTION*)lock);
#endif
}



//write&read lock
#define RWLOCK_IDLE 0 /* ���� */
#define RWLOCK_R 0x01 /* ���� */
#define RWLOCK_W 0x02 /* д�� */

struct pure_wrlock_t
{
#ifdef WIN32
	CRITICAL_SECTION _lock;
	HANDLE _ev; /* ֪ͨ�¼� Event */
#else
#endif
	int _st; /* ��״ֵ̬ */
	int _rlockCount; /* �������� */
	int _rwaitingCount; /* ���ȴ����� */
	int _wwaitingCount; /* д�ȴ����� */
};


pure_wrlock_p pure_wrlock_create()
{
	pure_wrlock_p p_lock = calloc(1, sizeof(pure_wrlock_t));
	if (p_lock)
	{
		InitializeCriticalSection(&(p_lock->_lock));

		p_lock->_ev = CreateEvent(NULL, TRUE, FALSE, NULL);
		ResetEvent(p_lock->_ev);
		
		if (p_lock->_ev == NULL)
		{
			DeleteCriticalSection(&(p_lock->_lock));
			free(p_lock);
			return 0;
		}
	}
	return p_lock;
}
void pure_wrlock_free(pure_wrlock_p lock)
{
	if (lock == 0)
		return;
	DeleteCriticalSection(&(lock->_lock));
	CloseHandle(lock->_ev);
	
	free(lock);
}
void pure_r_lock(pure_wrlock_p lock)
{
	int n_isWaitReturn = 0;

	if (lock == 0)
		return;

	//OutputDebugStringA("Read Lock\n");
	
	while (1)
	{
		//WaitForSingleObject(_stLock, INFINITE);
		EnterCriticalSection(&(lock->_lock));
		if (n_isWaitReturn)
		{
			/*
			* �ȴ��¼�����,���¾�����.
			*/
			--lock->_rwaitingCount;
		}
		if (lock->_st == RWLOCK_IDLE)
		{
			/*
			* ����״̬,ֱ�ӵõ�����Ȩ
			*/
			lock->_st = RWLOCK_R;
			lock->_rlockCount++;
			//ReleaseMutex(_stLock);
			LeaveCriticalSection(&(lock->_lock));
			break;
		}
		else if (lock->_st == RWLOCK_R)
		{
			if (lock->_wwaitingCount > 0)
			{
				/*
				* ��д�����ڵȴ�,��һ��ȴ�,��ʹд���ܻ�þ�������.
				*/
				++lock->_rwaitingCount;
				//ResetEvent(lock->_ev);
				LeaveCriticalSection(&(lock->_lock));
				/*
				* ��Ȼ LeaveCriticalSection() �� WaitForSingleObject() ֮����һ��ʱ�䴰��,
				* ��������windowsƽ̨���¼��ź��ǲ��ᶪʧ��,����û������.
				*/
				WaitForSingleObject(lock->_ev, INFINITE);

				EnterCriticalSection(&(lock->_lock));
				ResetEvent(lock->_ev);
				LeaveCriticalSection(&(lock->_lock));
				/*
				* �ȴ�����,�������Լ���.
				*/
				n_isWaitReturn = 1;
			}
			else
			{
				/*
				* �õ�����,����+1
				*/
				++lock->_rlockCount;
				LeaveCriticalSection(&(lock->_lock));
				break;
			}
		}
		else if (lock->_st == RWLOCK_W)
		{
			/*
			* �ȴ�д���ͷ�
			*/
			++lock->_rwaitingCount;
			//ResetEvent(lock->_ev);
			//SignalObjectAndWait(_stLock, _ev, INFINITE, FALSE);
			LeaveCriticalSection(&(lock->_lock));
			WaitForSingleObject(lock->_ev, INFINITE);

			EnterCriticalSection(&(lock->_lock));
			ResetEvent(lock->_ev);
			LeaveCriticalSection(&(lock->_lock));
			/*
			* �ȴ�����,�������Լ���.
			*/
			n_isWaitReturn = 1;
		}
		else
		{
			//assert(0);
			break;
		}
	}
	//OutputDebugStringA("out Read Lock\n");
}

void pure_w_lock(pure_wrlock_p lock)
{
	int n_isWaitReturn = 0;

	if (lock == 0)
		return;

	//OutputDebugStringA("Write Lock\n");

	while (1)
	{
		EnterCriticalSection(&(lock->_lock));
		if (n_isWaitReturn) 
			--lock->_wwaitingCount;
		if (lock->_st == RWLOCK_IDLE)
		{
			lock->_st = RWLOCK_W;
			LeaveCriticalSection(&(lock->_lock));
			break;
		}
		else
		{
			++lock->_wwaitingCount;
			//ResetEvent(lock->_ev);
			LeaveCriticalSection(&(lock->_lock));
			WaitForSingleObject(lock->_ev, INFINITE);

			EnterCriticalSection(&(lock->_lock));
			ResetEvent(lock->_ev);
			LeaveCriticalSection(&(lock->_lock));
			n_isWaitReturn = 1;
		}
	}
	//OutputDebugStringA("out Write Lock\n");
}
void pure_wr_unlock(pure_wrlock_p lock)
{
	if (lock == 0)
		return;

	//OutputDebugStringA("unLock\n");
	EnterCriticalSection(&(lock->_lock));
	if (lock->_rlockCount > 0)
	{
		/* �������� */
		--lock->_rlockCount;
		if (0 == lock->_rlockCount)
		{
			lock->_st = RWLOCK_IDLE;
			/* �ͷ� */
			if (lock->_wwaitingCount > 0 || lock->_rwaitingCount > 0)
			{
				/*
				* ��ʱ�����������ڵȴ�,�������еȴ����߳�.(�ֶ��¼�).
				* ʹ��Щ�������¾�����.
				*/
				SetEvent(lock->_ev);
			}
			else
			{
				/* ���� */
			}
		}
		else
		{
			/* ���ж��� */
		}
	}
	else
	{
		lock->_st = RWLOCK_IDLE;
		/* д������ */
		if (lock->_wwaitingCount > 0 || lock->_rwaitingCount > 0)
		{
			/*
			* �����ռ�л�����_stLock�������,�����¼�,��ô���ܻ�ʹһЩ�������ܵõ���������.
			* �������unlockʱ,��һ���߳����õ���rlock����wlock.������ͷŻ�����,ֻ��֮ǰ�Ѿ��ȴ����������л�����������Ȩ.
			*/
			SetEvent(lock->_ev);
		}
		else
		{
			/* ���� */
		}
	}
	LeaveCriticalSection(&(lock->_lock));
	//OutputDebugStringA("out unLock\n");
}
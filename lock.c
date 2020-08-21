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
	//这里第二个参数为空，锁类型默认.
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
#define RWLOCK_IDLE 0 /* 空闲 */
#define RWLOCK_R 0x01 /* 读锁 */
#define RWLOCK_W 0x02 /* 写锁 */

struct pure_wrlock_t
{
#ifdef WIN32
	CRITICAL_SECTION _lock;
	HANDLE _ev; /* 通知事件 Event */
#else
#endif
	int _st; /* 锁状态值 */
	int _rlockCount; /* 读锁计数 */
	int _rwaitingCount; /* 读等待计数 */
	int _wwaitingCount; /* 写等待计数 */
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
			* 等待事件返回,重新竞争锁.
			*/
			--lock->_rwaitingCount;
		}
		if (lock->_st == RWLOCK_IDLE)
		{
			/*
			* 空闲状态,直接得到控制权
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
				* 有写锁正在等待,则一起等待,以使写锁能获得竞争机会.
				*/
				++lock->_rwaitingCount;
				//ResetEvent(lock->_ev);
				LeaveCriticalSection(&(lock->_lock));
				/*
				* 虽然 LeaveCriticalSection() 和 WaitForSingleObject() 之间有一个时间窗口,
				* 但是由于windows平台的事件信号是不会丢失的,所以没有问题.
				*/
				WaitForSingleObject(lock->_ev, INFINITE);

				EnterCriticalSection(&(lock->_lock));
				ResetEvent(lock->_ev);
				LeaveCriticalSection(&(lock->_lock));
				/*
				* 等待返回,继续尝试加锁.
				*/
				n_isWaitReturn = 1;
			}
			else
			{
				/*
				* 得到读锁,计数+1
				*/
				++lock->_rlockCount;
				LeaveCriticalSection(&(lock->_lock));
				break;
			}
		}
		else if (lock->_st == RWLOCK_W)
		{
			/*
			* 等待写锁释放
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
			* 等待返回,继续尝试加锁.
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
		/* 读锁解锁 */
		--lock->_rlockCount;
		if (0 == lock->_rlockCount)
		{
			lock->_st = RWLOCK_IDLE;
			/* 释放 */
			if (lock->_wwaitingCount > 0 || lock->_rwaitingCount > 0)
			{
				/*
				* 此时有锁请求正在等待,激活所有等待的线程.(手动事件).
				* 使这些请求重新竞争锁.
				*/
				SetEvent(lock->_ev);
			}
			else
			{
				/* 空闲 */
			}
		}
		else
		{
			/* 还有读锁 */
		}
	}
	else
	{
		lock->_st = RWLOCK_IDLE;
		/* 写锁解锁 */
		if (lock->_wwaitingCount > 0 || lock->_rwaitingCount > 0)
		{
			/*
			* 如果在占有互斥量_stLock的情况下,触发事件,那么可能会使一些锁请求不能得到竞争机会.
			* 假设调用unlock时,另一个线程正好调用rlock或者wlock.如果不释放互斥量,只有之前已经等待的锁请求有机会获得锁控制权.
			*/
			SetEvent(lock->_ev);
		}
		else
		{
			/* 空闲 */
		}
	}
	LeaveCriticalSection(&(lock->_lock));
	//OutputDebugStringA("out unLock\n");
}
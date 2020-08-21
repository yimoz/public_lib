#ifndef _PURE_LOCK_H_
#define _PURE_LOCK_H_

typedef void* pure_lock_t;
typedef struct pure_wrlock_t pure_wrlock_t, pure_wrlock, *pure_wrlock_p;

pure_lock_t pure_lock_create();

void pure_lock_free(pure_lock_t lock);

void pure_lock(pure_lock_t lock);

void pure_unlock(pure_lock_t lock);

//write&read lock
pure_wrlock_p pure_wrlock_create();
void pure_wrlock_free(pure_wrlock_p lock);
void pure_r_lock(pure_wrlock_p lock);
void pure_w_lock(pure_wrlock_p lock);
void pure_wr_unlock(pure_wrlock_p lock);
#endif //_PURE_LOCK_H_
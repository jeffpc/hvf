#ifndef __SPINLOCK_H
#define __SPINLOCK_H

/*
 * Heavily based on Linux's spinlock implementation
 */

typedef struct {
	volatile unsigned int lock;
} spinlock_t;

#define SPIN_LOCK_UNLOCKED	(spinlock_t) { 0 }

static inline int __compare_and_swap(volatile unsigned int *lock,
		unsigned int old, unsigned int new)
{
	asm volatile(
		"	cs	%0,%3,%1"
		: "=d" (old), "=Q" (*lock)
		: "0" (old), "d" (new), "Q" (*lock)
		: "cc", "memory" );
	return old;
}

#define spin_is_locked(x) ((x)->lock != 0)

#define SPIN_RETRY 1000

extern void __spin_lock_wait(spinlock_t *lp, unsigned int pc);
extern int __spin_trylock_retry(spinlock_t *lp, unsigned int pc);

/**
 * spin_lock - lock a spinlock
 * @lock:	lock to lock
 */
static inline void spin_lock(spinlock_t *lock)
{
	unsigned long pc = 1 | (unsigned long) __builtin_return_address(0);

	if (unlikely(__compare_and_swap(&lock->lock, 0, pc) != 0))
		__spin_lock_wait(lock, pc);
}

/**
 * spin_trylock - attempt to try to acquire a lock, but do not spin if
 * acquisition would fail
 * @lock:	lock to lock
 */
static inline int spin_trylock(spinlock_t *lock)
{
	unsigned long pc = 1 | (unsigned long) __builtin_return_address(0);

	if (likely(__compare_and_swap(&lock->lock, 0, pc) == 0))
		return 1;
	return __spin_trylock_retry(lock, pc);
}

/**
 * spin_unlock - unlock a lock
 * @lock:	lock to unlock
 */
static inline void spin_unlock(spinlock_t *lock)
{
	__compare_and_swap(&lock->lock, lock->lock, 0);
}

extern void spin_lock_intsave(spinlock_t *lock, unsigned long *mask);
extern void spin_unlock_intrestore(spinlock_t *lock, unsigned long mask);

static inline void spin_double_lock(spinlock_t *l1, spinlock_t *l2)
{
	if (l1 < l2) {
		spin_lock(l1);
		spin_lock(l2);
	} else {
		spin_lock(l2);
		spin_lock(l1);
	}
}

static inline void spin_double_unlock(spinlock_t *l1, spinlock_t *l2)
{
	if (l1 < l2) {
		spin_unlock(l2);
		spin_unlock(l1);
	} else {
		spin_unlock(l1);
		spin_unlock(l2);
	}
}

#endif

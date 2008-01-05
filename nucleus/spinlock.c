#include <interrupt.h>
#include <spinlock.h>

void __spin_lock_wait(spinlock_t *lp, unsigned int pc)
{
	while (1) {
		if (__compare_and_swap(&lp->lock, 0, pc) == 0)
			return;
	}
}

int __spin_trylock_retry(spinlock_t *lp, unsigned int pc)
{
	int count = SPIN_RETRY;

	while (count-- > 0)
		if (__compare_and_swap(&lp->lock, 0, pc) == 0)
			return 1;
	return 0;
}

/**
 * spin_lock_intsave - disable interrupts and lock
 * @lock:	lock to lock
 * @mask:	pointer to where interrupt mask will be stored
 */
void spin_lock_intsave(spinlock_t *lock, unsigned long *mask)
{
	*mask = local_int_disable();
	spin_lock(lock);
}

/**
 * spin_unlock_intrestore - unlock and restore interrupt flags flags
 * @lock:	lock to unlock
 * @mask:	mask to restore
 */
void spin_unlock_intrestore(spinlock_t *lock, unsigned long mask)
{
        spin_unlock(lock);
        local_int_restore(mask);
}


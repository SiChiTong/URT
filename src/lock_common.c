/*
 * Copyright (C) 2013-2014  Shahbaz Youssefi <ShabbyX@gmail.com>
 *
 * This file is part of URT, Unified Real-Time Interface.
 *
 * URT is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * URT is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with URT.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "urt_internal.h"

#if URT_BI_SPACE

#ifdef __KERNEL__
# include <linux/semaphore.h>
#else
# include <sys/types.h>
# include <sys/stat.h>
# include <fcntl.h>
#endif

#include <urt_log.h>

/* global semaphore is a native semaphore in kernel space (already exported).  It is accessed through a sysfs file from user-space */
#ifndef __KERNEL__
static int urt_global_sem = -1;
#endif

int urt_global_sem_get(const char *name)
{
#ifdef __KERNEL__
	return 0;
#else
	if (urt_global_sem >= 0)
		close(urt_global_sem);
	urt_global_sem = open("/sys/urt"URT_SUFFIX"/global_sem", O_WRONLY);
	return urt_global_sem >= 0?0:errno;
#endif
}

void urt_global_sem_free(const char *name)
{
#ifdef __KERNEL__
#else
	if (urt_global_sem >= 0)
		close(urt_global_sem);
	urt_global_sem = -1;
#endif
}

void urt_global_sem_wait(void)
{
#ifdef __KERNEL__
	if (down_interruptible(&urt_global_sem))
		urt_err("error: global sem wait interrupted\n");
#else
	char command = URT_GLOBAL_SEM_WAIT;
	write(urt_global_sem, &command, 1);
#endif
}

void urt_global_sem_try_wait(void)
{
#ifdef __KERNEL__
	down_trylock(&urt_global_sem);
#else
	char command = URT_GLOBAL_SEM_TRY_WAIT;
	write(urt_global_sem, &command, 1);
#endif
}

void urt_global_sem_post(void)
{
#ifdef __KERNEL__
	up(&urt_global_sem);
#else
	char command = URT_GLOBAL_SEM_POST;
	write(urt_global_sem, &command, 1);
#endif
}

#endif

/* common lock to lockf translation */

static bool _lock_stop(volatile void *stop)
{
	return *(volatile sig_atomic_t *)stop;
}

int (urt_sem_wait)(urt_sem *sem, volatile sig_atomic_t *stop, ...)
{
	return urt_sem_waitf(sem, stop?_lock_stop:NULL, stop);
}
URT_EXPORT_SYMBOL(urt_sem_wait);

int (urt_mutex_lock)(urt_mutex *mutex, volatile sig_atomic_t *stop, ...)
{
	return urt_mutex_lockf(mutex, stop?_lock_stop:NULL, stop);
}
URT_EXPORT_SYMBOL(urt_mutex_lock);

int (urt_rwlock_read_lock)(urt_rwlock *rwl, volatile sig_atomic_t *stop, ...)
{
	return urt_rwlock_read_lockf(rwl, stop?_lock_stop:NULL, stop);
}
URT_EXPORT_SYMBOL(urt_rwlock_read_lock);

int (urt_rwlock_write_lock)(urt_rwlock *rwl, volatile sig_atomic_t *stop, ...)
{
	return urt_rwlock_write_lockf(rwl, stop?_lock_stop:NULL, stop);
}
URT_EXPORT_SYMBOL(urt_rwlock_write_lock);

int (urt_cond_wait)(urt_cond *cond, urt_mutex *mutex, volatile sig_atomic_t *stop, ...)
{
	return urt_cond_waitf(cond, mutex, stop?_lock_stop:NULL, stop);
}
URT_EXPORT_SYMBOL(urt_cond_wait);
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

#ifndef URT_TASK_H
#define URT_TASK_H

#include "urt_stdtypes.h"
#include "urt_compiler.h"
#include "urt_defaults.h"
#include "urt_time.h"

typedef struct urt_task_attr
{
	urt_time period;	/* period of task (zero for non-periodic) */
	urt_time start_time;	/* start time of task */
	size_t stack_size;	/* size of task stack (zero for default) */
	int priority;		/* priority of task (zero for minimum priority) */
	bool uses_fpu;		/* whether task would need to use FPU */
	bool soft;		/* whether task is soft real-time */
} urt_task_attr;

#include "urt_sys_task.h"

URT_DECL_BEGIN

URT_ATTR_WARN_UNUSED urt_task *(urt_task_new)(void (*func)(urt_task *, void *), void *data,
		const urt_task_attr *attr, int *error, ...);
void urt_task_delete(urt_task *task);

int urt_task_start(urt_task *task);
void urt_task_wait_period(urt_task *task);
/* urt_time urt_task_next_period(urt_task *task); */
/* urt_time urt_task_period_time_left(urt_task *task); */

/* bool urt_priority_is_valid(int p); */
/* bool urt_priority_is_higher(int a, int b) */

/* urt_priority gives the nth highest priority */
URT_INLINE int urt_priority(unsigned int n)
{
	return URT_MAX_PRIORITY - URT_MORE_PRIORITY * n;
}

URT_DECL_END

#endif

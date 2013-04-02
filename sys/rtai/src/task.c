/*
 * Copyright (C) 2013  Shahbaz Youssefi <ShabbyX@gmail.com>
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

#include <urt_internal.h>
#include <urt_task.h>
#include <urt_mem.h>
#include <urt_utils.h>
#include <urt_log.h>

void urt_task_delete(urt_task *task)
{
#ifdef __KERNEL__
	rt_task_delete(&task->rt_task);
#else
	rt_task_delete(task->rt_task);
	rt_thread_join(task->tid);
#endif
	urt_mem_delete(task);
}
URT_EXPORT_SYMBOL(urt_task_delete);

#ifdef __KERNEL__
static void _task_wrapper(long t)
#else
static void *_task_wrapper(void *t)
#endif
{
	urt_task *task = (void *)t;

	if (task->attr.start_time == 0)
		task->attr.start_time = urt_get_time();
	else
		urt_sleep(task->attr.start_time - urt_get_time());

#ifndef __KERNEL__
	if (task->attr.period > 0)
	{
		char name[URT_NAME_LEN + 1];
		if (urt_get_free_name(name) != URT_SUCCESS)
			return NULL;
		if ((task->rt_task = rt_task_init_schmod(nam2num(name), task->attr.priority,
				task->attr.stack_size, 0, SCHED_FIFO, 0xff)) == NULL)
			return NULL;
	}
	rt_make_hard_real_time();
#endif

	task->func(task, task->data);

#ifndef __KERNEL__
	rt_make_soft_real_time();
	rt_task_delete(NULL);

	return NULL;
#endif
}

int urt_task_start(urt_task *task)
{
#ifdef __KERNEL__
	int ret = rt_task_init(&task->rt_task, _task_wrapper, (long)task, task->attr.stack_size, task->attr.priority, task->attr.uses_fpu, NULL);
	if (ret)
		goto exit_bad_init;

	if (task->attr.period > 0)
		ret = rt_task_make_periodic(&task->rt_task, nano2count(task->attr.start_time), nano2count(task->attr.period));
	else
		ret = rt_task_resume(&task->rt_task);

	return ret?URT_FAIL:URT_SUCCESS;
exit_bad_init:
	if (ret == -EINVAL)
		urt_err("internal error: urt_task_new should have made sure parameters to urt_task_start are correct\n");
	return ret == -ENOMEM?URT_NO_MEM:URT_FAIL;
#else
	task->tid = rt_thread_create((void *)_task_wrapper, task, task->attr.stack_size);
	return task->tid?URT_SUCCESS:errno == EAGAIN?URT_AGAIN:URT_FAIL;
#endif
}
URT_EXPORT_SYMBOL(urt_task_start);

void urt_task_wait_period(urt_task *task)
{
	rt_task_wait_period();
}
URT_EXPORT_SYMBOL(urt_task_wait_period);

#ifndef __KERNEL__
urt_time urt_task_next_period(urt_task *task)
{
	urt_time cur;
	urt_task_attr *attr = &task->attr;

	/* make sure task is periodic */
	if (URT_UNLIKELY(attr->period <= 0))
		return 0;

	cur = urt_get_time();
	if (cur > attr->start_time)
		attr->start_time += (cur - attr->start_time + attr->period - 1) / attr->period * attr->period;

	return attr->start_time;
}
#endif

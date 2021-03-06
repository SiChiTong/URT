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

#include <urt.h>

URT_MODULE_LICENSE("GPL");
URT_MODULE_AUTHOR("Shahbaz Youssefi");
URT_MODULE_DESCRIPTION("sem test: main");

int test_arg = 0;			/* must be set to 123456 */
bool test_arg2 = 2;			/* must be set to 1 */
char *test_arg3[3] = {NULL};		/* must be set to "abc def", "ghi" */
unsigned int test_arg3_count = 0;	/* must be set to 2 */
char *sem_name = NULL;

URT_MODULE_PARAM_START()
URT_MODULE_PARAM(test_arg, int, "test argument")
URT_MODULE_PARAM(test_arg2, bool, "another argument")
URT_MODULE_PARAM_ARRAY(test_arg3, charp, &test_arg3_count, "last argument")
URT_MODULE_PARAM(sem_name, charp, "sem name")
URT_MODULE_PARAM_END()

static int test_start(int *unused);
static void test_body(int *unused);
static void test_end(int *unused);

URT_GLUE(test_start, test_body, test_end, int, interrupted, done)

static urt_sem *sem = NULL;
static urt_task *check_task = NULL;

static void _check(urt_task *task, void *data)
{
	int i;
	urt_out("main: waiting for 3 seconds\n");
	urt_sleep(3000000000ll);
	for (i = 0; i < 15; ++i)
		urt_sem_post(sem);
	done = 1;
}

static void _cleanup(void)
{
	urt_task_delete(check_task);
	urt_shsem_delete(sem);
	urt_exit();
}

static int test_start(int *unused)
{
	int ret;

	if (sem_name == NULL)
	{
		urt_out("Missing obligatory argument <sem_name=name>\n");
		return EXIT_FAILURE;
	}

	urt_out("main: starting test...\n");
	if (test_arg != 123456 || test_arg2 != 1 || test_arg3_count != 2 || test_arg3[0] == NULL || test_arg3[1] == NULL
			|| strcmp(test_arg3[0], "abc def") != 0 || strcmp(test_arg3[1], "ghi") != 0)
	{
		urt_out("main: bad arguments %d, %d and {%s, %s, %s}@%u\n", test_arg, test_arg2,
				test_arg3[0]?test_arg3[0]:"", test_arg3[1]?test_arg3[1]:"", test_arg3[2]?test_arg3[2]:"", test_arg3_count);
		return EXIT_FAILURE;
	}
	ret = urt_init();
	if (ret)
	{
		urt_out("main: init returned %d\n", ret);
		goto exit_no_init;
	}
	sem = urt_shsem_new(sem_name, 5);
	if (sem == NULL)
	{
		urt_out("main: no shared sem\n");
		goto exit_no_sem;
	}
	urt_out("main: sem allocated\n");
	return 0;
exit_no_sem:
	_cleanup();
exit_no_init:
	return EXIT_FAILURE;
}

static void test_body(int *unused)
{
	check_task = urt_task_new(_check);
	if (check_task == NULL)
	{
		urt_out("main: failed to create task\n");
		goto exit_no_task;
	}
	urt_task_start(check_task);
	return;
exit_no_task:
	done = 1;
}

static void test_end(int *unused)
{
	_cleanup();
	urt_out("main: test done\n");
}

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

#ifndef URT_SETUP_H
#define URT_SETUP_H

#include "urt_compiler.h"
#include "urt_consts.h"
#include <urt_sys_setup.h>

URT_DECL_BEGIN

int URT_ATTR_WARN_UNUSED urt_init(void);
void urt_exit(void);
void urt_recover(void);

/*
 * helper functions for tools
 *
 * print_names		These functions output information regarding internal states of URT.
 *			`urt_init` and `urt_exit` need to be called by the tool itself.
 * force_clear_name	Make the name available, cleaning up as much as possible.
 */
void urt_print_names(void);
void urt_force_clear_name(const char *name);

/*
 * helper macros for abstracting from kernel space and user space execution model.
 *
 * The URT_MODULE_* macros define the name and author of the module, used only in kernel
 * modules.
 *
 * The URT_GLUE macro takes five parameters:
 * - init: A small function to be called on program start.  In kernel space, this is
 *   the __init function.
 * - body: A non-real-time thread (which is the main thread in user space) that handles
 *   allocation of resources and various other operations.  The purpose of this function
 *   is to allow the init function to finish, which in kernel space signals the end of insmod.
 *   This way, rmmod can be called asynchronous to the execution of body.
 *   After the execution of body, the program waits for interrupt (user space) or
 *   rmmod (kernel space) before calling the exit function.
 * - exit: A function to be called on program exit.  In kernel space, this is
 *   the __exit funcion.
 * - interrupted: the name of a variable (defined by this macro) that would become non-zero
 *   when the program is interrupted (user space) or rmmod'ed (kernel space).
 * - done: the name of a variable (defined by this macro) that should be set by the user
 *   (in body for example) which indicates that it is safe to call the exit function.
 *
 * URT_GLUE_NO_INTERRUPT is similar to URT_GLUE, except that it does not install an interrupt
 * handler.  This is useful if the user wants to install her own interrupt handler.
 */
#ifdef __KERNEL__

#define URT_MODULE_LICENSE(...) MODULE_LICENSE(__VA_ARGS__)
#define URT_MODULE_AUTHOR(...) MODULE_AUTHOR(__VA_ARGS__)

#define urt_app_body_helper_(body)			\
static int urt_app_body_(void *arg)			\
{							\
	body();						\
	do_exit(0);					\
	return 0;					\
}

#define urt_app_init_helper_(init, body)		\
static int __init urt_app_init_(void)			\
{							\
	int err;					\
	struct task_struct *thread;			\
	err = init();					\
	if (err)					\
		return err;				\
	if ((thread = kthread_run(urt_app_body_, NULL,	\
			#body)) == ERR_PTR(-ENOMEM))	\
	{						\
		urt_err("error: could not create"	\
			"thread for body.  "		\
			"Calling it from __init\n");	\
		body();					\
	}						\
	return 0;					\
}

# define URT_GLUE(init, body, exit, interrupted, done)	\
static int interrupted = 0;				\
static int done = 0;					\
urt_app_body_helper_(body)				\
urt_app_init_helper_(init, body)			\
static void __exit urt_app_exit_(void)			\
{							\
	interrupted = 1;				\
	while (!done)					\
		urt_sleep(10000000);			\
	exit();						\
}							\
module_init(urt_app_init_);				\
module_exit(urt_app_exit_);

# define URT_GLUE_NO_INTERRUPT(init, body, exit)	\
urt_app_body_helper_(body)				\
urt_app_init_helper_(init, body)			\
static void __exit urt_app_exit_(void)			\
{							\
	exit();						\
}							\
module_init(urt_app_init_);				\
module_exit(urt_app_exit_);

#else /* !__KERNEL__ */

#define URT_MODULE_LICENSE(...)
#define URT_MODULE_AUTHOR(...)

# define URT_GLUE(init, body, exit, interrupted, done)	\
static volatile sig_atomic_t interrupted = 0;		\
static int done = 0;					\
static void urt_sig_handler_(int signum)		\
{							\
	interrupted = 1;				\
}							\
int main(int argc, char **argv)				\
{							\
	int err;					\
	/* set signal handler */			\
	struct sigaction sa;				\
	sa = (struct sigaction){.sa_handler = NULL};	\
	sa.sa_handler = urt_sig_handler_;		\
	sigemptyset(&sa.sa_mask);			\
	sigaction(SIGINT, &sa, NULL);			\
	sigaction(SIGHUP, &sa, NULL);			\
	sigaction(SIGTERM, &sa, NULL);			\
	sigaction(SIGQUIT, &sa, NULL);			\
	sigaction(SIGUSR1, &sa, NULL);			\
	sigaction(SIGUSR2, &sa, NULL);			\
	err = init();					\
	if (err)					\
		return err;				\
	body();						\
	/* wait for done */				\
	while (!done)					\
		usleep(10000);				\
	exit();						\
	return 0;					\
}

# define URT_GLUE_NO_INTERRUPT(init, body, exit)	\
int main(int argc, char **argv)				\
{							\
	int err = init();				\
	if (err)					\
		return err;				\
	body();						\
	exit();						\
	return 0;					\
}
#endif /* __KERNEL__ */

/*
 * TODO: add URT_DEFINE_ARG(type, name) so in kernel it would define a parameter and in user space it
 * would parse --name value from argv.  Not all types need to be supported, the ones supported by
 * kernel are enough
 */

URT_DECL_END

#endif

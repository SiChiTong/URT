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

#ifndef URT_SYS_LOG_H
#define URT_SYS_LOG_H

#ifdef __KERNEL__
# include <linux/kernel.h>
#endif
#include "urt_sys_rtai.h"

#ifdef __KERNEL__
# define urt_log_cont(f, ...) rt_printk(__VA_ARGS__)
#else
# define urt_log_cont(f, ...) rt_printk(__VA_ARGS__)
/* # define urt_log_cont(f, ...) do { if (f) fprintf(f, __VA_ARGS__); } while (0) */
#endif

#endif

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

#ifndef URT_SYS_MEM_H
#define URT_SYS_MEM_H

#ifdef __KERNEL__
# include <linux/vmalloc.h>
#else
# include <stdlib.h>
#endif
#include <urt_debug.h>

URT_DECL_BEGIN

URT_INLINE void urt_mem_delete(void *mem)
{
	URT_CHECK_NONRT_CONTEXT();

#ifdef __KERNEL__
	vfree(mem);
#else
	free(mem);
#endif
}

URT_DECL_END

#endif

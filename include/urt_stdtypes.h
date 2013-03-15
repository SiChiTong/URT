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

#ifndef URT_STDTYPES_H
#define URT_STDTYPES_H

#include "urt_config.h"

#if defined(URT_SYS_LINUX) || defined(URT_SYS_QNX) || defined(URT_SYS_VXWORKS) || defined(URT_SYS_RTAI_USER)
# include <stdbool.h>
# include <stdint.h>
#elif defined(URT_SYS_RTAI_KERNEL)
# include <kernel.h>
#endif

#endif
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

#ifndef URT_COMPILER_H
#define URT_COMPILER_H

#ifdef __GNUC__
# define URT_ATTR_PRINTF(format_index, args_index) __attribute__ ((format (printf, format_index, args_index)))
# define URT_ATTR_MALLOC __attribute__((malloc))
# define URT_ATTR_WARN_UNUSED __attribute__((warn_unused_result))
# define URT_ATTR_PURE __attribute__((pure))
/* Note: don't use LIKELY/UNLIKELY macros when all the `if` does is a goto */
# define URT_LIKELY(x) __builtin_expect(!!(x), 1)
# define URT_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
# define URT_ATTR_PRINTF(format_index, args_index)
# define URT_ATTR_MALLOC
# define URT_ATTR_WARN_UNUSED
# define URT_ATTR_PURE
# define URT_LIKELY(x) (x)
# define URT_UNLIKELY(x) (x)
#endif

#ifdef __cplusplus
# define URT_DECL_BEGIN extern "C" {
# define URT_DECL_END }
#else
# define URT_DECL_BEGIN
# define URT_DECL_END
#endif

#ifdef __KERNEL__
# define URT_EXPORT_SYMBOL(s) EXPORT_SYMBOL_GPL(s)
#else
# define URT_EXPORT_SYMBOL(s)
#endif

/*
 * in user-space, we are interested in having external linkage for inline functions, e.g., for python
 * bindings.  In user-space, one can define header functions as `inline` and have an `extern inline`
 * definition in a single .c file.  The linker would then do the right thing.  That approach however,
 * doesn't work in kernel space.  First, the inline semantics of gnu89 and gnu99/C99 differ, so in the
 * least, -std=gnu99 needs to be added to the kernel flags, which is not a problem.  Second, the
 * compiler uses __ilog2_u64, __ilog2_u32 and is_power_of_2 to perform do_div() which are defined as
 * static and therefore cannot be used in an inline function.  As a result, inline functions in
 * kernel space are simply made `static inline`.
 */
#ifdef __KERNEL__
# define URT_INLINE static inline
#else
# define URT_INLINE inline
#endif

#endif

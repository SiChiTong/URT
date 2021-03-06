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

#include <urt_std.h>
#include <urt_log.h>
#include "urt_internal.h"
#include <urt_sys_internal.h>
#include <urt_mem.h>

static bool _name_eq(const char *n1, const char *n2)
{
	unsigned int len = 0;

	/* n1 is taken from the registry, so it should be always NUL-terminated.  The check for length is redundant */
	while (len < URT_NAME_LEN && *n1 != '\0' && *n1 == *n2)
	{
		++n1;
		++n2;
		++len;
	}

	/* the check for length here is to make sure n2 is not accessed beyond URT_NAME_LEN */
	return len >= URT_NAME_LEN || (*n1 == '\0' && *n2 == '\0');
}

static void _name_cpy(char *to, const char *from)
{
	unsigned int i;

	for (i = 0; i < URT_NAME_LEN; ++i)
		if (from[i] == '\0')
			break;
		else
			to[i] = from[i];

	to[i] = '\0';
}

int urt_registry_init(void)
{
	int i;

	if (urt_global_mem->initialized)
		return urt_global_mem->sizeof_urt_internal == sizeof(struct urt_internal)
			&& urt_global_mem->sizeof_urt_registered_object == sizeof(struct urt_registered_object)?0:-1;

	*urt_global_mem = (urt_internal){
		.sizeof_urt_internal = sizeof(struct urt_internal),
		.sizeof_urt_registered_object = sizeof(struct urt_registered_object),
		.initialized = true,
	};

	for (i = 0; i < URT_NAME_LEN - 1; ++i)
		urt_global_mem->next_free_name[i] = '_';
	urt_global_mem->next_free_name[URT_NAME_LEN - 1] = '$';

	return 0;
}

static urt_registered_object *_find_by_name(const char *name)
{
	unsigned int i;
	urt_registered_object *ro = NULL;
	unsigned int max_index = urt_global_mem->objects_max_index;

	for (i = 0; i <= max_index; ++i)
	{
		ro = &urt_global_mem->objects[i];
		if ((ro->reserved || ro->count != 0) && _name_eq(ro->name, name))
			break;
	}

	return i > max_index?NULL:ro;
}

urt_registered_object *urt_reserve_name(const char *name, int *error)
{
	unsigned int i;
	urt_registered_object *obj = NULL;

	urt_global_sem_wait();

	/* make sure name doesn't exist */
	if (_find_by_name(name) != NULL)
		goto exit_exists;

	/* find a free space for the name */
	for (i = 0; i < URT_MAX_OBJECTS; ++i)
	{
		obj = &urt_global_mem->objects[i];
		if (!obj->reserved && obj->count == 0)
			break;
	}
	if (i == URT_MAX_OBJECTS)
		goto exit_max_reached;

	*obj = (urt_registered_object){
		.reserved = true
	};
	_name_cpy(obj->name, name);
	if (i > urt_global_mem->objects_max_index)
		urt_global_mem->objects_max_index = i;

	urt_global_sem_post();

	return obj;
exit_exists:
	if (error)
		*error = EEXIST;
	goto exit_fail;
exit_max_reached:
	if (error)
		*error = ENOSPC;
exit_fail:
	urt_global_sem_post();
	return NULL;
}

void urt_inc_name_count(urt_registered_object *ro)
{
	urt_global_sem_wait();
	++ro->count;
	ro->reserved = false;
	urt_global_sem_post();
}

#ifndef NDEBUG
static void _internal_mem_check(void)
{
	unsigned int i;
	bool error_given = false;

	for (i = 0; i < URT_MAX_OBJECTS; ++i)
	{
		urt_registered_object *obj = &urt_global_mem->objects[i];
		if ((obj->reserved || obj->count > 0) && i > urt_global_mem->objects_max_index)
		{
			urt_global_mem->objects_max_index = i;
			if (!error_given)
			{
				urt_err("internal error: objects_max_index too low");
				error_given = true;
			}
		}
	}
}
#else
# define _internal_mem_check() ((void)0)
#endif

static void _dec_count_common(urt_registered_object *ro,
		void (*release)(struct urt_registered_object *, void *, void *),
		void *address, void *user_data)
{
	unsigned int index = ro - urt_global_mem->objects;
	if (ro->count > 0)
		--ro->count;
	ro->reserved = false;

	/* take care of object cleanup */
	if (release)
		release(ro, address, user_data);

	/* if others still attached to this object, don't continue with cleanup */
	if (ro->count > 0)
		return;

	/* if removing max_index used, lower max_index used */
	if (URT_UNLIKELY(index == urt_global_mem->objects_max_index))
	{
		while (index > 0 && !urt_global_mem->objects[index].reserved
			&& urt_global_mem->objects[index].count == 0)
			--index;
		urt_global_mem->objects_max_index = index;
	}

	_internal_mem_check();
}

void urt_dec_name_count(urt_registered_object *ro,
		void (*release)(struct urt_registered_object *, void *, void *),
		void *address, void *user_data)
{
	urt_global_sem_wait();
	_dec_count_common(ro, release, address, user_data);
	urt_global_sem_post();
}

void urt_force_clear_name(const char *name)
{
	urt_registered_object *ro = NULL;

	urt_global_sem_wait();
	ro = _find_by_name(name);
	/* cleanup the related resources, too */
	if (ro)
	{
		urt_sys_force_clear_name(ro);
		do
		{
			_dec_count_common(ro, NULL, NULL, NULL);
		} while (ro->count > 0);
	}
	urt_global_sem_post();
}

urt_registered_object *urt_get_object_by_name_and_inc_count(const char *name)
{
	urt_registered_object *ro = NULL;

	urt_global_sem_wait();
	ro = _find_by_name(name);
	if (ro)
	{
		++ro->count;
		ro->reserved = false;
	}
	urt_global_sem_post();

	return ro;
}

static int _increment_name(char *name)
{
	int i;
	char digit;

	for (i = URT_NAME_LEN - 2; i >= 0; --i)
	{
		bool finished = true;
		digit = name[i];

		if (digit == '_')
			digit = '0';
		else if (digit >= '0' && digit < '9')
			++digit;
		else if (digit == '9')
			digit = 'A';
		else if (digit >= 'A' && digit < 'Z')
			++digit;
		else if (digit == 'Z')
		{
			digit = '_';
			finished = false;
		}
		else
			goto exit_internal;

		name[i] = digit;
		if (finished)
			break;
	}

	if (URT_UNLIKELY(i < 0))
	{
		for (i = 0; i < URT_NAME_LEN - 1; ++i)
			name[i] = '_';
		name[URT_NAME_LEN - 1] = '$';
		return 1;
	}

	return 0;
exit_internal:
	urt_err("internal error: next_free_name contains invalid character '%c' (%d)\n", digit, digit);
	for (i = 0; i < URT_NAME_LEN - 1; ++i)
		name[i] = '_';
	name[URT_NAME_LEN - 1] = '$';
	return -1;
}

static void _check_and_get_free_name(char *name)
{
	char *next_name;
	int ret;

	next_name = urt_global_mem->next_free_name;

	do
	{
		strncpy(name, next_name, URT_NAME_LEN);
		ret = _increment_name(next_name);
	} while (ret < 0 || _find_by_name(name) != NULL);
}

/*
 * free names have 5 characters in [A-Z0-9_] and a terminating $.
 * Since users are not supposed to use $, this will not collide with
 * user names. Therefore, no actual checking with the registry is made.
 */
int urt_get_free_name(char *name)
{
	int ret;
	char *next_name;

	urt_global_sem_wait();

	if (URT_UNLIKELY(urt_global_mem->names_exhausted))
		_check_and_get_free_name(name);
	else
	{
		next_name = urt_global_mem->next_free_name;
		strncpy(name, next_name, URT_NAME_LEN);
		ret = _increment_name(next_name);
		if (ret)
			urt_global_mem->names_exhausted = true;
		if (ret < 0)
			goto exit_fail;
	}

	urt_global_sem_post();
	return 0;
exit_fail:
	urt_global_sem_post();
	return ENOSPC;
}
URT_EXPORT_SYMBOL(urt_get_free_name);

void urt_print_names(void)
{
	unsigned int i;
	urt_registered_object *obj = NULL;

	urt_global_sem_wait();

	urt_out_cont("  index  | ");
	for (i = 0; i < URT_NAME_LEN / 2 - 2; ++i)
		urt_out_cont(" ");
	urt_out_cont("name");
	for (i = 0; i < URT_NAME_LEN - URT_NAME_LEN / 2 - 2; ++i)
		urt_out_cont(" ");
	urt_out_cont(" |  count  | reserved | bookkeeping | size (bytes) |  type\n");
	urt_out_cont("---------+-");
	for (i = 0; i < URT_NAME_LEN; ++i)
		urt_out_cont("-");
	urt_out_cont("-+---------+----------+-------------+--------------+--------\n");
	for (i = 0; i < URT_MAX_OBJECTS; ++i)
	{
		obj = &urt_global_mem->objects[i];
		if (!obj->reserved && obj->count == 0)
			continue;
		urt_out_cont(" %7u | %*s | %7u | %8s | %11s | %12zu | %6s\n", i, URT_NAME_LEN, obj->name, obj->count,
				obj->reserved?"Yes":"No", obj->has_bookkeeping?"Yes":"No",
				obj->size - (obj->has_bookkeeping?16:0),
				obj->type == URT_TYPE_MEM?"MEM ":
				obj->type == URT_TYPE_SEM?"SEM ":
				obj->type == URT_TYPE_MUTEX?"MUTEX":
				obj->type == URT_TYPE_RWLOCK?"RWLOCK":
				obj->type == URT_TYPE_COND?"COND":"??????");
	}
	urt_out_cont("\nmax index: %u\n", urt_global_mem->objects_max_index);
	urt_out_cont("next free name: %*s%s\n", URT_NAME_LEN, urt_global_mem->next_free_name,
			urt_global_mem->names_exhausted?" (cycled)":"");

	urt_global_sem_post();
}

void urt_dump_memory(const char *name, size_t start, size_t end)
{
	urt_registered_object *ro = NULL;
	char type;
	size_t size;
	void *obj;
	int error;
	size_t i, j;

	urt_global_sem_wait();
	ro = _find_by_name(name);
	if (!ro)
		urt_out_cont("No such name '%s'\n", name);
	else
	{
		type = ro->type;
		size = ro->size;
		if (ro->has_bookkeeping)
			size -= 16;
	}
	urt_global_sem_post();

	if (ro == NULL)
		return;

	/* attach to the object to get its address mapped into this process */
	switch (type)
	{
	case URT_TYPE_MEM:
		obj = urt_shmem_attach(name, &error);
		break;
	case URT_TYPE_SEM:
		obj = urt_shsem_attach(name, &error);
		break;
	case URT_TYPE_MUTEX:
		obj = urt_shmutex_attach(name, &error);
		break;
	case URT_TYPE_RWLOCK:
		obj = urt_shrwlock_attach(name, &error);
		break;
	case URT_TYPE_COND:
		obj = urt_shcond_attach(name, &error);
		break;
	default:
		urt_err("internal error: bad object type %d for name '%s'\n", type, name);
		return;
	}

	if (obj == NULL)
	{
		urt_out_cont("failed to attach to object with name '%s' (error: %d)\n", name, error);
		return;
	}

	/* limit the result to sane values */
	if (start > size)
		start = size;
	if (end == 0 || end > size)
		end = size;

	/* dump the object */
	urt_out_cont("Displaying contents of '%s':\n", name);
	for (i = start; i < end; i += 16)
	{
		urt_out_cont("%08zX:", i);

		/* hex codes */
		for (j = 0; j < 16 && i + j < end; ++j)
			urt_out_cont("%s %02X", j == 8?" ":"", ((unsigned char *)obj)[i + j]);
		for (; j < 16; ++j)
			urt_out_cont("%s   ", j == 8?" ":"");
		urt_out_cont("  ");

		/* printable characters */
		for (j = 0; j < 16 && i + j < end; ++j)
		{
			char c = ((char *)obj)[i + j];
			urt_out_cont("%c", isalnum(c)?c:'.');
		}
		urt_out_cont("\n");
	}

	/* detach from the object */
	switch (type)
	{
	case URT_TYPE_MEM:
		urt_shmem_detach(obj);
		break;
	case URT_TYPE_SEM:
		urt_shsem_detach(obj);
		break;
	case URT_TYPE_MUTEX:
		urt_shmutex_detach(obj);
		break;
	case URT_TYPE_RWLOCK:
		urt_shrwlock_detach(obj);
		break;
	case URT_TYPE_COND:
		urt_shcond_detach(obj);
		break;
	default:
		break;
	}
}

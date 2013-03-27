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

#include <urt_string.h>
#include <urt_log.h>
#include "urt_internal.h"

static bool _name_eq(const char *n1, const char *n2)
{
	while (*n1 != '\0' && *n1 == *n2)
	{
		++n1;
		++n2;
	}

	return *n1 == '\0' && *n2 == '\0';
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

void urt_registry_init(void)
{
	unsigned int i;

	if (urt_global_mem->initialized)
		return;
	urt_global_mem->initialized = true;

	urt_global_mem->objects_max_index = 0;
	for (i = 0; i < URT_MAX_OBJECTS; ++i)
		urt_global_mem->objects[i] = (urt_registered_object){
			.name[0] = '\0',
			.reserved = false,
			.count = 0,
			.address = NULL
		};

	urt_global_mem->next_free_name[0] = '_';
	urt_global_mem->next_free_name[1] = '_';
	urt_global_mem->next_free_name[2] = '_';
	urt_global_mem->next_free_name[3] = '_';
	urt_global_mem->next_free_name[4] = '_';
	urt_global_mem->next_free_name[5] = '$';
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

static urt_registered_object *_find_by_addr(void *address)
{
	unsigned int i;
	urt_registered_object *ro = NULL;
	unsigned int max_index = urt_global_mem->objects_max_index;

	for (i = 0; i <= max_index; ++i)
	{
		ro = &urt_global_mem->objects[i];
		if ((ro->reserved || ro->count != 0) && ro->address == address)
			break;
	}

	return i > max_index?NULL:ro;
}

urt_registered_object *urt_reserve_name(const char *name, int *error)
{
	unsigned int i;
	urt_registered_object *obj = NULL;

	urt_sem_wait(urt_global_sem);

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

	obj->reserved = true;
	_name_cpy(obj->name, name);
	if (i > urt_global_mem->objects_max_index)
		urt_global_mem->objects_max_index = i;

	urt_sem_post(urt_global_sem);

	return obj;
exit_exists:
	if (error)
		*error = URT_EXISTS;
	goto exit_fail;
exit_max_reached:
	if (error)
		*error = URT_MAX_REACHED;
exit_fail:
	urt_sem_post(urt_global_sem);
	return NULL;
}

void urt_inc_name_count(urt_registered_object *ro)
{
	urt_sem_wait(urt_global_sem);
	++ro->count;
	urt_sem_post(urt_global_sem);
}

static inline void _dec_count_common(urt_registered_object *ro)
{
	unsigned int index = ro - urt_global_mem->objects;
	if (URT_LIKELY(ro->count > 0))
		--ro->count;
	ro->reserved = false;
	/* if removing max_index used, lower max_index used */
	if (URT_UNLIKELY(index == urt_global_mem->objects_max_index))
	{
		while (index > 0 && !urt_global_mem->objects[index].reserved
			&& urt_global_mem->objects[index].count == 0)
			--index;
		urt_global_mem->objects_max_index = index;
	}
}

void urt_dec_name_count(urt_registered_object *ro)
{
	urt_sem_wait(urt_global_sem);
	_dec_count_common(ro);
	urt_sem_post(urt_global_sem);
}

void urt_deregister_name(const char *name)
{
	urt_registered_object *ro = NULL;

	urt_sem_wait(urt_global_sem);
	ro = _find_by_name(name);
	if (ro)
		_dec_count_common(ro);
	urt_sem_post(urt_global_sem);
}

void urt_deregister_addr(void *address)
{
	urt_registered_object *ro = NULL;

	urt_sem_wait(urt_global_sem);
	ro = _find_by_addr(address);
	if (ro)
		_dec_count_common(ro);
	urt_sem_post(urt_global_sem);
}

urt_registered_object *urt_get_object_by_name(const char *name)
{
	urt_registered_object *ro = NULL;

	urt_sem_wait(urt_global_sem);
	ro = _find_by_name(name);
	urt_sem_post(urt_global_sem);

	return ro;
}

urt_registered_object *urt_get_object_by_addr(void *address)
{
	urt_registered_object *ro = NULL;

	urt_sem_wait(urt_global_sem);
	ro = _find_by_addr(address);
	urt_sem_post(urt_global_sem);

	return ro;
}

static int _increment_name(char *name)
{
	int i;
	char digit;

	for (i = URT_NAME_LEN - 1; i >= 0; --i)
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
		name[0] = '_';
		name[1] = '_';
		name[2] = '_';
		name[3] = '_';
		name[4] = '_';
		name[5] = '$';
		return 1;
	}

	return 0;
exit_internal:
	urt_err("internal error: next_free_name contains invalid character '%c' (%d)\n", digit, digit);
	name[0] = '_';
	name[1] = '_';
	name[2] = '_';
	name[3] = '_';
	name[4] = '_';
	name[5] = '$';
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

	urt_sem_wait(urt_global_sem);

	if (URT_UNLIKELY(urt_global_mem->names_exhausted))
		_check_and_get_free_name(name);
	else
	{
		next_name = urt_global_mem->next_free_name;
		strncpy(name, next_name, URT_NAME_LEN);
		ret = _increment_name(next_name);
		if (URT_UNLIKELY(ret))
			urt_global_mem->names_exhausted = true;
		if (URT_UNLIKELY(ret < 0))
			goto exit_fail;
	}

	urt_sem_post(urt_global_sem);
	return URT_SUCCESS;
exit_fail:
	urt_sem_post(urt_global_sem);
	return URT_FAIL;
}
URT_EXPORT_SYMBOL(urt_get_free_name);

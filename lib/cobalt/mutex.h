/* SPDX-License-Identifier: LGPL-2.0-or-later */

/*
 * Copyright (C) 2026 Florian Bezdeka <florian.bezdeka@siemens.com>.
 * Copyright (C) 2005 Philippe Gerum <rpm@xenomai.org>.
 */

#ifndef _LIB_COBALT_MUTEX_H
#define _LIB_COBALT_MUTEX_H

#include <boilerplate/ancillaries.h>
#include <cobalt/uapi/mutex.h>

#include <pthread.h>

extern pthread_mutex_t *const cobalt_autoinit_mutex;

/**
 * Test if a mutex structure contains a valid autoinitializer.
 *
 * @return the mutex type on success,
 * @return -1 if not in supported autoinitializer state
 */
static int __attribute__((cold))
cobalt_mutex_autoinit_type(const pthread_mutex_t *mutex)
{
	static const pthread_mutex_t mutex_initializers[] = {
#if HAVE_DECL_PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP
		PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP,
#endif
#if HAVE_DECL_PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP
		PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP,
#endif
		PTHREAD_MUTEX_INITIALIZER
	};
	static const int mutex_types[] = {
#if HAVE_DECL_PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP
		PTHREAD_MUTEX_ERRORCHECK_NP,
#endif
#if HAVE_DECL_PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP
		PTHREAD_MUTEX_RECURSIVE_NP,
#endif
		PTHREAD_MUTEX_DEFAULT
	};
	int i;

	for (i = ARRAY_SIZE(mutex_types); i > 0; --i) {
		if (memcmp(mutex, &mutex_initializers[i - 1],
			   sizeof(mutex_initializers[0])) == 0)
			return mutex_types[i - 1];
	}
	return -1;
}

static int __attribute__((cold))
cobalt_mutex_doautoinit(union cobalt_mutex_union *umutex)
{
	struct cobalt_mutex_shadow *_mutex = &umutex->shadow_mutex;
	int err __attribute__((unused));
	pthread_mutexattr_t mattr;
	int ret = 0, type;

	type = cobalt_mutex_autoinit_type(&umutex->native_mutex);
	if (type < 0)
		return EINVAL;

	pthread_mutexattr_init(&mattr);
	pthread_mutexattr_settype(&mattr, type);
	err = __COBALT(pthread_mutex_lock(cobalt_autoinit_mutex));
	if (err) {
		ret = err;
		goto out;
	}
	if (_mutex->magic != COBALT_MUTEX_MAGIC)
		ret = __COBALT(
			pthread_mutex_init(&umutex->native_mutex, &mattr));
	err = __COBALT(pthread_mutex_unlock(cobalt_autoinit_mutex));
	if (err) {
		if (ret == 0)
			ret = err;
	}

out:
	pthread_mutexattr_destroy(&mattr);

	return ret;
}

static inline int cobalt_mutex_autoinit(union cobalt_mutex_union *umutex)
{
	if (umutex->shadow_mutex.magic != COBALT_MUTEX_MAGIC)
		return cobalt_mutex_doautoinit(umutex);

	return 0;
}

#endif //_LIB_COBALT_MUTEX_H

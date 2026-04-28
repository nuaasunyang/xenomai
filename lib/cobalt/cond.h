/* SPDX-License-Identifier: LGPL-2.0-or-later */

/*
 * Copyright (C) 2026 Florian Bezdeka <florian.bezdeka@siemens.com>.
 * Copyright (C) 2005 Philippe Gerum <rpm@xenomai.org>.
 */

#ifndef _LIB_COBALT_COND_H
#define _LIB_COBALT_COND_H

#include "current.h"
#include "internal.h"

#include <asm/xenomai/syscall.h>
#include <cobalt/uapi/cond.h>
#include <pthread.h>
#include <string.h>

struct cobalt_cond_cleanup_t {
	struct cobalt_cond_shadow *cond;
	struct cobalt_mutex_shadow *mutex;
	unsigned count;
	int err;
};

static int __attribute__((cold))
cobalt_cond_autoinit_type(const pthread_cond_t *cond)
{
	static const pthread_cond_t cond_initializer = PTHREAD_COND_INITIALIZER;

	return memcmp(cond, &cond_initializer, sizeof(cond_initializer)) == 0 ?
		       0 :
		       -1;
}

static int __attribute__((cold))
cobalt_cond_doautoinit(union cobalt_cond_union *ucond)
{
	if (cobalt_cond_autoinit_type(&ucond->native_cond) < 0)
		return EINVAL;

	return __COBALT(pthread_cond_init(&ucond->native_cond, NULL));
}

static inline int cobalt_cond_autoinit(union cobalt_cond_union *ucond)
{
	if (ucond->shadow_cond.magic != COBALT_COND_MAGIC)
		return cobalt_cond_doautoinit(ucond);
	return 0;
}

static void __pthread_cond_cleanup(void *data)
{
	struct cobalt_cond_cleanup_t *c = (struct cobalt_cond_cleanup_t *)data;
	xnhandle_t cur = cobalt_get_current();
	int err;

	/* if we still own the mutex, cond_wait_prologue wasn't called yet */
	if (xnsynch_fast_owner_check(mutex_get_ownerp(c->mutex), cur) == 0)
		return;

	do {
		err = XENOMAI_SYSCALL2(sc_cobalt_cond_wait_epilogue, c->cond,
				       c->mutex);
	} while (err == -EINTR);

	c->mutex->lockcnt = c->count;
}

static inline int do_sc_cond_wait_prologue(struct cobalt_cond_shadow *cnd,
					   struct cobalt_mutex_shadow *mx,
					   int *err, int timed,
					   const struct timespec *abstime)
{
#ifdef __USE_TIME_BITS64
	long sc_nr = sc_cobalt_cond_wait_prologue64;
#else
	long sc_nr = sc_cobalt_cond_wait_prologue;
#endif

	return XENOMAI_SYSCALL5(sc_nr, cnd, mx, err, timed, abstime);
}

#endif //_LIB_COBALT_COND_H

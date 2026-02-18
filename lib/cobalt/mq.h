/* SPDX-License-Identifier: LGPL-2.0-or-later */

/*
 * Copyright (C) 2026 Florian Bezdeka <florian.bezdeka@siemens.com>.
 */

#ifndef _LIB_COBALT_MQ_H
#define _LIB_COBALT_MQ_H

#include <asm/xenomai/syscall.h>

#include <mqueue.h>

static inline int __do_mq_timedreceive(mqd_t q, char *buffer, ssize_t *len,
				       unsigned int *prio,
				       const struct timespec *timeout)
{
#ifdef __USE_TIME_BITS64
	long sc_nr = sc_cobalt_mq_timedreceive64;
#else
	long sc_nr = sc_cobalt_mq_timedreceive;
#endif

	return XENOMAI_SYSCALL5(sc_nr, q, buffer, len, prio, timeout);
}

static inline int __do_mq_timesend(mqd_t q, const char *buffer, size_t len,
				   unsigned int prio, const struct timespec *to)
{
#ifdef __USE_TIME_BITS64
	long sc_nr = sc_cobalt_mq_timedsend64;
#else
	long sc_nr = sc_cobalt_mq_timedsend;
#endif

	return XENOMAI_SYSCALL5(sc_nr, q, buffer, len, prio, to);
}

#endif //_LIB_COBALT_MQ_H

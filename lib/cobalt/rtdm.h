/* SPDX-License-Identifier: LGPL-2.0-or-later */

/*
 * Copyright (C) 2026 Florian Bezdeka <florian.bezdeka@siemens.com>.
 */

#ifndef _LIB_COBALT_RTDM_H
#define _LIB_COBALT_RTDM_H

#include <asm/xenomai/syscall.h>

#include <errno.h>
#include <pthread.h>

static int set_errno(int ret)
{
	if (ret >= 0)
		return ret;

	errno = -ret;
	return -1;
}

static int do_ioctl(int fd, unsigned int request, void *arg)
{
	int ret, oldtype;

	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &oldtype);

	ret = XENOMAI_SYSCALL3(sc_cobalt_ioctl,	fd, request, arg);

	pthread_setcanceltype(oldtype, NULL);

	return ret;
}

#endif //_LIB_COBALT_RTDM_H

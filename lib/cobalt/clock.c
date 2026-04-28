/*
 * Copyright (C) 2005 Philippe Gerum <rpm@xenomai.org>.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
 */

#include "internal.h"

/**
 * @ingroup cobalt_api
 * @defgroup cobalt_api_time Clocks and timers
 *
 * Cobalt/POSIX clock and timer services
 *
 * Cobalt supports three built-in clocks:
 *
 * CLOCK_REALTIME maps to the nucleus system clock, keeping time as the amount
 * of time since the Epoch, with a resolution of one nanosecond.
 *
 * CLOCK_MONOTONIC maps to an architecture-dependent high resolution
 * counter, so is suitable for measuring short time
 * intervals. However, when used for sleeping (with
 * clock_nanosleep()), the CLOCK_MONOTONIC clock has a resolution of
 * one nanosecond, like the CLOCK_REALTIME clock.
 *
 * CLOCK_MONOTONIC_RAW is Linux-specific, and provides monotonic time
 * values from a hardware timer which is not adjusted by NTP. This is
 * strictly equivalent to CLOCK_MONOTONIC with Cobalt, which is not
 * NTP adjusted either.
 *
 * In addition, external clocks can be dynamically registered using
 * the cobalt_clock_register() service. These clocks are fully managed
 * by Cobalt extension code, which should advertise each incoming tick
 * by calling xnclock_tick() for the relevant clock, from an interrupt
 * context.
 *
 * Timer objects may be created with the timer_create() service using
 * any of the built-in or external clocks. The resolution of these
 * timers is clock-specific. However, built-in clocks all have
 * nanosecond resolution, as specified for clock_nanosleep().
 *
 * @see
 * <a href="http://www.opengroup.org/onlinepubs/000095399/functions/xsh_chap02_08.html#tag_02_08_05">
 * Specification.</a>
 *
 *@{
 */

/* @apitags{thread-unrestricted, switch-primary} */

COBALT_IMPL(unsigned int, sleep, (unsigned int seconds))
{
	struct timespec rqt, rem;
	int ret;

	if (cobalt_get_current_fast() == XN_NO_HANDLE)
		return __STD(sleep(seconds));

	rqt.tv_sec = seconds;
	rqt.tv_nsec = 0;
	ret = __WRAP(clock_nanosleep(CLOCK_MONOTONIC, 0, &rqt, &rem));
	if (ret)
		return rem.tv_sec;

	return 0;
}

/* @apitags{thread-unrestricted, switch-primary} */

COBALT_IMPL(int, usleep, (useconds_t usec))
{
	struct timespec rqt;

	if (cobalt_get_current_fast() == XN_NO_HANDLE)
		return __STD(usleep(usec));

	rqt.tv_sec = usec / 1000000;
	rqt.tv_nsec = (usec % 1000000) * 1000;
	return __WRAP(clock_nanosleep(CLOCK_MONOTONIC, 0, &rqt, NULL));
}

/** @} */

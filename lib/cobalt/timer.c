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

#include <asm/xenomai/syscall.h>

#include <errno.h>
#include <time.h>

/**
 * @addtogroup cobalt_api_time
 * @{
 */

/**
 * @fn int timer_create(clockid_t clockid, const struct sigevent *__restrict__ evp, timer_t * __restrict__ timerid)
 * @brief Create a timer
 *
 * This service creates a timer based on the clock @a clockid.
 *
 * If @a evp is not @a NULL, it describes the notification mechanism
 * used on timer expiration. Only thread-directed notification is
 * supported (evp->sigev_notify set to @a SIGEV_THREAD_ID).
 *
 * If @a evp is NULL, the current Cobalt thread will receive the
 * notifications with signal SIGALRM.
 *
 * The recipient thread is delivered notifications when it calls any
 * of the sigwait(), sigtimedwait() or sigwaitinfo() services.
 *
 * If this service succeeds, an identifier for the created timer is
 * returned at the address @a timerid. The timer is unarmed until
 * started with the timer_settime() service.
 *
 * @param clockid clock used as a timing base;
 *
 * @param evp description of the asynchronous notification to occur
 * when the timer expires;
 *
 * @param timerid address where the identifier of the created timer
 * will be stored on success.
 *
 * @retval 0 on success;
 * @retval -1 with @a errno set if:
 * - EINVAL, the clock @a clockid is invalid;
 * - EINVAL, the member @a sigev_notify of the @b sigevent structure at the
 *   address @a evp is not SIGEV_THREAD_ID;
 * - EINVAL, the  member @a sigev_signo of the @b sigevent structure is an
 *   invalid signal number;
 * - EAGAIN, the maximum number of timers was exceeded, recompile with a larger
 *   value.
 *
 * @see
 * <a href="http://www.opengroup.org/onlinepubs/000095399/functions/timer_create.html">
 * Specification.</a>
 *
 * @apitags{thread-unrestricted}
 */
COBALT_IMPL(int, timer_create, (clockid_t clockid,
				const struct sigevent *__restrict__ evp,
				timer_t * __restrict__ timerid))
{
	int ret;

	ret = -XENOMAI_SYSCALL3(sc_cobalt_timer_create,	clockid, evp, timerid);
	if (ret == 0)
		return 0;

	errno = ret;

	return -1;
}

/**
 * Delete a timer object.
 *
 * This service deletes the timer @a timerid.
 *
 * @param timerid identifier of the timer to be removed;
 *
 * @retval 0 on success;
 * @retval -1 with @a errno set if:
 * - EINVAL, @a timerid is invalid;
 * - EPERM, the timer @a timerid does not belong to the current process.
 *
 * @see
 * <a href="http://www.opengroup.org/onlinepubs/000095399/functions/timer_delete.html">
 * Specification.</a>
 *
 * @apitags{thread-unrestricted}
 */
COBALT_IMPL(int, timer_delete, (timer_t timerid))
{
	int ret;

	ret = -XENOMAI_SYSCALL1(sc_cobalt_timer_delete, timerid);
	if (ret == 0)
		return 0;

	errno = ret;

	return -1;
}

/**
 * Get expiration overruns count since the most recent timer expiration
 * signal delivery.
 *
 * This service returns @a timerid expiration overruns count since the most
 * recent timer expiration signal delivery. If this count is more than @a
 * DELAYTIMER_MAX expirations, @a DELAYTIMER_MAX is returned.
 *
 * @param timerid Timer identifier.
 *
 * @return the overruns count on success;
 * @return -1 with @a errno set if:
 * - EINVAL, @a timerid is invalid;
 * - EPERM, the timer @a timerid does not belong to the current process.
 *
 * @see
 * <a href="http://www.opengroup.org/onlinepubs/000095399/functions/timer_getoverrun.html">
 * Specification.</a>
 *
 * @apitags{unrestricted}
 */
COBALT_IMPL(int, timer_getoverrun, (timer_t timerid))
{
	int overrun;

	overrun = XENOMAI_SYSCALL1(sc_cobalt_timer_getoverrun, timerid);
	if (overrun >= 0)
		return overrun;

	errno = -overrun;

	return -1;
}

/** @} */

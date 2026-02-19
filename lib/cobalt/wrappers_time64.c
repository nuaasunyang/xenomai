// SPDX-License-Identifier: LGPL-2.0-or-later

/*
 * Copyright (C) 2026 Florian Bezdeka <florian.bezdeka@siemens.com>.
 * Copyright (C) 2010 Gilles Chanteperdrix <gilles.chanteperdrix@xenomai.org>.
 * Copyright (C) 2005 Philippe Gerum <rpm@xenomai.org>.
 */

/*
 * Magic ahead:
 * This file is compiled twice in case y2038/time64_t support is necessary
 * and requested. The first time with time64_t and a second time with native
 * time_t.
 */

#include "cond.h"
#include "internal.h"
#include "mq.h"
#include "mutex.h"
#include "rtdm.h"

#include <asm/xenomai/syscall.h>
#include <cobalt/uapi/syscall.h>
#include <rtdm/rtdm.h>

#include <errno.h>
#include <mqueue.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>

/**
 * Get the resolution of the specified clock.
 * @ingroup cobalt_api_time
 *
 * This service returns, at the address @a res, if it is not @a NULL, the
 * resolution of the clock @a clock_id.
 *
 * For both CLOCK_REALTIME and CLOCK_MONOTONIC, this resolution is the duration
 * of one system clock tick. No other clock is supported.
 *
 * @param clock_id clock identifier, either CLOCK_REALTIME or CLOCK_MONOTONIC;
 *
 * @param tp the address where the resolution of the specified clock will be
 * stored on success.
 *
 * @retval 0 on success;
 * @retval -1 with @a errno set if:
 * - EINVAL, @a clock_id is invalid;
 *
 * @see
 * <a href="http://www.opengroup.org/onlinepubs/000095399/functions/clock_getres.html">
 * Specification.</a>
 *
 * @apitags{unrestricted}
 */
COBALT_IMPL_TIME64(int, clock_getres, __clock_getres64,
		   (clockid_t clock_id, struct timespec *tp))
{
	int ret;

#ifdef __USE_TIME_BITS64
	ret = -XENOMAI_SYSCALL2(sc_cobalt_clock_getres64, clock_id, tp);
#else
	ret = -XENOMAI_SYSCALL2(sc_cobalt_clock_getres, clock_id, tp);
#endif

	if (ret) {
		errno = ret;
		return -1;
	}

	return 0;
}

static int __do_clock_gettime(clockid_t clock_id, struct timespec *tp)
{
#ifdef __USE_TIME_BITS64
	return -XENOMAI_SYSCALL2(sc_cobalt_clock_gettime64, clock_id, tp);
#else
	return -XENOMAI_SYSCALL2(sc_cobalt_clock_gettime, clock_id, tp);
#endif
}

/**
 * Read the specified clock.
 * @ingroup cobalt_api_time
 *
 * This service returns, at the address @a tp the current value of the clock @a
 * clock_id. If @a clock_id is:
 * - CLOCK_REALTIME, the clock value represents the amount of time since the
 *   Epoch, with a precision of one system clock tick;
 * - CLOCK_MONOTONIC or CLOCK_MONOTONIC_RAW, the clock value is given
 *   by an architecture-dependent high resolution counter, with a
 *   precision independent from the system clock tick duration.
 * - CLOCK_HOST_REALTIME, the clock value as seen by the host, typically
 *   Linux. Resolution and precision depend on the host, but it is guaranteed
 *   that both, host and Cobalt, see the same information.
 *
 * @param clock_id clock identifier, either CLOCK_REALTIME, CLOCK_MONOTONIC,
 *        or CLOCK_HOST_REALTIME;
 *
 * @param tp the address where the value of the specified clock will be stored.
 *
 * @retval 0 on success;
 * @retval -1 with @a errno set if:
 * - EINVAL, @a clock_id is invalid.
 *
 * @see
 * <a href="http://www.opengroup.org/onlinepubs/000095399/functions/clock_gettime.html">
 * Specification.</a>
 *
 * @apitags{unrestricted}
 */
COBALT_IMPL_TIME64(int, clock_gettime, __clock_gettime64,
		   (clockid_t clock_id, struct timespec *tp))
{
	int ret;

	switch (clock_id) {
	case CLOCK_REALTIME:
	case CLOCK_HOST_REALTIME:
		ret = __cobalt_vdso_gettime(CLOCK_REALTIME, tp);
		break;
	case CLOCK_MONOTONIC:
	case CLOCK_MONOTONIC_RAW:
		ret = __cobalt_vdso_gettime(clock_id, tp);
		break;
	default:
		ret = __do_clock_gettime(clock_id, tp);
	}

	if (ret) {
		errno = ret;
		return -1;
	}

	return 0;
}

/**
 * Set the specified clock.
 * @ingroup cobalt_api_time
 *
 * Set the CLOCK_REALTIME or Cobalt-specific clocks.
 *
 * @param clock_id the id of the clock to be set. CLOCK_REALTIME,
 * and Cobalt-specific clocks are supported.
 *
 * @param tp the address of a struct timespec specifying the new date.
 *
 * @retval 0 on success;
 * @retval -1 with @a errno set if:
 * - EINVAL, @a clock_id is undefined;
 * - EINVAL, the date specified by @a tp is invalid.
 *
 * @see
 * <a href="http://www.opengroup.org/onlinepubs/000095399/functions/clock_settime.html">
 * Specification.</a>
 *
 * @note Setting CLOCK_REALTIME may cause the caller to switch to
 * secondary mode.
 *
 * @apitags{unrestricted, switch-secondary}
 */
COBALT_IMPL_TIME64(int, clock_settime, __clock_settime64,
		   (clockid_t clock_id, const struct timespec *tp))
{
	int ret;

	if (clock_id == CLOCK_REALTIME)
		return __STD(clock_settime(CLOCK_REALTIME, tp));

#ifdef __USE_TIME_BITS64
	ret = -XENOMAI_SYSCALL2(sc_cobalt_clock_settime64, clock_id, tp);
#else
	ret = -XENOMAI_SYSCALL2(sc_cobalt_clock_settime, clock_id, tp);
#endif
	if (ret) {
		errno = ret;
		return -1;
	}

	return 0;
}

/**
 * @ingroup cobalt_api_time
 * @apitags{unrestricted}
 */
COBALT_IMPL_TIME64(int, clock_adjtime, __clock_adjtime64,
		   (clockid_t clock_id, struct timex *tx))
{
	int ret;

#ifdef XN_USE_TIME64_SYSCALL
	ret = -XENOMAI_SYSCALL2(sc_cobalt_clock_adjtime64, clock_id, tx);
#else
	ret = -XENOMAI_SYSCALL2(sc_cobalt_clock_adjtime, clock_id, tx);
#endif

	if (ret < 0) {
		errno = ret;
		return -1;
	}

	return ret;
}

/**
 * Sleep some amount of time.
 * @ingroup cobalt_api_time
 *
 * This service suspends the calling thread until the wakeup time specified by
 * @a rqtp, or a signal is delivered to the caller. If the flag TIMER_ABSTIME is
 * set in the @a flags argument, the wakeup time is specified as an absolute
 * value of the clock @a clock_id. If the flag TIMER_ABSTIME is not set, the
 * wakeup time is specified as a time interval.
 *
 * If this service is interrupted by a signal, the flag TIMER_ABSTIME is not
 * set, and @a rmtp is not @a NULL, the time remaining until the specified
 * wakeup time is returned at the address @a rmtp.
 *
 * The resolution of this service is one system clock tick.
 *
 * @param clock_id clock identifier, either CLOCK_REALTIME or CLOCK_MONOTONIC.
 *
 * @param flags one of:
 * - 0 meaning that the wakeup time @a rqtp is a time interval;
 * - TIMER_ABSTIME, meaning that the wakeup time is an absolute value of the
 *   clock @a clock_id.
 *
 * @param rqtp address of the wakeup time.
 *
 * @param rmtp address where the remaining time before wakeup will be stored if
 * the service is interrupted by a signal.
 *
 * @return 0 on success;
 * @return an error number if:
 * - EPERM, the caller context is invalid;
 * - ENOTSUP, the specified clock is unsupported;
 * - EINVAL, the specified wakeup time is invalid;
 * - EINTR, this service was interrupted by a signal.
 *
 * @see
 * <a href="http://www.opengroup.org/onlinepubs/000095399/functions/clock_nanosleep.html">
 * Specification.</a>
 *
 * @apitags{xthread-only, switch-primary}
 */
COBALT_IMPL_TIME64(int, clock_nanosleep, __clock_nanosleep_time64,
		   (clockid_t clock_id, int flags, const struct timespec *rqtp,
		    struct timespec *rmtp))
{
	int ret, oldtype;

	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &oldtype);

#ifdef __USE_TIME_BITS64
	ret = -XENOMAI_SYSCALL4(sc_cobalt_clock_nanosleep64, clock_id, flags,
				rqtp, rmtp);
#else
	ret = -XENOMAI_SYSCALL4(sc_cobalt_clock_nanosleep, clock_id, flags,
				rqtp, rmtp);
#endif

	pthread_setcanceltype(oldtype, NULL);

	return ret;
}

/**
 * Sleep some amount of time.
 * @ingroup cobalt_api_time
 *
 * This service suspends the calling thread until the wakeup time specified by
 * @a rqtp, or a signal is delivered. The wakeup time is specified as a time
 * interval.
 *
 * If this service is interrupted by a signal and @a rmtp is not @a NULL, the
 * time remaining until the specified wakeup time is returned at the address @a
 * rmtp.
 *
 * The resolution of this service is one system clock tick.
 *
 * @param rqtp address of the wakeup time.
 *
 * @param rmtp address where the remaining time before wakeup will be stored if
 * the service is interrupted by a signal.
 *
 * @retval 0 on success;
 * @retval -1 with @a errno set if:
 * - EPERM, the caller context is invalid;
 * - EINVAL, the specified wakeup time is invalid;
 * - EINTR, this service was interrupted by a signal.
 *
 * @see
 * <a href="http://www.opengroup.org/onlinepubs/000095399/functions/nanosleep.html">
 * Specification.</a>
 *
 * @apitags{xthread-only, switch-primary}
 */
COBALT_IMPL_TIME64(int, nanosleep, __nanosleep64,
		   (const struct timespec *rqtp, struct timespec *rmtp))
{
	int ret;

	ret = __WRAP(clock_nanosleep(CLOCK_REALTIME, 0, rqtp, rmtp));
	if (ret) {
		errno = ret;
		return -1;
	}

	return 0;
}

/**
 * @apitags{unrestricted}
 * @ingroup cobalt_api_time
 */
COBALT_IMPL_TIME64(int, gettimeofday, __gettimeofday64,
		   (struct timeval *tv, struct timezone *tz))
{
	struct timespec ts;
	int ret = __WRAP(clock_gettime(CLOCK_REALTIME, &ts));
	if (ret == 0) {
		tv->tv_sec = ts.tv_sec;
		tv->tv_usec = ts.tv_nsec / 1000;
	}
	return ret;
}

/**
 * @apitags{unrestricted}
 * @ingroup cobalt_api_time
 */
COBALT_IMPL_TIME64(time_t, time, __time64, (time_t *t))
{
	struct timespec ts;
	int ret = __WRAP(clock_gettime(CLOCK_REALTIME, &ts));
	if (ret)
		return (time_t)-1;

	if (t)
		*t = ts.tv_sec;
	return ts.tv_sec;
}

/**
 * Wait a bounded time on a condition variable.
 * @ingroup cobalt_api_cond
 *
 * This service is equivalent to pthread_cond_wait(), except that the calling
 * thread remains blocked on the condition variable @a cnd only until the
 * timeout specified by @a abstime expires.
 *
 * The timeout @a abstime is expressed as an absolute value of the @a clock
 * attribute passed to pthread_cond_init(). By default, @a CLOCK_REALTIME is
 * used.
 *
 * @param cond the condition variable to wait for;
 *
 * @param mutex the mutex associated with @a cnd;
 *
 * @param abstime the timeout, expressed as an absolute value of the clock
 * attribute passed to pthread_cond_init().
 *
 * @return 0 on success,
 * @return an error number if:
 * - EPERM, the caller context is invalid;
 * - EPERM, the specified condition variable is not process-shared and does not
 *   belong to the current process;
 * - EINVAL, the specified condition variable, mutex or timeout is invalid;
 * - EINVAL, another thread is currently blocked on @a cnd using another mutex
 *   than @a mx;
 * - EPERM, the specified mutex is not owned by the caller;
 * - ETIMEDOUT, the specified timeout expired.
 *
 * @see
 * <a href="http://www.opengroup.org/onlinepubs/000095399/functions/pthread_cond_timedwait.html">
 * Specification.</a>
 *
 * @apitags{xthread-only, switch-primary}
 */
COBALT_IMPL_TIME64(int, pthread_cond_timedwait, __pthread_cond_timedwait64,
		   (pthread_cond_t *cond, pthread_mutex_t *mutex,
		    const struct timespec *abstime))
{
	struct cobalt_cond_shadow *_cnd =
		&((union cobalt_cond_union *)cond)->shadow_cond;
	struct cobalt_mutex_shadow *_mx =
		&((union cobalt_mutex_union *)mutex)->shadow_mutex;
	struct cobalt_cond_cleanup_t c = {
		.cond = _cnd,
		.mutex = _mx,
	};
	int err, oldtype;

	if (_mx->magic != COBALT_MUTEX_MAGIC)
		return EINVAL;

	err = cobalt_cond_autoinit((union cobalt_cond_union *)cond);
	if (err)
		return err;

	if (_mx->attr.type == PTHREAD_MUTEX_ERRORCHECK) {
		xnhandle_t cur = cobalt_get_current();

		if (cur == XN_NO_HANDLE)
			return EPERM;

		if (xnsynch_fast_owner_check(mutex_get_ownerp(_mx), cur))
			return EPERM;
	}

	c.count = _mx->lockcnt;

	pthread_cleanup_push(&__pthread_cond_cleanup, &c);

	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &oldtype);

	err = do_sc_cond_wait_prologue(_cnd, _mx, &c.err, 1, abstime);

	pthread_setcanceltype(oldtype, NULL);

	pthread_cleanup_pop(0);

	while (err == -EINTR) {
		err = XENOMAI_SYSCALL2(sc_cobalt_cond_wait_epilogue, _cnd, _mx);
		pthread_testcancel();
	}

	_mx->lockcnt = c.count;

	return -err ?: -c.err;
}

/**
 * Attempt, during a bounded time, to send a message to a message queue.
 * @ingroup cobalt_api_mq
 *
 * This service is equivalent to mq_send(), except that if the message queue is
 * full and the flag @a O_NONBLOCK is not set for the descriptor @a fd, the
 * calling thread is only suspended until the timeout specified by @a
 * abs_timeout expires.
 *
 * @param q message queue descriptor;
 *
 * @param buffer pointer to the message to be sent;
 *
 * @param len length of the message;
 *
 * @param prio priority of the message;
 *
 * @param timeout the timeout, expressed as an absolute value of the
 * CLOCK_REALTIME clock.
 *
 * @return 0 and send a message on success;
 * @return -1 with no message sent and @a errno set if:
 * - EBADF, @a fd is not a valid message queue descriptor open for writing;
 * - EMSGSIZE, the message length exceeds the @a mq_msgsize attribute of the
 *   message queue;
 * - EAGAIN, the flag O_NONBLOCK is set for the descriptor @a fd and the message
 *   queue is full;
 * - EPERM, the caller context is invalid;
 * - ETIMEDOUT, the specified timeout expired;
 * - EINTR, the service was interrupted by a signal.
 *
 * @see
 * <a href="http://www.opengroup.org/onlinepubs/000095399/functions/mq_timedsend.html">
 * Specification.</a>
 *
 * @apitags{xthread-only, switch-primary}
 */
COBALT_IMPL_TIME64(int, mq_timedsend, __mq_timedsend_time64,
		   (mqd_t q, const char *buffer, size_t len, unsigned prio,
		    const struct timespec *timeout))
{
	int err, oldtype;

	if (timeout == NULL)
		return -EFAULT;

	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &oldtype);

	err = __do_mq_timesend(q, buffer, len, prio, timeout);

	pthread_setcanceltype(oldtype, NULL);

	if (!err)
		return 0;

	errno = -err;
	return -1;
}

/**
 * Attempt, during a bounded time, to receive a message from a message queue.
 * @ingroup cobalt_api_mq
 *
 * This service is equivalent to mq_receive(), except that if the flag @a
 * O_NONBLOCK is not set for the descriptor @a fd and the message queue is
 * empty, the calling thread is only suspended until the timeout @a abs_timeout
 * expires.
 *
 * @param q the queue descriptor;
 *
 * @param buffer the address where the received message will be stored on
 * success;
 *
 * @param len @a buffer length;
 *
 * @param prio address where the priority of the received message will be
 * stored on success.
 *
 * @param timeout the timeout, expressed as an absolute value of the
 * CLOCK_REALTIME clock.
 *
 * @return the message length, and copy a message at the address @a buffer on
 * success;
 * @return -1 with no message unqueued and @a errno set if:
 * - EBADF, @a fd is not a valid descriptor open for reading;
 * - EMSGSIZE, the length @a len is lesser than the message queue @a mq_msgsize
 *   attribute;
 * - EAGAIN, the queue is empty, and the flag @a O_NONBLOCK is set for the
 *   descriptor @a fd;
 * - EPERM, the caller context is invalid;
 * - EINTR, the service was interrupted by a signal;
 * - ETIMEDOUT, the specified timeout expired.
 *
 * @see
 * <a href="http://www.opengroup.org/onlinepubs/000095399/functions/mq_timedreceive.html">
 * Specification.</a>
 *
 * @apitags{xthread-only, switch-primary}
 */
COBALT_IMPL_TIME64(ssize_t, mq_timedreceive, __mq_timedreceive_time64,
		   (mqd_t q, char *__restrict__ buffer, size_t len,
		    unsigned *__restrict__ prio,
		    const struct timespec *__restrict__ timeout))
{
	ssize_t rlen = (ssize_t) len;
	int err, oldtype;

	if (timeout == NULL)
		return -EFAULT;

	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &oldtype);

	err = __do_mq_timedreceive(q, buffer, &rlen, prio, timeout);

	pthread_setcanceltype(oldtype, NULL);

	if (!err)
		return rlen;

	errno = -err;
	return -1;
}

/**
 * Attempt, during a bounded time, to lock a mutex.
 * @ingroup cobalt_api_mutex
 *
 * This service is equivalent to pthread_mutex_lock(), except that if the mutex
 * @a mx is locked by another thread than the current one, this service only
 * suspends the current thread until the timeout specified by @a to expires.
 *
 * @param mutex the mutex to be locked;
 *
 * @param to the timeout, expressed as an absolute value of the CLOCK_REALTIME
 * clock.
 *
 * @return 0 on success;
 * @return an error number if:
 * - EPERM, the caller is not allowed to perform the operation;
 * - EINVAL, the mutex @a mx is invalid;
 * - EPERM, the mutex is not process-shared and does not belong to the current
 *   process;
 * - ETIMEDOUT, the mutex could not be locked and the specified timeout
 *   expired;
 * - EDEADLK, the mutex is of the @a PTHREAD_MUTEX_ERRORCHECK type and the mutex
 *   was already locked by the current thread;
 * - EAGAIN, the mutex is of the @a PTHREAD_MUTEX_RECURSIVE type and the maximum
 *   number of recursive locks has been exceeded.
 *
 * @see
 * <a href="http://www.opengroup.org/onlinepubs/000095399/functions/pthread_mutex_timedlock.html">
 * Specification.</a>
 *
 * @apitags{xthread-only, switch-primary}
 */
static int __pthread_mutex_timedlock(pthread_mutex_t *mutex,
				     const struct timespec *to)
{
	struct cobalt_mutex_shadow *_mutex =
		&((union cobalt_mutex_union *)mutex)->shadow_mutex;
	struct xnthread_user_window *u_window;
	int status, ret, lazy_protect = 0;
	xnhandle_t cur;

	cur = cobalt_get_current();
	if (cur == XN_NO_HANDLE)
		return EPERM;

	ret = cobalt_mutex_autoinit((union cobalt_mutex_union *)mutex);
	if (ret)
		return ret;

	/* See __cobalt_pthread_mutex_lock() */
	status = cobalt_get_current_mode();
	if ((status & (XNRELAX | XNWEAK | XNDEBUG)) == 0) {
		if (_mutex->attr.protocol == PTHREAD_PRIO_PROTECT)
			goto protect;
	fast_path:
		ret = xnsynch_fast_acquire(mutex_get_ownerp(_mutex), cur);
		if (ret == 0) {
			_mutex->lockcnt = 1;
			return 0;
		}
	} else {
	slow_path:
		ret = xnsynch_fast_owner_check(mutex_get_ownerp(_mutex), cur);
		if (ret == 0)
			ret = -EBUSY;
	}

	if (ret == -EBUSY) {
		if (lazy_protect)
			u_window->pp_pending = XN_NO_HANDLE;

		switch (_mutex->attr.type) {
		case PTHREAD_MUTEX_NORMAL:
			break;

		case PTHREAD_MUTEX_ERRORCHECK:
			return EDEADLK;

		case PTHREAD_MUTEX_RECURSIVE:
			if (_mutex->lockcnt == UINT32_MAX)
				return EAGAIN;

			++_mutex->lockcnt;
			return 0;
		}
	}

#ifdef __USE_TIME_BITS64
	ret = XENOMAI_SYSCALL2(sc_cobalt_mutex_timedlock64, _mutex, to);
#else
	ret = XENOMAI_SYSCALL2(sc_cobalt_mutex_timedlock, _mutex, to);
#endif

	if (ret == 0)
		_mutex->lockcnt = 1;
	return -ret;
protect:
	u_window = cobalt_get_current_window();
	/*
	 * Can't nest lazy ceiling requests, have to take the slow
	 * path when this happens.
	 */
	if (u_window->pp_pending != XN_NO_HANDLE)
		goto slow_path;
	u_window->pp_pending = _mutex->handle;
	lazy_protect = 1;
	goto fast_path;
}

COBALT_IMPL_TIME64(int, pthread_mutex_timedlock, __pthread_mutex_timedlock64,
		   (pthread_mutex_t * mutex, const struct timespec *to))
{
	int ret;
	do
		ret = __pthread_mutex_timedlock(mutex, to);
	while (ret == EINTR);
	return ret;
}

#if __USE_TIME_BITS64 && __TIMESIZE == 32
int pthread_timedmutex_lock_interruptible_np_time64(pthread_mutex_t *mutex,
						    const struct timespec *to)
{
	return __pthread_mutex_timedlock(mutex, to);
}
#else
int pthread_timedmutex_lock_interruptible_np(pthread_mutex_t *mutex,
					     const struct timespec *to)
{
	return __pthread_mutex_timedlock(mutex, to);
}
#endif

COBALT_IMPL_TIME64(int, ioctl, __ioctl_time64,
		   (int fd, unsigned int request, ...))
{
	va_list ap;
	void *arg;
	int ret;

	va_start(ap, request);
	arg = va_arg(ap, void *);
	va_end(ap);

	ret = do_ioctl(fd, request, arg);
	if (ret != -EADV && ret != -ENOSYS)
		return set_errno(ret);

	return __STD(ioctl(fd, request, arg));
}

COBALT_IMPL_TIME64(int, recvmmsg, __recvmmsg64,
		   (int fd, struct mmsghdr *msgvec, unsigned int vlen,
		    unsigned int flags, struct timespec *timeout))
{
	int ret, oldtype;

	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &oldtype);

#ifdef __USE_TIME_BITS64
	ret = XENOMAI_SYSCALL5(sc_cobalt_recvmmsg64, fd, msgvec, vlen, flags,
			       timeout);
#else
	ret = XENOMAI_SYSCALL5(sc_cobalt_recvmmsg, fd, msgvec, vlen, flags,
			       timeout);
#endif

	pthread_setcanceltype(oldtype, NULL);

	if (ret != -EADV && ret != -ENOSYS)
		return set_errno(ret);

	return __STD(recvmmsg(fd, msgvec, vlen, flags, timeout));
}

COBALT_IMPL_TIME64(int, getsockopt, __getsockopt64,
		   (int fd, int level, int optname, void *optval,
		    socklen_t *optlen))
{
	struct _rtdm_getsockopt_args args = { level, optname, optval, optlen };
	int ret;

	ret = do_ioctl(fd, _RTIOC_GETSOCKOPT, &args);
	if (ret != -EADV && ret != -ENOSYS)
		return set_errno(ret);

	return __STD(getsockopt(fd, level, optname, optval, optlen));
}

COBALT_IMPL_TIME64(int, setsockopt, __setsockopt64,
		   (int fd, int level, int optname, const void *optval,
		    socklen_t optlen))
{
	struct _rtdm_setsockopt_args args = { level, optname, (void *)optval,
					      optlen };
	int ret;

	ret = do_ioctl(fd, _RTIOC_SETSOCKOPT, &args);
	if (ret != -EADV && ret != -ENOSYS)
		return set_errno(ret);

	return __STD(setsockopt(fd, level, optname, optval, optlen));
}

#if __USE_TIME_BITS64 && __TIMESIZE == 32

#define USEC_PER_SEC	1000000L
#define NSEC_PER_USEC	1000L

/*
 * The time64 wrapper for select() is a little different:
 * There is no y2038 safe syscall for select() itself, but we have pselect()
 * without signal support.
 */
static inline int do_select(int __nfds, fd_set *__restrict __readfds,
			      fd_set *__restrict __writefds,
			      fd_set *__restrict __exceptfds,
			      struct timeval *__restrict __timeout)
{
	struct timespec to;
	int err, oldtype;

	if (__timeout) {
		to.tv_sec =
			__timeout->tv_sec + (__timeout->tv_usec / USEC_PER_SEC);
		to.tv_nsec =
			(__timeout->tv_usec % USEC_PER_SEC) * NSEC_PER_USEC;
	}

	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &oldtype);

	/*
	 * Note: No sigmask here, we already reached the limit of 5
	 * syscall parameters
	 */
	err = XENOMAI_SYSCALL5(sc_cobalt_pselect64, __nfds, __readfds,
			       __writefds, __exceptfds, __timeout ? &to : NULL);

	pthread_setcanceltype(oldtype, NULL);

	if (err == -EADV || err == -EPERM || err == -ENOSYS) {
		err = __STD(__select64(__nfds, __readfds, __writefds,
				       __exceptfds, __timeout));
	} else if (__timeout) {
		__timeout->tv_sec = to.tv_sec;
		__timeout->tv_usec = to.tv_nsec / 1000;
	}

	if (err >= 0)
		return err;

	errno = -err;
	return -1;
}
#else
static inline int do_select(int __nfds, fd_set *__restrict __readfds,
			    fd_set *__restrict __writefds,
			    fd_set *__restrict __exceptfds,
			    struct timeval *__restrict __timeout)
{
	int err, oldtype;

	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &oldtype);

	err = XENOMAI_SYSCALL5(sc_cobalt_select, __nfds,
			       __readfds, __writefds, __exceptfds, __timeout);

	pthread_setcanceltype(oldtype, NULL);

	if (err == -EADV || err == -EPERM || err == -ENOSYS)
		return __STD(select(__nfds, __readfds,
				    __writefds, __exceptfds, __timeout));

	if (err >= 0)
		return err;

	errno = -err;
	return -1;
}
#endif

COBALT_IMPL_TIME64(int, select, __select64,
		   (int __nfds, fd_set *__restrict __readfds,
		    fd_set *__restrict __writefds,
		    fd_set *__restrict __exceptfds,
		    struct timeval *__restrict __timeout))
{
	return do_select(__nfds, __readfds, __writefds, __exceptfds, __timeout);
}

/**
 * @fn int sem_timedwait(sem_t *sem, const struct timespec *abs_timeout)
 * @brief Attempt to decrement a semaphore with a time limit
 * @ingroup cobalt_api_sem
 *
 * This service is equivalent to sem_wait(), except that the caller is only
 * blocked until the timeout @a abs_timeout expires.
 *
 * @param sem the semaphore to be decremented;
 *
 * @param abs_timeout the timeout, expressed as an absolute value of
 * the relevant clock for the semaphore, either CLOCK_MONOTONIC if
 * SEM_RAWCLOCK was mentioned via sem_init_np(), or CLOCK_REALTIME
 * otherwise.
 *
 * @retval 0 on success;
 * @retval -1 with @a errno set if:
 * - EPERM, the caller context is invalid;
 * - EINVAL, the semaphore is invalid or uninitialized;
 * - EINVAL, the specified timeout is invalid;
 * - EPERM, the semaphore @a sm is not process-shared and does not belong to the
 *   current process;
 * - EINTR, the caller was interrupted by a signal while blocked in this
 *   service;
 * - ETIMEDOUT, the semaphore could not be decremented and the
 *   specified timeout expired.
 *
 * @see
 * <a href="http://www.opengroup.org/onlinepubs/000095399/functions/sem_timedwait.html">
 * Specification.</a>
 *
 * @apitags{xthread-only, switch-primary}
 */
COBALT_IMPL_TIME64(int, sem_timedwait, __sem_timedwait64,
		   (sem_t *sem, const struct timespec *abs_timeout))
{
	struct cobalt_sem_shadow *_sem =
		&((union cobalt_sem_union *)sem)->shadow_sem;
	int ret, oldtype;

	ret = __RT(sem_trywait(sem));
	if (ret != -1 || errno != EAGAIN)
		return ret;

	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &oldtype);

#ifdef __USE_TIME_BITS64
	ret = XENOMAI_SYSCALL2(sc_cobalt_sem_timedwait64, _sem, abs_timeout);
#else
	ret = XENOMAI_SYSCALL2(sc_cobalt_sem_timedwait, _sem, abs_timeout);
#endif

	pthread_setcanceltype(oldtype, NULL);

	if (ret) {
		errno = -ret;
		return -1;
	}

	return 0;
}

COBALT_IMPL_TIME64(int, sigtimedwait, __sigtimedwait64,
		   (const sigset_t *set, siginfo_t *si,
		    const struct timespec *timeout))
{
	int ret, oldtype;

	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &oldtype);

#ifdef __USE_TIME_BITS64
	ret = XENOMAI_SYSCALL3(sc_cobalt_sigtimedwait64, set, si, timeout);
#else
	ret = XENOMAI_SYSCALL3(sc_cobalt_sigtimedwait, set, si, timeout);
#endif
	if (ret < 0) {
		errno = -ret;
		ret = -1;
	}

	pthread_setcanceltype(oldtype, NULL);

	return ret;
}

/**
 * @fn timer_settime(timer_t timerid, int flags, const struct itimerspec *__restrict__ value, struct itimerspec *__restrict__ ovalue)
 * @brief Start or stop a timer
 * @ingroup cobalt_api_time
 *
 * This service sets a timer expiration date and reload value of the
 * timer @a timerid. If @a ovalue is not @a NULL, the current
 * expiration date and reload value are stored at the address @a
 * ovalue as with timer_gettime().
 *
 * If the member @a it_value of the @b itimerspec structure at @a
 * value is zero, the timer is stopped, otherwise the timer is
 * started. If the member @a it_interval is not zero, the timer is
 * periodic. The current thread must be a Cobalt thread (created with
 * pthread_create()) and will be notified via signal of timer
 * expirations.
 *
 * When starting the timer, if @a flags is TIMER_ABSTIME, the expiration value
 * is interpreted as an absolute date of the clock passed to the timer_create()
 * service. Otherwise, the expiration value is interpreted as a time interval.
 *
 * Expiration date and reload value are rounded to an integer count of
 * nanoseconds.
 *
 * @param timerid identifier of the timer to be started or stopped;
 *
 * @param flags one of 0 or TIMER_ABSTIME;
 *
 * @param value address where the specified timer expiration date and reload
 * value are read;
 *
 * @param ovalue address where the specified timer previous expiration date and
 * reload value are stored if not @a NULL.
 *
 * @retval 0 on success;
 * @retval -1 with @a errno set if:
 * - EINVAL, the specified timer identifier, expiration date or reload value is
 *   invalid. For @a timerid to be valid, it must belong to the current process.
 *
 * @see
 * <a href="http://www.opengroup.org/onlinepubs/000095399/functions/timer_settime.html">
 * Specification.</a>
 *
 * @apitags{xcontext, switch-primary}
 */
COBALT_IMPL_TIME64(int, timer_settime, __timer_settime64,
		   (timer_t timerid, int flags,
		    const struct itimerspec *__restrict__ value,
		    struct itimerspec *__restrict__ ovalue))
{
	int ret;

#ifdef XN_USE_TIME64_SYSCALL
	long sc_nr = sc_cobalt_timer_settime64;
#else
	long sc_nr = sc_cobalt_timer_settime;
#endif

	ret = -XENOMAI_SYSCALL4(sc_nr, timerid, flags, value, ovalue);
	if (ret == 0)
		return 0;

	errno = ret;

	return -1;
}


/**
 * @fn int timer_gettime(timer_t timerid, struct itimerspec *value)
 * @brief Get timer next expiration date and reload value.
 * @ingroup cobalt_api_time
 *
 * This service stores, at the address @a value, the expiration date
 * (member @a it_value) and reload value (member @a it_interval) of
 * the timer @a timerid. The values are returned as time intervals,
 * and as multiples of the system clock tick duration (see note in
 * section @ref cobalt_api_time "Clocks and timers services" for details
 * on the duration of the system clock tick). If the timer was not
 * started, the returned members @a it_value and @a it_interval of @a
 * value are zero.
 *
 * @param timerid timer identifier;
 *
 * @param value address where the timer expiration date and reload value are
 * stored on success.
 *
 * @retval 0 on success;
 * @retval -1 with @a errno set if:
 * - EINVAL, @a timerid is invalid. For @a timerid to be valid, it
 * must belong to the current process.
 *
 * @see
 * <a href="http://www.opengroup.org/onlinepubs/000095399/functions/timer_gettime.html">
 * Specification.</a>
 *
 * @apitags{unrestricted}
 */
COBALT_IMPL_TIME64(int, timer_gettime, __timer_gettime64,
		   (timer_t timerid, struct itimerspec *value))
{
	int ret;

#ifdef XN_USE_TIME64_SYSCALL
	long sc_nr = sc_cobalt_timer_gettime64;
#else
	long sc_nr = sc_cobalt_timer_gettime;
#endif

	ret = -XENOMAI_SYSCALL2(sc_nr, timerid, value);
	if (ret == 0)
		return 0;

	errno = ret;

	return -1;
}

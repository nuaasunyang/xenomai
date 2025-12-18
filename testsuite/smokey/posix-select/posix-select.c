/*
 * Copyright (C) 2011-2012 Gilles Chanteperdrix <gch@xenomai.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *  
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <mqueue.h>
#include <pthread.h>
#include <boilerplate/atomic.h>
#include <sys/select.h>
#include <sys/types.h>
#include <fcntl.h>
#include <smokey/smokey.h>

smokey_test_plugin(posix_select,
		   SMOKEY_NOARGS,
		   "Check POSIX select service"
);

static const char *tunes[] = {
    "Surfing With The Alien",
    "Lords of Karma",
    "Banana Mango",
    "Psycho Monkey",
    "Luminous Flesh Giants",
    "Moroccan Sunset",
    "Satch Boogie",
    "Flying In A Blue Dream",
    "Ride",
    "Summer Song",
    "Speed Of Light",
    "Crystal Planet",
    "Raspberry Jam Delta-V",
    "Champagne?",
    "Clouds Race Across The Sky",
    "Engines Of Creation"
};

static int test_status;

static void *mq_thread(void *cookie)
{
	mqd_t mqd = (mqd_t)(long)cookie;
	unsigned int i = 0, prio;
	fd_set inset, tmp_inset;
	char buf[128];
	int ret;

	FD_ZERO(&inset);
	FD_SET(mqd, &inset);

	for (;;) {
		tmp_inset = inset;

		ret = smokey_check_errno(select(mqd + 1, &tmp_inset, NULL, NULL, NULL));
		if (ret < 0) {
			test_status = ret;
			break;
		}

		ret = smokey_check_errno(mq_receive(mqd, buf, sizeof(buf), &prio));
		if (ret < 0) {
			test_status = ret;
			break;
		}

		if (strcmp(buf, "/done") == 0)
			break;
	
		if (!smokey_assert(strcmp(buf, tunes[i]) == 0)) {
			test_status = -EINVAL;
			break;
		}

		smokey_trace("received %s", buf);
		i = (i + 1) % (ARRAY_SIZE(tunes));
	}

	return NULL;
}

static int test_posix_select_with_mq(void)
{
	struct mq_attr qa;
	pthread_t tcb;
	int i, j, ret;
	mqd_t mq;

	mq_unlink("/select_test_mq");
	qa.mq_maxmsg = 128;
	qa.mq_msgsize = 128;
	mq = smokey_check_errno(mq_open("/select_test_mq", O_RDWR | O_CREAT | O_NONBLOCK, 0, &qa));
	if (mq < 0)
		return mq;

	ret = smokey_check_status(pthread_create(&tcb, NULL, mq_thread, (void *)(long)mq));
	if (ret)
		return ret;

	for (j = 0; j < 3; j++) {
		for (i = 0; i < ARRAY_SIZE(tunes); i++) {
			ret = smokey_check_errno(mq_send(mq, tunes[i], strlen(tunes[i]) + 1, 0));
			if (ret < 0) {
				smokey_check_status(pthread_cancel(tcb));
				goto out;
			}
			usleep(100000);
		}
	}
	ret = smokey_check_errno(mq_send(mq, "/done", sizeof "/done", 0));
	if (ret < 0) {
		smokey_check_status(pthread_cancel(tcb));
		goto out;
	}
	usleep(300000);
	smp_rmb();
	ret = test_status;
out:
	pthread_join(tcb, NULL);

	mq_unlink("/select_test_mq");

	return ret;
}

static int test_select_timeout(void)
{
	unsigned long diff_in_nsec;
	struct timespec ts1, ts2;
	struct timeval tv;
	int ret;

	/* Invalid timeval, usec too big, normalization to 1 sec, 1 usec */
	tv.tv_sec = 0;
	tv.tv_usec = 1000 * 1000 + 1;

	ret = smokey_check_errno(clock_gettime(CLOCK_MONOTONIC, &ts1));
	if (ret)
		return ret;

	/*
	 * y2038 speciality ahead:
	 * select() services provided by Linux and Cobalt both normalize the
	 * timeout value before using it. pselect() services do not do that.
	 *
	 * As the select() service of libcobalt is routed to the pselect() of
	 * Cobalt we have to do the normalization in userspace. The test below
	 * is exactly stressing that case.
	 */
	ret = select(0, NULL, NULL, NULL, &tv);
	if (!smokey_assert(ret == 0))
		return ret;

	ret = smokey_check_errno(clock_gettime(CLOCK_MONOTONIC, &ts2));
	if (ret)
		return ret;

	/*
	 * Check that the writeback happened as expected. This makes also sure
	 * that the normalization took place.
	 */
	smokey_assert(tv.tv_sec == 1);
	smokey_assert(tv.tv_usec == 1);

	diff_in_nsec = (ts2.tv_sec * 1000 * 1000 * 1000) + ts2.tv_nsec;
	diff_in_nsec -= (ts1.tv_sec * 1000 * 1000 * 1000) + ts1.tv_nsec;

	/* Check that at least 1sec has passed */
	smokey_assert(diff_in_nsec > 1 * 1000 * 1000 * 1000); // 1sec

	return 0;
}

static int run_posix_select(struct smokey_test *t, int argc, char *const argv[])
{
	int ret;

	ret = test_posix_select_with_mq();
	if (ret)
		return ret;

	ret = test_select_timeout();
	if (ret)
		return ret;

	return ret;
}

/*
 * SCHED_QUOTA test.
 *
 * Copyright (C) Philippe Gerum <rpm@xenomai.org>
 *
 * Released under the terms of GPLv2.
 */

#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <pthread.h>
#include <semaphore.h>
#include <sched.h>
#include <errno.h>
#include <error.h>
#include <sys/cobalt.h>
#include <boilerplate/time.h>
#include <boilerplate/ancillaries.h>
#include <copperplate/threadobj.h>
#include <smokey/smokey.h>

smokey_test_plugin(sched_quota,
		   SMOKEY_ARGLIST(
			   SMOKEY_INT(quota),
			   SMOKEY_INT(threads),
		   ),
   "Check the SCHED_QUOTA scheduling policy. Using a pool\n"
   "\tof SCHED_FIFO threads, the code first calibrates, by estimating how\n"
   "\tmuch work the system under test can perform when running\n"
   "\tuninterrupted over a second.\n\n"
   "\tThe same thread pool is re-started afterwards, as a SCHED_QUOTA\n"
   "\tgroup this time, which is allotted a user-definable percentage of\n"
   "\tthe global quota interval (CONFIG_XENO_OPT_SCHED_QUOTA_PERIOD).\n"
   "\tUsing the reference calibration value obtained by running the\n"
   "\tSCHED_FIFO pool, the percentage of runtime consumed by the\n"
   "\tSCHED_QUOTA group over a second is calculated.\n\n"
   "\tA successful test shows that the effective percentage of runtime\n"
   "\tobserved with the SCHED_QUOTA group closely matches the allotted\n"
   "\tquota (barring rounding errors and marginal latency)."
);

#define MAX_THREADS 8
#define PERIOD_NS   10000000	/* 10 ms, default XENO_OPT_SCHED_QUOTA_PERIOD */
#define NR_PERIODS  100

static enum runtime_phase {
	idle_phase,
	calibration_phase,
	accuracy_measurement_phase,
} current_phase = idle_phase;

static pthread_t threads[MAX_THREADS];

static unsigned long long counts[MAX_THREADS];

static int nrthreads;

static pthread_cond_t barrier;

static pthread_mutex_t lock;

static sem_t ready;

static int tgid = -1;

static void __attribute__(( noinline ))
do_work(unsigned long long *loops, long work_ns)
{
	struct timespec now, to, delay;

	clock_gettime(CLOCK_MONOTONIC, &now);
	timespec_adds(&to, &now, work_ns);

	while (to.tv_sec > now.tv_sec ||
	       (to.tv_sec == now.tv_sec && to.tv_nsec > now.tv_nsec)) {
		clock_gettime(CLOCK_MONOTONIC, &now);
		if (loops) {
			threadobj_spin(1000);
			(*loops)++;
			/*
			 * Yield to siblings. We do that every ten loops
			 * not to over-consume the budget with scheduling
			 * overhead.
			 *
			 * Do not use sched_yield as the sibling(s) may not be
			 * awake and we would then be delayed by the cobalt
			 * core for TICK_NSEC.
			 */
			if (nrthreads > 1 && !(*loops % 10)) {
				delay.tv_sec = 0;
				delay.tv_nsec = 5000;
				clock_nanosleep(CLOCK_MONOTONIC, 0,
						&delay, NULL);
			}
		}
	}
}

static void wait_phase(enum runtime_phase phase)
{
	sem_post(&ready);

	pthread_mutex_lock(&lock);
	for (;;) {
		if (current_phase == phase)
			break;
		pthread_cond_wait(&barrier, &lock);
	}
	pthread_mutex_unlock(&lock);
}

static void start_phase(enum runtime_phase phase)
{
	pthread_mutex_lock(&lock);
	current_phase = phase;
	pthread_cond_broadcast(&barrier);
	pthread_mutex_unlock(&lock);
}

static void *thread_body(void *arg)
{
	struct sched_param_ex param_ex;
	unsigned long long *loops = arg;
	int oldstate, oldtype;

	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &oldstate);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &oldtype);

	wait_phase(calibration_phase);

	do_work(loops, PERIOD_NS * NR_PERIODS);

	param_ex.sched_priority = 1;
	param_ex.sched_quota_group = tgid;
	pthread_setschedparam_ex(pthread_self(), SCHED_QUOTA, &param_ex);

	wait_phase(accuracy_measurement_phase);

	do_work(loops, PERIOD_NS * NR_PERIODS);

	return NULL;
}

static void new_thread(pthread_t *tid, const char *name,
		       unsigned long long *loops)
{
	struct sched_param param;
	pthread_attr_t attr;
	int ret;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
	pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
	param.sched_priority = 1;
	pthread_attr_setschedparam(&attr, &param);
	ret = pthread_create(tid, &attr, thread_body, loops);
	if (ret)
		error(1, ret, "pthread_create(SCHED_FIFO)");

	pthread_attr_destroy(&attr);
	pthread_setname_np(*tid, name);
}

static double run_quota(int quota)
{
	unsigned long long loops_at_calibration, loops_at_accuracy;
	size_t len = sched_quota_confsz();
	union sched_config cf;
	int ret, n;
	double percent;
	char label[12];

	cf.quota.op = sched_quota_add;
	cf.quota.add.pshared = 0;
	ret = sched_setconfig_np(0, SCHED_QUOTA, &cf, len);
	if (ret)
		error(1, ret, "sched_setconfig_np(add-quota-group)");

	tgid = cf.quota.info.tgid;
	cf.quota.op = sched_quota_set;
	cf.quota.set.quota = quota;
	cf.quota.set.quota_peak = quota;
	cf.quota.set.tgid = tgid;
	ret = sched_setconfig_np(0, SCHED_QUOTA, &cf, len);
	if (ret)
		error(1, ret, "sched_setconfig_np(set-quota, tgid=%d)", tgid);

	smokey_trace("new thread group #%d on CPU0, quota sum is %d%%",
		     tgid, cf.quota.info.quota_sum);
	smokey_trace("calibration x %d thread%s",
		     nrthreads, nrthreads > 1 ? "s" : "");

	for (n = 0; n < nrthreads; n++) {
		snprintf(label, sizeof(label), "t%d", n);
		new_thread(&threads[n], label, &counts[n]);
		sem_wait(&ready);
	}

	start_phase(calibration_phase);

	/*
	 * Wait for the calibration to end, then collect the expected
	 * number of loops we should be able to run during the
	 * allotted quota, even in case of runtime disruption.
	 */
	for (n = 0; n < nrthreads; n++)
		sem_wait(&ready);
	for (n = 0, loops_at_calibration = 0; n < nrthreads; n++) {
		smokey_trace("calibration: quota_thread[%d]: %llu loops",
			     n, counts[n]);
		loops_at_calibration += counts[n];
		counts[n] = 0;
	}

	smokey_trace("starting accuracy measurement");

	start_phase(accuracy_measurement_phase);

	/*
	 * Wait for the accuracy measurement to end, then collect the
	 * numbers.
	 */
	for (n = 0; n < nrthreads; n++)
		pthread_join(threads[n], NULL);
	for (n = 0, loops_at_accuracy = 0; n < nrthreads; n++) {
		smokey_trace("accuracy: quota_thread[%d]: %llu loops",
			     n, counts[n]);
		loops_at_accuracy += counts[n];
	}

	percent = 100.0 * (double)loops_at_accuracy / (double)loops_at_calibration;

	cf.quota.op = sched_quota_remove;
	cf.quota.remove.tgid = tgid;
	ret = sched_setconfig_np(0, SCHED_QUOTA, &cf, len);
	if (ret)
		error(1, ret, "sched_setconfig_np(remove-quota-group)");

	return percent;
}

static int run_sched_quota(struct smokey_test *t, int argc, char *const argv[])
{
	pthread_t me = pthread_self();
	int ret, quota = 0, policies;
	struct sched_param param;
	cpu_set_t affinity;
	double effective;

	ret = cobalt_corectl(_CC_COBALT_GET_POLICIES, &policies, sizeof(policies));
	if (ret || (policies & _CC_COBALT_SCHED_QUOTA) == 0)
		return -ENOSYS;
	
	CPU_ZERO(&affinity);
	CPU_SET(0, &affinity);
	ret = sched_setaffinity(0, sizeof(affinity), &affinity);
	if (ret)
		error(1, errno, "sched_setaffinity");

	smokey_parse_args(t, argc, argv);
	pthread_mutex_init(&lock, NULL);
	pthread_cond_init(&barrier, NULL);
	sem_init(&ready, 0, 0);

	param.sched_priority = 50;
	ret = pthread_setschedparam(me, SCHED_FIFO, &param);
	if (ret) {
		warning("pthread_setschedparam(SCHED_FIFO, 50) failed");
		return -ret;
	}

	if (SMOKEY_ARG_ISSET(sched_quota, quota))
		quota = SMOKEY_ARG_INT(sched_quota, quota);

	if (quota <= 0)
		quota = 10;

	if (SMOKEY_ARG_ISSET(sched_quota, threads))
		nrthreads = SMOKEY_ARG_INT(sched_quota, threads);

	if (nrthreads <= 0)
		nrthreads = 3;
	if (nrthreads > MAX_THREADS)
		error(1, EINVAL, "max %d threads", MAX_THREADS);

	effective = run_quota(quota);
	smokey_trace("%d thread%s: cap=%d%%, effective=%.1f%%",
		     nrthreads, nrthreads > 1 ? "s": "", quota, effective);

	if (!smokey_on_vm && fabs(effective - (double)quota) > 1.5) {
		smokey_warning("out of quota: %.1f%%",
			       effective - (double)quota);
		return -EPROTO;
	}

	return 0;
}

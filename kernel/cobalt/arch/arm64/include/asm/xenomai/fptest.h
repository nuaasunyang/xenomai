/*
 * SPDX-License-Identifier: GPL-2.0
 *
 * Copyright (C) 2006 Gilles Chanteperdrix <gilles.chanteperdrix@xenomai.org>
 */

#ifndef _COBALT_ARM64_FPTEST_H
#define _COBALT_ARM64_FPTEST_H

#include <asm/xenomai/uapi/fptest.h>
#include <asm/hwcap.h>
#include <asm/neon.h>

#define have_fp (ELF_HWCAP & HWCAP_FP)

static inline int fp_linux_begin(void)
{
	kernel_neon_begin();

	return 0;
}

static inline void fp_linux_end(void)
{
	kernel_neon_end();
}

static inline int fp_detect(void)
{
	return have_fp ? __COBALT_HAVE_FPU : 0;
}

#endif /* !_COBALT_ARM64_FPTEST_H */

/*
 * SPDX-License-Identifier: GPL-2.0
 *
 * Copyright (C) 2006 Gilles Chanteperdrix <gilles.chanteperdrix@xenomai.org>
 */

#ifndef _COBALT_ARM64_FPTEST_H
#define _COBALT_ARM64_FPTEST_H

#include <asm/hwcap.h>
#include <asm/neon.h>

#include <asm/xenomai/uapi/fptest.h>
#include <asm-generic/xenomai/wrappers.h>

#define have_fp (ELF_HWCAP & HWCAP_FP)

struct cobalt_arch_fp_state {
	struct user_fpsimd_state state;
};

static inline int fp_linux_begin(struct cobalt_arch_fp_state *state)
{
	kernel_neon_begin(&state->state);

	return 0;
}

static inline void fp_linux_end(struct cobalt_arch_fp_state *state)
{
	kernel_neon_end(&state->state);
}

static inline int fp_detect(void)
{
	return have_fp ? __COBALT_HAVE_FPU : 0;
}

#endif /* !_COBALT_ARM64_FPTEST_H */

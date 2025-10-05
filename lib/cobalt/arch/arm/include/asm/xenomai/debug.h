/*
 * Copyright (c) Siemens, 2025
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 */

#ifndef _LIB_COBALT_ARM_DEBUG_H
#define _LIB_COBALT_ARM_DEBUG_H

#include <signal.h>
#include "current.h"

static inline int cobalt_avoid_backtrace(int reason)
{
	if (reason == SIGDEBUG_MIGRATE_SIGNAL)
		return 1;

	if (cobalt_get_current_mode() & PTHREAD_WARNSW)
		return 1;

	return 0;
}

#endif /* !_LIB_COBALT_ARM_DEBUG_H */

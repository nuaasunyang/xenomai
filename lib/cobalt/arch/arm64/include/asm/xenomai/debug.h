/*
 * Copyright (c) Siemens, 2025
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 */

#ifndef _LIB_COBALT_ARM64_DEBUG_H
#define _LIB_COBALT_ARM64_DEBUG_H

static inline int cobalt_avoid_backtrace(int reason)
{
	return 0;
}

#endif /* !_LIB_COBALT_ARM64_DEBUG_H */

// SPDX-License-Identifier: LGPL-2.0-or-later

/*
 * Copyright (C) 2026 Florian Bezdeka <florian.bezdeka@siemens.com>.
 */

/*
 * Magic ahead:
 * This file is compiled twice in case y2038/time64_t support is necessary
 * and requested. The first time with time64_t and a second time with native
 * time_t.
 */

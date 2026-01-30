/*
 * Copyright (C) 2005 Jan Kiszka <jan.kiszka@web.de>.
 * Copyright (C) 2005 Heikki Lindholm <holindho@cs.helsinki.fi>.
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

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307
  USA.
 */

#include <sys/mman.h>
#include <rtdm/rtdm.h>
#include <cobalt/uapi/syscall.h>
#include <asm/xenomai/syscall.h>

COBALT_IMPL(void *, mmap64, (void *addr, size_t length, int prot, int flags,
			     int fd, off64_t offset))
{
	struct _rtdm_mmap_request rma;
	int ret;

	if (fd < 0) /* We don't do anonymous mappings. */
		goto regular;

	/* RTDM ignores the address hint, and rejects MAP_FIXED. */
	rma.length = length;
	rma.offset = offset;
	rma.prot = prot;
	rma.flags = flags;

	ret = XENOMAI_SYSCALL3(sc_cobalt_mmap, fd, &rma, &addr);
	if (ret != -EADV && ret != -ENOSYS) {
		if (ret) {
			errno = -ret;
			return MAP_FAILED;
		}

		return addr;
	}

regular:
#if mmap64 == mmap
	return __STD(mmap(addr, length, prot, flags, fd, offset));
#else
	return __STD(mmap64(addr, length, prot, flags, fd, offset));
#endif
}

COBALT_IMPL(void *, mmap, (void *addr, size_t length, int prot, int flags,
			   int fd, off_t offset))
{
	return __COBALT(mmap64(addr, length, prot, flags, fd, offset));
}

__weak
void *__real_mmap(void *addr, size_t length, int prot, int flags,
		  int fd, off_t offset)
{
	return mmap(addr, length, prot, flags, fd, offset);
}

#if mmap64 != mmap
__weak
void *__real_mmap64(void *addr, size_t length, int prot, int flags,
		  int fd, off64_t offset)
{
	return mmap64(addr, length, prot, flags, fd, offset);
}
#endif

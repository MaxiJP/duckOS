/*
	This file is part of duckOS.

	duckOS is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	duckOS is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with duckOS.  If not, see <https://www.gnu.org/licenses/>.

	Copyright (c) Byteduck 2016-2021. All rights reserved.
*/

#pragma once

#if defined(__i386__)
#include "kernel/IO.h"
#endif

#define CMOS_PORT 0x70
#define NMI_FLAG 0x80

namespace Interrupt {
	class NMIDisabler {
	public:
		inline NMIDisabler() {
#if defined(__i386__)
			IO::outb(CMOS_PORT, NMI_FLAG | IO::inb(CMOS_PORT));
#endif
			//TODO: aarch64
		}

		inline ~NMIDisabler() {
#if defined(__i386__)
			IO::outb(CMOS_PORT, IO::inb(CMOS_PORT) & (~NMI_FLAG));
#endif
			//TODO: aarch64
		}
	};
}


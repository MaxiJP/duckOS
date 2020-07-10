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

    Copyright (c) Byteduck 2016-2020. All rights reserved.
*/

//A simple program that does nothing.

#include <stdio.h>
#include <unistd.h>

int main(int argc, char **argv) {
	uint8_t* brk = sbrk(0) - 1;
	for(int i = 0; i < 1000; i++) {
		brk[0] = 0;
		sbrk(4096);
		brk = sbrk(0) - 1;
	}

	return 0;
}

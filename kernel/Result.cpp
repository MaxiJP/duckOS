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

#include "Result.hpp"
#include "kernel/kstd/kstdio.h"

const Result Result::Success {0};

Result::Result(int code): _code(code) {
}

bool Result::is_success() const {
	return _code == 0;
}

bool Result::is_error() const {
	return _code != 0;
}

int Result::code() const {
	return _code;
}

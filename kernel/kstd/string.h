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

#include <kernel/kstd/types.h>

namespace kstd {
	class string {
	public:
		string();
		string(const string& string);
		string(string&& string) noexcept;
		string(const char* string);
		~string();

		string& operator=(const char* str);
		string& operator=(const string& str);
		string& operator=(string&& str) noexcept;
		string& operator+=(const string& str);
		string& operator+=(const char* str);
		string operator+(const string& b) const;
		string operator+(const char* str) const;
		bool operator==(const string& str) const;
		bool operator==(const char* str) const;
		bool operator!=(const string& str) const;
		bool operator!=(const char* str) const;
		char& operator[](size_t index) const;

		size_t length() const;
		char* c_str() const;
		char* data() const;
		string substr(size_t start, size_t length) const;
		size_t find(const string& str, size_t start = 0) const;
		size_t find(const char* str, size_t start = 0) const;
		size_t find(const char c, size_t start = 0) const;
		size_t find_last_of(const string& str, size_t end = -1) const;
		size_t find_last_of(const char* str, size_t end = -1) const;
		size_t find_last_of(const char c, size_t end = -1) const;

	private:
		string(const char* a, size_t size_a, const char* b, size_t size_b);
		void append(const char* str, size_t length);
		void expand_to(size_t min_size);

		size_t _size;
		size_t _length;
		char* _cstring;
	};
}

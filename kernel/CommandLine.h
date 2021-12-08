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

#pragma once

#include <kernel/multiboot.h>
#include <kernel/kstd/string.h>
#include <kernel/kstd/vector.hpp>

class CommandLine {
public:
	struct Option {
		kstd::string name, value;
	};

	CommandLine(const struct multiboot_info& header);
	static CommandLine& inst();

	const kstd::string& get_option_value(char* name);
	bool has_option(char* name);
	const kstd::string& get_cmdline();

private:
	static CommandLine* _inst;

	kstd::string cmdline;
	kstd::string nullopt = "";
	kstd::vector<Option> options;
};



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

#include <kernel/kstd/Arc.h>
#include <kernel/kstd/queue.hpp>
#include <kernel/kstd/unix_types.h>
#include <kernel/tasking/Mutex.h>

class Process;
class SocketFSClient {
public:
	explicit SocketFSClient(sockid_t id, pid_t pid): id(id), pid(pid), data_queue() {}

	sockid_t id;
	pid_t pid;
	kstd::queue<uint8_t> data_queue;
	Mutex data_lock {"SocketFSClient"};
	BooleanBlocker blocker;
};


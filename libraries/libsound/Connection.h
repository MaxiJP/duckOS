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

	Copyright (c) Byteduck 2016-2022. All rights reserved.
*/

#pragma once

#include <libduck/Result.h>
#include <libriver/river.h>
#include "SampleBuffer.h"

namespace Sound {
	class Connection {
	public:
		static Duck::ResultRet<std::shared_ptr<Connection>> create();

		void queue_samples(const SampleBuffer& buffer);

	private:
		explicit Connection(std::shared_ptr<River::Endpoint> endpoint);

		std::shared_ptr<River::Endpoint> m_endpoint;
		pid_t m_server_pid;
		uint32_t m_server_samplerate;

		//RIVER FUNCTIONS
		River::Function<bool, int, size_t> server_queue_samples = {"queue_samples"};
		River::Function<uint32_t> get_server_sample_rate = {"get_sample_rate"};
		River::Function<pid_t> get_server_pid = {"get_server_pid"};
	};
}



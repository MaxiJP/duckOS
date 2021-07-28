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

#ifndef DUCKOS_WAITBLOCKER_H
#define DUCKOS_WAITBLOCKER_H

#include "Blocker.h"
#include "Process.h"

class WaitBlocker: public Blocker {
public:
	WaitBlocker(kstd::shared_ptr<Thread> thread, pid_t wait_for);
	bool is_ready() override;

	pid_t waited_pid();
	pid_t error();
	pid_t exit_status();

private:
	int _err = 0;
	int _exit_status = 0;
	pid_t _wait_pid;
	pid_t _wait_pgid;
	kstd::shared_ptr<Thread> _thread;
};

#endif //DUCKOS_WAITBLOCKER_H

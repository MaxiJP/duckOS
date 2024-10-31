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

#include "Mutex.h"
#include "TaskManager.h"

extern bool g_panicking;

Mutex::Mutex(const kstd::string& name): Lock(name) {}

Mutex::~Mutex() = default;

bool Mutex::locked() {
	return m_holding_thread.load(MemoryOrder::SeqCst) != -1;
}

void Mutex::release() {
	if(!TaskManager::enabled() || g_panicking)
		return;

	TaskManager::ScopedCritical crit;

	// Decrease counter. If the counter is zero, release the lock
	if(m_times_locked.sub(1, MemoryOrder::Release) == 1) {
		TaskManager::current_thread()->released_lock(this);
		m_holding_thread.store(-1, MemoryOrder::SeqCst);
	}
}

void Mutex::acquire() {
	acquire_with_mode<AcquireMode::Normal>();
	ASSERT(!TaskManager::in_critical() || g_panicking);
}

bool Mutex::try_acquire() {
	return acquire_with_mode<AcquireMode::Try>();
}

void Mutex::acquire_and_enter_critical() {
	acquire_with_mode<AcquireMode::EnterCritical>();
}

template<Mutex::AcquireMode mode>
inline bool Mutex::acquire_with_mode() {
	auto cur_thread = TaskManager::current_thread();
	if(!TaskManager::enabled() || !cur_thread || g_panicking)
		return true; //Tasking isn't initialized yet
	auto cur_tid = cur_thread->tid();

	//Loop while the lock is held
	while(true) {
		if constexpr(mode == AcquireMode::EnterCritical)
			TaskManager::enter_critical();

		// Try locking if no thread is holding
		tid_t expected = -1;
		if(m_holding_thread.compare_exchange_strong(expected, cur_tid, MemoryOrder::Acquire)) {
			TaskManager::current_thread()->acquired_lock(this);
			break;
		}

		// Current thread is already holding
		if(expected == cur_tid)
			break;

#ifdef DEBUG
		m_contest_count.add(1, MemoryOrder::Relaxed);
#endif

		if constexpr(mode == AcquireMode::Try) {
			return false;
		}

		if constexpr(mode == AcquireMode::EnterCritical) {
			TaskManager::leave_critical();
			ASSERT(!TaskManager::in_critical());
		}

		TaskManager::yield();
	}

	// We've got the lock!
	m_times_locked.add(1, MemoryOrder::Acquire);
	return true;
}

bool Mutex::held_by_current_thread() {
	auto cur_thread = TaskManager::current_thread();
	return !cur_thread || cur_thread->tid() == m_holding_thread.load(MemoryOrder::SeqCst);
}

ScopedCriticalLocker::ScopedCriticalLocker(Mutex& lock): m_lock(lock) {
	m_lock.acquire_and_enter_critical();
}

ScopedCriticalLocker::~ScopedCriticalLocker() {
	release();
}

void ScopedCriticalLocker::release() {
	if(m_released)
		return;
	m_released = true;
	m_lock.release();
	TaskManager::leave_critical();
}

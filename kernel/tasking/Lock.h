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
#include <kernel/kstd/string.h>
#include <kernel/Atomic.h>

#define LOCK(lock) const ScopedLocker __locker((lock))
#define LOCK_N(lock, name) const ScopedLocker name((lock))

class Lock;

class ScopedLocker {
public:
	explicit ScopedLocker(Lock& lock);
	~ScopedLocker();
private:
	Lock& _lock;
};

class Lock {
public:
	explicit Lock(const kstd::string& name);
	~Lock();

	virtual bool locked() = 0;
	virtual void acquire() = 0;
	virtual void release() = 0;
	virtual const kstd::string& name() { return m_name; }

#ifdef DEBUG
	virtual unsigned long contest_count() { return m_contest_count.load(MemoryOrder::Relaxed); }
#endif

	template<typename R, typename F>
	R synced(F&& lambda) {
		LOCK(*this);
		return lambda();
	}

protected:
	kstd::string m_name;
	Atomic<unsigned long> m_contest_count = 0;
};


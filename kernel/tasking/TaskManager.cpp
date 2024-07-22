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

#include <kernel/tasking/TaskManager.h>
#include <kernel/kmain.h>
#include <kernel/arch/Processor.h>
#include <kernel/filesystem/procfs/ProcFS.h>
#include "TSS.h"
#include "Process.h"
#include "Thread.h"
#include "Reaper.h"
#include <kernel/arch/Processor.h>
#include <kernel/kstd/KLog.h>
#include <kernel/net/NetworkManager.h>
#include "../device/DiskDevice.h"

TSS TaskManager::tss;
Mutex TaskManager::g_tasking_lock {"Tasking"};
Mutex TaskManager::g_process_lock {"Process"};

kstd::Arc<Thread> cur_thread;
Process* kernel_process;
kstd::vector<Process*>* processes = nullptr;
Thread* TaskManager::g_next_thread;

Atomic<int> next_pid = 0;
bool tasking_enabled = false;
bool yield_async = false;
bool preempting = false;

void kidle(){
	tasking_enabled = true;
	TaskManager::yield();
	TaskManager::idle_task();
}

ResultRet<kstd::Arc<Thread>> TaskManager::thread_for_tid(tid_t tid) {
	if(!tid)
		return Result(-ENOENT);
	for(int i = 0; i < processes->size(); i++) {
		auto cur = processes->at(i);
		if(cur->pid() == 0 || cur->state() != Process::DEAD) {
			for (auto& thread : cur->threads()) {
				if (thread != tid)
					continue;
				auto threadobj = cur->get_thread(thread);
				if (threadobj)
					return threadobj;
				return Result(ENOENT); // Thread must've died
			}
		}
	}
	return Result(ENOENT);
}

ResultRet<Process*> TaskManager::process_for_pid(pid_t pid){
	if(!pid)
		return Result(-ENOENT);
	for(int i = 0; i < processes->size(); i++) {
		auto cur = processes->at(i);
		if(cur->pid() == pid && cur->state() != Process::DEAD)
			return processes->at(i);
	}
	return Result(-ENOENT);
}

ResultRet<Process*> TaskManager::process_for_pgid(pid_t pgid, pid_t excl){
	if(!pgid)
		return Result(-ENOENT);
	for(int i = 0; i < processes->size(); i++) {
		auto cur = processes->at(i);
		if(cur->pgid() == pgid && cur->pid() != excl && cur->state() != Process::DEAD)
			return processes->at(i);
	}
	return Result(-ENOENT);
}

ResultRet<Process*> TaskManager::process_for_ppid(pid_t ppid, pid_t excl){
	if(!ppid)
		return Result(-ENOENT);
	for(int i = 0; i < processes->size(); i++) {
		auto cur = processes->at(i);
		if(cur->ppid() == ppid && cur->pid() != excl && cur->state() != Process::DEAD)
			return processes->at(i);
	}
	return Result(-ENOENT);
}

ResultRet<Process*> TaskManager::process_for_sid(pid_t sid, pid_t excl){
	if(!sid)
		return Result(-ENOENT);
	for(int i = 0; i < processes->size(); i++) {
		auto cur = processes->at(i);
		if(cur->sid() == sid && cur->pid() != excl && cur->state() != Process::DEAD)
			return processes->at(i);
	}
	return Result(-ENOENT);
}

void TaskManager::kill_pgid(pid_t pgid, int sig) {
	if(!pgid)
		return;
	for(int i = 0; i < processes->size(); i++) {
		auto cur = processes->at(i);
		if(cur->pgid() == pgid) {
			cur->kill(sig);
			return;
		}
	}
}

void TaskManager::reparent_orphans(Process* dead) {
	CRITICAL_LOCK(g_tasking_lock);
	for(auto process : *processes)
		if(process->ppid() == dead->pid())
			process->set_ppid(1);
}

bool TaskManager::enabled(){
	return tasking_enabled;
}

bool TaskManager::is_idle() {
	if(!kernel_process)
		return true;
	return cur_thread->tid() == kernel_process->pid();
}

bool TaskManager::is_preempting() {
	return preempting;
}

pid_t TaskManager::get_new_pid(){
	return next_pid.add(1);
}

void TaskManager::init(){
	KLog::dbg("TaskManager", "Initializing tasking...");
	processes = new kstd::vector<Process*>();

	//Setup TSS
	memset(&tss, 0, sizeof(TSS));
	tss.ss0 = 0x10;
	tss.cs = 0x0b;
	tss.ss = 0x13;
	tss.ds = 0x13;
	tss.es = 0x13;
	tss.fs = 0x13;
	tss.gs = 0x13;

	//Create kernel process
	kernel_process = Process::create_kernel("[kernel]", kidle);
	processes->push_back(kernel_process);

	//Create kinit process
	auto kinit_process = Process::create_kernel("[kinit]", kmain_late);
	processes->push_back(kinit_process);
	queue_thread(kinit_process->get_thread(kinit_process->pid()));

	//Create kernel threads
	kernel_process->spawn_kernel_thread(kreaper_entry);
	kernel_process->spawn_kernel_thread(NetworkManager::task_entry);
	kernel_process->spawn_kernel_thread(DiskDevice::cache_writeback_task_entry);

	//Preempt
	cur_thread = kernel_process->get_thread(kernel_process->pid());
	// TODO: AARCH64
#if defined (__i686__)
	preempt_init_asm(cur_thread->registers.gp.esp);
#endif
}

kstd::vector<Process*>* TaskManager::process_list() {
	return processes;
}

kstd::Arc<Thread>& TaskManager::current_thread() {
	return cur_thread;
}

Process* TaskManager::current_process() {
	return cur_thread->process();
}

int TaskManager::add_process(Process* proc){
	g_process_lock.acquire();
	ProcFS::inst().proc_add(proc);
	processes->push_back(proc);
	g_process_lock.release();

	auto& threads = proc->threads();
	for(const auto& tid : threads)
		queue_thread(proc->get_thread(tid));
	return proc->pid();
}

void TaskManager::remove_process(Process* proc) {
	LOCK(g_process_lock);
	ProcFS::inst().proc_remove(proc);
	for(size_t i = 0; i < processes->size(); i++) {
		if(processes->at(i) == proc) {
			processes->erase(i);
			return;
		}
	}
}

void TaskManager::queue_thread(const kstd::Arc<Thread>& thread) {
	if(!thread) {
		KLog::warn("TaskManager", "Tried queueing null thread!");
		return;
	}
	if(thread->tid() == kernel_process->pid()) {
		KLog::warn("TaskManager", "Tried queuing kidle thread!");
		return;
	}
	if(thread->state() != Thread::ALIVE) {
		KLog::warn("TaskManager", "Tried queuing {} thread!", thread->state_name());
		return;
	}

	ScopedCritical crit;
	if(g_next_thread)
		g_next_thread->enqueue_thread(thread.get());
	else
		g_next_thread = thread.get();
}

kstd::Arc<Thread> TaskManager::pick_next_thread() {
	ASSERT(g_tasking_lock.held_by_current_thread());

	// Make sure the next thread is in a runnable state
	while(g_next_thread && !g_next_thread->can_be_run())
		g_next_thread = g_next_thread->next_thread();

	// If we don't have a next thread to run, either continue running the current thread or run kidle
	if(!g_next_thread) {
		if(cur_thread->can_be_run()) {
			return cur_thread;
		} else if(kernel_process->get_thread(kernel_process->pid())->state() != Thread::ALIVE) {
			PANIC("KTHREAD_DEADLOCK", "The kernel idle thread is blocked!");
		} else {
			return kernel_process->get_thread(kernel_process->pid());
		}
	}

	auto next = g_next_thread->self();
	g_next_thread = g_next_thread->next_thread();
	return next;
}

bool TaskManager::yield() {
	ASSERT(!preempting);
	if(Processor::in_interrupt()) {
		// We can't yield in an interrupt. Instead, we'll yield immediately after we exit the interrupt
		yield_async = true;
		return false;
	} else {
		preempt();
		return true;
	}
}

bool TaskManager::yield_if_not_preempting() {
	if(!preempting)
		return yield();
	return true;
}

bool TaskManager::yield_if_idle() {
	if(!kernel_process)
		return false;
	if(cur_thread->tid() == kernel_process->pid())
		return yield();
	return false;
}

void TaskManager::do_yield_async() {
	if(yield_async) {
		yield_async = false;
		preempt();
	}
}

void TaskManager::tick() {
	ASSERT(Processor::in_interrupt());
	yield();
}

Atomic<int, MemoryOrder::SeqCst> g_critical_count = 0;

void TaskManager::enter_critical() {
	Processor::disable_interrupts();
	g_critical_count.add(1, MemoryOrder::Acquire);
}

void TaskManager::leave_critical() {
	ASSERT(g_critical_count.load() > 0);
	if(g_critical_count.sub(1, MemoryOrder::Release) == 1)
		Processor::enable_interrupts();
}

bool TaskManager::in_critical() {
	return g_critical_count.load();
}

void TaskManager::preempt(){
	if(!tasking_enabled)
		return;
	ASSERT(!g_critical_count.load());

	g_tasking_lock.acquire_and_enter_critical();
	preempting = true;

	// Try unblocking threads that are blocked
	if(g_process_lock.try_acquire()) {
		for(auto& process : *processes) {
			if(process->state() != Process::ALIVE && process->state() != Process::STOPPED)
				continue;
			for(auto& tid : process->threads()) {
				auto thread = process->get_thread(tid);
				if(!thread)
					continue;
				if(thread->state() == Thread::BLOCKED && thread->should_unblock())
					thread->unblock();
			}
		}
		g_process_lock.release();
	}

	// Pick a new thread
	auto old_thread = cur_thread;
	auto next_thread = pick_next_thread();

	bool should_preempt = old_thread != next_thread;

	//If we were just in a signal handler, don't save the esp to old_proc->registers
	uint32_t* old_esp;
	uint32_t dummy_esp;
	if(!old_thread) {
		old_esp = &dummy_esp;
	} if(old_thread->in_signal_handler()) {
		old_esp = &old_thread->signal_registers.gp.esp;
	} else {
		old_esp = &old_thread->registers.gp.esp;
	}

	//If we just finished handling a signal, set in_signal_handler to false.
	if(old_thread && old_thread->just_finished_signal()) {
		should_preempt = true;
		old_thread->just_finished_signal() = false;
		old_thread->in_signal_handler() = false;
	}

	//If we're about to start handling a signal, set in_signal_handler to true.
	if(next_thread->ready_to_handle_signal()) {
		should_preempt = true;
		next_thread->in_signal_handler() = true;
		next_thread->ready_to_handle_signal() = false;
	}

	//If we're switching to a process in a signal handler, use the esp from signal_registers
	uint32_t* new_esp;
	if(next_thread->in_signal_handler()) {
		new_esp = &next_thread->signal_registers.gp.esp;
		tss.esp0 = (size_t) next_thread->signal_stack_top();
	} else {
		new_esp = &next_thread->registers.gp.esp;
		tss.esp0 = (size_t) next_thread->kernel_stack_top();
	}

	if(should_preempt)
		next_thread->process()->set_last_active_thread(next_thread->tid());

	// Switch context.
	preempting = false;
	if(!next_thread->can_be_run())
		PANIC("INVALID_CONTEXT_SWITCH", "Tried to switch to thread %d of PID %d in state %d", next_thread->tid(), next_thread->process()->pid(), next_thread->state());
	if(should_preempt) {
		// If we can run the old thread, re-queue it after we preempt
		if(old_thread->tid() != kernel_process->pid() && old_thread->can_be_run())
			queue_thread(old_thread);

		cur_thread = next_thread;
		next_thread.reset();

		Processor::save_fpu_state((void*&) old_thread->fpu_state);
		old_thread.reset();
		// TODO: AARCH64
#if defined(__i386__)
		preempt_asm(old_esp, new_esp, cur_thread->page_directory()->entries_physaddr());
#endif
		Processor::load_fpu_state((void*&) cur_thread->fpu_state);
	}


	preempt_finish();
}

void TaskManager::preempt_finish() {
	ASSERT(g_tasking_lock.times_locked() == 1);
	g_tasking_lock.release();
	leave_critical();

	// Hack(?) to get signals to dispatch, thread to die if it needs, etc
	cur_thread->enter_critical();
	current_thread()->leave_critical();
}

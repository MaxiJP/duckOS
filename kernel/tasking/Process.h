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
#include "Signal.h"
#include "../memory/VMSpace.h"
#include "../kstd/map.hpp"
#include <kernel/User.h>
#include <kernel/kstd/string.h>
#include "../api/poll.h"
#include "../api/mmap.h"
#include "Tracer.h"
#include "../kstd/KLog.h"

class FileDescriptor;
class Blocker;
class ProcessArgs;
class TTYDevice;
class Thread;
class PageDirectory;
class LinkedInode;

namespace ELF {struct elf32_header;};

template<typename T>
class UserspacePointer;

class Process {
public:
	enum State {
		ALIVE = 0, //The process has not exited yet
		ZOMBIE = 1, //The process has exited and needs to be reaped
		DEAD = 2, //The process has been reaped and needs to be removed from the process table
		STOPPED = 4, //The process has been stopped by a signal or debugger
	};

	~Process();

	//Construction
	static Process* create_kernel(const kstd::string& name, void (*func)());
	static ResultRet<Process*> create_user(const kstd::string& executable_loc, User& file_open_user, ProcessArgs* args, pid_t pid, pid_t parent);

	//Process Info
	pid_t pid();
	pid_t pgid();
	pid_t ppid();
	void set_ppid(pid_t ppid);
	pid_t sid();
	User user();
	const kstd::string& name();
	const kstd::string& exe();
	kstd::Arc<LinkedInode> cwd();
	void set_tty(kstd::Arc<TTYDevice> tty);
	State state();
	int all_threads_state();
	int exit_status();
	bool is_kernel_mode();

	//Threads
	tid_t last_active_thread();
	void set_last_active_thread(tid_t tid);
	kstd::Arc<Thread> spawn_kernel_thread(void (*entry)());
	const kstd::vector<tid_t>& threads();
	kstd::Arc<Thread> get_thread(tid_t tid);

	//Signals and death
	void kill(int signal);
	void die();

	//Memory
	PageDirectory* page_directory();
	kstd::Arc<VMSpace> vm_space();
	ResultRet<kstd::Arc<VMRegion>> map_object(kstd::Arc<VMObject> object, VMProt prot);
	ResultRet<kstd::Arc<VMRegion>> map_object(kstd::Arc<VMObject> object, VirtualAddress address, VMProt prot);
	size_t used_pmem() const;
	size_t used_vmem() const;
	size_t used_shmem() const;

	//Tracing
	bool is_traced_by(Process* proc);
	bool is_traced();

	//Syscalls
	void check_ptr(const void* ptr, bool write = false);
	void sys_exit(int status);
	ssize_t sys_read(int fd, UserspacePointer<uint8_t> buf, size_t count);
	ssize_t sys_write(int fd, UserspacePointer<uint8_t> buf, size_t count);
	pid_t sys_fork(ThreadRegisters& regs);
	int exec(const kstd::string& filename, ProcessArgs* args);
	int sys_execve(UserspacePointer<char> filename, UserspacePointer<char*> argv, UserspacePointer<char*> envp);
	int sys_open(UserspacePointer<char> filename, int options, int mode);
	int sys_close(int file);
	int sys_chdir(UserspacePointer<char> path);
	int sys_getcwd(UserspacePointer<char> buf, size_t length);
	int sys_readdir(int file, UserspacePointer<char> buf, size_t len);
	int sys_fstat(int file, UserspacePointer<struct stat> buf);
	int sys_stat(UserspacePointer<char> file, UserspacePointer<struct stat> buf);
	int sys_lstat(UserspacePointer<char> file, UserspacePointer<struct stat> buf);
	int sys_lseek(int file, off_t off, int whence);
	int sys_waitpid(pid_t pid, UserspacePointer<int> status, int flags);
	int sys_gettimeofday(UserspacePointer<timeval> t, UserspacePointer<void*> z);
	int sys_sigaction(int sig, UserspacePointer<sigaction> new_action, UserspacePointer<sigaction> old_action);
	int sys_kill(pid_t pid, int sig);
	int sys_unlink(UserspacePointer<char> name);
	int sys_link(UserspacePointer<char> oldpath, UserspacePointer<char> newpath);
	int sys_rmdir(UserspacePointer<char> name);
	int sys_mkdir(UserspacePointer<char> path, mode_t mode);
	int sys_mkdirat(int fd, UserspacePointer<char> path, mode_t mode);
	int sys_truncate(UserspacePointer<char> path, off_t length);
	int sys_ftruncate(int fd, off_t length);
	int sys_pipe(UserspacePointer<int>, int options);
	int sys_dup(int oldfd);
	int sys_dup2(int oldfd, int newfd);
	int sys_isatty(int fd);
	int sys_symlink(UserspacePointer<char> file, UserspacePointer<char> linkname);
	int sys_symlinkat(UserspacePointer<char> file, int dirfd, UserspacePointer<char> linkname);
	int sys_readlink(UserspacePointer<char> file, UserspacePointer<char> buf, size_t bufsize);
	int sys_readlinkat(UserspacePointer<struct readlinkat_args> args);
	int sys_getsid(pid_t pid);
	int sys_setsid();
	int sys_getpgid(pid_t pid);
	int sys_getpgrp();
	int sys_setpgid(pid_t pid, pid_t new_pgid);
	int sys_setuid(uid_t uid);
	int sys_seteuid(uid_t euid);
	uid_t sys_getuid();
	uid_t sys_geteuid();
	int sys_setgid(gid_t gid);
	int sys_setegid(gid_t egid);
	gid_t sys_getgid();
	gid_t sys_getegid();
	int sys_setgroups(size_t count, UserspacePointer<gid_t> gids);
	int sys_getgroups(int count, UserspacePointer<gid_t> gids);
	mode_t sys_umask(mode_t new_mask);
	int sys_chmod(UserspacePointer<char> file, mode_t mode);
	int sys_fchmod(int fd, mode_t mode);
	int sys_chown(UserspacePointer<char> file, uid_t uid, gid_t gid);
	int sys_fchown(int fd, uid_t uid, gid_t gid);
	int sys_lchown(UserspacePointer<char> file, uid_t uid, gid_t gid);
	int sys_ioctl(int fd, unsigned request, UserspacePointer<void*> argp);
	int sys_shmcreate(UserspacePointer<shmcreate_args> args_p);
	int sys_shmattach(int id, void* addr, UserspacePointer<struct shm> s);
	int sys_shmdetach(int id);
	int sys_shmallow(int id, pid_t pid, int perms);
	int sys_poll(UserspacePointer<pollfd> pollfd, nfds_t nfd, int timeout);
	int sys_ptsname(int fd, UserspacePointer<char> buf, size_t bufsize);
	int sys_sleep(UserspacePointer<timespec> time, UserspacePointer<timespec> remainder);
	int sys_threadcreate(void* (*entry_func)(void* (*)(void*), void*), void* (*thread_func)(void*), void* arg);
	int sys_gettid();
	int sys_threadjoin(tid_t tid, UserspacePointer<void*> retp);
	int sys_threadexit(void* return_value);
	int sys_access(UserspacePointer<char> pathname, int mode);
	Result sys_mmap(UserspacePointer<struct mmap_args> args);
	int sys_munmap(void* addr, size_t length);
	int sys_mprotect(void* addr, size_t length, int prot);
	int sys_uname(UserspacePointer<struct utsname> buf);
	int sys_ptrace(UserspacePointer<struct ptrace_args> args);
	int sys_socket(int domain, int type, int protocol);
	int sys_bind(int sockfd, UserspacePointer<struct sockaddr> addr, uint32_t addrlen);
	int sys_connect(int sockfd, UserspacePointer<struct sockaddr> addr, uint32_t addrlen);
	int sys_setsockopt(UserspacePointer<struct setsockopt_args> ptr);
	int sys_getsockopt(UserspacePointer<struct getsockopt_args> ptr);
	int sys_recvmsg(int sockfd, UserspacePointer<struct msghdr> msg, int flags);
	int sys_sendmsg(int sockfd, UserspacePointer<struct msghdr> msg, int flags);
	int sys_getifaddrs(UserspacePointer<struct ifaddrs> buf, size_t memsz);
	int sys_listen(int sockfd, int backlog);
	int sys_shutdown(int sockfd, int how);
	int sys_accept(int sockfd, UserspacePointer<struct sockaddr> addr, UserspacePointer<uint32_t> addrlen);
	int sys_futex(UserspacePointer<int> futex, int operation);

private:
	friend class Thread;
	friend class Reaper;
	Process(const kstd::string& name, size_t entry_point, bool kernel, ProcessArgs* args, pid_t pid, pid_t ppid);
	Process(Process* to_fork, ThreadRegisters& regs);

	void alert_thread_died(kstd::Arc<Thread> thread);
	void insert_thread(const kstd::Arc<Thread>& thread);
	void remove_thread(const kstd::Arc<Thread>& thread);

	template<typename F>
	void for_each_thread(F&& callback) {
		LOCK(_thread_lock);
		for (auto tid : _tids) {
			auto thread = get_thread(tid);
			if(!thread)
				continue;
			if(!callback(thread))
				break;
		}
	}

	void reap();
	bool stop(int signal);
	bool is_stopping();
	void cont();
	void notify_thread_stopping(Thread* thread);

	//Identifying info and state
	kstd::string _name = "";
	kstd::string _exe = "";
	pid_t _pid = 0;
	pid_t _ppid = 0;
	pid_t _sid = 0;
	pid_t _pgid = 0;
	kstd::Arc<TTYDevice> _tty;
	User _user;
	mode_t _umask = 022;
	int _exit_status = 0;
	State _state;
	bool _died_gracefully = false;
	bool _kernel_mode = false;
	Atomic<bool> _ready_to_destroy = false;
	Atomic<bool> _stopping = false;
	Mutex m_starting_lock {"Process::Starting"};

	//Memory
	kstd::Arc<VMSpace> _vm_space;
	kstd::Arc<PageDirectory> _page_directory;
	kstd::vector<kstd::Arc<VMRegion>> _vm_regions;
	Mutex m_mem_lock {"Process::Memory"};
	size_t m_used_shmem = 0;

	//Files & Pipes
	Mutex m_fd_lock { "Process::FileDescriptor" };
	kstd::vector<kstd::Arc<FileDescriptor>> _file_descriptors;
	kstd::Arc<LinkedInode> _cwd;

	//Signals
	Signal::SigAction signal_actions[32] = {{Signal::SigAction()}};

	//Threads
	kstd::map<tid_t, kstd::Arc<Thread>> _threads;
	kstd::map<tid_t, void*> _thread_return_values;
	kstd::vector<tid_t> _tids;
	tid_t _last_active_thread = 1;
	Mutex _thread_lock {"Process::Thread"};

	//Tracing
	Mutex _tracing_lock {"Process::Tracing"};
	kstd::vector<kstd::Arc<Tracer>> _tracers;

	Process* _self_ptr;
};

void print_arg(Process* thread, KLog::FormatRules rules);


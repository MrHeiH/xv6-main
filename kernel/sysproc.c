
#include "include/types.h"
#include "include/riscv.h"
#include "include/param.h"
#include "include/memlayout.h"
#include "include/spinlock.h"
#include "include/proc.h"
#include "include/syscall.h"
#include "include/timer.h"
#include "include/kalloc.h"
#include "include/string.h"
#include "include/printf.h"

extern int exec(char *path, char **argv);

uint64
sys_exec(void) {
	char path[FAT32_MAX_PATH], *argv[MAXARG];
	int i;
	uint64 uargv, uarg;

	if(argstr(0, path, FAT32_MAX_PATH) < 0 || argaddr(1, &uargv) < 0) {
		return -1;
	}
	memset(argv, 0, sizeof(argv));
	for(i = 0;; i++) {
		if(i >= NELEM(argv)) {
			goto bad;
		}
		if(fetchaddr(uargv + sizeof(uint64) * i, (uint64 *)&uarg) < 0) {
			goto bad;
		}
		if(uarg == 0) {
			argv[i] = 0;
			break;
		}
		argv[i] = kalloc();
		if(argv[i] == 0)
			goto bad;
		if(fetchstr(uarg, argv[i], PGSIZE) < 0)
			goto bad;
	}

	int ret = exec(path, argv);

	for(i = 0; i < NELEM(argv) && argv[i] != 0; i++)
		kfree(argv[i]);

	return ret;

bad:
	for(i = 0; i < NELEM(argv) && argv[i] != 0; i++)
		kfree(argv[i]);
	return -1;
}

uint64
sys_exit(void) {
	int n;
	if(argint(0, &n) < 0)
		return -1;
	exit(n);
	return 0;    // not reached
}

uint64
sys_getpid(void) {
	return myproc()->pid;
}

uint64
sys_getppid(void) {
	struct proc *p = myproc();
	if(p->parent) {
		return p->parent->pid;
	}
	return -1;
}

uint64
sys_fork(void) {
	return fork();
}

uint64
sys_wait(void) {
	uint64 p;
	if(argaddr(0, &p) < 0)
		return -1;
	return wait(-1, p, 0);
}

uint64
sys_sbrk(void) {
	int addr;
	int n;

	if(argint(0, &n) < 0)
		return -1;
	addr = myproc()->sz;
	if(growproc(n) < 0)
		return -1;
	return addr;
}

uint64
sys_sleep(void) {
	int n;
	uint ticks0;

	if(argint(0, &n) < 0)
		return -1;
	acquire(&tickslock);
	ticks0 = ticks;
	while(ticks - ticks0 < n) {
		if(myproc()->killed) {
			release(&tickslock);
			return -1;
		}
		sleep(&ticks, &tickslock);
	}
	release(&tickslock);
	return 0;
}

uint64
sys_kill(void) {
	int pid;

	if(argint(0, &pid) < 0)
		return -1;
	return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void) {
	uint xticks;

	acquire(&tickslock);
	xticks = ticks;
	release(&tickslock);
	return xticks;
}

uint64
sys_trace(void) {
	int mask;
	if(argint(0, &mask) < 0) {
		return -1;
	}
	myproc()->tmask = mask;
	return 0;
}

uint64
sys_clone(void) {
	uint64 flag;
	uint64 stack_p;
	if(argaddr(0, &flag) < 0) {
		return -1;
	}
	if(argaddr(1, &stack_p) < 0) {
		return -1;
	}

	return clone(flag, (void *)stack_p);
}

uint64
sys_wait4(void) {
	int pid;
	uint64 status_p;
	int options;
	if(argint(0, &pid) < 0)
		return -1;
	if(argaddr(1, &status_p) < 0)
		return -1;
	if(argint(2, &options) < 0)
		return -1;

	return wait(pid, status_p, options);
}

uint64
sys_sched_yield(void) {
	yield();
	return 0;
}
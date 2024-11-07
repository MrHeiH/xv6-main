// Timer Interrupt handler
#include "include/types.h"
#include "include/param.h"
#include "include/riscv.h"
#include "include/sbi.h"
#include "include/spinlock.h"
#include "include/timer.h"
#include "include/printf.h"
#include "include/proc.h"
#include "include/vm.h"
#include "include/syscall.h"

struct spinlock tickslock;
uint ticks;

void timerinit() {
	initlock(&tickslock, "time");
#ifdef DEBUG
	printf("timerinit\n");
#endif
}

void set_next_timeout() {
	// There is a very strange bug,
	// if comment the `printf` line below
	// the timer will not work.

	// this bug seems to disappear automatically
	// printf("");
	sbi_set_timer(r_time() + INTERVAL);
}

void timer_tick() {
	acquire(&tickslock);
	ticks++;
	wakeup(&ticks);
	release(&tickslock);
	set_next_timeout();
}

uint64
sys_gettimeofday(void) {
	uint64 time_p;
	int num;
	uint xticks;
	struct timespec tp;
	if(argaddr(0, &time_p) < 0) {
		return -1;
	}
	if(argint(1, &num) < 0) {
		return -1;
	}

	acquire(&tickslock);
	xticks = ticks;
	release(&tickslock);

	tp.sec  = xticks / 200;
	tp.usec = (xticks * 1000000 / 200) % 1000000;
	if(copyout2(time_p, (char *)&tp, sizeof(tp)) < 0)
		return -1;

	return 0;
}

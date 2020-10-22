#define _XOPEN_SOURCE

#include <ucontext.h>
#include <stdio.h>
#include <stdlib.h>

ucontext_t schedulerContext, mainContext;

static void func1()
{
	printf("In func1.\n");
}

int main(int argc, char **argv) {

	getcontext(&schedulerContext);
	schedulerContext.uc_stack.ss_sp = malloc(8192);
	schedulerContext.uc_stack.ss_size = 8192;
	schedulerContext.uc_stack.ss_flags = 0;
	schedulerContext.uc_link = NULL;
	//schedulerContext.uc_link = mcp;

	// printf("Before getcontext\n");
	// getcontext(&mainContext);
	printf("Before makecontext\n");
	makecontext(&schedulerContext, func1, 0);

	// if (i == 0)
	// {
	// 	printf("Before setcontext\n");
	// 	setcontext(&schedulerContext);
	// }
	printf("Before swapcontext\n");
	swapcontext(&mainContext, &schedulerContext);

	printf("Success.\n");
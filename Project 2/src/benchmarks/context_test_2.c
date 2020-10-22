#define _XOPEN_SOURCE

#include <ucontext.h>
#include <stdio.h>
#include <stdlib.h>

//ucontext_t schedulerContext, mainContext;
ucontext_t mainContext;

static void func1()
{
	printf("In func1.\n");
}

int main(int argc, char **argv) {

    ucontext_t * scp = (ucontext_t *) malloc(sizeof(ucontext_t));
	getcontext(scp);
	scp -> uc_stack.ss_sp = malloc(8192);
	scp -> uc_stack.ss_size = 8192;
	scp -> uc_stack.ss_flags = 0;
	scp -> uc_link = &mainContext;
	//schedulerContext.uc_link = mcp;

	// printf("Before getcontext\n");
	getcontext(&mainContext);
	printf("Before makecontext\n");
	makecontext(&(*scp), func1, 0);

	// if (i == 0)
	// {
	// 	printf("Before setcontext\n");
	// 	setcontext(&schedulerContext);
	// }
	printf("Before swapcontext\n");
	swapcontext(&mainContext, &(*scp));

	printf("Success.\n");
}
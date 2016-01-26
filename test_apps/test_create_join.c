#include <stdio.h>
#include <stdlib.h>
#include "uthread.h"

void *thread0(void *arg)
{
	printf("thread 0 - arg = %ld\n", (long)arg);
	return (void *)0;
}

void *thread1(void *arg)
{
	printf("thread 1 - arg = %ld\n", (long)arg);
	return (void *)11;
}

void *thread2(void *arg)
{
	printf("thread 2 - arg = %ld\n", (long)arg);
	return (void *)22;
}

int main()
{
	int i;
	void *uthread_ret[3];
	uthread_t tid[3];
	
	uthread_create(&tid[0], thread0, (void *)0x0, 3);
	uthread_create(&tid[1], thread1, (void *)0x1, 1);
	uthread_create(&tid[2], thread2, (void *)0x2, 2);
	
	for(i=0; i<3; i++)
	{
		uthread_join(tid[i], &uthread_ret[i]);
		printf("thread %d returned %ld\n", i, (long)uthread_ret[i]);
	}
	return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include "uthread.h"

typedef struct per_thread_t
{
	long id;
	long period;//ms
	int priority;
	struct timespec period_start;
} per_thread_t;

uthread_mutex_t umutex[3];

//strictly speaking there is a data race on the variable g
//as the threads access g with different locks
//However, we prevent the data race by careful timing manipulation
int g = 1;

void *periodic_thread_L(void *arg)
{
	int i;
	long j;
	long s = 0;
	struct timespec next;
	long long tmp;
	per_thread_t *t_arg = (per_thread_t *)arg;
	printf("thread id: %ld, period: %ldms\n", t_arg->id, t_arg->period);
	
	next = t_arg->period_start;
	for(i=0; i<2; i++)
	{
		if (i>0) //skip the first period just for testing
		{
			uthread_mutex_lock(&umutex[0]);
			for(j=0; j<30000000; j++)
			{
				s = s + j;
			}
			g = g + 10;
			uthread_mutex_unlock(&umutex[0]);
		}
		
		//get the next period start
		tmp = (next.tv_sec*1000000000 + next.tv_nsec + t_arg->period*1000000);
		next.tv_sec = tmp/1000000000;
		next.tv_nsec = tmp%1000000000;

		uthread_abstime_nanosleep(&next);
	}

	return (void *)0;
}

void *periodic_thread_M(void *arg)
{
	int i;
	long j;
	long s = 0;
	struct timespec next;
	long long tmp;
	per_thread_t *t_arg = (per_thread_t *)arg;
	printf("thread id: %ld, period: %ldms\n", t_arg->id, t_arg->period);
	
	next = t_arg->period_start;
	for(i=0; i<2; i++)
	{
		if (i>0) //skip the first period just for testing
		{
			uthread_mutex_lock(&umutex[1]);
			for(j=0; j<10000000; j++)
			{
				s = s + j;
			}
			g = g*10;
			uthread_mutex_unlock(&umutex[1]);
		}

		//get the next period start
		tmp = (next.tv_sec*1000000000 + next.tv_nsec + t_arg->period*1000000);
		next.tv_sec = tmp/1000000000;
		next.tv_nsec = tmp%1000000000;
		
		uthread_abstime_nanosleep(&next);
	}

	return (void *)0;
}

void *periodic_thread_H(void *arg)
{
	int i;
	long j;
	long s = 0;
	struct timespec next;
	long long tmp;
	per_thread_t *t_arg = (per_thread_t *)arg;
	printf("thread id: %ld, period: %ldms\n", t_arg->id, t_arg->period);
	
	next = t_arg->period_start;
	for(i=0; i<2; i++)
	{
		if (i>0) //skip the first period just for testing
		{
			uthread_mutex_lock(&umutex[0]);
			for(j=0; j<10000000; j++)
			{
				s = s + j;
			}
			uthread_mutex_unlock(&umutex[0]);
		}

		//get the next period start
		tmp = (next.tv_sec*1000000000 + next.tv_nsec + t_arg->period*1000000);
		next.tv_sec = tmp/1000000000;
		next.tv_nsec = tmp%1000000000;

		uthread_abstime_nanosleep(&next);
	}

	return (void *)0;
}

int main()
{
	int i;
	void *uthread_ret[3];
	uthread_t tid[3];
	per_thread_t thread_arg[3];
	struct timespec period_start;
	int lock_attr;
	resource_entry_t *resource_table;

	lock_attr = UTHREAD_MUTEX_ATTR_NONE;//UTHREAD_MUTEX_ATTR_NONE;
	for(i=0; i<2; i++)
		uthread_mutex_init(&umutex[i], lock_attr);

	uthread_gettime(&period_start);
	
	thread_arg[0].id = 0;
	thread_arg[0].period = 800;
	thread_arg[0].priority = 1; //high priority
	thread_arg[0].period_start = period_start;
	
	thread_arg[1].id = 1;
	thread_arg[1].period = 500;
	thread_arg[1].priority = 2; //middle priority
	thread_arg[1].period_start = period_start;
	
	thread_arg[2].id = 2;
	thread_arg[2].period = 490;
	thread_arg[2].priority = 3; //low priority
	thread_arg[2].period_start = period_start;

	uthread_create(&tid[0], periodic_thread_H, (void *)&thread_arg[0], thread_arg[0].priority);
	uthread_create(&tid[1], periodic_thread_M, (void *)&thread_arg[1], thread_arg[1].priority);
	uthread_create(&tid[2], periodic_thread_L, (void *)&thread_arg[2], thread_arg[2].priority);

	//tell the library about resouces used to enable SRP, two resources in this program
	resource_table = (resource_entry_t *)malloc(sizeof(resource_entry_t)*2);
	//for umutex[0], it is used by tid[0] and tid[2]
	resource_table[0].resource = umutex[0];
	resource_table[0].thread_array = (uthread_t *)malloc(sizeof(uthread_t)*2);
	resource_table[0].thread_array[0] = tid[0];
	resource_table[0].thread_array[1] = tid[2];
	resource_table[0].n = 2;
	//for umutex[1], it is used by tid[1]
	resource_table[1].resource = umutex[1];
	resource_table[1].thread_array = (uthread_t *)malloc(sizeof(uthread_t)*1);
	resource_table[1].thread_array[0] = tid[1];
	resource_table[1].n = 1;

	uthread_srp_enable(resource_table, 2);

	for(i=0; i<3; i++)
	{
		uthread_join(tid[i], &uthread_ret[i]);
		printf("thread %d returned %ld\n", i, (long)uthread_ret[i]);
	}

	//g=20,  any others
	//g=110, stack-based priority ceiling protocol
	printf("g = %d\n", g);
	return 0;
}

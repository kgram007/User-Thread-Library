#ifndef _UTHREADLIB_H_
#define _UTHREADLIB_H_


#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <ucontext.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>

#include "uthread.h"


#define UTHREAD_STACK_SIZE	8192

#define PRIORITY_HIGHEST	0
#define PRIORITY_LOWEST		100

#define MUTEXT_LOCKED		1
#define MUTEXT_UNLOCKED		0

enum state
{
	STATE_READY = 1,
	STATE_RUNNING,
	STATE_BLOCKED,
	STATE_EXIT,
	STATE_JOIN_WAIT
};


typedef struct TCB
{
	int id;
	int state;
	ucontext_t *context;
	int priority;
	void *retVal;

#ifdef PAR
	ucontext_t *parent;
#endif
	timer_t timerID;
	struct TCB *joinWait;
	struct TCB *next;
	struct TCB *prev;
}sTCB;


typedef struct MutexBlock
{
	uthread_mutex_t *mutex;
	int attr;
	int state;
	int owener_OrgPrio;
	int count;
	int mutexCeiling;
	struct TCB *ownerTCB;
	struct MutexBlock *next;
	struct MutexWaitList *waitList;
	struct MutexWaitList *waitList_tail;
}sMutexBlock;


typedef struct MutexWaitList
{
	struct TCB *tcb;
	struct MutexWaitList *next;
	int valid;
}sMutexWaitList;


typedef struct sUThread_List
{
	struct MutexBlock *head;
	struct MutexBlock *tail;
	int numMutex;

}sUThread_List;


extern sTCB *UThread_MainTCB;
extern int SystemCeiling;
extern int SRP_Enabled;



ucontext_t* Prepare_Context(void (*func)(void), ucontext_t *linkContext, long stackSize, int argc, void *start, void *args);


#endif

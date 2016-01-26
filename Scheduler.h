
#ifndef _SCHEDULER_H_
#define _SCHEDULER_H_

#include "Uthread_Lib.h"

typedef struct Sched_Queue
{
	sTCB *head;
	sTCB *tail;
	int numThreads;

}sSched_Queue;



extern int Sched_CurrThreadID;
extern sSched_Queue Sched_PriorityQueue;
extern sTCB *Sched_IdleTCB;

extern sTCB *CurrentThread;


sTCB* Find_PriorityInsert(int priority);
int Add_ThreadToQueue(sTCB *tcb);
int Delete_ThreadFromQueue(sTCB *tcb);
int Reorder_Queue(sTCB *tcb);

sTCB* Find_ThreadByID(int id);

void Schedule_Threads();
void Init_TimerSignal();
void Set_Timer(sTCB *tcb, const struct timespec *request);

void Unblock_TimerSignal(int sig);
void Unblock_TimerSignal(int sig);

void *Idle_Thread();
int init_Idle_Thread();


sTCB* Get_RunningThread();
sTCB* Get_PriorityReadyThread();

void Init_TimerSignal();


#endif

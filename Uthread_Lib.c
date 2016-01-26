/////////////////////////****  Assignment #3  ****//////ucontext_t *oucp//////////////////
//
//	File Name:	 main.c
//	Author:		 Ramsundar K G
//	Date:		 12 Apr 2015
//
//	Description: Containts the main function
//
//////////////////////////////////////////////////////////////////////////


/*************************  Includes  *******************************/
#include "Uthread_Lib.h"
#include "Scheduler.h"
/********************************************************************/

/*************************  Variables  ******************************/
int UThread_init_Done = 0;
ucontext_t UThread_MainContext;
sTCB *UThread_MainTCB;
sUThread_List UThread_MutexList;
int SystemCeiling = PRIORITY_LOWEST+1;
int SRP_Enabled = 0;
/********************************************************************/


/********************************************************************/
// Init the Main TCB
/********************************************************************/
int init_MainTCB()
{
	UThread_MainTCB = (sTCB *) malloc(sizeof(sTCB));
	UThread_MainTCB->context = (ucontext_t *) malloc(sizeof(ucontext_t));

	UThread_MainTCB->state = STATE_RUNNING;
	UThread_MainTCB->id = -1;
	UThread_MainTCB->priority = PRIORITY_HIGHEST;

	CurrentThread = UThread_MainTCB;

	Add_ThreadToQueue(UThread_MainTCB);

	return 0;
}


/********************************************************************/
// Init Uthread Lib
/********************************************************************/
void init_Uthread()
{
	init_MainTCB();
	init_Idle_Thread();
	Init_TimerSignal();
}


/********************************************************************/
// Prepare the context for the context switching
/********************************************************************/
ucontext_t* Prepare_Context(void (*func)(void), ucontext_t *linkContext, long stackSize, int argc, void *start, void *args)
{
	ucontext_t *context = (ucontext_t *) malloc(sizeof(ucontext_t));
	getcontext(context);

	context->uc_stack.ss_sp = (char *) malloc( stackSize );
	context->uc_stack.ss_size = stackSize;
	context->uc_link = linkContext;

	if(argc == 0)
	{
		makecontext(context, func, 0);
	}
	else
	{
		makecontext(context, func, 2, start, args);
	}

	return context;
}


/********************************************************************/
// Exit Thread Wrapper
/********************************************************************/
void Exit_Thread()
{	
	uthread_exit(NULL);
}



/********************************************************************/
// Start Thread Wrapper
/********************************************************************/
void Start_Thread(void *(*start)(void *), void *arg)
{
	void *retVal = start((void *)arg);
	sTCB *tcb = Get_RunningThread();
	tcb->retVal = retVal;
}



/********************************************************************/
// Uthread Create
/********************************************************************/
int uthread_create(uthread_t *tid, void *(*start)(void *), void *args, int priority)
{
	if(UThread_init_Done == 0)
	{
		UThread_init_Done = 1;
		init_Uthread();
	}

	ucontext_t *linkContext = Prepare_Context(Exit_Thread, NULL, UTHREAD_STACK_SIZE, 0, NULL, NULL);
	if(linkContext == NULL)
	{
		printf("\nError Allocating Memory for Thread Context !!");
		return -1;
	}

	ucontext_t *threadContext = Prepare_Context((void (*) (void))Start_Thread, linkContext, UTHREAD_STACK_SIZE, 2, start, args);
	if(threadContext == NULL)
	{
		printf("\nError Allocating Memory for Thread Context !!");
		return -1;
	}


	sTCB *tcb = (sTCB *) malloc(sizeof(sTCB));
	if(tcb == NULL)
	{
		printf("\nError Allocating Memory for TCB !!");
		return -1;
	}

	tcb->context = threadContext;
	tcb->state = STATE_READY;
	tcb->id = Sched_CurrThreadID++;
	tcb->priority = priority;

	*tid = tcb->id;

	Add_ThreadToQueue(tcb);

	Schedule_Threads();

	return 0;
}



/********************************************************************/
// Uthread Join
/********************************************************************/
int uthread_join(uthread_t tid, void **value_ptr)
{
	sTCB *tcb = Find_ThreadByID(tid);
	if(tcb != NULL)
	{
		CurrentThread->joinWait = tcb;
		if(tcb->state != STATE_EXIT)
		{
			CurrentThread->state = STATE_JOIN_WAIT;
			Schedule_Threads();
		}

		*value_ptr = (void *) tcb->retVal;
	}
	else
	{
		return -1;
	}

	return 0;
}



/********************************************************************/
// Uthread Exit
/********************************************************************/
void uthread_exit(void *value_ptr)
{
	sTCB *tcb_curr = Get_RunningThread();
	tcb_curr->state = STATE_EXIT;

	if(value_ptr != NULL)
		value_ptr = tcb_curr->retVal;

	Schedule_Threads();
}


/********************************************************************/
// Find the diff between two timespecs
/********************************************************************/
struct timespec timespec_Diff(struct timespec start, struct timespec end)
{
	struct timespec temp;
	if ( (end.tv_nsec-start.tv_nsec) < 0 )
	{
		temp.tv_sec = end.tv_sec-start.tv_sec-1;
		temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
	}
	else
	{
		temp.tv_sec = end.tv_sec-start.tv_sec;
		temp.tv_nsec = end.tv_nsec-start.tv_nsec;
	}
	return temp;
}



/********************************************************************/
// Uthread Get current time
/********************************************************************/
int uthread_gettime(struct timespec *tp)
{
	return clock_gettime(CLOCK_MONOTONIC, tp);
}



/********************************************************************/
// Uthread nanosleep
/********************************************************************/
int uthread_abstime_nanosleep(const struct timespec *request)
{
	sTCB *tcb_curr = Get_RunningThread();
	tcb_curr->state = STATE_BLOCKED;
	Sched_IdleTCB->state = STATE_READY;

	struct timespec timeCurr;
	uthread_gettime(&timeCurr);

	struct timespec interval = timespec_Diff(timeCurr, *request);

	Set_Timer(tcb_curr, &interval);

	Schedule_Threads();
}


/********************************************************************/
// Add the Mutex Block to Mutex List
/********************************************************************/
int Add_MutexToList(sMutexBlock *MuxBlk)
{
	if(UThread_MutexList.head == NULL)
	{
		MuxBlk->next = NULL;
		UThread_MutexList.head = MuxBlk;
		UThread_MutexList.tail = MuxBlk;
		UThread_MutexList.numMutex = 1;
	}
	else
	{
		MuxBlk->next = UThread_MutexList.tail->next;
		UThread_MutexList.tail->next = MuxBlk;

		UThread_MutexList.numMutex++;
	}

	return 0;
}



/********************************************************************/
// Find Mutex block using mutex value from the mutex list
/********************************************************************/
sMutexBlock *Find_Mutex(uthread_mutex_t *mutex)
{
	sMutexBlock *MuxBlk = UThread_MutexList.head;

	while(MuxBlk != NULL)
	{
		if(MuxBlk->mutex == mutex)
		{
			break;
		}
		MuxBlk = MuxBlk->next;
	}

	return MuxBlk;
}



/********************************************************************/
// Find waiting TCB from the specific mutex's wait list
/********************************************************************/
sMutexWaitList *Find_WaitListTCB(sMutexBlock *MuxBlk, sTCB *tcb)
{
	sMutexWaitList *WaitList = MuxBlk->waitList;

	while(WaitList != NULL)
	{
		if(WaitList->tcb == tcb && WaitList->valid == 1)
		{
			break;
		}

		WaitList = WaitList->next;
	}

	return WaitList;
}



/********************************************************************/
// Add a TCB to wait list
/********************************************************************/
int Add_WaitList(sMutexBlock *MuxBlk, sTCB *tcb)
{
	if(MuxBlk->waitList == NULL)
	{
		MuxBlk->waitList = (sMutexWaitList *) malloc(sizeof(sMutexWaitList));
		MuxBlk->waitList->next = NULL;
		MuxBlk->waitList_tail = NULL;
		MuxBlk->waitList->tcb = tcb;
		MuxBlk->waitList->valid = 1;
	}
	else
	{
		MuxBlk->waitList->valid = 1;
		MuxBlk->waitList->next = MuxBlk->waitList_tail->next;
		MuxBlk->waitList_tail->next = MuxBlk->waitList;
	}

	return 0;
}


/********************************************************************/
// Remove a TCB from wait list
/********************************************************************/
int Remove_WaitList(sMutexBlock *MuxBlk, sTCB *tcb)
{
	sMutexWaitList *obsolete = Find_WaitListTCB(MuxBlk, tcb);

	if(obsolete != NULL)
	{
		obsolete->valid = 0;
	}

	return 0;
}


/********************************************************************/
// Set all TCBs in wait list to ready state
/********************************************************************/
void MakeReady_WaitList(sMutexBlock *MuxBlk)
{
	sMutexWaitList *WaitList = MuxBlk->waitList;

	while(WaitList != NULL)
	{
		if(WaitList->valid == 1)
		{
			WaitList->tcb->state = STATE_READY;
		}

		WaitList = WaitList->next;
	}
}


/********************************************************************/
// Uthread mutex init
/********************************************************************/
int uthread_mutex_init(uthread_mutex_t *mutex, const int attr)
{
	sMutexBlock *MuxBlk = (sMutexBlock *) malloc((sizeof(sMutexBlock)));
	if(MuxBlk == NULL)
	{
		printf("\nError Allocating Memory for Mutex Block !!");
		return -1;
	}

	MuxBlk->mutex = mutex;
	MuxBlk->attr = attr;
	MuxBlk->state = MUTEXT_UNLOCKED;
	MuxBlk->ownerTCB = NULL;
	MuxBlk->mutexCeiling = PRIORITY_LOWEST;

	Add_MutexToList(MuxBlk);

	return 0;
}


/********************************************************************/
// Mutex Lock
/********************************************************************/
int uthread_mutex_lock(uthread_mutex_t *mutex)
{
	sMutexBlock *MuxBlk = Find_Mutex(mutex);

	sTCB *tcb_curr = Get_RunningThread();
	if(MuxBlk != NULL)
	{
		if(SRP_Enabled == 1)
		{
			MuxBlk->state = MUTEXT_LOCKED;
			if( MuxBlk->mutexCeiling < SystemCeiling )
			{
				SystemCeiling = MuxBlk->mutexCeiling;
			}
			MuxBlk->count++;
		}
		else
		{
			if(MuxBlk->state == MUTEXT_UNLOCKED)
			{
				MuxBlk->ownerTCB = tcb_curr;
				MuxBlk->state = MUTEXT_LOCKED;
				MuxBlk->count++;
			}
			else
			{
				Add_WaitList(MuxBlk, tcb_curr);
				tcb_curr->state = STATE_BLOCKED;

				if(MuxBlk->attr == UTHREAD_MUTEX_ATTR_PI)
				{
					MuxBlk->owener_OrgPrio = MuxBlk->ownerTCB->priority;
					MuxBlk->ownerTCB->priority = tcb_curr->priority;
					Reorder_Queue(MuxBlk->ownerTCB);
				}

				Schedule_Threads();
			}
		}
	}

	return 0;
}


/********************************************************************/
// Mutex Unlock
/********************************************************************/
int uthread_mutex_unlock(uthread_mutex_t *mutex)
{
	sMutexBlock *MuxBlk = Find_Mutex(mutex);

	sTCB *tcb_curr = Get_RunningThread();
	if(MuxBlk != NULL)
	{
		MuxBlk->state = MUTEXT_UNLOCKED;
		MuxBlk->count--;

		if(MuxBlk->count < 0)
			MuxBlk->count = 0;

		if(SRP_Enabled == 1)
		{
			if(MuxBlk->count == 0)
			{
				SystemCeiling = PRIORITY_LOWEST+1;
				CurrentThread->state = STATE_READY;
			}

			Schedule_Threads();
		}
		else
		if(MuxBlk->waitList != NULL)
		{
			MakeReady_WaitList(MuxBlk);

			if(MuxBlk->attr == UTHREAD_MUTEX_ATTR_PI)
			{
				if(MuxBlk->count == 0)
				{
					MuxBlk->ownerTCB->priority = MuxBlk->owener_OrgPrio;
					Reorder_Queue(MuxBlk->ownerTCB);
				}
			}

			Schedule_Threads();
		}
	}

	return 0;
}


/********************************************************************/
// Enable and init SRP
/********************************************************************/
int uthread_srp_enable(resource_entry_t *resource_table, int n)
{
	int i, j;

	SRP_Enabled = 1;
	
	for(i=0; i<n; i++)
	{
		sMutexBlock *MuxBlk = UThread_MutexList.head;//Find_Mutex( &(resource_table[i].resource) );
		for(j=0; j<resource_table[i].n; j++)
		{
			sTCB *tcb = Find_ThreadByID(resource_table[i].thread_array[j]);
			if( MuxBlk->mutexCeiling > tcb->priority )
			{
				MuxBlk->mutexCeiling = tcb->priority;
			}
			MuxBlk = MuxBlk->next;
		}
	}

	return 0;
}











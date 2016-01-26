/////////////////////////****  Assignment #3  ****////////////////////////
//
//	File Name:	 main.c
//	Author:		 Ramsundar K G
//	Date:		 
//
//	Description: Containts the main function
//
//////////////////////////////////////////////////////////////////////////


/*************************  Includes  *******************************/

#include "Scheduler.h"

/********************************************************************/

/*************************  Variables  ******************************/
int Sched_CurrThreadID = 0;
sSched_Queue Sched_PriorityQueue;
sTCB *Sched_IdleTCB;
sTCB *CurrentThread;
/********************************************************************/


void *Idle_Thread()
{
	while(1)
	{
		#ifdef DEBUG_PRINT
		printf("\nIdle");
		#endif
		sleep(1);
	}
}


int init_Idle_Thread()
{
	Sched_IdleTCB = (sTCB *) malloc(sizeof(sTCB));
	Sched_IdleTCB->context = Prepare_Context((void*)Idle_Thread, NULL, UTHREAD_STACK_SIZE, 1, NULL, NULL);
	if(Sched_IdleTCB->context == NULL)
	{
		printf("\nError Allocating Memory for Thread Context !!");
		return -1;
	}

	Sched_IdleTCB->state = STATE_READY;
	Sched_IdleTCB->id = -2;
	Sched_IdleTCB->priority = PRIORITY_LOWEST+2;

	Add_ThreadToQueue(Sched_IdleTCB);

	return 0;
}


sTCB* Find_PriorityInsert(int priority)
{
	sTCB *tcb = Sched_PriorityQueue.head;

	while(tcb != NULL)
	{
		if(tcb->priority > priority)
		{
			break;
		}

		tcb = tcb->next;
	}

	return tcb;
}



void Instert_Before(sTCB *tcb_Old, sTCB* tcb_New)
{
	tcb_New->prev = tcb_Old->prev;
	tcb_New->next = tcb_Old;
	if(tcb_Old->prev == NULL)
	{
		Sched_PriorityQueue.head = tcb_New;
	}
	else
	{
		tcb_Old->prev->next = tcb_New;
	}
	tcb_Old->prev = tcb_New;
}


void Instert_After(sTCB *tcb_Old, sTCB* tcb_New)
{
	tcb_New->prev = tcb_Old;
	tcb_New->next = tcb_Old->next;
	if(tcb_Old->next == NULL)
	{
		Sched_PriorityQueue.tail = tcb_New;
	}
	else
	{
		tcb_Old->next->prev = tcb_New;
	}
	tcb_Old->next = tcb_New;
}


int Add_ThreadToQueue(sTCB *tcb)
{
	if(Sched_PriorityQueue.head == NULL)
	{
		tcb->next = NULL;
		tcb->prev = NULL;
		Sched_PriorityQueue.head = tcb;
		Sched_PriorityQueue.tail = tcb;
		//Sched_PriorityQueue.curr = tcb;
		Sched_PriorityQueue.numThreads = 1;
	}
	else
	{
		sTCB *tcb_Old = Find_PriorityInsert(tcb->priority);
		if(tcb_Old != NULL)
		{
			Instert_Before(tcb_Old, tcb);
		}
		else
		{
			Instert_After(Sched_PriorityQueue.tail, tcb);
		}

		//Sched_PriorityQueue.curr = tcb;
		Sched_PriorityQueue.numThreads++;
	}

	return 0;
}


int Delete_ThreadFromQueue(sTCB *tcb)
{
	if(Sched_PriorityQueue.head == NULL)
	{
		return -1;
	}

	if(tcb->prev == NULL)
	{
		Sched_PriorityQueue.head = tcb->next;
	}
	else
	{
		tcb->prev->next = tcb->next;
	}

	if(tcb->prev != NULL)
	{
		tcb->next->prev = tcb->prev;
	}

	Sched_PriorityQueue.numThreads--;

	free(tcb);
	
	return 0;
}



int Reorder_Queue(sTCB *tcb)
{
	sTCB *tcb_new = (sTCB *) malloc(sizeof(sTCB));
	*tcb_new = *tcb;

	Add_ThreadToQueue(tcb_new);
	Delete_ThreadFromQueue(tcb);

	return 0;
}



sTCB* Find_ThreadByID(int id)
{
	sTCB *tcb = Sched_PriorityQueue.head;

	while(tcb != NULL)
	{
		if(tcb->id == id)
		{
			break;
		}
		tcb = tcb->next;
	}

	return tcb;
}


sTCB* Find_ThreadByState(int state)
{
	sTCB *tcb = Sched_PriorityQueue.head;

	while(tcb != NULL)
	{
		if((tcb->state==STATE_JOIN_WAIT) && (tcb->joinWait->state == STATE_EXIT)) 
		{
			tcb->state = STATE_READY;
		}
		if(tcb->state == state)
		{
			break;
		}
		tcb = tcb->next;
	}

	return tcb;
}


sTCB* Get_RunningThread()
{
	return CurrentThread;
}


sTCB* Get_PriorityReadyThread()
{
	return Find_ThreadByState(STATE_READY);
}


void Delete_CurrentThread()
{
	sTCB *tcb_curr = Get_RunningThread();

	Delete_ThreadFromQueue(tcb_curr);
}


void Schedule_Threads()
{
	sTCB *tcb_curr = Get_RunningThread();
	sTCB *tcb_toRun = Get_PriorityReadyThread();

	if(tcb_toRun == NULL)
	{
		return;
	}

	if(tcb_curr == tcb_toRun)
	{
		return;
	}

	if(tcb_curr->state == STATE_READY || tcb_curr->state == STATE_RUNNING)
	{
		if( (SRP_Enabled == 1) )
		{
			if(tcb_toRun->priority < SystemCeiling)
			{
				if(tcb_curr->priority > tcb_toRun->priority)
				{
					tcb_curr->state = STATE_READY;
					tcb_toRun->state = STATE_RUNNING;

					CurrentThread = tcb_toRun;
					swapcontext(tcb_curr->context, tcb_toRun->context);
				}
			}
		}
		else
		{
			if(tcb_curr->priority > tcb_toRun->priority)
			{
				tcb_curr->state = STATE_READY;
				tcb_toRun->state = STATE_RUNNING;

				CurrentThread = tcb_toRun;
				swapcontext(tcb_curr->context, tcb_toRun->context);
			}
		}

	}
	else
	{
		tcb_toRun->state = STATE_RUNNING;

		CurrentThread = tcb_toRun;
		swapcontext(tcb_curr->context, tcb_toRun->context);
	}
}


void Block_TimerSignal(int sig)
{
  sigset_t mask;
  sigemptyset(&mask);
  sigaddset(&mask, sig);
  sigprocmask(SIG_BLOCK, &mask, NULL);
}


void Unblock_TimerSignal(int sig)
{
  sigset_t mask;
  sigemptyset(&mask);
  sigaddset(&mask, sig);
  sigprocmask(SIG_UNBLOCK, &mask,NULL);
}


void Signal_Handler(int sig, siginfo_t * siginfo, void *arg)
{
	sTCB *tcb = (sTCB *)(siginfo->si_value.sival_ptr);
	timer_delete(tcb->timerID);
	tcb->state = STATE_READY;

	Schedule_Threads();
}


void Init_TimerSignal()
{
	Block_TimerSignal(SIGVTALRM);

	struct sigaction sa;
	sa.sa_flags = SA_SIGINFO;
	sa.sa_sigaction = Signal_Handler;
	sigemptyset(&sa.sa_mask);
	sigaction(SIGVTALRM, &sa, NULL);

	Unblock_TimerSignal(SIGVTALRM);
}


void Set_Timer(sTCB *tcb, const struct timespec *request)
{
	Block_TimerSignal(SIGVTALRM);

	struct sigevent event;
	event.sigev_signo = SIGVTALRM;
	event.sigev_notify = SIGEV_SIGNAL;
	event.sigev_value.sival_ptr = (void *)tcb;
	timer_create(CLOCK_MONOTONIC, &event, &tcb->timerID);

	struct itimerspec timerSpec;
	timerSpec.it_value.tv_sec = request->tv_sec;
	timerSpec.it_value.tv_nsec = request->tv_nsec;
	timerSpec.it_interval.tv_sec = request->tv_sec;
	timerSpec.it_interval.tv_nsec = request->tv_nsec;

	Unblock_TimerSignal(SIGVTALRM);

	timer_settime(tcb->timerID, 0, &timerSpec, NULL);
}

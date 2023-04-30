#include "ec440threads.h"
#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>
#include <setjmp.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

/* You can support more threads. At least support this many. */
#define MAX_THREADS 128

/* Your stack should be this many bytes in size */
#define THREAD_STACK_SIZE 32767

/* Number of microseconds between scheduling events */
#define SCHEDULER_INTERVAL_USECS (50 * 1000)

/* Extracted from private libc headers. These are not part of the public
 * interface for jmp_buf.
 */
#define JB_RBX 0
#define JB_RBP 1
#define JB_R12 2
#define JB_R13 3
#define JB_R14 4
#define JB_R15 5
#define JB_RSP 6
#define JB_PC 7

#define MAIN_THREAD_ID 0

/* Used to check if a thread is not valid in mutex and barrier init */
#define ERROR_THREAD_ID 1000
/* thread_status identifies the current state of a thread. You can add, rename,
 * or delete these values. This is only a suggestion. */
enum thread_status
{
	TS_EXITED,
	TS_RUNNING,
	TS_READY,
	TS_BLOCKED
};

/* array of TCBS (no need to delete exited thread) */
struct thread_control_block* TCB_arr[MAX_THREADS];
/* current running thread */
pthread_t glb_thread_curr = -1;
/* count thread id */
int pthread_idNum = 0;
/*signal sets*/
sigset_t new_set, old_set;



/* The thread control block stores information about a thread. You will
 * need one of this per thread.
 */
struct thread_control_block {
	/* TODO: add a thread ID */
	/* TODO: add information about its stack */
	/* TODO: add information about its registers */
	/* TODO: add information about the status (e.g., use enum thread_status) */
	/* Add other information you need to manage this thread */
	pthread_t t_id;
	enum thread_status t_status;
	void *t_stackTail;
	jmp_buf t_env;
	int exit_status;//not necessary, just 0 for assignment
	void* block;
};

/* Barrier Union */
union barrier_t{
	struct{
		int count;//total threads to count to
		int current_count;//current number of threads blocked
		pthread_t *barrier_blocked_arr; //store ids of the barrier blocked threads
		int number_of_returns;
	};
	pthread_barrier_t barrier;
};

/* Mutex Union */
union mutex_t{
	struct{
		int lock;//0 or 1 on lock
		int current_count;//current number of threads blocked
		pthread_t *mutex_blocked_arr;
	};
	pthread_mutex_t mutex;
};


static void schedule(int signal)
{
	/* TODO: implement your round-robin scheduler 
	 * 1. Use setjmp() to update your currently-active thread's jmp_buf
	 *    You DON'T need to manually modify registers here.
	 * 2. Determine which is the next thread that should run
	 * 3. Switch to the next thread (use longjmp on that thread's jmp_buf)
	 */
	if(TCB_arr[pthread_self()]->t_status!=TS_BLOCKED){
		if(TCB_arr[pthread_self()]->t_status!=TS_EXITED){
			TCB_arr[pthread_self()]->t_status=TS_READY;
		}
	}
	if(sigsetjmp(TCB_arr[pthread_self()]->t_env,1)==0){
		//add logic to check if next thread is ready then jmp to that 
		while(1){
			glb_thread_curr++;
			if(glb_thread_curr==MAX_THREADS){
				glb_thread_curr=0;
			}
			if(TCB_arr[pthread_self()]!=NULL){
				if(TCB_arr[pthread_self()]->t_status==TS_READY){
					TCB_arr[pthread_self()]->t_status=TS_RUNNING;
					break;
				}
			}
		}
		ualarm(SCHEDULER_INTERVAL_USECS,0);
		siglongjmp(TCB_arr[pthread_self()]->t_env,1);
	}
	// Fall through if just longjmp'd, the thread resumes execution
	/* Free ALL TS_EXITED threads */
	for(int i=1;i<MAX_THREADS;i++){
		if(TCB_arr[i]!=NULL&&TCB_arr[i]->t_status==TS_EXITED){
			free(TCB_arr[i]->t_stackTail);
			free(TCB_arr[i]);
			TCB_arr[i]=NULL;
		}
	}
}

static void scheduler_init()
{
	/* TODO: do everything that is needed to initialize your scheduler. For example:
	 * - Allocate/initialize global threading data structures
	 * - Create a TCB for the main thread. Note: This is less complicated
	 *   than the TCBs you create for all other threads. In this case, your
	 *   current stack and registers are already exactly what they need to be!
	 *   Just make sure they are correctly referenced in your TCB.
	 * - Set up your timers to call schedule() at a 50 ms interval (SCHEDULER_INTERVAL_USECS)
	 */

	/* setup alarm timer and signal */
	struct sigaction alrm_struct;
    alrm_struct.sa_flags = SA_NODEFER;
    alrm_struct.sa_handler = schedule;
    sigaction(SIGALRM,&alrm_struct,0);

	/*setup new and old sets*/
	sigemptyset(&new_set);
	sigemptyset(&old_set);
    sigaddset(&new_set, SIGALRM);

	/* setup main thread */
	//No need to do anything with jmpbuf
	TCB_arr[MAIN_THREAD_ID] = (struct thread_control_block*)malloc(sizeof(struct thread_control_block));
	TCB_arr[MAIN_THREAD_ID]->t_id = MAIN_THREAD_ID;
	TCB_arr[MAIN_THREAD_ID]->t_status = TS_RUNNING;
	TCB_arr[MAIN_THREAD_ID]->t_stackTail = NULL;
	glb_thread_curr = MAIN_THREAD_ID;

	ualarm(SCHEDULER_INTERVAL_USECS,0);
}

//disable signals so that the thread can run in Critical section
static void lock(){
	if(sigprocmask(SIG_BLOCK,&new_set, &old_set)!=0){
		perror("lock: sigprocmask failure");
	}
}

//enable signals so that the thread can run after Critical section
static void unlock(){
	if(sigprocmask(SIG_SETMASK, &old_set,NULL)!=0){
		perror("unlock: sigprocmask failure");
	}
}

//initialize a given mutex
int pthread_mutex_init(pthread_mutex_t *restrict mutex,const pthread_mutexattr_t *restrict attr){
	// union mutex_t mutex_union;
	// mutex_union.current_count = 0;
	// mutex_union.lock = 0;
	// mutex_union.mutex_blocked_arr = (pthread_t*)malloc(MAX_THREADS*sizeof(pthread_t));
	// for(int i=0;i<MAX_THREADS;i++){
	// 	mutex_union.mutex_blocked_arr[i] = ERROR_THREAD_ID;
	// }
	//memset(mutex_union.mutex_blocked_arr,ERROR_THREAD_ID,sizeof(mutex_union.mutex_blocked_arr));
	//memcpy(mutex,&mutex_union,sizeof(pthread_mutex_t));
	mutex->__data.__lock = 0;
	return 0;
}

/*destroy given mutex
* check if lock is greater than 0 before use
*/
int pthread_mutex_destroy(pthread_mutex_t *mutex){
	// union mutex_t mutex_union;
	// memcpy(&mutex_union,mutex,sizeof(pthread_mutex_t));
	// mutex_union.current_count = -1;
	// mutex_union.lock = -1;
	// free(mutex_union.mutex_blocked_arr);
	mutex->__data.__lock = -1;
	return 0;
}

int pthread_mutex_lock(pthread_mutex_t *mutex){
	// union mutex_t mutex_union;
	// memcpy(&mutex_union,mutex,sizeof(pthread_mutex_t));
	//lock for mutex lock and check if lock not aquired
	if(mutex->__data.__lock == -1){
		return -1;
	}
	lock();
	if(mutex->__data.__lock==1){
		TCB_arr[pthread_self()]->t_status = TS_BLOCKED;
		TCB_arr[pthread_self()]->block = mutex;
	}
	//while lock cannot be aquired
	while(mutex->__data.__lock==1){
		//if lock not available (lock=1)
		//memcpy(mutex,&mutex_union,sizeof(pthread_mutex_t));
		unlock();
		//scheduler only runs ready threads
		schedule(1);
		lock();
	}
	//if lock can be aquired
	mutex->__data.__lock=1;
	//memcpy(mutex,&mutex_union,sizeof(pthread_mutex_t));
	unlock();
	return 0;
}

int pthread_mutex_unlock(pthread_mutex_t *mutex){
	//union mutex_t mutex_union;
	//memcpy(&mutex_union,mutex,sizeof(pthread_mutex_t));
	//thread should be locked before executing unlock
	if(mutex->__data.__lock == -1){
		return -1;
	}
	lock();
	mutex->__data.__lock=0;
	for(int i=0;i<MAX_THREADS;i++){
		if(TCB_arr[i]!=NULL&&TCB_arr[i]->block==mutex){
			if(TCB_arr[i]->t_status==TS_BLOCKED){
				TCB_arr[i]->t_status=TS_READY;
				TCB_arr[i]->block = NULL;
			}
		}
	}
	//memcpy(mutex,&mutex_union,sizeof(pthread_mutex_t));
	unlock();
	return 0;
}

//initualize the barrier
int pthread_barrier_init(pthread_barrier_t *restrict barrier,const pthread_barrierattr_t *restrict attr,unsigned count){
	if(count<=0){
		return EINVAL;
	}
	union barrier_t barr_union;
	barr_union.count = count;
	barr_union.current_count = 0;
	barr_union.number_of_returns = 0;
	//barr_union.barrier_blocked_arr = (pthread_t*)malloc(count*sizeof(pthread_t));
	//memset(barr_union.barrier_blocked_arr,ERROR_THREAD_ID,sizeof(barr_union.barrier_blocked_arr));
	memcpy(barrier,&barr_union,sizeof(pthread_barrier_t));
	return 0;
}

//Use the barrier and wait
int pthread_barrier_wait(pthread_barrier_t *barrier){
	lock();
	union barrier_t barr_union;
	memcpy(&barr_union,barrier,sizeof(pthread_barrier_t));
	//if not reached the no. of threads needed to unblock
	barr_union.current_count++;
	while(barr_union.current_count<barr_union.count){
		//barr_union.barrier_blocked_arr[barr_union.current_count-1] = pthread_self();
		memcpy(barrier,&barr_union,sizeof(pthread_barrier_t));
		TCB_arr[pthread_self()]->t_status = TS_BLOCKED;
		TCB_arr[pthread_self()]->block = barrier;
		unlock();
		schedule(1);
		memcpy(&barr_union,barrier,sizeof(pthread_barrier_t));
		lock();

	}
	if(barr_union.current_count==barr_union.count){
		barr_union.number_of_returns++;
		memcpy(barrier,&barr_union,sizeof(pthread_barrier_t));
		//unblock barrier threads when threads fall here
		for(int i=0;i<MAX_THREADS;i++){
			if(TCB_arr[i]!=NULL&&TCB_arr[i]->block==barrier){
				if(TCB_arr[i]->t_status==TS_BLOCKED){
					TCB_arr[i]->t_status=TS_READY;
					TCB_arr[i]->block=NULL;
				}
			}
		}
		unlock();
		return PTHREAD_BARRIER_SERIAL_THREAD;
	}
	barr_union.number_of_returns++;
	if(barr_union.number_of_returns==barr_union.count){
		barr_union.current_count=0;
		barr_union.number_of_returns=0;
	}
	memcpy(barrier,&barr_union,sizeof(pthread_barrier_t));
	unlock();
	return 0;
}

//destroy the barrier
int pthread_barrier_destroy(pthread_barrier_t *barrier){
	union barrier_t barr_union;
	barr_union.count = 0;
	barr_union.current_count = 0;
	memcpy(barrier,&barr_union,sizeof(pthread_barrier_t));
	return 0;
}

int pthread_create(
	pthread_t *thread, const pthread_attr_t *attr,
	void *(*start_routine) (void *), void *arg)
{
	// Create the timer and handler for the scheduler. Create thread 0.
	static bool is_first_call = true;
	if (is_first_call)
	{
		is_first_call = false;
		scheduler_init();
	}

	/* error check pthread ids for exceeding max limit */
	pthread_idNum++;// 0 (main) to 127 in TCB_arr
	bool id_found = false;
	if(pthread_idNum==MAX_THREADS){
		for(int i = 1;i < MAX_THREADS;i++){
			/* || will not evaluate second operand when first is true */
			if(TCB_arr[i]==NULL || TCB_arr[i]->t_status == TS_EXITED){
				/* free memory from this one exited thread if needed */
				// if(TCB_arr[i]->t_status == TS_EXITED){
				// 	free(TCB_arr[i]->t_stackTail);
				// 	free(TCB_arr[i]);
				// 	TCB_arr[i]=NULL;
				// }
				/* set the pthread_idNum to replace the exited thread */
				pthread_idNum = i;
				id_found = true;
				break;
			}
		}
		if(!id_found){
			/* return non-zero on fail */
			return -1;
		}
	}
	
	/* malloc TCB for new thread and init values */
	TCB_arr[pthread_idNum] = (struct thread_control_block*)malloc(sizeof(struct thread_control_block));
	TCB_arr[pthread_idNum]->t_id = pthread_idNum;
	TCB_arr[pthread_idNum]->t_stackTail = malloc(THREAD_STACK_SIZE);
	TCB_arr[pthread_idNum]->block = NULL;
	/* setup stack with pthread_exit */
	void* stk_exit = TCB_arr[pthread_idNum]->t_stackTail + THREAD_STACK_SIZE - 8;
	*(unsigned long int *)stk_exit = (unsigned long int)&pthread_exit;
	/* setup registers */
	sigsetjmp(TCB_arr[pthread_idNum]->t_env,1);
	TCB_arr[pthread_idNum]->t_env->__jmpbuf[JB_R12] = (unsigned long int)start_routine;
	TCB_arr[pthread_idNum]->t_env->__jmpbuf[JB_R13] = (unsigned long int)arg;
	TCB_arr[pthread_idNum]->t_env->__jmpbuf[JB_RSP] = ptr_mangle((unsigned long int)stk_exit);
	TCB_arr[pthread_idNum]->t_env->__jmpbuf[JB_PC] = ptr_mangle((unsigned long int)&start_thunk);
	*thread = pthread_idNum;
	TCB_arr[pthread_idNum]->t_status = TS_READY;
	/* TODO: Return 0 on successful thread creation, non-zero for an error.
	 *       Be sure to set *thread on success.
	 * Hints:
	 * The general purpose is to create a TCB:
	 * - Create a stack.
	 * - Assign the stack pointer in the thread's registers. Important: where
	 *   within the stack should the stack pointer be? It may help to draw
	 *   an empty stack diagram to answer that question.
	 * - Assign the program counter in the thread's registers.
	 * - Wait... HOW can you assign registers of that new stack? 
	 *   1. call setjmp() to initialize a jmp_buf with your current thread
	 *   2. modify the internal data in that jmp_buf to create a new thread environment
	 *      env->__jmpbuf[JB_...] = ...
	 *      See the additional note about registers below
	 *   3. Later, when your scheduler runs, it will longjmp using your
	 *      modified thread environment, which will apply all the changes
	 *      you made here.
	 * - Remember to set your new thread as TS_READY, but only  after you
	 *   have initialized everything for the new thread.
	 * - Optionally: run your scheduler immediately (can also wait for the
	 *   next scheduling event).
	 */
	/*
	 * Setting registers for a new thread:
	 * When creating a new thread that will begin in start_routine, we
	 * also need to ensure that `arg` is passed to the start_routine.
	 * We cannot simply store `arg` in a register and set PC=start_routine.
	 * This is because the AMD64 calling convention keeps the first arg in
	 * the EDI register, which is not a register we control in jmp_buf.
	 * We provide a start_thunk function that copies R13 to RDI then jumps
	 * to R12, effectively calling function_at_R12(value_in_R13). So
	 * you can call your start routine with the given argument by setting
	 * your new thread's PC to be ptr_mangle(start_thunk), and properly
	 * assigning R12 and R13.
	 *
	 * Don't forget to assign RSP too! Functions know where to
	 * return after they finish based on the calling convention (AMD64 in
	 * our case). The address to return to after finishing start_routine
	 * should be the first thing you push on your stack.
	 */
	//schedule(0);
	return 0;
}

void pthread_exit(void *value_ptr)
{
	/* TODO: Exit the current thread instead of exiting the entire process.
	 * Hints:
	 * - Release all resources for the current thread. CAREFUL though.
	 *   If you free() the currently-in-use stack then do something like
	 *   call a function or add/remove variables from the stack, bad things
	 *   can happen.
	 * - Update the thread's status to indicate that it has exited
	 */
	TCB_arr[pthread_self()]->t_status = TS_EXITED;
	ualarm(0,0);
	schedule(1);
	exit(1);
}

pthread_t pthread_self(void)
{
	/* TODO: Return the current thread instead of -1
	 * Hint: this function can be implemented in one line, by returning
	 * a specific variable instead of -1.
	 */
	return glb_thread_curr;
}

/* Don't implement main in this file!
 * This is a library of functions, not an executable program. If you
 * want to run the functions in this file, create separate test programs
 * that have their own main functions.
 */
#include "ec440threads.h"
#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>
#include <setjmp.h>
#include <signal.h>

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

/* thread_status identifies the current state of a thread. You can add, rename,
 * or delete these values. This is only a suggestion. */
enum thread_status
{
	TS_EXITED,
	TS_RUNNING,
	TS_READY
};

/* array of TCBS (no need to delete exited thread) */
struct thread_control_block* TCB_arr[MAX_THREADS];
/* current running thread */
pthread_t glb_thread_curr = -1;
/* count thread id */
int pthread_idNum = 0;

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
	int exit_status;
	//struct thread_control_block* t_next;
};

static void schedule(int signal)
{
	/* TODO: implement your round-robin scheduler 
	 * 1. Use setjmp() to update your currently-active thread's jmp_buf
	 *    You DON'T need to manually modify registers here.
	 * 2. Determine which is the next thread that should run
	 * 3. Switch to the next thread (use longjmp on that thread's jmp_buf)
	 */
	TCB_arr[pthread_self()]->t_status==TS_READY;
	if(setjmp(TCB_arr[pthread_self()]->t_env)==0){
		//add logic to check if next thread is ready then jmp to that 
		while(1){
			glb_thread_curr++;
			if(glb_thread_curr==MAX_THREADS){
				glb_thread_curr=0;
			}
			if(TCB_arr[pthread_self()]->t_status==TS_READY){
				TCB_arr[pthread_self()]->t_status==TS_RUNNING;
				break;
			}
		}
		longjmp(TCB_arr[pthread_self()]->t_env,1);
	}
	// Fall through if just longjmp'd, the thread resumes execution
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

	/* setup main thread */
	//jmp_buf main_buf;
	TCB_arr[MAIN_THREAD_ID] = (struct thread_control_block*)malloc(sizeof(struct thread_control_block));
	TCB_arr[MAIN_THREAD_ID]->t_id = MAIN_THREAD_ID;
	TCB_arr[MAIN_THREAD_ID]->t_status = TS_RUNNING;
	TCB_arr[MAIN_THREAD_ID]->t_stackTail = NULL;
	//TCB_arr[MAIN_THREAD_ID]->t_env = main_buf;
	//TCB_arr[MAIN_THREAD_ID]->t_next = NULL;

	ualarm(SCHEDULER_INTERVAL_USECS,0);
}

/* move exit to top of stack for pthread_create */
void pthread_init(int id){
	void* stk_exit = TCB_arr[id]->t_stackTail + THREAD_STACK_SIZE - 8;
	*(unsigned long int *)stk_exit = (unsigned long int)&pthread_exit;
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

	/* malloc TCB for new thread */
	pthread_idNum++;
	TCB_arr[pthread_idNum] = (struct thread_control_block*)malloc(sizeof(struct thread_control_block));
	TCB_arr[pthread_idNum]->t_id = pthread_idNum;
	TCB_arr[pthread_idNum]->t_stackTail = malloc(THREAD_STACK_SIZE);
	pthread_init(pthread_idNum);
	TCB_arr[pthread_idNum]->t_status = TS_READY;
	// struct thread_control_block* n_thread = (struct thread_control_block*)malloc(sizeof(struct thread_control_block));
	// void* t_stack = malloc(THREAD_STACK_SIZE);
	// n_thread->t_stackTail = t_stack;
	// n_thread->t_id = pthread_idNum++;
	// n_thread->t_status = TS_READY;
	// pthread_init(n_thread,t_stack);
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
	return -1;
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
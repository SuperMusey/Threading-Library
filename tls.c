#include "tls.h"
#include <signal.h>
#include <unistd.h>

#define MAX_THREAD_COUNT 128

/*
 * This is a good place to define any data structures you will use in this file.
 * For example:
 *  - struct TLS: may indicate information about a thread's local storage
 *    (which thread, how much storage, where is the storage in memory)
 *  - struct page: May indicate a shareable unit of memory (we specified in
 *    homework prompt that you don't need to offer fine-grain cloning and CoW,
 *    and that page granularity is sufficient). Relevant information for sharing
 *    could be: where is the shared page's data, and how many threads are sharing it
 *  - Some kind of data structure to help find a TLS, searching by thread ID.
 *    E.g., a list of thread IDs and their related TLS structs, or a hash table.
 */
typedef struct thread_local_storage{
	pthread_t tid;
	unsigned int size;
	unsigned int page_num;
	struct page **pages;	
} TLS;

struct page{
	unsigned int address;
	int ref_count;
};

struct tls_tid{
	pthread_t tid;
	TLS *tls;
};

/*
 * Now that data structures are defined, here's a good place to declare any
 * global variables.
 */

struct tls_tid tls_tid_pairs[MAX_THREAD_COUNT];
int pageSize;

/*
 * With global data declared, this is a good point to start defining your
 * static helper functions.
 */

void tls_handler(int sig, siginfo_t *si, void *context){
	/*get the page at which the fault occured
	* binary arithmetic remove the page offset from the address
	* by ANDing with 0's of pagesize bits to get the top half 
	* of the address which is the page number
	*/
	unsigned int p_fault_num = *((unsigned int*)si->si_addr) & ~(pageSize-1);

	/*check if segfault was cause by thread
	* touching forbidden memory
	*/
	for(int i=0;i<MAX_THREAD_COUNT;i++){
		if(tls_tid_pairs[i].tid == pthread_self() || tls_tid_pairs[i].tls == NULL){
			continue;
		}
		for(int i=0;i<tls_tid_pairs[i].tls->page_num;i++){
			if(((unsigned int)tls_tid_pairs[i].tls->pages[i]->address & ~(pageSize-1))==p_fault_num){
				pthread_exit(0);
			}
		}
	}
	/*otherwise it is normal fault
	*/
	signal(SIGSEGV,SIG_DFL);
	signal(SIGBUS,SIG_DFL);
	raise(sig);
}

void tls_init(){
	/* init tls_tid_pairs
	*/
	for(int i=0;i<MAX_THREAD_COUNT;i++){
		tls_tid_pairs[i].tid = 0;
		tls_tid_pairs[i].tls = NULL;
	}

	pageSize = getpagesize();
	struct sigaction sigact;
	/*Handle page faults
	* SIGSEGV, SIGBUS
	*/
	sigact.sa_flags = SA_SIGINFO;
	sigact.sa_sigaction = tls_handler;
	sigaction(SIGBUS,&sigact,NULL);
	sigaction(SIGSEGV,&sigact,NULL);
} 

/*
 * Lastly, here is a good place to add your externally-callable functions.
 */ 

int tls_create(unsigned int size)
{
	return 0;
}

int tls_destroy()
{
	return 0;
}

int tls_read(unsigned int offset, unsigned int length, char *buffer)
{
	return 0;
}

int tls_write(unsigned int offset, unsigned int length, const char *buffer)
{
	return 0;
}

int tls_clone(pthread_t tid)
{
	return 0;
}

#include "tls.h"
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdlib.h>

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
	unsigned int size;//size in bytes
	unsigned int page_num;//number of pages
	struct page **pages;//array of pointers to pages
} TLS;

struct page{
	unsigned long int address;
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
static int first_create = 1;

/*
 * With global data declared, this is a good point to start defining your
 * static helper functions.
 */

void tls_protect(struct page *p){
	/*protect the given page from all access
	*/
	if(mprotect((void*)(p->address),pageSize,PROT_NONE)<0){
		fprintf(stderr, "tls_protect: could not protect page\n");
		exit(0);
	}
}

void tls_read_unprotect(struct page *p){
	/*unprotect the given page from read
	*/
	if(mprotect((void*)(p->address),pageSize,PROT_READ)<0){
		fprintf(stderr, "tls_read_unprotect: could not unprotect page\n");
		exit(0);
	}
}

void tls_write_unprotect(struct page *p){
	/*unprotect the given page from read
	*/
	if(mprotect((void*)(p->address),pageSize,PROT_WRITE)<0){
		fprintf(stderr, "tls_write_unprotect: could not unprotect page\n");
		exit(0);
	}
}

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
			if((tls_tid_pairs[i].tls->pages[i]->address & ~(pageSize-1))==p_fault_num){
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
	/*error checks
	* check if thread has more than 0 bytes storage
	*/
	pthread_t tid = pthread_self();
	for(int i=0;i<MAX_THREAD_COUNT;i++){
		if(tls_tid_pairs[i].tid == tid && tls_tid_pairs[i].tls != NULL){
			if(tls_tid_pairs[i].tls->size>0){
				return -1;
			}
		}
	}
	/*on first create
	*/
	if(first_create){
		first_create = 0;
		tls_init();
	}

	/*Allocate TLS using calloc/malloc
	*/
	//TLS *tls_made = (TLS*)malloc(sizeof(TLS));



	return 0;
}

int tls_destroy()
{
	/*error checks
	* check if thread does not have LSA
	*/
	pthread_t tid = pthread_self();
	int found_tls = 0;
	for(int i=0;i<MAX_THREAD_COUNT;i++){
		if(tls_tid_pairs[i].tid == tid && tls_tid_pairs[i].tls != NULL){
			found_tls = 1;
		}
	}
	if(!found_tls){
		return -1;
	}
	return 0;
}

int tls_read(unsigned int offset, unsigned int length, char *buffer)
{
	/*error checks
	* check if thread does not have LSA
	* check if we read more data than possible
	*/
	pthread_t tid = pthread_self();
	int found_tls = 0;
	int tls_tid_indx = -1;
	for(int i=0;i<MAX_THREAD_COUNT;i++){
		if(tls_tid_pairs[i].tid == tid && tls_tid_pairs[i].tls != NULL){
			found_tls = 1;
			tls_tid_indx = i;
		}
	}
	if(!found_tls){
		return -1;
	}
	if(tls_tid_pairs[tls_tid_indx].tls->size<offset+length){
		return -1;
	}

	/*unprotect all pages for read
	*/
	for(int i=0;i<tls_tid_pairs[tid].tls->page_num;i++){
		tls_read_unprotect(tls_tid_pairs[tid].tls->pages[i]);
	}

	/*perform read
	*/
	int buf_idx=0;
	int byt_idx=0;
	for(buf_idx=0,byt_idx=offset;byt_idx<(offset+length);byt_idx++,buf_idx++){
		unsigned long int page_number = byt_idx/pageSize;
		int page_byte_offset = byt_idx%pageSize;
		buffer[buf_idx] = *((char*)((void*)(tls_tid_pairs[tid].tls->pages[page_number]->address+page_byte_offset)));
	}

	/*reprotect all pages
	*/
	for(int i=0;i<tls_tid_pairs[tid].tls->page_num;i++){
		tls_protect(tls_tid_pairs[tid].tls->pages[i]);
	}

	return 0;
}

int tls_write(unsigned int offset, unsigned int length, const char *buffer)
{
	/*error checks
	* check if thread does not have LSA
	* check if we read more data than possible
	*/
	pthread_t tid = pthread_self();
	int found_tls = 0;
	int tls_tid_indx = -1;
	for(int i=0;i<MAX_THREAD_COUNT;i++){
		if(tls_tid_pairs[i].tid == tid && tls_tid_pairs[i].tls != NULL){
			found_tls = 1;
			tls_tid_indx = i;
		}
	}
	if(!found_tls){
		return -1;
	}
	if(tls_tid_pairs[tls_tid_indx].tls->size<offset+length){
		return -1;
	}

	/*unprotect all pages for write
	*/
	for(int i=0;i<tls_tid_pairs[tid].tls->page_num;i++){
		tls_write_unprotect(tls_tid_pairs[tid].tls->pages[i]);
	}

	/*perform write
	* if page is shared, COW implementation
	*/
	int buf_idx=0;
	int byt_idx=0;
	for(buf_idx=0,byt_idx=offset;byt_idx<(offset+length);byt_idx++,buf_idx++){
		unsigned long int page_number = byt_idx/pageSize;
		int page_byte_offset = byt_idx%pageSize;
		/*COW check in other pages
		*/
		if(tls_tid_pairs[tid].tls->pages[page_number]->ref_count>1){
			
		}
		*((char*)((void*)(tls_tid_pairs[tid].tls->pages[page_number]->address+page_byte_offset)))=buffer[buf_idx];
	}


	/*reprotect all pages
	*/
	for(int i=0;i<tls_tid_pairs[tid].tls->page_num;i++){
		tls_protect(tls_tid_pairs[tid].tls->pages[i]);
	}

	return 0;
}

int tls_clone(pthread_t tid)
{
	/*error checks
	* check if calling thread has a LSA
	* or if target has no LSA
	*/
	pthread_t ctid = pthread_self();
	int found_target = 0;
	for(int i=0;i<MAX_THREAD_COUNT;i++){
		if(tls_tid_pairs[i].tid == ctid && tls_tid_pairs[i].tls != NULL){
			return -1;//calling thread has lsa
		}
		if(tls_tid_pairs[i].tid == tid && tls_tid_pairs[i].tls != NULL){
			found_target = 1;
		}
	}
	if(!found_target){
		return -1;//target does not have lsa
	}

	return 0;
}

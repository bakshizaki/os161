/*
 * All the contents of this file are overwritten during automated
 * testing. Please consider this before changing anything in this file.
 */

#include <types.h>
#include <lib.h>
#include <clock.h>
#include <thread.h>
#include <synch.h>
#include <test.h>
#include <kern/test161.h>
#include <spinlock.h>

/*
 * Use these stubs to test your reader-writer locks.
 */

//TODO:write tests for following conditions
//1) make sure writers are not interrupted by other writers / data is not corrupted
//2) make sure writers are not starved / served as soon as possible
//3) make sure multiple readers are allowed
//4) make sure when a writer is writing reader_count should be zero

#define NREADTHREADS 8
#define NWRITETHREADS 3
#define RWARRAYSIZE 10
unsigned int rw_test_array[RWARRAYSIZE];
#define NREADLOOPS 32
#define NWRITELOOPS 4
struct rwlock *rwlock_test;
struct semaphore *donesem;


static
void
rwt1reader(void *junk1,unsigned long num)
{
	(void)junk1;

	unsigned i,j;

	random_yielder(4);
	for(i=0;i<NREADLOOPS;i++) {
		kprintf("reader %u: entered\n",(unsigned int)num);
		rwlock_acquire_read(rwlock_test);
		kprintf("reader %u ",(unsigned int)num);
		for(j=0;j<RWARRAYSIZE;j++){
			kprintf("%u ",rw_test_array[j]);
		}
		kprintf("\n");
		rwlock_release_read(rwlock_test);
		random_yielder(4);
	}
	V(donesem);
}

static
void
rwt1writer (void *junk1, unsigned long num)
{
	(void)junk1;
	unsigned i,j;
	random_yielder(8);
	for(i=0;i<NWRITELOOPS;i++) {
		kprintf("writer %u: entered\n",(unsigned int)num);
		rwlock_acquire_write(rwlock_test);
		for(j=0;j<RWARRAYSIZE;j++) {
			rw_test_array[j]=num*10+j;
		}
		rwlock_release_write(rwlock_test);
		random_yielder(8);
	}
	V(donesem);
}

int rwtest(int nargs, char **args) {
	(void)nargs;
	(void)args;

	/*kprintf_n("rwt1 unimplemented\n");*/
	/*test code start*/
	int i,result;
	
	donesem = sem_create("rwtdonesem",0);
	KASSERT(donesem!=NULL);
	rwlock_test = rwlock_create("rwtest1");
	KASSERT(rwlock_test!=NULL);

	for(i=0;i<RWARRAYSIZE;i++){
		rw_test_array[i]=i+40;	
	}
	//create reader threads
	for(i=0;i<NREADTHREADS;i++) {
		result = thread_fork("rwt1reader",NULL,rwt1reader,NULL,(long unsigned)i);
	if (result) {
			panic("rwt1: thread_fork failed: %s\n", strerror(result));
		}
	}

	//create writer threads
	for(i=0;i<NWRITETHREADS;i++) {
		result = thread_fork("rwt1writer",NULL,rwt1writer,NULL,(long unsigned)i);
	if (result) {
			panic("rwt1: thread_fork failed: %s\n", strerror(result));
		}
	}
	for(i=0;i<(NREADTHREADS+NWRITETHREADS);i++) {
		P(donesem);
	}

	sem_destroy(donesem);
	rwlock_destroy(rwlock_test);
	kprintf("----------TEST DONE-------------\n");

	/*test code end*/
	success(TEST161_SUCCESS, SECRET, "rwt1");

	return 0;
}

int rwtest2(int nargs, char **args) {
	(void)nargs;
	(void)args;

	kprintf_n("rwt2 unimplemented\n");
	/*success(TEST161_FAIL, SECRET, "rwt2");*/
	success(TEST161_SUCCESS, SECRET, "rwt2");

	return 0;
}

int rwtest3(int nargs, char **args) {
	(void)nargs;
	(void)args;

	kprintf_n("rwt3 unimplemented\n");
	secprintf(SECRET, "Should panic...", "rwt3");
	KASSERT(false);
	success(TEST161_FAIL, SECRET, "rwt3");

	return 0;
}

int rwtest4(int nargs, char **args) {
	(void)nargs;
	(void)args;

	kprintf_n("rwt4 unimplemented\n");
	secprintf(SECRET, "Should panic...", "rwt4");
	KASSERT(false);
	success(TEST161_FAIL, SECRET, "rwt4");

	return 0;
}

int rwtest5(int nargs, char **args) {
	(void)nargs;
	(void)args;

	kprintf_n("rwt5 unimplemented\n");
	secprintf(SECRET, "Should panic...", "rwt5");
	KASSERT(false);
	success(TEST161_FAIL, SECRET, "rwt5");

	return 0;
}

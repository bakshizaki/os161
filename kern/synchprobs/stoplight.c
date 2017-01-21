/*
 * Copyright (c) 2001, 2002, 2009
 *	The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Driver code is in kern/tests/synchprobs.c We will replace that file. This
 * file is yours to modify as you see fit.
 *
 * You should implement your solution to the stoplight problem below. The
 * quadrant and direction mappings for reference: (although the problem is, of
 * course, stable under rotation)
 *
 *   |0 |
 * -     --
 *    01  1
 * 3  32
 * --    --
 *   | 2|
 *
 * As way to think about it, assuming cars drive on the right: a car entering
 * the intersection from direction X will enter intersection quadrant X first.
 * The semantics of the problem are that once a car enters any quadrant it has
 * to be somewhere in the intersection until it call leaveIntersection(),
 * which it should call while in the final quadrant.
 *
 * As an example, let's say a car approaches the intersection and needs to
 * pass through quadrants 0, 3 and 2. Once you call inQuadrant(0), the car is
 * considered in quadrant 0 until you call inQuadrant(3). After you call
 * inQuadrant(2), the car is considered in quadrant 2 until you call
 * leaveIntersection().
 *
 * You will probably want to write some helper functions to assist with the
 * mappings. Modular arithmetic can help, e.g. a car passing straight through
 * the intersection entering from direction X will leave to direction (X + 2)
 * % 4 and pass through quadrants X and (X + 3) % 4.  Boo-yah.
 *
 * Your solutions below should call the inQuadrant() and leaveIntersection()
 * functions in synchprobs.c to record their progress.
 */

#include <types.h>
#include <lib.h>
#include <thread.h>
#include <test.h>
#include <synch.h>

/*
 * Called by the driver during initialization.
 */

/* Answers to code reading questions:
 * 1.Assume that Buffalonians are not Buffalonians and obey the law: whoever 
 * arrives at the intersection first proceeds first. Using the language of 
 * synchronization primitives describe the way this intersection is controlled.
 * In what ways is this method suboptimal?
 * Ans: Here the first can arriving the intersection is acquiring lock for 3
 * quadrant (e.g. 1 acquiring lock for 1,0,3). This is suboptimal since
 * if there are less than 4 cars arriving at the same time each can acquire
 * lock for their upcoming desired quadrant and complete their journey without
 * crashing.
 *
 * 2. Now, assume that the Buffalonians are Buffalonians and do not follow the
 * convention described above. In what one instance can this four-­‐‑way-­‐‑stop
 * intersection produce a deadlock? It is helpful to think of this in terms of
 * the model we are using instead of trying to visualize an actual intersection.
 * Ans: It works well if everyone is allowed to acquire their desired lock when
 * they arrive at the intersection except the case that 4 cars arrive at the
 * intersection in which case it becomes a deadlock.
 *
 */

#define NUM_QUADRANTS 4
//lock for each quadrant
struct lock *lock_0;
struct lock *lock_1;
struct lock *lock_2;
struct lock *lock_3;

//condition variable and its lock, since the above locks are shared, they should
//be protected
struct cv *cv_avoid_deadlock;
struct lock *lk_avoid_deadlock;

struct lock * get_target_lock(uint32_t target_quadrant);
void get_other_locks(struct lock *other_locks[], uint32_t target_quadrant);
void safely_acquire_lock(uint32_t target_quadrant);
void simply_acquire_lock(uint32_t target_quadrant);
void simply_release_lock(uint32_t target_quadrant);

void
stoplight_init() {
	lock_0=lock_create("lock_0");
	lock_1=lock_create("lock_1");
	lock_2=lock_create("lock_2");
	lock_3=lock_create("lock_3");
	lk_avoid_deadlock=lock_create("lk_avoid_deadlock");
	cv_avoid_deadlock=cv_create("cv_avoid_deadlock");
	return;
}

/*
 * Called by the driver during teardown.
 */

void stoplight_cleanup() {
	lock_destroy(lock_0);
	lock_destroy(lock_1);
	lock_destroy(lock_2);
	lock_destroy(lock_3);
	lock_destroy(lk_avoid_deadlock);
	cv_destroy(cv_avoid_deadlock);
	return;
}

struct lock *
get_target_lock(uint32_t target_quadrant)
{
	switch(target_quadrant)
	{
		case 0:
			return lock_0;
			break;
		case 1:
			return lock_1;
			break;
		case 2:
			return lock_2;
			break;
		case 3:
			return lock_3;
			break;
		default:
			kprintf("ERROR: Undefined quadrant\n");
			break;
	};
	KASSERT(false);
	//asdfj
	return NULL;
}

void
get_other_locks(struct lock *other_locks[], uint32_t target_quadrant)
{
	int i,j=0;
	for(i=0;i<NUM_QUADRANTS;i++)
	{
		if(i!=(int)target_quadrant) {
			other_locks[j++]=get_target_lock(i);
		}	
	}

}


void
safely_acquire_lock(uint32_t target_quadrant)
{
	struct lock *target_lock=get_target_lock(target_quadrant);
	struct lock *other_locks[NUM_QUADRANTS];
	get_other_locks(other_locks,target_quadrant);
	lock_acquire(lk_avoid_deadlock);
	// wait till target lock is free and one other lock is free
	// this avoids deadlock of 4 cars
	while(is_lock_acquired(target_lock) || 
			(is_lock_acquired(other_locks[0]) && 
			 is_lock_acquired(other_locks[1]) && is_lock_acquired(other_locks[2]))) {
		cv_wait(cv_avoid_deadlock,lk_avoid_deadlock);

	}
	lock_acquire(target_lock);
	lock_release(lk_avoid_deadlock);
}

void
simply_acquire_lock(uint32_t target_quadrant)
{
	struct lock *target_lock=get_target_lock(target_quadrant);
	lock_acquire(lk_avoid_deadlock);
	while(is_lock_acquired(target_lock)) {
		cv_wait(cv_avoid_deadlock,lk_avoid_deadlock);
	}
	lock_acquire(target_lock);
	lock_release(lk_avoid_deadlock);
}

void
simply_release_lock(uint32_t target_quadrant)
{
	struct lock *target_lock=get_target_lock(target_quadrant);
	lock_acquire(lk_avoid_deadlock);
	lock_release(target_lock);
	cv_broadcast(cv_avoid_deadlock,lk_avoid_deadlock);
	lock_release(lk_avoid_deadlock);
}

void
turnright(uint32_t direction, uint32_t index)
{
	(void)direction;
	(void)index;

	uint32_t target_quadrant=direction;
	safely_acquire_lock(target_quadrant);
	inQuadrant(target_quadrant,index);
	leaveIntersection(index);
	simply_release_lock(target_quadrant);

	return;
}
void
gostraight(uint32_t direction, uint32_t index)
{
	(void)direction;
	(void)index;

	uint32_t target_quadrant=direction;
	safely_acquire_lock(target_quadrant);
	inQuadrant(target_quadrant,index);
	uint32_t new_target_quadrant=(target_quadrant + NUM_QUADRANTS - 1) % NUM_QUADRANTS;
	simply_acquire_lock(new_target_quadrant);
	inQuadrant(new_target_quadrant,index);
	simply_release_lock(target_quadrant);
	target_quadrant=new_target_quadrant;
	leaveIntersection(index);
	simply_release_lock(target_quadrant);
	
	return;
}
void
turnleft(uint32_t direction, uint32_t index)
{
	(void)direction;
	(void)index; 

	uint32_t target_quadrant=direction;
	safely_acquire_lock(target_quadrant);
	inQuadrant(target_quadrant,index);
	uint32_t new_target_quadrant=(direction + NUM_QUADRANTS - 1) % NUM_QUADRANTS;
	simply_acquire_lock(new_target_quadrant);
	inQuadrant(new_target_quadrant,index);
	simply_release_lock(target_quadrant);
	target_quadrant=new_target_quadrant;
	new_target_quadrant=(target_quadrant + NUM_QUADRANTS - 1) % NUM_QUADRANTS;
	simply_acquire_lock(new_target_quadrant);
	inQuadrant(new_target_quadrant,index);
	simply_release_lock(target_quadrant);
	target_quadrant=new_target_quadrant;
	leaveIntersection(index);
	simply_release_lock(target_quadrant);


	return;
}

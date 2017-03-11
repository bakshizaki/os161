/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
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
 * rmtest.c
 *
 * 	Tests file system synchronization by deleting an open file and
 * 	then attempting to read it.
 *
 * This should run correctly when the file system assignment is complete.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <err.h>
#include <errno.h>

#include <test161/test161.h>

// 23 Mar 2012 : GWA : BUFFER_COUNT must be even.


int
main(int argc, char **argv)
{

	// 23 Mar 2012 : GWA : Assume argument passing is *not* supported.

	(void) argc;
	(void) argv;
	/*const char *test_arg[10] = {"/testbin/argtest", "foo", "os161", "bar" };*/
	/*test_arg[4] = NULL;*/
	printf("Hello World3\n");
	/*execv("test_prog", (char **)test_arg);*/
	/*int pid;*/
	/*int pid2;*/
	/*pid = fork();*/
	/*if(pid == -1)*/
	/*{*/
		/*printf("fork failed");*/
		/*exit(1);*/
	/*} else if(pid == 0) {*/
		/*printf("Hello from child\n");*/
		/*[>while(1);<]*/
		/*_exit(0);*/
	/*} else {*/
		/*int status;*/
		/*pid2 = waitpid(pid, &status, 0);*/
		/*printf("Hello from parent\n");*/
		/*printf("Child pid:%d\n", pid);*/
		/*printf("Wait pid:%d\n",pid2);*/
		/*[>while(1);<]*/
	/*}*/
	return 0;
}

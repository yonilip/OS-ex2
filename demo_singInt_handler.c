/*
 * SIGINT signal handler demo program.
 * Hebrew University OS course.
 * Questions: os@cs.huji.ac.il
 */

#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#define SECOND 1000000


sigset_t sigSet;


void catch_int(int sigNum) {
	// Install catch_int as the signal handler for SIGINT.
	printf(" Don't do that!\n");
	fflush(stdout);
 }


int main(void)
{
	// Install catch_int as the signal handler for SIGINT.
	struct sigaction sa;
	sa.sa_handler = &catch_int;
	if (sigaction(SIGINT, &sa,NULL) < 0) {
		printf("sigaction error.");
	}

	sigaddset(&sigSet, SIGINT);
	sigprocmask(SIG_SETMASK, &sigSet, NULL);

	usleep(SECOND);
	usleep(SECOND);
	usleep(SECOND);
	usleep(SECOND);
	usleep(SECOND);
	usleep(SECOND);
	usleep(SECOND);
	usleep(SECOND);
	usleep(SECOND);

	sigemptyset(&sigSet);
	sigprocmask(SIG_UNBLOCK, &sigSet, NULL);

/*	for(;;) {
	  pause(); // wait until receiving a signal
  }*/
  return 0;
}


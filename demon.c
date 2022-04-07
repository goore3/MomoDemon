#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>

volatile int killSignal = 0;

static void init_demon()
{
	pid_t pid;

	pid = fork();

	if (pid < 0)
		exit(EXIT_FAILURE);
	if (pid > 0)
		exit(EXIT_SUCCESS);
	if (setsid() < 0)
		exit(EXIT_FAILURE);

	signal(SIGCHLD, SIG_IGN);
	signal(SIGHUP, SIG_IGN);

	pid = fork();

	if (pid < 0)
		exit(EXIT_FAILURE);
	if (pid > 0)
		exit(EXIT_SUCCESS);

	umask(0);

	chdir("/");
	
	int x;
	for (x = sysconf(_SC_OPEN_MAX); x>=0; x--)
	{
		close(x);
	}
	openlog ("DEMON", LOG_PID, LOG_DAEMON);
}

static void kill_dem_demon(int signum){
	killSignal = 1;
}	

static void init_signals(void)
{
	signal(SIGINT, kill_dem_demon);
	signal(SIGTERM, kill_dem_demon);
}

int main()
{
	init_demon();
	init_signals();	

	while(killSignal==0)
	{
		syslog (LOG_NOTICE, "DEMON ODPALONY");
		sleep(20);
	}

	syslog(LOG_NOTICE, "DEMON UŚMIERCONY");
	closelog();

	return EXIT_SUCCESS;
}

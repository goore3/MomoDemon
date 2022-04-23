#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>
#include <string.h>
#include <regex.h>
#include "copy.c"

volatile int killSignal = 0;
volatile int sleepTime = 300;
volatile int isRecursive = 0;
volatile int mmapMinSize = 10000;
volatile int started = 0;

static void init_demon()
{
	pid_t pid;
	printf("1\n");
	pid = fork();
	printf("2\n");
	if (pid < 0)
		exit(EXIT_FAILURE);
	if (pid > 0)
		exit(EXIT_SUCCESS);
	if (setsid() < 0)
		exit(EXIT_FAILURE);
	printf("3\n");
	signal(SIGCHLD, SIG_IGN);
	signal(SIGHUP, SIG_IGN);
	printf("4\n");
	pid = fork();
	printf("5\n");
	if (pid < 0)
		exit(EXIT_FAILURE);
	if (pid > 0)
		exit(EXIT_SUCCESS);
	printf("6\n");
	umask(0);

	chdir("/");
	printf("7\n");
	int x;
	for (x = sysconf(_SC_OPEN_MAX); x>=0; x--)
	{
		close(x);
	}
	openlog ("DEMON", LOG_PID, LOG_DAEMON);
	printf("8\n");
}

static void kill_dem_demon(int signum){
	killSignal = 1;
}	

//This function will be replaced with the sync function


static void init_signals(void)
{
	//signal(SIGUSR1, empty);
	signal(SIGUSR2, kill_dem_demon);
}

static int verifyArguments(int argc, char* argv[]){
	int opt;
	if(argc < 3) {
		printf("Otrzymano %d argument. Minimalna iloœæ parametrów to 2.\n", argc - 1);
		return -1;
	} else {
		if(access(argv[1],F_OK) != 0 || access(argv[2], F_OK) != 0) {
			printf("Jedna ze œcie¿ek nie istnieje lub nie móg³a byæ otwarta!\n");
			return -1;
		}
	}
	regex_t regex;
	int flag = regcomp(&regex, "^[0-9]*$", REG_EXTENDED);
	if(flag){
		printf("Regex nie mog³ byæ skompilowany");
		return -1;
	} 
	while((opt = getopt(argc, argv, "Rs:m:")) != -1){
		switch(opt){
			case 'R':
				isRecursive = 1;
				break;
			case 's':
				if(!regexec(&regex, optarg, 0, NULL, 0)){
					sleepTime = atoi(optarg);
				} else {
					printf("Zamiast liczby otrzymano %s.\n", optarg);
					return -1;
				}
				break;
			case 'm':
				if(!regexec(&regex, optarg, 0, NULL, 0)){
					mmapMinSize = atoi(optarg);
				} else {
					printf("Zamiast liczby otrzymano %s.\n", optarg);
					return -1;
				}
				break;
			case ':':
				printf("Ta opcja wymaga wartoœci!\n");
				break;
			case '?':
				printf("Wykryto nieokreœlony argument %c.\n", optopt);
				break;
		}
	}
	return 0;
}

//demon(pathSource,pathDestination,(*)sleepTime,(*)mmapMinFileSize)
int main(int argc, char* argv[]){
	if(started == 0){
		int flag = verifyArguments(argc, argv);
		if(flag!=0){
			printf("Komenda siê nie wykona³a. Sk³adnia komendy to demon [pathSource] [pathDestination] *[-R] *[-s sleepTime] *[-m mmapMinFileSize]\n");
			exit(EXIT_FAILURE);
		}
		printf("Wykona³ siê demon ");
		if(isRecursive){
			printf("w trybie rekursywnym. ");
		} else {
			printf(".");
		}
		printf("Bêdzie siê wykonywa³ co %d sekund ", sleepTime);
		printf("i minimalna wielkoœæ pliku aby wykorzystaæ funkcjê mmap wynosi %d.\n", mmapMinSize);
	}
	started = 1;
	init_demon();
	init_signals();
	search(argv[1], argv[2]);
	while(killSignal==0)
	{
		syslog (LOG_NOTICE, "DEMON ODPALONY");
		sleep(sleepTime);
		search(argv[1], argv[2]);
	}

	syslog(LOG_NOTICE, "Demon uœmiercony.");
	closelog();
	return EXIT_SUCCESS;
}

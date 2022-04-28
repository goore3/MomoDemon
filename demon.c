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
volatile long int mmapMinSize = 1000000;
volatile int wakeUp = 0;
char sourcePath[100];
char destinationPath[100];

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

	chdir("./");

	int x;
	for (x = sysconf(_SC_OPEN_MAX); x >= 0; x--)
	{
		close(x);
	}

	openlog("DEMON", LOG_PID, LOG_DAEMON);
}

// TODO: Make it better
static void killDemon()
{
	exit(EXIT_SUCCESS);
	syslog(LOG_NOTICE, "Demon usmiercony.");
	closelog();
}

//This function will be replaced with the sync function
static void wakeUpDemon()
{
	wakeUp = 1;
	syslog(LOG_NOTICE, "Demon został manualnie wybudzony i wykonuje synchronizacje.");
}

static void init_signals(void)
{
	signal(SIGUSR1, wakeUpDemon);
	signal(SIGUSR2, killDemon);
}

static int verifyArguments(int argc, char *argv[])
{
	struct stat sbSrc, sbDest;
	stat(argv[1], &sbSrc);
	stat(argv[2], &sbDest);
	int opt;
	if (argc < 3)
	{
		printf("Otrzymano %d argument. Minimalna ilosc parametrow to 2.\n", argc - 1);
		return -1;
	}
	else
	{
		if (!(S_ISDIR(sbSrc.st_mode) && S_ISDIR(sbDest.st_mode)))
		{
			printf("Jedna ze sciezek nie istnieje lub nie mogla byc otwarta!\n");
			return -1;
		}
	}

	strcpy(sourcePath, argv[1]);
	strcpy(destinationPath, argv[2]);
	regex_t regex;
	if (regcomp(&regex, "^[0-9]*$", REG_EXTENDED))
	{
		printf("Regex nie mogl byc skompilowany");
		return -1;
	}

	while ((opt = getopt(argc, argv, "Rs:m:")) != -1)
	{
		switch (opt)
		{
			case 'R':
				isRecursive = 1;
				break;
			case 's':
				if (!regexec(&regex, optarg, 0, NULL, 0))
				{
					sleepTime = atoi(optarg);
				}
				else
				{
					printf("Zamiast liczby otrzymano %s.\n", optarg);
					return -1;
				}

				break;
			case 'm':
				if (!regexec(&regex, optarg, 0, NULL, 0))
				{
					mmapMinSize = atol(optarg);
				}
				else
				{
					printf("Zamiast liczby otrzymano %s.\n", optarg);
					return -1;
				}

				break;
			case ':':
				printf("Ta opcja wymaga wartosci!\n");
				break;
			case '?':
				printf("Wykryto nieokreslony argument %c.\n", optopt);
				break;
		}
	}

	return 0;
}

//demon(pathSource,pathDestination,(*)sleepTime,(*)mmapMinFileSize)
int main(int argc, char *argv[])
{
	int errorCode = verifyArguments(argc, argv);
	if (errorCode != 0)
	{
		printf("Komenda sie nie wykonala. Skladnia komendy to demon[pathSource][pathDestination] *[-R] *[-s sleepTime] *[-m mmapMinFileSize]\n");
		exit(EXIT_FAILURE);
	}

	printf("Demon wykona sie ");
	if (isRecursive)
	{
		printf("w trybie rekursywnym. ");
	}
	else
	{
		printf(".");
	}

	printf("Bedzie sie wykonywac co %d sekund ", sleepTime);
	printf("i minimalna wielkosc pliku aby wykorzystac funkcje mmap wynosi %ld bajtow.\n", mmapMinSize);
	init_demon();
	syslog(LOG_NOTICE, "DEMON ODPALONY");
	init_signals();
	while (killSignal == 0)
	{
		sleep(sleepTime);
		synchronization(sourcePath, destinationPath, isRecursive, mmapMinSize);
		if(wakeUp == 1){
			syslog(LOG_NOTICE, "Manualna synchronizacja wykonana");
			wakeUp = 0;
		} else {
			syslog(LOG_NOTICE, "Synchronizacja wykonana");	
		}
	}

	syslog(LOG_NOTICE, "Demon usmiercony.");
	closelog();

	return EXIT_SUCCESS;
}

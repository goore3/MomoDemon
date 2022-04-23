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
char source[100];
char destination[100];

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
	for (x = sysconf(_SC_OPEN_MAX); x>=0; x--)
	{
		close(x);
	}
	openlog ("DEMON", LOG_PID, LOG_DAEMON);
}

static void kill_dem_demon(int signum){
	killSignal = 1;
}	

//This function will be replaced with the sync function
static void empty(){
	syslog(LOG_NOTICE, "Wykonaj funkcje");
}

static void init_signals(void)
{
	signal(SIGUSR1, empty);
	signal(SIGUSR2, kill_dem_demon);
}

static int verifyArguments(int argc, char* argv[]){
	int opt;
	if(argc < 3) {
		printf("Otrzymano %d argument. Minimalna ilo�� parametr�w to 2.\n", argc - 1);
		return -1;
	} else {
		if(access(argv[1],F_OK) != 0 || access(argv[2], F_OK) != 0) {
			printf("Jedna ze �cie�ek nie istnieje lub nie m�g�a by� otwarta!\n");
			return -1;
		}
	}
	strcpy(source, argv[1]);
	strcpy(destination, argv[2]);
	regex_t regex;
	int flag = regcomp(&regex, "^[0-9]*$", REG_EXTENDED);
	if(flag){
		printf("Regex nie mog� by� skompilowany");
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
				printf("Ta opcja wymaga warto�ci!\n");
				break;
			case '?':
				printf("Wykryto nieokre�lony argument %c.\n", optopt);
				break;
		}
	}
	return 0;
}

//demon(pathSource,pathDestination,(*)sleepTime,(*)mmapMinFileSize)
int main(int argc, char* argv[]){
	int flag = verifyArguments(argc, argv);
	if(flag!=0){
		printf("Komenda się nie wykonała. Składnia komendy to demon [pathSource] [pathDestination] *[-R] *[-s sleepTime] *[-m mmapMinFileSize]\n");
		exit(EXIT_FAILURE);
	}
	printf("Wykonał się demon ");
	if(isRecursive){
		printf("w trybie rekursywnym. ");
	} else {
		printf(".");
	}
	printf("Będzie się wykonywał co %d sekund ", sleepTime);
	printf("i minimalna wielkość pliku aby wykorzystać funkcję mmap wynosi %d.\n", mmapMinSize);
	init_demon();
	syslog (LOG_NOTICE, "DEMON ODPALONY");
	init_signals();
	while(killSignal==0)
	{
		sleep(sleepTime);
		search(source, destination);
		syslog (LOG_NOTICE, "Synchronizacja wykonana");
	}
	syslog(LOG_NOTICE, "Demon uśmiercony.");
	closelog();

	return EXIT_SUCCESS;
}

/*
	TODO:
	- Ditt program måste ha inbyggda felkontroller för returvärden från systemanropen. 
	  Om du upptäcker ett fel skall vettiga felmeddelanden skrivas ut till användaren.
	- Lämna inge processer öppna, varken FG, BG eller Zombies
	- Shellen ska INTE termineras av ctrl+c, bara den aktiva processen
	- FG: PID ska skrivas ut vid start och slut av en process utan fördröjning
	- BG: PID ska skrivas ut omedelbart vid start, vid slut senast efter nästa kommando matas in.
	- Två sätt att dektektera BG processer har terminerats (om två, definiera via macro SIGNALDETECTION)
		- Via polling (tex wait()/waitpid())
		- Genom signalhantering (ej krav för ID2200)

	Antaganden:
	- Användaren inte skriver kommandon som är längre än 70 tecken
	- Användaren inte ger kommandon med fler än 5 argument
	- Du behöver inte parsa kommandoraden för att kunna hantera |, >, < eller ;
	- Sökväg till mapp/fil får maximalt vara 255 tecken


	TODO:
	- Frigöra variabler bättre? Hur blir det med charpointers när de får nya värden?

*/
#include <stdio.h>		// printf, getline
#include <stdbool.h>	// bool
#include <stdlib.h>		// Malloc, free
#include <unistd.h> 	// definierar bland annat fork() och STDIN_FILENO
#include <string.h>		// String functions
#include <sys/types.h>	// definierar typen pid_t
#include <sys/wait.h>
#include <errno.h>		// Errno behöver vi verkligen använda denna?

void printError(char* command) {
	fprintf (stderr, "%s failed: %s\n", command, strerror(errno));
}


bool checkIfBG(char *input) {
	int i;
	int length = sizeof(input);
	for( i=0; i<length; i++) {
		if(input[i] == '&') {
			input[i] = '\0';
			return true;
		}
	}
	return false;
}

int main(int argc, char const *argv[], char *envp[]) {
	/* Temporary variables */
	int i;						// Counting
	bool running = true;		// Should we continue running the shell?
	bool bg = false;
	pid_t child_pid;
	char path[255];		
	char *input;				// Stores the input from the user
	char *command;
	char *parameters[6];
	char *temp;					// Stores temporary strings
	int bytesRead;				// How many bytes were read from the user
	int inBuffer = 70;			// Bytes to allocate the users input (getline allocates more if needed)

	strcpy(path, getenv("PWD")); // PUTENV Pekar om env variabel till sträng, setenv kopierar sträng

	// Ignorera CTRL - C 
	if (signal(SIGINT, SIG_IGN) != SIG_IGN)
	    signal(SIGINT, SIG_IGN);


	while(running) {
		printf("%s: ", path);
		input = (char *)malloc(inBuffer+1);
		if((bytesRead = *fgets(input, inBuffer, stdin)) < 0)
			printf("ERROR ERROR ERROR!\n");

		command = strtok(input, " \n");
		if(command == NULL)
			command = "";
		if(strcmp(command, "exit") == 0) {
			printf("Exiting..\n");
			running = false;
		} else if(strcmp(command, "cd") == 0) {
			command = strtok(NULL, " \n");
			if(chdir(command) < 0) {
				strcpy(path, getenv("HOME"));
				setenv("PWD", path, 1);
				if(chdir(path) < 0)	
					printError("cd");
			} else {
				strcpy(path, getcwd(0,0));
				setenv("PWD", path, 1);
			}
		} else if(strcmp(command, "") == 0) {
		 /*
			 ____   ___    _   _  ___ _____ _   _ ___ _   _  ____ _ 
			|  _ \ / _ \  | \ | |/ _ \_   _| | | |_ _| \ | |/ ___| |
			| | | | | | | |  \| | | | || | | |_| || ||  \| | |  _| |
			| |_| | |_| | | |\  | |_| || | |  _  || || |\  | |_| |_|
			|____/ \___/  |_| \_|\___/ |_| |_| |_|___|_| \_|\____(_)

		 */
		}else {
			// Normal command!
			bg = checkIfBG(input);
			child_pid = fork();
			if(child_pid == 0) {
				// CHILD
				parameters[0] = command;
				for(i=1; i<6; i++) {
					temp = strtok(NULL, " \n");
					parameters[i] = temp;
					
				}
				parameters[i] = (char*)NULL;
				if(execvp(command, parameters) < 0) 
					printError("execvp");
				return -1;
			} else {
				
				// PARENT
				if( bg ) {
					// BG process
					printf("BG child created with PID: %i\n", child_pid);
				} else {
					// FG process!
					printf("FG Child created with PID: %i\n", child_pid);
					waitpid(child_pid, NULL, 0);
					printf("FG Child with PID: %i terminated\n", child_pid);
				}
			}

		

	}
	// Free allocated memory
	free(input);
	
	return 0;
}

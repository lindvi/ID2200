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
#include <sys/stat.h>	// För stat (används för att se om en sökväg är fil eller mapp i cd)
#include <libgen.h>		// dirname(), basename()

char *getParentDir(char *dir) {
	return dirname(dir);
}

int main(int argc, char const *argv[], char *envp[]) {
	/* Temporary variables */
	struct stat s;
	int i;						// Counting
	bool running = true;		// Should we continue running the shell?
	pid_t child_pid;
	char path[255];
	char newPath[255];			
	//char temp[255];
	char *input;				// Stores the input from the user
	char *command;
	char *parameters[6];
	char *temp;					// Stores temporary strings
	int bytesRead;				// How many bytes were read from the user
	int inBuffer = 70;			// Bytes to allocate the users input (getline allocates more if needed)

	strcpy(path, getenv("PWD")); // PUTENV Pekar om env variabel till sträng, setenv kopierar sträng

	while(running) {
		printf("%s: ", path);
		input = (char *)malloc(inBuffer+1);
		if((bytesRead = *fgets(input, inBuffer, stdin)) < 0)
			printf("ERROR ERROR ERROR!\n");

		command = strtok(input, " \n");

		if(strcmp(command, "exit") == 0) {
			printf("Exiting..\n");
			running = false;
		} else if(strcmp(command, "cd") == 0) {
			printf("in cd now..\n");
			if( (temp = strtok(NULL, " \n")) == NULL) {
				strcpy(path, "~");
				setenv("PWD", path, 1);	// Hämta HOME istället? funkar det om man sätter pwd till enbart ~?	
			} else { // Finns en "parameter" till cd
				if(strcmp(temp, "..") == 0) {
					strcpy(path, dirname(path));
					setenv("PWD", path, 1);
				} else {
					strcpy(newPath, path);	// Copy current path
					strcat(newPath, temp);	// Add the folder to newPath 
					if(stat(newPath, &s)) {
						if(s.st_mode & S_IFDIR) {
							strcpy(path, newPath);
							setenv("PWD", path, 1);
						} else if(s.st_mode & S_IFREG) {
							printf("%s is not a directory.\n", temp);

						} else {
							printf("%s is neither a directory or a file. Probably doesn't even exist.\n", temp);
						}
					} else {
						printf("Error: stat() failed!\n");
					}
				}
			}
		} else {
			child_pid = fork();
			if(child_pid == 0) {
				parameters[0] = command;
				for(i=1; i<6; i++) {
					temp = strtok(NULL, " \n");
					parameters[i] = temp;
					
				}
				parameters[i] = (char*)NULL;
				execvp(command, parameters);
				return 0;
			} else {
				// PARENT
				//int status;
				//while(wait(&status) == child_pid);
			}
		}

	}
	// Free allocated memory
	free(input);
	
	return 0;
}

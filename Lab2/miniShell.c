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
	- Max 10 processer igång samtidigt


	TODO:
	- Frigöra variabler bättre? Hur blir det med charpointers när de får nya värden?

*/
#include <stdio.h>		/* printf, getline */
#include <stdbool.h>	/* bool */
#include <stdlib.h>		/* Malloc, free */
#include <unistd.h> 	/* definierar bland annat fork() och STDIN_FILENO */
#include <string.h>		/* String functions */
#include <sys/types.h>	/* definierar typen pid_t */
#include <sys/wait.h>	/* behövs för waitpid */
#include <errno.h>		/* Errno behöver vi verkligen använda denna? */

/* Skriver ut felmeddelande på stderr. Tolkar errno till sträng. */
void printError(char* command) {
	fprintf (stderr, "%s failed: %s\n", command, strerror(errno));
}

/*Kollar om & är inskrivet och därför bakgrundsprocess (TRUE / FALSE) 
Ser ut som den gör då fgets gör mellanslag till NUL dvs: '\0' */
bool checkIfBG(char *input) {
	int i;
	int length = sizeof(input);
	for( i=0; i<length; i++) {
		if(input[i] == '&') {
			input[i] = 32;		/* 32 = space */
			return true;
		}
	}
	return false;
}

/* Kollar om något barn har returnerat och skriver isf ut information om det */
void checkChilds() {
	int pid;			/* Spara pid i denna*/
	pid = waitpid(-1, NULL, WNOHANG); // WNOHANG, returnerar 0/-1 direkt om inget förändrat
	if(pid > 0)			/* Har vi fått en PID, då har den terminerats*/
		printf("BG Child %i terminated.\n", pid ); /* Skriv ut meddelande i konsollen */
}


int main(int argc, char const *argv[], char *envp[]) {
	/* Temporary variables */
	int i;						/* Counting */
	bool running = true;		/* Ska shellen fortsätta? */
	bool bg = false;			/* Är det en bakgrundsprocess som skall startas? */
	pid_t child_pid;			/* Spara PID information */
	char path[256];				/* Sökväg som programmet nu befinner sig i*/
	char *input;				/* Användarens input sparas i denna */
	char *command;				/* Tokenizerad input */
	char *parameters[6];		/* Sammanställning av alla parametrar till execvp */
	char *temp;					/* Spara temporära strängar */
	int bytesRead;				/* Hur många bytes var lästa från input */
	int inBuffer = 70;			/* Hur många bytes som skall allokeras till användarens input (getline allokerar mer om det behövs) */

	strcpy(path, getenv("PWD")); /* Hämtar environment variabel PWD och sparar i path */

	// Ignorera CTRL - C 
	if (signal(SIGINT, SIG_IGN) != SIG_IGN)
	    signal(SIGINT, SIG_IGN);

	while(running) {
		printf("%s: ", path);	/* Skriv ut mappen vi är */
		
		input = (char *)malloc(inBuffer+1);	/* Allokera minne till input */
		if((bytesRead = *fgets(input, inBuffer, stdin)) < 0)	/* Hämta user input */
			printf("Read Error from stdin!\n");	/*Läste inga bytes från stdin */
		checkChilds();		/* Kolla om någon child har stängts av */
		command = strtok(input, " \n");	/* hämta första ordet i input (bryter på space och NEWLINE) */
		if(command == NULL) 	/* Om användaren bara tryckte enter */
			command = "";		/* måste man sätta command till en tom sträng, annars segmentation fault */
		if(strcmp(command, "exit") == 0) {	/* Användaren vill avsluta */
			printf("Exiting..\n");
			running = false;				/* Bryt loopen */
		} else if(strcmp(command, "cd") == 0) {	/* Användaren vill byta mapp */
			command = strtok(NULL, " \n");		/* Hämta nästa ord för att se vart vi ska */
			if(chdir(command) < 0) {			/* Mappen finns inte */
				strcpy(path, getenv("HOME"));	/* Hämta HOME variabeln och spara i path */
				setenv("PWD", path, 1);			/* Sätt PWD till den nya path:en */
				if(chdir(path) < 0)				/* Går inte det heller, skriv ut felmeddelande till användaren */
					printError("cd");
			} else {							/* Det gick bra att byta mapp */
				strcpy(path, getcwd(0,0));		/* Hämta vilken mapp vi är i och spara i path */
				setenv("PWD", path, 1);			/* Sätt PWD till path, detta för att program som initieras av execvp ska veta vilken mapp vi är i. */
			}
		} else if(strcmp(command, "") == 0) {   /* För att hindra uppstart av execvp utan namn på programm */
		 /*

			 ____   ___    _   _  ___ _____ _   _ ___ _   _  ____ _ 
			|  _ \ / _ \  | \ | |/ _ \_   _| | | |_ _| \ | |/ ___| |
			| | | | | | | |  \| | | | || | | |_| || ||  \| | |  _| |
			| |_| | |_| | | |\  | |_| || | |  _  || || |\  | |_| |_|
			|____/ \___/  |_| \_|\___/ |_| |_| |_|___|_| \_|\____(_)

		 */
		} else { /* Inget inbyggt kommando! */
			bg = checkIfBG(input);		/* Kolla om det ska köras som en bakgrundsprocess */
			child_pid = fork();			/* FORK IT! */
			if(child_pid == 0) {
				/* CHILD */
				parameters[0] = command;	/* Namnet på som första parametern */
				for(i=1; i<6; i++) {		/* Resten efter det */
					temp = strtok(NULL, " \n");
					parameters[i] = temp;
					
				}
				parameters[i] = (char*)NULL;	/* Nulla sista parametern, annars blir man KIB:ad */
				if(execvp(command, parameters) < 0) /* Byt ut koden mot koden som skall köras */
					printError("execvp");
				return -1;	/* Kommer vi hit har något gått galet, returnera -1 */
			} else {
				/* PARENT */ 
				if( bg ) {
					/* BG PROCESS */
					printf("BG child created with PID: %i\n", child_pid);
				} else {
					/* FG PROCESS */
					printf("FG Child created with PID: %i\n", child_pid);
					waitpid(child_pid, NULL, 0);	/* Vänta på att childen avslutas */
					printf("FG Child with PID: %i terminated\n", child_pid);
				}
			}
		}
		/* Free allocated memory */
		free(input);
	}
	return 0; /* Avslutades korrekt, returnera 0 */
}

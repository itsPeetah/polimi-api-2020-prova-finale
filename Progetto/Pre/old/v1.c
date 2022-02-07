/*
	Prova finale del corso di Algoritmi e Strutture Dati
	A.A. 2020-21
	Pietro Guglielmo Moroni, 10625198
	pietroguglielmo.moroni@mail.polimi.it
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXCHARS 1024

#define TRUE 1
#define FALSE 0

#define FULLSTOP "."
#define EMPTYLINE ".\n"

#define CMD_CHANGE 'c'
#define CMD_DELETE 'd'
#define CMD_UNDO 'u'
#define CMD_REDO 'r'
#define CMD_PRINT 'p'
#define CMD_QUIT 'q'

#define COMMAND_CODES "cdurpq"

#define CMD_INSERT 'i'

/* Types ----------------------------------------------------- */


typedef struct line{
	struct line* next;
	struct line* prev;
	int length;
	int index;
}line_t;

typedef struct {
	line_t* head;
	int lineCount;
}Text;

/* Globals --------------------------------------------------- */

// Reading input
char* inputBuffer = NULL;
size_t inputBufferLength = 0;
ssize_t inputLength;

// Program state
int isDone = FALSE;
int editing = FALSE;



/* Methods and functions ------------------------------------- */

char* GetCommandCode(char* command);
void ParseCommand(char* commandCode);

// NEEDED FUNCTIONS 
/*
	1) Delete (needed text structure)
	2) Print (needed text structure)
	3) Parse command -> needed for easier u/r handling
	4) Get Complementary Command -> for u/r

*/

/* Main ------------------------------------------------------ */

int main(int argc, char** argv){

	// Main cycle
	while(isDone != TRUE){

		// Read input
		inputBuffer = NULL;
		inputBufferLength = 0;
		inputLength = getline(&inputBuffer, &inputBufferLength, stdin);

		printf("-2: %c\n", inputBuffer[inputLength-2]);
		printf("-1: %c\n", inputBuffer[inputLength-1]);
		printf("-0: %c\n", inputBuffer[inputLength]);


		if(!editing) ParseCommand(GetCommandCode(inputBuffer));
		else {
			// Read till full stop

			// Appky at full stop

		}




		free(inputBuffer);
	} 

	
}

/* Method and function bodies -------------------------------- */

char* GetCommandCode(char* buffer){
	// returns the first matching character to one of the command codes
	return strpbrk(buffer, COMMAND_CODES);
}


void ParseCommand(char* commandCode){

	// Trimming the command code from the buffer
	int n1, n2;

	// printf("%c\n", *commandCode);
	switch(*commandCode){
		case CMD_CHANGE:

			// Parse the command margins
			sscanf(inputBuffer,"%d,%dc", &n1, &n2);
			// Start listening
			editing = TRUE;

			// Debug
			printf("Changing from: %d to: %d\n", n1, n2);


			break;
		case CMD_DELETE:

			// Parse command margins
			sscanf(inputBuffer,"%d,%dd", &n1, &n2);

			// Compute and store complementary command
			// Delete lines (full command in buffer)
			
			// Debug
			printf("Deleting from: %d to: %d\n", n1, n2);

			break;
		case CMD_PRINT:

			// Parse command margins
			sscanf(inputBuffer,"%d,%dp", &n1, &n2);

			// Print lines (full command in buffer)


			// Debug
			printf("Printing from: %d to: %d\n", n1, n2);

			break;
		case CMD_UNDO:

			// Parse number of undos
			sscanf(inputBuffer,"%du", &n1);

			// Execute n commands
			// Switch history state

			// Debug
			printf("Undoing %d edits\n", n1);

			break;
		case CMD_REDO:

			// Parse number of redos
			sscanf(inputBuffer,"%dr", &n1);
			
			// Execute n commands
			// Switch history state

			// Debug
			printf("Redoing %d edits\n", n1);

			break;
		case CMD_QUIT:

			// Quit the program
			isDone = TRUE;

			return;
	}

}









































/* ABYSS */

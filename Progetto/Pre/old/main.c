/*
	Prova finale del corso di Algoritmi e Strutture Dati
	A.A. 2020-21
	Pietro Guglielmo Moroni, 10625198
	pietroguglielmo.moroni@mail.polimi.it
*/

/*
	TO DO:

	1) Delete (needed text structure)
	2) Print (needed text structure)
	3) Parse command -> needed for easier u/r handling
	4) Get Complementary Command -> for u/r

*/

/*
	DONE:

	1) Command input and parsing
	2) quit command

	•) Change command
	•) Print command
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXCHARS 1024

#define TRUE 1
#define FALSE 0

#define FULLSTOP "."
#define EMPTYLINE ".\n"

#define CC_CHANGE 'c'
#define CC_DELETE 'd'
#define CC_UNDO 'u'
#define CC_REDO 'r'
#define CC_PRINT 'p'
#define CC_QUIT 'q'

#define CC_INSERT 'i'

/* Shortcuts ------------------------------------------------- */

#define COMMAND_CODE inputBuffer[inputLength-2]
#define LINE_DELTA to-from+1

/* Debug ----------------------------------------------------- */

#define DB_INSERT_COMMAND_PROMPT printf("Insert a command:\n");
#define DB_C_FB printf("Changing from: %d to: %d\n", from, to);
#define DB_C_PROGRESS printf("from: %d, to: %d, currently: %d\n",from, to, from+progress );
#define DB_P_FB printf("Printing from: %d to: %d\n", from, to);

/* Types ----------------------------------------------------- */

enum pstate {ListeningForCommand = 0, ChangingText = 1, Terminated = 2};
enum bool {False = 0, True = 1};

typedef struct line{
	struct line* next;
	struct line* prev;
	int length;
	int index;
	char* content;
}line_t;

typedef struct {
	line_t* first;
	line_t* last;
	int lineCount;
}text_t;

typedef struct{
	char code;
	char* line;
}edit_t;

typedef struct edit_elem{
	struct edit_elem* next;
	struct edit_elem* prev;
	edit_t* user;
	edit_t* complementary;
	int index;
}edit_history_t;

/* Globals --------------------------------------------------- */

// Reading input
char* inputBuffer = NULL;
size_t inputBufferLength = 0;
ssize_t inputLength;

// Program state
enum pstate programState = ListeningForCommand;

text_t text;
edit_history_t history;
line_t* cursor;


/* Methods and functions ------------------------------------- */

void GetInput();
void ParseCommand(char commandCode);
line_t* GetLine(int index);

void OnChange(int from, int to);
void OnPrint(int from, int to);

/* Main ------------------------------------------------------ */

int main(int argc, char** argv){

	// Init
	text.first = NULL;
	text.last = NULL;
	text.lineCount = 0;

	history.index = 0;
	history.user = NULL;
	history.complementary = NULL;
	history.next = NULL;
	history.prev = NULL;

	cursor = NULL;

	// Main cycle
	while(programState != Terminated){

		// Read and parse input
		DB_INSERT_COMMAND_PROMPT
		GetInput();
		ParseCommand(COMMAND_CODE);

	} 

	
}

/* Method and function bodies -------------------------------- */

void GetInput(){

	if(inputBuffer != NULL) free(inputBuffer);

	inputBuffer = NULL;
	inputBufferLength = 0;
	inputLength = getline(&inputBuffer, &inputBufferLength, stdin);
}

void ParseCommand(char commandCode){

	int n1, n2;

	switch(commandCode){
		case CC_CHANGE:
			// Parse the command margins and execute
			sscanf(inputBuffer,"%d,%dc", &n1, &n2);
			OnChange(n1, n2);
			break;
		case CC_DELETE:

			// Parse command margins
			sscanf(inputBuffer,"%d,%dd", &n1, &n2);

			// Compute and store complementary command
			// Delete lines (full command in buffer)
			
			// Debug
			printf("Deleting from: %d to: %d\n", n1, n2);

			break;
		case CC_PRINT:
			// Parse command margins and execute
			sscanf(inputBuffer,"%d,%dp", &n1, &n2);
			OnPrint(n1, n2);
			break;
		case CC_UNDO:

			// Parse number of undos
			sscanf(inputBuffer,"%du", &n1);

			// Execute n commands
			// Switch history state

			// Debug
			printf("Undoing %d edits\n", n1);

			break;
		case CC_REDO:

			// Parse number of redos
			sscanf(inputBuffer,"%dr", &n1);
			
			// Execute n commands
			// Switch history state

			// Debug
			printf("Redoing %d edits\n", n1);

			break;
		case CC_QUIT:

			// Quit the program
			programState = Terminated;

			return;
	}

}

line_t* GetLine(int index){

	printf("Index: %d, cursor: %d, first: %d, last: %d\n", index, cursor->index,text.first->index,text.last->index);

	if(index > text.lineCount || text.lineCount < 1) return NULL;
	else if(index == cursor->index) return cursor;
	else if(index == 1) return text.first;
	else if(index == text.lineCount) return text.last;
	else{

		// Start search from one of the terminals of the text structure
		line_t* searchFrom = index > (int)(text.lineCount * 0.5) ? text.first : text.last;
		// Start search from the last line edidted
		 if (abs(searchFrom->index - index) >= abs(cursor->index - index)) searchFrom = cursor;
		 
		// Search
		while(searchFrom != NULL && searchFrom->index != index){
			if(searchFrom->index-index < 0) searchFrom = searchFrom->next;
			else searchFrom = searchFrom->prev;
		}

		return searchFrom;
	}
}

void OnChange(int from, int to){
	// Start listening
	programState = ChangingText;

	int progress = 0;
	do{
		GetInput();
		
		if(text.first == NULL) // Was text empty?
		{
			text.first = malloc(sizeof(line_t));
			text.last = text.first;

			cursor = text.first;
			cursor->next = NULL;
			cursor->prev = NULL;
			cursor->length = inputLength;

			text.lineCount = 1;
			cursor->index = text.lineCount;
			
		} else if (text.lineCount < from + progress) // Adding new line?
		{
			cursor = malloc(sizeof(line_t));
			cursor->next = NULL;
			text.last->next = cursor;
			cursor->prev = text.last;
			cursor->length = inputLength;
			text.last = cursor;

			text.lineCount++;
			cursor->index = text.lineCount;
		} else 
		{
			// DO STUFF
		}

		// cursor->content = malloc(inputLength);
		// strcpy(cursor->content, inputBuffer);

		cursor->content = inputBuffer;

		progress++;
		

	} while (progress < LINE_DELTA);

	// Flush the fullstop
	char c;
	scanf("%c%*c", &c);
}

void OnPrint(int from, int to){

	DB_P_FB
	int i = from;
	line_t* lineToPrint = GetLine(from);

	while(i <= to){
		// if(lineToPrint != NULL) printf("LINE INDEX: %d\n", lineToPrint->index);
		if(lineToPrint != NULL && i <= text.lineCount){
			 printf("%s", lineToPrint->content);
			 lineToPrint = lineToPrint->next;
		} else printf(".\n");

		i++;
	}

}









































/* ABYSS */

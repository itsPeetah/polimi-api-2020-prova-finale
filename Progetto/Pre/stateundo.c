#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_CAPACITY 8
#define CMD_CHANGE 'c'
#define CMD_PRINT 'p'
#define CMD_DELETE 'd'
#define CMD_QUIT 'q'
#define CMD_UNDO 'u'
#define CMD_REDO 'r'

#define INTERNAL_INSERT 'i'

#define LAST_INDEX text.length-1

/* Data types ---------------------------------------------------- */

enum program_status{Running = 0, Terminated = -1};

typedef struct{
    char* content;
    int len;
}line_t;

typedef struct {
    line_t** text;  // Base of the line array
    int length;     // Actually used space
    int capacity;   // Available space in the array
} text_t;

typedef struct restore_line{
    struct restore_line* next;
    char* content;
}restore_line_t;

typedef struct edit{
    char code;
    int previousTextLength;
    int startRestoringFrom;
    int restoreAmount;
    restore_line_t* headOfLines;
}edit_t;

typedef struct history_entry{
    int timeStamp;
    edit_t* edit;
    struct history_entry* next;
    struct history_entry* prev;
}history_t;

typedef struct state{
    int time;
    struct state* next;
    struct state* prev;
    edit_t* undo;
    edit_t* redo;
}state_t;

/* Input events -------------------------------------------------- */

void OnChange(int from, int to);
void OnPrint(int from, int to);
void OnDelete(int from, int to);
void OnUndo(int amount);
void OnRedo(int amount);

/* Internal commands --------------------------------------------- */

void DoChange(edit_t* edit);
void DoInsert(edit_t* edit);

/* Utilities ----------------------------------------------------- */

void CheckTextCapacity(int neededIndex);
int NewLineRequired(int index);
void SetLineContent(int index, char** content, size_t size);
void ResizeText(int sizeDelta);
void SetTextSize(int newSize);
void ShiftLines(int from, int offset); // Reverse = 0 -> delete, reverse = 1: insert

history_t* NewTimelineEntry(history_t* appendTo);
edit_t* NewEntryEdit(history_t* entry);


state_t* InitializeState(state_t* previous);
edit_t* InitializeStateEdit(edit_t* e, char code, int prevTexLen, int strRestFrm, int rstAmount);

/* Globals ------------------------------------------------------- */

// Program state
enum program_status status = Running;
text_t text;
int currentTime, maxTime;

history_t* timeline;
state_t* state;

// Input handling
char* in_buf;
size_t in_buf_size;
ssize_t in_len;

/* Main ---------------------------------------------------------- */

int main(int argc, char** argv){

    // Initialize text
    text.capacity = INITIAL_CAPACITY;
    text.length = 0;
    text.text = (line_t**)malloc(text.capacity * sizeof(line_t*));

    // Initialize timeline
    currentTime = 0;
    maxTime = 0;

    // Initialize State
    state = InitializeState(NULL);

    // Initialize input
    in_buf = NULL;
    in_buf_size = 0;

    while(status > Terminated)
    {
        // Get input
        in_buf = NULL;
        in_len = getline(&in_buf, &in_buf_size,stdin);

        // Parse command
        int n1, n2;
        char command = in_buf[in_len - 2];
        switch (command)
        {
        case CMD_CHANGE:
            sscanf(in_buf, "%d,%dc", &n1, &n2);
            currentTime++;
            OnChange(n1, n2);    
            break;
        case CMD_PRINT:
            sscanf(in_buf, "%d,%dp", &n1, &n2);
            OnPrint(n1, n2);
            break;
        case CMD_DELETE:
            sscanf(in_buf, "%d,%dd", &n1, &n2);
            currentTime++;
            OnDelete(n1, n2);
            break;
        case CMD_UNDO:
            sscanf(in_buf, "%du", &n1);
            OnUndo(n1);
            break;
        case CMD_REDO:
            sscanf(in_buf, "%dr", &n1);
            OnRedo(n1);
            break;
        case CMD_QUIT:
            status = Terminated;
            break;
        }

        if(currentTime < 0) currentTime = 0;
        if (currentTime > maxTime) maxTime = currentTime;

    }

    // Deallocate input line buffer
    free(in_buf);
    in_buf = NULL;

}

/* User commands ------------------------------------------------- */

void OnPrint(int from, int to){

    for(int i = from; i <= to; i++){
        if(i-1 >= text.length || i == 0) // If the line does not exist print '.'
            printf(".\n");
        else 
            printf("%s", text.text[i-1]->content); // Print the content of the line
    }
}

void OnChange(int from, int to){
    CheckTextCapacity(to);

    state_t* nextState = InitializeState(state);
    nextState->undo = InitializeStateEdit(nextState->undo, CMD_CHANGE, text.length, from, -1);
    


    // init line cursors for storing overwritten lines into the undo
    restore_line_t* uLineHead = NULL;
    restore_line_t* uLineCursor = NULL;

    for(int i = from; i <= to; i++){

        // Get input
        in_buf = NULL;
        in_len = getline(&in_buf, &in_buf_size,stdin);
       
        // If the line was already existing and it's being replaced store it into the undo
        if(!NewLineRequired(i))
        {
            // Save lines that are gonna be replaced in the undo
            if(uLineHead == NULL){
                uLineHead = (restore_line_t*)malloc(sizeof(restore_line_t));
                uLineHead->next = NULL;
                uLineCursor = uLineHead;
            } else {
                uLineCursor->next = (restore_line_t*)malloc(sizeof(restore_line_t));
                uLineCursor->next->next = NULL;
                uLineCursor = uLineCursor->next;
            }
            uLineCursor->content = text.text[i-1]->content;
        }

        // Set text
        SetLineContent(i, &in_buf, in_buf_size);
    }

    nextState->undo->headOfLines = uLineHead;

    // Get rid of the period at the end of the lines to change
    char fullstop;
    scanf("%*c%c", &fullstop);

    state = nextState;
}

void OnDelete(int from, int to){
    
    int offset = to - from + 1; // Amount of deleted line (text is to be shifted by this amount)
    int cursor = from - 1;      // Where to start shifting the text from

    state_t* nextState = InitializeState(state);
    nextState->undo = InitializeStateEdit(nextState->undo, INTERNAL_INSERT, text.length, from, offset);


    // Store lines that will actually be deleted in the undo
    {
        restore_line_t* uLineHead = NULL;
        restore_line_t* uLineCursor = NULL;
        int counter = 0;

        while(counter < offset){

            if(from+counter > text.length) break;

            if(uLineHead == NULL){
                uLineHead = (restore_line_t*)malloc(sizeof(restore_line_t));
                uLineHead->next = NULL;
                uLineCursor = uLineHead;
            } else {
                uLineCursor->next = (restore_line_t*)malloc(sizeof(restore_line_t));
                uLineCursor->next->next = NULL;
                uLineCursor = uLineCursor->next;
            }

            uLineCursor->content = text.text[from+counter-1]->content;

            counter++;
        }

        nextState->undo->headOfLines = uLineHead;
    }

    // Delete lines by shifting the text over by [offset] towards the head
    ShiftLines(cursor, offset);
    
    // Set new text length (old line count - deleted lines count) and clamp to N+
    ResizeText(-offset);

    state = nextState;
}

void OnUndo(int amount){

    state_t* timeMachine = state;

    while(timeMachine != NULL && timeMachine->undo != NULL && amount > 0){
        switch(timeMachine->undo->code){
            case CMD_CHANGE:
                DoChange(timeMachine->undo);
                break;
            case INTERNAL_INSERT:
                DoInsert(timeMachine->undo);
                break;
        }

        timeMachine = timeMachine->prev;
        amount--;
    }

    state = timeMachine;
}

void OnRedo(int amount){

   
}

/* Internal commands --------------------------------------------- */

void DoChange(edit_t* edit){

    SetTextSize(edit->previousTextLength);

    // Restore stored lines;
    restore_line_t* lineCursor = edit->headOfLines;
    int restoreLocation = edit->startRestoringFrom;
    while(lineCursor != NULL){
        SetLineContent(restoreLocation, &lineCursor->content, 128);
        lineCursor = lineCursor->next;
        restoreLocation++;
    }

}

void DoInsert(edit_t* edit){

    SetTextSize(edit->previousTextLength);

    // Pull lines towards tail
    int pullFrom;
    for(int i = 0; i < edit->restoreAmount; i++){

        // prevent pulling from before line 0
        pullFrom = LAST_INDEX - i - edit->restoreAmount;
        if(pullFrom < 0) pullFrom = 0;

        text.text[LAST_INDEX - i]->content = text.text[pullFrom]->content;
    }

    // return;

    // Restore stored lines
    restore_line_t* lineCursor = edit->headOfLines;
    int restoreLocation = edit->startRestoringFrom;
    while(lineCursor != NULL){
        SetLineContent(restoreLocation, &lineCursor->content, 128);
        lineCursor = lineCursor->next;
        restoreLocation++;
    }

}





/* Utilities ----------------------------------------------------- */

void ResizeText(int sizeDelta){
    text.length += sizeDelta;
    if(text.length <= 0) text.length = 0;
}

void SetTextSize(int newSize){
    text.length = newSize;
}

void CheckTextCapacity(int neededIndex){
    // Increase text capacity to fit the required index

    while(neededIndex > text.capacity)
    {
        // Duplicate size and re-allocate
        text.capacity += INITIAL_CAPACITY;
        text.text = (line_t**)realloc(text.text, text.capacity * sizeof(line_t*));
    }
}

int NewLineRequired(int index){
    if(index > text.length) // adding new line
    {
        // Free se non null per evitare sprechi TODO
        text.text[index-1] = (line_t*)malloc(sizeof(line_t));
        text.length++;
        return 1;
    }

    else return 0;
}

void SetLineContent(int index, char** content, size_t size){ 
    // Set text
    text.text[index-1]->content = (*content);
    text.text[index-1]->len = size;
}

void ShiftLines(int from, int offset){
    
    int cursor = from;
    while(cursor < text.length){   
        // Shift the text until there are lines ahead
        if(cursor + offset < text.length)   
            text.text[cursor]->content = text.text[cursor + offset]->content;
        
        cursor++;
    }
}

history_t* NewTimelineEntry(history_t* appendTo){

    history_t* new;

    // TODO CHECK FOR OVERRIDE


    if(appendTo == NULL){
        new = (history_t*)malloc(sizeof(history_t));
        new->edit = NULL;
        new->timeStamp = currentTime;
        new->prev = NULL;
        new->next = NULL;
        return new;
    }else{
        new = (history_t*)malloc(sizeof(history_t));
        new->edit = NULL;
        new->timeStamp = currentTime;
        new->prev = appendTo;
        new->next = NULL;
        appendTo->next = new;
        return new;
    }

}

edit_t* NewEntryEdit(history_t* entry){

    if(entry->edit == NULL){
        edit_t* edit = (edit_t*)malloc(sizeof(edit_t));
        edit->headOfLines = NULL;
        return edit;
    }else{
        return entry->edit;
    }

}








state_t* InitializeState(state_t* previous){

    state_t* s = (state_t*)malloc(sizeof(state_t));
    s->time = currentTime;
    s->next = NULL;
    s->prev = NULL;
    s->undo = NULL;
    s->redo = NULL;

    if(previous != NULL){
        previous->next = s;
        s->prev = previous;
        s->time = previous->time+1;
    }

    return s;
}

edit_t* InitializeStateEdit(edit_t* e, char code, int prevTexLen, int strRestFrm, int rstAmount){

    if(e == NULL){
        e = (edit_t*)malloc(sizeof(edit_t));
    }
    e->code = code;
    e->previousTextLength = prevTexLen;
    e->startRestoringFrom = strRestFrm;
    e->restoreAmount = rstAmount;
    if(e->headOfLines != NULL){
        // TODO clear lines
        // TODO free head
    }
    e->headOfLines = NULL;
    return e;
}
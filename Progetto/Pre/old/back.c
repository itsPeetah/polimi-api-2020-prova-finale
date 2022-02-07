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

// Custom commands
#define CUS_INSERT 'i'
#define CUS_SETLENGTH 'l'
#define CUS_SKIP 's'

/* Data types ---------------------------------------------------- */

enum program_state{Running = 0, Terminated = -1, Editing = 1, Restoring = 2};

typedef struct{
    char* content;
    int len;
}line_t;

typedef struct {
    line_t** text;  // Base of the line array
    int length;     // Actually used space
    int capacity;   // Available space in the array
} text_t;

typedef struct line_elem{
    struct line_elem* next;
    line_t* line;
}line_list_t;

typedef struct edit{
    line_list_t* lines;
    char code;
    int val;
    int val2;
} edit_t;

typedef struct edit_elem{
    struct edit_elem* prev;
    struct edit_elem* next;
    edit_t* undo;
    edit_t* redo;
    int time;
} record_t;


/* Function declarations ----------------------------------------- */

void OnChange(int from, int to);
void OnPrint(int from, int to);
void OnDelete(int from, int to);

void DoSetLength(int delta);
void DoChange(edit_t* edit);
void DoInsert(edit_t* edit);
void DoDelete(int from, int to);

void PerformUndos(int amount);
void PerformRedos(int amount);

void InitRecord(record_t* record);
void ChainRecord(record_t* base, record_t** record2append);

void InitEdit(edit_t* edit, char code, int val);


/* Globals ------------------------------------------------------- */

// Program state
enum program_state state = Running;
text_t text;
int currentTime, maxTime;
record_t* history = NULL;

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

    // Initialize history
   

    // Initialize input
    in_buf = NULL;
    in_buf_size = 0;

    while(state > Terminated)
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
            OnChange(n1, n2);
            // Increment the time after the command has been executed
            currentTime++;
            break;
        case CMD_PRINT:
            sscanf(in_buf, "%d,%dp", &n1, &n2);
            OnPrint(n1, n2);
            break;
        case CMD_DELETE:
            sscanf(in_buf, "%d,%dd", &n1, &n2);
            OnDelete(n1, n2);
            // Increment the time after the command has been executed
            currentTime++;
            break;
        case CMD_UNDO:
            sscanf(in_buf, "%du", &n1);
            PerformUndos(n1);
            break;
        case CMD_REDO:
            sscanf(in_buf, "%dr", &n1);
            PerformRedos(n1);
            break;
        case CMD_QUIT:
            state = Terminated;
            break;
        }

        if(currentTime < 0) currentTime = 0;
        if (currentTime > maxTime) maxTime = currentTime;

        // Advance in history
        ChainRecord(history, &history->next);
        history = history->next;
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

    // init history
    if(history == NULL){
        history = (record_t*)malloc(sizeof(record_t));
        InitRecord(history);
    }
    history->time = currentTime;

    edit_t* rEdit = history->redo;
    edit_t* uEdit = history->undo;

    InitEdit(rEdit, CMD_CHANGE, from - 1);
    InitEdit(uEdit, CMD_CHANGE, from - 1);

    while(to > text.capacity) // trying to add more lines than the capacity of the array
    {
        // Duplicate size and re-allocate
        text.capacity += INITIAL_CAPACITY;
        text.text = (line_t**)realloc(text.text, text.capacity * sizeof(line_t*));
    }

    int newLines = 0;
    line_list_t* undoLine = uEdit->lines;
    line_list_t* redoLine = rEdit->lines;

    for(int i = from; i <= to; i++){

        // Get input
        in_buf = NULL;
        in_len = getline(&in_buf, &in_buf_size,stdin);
       
        if(i > text.length) // adding new line
        {
            newLines++;

            // Free se non null per evitare sprechi TODO
            text.text[i-1] = (line_t*)malloc(sizeof(line_t));
            text.text[i-1]->len = 0;
            text.length++;
        }

        // if the line was not empty -> save it in undos
        if(text.text[i-1]->len > 0){
            // init memory
            if(undoLine == NULL){
                undoLine = (line_list_t*)malloc(sizeof(line_list_t));
                undoLine->next = NULL;
                undoLine->line = (line_t*)malloc(sizeof(line_t));
            }
            // Save line length and content
            undoLine->line->len = text.text[i-1]->len;
            undoLine->line->content = text.text[i-1]->content;
            // Prepare for saving a new one if necessary
            undoLine = undoLine->next;
        }

        // Save redo
        // Init memory
        if(redoLine == NULL){
            redoLine = (line_list_t*)malloc(sizeof(line_list_t));
            redoLine->next = NULL;
            redoLine->line = (line_t*)malloc(sizeof(line_t));
        }
        // Save line length and content
        redoLine->line->len = in_buf_size;
        redoLine->line->content = in_buf;
        // Prepare for saving a new one if necessary
        undoLine = undoLine->next;

        // Set text
        text.text[i-1]->content = in_buf;
        text.text[i-1]->len = in_buf_size;
    }

    uEdit->val2 = -newLines;

    // Get rid of the period at the end of the lines to change
    char fullstop;
    scanf("%*c%c", &fullstop);
}

void OnDelete(int from, int to){

    if(history == NULL){
        history = (record_t*)malloc(sizeof(record_t));
        InitRecord(history);
    }
    edit_t* rEdit = history->redo;
    edit_t* uEdit = history->undo;

    InitEdit(rEdit, CMD_DELETE, from - 1);
    rEdit->val2 = to - 1;
    InitEdit(uEdit, CUS_INSERT, from - 1);
    uEdit->val2 = 0;
    
    int offset = to - from + 1; // Amount of deleted line (text is to be shifted by this amount)
    int cursor = from - 1;      // Where to start shifting the text from

    line_list_t* undoLine = uEdit->lines;

    while(cursor < text.length){
        if(cursor + offset < text.length)   // Shift the text until there are lines to do so
        {
            if(undoLine == NULL){
                undoLine = (line_list_t*)malloc(sizeof(line_list_t));
                undoLine->next = NULL;
                undoLine->line = (line_t*)malloc(sizeof(line_t));
            }
            // save line length and content
            undoLine->line->len = text.text[cursor]->len;
            undoLine->line->content = text.text[cursor]->content;
            // Prepare for saving a new one if necessary
            undoLine = undoLine->next;

            text.text[cursor]->content = text.text[cursor + offset]->content;
        }

        // Insert will have to increase size
        uEdit->val2++;

        // Go to the next line
        cursor++;
    }
    
    // Set new text length (old line count - deleted lines count) and clamp to N+
    text.length-=offset;
    if(text.length <= 0) text.length = 0;
}


void PerformUndos(int amount){

    while(history->prev != NULL && amount > 0){

        if(history->undo != NULL){
            switch (history->undo->code)
            {
            case CMD_CHANGE:
                DoChange(history->undo);
                if(history->undo->val2 != 0) DoSetLength(history->undo->val2);
                break;
            }
        }

        amount--;
        history = history->prev;
    }

}
void PerformRedos(int amount){
    while(history->next != NULL && amount > 0){

        if(history->redo != NULL){
            switch (history->redo->code)
            {
            case CUS_INSERT:
                DoInsert(history->redo);
                break;
            case CMD_DELETE:
                DoDelete(history->redo->val, history->redo->val2);
                break;
            }
        }
        
        amount--;
        history = history->next;
    }
}

/* Internal commands --------------------------------------------- */

void DoSetLength(int delta){
    text.length += delta;
}

void DoChange(edit_t* edit){

    line_list_t* cursor = edit->lines;
    int linesChanged = 0;
    while(cursor != NULL){
        text.text[edit->val + linesChanged]->content = cursor->line->content;
        linesChanged++;
    }

}

void DoInsert(edit_t* edit){

    int previousLength = text.length;
    DoSetLength(edit->val2);
    int cursor = 0;
    while(cursor < edit->val2){

        text.text[text.length - cursor - 1]->content = text.text[previousLength - cursor - 1]->content;
        cursor++;
    }
    
    cursor = 0;
    while(edit->lines != NULL){
        text.text[edit->val + cursor]->content = edit->lines->line->content;
        edit->lines = edit->lines->next;
        cursor++;
    }
}

void DoDelete(int from, int to){
    int offset = to - from + 1; // Amount of deleted line (text is to be shifted by this amount)
    int cursor = from - 1;      // Where to start shifting the text from

    while(cursor < text.length){
        if(cursor + offset < text.length)   // Shift the text until there are lines to do so
        {
            text.text[cursor]->content = text.text[cursor + offset]->content;
        }
        // Go to the next line
        cursor++;
    }
    // Set new text length (old line count - deleted lines count) and clamp to N+
    text.length-=offset;
    if(text.length <= 0) text.length = 0;
}

/* History utilities --------------------------------------------- */

void InitRecord(record_t* record){
    record->prev = NULL;
    record->undo = NULL;
    record->redo = NULL;
    record->next = NULL;
}

void ChainRecord(record_t* base, record_t** record2append){

    if((*record2append) == NULL)
    {
        (*record2append) = (record_t*)malloc(sizeof(record_t));
        InitRecord((*record2append));
    }
    base ->next =(*record2append);
    (*record2append)->prev = base;

}

void InitEdit(edit_t* edit, char code, int val){
    edit->lines = NULL;
    edit->code = code;
    edit->val = val;
}
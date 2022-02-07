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

#define INTERNAL_SETLEN 'l'
#define INTERNAL_INSERT 'i'
#define HYBRID_CHANGELEN 's' // For both setlen and change

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
    char* content;
    int len;
}list_line_t;

typedef struct edit{
    int timestamp;
    struct edit* next;
    struct edit* prev;
    char code;          // CMD_CHANGE or INTERNAL_SETLEN
    int val, sec;       // CHANGE: from, SETLEN: delta
    list_line_t* lines; // CHANGE: go on till NULL; SETLEN: NULL
}edit_t;

/*
    edit_t manual:
    - timestamp: time when the edit has happened. Useful for navigation
    - next, prev: next and previous items in history
    - code: one of either c, d, l, i or s
    - val: c: from, d: from, l: not initialized, i: from, s: from
    - sec: c: not initialized, d: to, l: delta, i: how many, s: delta
    - lines: c,i,s: go till null; d,l: null 
*/


/* Input events -------------------------------------------------- */

void Recap();

void OnChange(int from, int to);
void OnPrint(int from, int to);
void OnDelete(int from, int to);
void OnUndo(int amount);
void OnRedo(int amount);

/* Internal commands --------------------------------------------- */


void DoChange(edit_t* edit);
void DoChangeAndResize(edit_t* edit);
void DoInsert(edit_t* edit);
void DoDelete(edit_t* edit);


/* Utilities ----------------------------------------------------- */

void CheckTextCapacity(int neededIndex);
int NewLineRequired(int index);
void SetLineContent(int index, char** content, size_t size);
void ResizeText(int sizeDelta);
void SetTextSize(int newSize);
void ShiftLines(int from, int offset, int reverse); // Reverse = 0 -> delete, reverse = 1: insert
edit_t* InitEdit(edit_t* appendTo);
void InitEditLines(edit_t* edit);
list_line_t* GetLinesForInsert(int from, int to);

/* Globals ------------------------------------------------------- */

// Program state
enum program_state state = Running;
text_t text;
int currentTime, maxTime;

edit_t* undos;
edit_t* redos;

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
    currentTime = -1;
    maxTime = -1;

    // Initialize undos and redos
    undos = NULL;
    redos = NULL;

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
            state = Terminated;
            break;
        }

        if(currentTime < 0) currentTime = 0;
        if (currentTime > maxTime) maxTime = currentTime;

        Recap();
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

    // Undo
    undos = InitEdit(undos);
    if(undos->lines == NULL) InitEditLines(undos);
    undos->val = from;
    undos->code = HYBRID_CHANGELEN;

    // Redo
    redos = InitEdit(redos);
    if(redos->lines == NULL) InitEditLines(redos);
    redos->val = from;
    redos->code = CMD_CHANGE;

    list_line_t* uLine = undos->lines;
    list_line_t* rLine = redos->lines;

    undos->sec = text.length;

    for(int i = from; i <= to; i++){

        // Advance uLine and rLine if this is not the first iteration
        {
            if(uLine->len > 0 && uLine->next == NULL){
                uLine->next = (list_line_t*)malloc(sizeof(list_line_t));
                uLine->next->len = 0;
                uLine->next->next = NULL;
                uLine = uLine->next;
            }
            if(rLine->len > 0 && rLine->next == NULL){
                rLine->next = (list_line_t*)malloc(sizeof(list_line_t));
                rLine->next->len = 0;
                rLine->next->next = NULL;
                rLine = rLine->next;
            }
        }
        
        // Get input
        in_buf = NULL;
        in_len = getline(&in_buf, &in_buf_size,stdin);
       
        if(NewLineRequired(i))
        {
            // Do something?
        }

        // Store old line info in undo
        uLine->len = text.text[i-1]->len;
        uLine->content = text.text[i-1]->content;
        uLine->next = NULL;

        // Set text
        SetLineContent(i, &in_buf, in_buf_size);

        // Store new line info in redo
        rLine->len = text.text[i-1]->len;
        rLine->content = text.text[i-1]->content;
        rLine->next = NULL;
    }

    redos->sec = text.length;

    // Get rid of the period at the end of the lines to change
    char fullstop;
    scanf("%*c%c", &fullstop);
}

void OnDelete(int from, int to){
    
    // Undo
    undos = InitEdit(undos);
    undos->val = from;
    undos->sec = to - from + 1;
    undos->lines = GetLinesForInsert(from, to);
    undos->code = INTERNAL_INSERT;

    // Redo
    redos = InitEdit(redos);
    redos->val = from;
    redos->sec = to;
    redos->code = CMD_DELETE;
    
    int offset = to - from + 1; // Amount of deleted line (text is to be shifted by this amount)
    int cursor = from - 1;      // Where to start shifting the text from

    ShiftLines(cursor, offset, 0);
    
    // Set new text length (old line count - deleted lines count) and clamp to N+
    ResizeText(-offset);
}

void OnUndo(int amount){

    // int a = 0;

    while(undos != NULL && redos != NULL && currentTime>=0 && amount > 0){
        // printf("U%d\n", a++);

        switch(undos->code){
            case INTERNAL_INSERT:
                DoInsert(undos);
                break;
            case HYBRID_CHANGELEN:
                DoChangeAndResize(undos);
                break;
        }

        // Walk back syncronously
        if(undos->prev != NULL) undos = undos->prev;
        if(redos->prev != NULL) redos = redos->prev;

        currentTime--;
        amount--;
    }

}

void OnRedo(int amount){

    while(redos != NULL && undos != NULL && currentTime <= maxTime && amount--){
        switch(redos->code){
            case CMD_DELETE:
                DoDelete(undos);
                break;
            case CMD_CHANGE:
                DoChangeAndResize(undos);
                break;
        }

        // printf("R%d\n", a++);

        // Walk forward syncronously
        if(redos->next != NULL) redos = redos->next;
        if(undos->next != NULL) undos = undos->next;

        currentTime++;
        amount--;
    }
}

/* Internal commands --------------------------------------------- */

void DoChange(edit_t* edit){

}

void DoChangeAndResize(edit_t* edit){

    SetTextSize(edit->sec);
    list_line_t* line_cursor = undos->lines;
    int position = undos->val - 1;
    while(line_cursor != NULL && line_cursor->content != NULL && position<text.length){
        SetLineContent(position, &line_cursor->content, line_cursor->len);
    }
}

void DoDelete(edit_t* edit){

    int offset = edit->sec - edit->val + 1; // Amount of deleted line (text is to be shifted by this amount)
    int cursor = edit->val - 1;      // Where to start shifting the text from

    ShiftLines(cursor, offset, 0);
    
    // Set new text length (old line count - deleted lines count) and clamp to N+
    ResizeText(-offset);
}

void DoInsert(edit_t* edit){

    int offset = edit->sec - edit->val + 1;
    ResizeText(offset);

    ShiftLines(0, offset, 1);
    list_line_t* line_cursor = undos->lines;
    int position = edit->val;
    while(line_cursor != NULL && position<text.length){
        SetLineContent(position, &line_cursor->content, line_cursor->len);
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

void ShiftLines(int from, int offset, int reverse){
    
    int cursor;

    if(reverse == 0) // reverse == 0: delete (fall in place)
    {
        cursor = from;
        while(cursor < text.length){   
            // Shift the text until there are lines ahead
            if(cursor + offset < text.length)   
                text.text[cursor]->content = text.text[cursor + offset]->content;
            
            cursor++;
        }
    } 
    else // reverse == 1: insert (make space)
    {
        // I might need to check for space    
        cursor = text.length - 1;
        while(cursor > from)
            text.text[cursor]->content = text.text[cursor-1]->content;

        cursor--;
    }
}

edit_t* InitEdit(edit_t* appendTo){

    edit_t* edit = (edit_t*)malloc(sizeof(edit_t));

    edit->timestamp = currentTime;
    edit->val = 0;
    edit->sec = 0;
    edit->code = 0;
    edit->lines = NULL;
    edit->next = NULL;
    edit->prev = appendTo == NULL ? NULL : appendTo;
    if(appendTo != NULL) {
        
        // clear all the edits from next to null TODO
        // ALSO reset maxtime

        appendTo->next = edit;
    }

    return edit;
}

void InitEditLines(edit_t* edit){
    edit->lines = (list_line_t*)malloc(sizeof(list_line_t));
    edit->lines->len = 0;
    edit->lines->next = NULL;
}

list_line_t* GetLinesForInsert(int from, int to){
    list_line_t* head = (list_line_t*)malloc(sizeof(list_line_t));

    list_line_t* cursor = head;

    for(int i = from - 1; i < to; i++){
        if(i < text.length){
            cursor->len = text.text[i]->len;
            cursor->content = text.text[i]->content;
            cursor->next = (list_line_t*)malloc(sizeof(list_line_t));
            cursor = cursor->next;
        }
    }
    free(cursor);

    return head;
}









void Recap(){

    printf("\n-------------------\n");
    printf("Text lenght:%d\nCurrent time: %d\nMax time: %d\n", text.length, currentTime, maxTime);
    printf("---\nText:\n");
    for(int i = 0; i < text.length; i++){
        printf("%d) %s", i+1, text.text[i]->content);
    }


    edit_t* cursor;
    int count;
    list_line_t* line;

    printf("---\nUndos ahead:\n");
    cursor = undos->next;
    count = 0;
    
    while(cursor != NULL){
        
        printf("Undo #%d\n", ++count);
        printf("%c, %d, %d\nLines:\n", cursor->code, cursor->val, cursor->sec);

        line = cursor->lines;
        while(line != NULL){
            printf("%s", line->content);
            line = line->next;
        }


        cursor = cursor->next;
    }

    printf("---\nUndos behind:\n");
    cursor = undos;
    count = 0;
    
    while(cursor != NULL){
        
        printf("Undo #%d\n", --count);
        printf("%c, %d, %d\nLines:\n", cursor->code, cursor->val, cursor->sec);

        line = cursor->lines;
        while(line != NULL){
            printf("%s", line->content);
            line = line->next;
        }

        cursor = cursor->prev;
    }

    printf("---\nRedos ahead:\n");
    cursor = redos->next;
    count = 0;
    
    while(cursor != NULL){
        
        printf("Redo #%d\n", ++count);
        printf("%c, %d, %d\nLines:\n", cursor->code, cursor->val, cursor->sec);
        
        line = cursor->lines;
        while(line != NULL){
            printf("%s", line->content);
            line = line->next;
        }
        
        cursor = cursor->next;

    }

    printf("---\nRedos behind:\n");
    cursor = redos;
    count = 0;
    
    while(cursor != NULL){
        
        printf("Redo #%d\n", --count);
        printf("%c, %d, %d\nLines:\n", cursor->code, cursor->val, cursor->sec);
        
        line = cursor->lines;
        while(line != NULL){
            printf("%s", line->content);
            line = line->next;
        }
        
        cursor = cursor->prev;
    }

    printf("\n-------------------\n");
}
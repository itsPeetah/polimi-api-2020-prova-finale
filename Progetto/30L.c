#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define min(a,b) (((a)<(b))?(a):(b))
#define max(a,b) (((a)>(b))?(a):(b))

#define TEXT_BLOCK_SIZE 32
#define EDIT_BLOCK_SIZE 8

#define CHANGE 'c'
#define PRINT 'p'
#define DELETE 'd'
#define QUIT 'q'
#define UNDO 'u'
#define REDO 'r'
#define SKIP 0

/* ===================================================================== */
/* typedefs */

typedef struct edit{
    char code;
    int location, size, setlen, fill;
    char **lines;
}edit_t;

typedef struct state{
    edit_t *undo, *redo;
}state_t;

/* ===================================================================== */
/* Function declarations */

void AdjustCapacity(int required_lines);
void SetTextCapacity(int capacity);
void SetTextLength(int length);
void SetLine(int location, char **content);

void UpdateHistory();
void FreeStateContent(int index);
edit_t *GetStateEdit(char which);
void SetupEdit(edit_t *e, char code, int loc, int setl, int size);
void AddLineToEdit(edit_t *e, char **lineContent);

void OnQuit();
void OnPrint(int from, int to);
void OnChange(int from, int to);
void OnDelete(int from, int to);
void OnUndo(int steps);
void OnRedo(int steps);

void QueueUndos(int amount);
void QueueRedos(int amount);
void RestoreEdits();

void TryRestoreState();

/* ===================================================================== */
/* Globals */

char **text;    // Text container
int t_cap = 0;  // Text capacity
int t_len = 0;  // Text length

char *in_buf = NULL;    // Buffer for all things input
size_t in_size = 0;     // Size of the buffer
ssize_t in_len = 0;     // Length of the input line

char status = 0;    // Program status (aka what action is being performed)

state_t *history;       // Edit timeline
int h_cap = 0;          // Allocated blocks
int stateCount = 0;     // Max time
int currentState = 0;   // Current time

int actions_to_restore = 0; // Undo/Redo queue

char **rightMost = NULL;    // Most recent copy of the whole text before any undos are performed
int rm_len = 0;             // # of lines in the rightmost state
int rm_state = 0;           // what state is it?

/* ===================================================================== */
/* Main */

int main(int argc, char* argv[]){

    // Init text
    SetTextCapacity(TEXT_BLOCK_SIZE);

    // Init history (count = 1, current = 0)
    UpdateHistory();

    int arg1, arg2;
    while(status != QUIT){
        // Get input
        in_buf = NULL;
        in_len = getline(&in_buf, &in_size, stdin);
        
        // Execute input
        switch (status = in_buf[in_len - 2]){
            case PRINT:
                RestoreEdits();
                sscanf(in_buf, "%d,%dp", &arg1, &arg2);
                OnPrint(arg1, arg2);
                break;
            case CHANGE:
                RestoreEdits(); 
                sscanf(in_buf, "%d,%dc", &arg1, &arg2);
                OnChange(arg1, arg2);
                break;
            case DELETE:
                RestoreEdits();
                sscanf(in_buf, "%d,%dd", &arg1, &arg2);
                OnDelete(arg1, arg2);
                break;
            case UNDO:
                sscanf(in_buf, "%du", &arg1);
                QueueUndos(arg1);
                break;
            case REDO:
                sscanf(in_buf, "%dr", &arg1);
                QueueRedos(arg1);
                break;
        }
    }
    return 0;
}

/* ===================================================================== */
/* Text support */

void SetTextCapacity(int capacity){
    t_cap = capacity;
    text = (char**)realloc(text, t_cap * sizeof(char*));
}

void AdjustTextCapacity(int required_lines)
{
    int capacity = t_cap;

    if(required_lines < capacity) {
        // Decrease capacity to trim out any non required text space
        while(capacity - TEXT_BLOCK_SIZE >= required_lines) capacity -= TEXT_BLOCK_SIZE;
    }
    else {
        // Increase capacity to fit the required text size
        while(required_lines > capacity) capacity += TEXT_BLOCK_SIZE;
    }

    SetTextCapacity(capacity);
}

void SetTextLength(int length){
    t_len = length;
    AdjustTextCapacity(t_len);
}

void SetLine(int location, char** content){
    text[location-1] = (*content);
}

/* ===================================================================== */
/* History support */

void UpdateHistory(){

    // Making changes in the present
    if(stateCount == 0 || currentState == stateCount-1){
        // Increase count by one state and reallocate memory
        stateCount++;
    }
    // Making changes in the past
    else {
        // Deallocate old states
        for(int i = currentState + 1; i < stateCount; i++){
            FreeStateContent(i);
        }
        // Set new size and reallocate
        stateCount = currentState + 2;
    }

    int capacity = h_cap;
    while(stateCount >= capacity) capacity += EDIT_BLOCK_SIZE;
    while(stateCount < capacity - EDIT_BLOCK_SIZE) capacity -= EDIT_BLOCK_SIZE;

    // Resize history space
    if(capacity != h_cap) history = (state_t*)realloc(history, capacity * sizeof(state_t));

    // Initialize new state
    history[stateCount - 1].undo = NULL;
    history[stateCount - 1].redo = NULL;

    h_cap = capacity;
}

void FreeStateContent(int index){
    
    edit_t *e = history[index].undo;
    if(e != NULL){
        free(e);
        history[index].undo = NULL;
    }
    
    e = history[index].redo;
    if(e != NULL){
        free(e);
        history[index].redo = NULL;
    }
    
}

edit_t* GetStateEdit(char which){
    if(which == UNDO){
        // If empty, generate (could do realloc/calloc but is it necessary?)
        if(history[currentState + 1].undo == NULL) {
            history[currentState + 1].undo = (edit_t*)malloc(sizeof(edit_t));
            history[currentState + 1].undo->code = 0;
            history[currentState + 1].undo->location = 0;
            history[currentState + 1].undo->size = 0;
            history[currentState + 1].undo->setlen = 0;
            history[currentState + 1].undo->lines = NULL;
        }
        return history[currentState+1].undo;
    }
    // If empty, generate (could do realloc/calloc but is it necessary?)
    if(history[currentState].redo == NULL){
        history[currentState].redo = (edit_t*)malloc(sizeof(edit_t));
        history[currentState].redo->code = 0;
        history[currentState].redo->location = 0;
        history[currentState].redo->size = 0;
        history[currentState].redo->setlen = 0;
        history[currentState].redo->lines = NULL;
    }

    return history[currentState].redo;
}

void SetupEdit(edit_t *e, char code, int loc, int setl, int size){
    e->code = code;
    e->location = loc;
    e->setlen = setl;
    e->size = size;
    e->fill = 0;
    e->lines = NULL;
    e->lines = (char**)realloc(e->lines, size * sizeof(char*));
}

void AddLineToEdit(edit_t *e, char **lineContent){
    // Add line to edit and increase counter
    e->lines[e->fill] = (*lineContent);
    e->fill++;
}

/* ===================================================================== */
/* Command events */

void OnPrint(int from, int to){
    // Prind valid lines, replace invalid ones with '.'
    for(int i = from; i <= to; i++){
        if(i > 0 && i <= t_len) printf("%s", text[i-1]);
        else printf(".\n");
    }
}

void OnChange(int from, int to){
    
    int prevLen = t_len;
    // Expand/Trim text to fit
    SetTextLength(max(to, prevLen));

    UpdateHistory();  // count = current + 2, current+1 is the undo's state
   
    edit_t *undo = GetStateEdit(UNDO);
    SetupEdit(undo, CHANGE, from, prevLen, to - from + 1);
    
    edit_t *redo = GetStateEdit(REDO);
    SetupEdit(redo, CHANGE, from, t_len, to - from + 1);

    for(int i = from; i <= to; i++){
        // Get input
        in_buf = NULL;
        in_len = getline(&in_buf, &in_size, stdin);

        // If line is being overwritten
        if(i <= prevLen) {
            AddLineToEdit(undo, &text[i-1]);
        }

        SetLine(i, &in_buf);
        AddLineToEdit(redo, &in_buf);
    }

    currentState++;

    rm_state = 0;
}

void OnDelete(int from, int to){
    if(from > t_len || to < 1) {
        UpdateHistory();
    
        edit_t *undo = GetStateEdit(UNDO);
        SetupEdit(undo, SKIP, 0, 0, 0);
    
        edit_t *redo = GetStateEdit(REDO);
        SetupEdit(redo, SKIP, 0, 0, 0);

        currentState++;
        return;
    }

    int lastToRemove = min(to, t_len);
    int offset = lastToRemove - from + 1;

    UpdateHistory();
    
    edit_t *undo = GetStateEdit(UNDO);
    SetupEdit(undo, DELETE, from, t_len, offset);
    
    edit_t *redo = GetStateEdit(REDO);
    SetupEdit(redo, DELETE, from, t_len-offset, offset);

    int cursor = from - 1;
    while(cursor < t_len){

        if(cursor < lastToRemove){
            AddLineToEdit(undo, &text[cursor]);
        }

        if(cursor + offset < t_len){
            text[cursor] = text[cursor+offset];
            text[cursor + offset] = NULL;
        }
        cursor++;
    }

    SetTextLength(t_len-offset);
    currentState++;
    rm_state = 0;
}

void OnUndo(int steps){

    if(currentState == stateCount - 1){
        rightMost = (char**)realloc(rightMost, t_cap * sizeof(char*));
        rm_len = t_len;
        rm_state = currentState;
        for(int i = 0; i < rm_len; i++)
            rightMost[i] = text[i];
    }
    
    edit_t *undo;
    while(currentState > 0 && steps > 0){
        
        undo = history[currentState].undo;
        switch (undo->code){
        case CHANGE:
            /* Undo Change ------------------ */
            {
                SetTextLength(undo->setlen);
                for(int i = 0; i < undo->fill; i++){
                    SetLine(undo->location + i, &undo->lines[i]);
                }
            }
            break;
        case DELETE:
            /* Undo Delete (do insert) ------ */
            {
                SetTextLength(undo->setlen);
                int cursor = t_len;
                while(cursor >= undo->location){
                    // Inserting at the end
                    if(t_len - undo->fill == undo->location - 1){
                        SetLine(cursor, &undo->lines[cursor-undo->location]);
                    } 
                    // Inserting in the middle
                    else {
                        if(cursor-undo->fill > 0) text[cursor - 1] = text[cursor-undo->fill - 1];
                        if(cursor <= undo->location + undo->fill - 1) SetLine(cursor, &undo->lines[cursor-undo->location]);
                    }
                    cursor--;
                }
            }
            break;
        }
        steps--;
        currentState--;
    }
}

void OnRedo(int steps){

    edit_t *redo;
    while(currentState < stateCount - 1 && steps > 0){

        redo = history[currentState].redo;
        switch (redo->code){
        case CHANGE:
            /* Redo Change ------------------ */
            {
                SetTextLength(redo->setlen);
                for(int i = 0; i < redo->fill; i++){
                    SetLine(redo->location + i, &redo->lines[i]);
                }
            }
            break;
        case DELETE:
            /* Redo Delete ------------------ */
            {
                int cursor = redo->location - 1;
                while(cursor < t_len){

                    if(cursor + redo->size < t_len){
                        text[cursor] = text[cursor+redo->size];
                        text[cursor + redo->size] = NULL;
                    }
                    cursor++;
                }
                SetTextLength(redo->setlen);
            }
            break;
        }
        steps--;
        currentState++;
    }
}

void QueueUndos(int amount){
    actions_to_restore -= amount;
    if(currentState + actions_to_restore < 0) actions_to_restore = -currentState;
}

void QueueRedos(int amount){
    actions_to_restore += amount;
    if(actions_to_restore + currentState > stateCount - 1) actions_to_restore = stateCount - 1 - currentState;
}

void RestoreEdits(){

    if(actions_to_restore == 0) return;
    TryRestoreState();

    if(actions_to_restore > 0)
        OnRedo(actions_to_restore);
    else if(actions_to_restore < 0)
        OnUndo(-actions_to_restore);
    actions_to_restore = 0;
}

void TryRestoreState(){

    // Calculate target state
    int target = currentState+actions_to_restore;
    
    // Check if 0 or the rm state are closer to the target
    if (actions_to_restore < 0 && target < -actions_to_restore){
        actions_to_restore = target;
        SetTextLength(0);
        currentState = 0;
    }
    else if(actions_to_restore > 0 && rm_state > 0){
        
        if(abs(target-rm_state)<actions_to_restore)
        {
            SetTextLength(rm_len);
            currentState = rm_state;
            for(int i = 0; i < rm_len; i++)
                text[i] = rightMost[i];
            actions_to_restore = target-rm_state;
        }
    }
}


// Line 476 as a tribute to the Roman Empire
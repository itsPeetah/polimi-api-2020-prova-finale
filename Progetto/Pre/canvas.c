#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEXT_BLOCK_SIZE 8
#define STATE_BLOCK_SIZE 4

#define CHANGE 'c'
#define PRINT 'p'
#define DELETE 'd'
#define QUIT 'q'
#define UNDO 'u'
#define REDO 'r'
#define SKIP 's'
#define UNINITIALIZED 0

/* ===================================================================== */
/* typedefs */

typedef struct edit{
    char code;
    int location;
    int size;
    int setlen;
    char **lines;
}edit_t;

typedef struct state{
    edit_t *undo;
    edit_t *redo;
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

int MinInt(int a, int b);
int MaxInt(int a, int b);

void OnQuit();
void OnPrint(int from, int to);
void OnChange(int from, int to);
void OnDelete(int from, int to);
void OnUndo(int steps);
void OnRedo(int steps);

void DoChange(edit_t *e);

/* ===================================================================== */
/* Globals */

char **text;
int t_cap = 0;  // Text capacity
int t_len = 0;  // Text length

char *in_buf = NULL;
size_t in_size = 0;
ssize_t in_len = 0;

char status = 0;

state_t *history;
int s_cap = 0;      // Allocated blocks
int s_count = 0;    // Max time
int s_curr = 0;     // Current time

/* ===================================================================== */
/* Main */

int main(int argc, char* argv[]){

    // Init text
    SetTextCapacity(TEXT_BLOCK_SIZE);

    // Init history
    UpdateHistory();  // count = 1, current = 0;

    while(status != QUIT){

        in_buf = NULL;
        in_len = getline(&in_buf, &in_size, stdin);
        
        status = in_buf[in_len - 2];
        int n1, n2;

        switch (status)
        {
            case QUIT:
                OnQuit();
                break;
            case PRINT:
                sscanf(in_buf, "%d,%dp", &n1, &n2);
                OnPrint(n1, n2);
                break;
            case CHANGE:
                sscanf(in_buf, "%d,%dc", &n1, &n2);
                OnChange(n1, n2);
                break;
            case DELETE:
                sscanf(in_buf, "%d,%dd", &n1, &n2);
                OnDelete(n1, n2);
                break;
            case UNDO:
                sscanf(in_buf, "%du", &n1);
                OnUndo(n1);
                break;
            case REDO:
                sscanf(in_buf, "%dr", &n1);
                OnRedo(n1);
                break;
        }
    }

    return 0;
}

/* ===================================================================== */
/* Text support */

void SetTextCapacity(int capacity)
{
    t_cap = capacity;
    text = (char**)realloc(text, t_cap * sizeof(char*));
}

void AdjustTextCapacity(int required_lines)
{
    int capacity = t_cap;

    if(required_lines < capacity) 
    {
        // Decrease capacity to trim out any non required text space
        while(capacity - TEXT_BLOCK_SIZE >= required_lines)
        {
            capacity -= TEXT_BLOCK_SIZE;
        }
    }
    else
    {
        // Increase capacity to fit the required text size
        while(required_lines > capacity)
        {
            capacity += TEXT_BLOCK_SIZE;
        }
    }

    SetTextCapacity(capacity);
}

void SetTextLength(int length)
{
    t_len = length;
    AdjustTextCapacity(t_len);
}

void SetLine(int location, char** content)
{
    text[location-1] = (*content);
}

/* ===================================================================== */
/* History support */

void UpdateHistory()
{
    // First or latest state
    if(s_count == 0 || s_curr == s_count-1)
    {
        // Increase count by one state and reallocate memory
        s_count++;
    }
    // Changing the past/future 
    else 
    {
        // Deallocate old states
        for(int i = s_curr + 1; i < s_count; i++){
            FreeStateContent(i);
        }

        // Set new size and reallocate
        s_count = s_curr + 2;
    }

    // Resize history space
    history = (state_t*)realloc(history, s_count * sizeof(state_t));

    // Initialize new state
    history[s_count - 1].undo = NULL;
    history[s_count - 1].redo = NULL;
}

void FreeStateContent(int index){

    // FIX not space-optimized 
    // Dangling pointer to the lines

    if(history[index].undo != NULL)
    {
        free(history[index].undo);
        history[index].undo = NULL;
    }
    
    if(history[index].redo != NULL)
    {
        free(history[index].redo);
        history[index].redo = NULL;
    }
    
}

edit_t* GetStateEdit(char which)
{
    if(which == UNDO)
    {
        // If empty, generate
        if(history[s_curr + 1].undo == NULL)
        {
            history[s_curr + 1].undo = (edit_t*)malloc(sizeof(edit_t));
            history[s_curr + 1].undo->code = 0;
            history[s_curr + 1].undo->location = 0;
            history[s_curr + 1].undo->size = 0;
            history[s_curr + 1].undo->setlen = 0;
            history[s_curr + 1].undo->lines = NULL;
        }

        return history[s_curr+1].undo;
    }

    // If empty, generate
    if(history[s_curr].redo == NULL){
        history[s_curr].redo = (edit_t*)malloc(sizeof(edit_t));
        history[s_curr].redo->code = 0;
        history[s_curr].redo->location = 0;
        history[s_curr].redo->size = 0;
        history[s_curr].redo->setlen = 0;
        history[s_curr].redo->lines = NULL;
    }

    return history[s_curr].redo;
}

void SetupEdit(edit_t *e, char code, int loc, int setl, int size)
{
    e->code = code;
    e->location = loc;
    e->setlen = setl;
    e->size = size;
    e->lines = NULL;
}

void AddLineToEdit(edit_t *e, char **lineContent)
{
    e->size++;
    e->lines = (char**)realloc(e->lines, e->size * sizeof(char*));
    e->lines[e->size-1] = (*lineContent);
}

/* ===================================================================== */
/* Utilities */

int MinInt(int a, int b)
{
    if(a <= b) return a;
    return b;
}

int MaxInt(int a, int b)
{
    if(a >= b) return a;
    return b;
}

/* ===================================================================== */
/* Command events */

void OnQuit()
{
    // printf("Quit\n");
}

void OnPrint(int from, int to)
{
    // printf("Print(%d,%d)\n", from, to);

    for(int i = from; i <= to; i++){
        if(i > 0 && i <= t_len) printf("%s", text[i-1]);
        else printf(".\n");
    }
}

void OnChange(int from, int to)
{
    // printf("Change(%d,%d)\n",from, to);

    int prevLen = t_len;
    // Expand/Trim text to fit
    SetTextLength(MaxInt(to, prevLen));

    UpdateHistory();  // count = current + 2, current+1 is the undo's state
   
    edit_t *undo = GetStateEdit(UNDO);
    SetupEdit(undo, CHANGE, from, prevLen, 0);
    
    edit_t *redo = GetStateEdit(REDO);
    SetupEdit(redo, CHANGE, from, t_len, 0);


    for(int i = from; i <= to; i++){

        // printf("Getting input for line: %d\n", i);

        // Get input
        in_buf = NULL;
        in_len = getline(&in_buf, &in_size, stdin);

        // If line is being overwritten
        if(i <= prevLen)
        {
            AddLineToEdit(undo, &text[i-1]);

            // Free the memory used for its content (pre undo)
            // free(text[i-1]);
        }

        SetLine(i, &in_buf);

        AddLineToEdit(redo, &in_buf);
    }

    s_curr++;
}

void OnDelete(int from, int to)
{
    // printf("Delete(%d,%d)\n", from, to);
    if(from > t_len || to < 1) 
    {
        UpdateHistory();
    
        edit_t *undo = GetStateEdit(UNDO);
        SetupEdit(undo, SKIP, 0, 0, 0);
    
        edit_t *redo = GetStateEdit(REDO);
        SetupEdit(redo, SKIP, 0, 0, 0);

        s_curr++;
        return;
    }

    int lastToRemove = MinInt(to, t_len);
    int offset = lastToRemove - from + 1;

    UpdateHistory();
    
    edit_t *undo = GetStateEdit(UNDO);
    SetupEdit(undo, DELETE, from, t_len, 0);
    
    edit_t *redo = GetStateEdit(REDO);
    SetupEdit(redo, DELETE, from, t_len-offset, offset);

    // ShiftLinesLeft(from, offset);
    // Replaced shift lines call with its code to implement undoing/redoing

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

    s_curr++;
}

void OnUndo(int steps){

    edit_t *undo;

    while(s_curr > 0 && steps > 0)
    {
        undo = history[s_curr].undo;

        // printf("Undo: %c ----\n", undo->code);
        // printf("Where: %d\n", undo->location);
        // printf("Set Length to: %d\n", undo->setlen);
        // printf("Lines: %d\n", undo->size);
        // for(int i = 0; i< undo->size; i++){
        //     printf("(%d) %s", i+1, undo->lines[i]);
        // }


        switch (undo->code)
        {
        case CHANGE:
            /* Undo Change ------------------ */
            {
                SetTextLength(undo->setlen);

                for(int i = 0; i < undo->size; i++){
                    SetLine(undo->location + i, &undo->lines[i]);
                }
            }
            break;
        case DELETE:
            /* Undo Delete (do insert) ------ */
            {
                // TODO Maybe here make it only one loop?


                SetTextLength(undo->setlen);

                int cursor = t_len - 1;
                while(cursor >= undo->location){
                    if(cursor-undo->size >= 0)
                        text[cursor] = text[cursor-undo->size];

                    cursor--;
                }

                for(int i = 0; i < undo->size; i++){

                    // text[pullTo +i ] = text[pullFrom + i];
                    SetLine(undo->location + i, &undo->lines[i]);
                }
            }

            break;
        }

        steps--;
        s_curr--;
    }


}

void OnRedo(int steps){

    edit_t *redo;

    while(s_curr < s_count - 1 && steps > 0)
    {

        redo = history[s_curr].redo;

        // printf("Redo %c----\n", redo->code);
        // printf("Where: %d\n", redo->location);
        // printf("Set Length to: %d\n", redo->setlen);
        // printf("Lines: %d\n", redo->size);
        // for(int i = 0; i< redo->size && redo->code != DELETE; i++){
        //     printf("(%d) %s", i+1, redo->lines[i]);
        // }

        switch (redo->code)
        {
        case CHANGE:
            /* Redo Change ------------------ */
            {
                SetTextLength(redo->setlen);
                for(int i = 0; i < redo->size; i++){
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
        s_curr++;
    }
}













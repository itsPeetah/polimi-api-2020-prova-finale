#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEXT_BLOCK_SIZE 8
#define CHANGE 'c'
#define PRINT 'p'
#define DELETE 'd'
#define QUIT 'q'
#define UNDO 'u'
#define REDO 'r'
#define INSERT 'i'

/* ===================================================================== */
/* typedefs */

typedef struct edit{
    char code;
    int location;
    int size;
    char **lines;
}edit_t;

typedef struct state{
    struct state *prev, *next;
    edit_t *undo;
    edit_t *redo;
}state_t;

/* ===================================================================== */
/* Function declarations */

void AdjustCapacity(int required_lines);
void SetCapacity(int capacity);

void SetLength(int length);
void SetLine(int location, char** content);
void ShiftLinesLeft(int from, int offset);

int MinInt(int a, int b);
int MaxInt(int a, int b);

void OnQuit();
void OnPrint(int from, int to);
void OnChange(int from, int to);
void OnDelete(int from, int to);
void OnUndo(int steps);
void OnRedo(int steps);

/* ===================================================================== */
/* Globals */

char **text;
int t_cap = 0;                  // Text capacity
int t_len = 0;                  // Text length

char *in_buf = NULL;
size_t in_size = 0;
ssize_t in_len = 0;

char status = 0;

state_t **history;

/* ===================================================================== */
/* Main */

int main(int argc, char* argv[]){

    // Init text
    SetCapacity(TEXT_BLOCK_SIZE);

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

void SetCapacity(int capacity)
{
    t_cap = capacity;
    text = (char**)realloc(text, t_cap * sizeof(char*));
}

void AdjustCapacity(int required_lines)
{

    int capacity = t_cap;
    // printf("Text had capacity: %d\n",capacity);

    if(required_lines < capacity) 
    {
        // Decrease capacity to trim out any non required text space
        while(capacity - TEXT_BLOCK_SIZE >= required_lines)
        {
            capacity -= TEXT_BLOCK_SIZE;
        }

        // Release memory used for excess text lines
        // FreeOldText(required_lines, t_cap);

    }
    else
    {
        // Increase capacity to fit the required text size
        while(required_lines > capacity)
        {
            capacity += TEXT_BLOCK_SIZE;
        }
    }

    // printf("Text now has capacity: %d\n",capacity);

    SetCapacity(capacity);
}

void SetLength(int length)
{
    t_len = length;
    AdjustCapacity(t_len);
}

void SetLine(int location, char** content)
{
    text[location-1] = (*content);
}

void ShiftLinesLeft(int from, int offset)
{
    int cursor = from - 1;
    while(cursor < t_len){

        if(cursor + offset < t_len){

            text[cursor] = text[cursor+offset];
            text[cursor + offset] = NULL;
        }
        cursor++;
    }
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
    SetLength(MaxInt(to, prevLen));

    for(int i = from; i <= to; i++){

        // printf("Getting input for line: %d\n", i);

        // Get input
        in_buf = NULL;
        in_len = getline(&in_buf, &in_size, stdin);

        // If line is being overwritten
        if(i < prevLen)
        {
            // Free the memory used for its content
            free(text[i-1]);
        }

        SetLine(i, &in_buf);
    }
}
void OnDelete(int from, int to){
    // printf("Delete(%d,%d)\n", from, to);
    if(from > t_len || to < 1) return;

    int lastToRemove = MinInt(to, t_len);
    int offset = lastToRemove - from + 1;

    ShiftLinesLeft(from, offset);

    SetLength(t_len-offset);

}
void OnUndo(int steps){
    // printf("Undo(%d)\n", steps);

}
void OnRedo(int steps){
    // printf("Redo(%d)\n", steps);
}

/* ===================================================================== */
/* Restores */













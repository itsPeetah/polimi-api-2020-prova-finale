#include <stdio.h>
#include <stdlib.h>

int CalculateTargetState(int steps);
int FindClosestSamplePoint(int to);

// CTX
int f = 5;
int max = 50;
int current;
int steps;

int main(){

    printf("Current state: ");
    scanf("%d", &current);
    printf("Steps: ");
    scanf("%d", &steps);

    // TARGET and CLOSEST SP
    int target = CalculateTargetState(steps);
    int closest = FindClosestSamplePoint(target);
    printf("Target state: %d; Closest SP: %d\n", target, closest);

    // Current is closer to the target then the closest sample point
    // I can add dist(closest sp) - 1 because it's still 2 actions
    if(abs(current-target) <= abs(closest-target))
    {
        printf("Current position is closer.\n");
        printf("Performing %d step(s).\n", target-current);
    }
    // Current is further from the target then the closest sample point
    else
    {
        printf("The sample point is closer.\n");
        printf("Restoring state %d.\n", closest);
        printf("Performing %d step(s).\n", target-closest);
    }
    

    return 0;    
}

int CalculateTargetState(int steps)
{
    int target = current + steps;
    if(target < 0) target = 0;
    else if (target > max - 1) target = max - 1;

    return target;
}

int FindClosestSamplePoint(int to)
{
    int remainder = to%f;
    int closest = to - remainder;
    if(remainder > f/2 && closest + f < max - 1) 
        closest += f; 
    return closest;
}
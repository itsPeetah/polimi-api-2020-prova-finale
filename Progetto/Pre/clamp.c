#include <stdio.h>

#define clamp(v,m,M) (((v)<(m))?(m):((v)>(M))?(M):(v))

int main(){

    int a = 5;
    int b;
    if ((b = 7)>a) printf("%d, %d", b, a);
}
#include<stdio.h>

void swap(long int *a,long int *b)
{
        long int tmp=*a;
        *a=*b;
        *b=tmp;
}
 
int main()
{
        long int a,b;
        a=1;
        b=2;
        swap(&a,&b);
}

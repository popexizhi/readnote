#include<stdio.h>

void swap(int *a,int *b)
{
        int tmp=*a;
        *a=*b;
        *b=tmp;
}
 
int main()
{
        int a,b;
        a=1;
        b=2;
        swap(&a,&b);
}

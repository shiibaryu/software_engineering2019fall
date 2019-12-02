#include <stdio.h>
#include <stdlib.h>

int data[10000];

static inline int get_random(int min,int max)
{
        return min + (int)(rand()*(max-min+1.0)/(1.0+RAND_MAX));
}

void swap(int *a,int *b)
{
        int ret = *a;
        *a = *b;
        *b = ret;
}

void selection_sort(int n)
{
        int i,max,t;
        for(i=n;i>=0;i--){
                max = i;
                for(t=i-1;t>=0;t--){
                        if(data[max]<data[t]){
                                max = t;
                        }
                }
                swap(&data[i],&data[max]);
        }
}

int binary_search(int val,int max,int min)
{
        int pos = (max + min)/2;
        if(max < min || pos > 10000){
                return 0;
        }
        if(data[pos] == val){
                return pos;
        }else if(data[pos] < val){
                return binary_search(val,max,min+1);
        }else if(data[pos] > val){
                return binary_search(val,max-1,min);
        }
}


int main()
{
        int i;
        int size = 10000;
        for(i=0;i<size;i++){
                data[i] = get_random(1,size);
        }
        
        selection_sort(size);

        for(i=0;i<size;i++){
                printf("%d ",data[i]);
        }
        
        printf("\nPlease type a value you want to search\n");
        scanf("%d",&i);

        int ans=0;
        if(ans=binary_search(i,size-1,0)){
                printf("Found\n");
                printf("The position of value %d is %d\n",i,ans); 
        }else{
                printf("404 Not Found\n");
        }

        return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct list{
        int data;
        struct list *next;
        struct list *prev;
};

int data[10010];
int flag;

int open_addr(int rest,int x,int delta)
{
        if(flag){
                return 0;
        }
        if(data[rest] == 0){
               data[rest] = x;
               flag = 1;
               return 0; 
        }else{
               rest = rest + delta;
               if(rest > 10000){
                        rest = rest - 10000;
                }
                return open_addr(rest,x,delta);
        }
}

int main()
{
        int i;
        struct list *list = malloc(sizeof(struct list));
        
        for(i=0;i<10000;i++){
                list->data = rand()+1;
                list->next = malloc(sizeof(struct list));
                struct list *ls = list;
                list = list->next;
                list->prev = ls;
        }
        while(list->prev != NULL){
                list = list->prev;
        }

        int rest;
        int delta=7;
        while(list->next != NULL){
               rest = list->data%10000;
               flag = 0;
               open_addr(rest,list->data,delta);
               list = list->next;
        }
       
        for(i=0;i<10000;i++){
                printf ("%d ",data[i]);
        }
        printf("\nPlease type a Key for hash\n");
        printf("Range of the key is 1 to 10000\n");
        scanf("%d",&i);
        if(i <= 0 || i > 10000){
                printf("invalid range\n");
                return 0;
        }
        printf("Found\n");
        printf("The value is %d\n",data[i-1]);

        return 0;
}

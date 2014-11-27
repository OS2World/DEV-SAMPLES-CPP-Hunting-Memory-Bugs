#include <stdio.h>
#include <string.h>
#include <stdlib.h>

struct entry {         
        int low;
        char line[80];
};

#define MAXALLOCATIONS 16000
entry *alloc[MAXALLOCATIONS];
int allocs=0;

int main(int argc, char **argv) {
        if(argc>2 || (argc==2 && strcmp(argv[1],"/?")==0))
        {
                fprintf(stderr,"usage: leakfind [<trace file>]\n");
                return 1;
        }
        const char *fn;
        if(argc==2) fn=argv[1];
        else fn="tracemon.log";
        FILE *fp=fopen(fn,"r");
        if(!fp) {
                perror(fn);
                return 2;
        }
        
        char line[256];
        while(fgets(line,256,fp)) {
                int low=strtol(line+36,NULL,16);
                if(line[18]=='m' || line[18]=='n') {
                        //allocation
                        if(allocs==MAXALLOCATIONS) {
                                fprintf(stderr,"Too many peak allocations\n");
                                return 3;
                        }
                        entry *e=new entry;
                        e->low = low;
                        memcpy(e->line,line,79);
                        e->line[79]='\0';
                        alloc[allocs++] = e;
                } else {
                        //deallocation
                        int a;
                        for(a=0; a<allocs; a++)
                                if(alloc[a]->low == low)
                                        break;
                        if(a<allocs) {
                                delete alloc[a];
                                memmove(alloc+a,alloc+a+1,sizeof(alloc[0])*(allocs-a-1));
                                allocs--;
                        } else {
                                fprintf(stderr,"Could not find allocated block\n%s",line);
                                return 3;
                        }
                }
        }
        if(allocs==0) {
                printf("No leaks\n");
        } else {        
                printf("Leaks:\n");
                for(int a=0; a<allocs; a++)
                        printf("%s",alloc[a]->line);
        }                
        
        return 0;
}


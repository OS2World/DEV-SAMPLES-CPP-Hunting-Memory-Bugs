#include <stdio.h>
#include <stdlib.h>
#include <search.h>

struct groupentry {
        unsigned maxsize;
        unsigned operations;         //number of total allocations this size
        unsigned current_watermark;  //current number allocs
        unsigned high_watermark;
};

#define MAXGROUP 9
static groupentry ge[MAXGROUP] = {
        {4},              //smaller than a pointer
        {8},              //smaller than usual overhead
        {16},             //
        {256},
        {1024},
        {4096},           //less than a page
        {32768},
        {65536},
        {0xffffffffUL}    //very big
};


struct sumentry {
        unsigned size;
        unsigned count;
};
#define MAXSUM 4096
sumentry se[MAXSUM];
unsigned sumentries=0;
int toomanysumentries=0;

static int
#ifdef __IBMCPP__
_Optlink
#endif
comparesumentries(const void *p1, const void *p2) {
        if(((sumentry*)p1)->size < ((sumentry*)p2)->size)
                return -1;
        else
                return 1;
}

static void sortsumentries() {
        qsort(se,sumentries,sizeof(se[0]),comparesumentries);
}

int main(int argc, char **argv) {
        if(argc!=2) {
                fprintf(stderr,"usage: heapstat <heap trace file>\n");
                return 1;
        }
        
        FILE *fp=fopen(argv[1],"r");
        if(!fp) {
                perror(argv[1]);
                return 2;
        }
        
        char line[256];
        while(fgets(line,256,fp)) {
                unsigned size=strtol(line+63,0,16);
                int i;
                for(i=0; ge[i].maxsize<size; i++)
                        ;
                if(line[18]=='m' || line[18]=='n') {
                        //allocation
                        ge[i].operations++;
                        ge[i].current_watermark++;
                        if(ge[i].current_watermark>ge[i].high_watermark)
                                ge[i].high_watermark = ge[i].current_watermark;
                        if(!toomanysumentries) {
                                int j;
                                for(j=0; j<sumentries && se[j].size!=size; j++)
                                        ;
                                if(j<sumentries)
                                        se[j].count++;
                                else if(sumentries<MAXSUM) {
                                        se[sumentries].size = size;
                                        se[sumentries].count = 1;
                                        sumentries++;
                                } else
                                        toomanysumentries=1;
                        }
                } else {
                        //deallocation
                        ge[i].current_watermark--;
                }
        }
        fclose(fp);

        printf("Heap statistics:\n");
        printf("Size<=     Operations  Peak\n");
        for(int i=0; i<MAXGROUP; i++) {
                printf("%10u %10u %6u\n",
                       ge[i].maxsize,
                       ge[i].operations,
                       ge[i].high_watermark
                      ); 
        }
        if(!toomanysumentries) {
                sortsumentries();
                printf("  Size  Count\n");
                for(int j=0; j<sumentries; j++)
                        printf("%7u %5u\n",se[j].size,se[j].count);
        } else
                printf("Allocations are too diverse to print indivual statistics\n");
        
        return 0;
}


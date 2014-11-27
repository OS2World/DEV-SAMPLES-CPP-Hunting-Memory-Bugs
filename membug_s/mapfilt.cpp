#include <stdio.h>
#include <search.h>
#include <stdlib.h>
#include <string.h>

static char *line[5000];
static int lines=0;

static void readit(const char *inname) {
        FILE *fp=fopen(inname,"r");
        if(!fp) {
                perror(inname);
                exit(1);
        }

        char l[512];
        while(fgets(l,512,fp)) {
                int slen=strlen(l);
#if defined(__WATCOMC__)
                //test for Watcoms wlink .MAP files
                if(slen>13 &&
                   l[4]==':' &&
                   l[0]=='0'
#elif defined(__IBMCPP__)
                //test for VAC++ ilink map files
                if(slen>14 &&
                   l[5]==':' &&
                   l[1]=='0'
#else
#error Unknown compiler
#endif
                  )
                {
                        line[lines] = (char*)malloc(slen+1);
                        memcpy(line[lines],l,slen+1);
                        lines++;
                }
        }
        fclose(fp);
}

static int 
#ifdef __IBMCPP__
_Optlink
#endif
compare(const void *p1, const void *p2) {
        return strcmp(*(const char**)p1,*(const char**)p2);
}

static void sortit() {
        qsort(line,lines,sizeof(line[0]),compare);
}


static void writeit(const char *fn) {
        FILE *fp=fopen(fn,"w");
        if(!fp) {
                perror(fn);
                exit(2);
        }

        for(int i=0; i<lines; i++)
                fputs(line[i],fp);

        fclose(fp);
}

int main(int argc, char **argv) {
        if(argc!=3) {
                fprintf(stderr,"usage: mapfilt <mapfile> <filtered mapfile>\n");
                return 1;
        }

        readit(argv[1]);
        sortit();
        writeit(argv[2]);

        return 0;
}

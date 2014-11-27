#if 0
//double free
void main(void) {
        char *p=new char[10];
        delete[] p;
        delete[] p;
}
#endif

#if 0
#include <stdlib.h>
//freeing uninitialized pointer
char *p;
void main(void) {
        free(p);
}
#endif

#if 0
//accessing freed memory
void main(void) {
        char *p=new char[10];
        delete[] p;
        
        *p = 'A';
}
#endif

#if 1
//memory leak
void main(void) {
        char *p1=new char[10];
        char *p2=new char[10];
        *p2='A';
        delete[] p1;
}
#endif

#if 0
//mix of heap management functions
void main(void) {
        char *p=new char[10];
        delete p;
}
#endif



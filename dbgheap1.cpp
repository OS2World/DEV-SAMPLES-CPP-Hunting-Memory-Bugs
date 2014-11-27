/* Simple debug heap that catches memory overwrites
 * This code should be compiled with stack checking OFF, since
 * malloc/realloc/free can be called very early when the stack has not been
 * initialized (or the stack-check code has not been initialized)
 */

#define INCL_DOSMEMMGR
#include <os2.h>

#include <stdlib.h>
#include <string.h>

struct chunk_header {
        unsigned chunk_size;            //# of comitted bytes
        unsigned block_size;            //heap block size (requested size)
        unsigned block_offset;          //offset from start of block to (user-)pointer
};
#define FILL_BYTE 0xfe

static void *allocmem(unsigned size) {
        //round up to nearest 4KB block size
        unsigned chunk_size=((size+sizeof(chunk_header)+4095)&~4095);
        unsigned alloc_size=chunk_size + 4096; //extra for uncommitted page

        APIRET rc;

        //allocate the memory (reserve virtual address space)
        char *pchunk;
        rc=DosAllocMem((PPVOID)&pchunk, alloc_size, PAG_READ|PAG_WRITE);
        if(rc!=0) return 0;

        //commit all but the last page
        rc=DosSetMem(pchunk, chunk_size, PAG_READ|PAG_WRITE|PAG_COMMIT);
        if(rc!=0) {
                DosFreeMem(pchunk);
                return 0;
        }

        //set initial bytes of block to something
        memset(pchunk,FILL_BYTE,alloc_size-4096);

        //setup the chunk housekeeping structure
        chunk_header *chp=(chunk_header *)pchunk;
        chp->chunk_size   = chunk_size;
        chp->block_size   = size;
        chp->block_offset = chp->chunk_size-chp->block_size;

        return pchunk+chp->block_offset;
}

static inline chunk_header *ptr2chunk(void *p) {
        return (chunk_header*)(((unsigned long)p)&0xFFFFE000);
}

static void freemem(void *p) {
        char *pchunk=(char*)ptr2chunk(p);
        chunk_header *chp=(chunk_header *)pchunk;

        //check for overwrite of the chunk header structure
        if(chp->block_offset != chp->chunk_size-chp->block_size) {
                exit(0);
        }
        //check for correct pointer
        if(p != pchunk+chp->block_offset) {
                exit(0);
        }
        //check for overwrite of the unused bytes before the block
        for(unsigned char *checkp=((unsigned char*)pchunk)+sizeof(chunk_header);
            checkp<p;
            checkp++)
        {
                if(*checkp != FILL_BYTE)
                        exit(0);
        }

        DosFreeMem(pchunk);
}

void *operator new(size_t bytes) {
        return allocmem(bytes);
}

void operator delete(void *p) {
        if(p) freemem(p);
}

#if 1
//your compiler may or may not support these
void *operator new[](size_t bytes) {
        return allocmem(bytes);
}

void operator delete[](void *p) {
        if(p) freemem(p);
}
#endif

#if 1
//You may or may not want to override malloc/free
void *malloc(size_t size) {
        return allocmem((unsigned)size);
}

void free(void *p) {
        freemem(p);
}

void *realloc(void *oldp, size_t size) {
        //realloc is a bit tricky since it can allocate, deallocate, 
        //shrink and expand the block
        void *newp=0;
        if(size>0) {
                if(!oldp) {
                        //allocate
                        newp = allocmem(size);
                } else {
                        //reallocate
                        newp = allocmem(size);
                        chunk_header *newchunk=ptr2chunk(newp);
                        //copy contents
                        chunk_header *oldchunk = ptr2chunk(oldp);
                        if(oldchunk->block_size<newchunk->block_size)
                                memcpy(newp,oldp,oldchunk->block_size);
                        else
                                memcpy(newp,oldp,newchunk->block_size);
                        freemem(oldp);        
                }
        } else {
                if(oldp) {
                        //deallocate
                        freemem(oldp);
                } else {
                        //nothing
                }
        }
        return newp;
}

void *calloc(size_t n, size_t size) {
        size_t bytes=n*size;
        if(bytes==0) return 0;
        void *p = allocmem(bytes);
        if(p)
                memset(p,0,bytes);
        return p;        
}

extern "C" void *_nmalloc(size_t size) {
        return allocmem((unsigned)size);
}

#endif

#if 0
int main() {
        char *p = new char[12];
        strcpy(p,"Hello world");                  //ok
        strcpy(p,"Hello my friend. What's your name?"); //error
        delete[] p;
        return 0;
}
#endif

/* dbgheap2.cpp - Advanced debug heap
 * This code should be compiled with stack checking OFF, since
 * malloc/realloc/free can be called very early when the stack has not been
 * initialized (or the stack-check code has not been initialized)
 */

#define INCL_DOSMEMMGR
#define INCL_DOSFILEMGR
#define INCL_DOSPROCESS
#define INCL_DOSNMPIPES
#include <os2.h>

#include <stdlib.h>
#include <string.h>

struct chunk_header {
        unsigned chunk_size;            //# of comitted bytes
        unsigned block_size;            //heap block size (requested size)
        unsigned block_offset;          //offset from start of block to (user-)pointer
        unsigned mgrclass;              //management class C/C++/C++ array
        unsigned heap;
};
#define FILL_BYTE 0xfe

#define HEAPMGRCLASS_C          1
#define HEAPMGRCLASS_CPP        2
#define HEAPMGRCLASS_CPPARRAY   3

extern "C" void heap_panic(); //located in assembler file

static unsigned heap_identification=(unsigned)&heap_identification;


//convert a 32-bit integer to a 8-digit hex string
static void i2x(unsigned x, char *buf) {
        buf+=7;
        for(int i=0; i<8; i++) {
                static char xdigit[17]="0123456789abcdef";
                *(buf--) = xdigit[x&0xf];
                x>>=4;
        }        
}

static HFILE log_operationstart(int alloc, void *caller, int mgrclass)
{
        APIRET rc;
        rc = DosWaitNPipe("\\pipe\\tracemon",-1);
        if(rc) heap_panic();
        HFILE hf;
        ULONG action;
        rc = DosOpen("\\pipe\\tracemon",
                     &hf,
                     &action,
                     0,
                     0,
                     OPEN_ACTION_FAIL_IF_NEW|OPEN_ACTION_OPEN_IF_EXISTS,
                     OPEN_FLAGS_NOINHERIT|OPEN_SHARE_DENYNONE|OPEN_ACCESS_WRITEONLY,
                     0
                    );
        if(rc) heap_panic(); //could not connect to trace monitor
        
        
        char buf[9];
        ULONG byteswritten;
        PTIB ptib;
        PPIB ppib;
        DosGetInfoBlocks(&ptib,&ppib);
        i2x(ptib->tib_ptib2->tib2_ultid,buf);
        DosWrite(hf,buf,8,&byteswritten);
        DosWrite(hf,(PSZ)" ",1,&byteswritten);
        
        i2x(heap_identification,buf);
        DosWrite(hf,buf,8,&byteswritten);
        DosWrite(hf,(PSZ)" ",1,&byteswritten);
        
        const char *op;
        if(alloc) {
                if(mgrclass==HEAPMGRCLASS_C)
                        op = "malloc()";
                else if(mgrclass==HEAPMGRCLASS_CPP)
                        op = "new     ";
                else /*if(mgrclass==HEAPMGRCLASS_CPPARRAY)*/
                        op = "new[]   ";
        } else {
                if(mgrclass==HEAPMGRCLASS_C)
                        op = "free()  ";
                else if(mgrclass==HEAPMGRCLASS_CPP)
                        op = "delete  ";
                else /*if(mgrclass==HEAPMGRCLASS_CPPARRAY)*/
                        op = "delete[]";
        }
        DosWrite(hf,(PSZ)op,8,&byteswritten);
        DosWrite(hf,(PSZ)" ",1,&byteswritten);
        
        i2x((unsigned)caller,buf);
        DosWrite(hf,buf,8,&byteswritten);
        DosWrite(hf,(PSZ)" ",1,&byteswritten);
        
        return hf;
}

static void log_range(HFILE hf, unsigned low, unsigned high) {
        char buf[9];
        ULONG byteswritten;
        i2x(low,buf);
        DosWrite(hf,buf,8,&byteswritten);
        DosWrite(hf,(PSZ)"-",1,&byteswritten);
        i2x(high,buf);
        DosWrite(hf,buf,8,&byteswritten);
        DosWrite(hf,(PSZ)" ",1,&byteswritten);
}

static void log_norange(HFILE hf) {
        ULONG byteswritten;
        DosWrite(hf,(PSZ)"                  ",18,&byteswritten);
}

static void log_userptr(HFILE hf, unsigned ptr) {
        char buf[9];
        ULONG byteswritten;
        i2x(ptr,buf);
        DosWrite(hf,buf,8,&byteswritten);
        DosWrite(hf,(PSZ)" ",1,&byteswritten);
}

static void log_size(HFILE hf, unsigned size) {
        char buf[9];
        ULONG byteswritten;
        i2x(size,buf);
        DosWrite(hf,buf,8,&byteswritten);
}

static void log_nosize(HFILE hf) {
        ULONG byteswritten;
        DosWrite(hf,(PSZ)"        ",8,&byteswritten);
}

static void log_operationfailure(HFILE hf, const char *errmsg) {
        ULONG byteswritten;
        DosWrite(hf,(PSZ)" ",1,&byteswritten);
        DosWrite(hf,(PSZ)errmsg,strlen(errmsg),&byteswritten);
        DosWrite(hf,(PSZ)"\r\n",2,&byteswritten);
        DosClose(hf);
        heap_panic();
}

static void log_operationsuccess(HFILE hf)
{
        ULONG byteswritten;
        DosWrite(hf,(PSZ)"\r\n",2,&byteswritten);
        DosClose(hf);
}

static void *allocmem(unsigned size, void *caller, int mgrclass)
{
        HFILE hf=log_operationstart(1,caller,mgrclass);
        
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
        chp->mgrclass     = mgrclass;
        chp->heap = heap_identification;
        
        log_range(hf,(unsigned)pchunk,(unsigned)pchunk+chunk_size-1);
        log_userptr(hf,(unsigned)pchunk+chp->block_offset);
        log_size(hf,size);
        
        log_operationsuccess(hf);
        return pchunk+chp->block_offset;
}

static inline chunk_header *ptr2chunk(void *p) {
        return (chunk_header*)(((unsigned long)p)&0xFFFFE000);
}

static void freemem(void *p, void *caller, int mgrclass) {
        HFILE hf=log_operationstart(0,caller,mgrclass);
        
        char *pchunk=(char*)ptr2chunk(p);
        chunk_header *chp=(chunk_header *)pchunk;

        //check for read/write access to the pages
        APIRET rc;
        ULONG cb=4096;
        ULONG flag;
        rc = DosQueryMem((PVOID)pchunk, &cb, &flag);
        if(rc) {
                log_norange(hf);
                log_userptr(hf,(unsigned)p);
                log_operationfailure(hf,"DosQueryMem failed");
        }
        if(!((flag&PAG_COMMIT) && //must be committed
             (flag&PAG_BASE) &&   //mustbe base
             (flag&PAG_READ) &&   //must be readable
             (flag&PAG_WRITE)))   //must be writable
        {
                log_norange(hf);
                log_userptr(hf,(unsigned)p);
                log_nosize(hf);
                log_operationfailure(hf,"Invalid user pointer");
        }
        //check for overwrite of the chunk header structure
        if(chp->block_offset != chp->chunk_size-chp->block_size) {
                log_norange(hf);
                log_userptr(hf,(unsigned)p);
                log_nosize(hf);
                log_operationfailure(hf,"Bad pointer or header overwrite");
        }
        //check for correct pointer
        if(p != pchunk+chp->block_offset) {
                log_norange(hf);
                log_userptr(hf,(unsigned)p);
                log_nosize(hf);
                log_operationfailure(hf,"Bad pointer or header overwrite");
        }
        //check for valid mgrclass
        if(chp->mgrclass!=HEAPMGRCLASS_C &&
           chp->mgrclass!=HEAPMGRCLASS_CPP &&
           chp->mgrclass!=HEAPMGRCLASS_CPPARRAY)
        {
                log_norange(hf);
                log_userptr(hf,(unsigned)p);
                log_nosize(hf);
                log_operationfailure(hf,"Bad heap management class (overwrite?)");
        }   
        //check for overwrite of the unused bytes before the block
        for(unsigned char *checkp=((unsigned char*)pchunk)+sizeof(chunk_header);
            checkp<p;
            checkp++)
        {
                if(*checkp != FILL_BYTE) {
                        log_norange(hf);
                        log_userptr(hf,(unsigned)p);
                        log_nosize(hf);
                        log_operationfailure(hf,"Overwrite before user range");
                }        
        }
        //check for incorrect mgrclass
        if(chp->mgrclass != mgrclass) {
                log_range(hf,(unsigned)pchunk,(unsigned)pchunk+chp->chunk_size-1);
                log_userptr(hf,(unsigned)p);
                log_size(hf,chp->chunk_size);
                log_operationfailure(hf,"Mix of heap management classes");
        }
        //check for incorrect heap
        if(chp->heap!=heap_identification) {
                log_range(hf,(unsigned)pchunk,(unsigned)pchunk+chp->chunk_size-1);
                log_userptr(hf,(unsigned)p);
                log_size(hf,chp->chunk_size);
                log_operationfailure(hf,"Incorect heap module");
        }
        
        log_range(hf,(unsigned)pchunk,(unsigned)pchunk+chp->chunk_size-1);
        log_userptr(hf,(unsigned)p);
        log_size(hf,chp->block_size);
        
        //decommit the pages
        rc=DosSetMem(pchunk, chp->chunk_size, PAG_DECOMMIT);
        if(rc) {
                log_operationfailure(hf,"DosSetMem failed!?!");
        }        
        
        log_operationsuccess(hf);
}

extern "C" {
void *dbg_new(size_t bytes, void *caller) {
        return allocmem(bytes,caller,HEAPMGRCLASS_CPP);
}

void dbg_delete(void *p, void *caller) {
        if(p) freemem(p,caller,HEAPMGRCLASS_CPP);
}

void *dbg_new_array(size_t bytes, void *caller) {
        return allocmem(bytes,caller,HEAPMGRCLASS_CPPARRAY);
}

void dbg_delete_array(void *p, void *caller) {
        if(p) freemem(p,caller,HEAPMGRCLASS_CPPARRAY);
}

void *dbg_malloc(size_t size, void *caller) {
        return allocmem((unsigned)size,caller,HEAPMGRCLASS_C);
}

void dbg_free(void *p, void *caller) {
        freemem(p,caller,HEAPMGRCLASS_C);
}

void *dbg_realloc(void *oldp, size_t size, void *caller) {
        //realloc is a bit tricky since it can allocate, deallocate, 
        //shrink and expand the block
        void *newp=0;
        if(size>0) {
                if(!oldp) {
                        //allocate
                        newp = dbg_malloc(size,caller);
                } else {
                        //reallocate
                        newp = dbg_malloc(size,caller);
                        chunk_header *newchunk=ptr2chunk(newp);
                        //copy contents
                        chunk_header *oldchunk = ptr2chunk(oldp);
                        if(oldchunk->chunk_size<newchunk->chunk_size)
                                memcpy(newp,oldp,oldchunk->chunk_size);
                        else
                                memcpy(newp,oldp,newchunk->chunk_size);
                }
        } else {
                if(oldp) {
                        //deallocate
                        dbg_free(oldp,caller);
                } else {
                        //nothing
                }
        }
        return newp;
}

void *dbg_calloc(size_t n, size_t size, void *caller) {
        size_t bytes=n*size;
        if(bytes==0) return 0;
        void *p = allocmem(bytes,caller,HEAPMGRCLASS_C);
        if(p)
                memset(p,0,bytes);
        return p;        
}

} //extern "C"

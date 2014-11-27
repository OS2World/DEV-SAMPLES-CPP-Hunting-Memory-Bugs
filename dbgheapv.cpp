#include <stdlib.h>
#include <new.h>

extern "C" heap_panic();

extern "C" {

void *malloc(size_t size) {
        return (void*)size;
}

void free(void *p) {
        heap_panic();
}

void *realloc(void *p, size_t size) {
        free(p);
        return malloc(size);
}

}; //extern "C"

void *operator new(size_t size) {
        return malloc(size);
}

void *operator new[](size_t size) {
        return malloc(size);
}

void operator delete(void *location) {
        free(location);
}

void operator delete[](void *location) {
        free(location);
}




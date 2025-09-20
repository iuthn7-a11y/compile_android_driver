// comm.h
#ifndef COMM_H
#define COMM_H

#include <linux/types.h>

typedef struct _COPY_MEMORY {
    pid_t pid;
    unsigned long addr;
    void *buffer;
    size_t size;
} COPY_MEMORY;

typedef struct _MODULE_BASE {
    pid_t pid;
    char *name;
    unsigned long base;
} MODULE_BASE;

#endif

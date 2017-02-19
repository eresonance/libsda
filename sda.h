/* libsda - Simple dynamic array library
 *
 * Copyright (c) 2017 - Devin Linnington
 * 
 * Based on the project SDA:
 * https://github.com/antirez/sda
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */


#ifndef __SDA_H
#define __SDA_H

#define SDA_MAX_PREALLOC (1024*1024)

#include <sys/types.h>
#include <stdint.h>

//Types designed for type-checking arguments to functions that may use sda arrays
typedef int *sdaint;
typedef double *sdadouble;
typedef float *sdafloat;
//You can also use this generic type:
typedef void *sda;
//But then you need to use the sdaat function to access elements

struct __attribute__ ((__packed__)) sdahdr8 {
    uint8_t len; /* used */
    uint8_t alloc; /* excluding the header */
    uint8_t sz;  /* size of element */
    unsigned char flags; /* 3 lsb of type, 5 unused bits */
    char buf[];
};
struct __attribute__ ((__packed__)) sdahdr16 {
    uint16_t len; /* used */
    uint16_t alloc; /* excluding the header */
    uint8_t sz;  /* size of element */
    unsigned char flags; /* 3 lsb of type, 5 unused bits */
    char buf[];
};
struct __attribute__ ((__packed__)) sdahdr32 {
    uint32_t len; /* used */
    uint32_t alloc; /* excluding the header */
    uint8_t sz;  /* size of element */
    unsigned char flags; /* 3 lsb of type, 5 unused bits */
    char buf[];
};
struct __attribute__ ((__packed__)) sdahdr64 {
    uint64_t len; /* used */
    uint64_t alloc; /* excluding the header */
    uint8_t sz;  /* size of element */
    unsigned char flags; /* 3 lsb of type, 5 unused bits */
    char buf[];
};

//sdahdr* type
#define SDA_HTYPE_8  1
#define SDA_HTYPE_16 2
#define SDA_HTYPE_32 3
#define SDA_HTYPE_64 4
#define SDA_HTYPE_BITS 3
#define SDA_HTYPE_MASK 7

#if 0
//element type for run-time type-checking
#define SDA_ETYPE_CHAR  0
#define SDA_ETYPE_INT32 1
#define SDA_ETYPE_INT64 2
#define SDA_ETYPE_BITS  3
#define SDA_ETYPE_OFF   (SDA_HTYPE_BITS+1)
#define SDA_ETYPE_MASK  7<<(SDA_ETYPE_OFF)
//element sign
#define SDA_ESGN_SGN  0
#define SDA_ESGN_USGN 1
#define SDA_ESGN_BITS 1
#define SDA_ESGN_OFF  (SDA_ETYPE_OFF+1)
#define SDA_ESGN_MASK  1<<(SDA_ESGN_OFF)
#endif

#define SDA_HDR_TYPE(T)  struct sdahdr##T
#define SDA_HDR_VAR(T,s) struct sdahdr##T *sh = (void*)((s)-(sizeof(struct sdahdr##T)))
#define SDA_HDR(T,s) ((struct sdahdr##T *)((s)-(sizeof(struct sdahdr##T))))

/******* Methods for accessing each member *******/

/** Returns flags */
static inline unsigned char sdaflags(const sda s) {
    return ((const unsigned char *)s)[-1];
}

/** Returns len */
static inline size_t sdalen(const sda s) {
    unsigned char flags = sdaflags(s);
    switch(flags&SDA_HTYPE_MASK) {
        case SDA_HTYPE_8:
            return SDA_HDR(8,s)->len;
        case SDA_HTYPE_16:
            return SDA_HDR(16,s)->len;
        case SDA_HTYPE_32:
            return SDA_HDR(32,s)->len;
        case SDA_HTYPE_64:
            return SDA_HDR(64,s)->len;
    }
    return 0;
}

/* sdaalloc() = sdaavail() + sdalen() */
/** Returns alloc */
static inline size_t sdaalloc(const sda s) {
    unsigned char flags = sdaflags(s);
    switch(flags&SDA_HTYPE_MASK) {
        case SDA_HTYPE_8:
            return SDA_HDR(8,s)->alloc;
        case SDA_HTYPE_16:
            return SDA_HDR(16,s)->alloc;
        case SDA_HTYPE_32:
            return SDA_HDR(32,s)->alloc;
        case SDA_HTYPE_64:
            return SDA_HDR(64,s)->alloc;
    }
    return 0;
}

/** Returns sz */
static inline size_t sdasz(const sda s) {
    unsigned char flags = sdaflags(s);
    switch(flags&SDA_HTYPE_MASK) {
        case SDA_HTYPE_8:
            return SDA_HDR(8,s)->sz;
        case SDA_HTYPE_16:
            return SDA_HDR(16,s)->sz;
        case SDA_HTYPE_32:
            return SDA_HDR(32,s)->sz;
        case SDA_HTYPE_64:
            return SDA_HDR(64,s)->sz;
    }
    return 0;
}

/******* Helper methods for members *******/

/**
 * Returns the buffer size in bytes.
 */
static inline size_t sdasizeof(const sda s) {
    unsigned char flags = sdaflags(s);
    switch(flags&SDA_HTYPE_MASK) {
        case SDA_HTYPE_8: {
            SDA_HDR_VAR(8,s);
            return sh->sz * sh->len;
        }
        case SDA_HTYPE_16: {
            SDA_HDR_VAR(16,s);
            return sh->sz * sh->len;
        }
        case SDA_HTYPE_32: {
            SDA_HDR_VAR(32,s);
            return sh->sz * sh->len;
        }
        case SDA_HTYPE_64: {
            SDA_HDR_VAR(64,s);
            return sh->sz * sh->len;
        }
    }
    return 0;
}

/**
 * Returns the number of free elements that have been pre-allocated.
 */
static inline size_t sdaavail(const sda s) {
    unsigned char flags = sdaflags(s);
    switch(flags&SDA_HTYPE_MASK) {
        case SDA_HTYPE_8: {
            SDA_HDR_VAR(8,s);
            return sh->alloc - sh->len;
        }
        case SDA_HTYPE_16: {
            SDA_HDR_VAR(16,s);
            return sh->alloc - sh->len;
        }
        case SDA_HTYPE_32: {
            SDA_HDR_VAR(32,s);
            return sh->alloc - sh->len;
        }
        case SDA_HTYPE_64: {
            SDA_HDR_VAR(64,s);
            return sh->alloc - sh->len;
        }
    }
    return 0;
}

/**
 * Sets len directly (no re-allocation!)
 */
static inline void sdasetlen(sda s, size_t newlen) {
    unsigned char flags = sdaflags(s);
    switch(flags&SDA_HTYPE_MASK) {
        case SDA_HTYPE_8:
            SDA_HDR(8,s)->len = newlen;
            break;
        case SDA_HTYPE_16:
            SDA_HDR(16,s)->len = newlen;
            break;
        case SDA_HTYPE_32:
            SDA_HDR(32,s)->len = newlen;
            break;
        case SDA_HTYPE_64:
            SDA_HDR(64,s)->len = newlen;
            break;
    }
}

/**
 * Increments len directly (no re-allocation!)
 */
static inline void sdainclen(sda s, size_t inc) {
    unsigned char flags = sdaflags(s);
    switch(flags&SDA_HTYPE_MASK) {
        case SDA_HTYPE_8:
            SDA_HDR(8,s)->len += inc;
            break;
        case SDA_HTYPE_16:
            SDA_HDR(16,s)->len += inc;
            break;
        case SDA_HTYPE_32:
            SDA_HDR(32,s)->len += inc;
            break;
        case SDA_HTYPE_64:
            SDA_HDR(64,s)->len += inc;
            break;
    }
}

/**
 * Sets alloc directly (no re-allocation!)
 */
static inline void sdasetalloc(sda s, size_t newlen) {
    unsigned char flags = sdaflags(s);
    switch(flags&SDA_HTYPE_MASK) {
        case SDA_HTYPE_8:
            SDA_HDR(8,s)->alloc = newlen;
            break;
        case SDA_HTYPE_16:
            SDA_HDR(16,s)->alloc = newlen;
            break;
        case SDA_HTYPE_32:
            SDA_HDR(32,s)->alloc = newlen;
            break;
        case SDA_HTYPE_64:
            SDA_HDR(64,s)->alloc = newlen;
            break;
    }
}

/** Sets flags */
static inline void sdasetflags(sda s, unsigned char flags) {
    ((unsigned char*)s)[-1] = flags;
}

#if 0
/******* Higher level methods for operating on sda's *******/

sda sdanew(const void *init, size_t initlen);
sda sdaempty(void);
sda sdadup(const sda s);
void sdafree(sda s);
sda sdagrowzero(sda s, size_t len);
sda sdacat(sda s, const void *t, size_t len);
sda sdacatsda(sda s, const sda t);
sda sdacpy(sda s, const char *t, size_t len);

void sdarange(sda s, int start, int end);
void sdaupdatelen(sda s);
void sdaclear(sda s);
int sdacmp(const sda s1, const sda s2);

/******* Lower level methods for operating on sda's *******/

sda sdaMakeRoomFor(sda s, size_t addlen);
void sdaIncrLen(sda s, int incr);
sda sdaRemoveFreeSpace(sda s);
size_t sdaAllocSize(sda s);
void *sdaAllocPtr(sda s);

/******* Allocator exposure *******/

/* Export the allocator used by SDA to the program using SDA.
 * Sometimes the program SDA is linked to, may use a different set of
 * allocators, but may want to allocate or free things that SDA will
 * respectively free or allocate. */
void *sda_malloc(size_t size);
void *sda_realloc(void *ptr, size_t size);
void sda_free(void *ptr);
#endif //0

#endif //__SDA_H


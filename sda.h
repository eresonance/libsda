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

// the declaration
// FIXME
#define SDA(type, variable) do ( \
    ) while(0)


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

#define SDA_TYPE_8  1
#define SDA_TYPE_16 2
#define SDA_TYPE_32 3
#define SDA_TYPE_64 4
#define SDA_TYPE_MASK 7
#define SDA_TYPE_BITS 3
#define SDA_HDR_TYPE(T)  struct sdahdr##T
#define SDA_HDR_VAR(T,s) struct sdahdr##T *sh = (void*)((s)-(sizeof(struct sdahdr##T)))
#define SDA_HDR(T,s) ((struct sdahdr##T *)((s)-(sizeof(struct sdahdr##T))))

/******* Methods for accessing each member *******/

/** Returns len */
static inline size_t sdalen(const void *s) {
    unsigned char flags = s[-1];
    switch(flags&SDS_TYPE_MASK) {
        case SDS_TYPE_8:
            return SDS_HDR(8,s)->len;
        case SDS_TYPE_16:
            return SDS_HDR(16,s)->len;
        case SDS_TYPE_32:
            return SDS_HDR(32,s)->len;
        case SDS_TYPE_64:
            return SDS_HDR(64,s)->len;
    }
    return 0;
}

/* sdaalloc() = sdaavail() + sdalen() */
/** Returns alloc */
static inline size_t sdaalloc(const void *s) {
    unsigned char flags = s[-1];
    switch(flags&SDS_TYPE_MASK) {
        case SDS_TYPE_8:
            return SDS_HDR(8,s)->alloc;
        case SDS_TYPE_16:
            return SDS_HDR(16,s)->alloc;
        case SDS_TYPE_32:
            return SDS_HDR(32,s)->alloc;
        case SDS_TYPE_64:
            return SDS_HDR(64,s)->alloc;
    }
    return 0;
}

/** Returns sz */
static inline size_t sdasz(const void *s) {
    unsigned char flags = s[-1];
    switch(flags&SDS_TYPE_MASK) {
        case SDS_TYPE_8:
            return SDS_HDR(8,s)->sz;
        case SDS_TYPE_16:
            return SDS_HDR(16,s)->sz;
        case SDS_TYPE_32:
            return SDS_HDR(32,s)->sz;
        case SDS_TYPE_64:
            return SDS_HDR(64,s)->sz;
    }
    return 0;
}

/******* Helper methods for members *******/

/**
 * Returns the buffer size in bytes.
 */
static inline size_t sdasizeof(const void *s) {
    unsigned char flags = s[-1];
    switch(flags&SDS_TYPE_MASK) {
        case SDS_TYPE_8:
            SDA_HDR_VAR(8,s);
            return sh->sz * sh->len;
        case SDS_TYPE_16:
            SDA_HDR_VAR(16,s);
            return sh->sz * sh->len;
        case SDS_TYPE_32:
            SDA_HDR_VAR(32,s);
            return sh->sz * sh->len;
        case SDS_TYPE_64:
            SDA_HDR_VAR(64,s);
            return sh->sz * sh->len;
    }
    return 0;
}

/**
 * Returns the number of free elements that have been pre-allocated.
 */
static inline size_t sdaavail(const void *s) {
    unsigned char flags = s[-1];
    switch(flags&SDS_TYPE_MASK) {
        case SDS_TYPE_8:
            SDA_HDR_VAR(8,s);
            return sh->alloc - sh->len;
        case SDS_TYPE_16:
            SDA_HDR_VAR(16,s);
            return sh->alloc - sh->len;
        case SDS_TYPE_32:
            SDA_HDR_VAR(32,s);
            return sh->alloc - sh->len;
        case SDS_TYPE_64:
            SDA_HDR_VAR(64,s);
            return sh->alloc - sh->len;
    }
    return 0;
}

/**
 * Sets len directly (no re-allocation!)
 */
static inline void sdasetlen(void *s, size_t newlen) {
    unsigned char flags = s[-1];
    switch(flags&SDA_TYPE_MASK) {
        case SDA_TYPE_8:
            SDA_HDR(8,s)->len = newlen;
            break;
        case SDA_TYPE_16:
            SDA_HDR(16,s)->len = newlen;
            break;
        case SDA_TYPE_32:
            SDA_HDR(32,s)->len = newlen;
            break;
        case SDA_TYPE_64:
            SDA_HDR(64,s)->len = newlen;
            break;
    }
}

/**
 * Increments len directly (no re-allocation!)
 */
static inline void sdainclen(void *s, size_t inc) {
    unsigned char flags = s[-1];
    switch(flags&SDA_TYPE_MASK) {
        case SDA_TYPE_8:
            SDA_HDR(8,s)->len += inc;
            break;
        case SDA_TYPE_16:
            SDA_HDR(16,s)->len += inc;
            break;
        case SDA_TYPE_32:
            SDA_HDR(32,s)->len += inc;
            break;
        case SDA_TYPE_64:
            SDA_HDR(64,s)->len += inc;
            break;
    }
}

/**
 * Sets alloc directly (no re-allocation!)
 */
static inline void sdasetalloc(void *s, size_t newlen) {
    unsigned char flags = s[-1];
    switch(flags&SDA_TYPE_MASK) {
        case SDA_TYPE_8:
            SDA_HDR(8,s)->alloc = newlen;
            break;
        case SDA_TYPE_16:
            SDA_HDR(16,s)->alloc = newlen;
            break;
        case SDA_TYPE_32:
            SDA_HDR(32,s)->alloc = newlen;
            break;
        case SDA_TYPE_64:
            SDA_HDR(64,s)->alloc = newlen;
            break;
    }
}

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

#endif


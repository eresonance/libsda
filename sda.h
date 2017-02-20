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

#include <sys/types.h>
#include <stdint.h>
#include "sdsalloc.h"

#if defined(SDA_TEST_MAIN)
#include <stdio.h>
#endif

//Determines the number of elements to pre-allocate
#define SDA_MAX_PREALLOC (64)

//Types designed for type-checking arguments to functions that may use sda arrays
typedef char *sdachar;
typedef unsigned char *sdauchar;
typedef int *sdaint;
typedef unsigned int *sdauint;
//Otherwise you can also use this generic type:
typedef void *sda;
//But then you need to use the sda_* functions to access elements

//special attribute that tells gcc to automatically free the sda array when going out of scope
#define sda_raii __attribute__ ((__cleanup__ (_sda_raii_free)))

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
//agnostic/universal struct used in common methods that need *just* the header methods
struct sdahdr_uni {
    size_t len;
    size_t alloc;
    uint8_t sz;
    unsigned char flags;
};

//sdahdr* type
#define SDA_HTYPE_8  1
#define SDA_HTYPE_16 2
#define SDA_HTYPE_32 3
#define SDA_HTYPE_64 4
#define SDA_HTYPE_BITS 3
#define SDA_HTYPE_MASK 7

#define SDA_HDR_TYPE(T)  struct sdahdr##T
#define SDA_HDR_VAR(T,s) struct sdahdr##T *sh = (void*)(((char *)s)-(sizeof(struct sdahdr##T)))
#define SDA_HDR(T,s) ((struct sdahdr##T *)(((char *)s)-(sizeof(struct sdahdr##T))))

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

/** Returns a universal struct with everything populated but the buffer */
static inline void sdahdr(const sda s, struct sdahdr_uni *ret) {
    unsigned char flags = sdaflags(s);
    ret->flags = flags;
    switch(flags&SDA_HTYPE_MASK) {
        case SDA_HTYPE_8: {
            SDA_HDR_VAR(8,s);
            ret->len = sh->len;
            ret->alloc = sh->alloc;
            ret->sz = sh->sz;
            break;
        }
        case SDA_HTYPE_16: {
            SDA_HDR_VAR(16,s);
            ret->len = sh->len;
            ret->alloc = sh->alloc;
            ret->sz = sh->sz;
            break;
        }
        case SDA_HTYPE_32: {
            SDA_HDR_VAR(32,s);
            ret->len = sh->len;
            ret->alloc = sh->alloc;
            ret->sz = sh->sz;
            break;
        }
        case SDA_HTYPE_64: {
            SDA_HDR_VAR(64,s);
            ret->len = sh->len;
            ret->alloc = sh->alloc;
            ret->sz = sh->sz;
            break;
        }
        default:
            ret->len = 0;
            ret->alloc = 0;
            ret->sz = 0;
            break;
    }
}

/******* Helper methods for members *******/

/**
 * Returns the in-use buffer size in bytes (len*sz).
 */
static inline size_t sdasizeofbuf(const sda s) {
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
 * Returns the total allocated buffer size in bytes (alloc*sz).
 */
static inline size_t sdasizeofalloc(const sda s) {
    unsigned char flags = sdaflags(s);
    switch(flags&SDA_HTYPE_MASK) {
        case SDA_HTYPE_8: {
            SDA_HDR_VAR(8,s);
            return sh->sz * sh->alloc;
        }
        case SDA_HTYPE_16: {
            SDA_HDR_VAR(16,s);
            return sh->sz * sh->alloc;
        }
        case SDA_HTYPE_32: {
            SDA_HDR_VAR(32,s);
            return sh->sz * sh->alloc;
        }
        case SDA_HTYPE_64: {
            SDA_HDR_VAR(64,s);
            return sh->sz * sh->alloc;
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

/******* Higher level methods for operating on sda's *******/

/**
 * Create a new sda with the content specified by the 'init' pointer and 'init_sz'.
 *
 * If NULL is used for 'init' the array is initialized with zero bytes.
 *
 * Note that init_sz should be evenly divisible by sizeof(s)
 *
 * @param s: Pointer to sda array you want to init.
 * @param init: Pointer to array to initialize the sda buffer to, can be NULL.
 * @param init_sz: is sizeof(init) in bytes.
 */
#define sdanewsz(s, init, init_sz) (typeof(s))_sdanewsz((init), (init_sz), sizeof(*(s)))

/**
 * Create a new sda with the content specified by the 'init' *array* (NOT pointer).
 *
 * If NULL is used for 'init' the sda array is initialized with zero bytes.
 *
 * @param s: Pointer to sda array you want to init.
 * @param init: Array to initialize the sda buffer to, can be NULL. Not a pointer!
 */
#define sdanew(s, init) (typeof(s))_sdanewsz((init), sizeof(init), sizeof(*(s)))

/** Create an empty (zero length) sda array */
#define sdaempty(s) sdanewsz((s), NULL, 0)

/** Duplicate an sda array. */
#define sdadup(s, src) sdanewsz((s), (src), sdasizeofbuf(src))
//FIXME: Check whether sdasz(src) == sizeof(s)

/** Free an sda array
 * @param s: Can be NULL
 * @return: NULL always
 */
sda sdafree(sda s);

/** Make an sda array zero length, setting buffer as free space to be used later */
sda sdaclear(sda s);
/** Set the length of an sds array, reallocating as neccesary */
sda sdaresize(sda s, size_t len);

sda sda_append(sda s, const void *t, size_t size);
sda sda_extend(sda s, const sda t);
size_t sda_index(sda s, const void *x);
void *sda_pop(sda s);

//This macro:
// Tests if i is in range
//  Gets a void* pointer to element at i, casts it to the type of s that was passed in,
//  dereferences it, and then casts to the type that s was pointing to.
// Else:
//  Returns 0
/** 
 * Get an element from sda array s at index i.
 * This only works if the sda array s is not a "void *" (needs a type).
 * Returns NULL if i >= len of s
 */
#define sda_get(s, i) ({ \
    i < sdalen(s) ? \
        (typeof(*s))*((typeof(s))_sda_ptr_at((s), (i))) \
        : 0; \
    })
/**
 * Returns void pointer to element i of sds array s.
 * Return NULL if i >= len of s.
 */
static inline void *sda_ptr_at(sda s, size_t i) {
    unsigned char flags = sdaflags(s);
    switch(flags&SDA_HTYPE_MASK) {
        case SDA_HTYPE_8: {
            SDA_HDR_VAR(8,s);
            assert(i < sh->len);
            if(i < sh->len) return ((char *)s) + (sh->sz * i);
            return NULL;
        }
        case SDA_HTYPE_16: {
            SDA_HDR_VAR(16,s);
            assert(i < sh->len);
            if(i < sh->len) return ((char *)s) + (sh->sz * i);
            return NULL;
        }
        case SDA_HTYPE_32: {
            SDA_HDR_VAR(32,s);
            assert(i < sh->len);
            if(i < sh->len) return ((char *)s) + (sh->sz * i);
            return NULL;
        }
        case SDA_HTYPE_64: {
            SDA_HDR_VAR(64,s);
            assert(i < sh->len);
            if(i < sh->len) return ((char *)s) + (sh->sz * i);
            return NULL;
        }
    }
    return NULL;
}
// Unsafe
#define _sda_ptr_at(s, i) (((char *)s) + (sdasz(s)*(i)))

/**
 * Sets element i in sds array s to x.
 * x must be a rvalue and not a pointer.
 */
#define sda_set(s, i, x) ({ \
    typeof(x) tmp = (x); \
    _sda_set((s), (i), &tmp, sizeof(tmp)); \
    })
/**
 * Copies t of size bytes into index i of sds array s.
 */
static inline void _sda_set(sda s, size_t i, const void *t, size_t size) {
    unsigned char flags = sdaflags(s);
    switch(flags&SDA_HTYPE_MASK) {
        case SDA_HTYPE_8: {
            SDA_HDR_VAR(8,s);
            size_t start = i*sh->sz;
            //make sure we don't overrun len
            if((start + size) <= (sh->len*sh->sz)) {
                memcpy((((char *)s) + start), t, size);
            }
            break;
        }
        case SDA_HTYPE_16: {
            SDA_HDR_VAR(16,s);
            size_t start = i*sh->sz;
            //make sure we don't overrun len
            if((start + size) <= (sh->len*sh->sz)) {
                memcpy((((char *)s) + start), t, size);
            }
            break;
        }
        case SDA_HTYPE_32: {
            SDA_HDR_VAR(32,s);
            size_t start = i*sh->sz;
            //make sure we don't overrun len
            if((start + size) <= (sh->len*sh->sz)) {
                memcpy((((char *)s) + start), t, size);
            }
            break;
        }
        case SDA_HTYPE_64: {
            SDA_HDR_VAR(64,s);
            size_t start = i*sh->sz;
            //make sure we don't overrun len
            if((start + size) <= (sh->len*sh->sz)) {
                memcpy((((char *)s) + start), t, size);
            }
            break;
        }
    }
}
// Unsafe
#define _sdacpyi(s,i,t,size) memcpy(_sda_ptr_at((s),(i)),(t),(size))

#if 0
sda sdacat(sda s, const void *t, size_t len);
sda sdacpy(sda s, const char *t, size_t len);

void sdaslice(sda s, int start, int end);
int sdacmp(const sda s1, const sda s2);
#endif //0

/******* Lower level methods for operating on sda's *******/

sda sdaMakeRoomFor(sda s, size_t addlen);
sda sdaRemoveFreeSpace(sda s);
size_t sdaAllocSize(sda s);
void *sdaAllocPtr(sda s);

/* Don't call directly */
sda _sdanewsz(const void *init, size_t init_sz, size_t type_sz);
void _sda_raii_free(void *s);

/******* Allocator exposure *******/

/* Export the allocator used by SDA to the program using SDA.
 * Sometimes the program SDA is linked to, may use a different set of
 * allocators, but may want to allocate or free things that SDA will
 * respectively free or allocate. */
static inline void *_sda_malloc(size_t size) {
#if defined(SDA_TEST_MAIN)
    void *tmp = s_malloc(size);
    printf("s_malloc %p %zu\n", tmp, size);
    return tmp;
#else
    return s_malloc(size);
#endif
}
static inline void *_sda_realloc(void *ptr, size_t size) {
#if defined(SDA_TEST_MAIN)
    void *tmp = s_realloc(ptr,size);
    printf("s_realloc %p %zu -> %p\n", ptr, size, tmp);
    return tmp;
#else
    return s_realloc(ptr,size);
#endif
}
static inline void _sda_free(void *ptr) {
#if defined(SDA_TEST_MAIN)
    printf("s_free %p\n", ptr);
#endif
    s_free(ptr);
}

#endif //__SDA_H


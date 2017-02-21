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
#include <string.h>
#include "sdsalloc.h"

#if defined(SDA_TEST_MAIN)
#include <stdio.h>
#endif

//Determines the maximum number of bytes to pre-allocate, must be multiples of 64
#define SDA_MAX_PREALLOC (64*4)

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

// small version of the header, 32+16+8+8 = 64
struct __attribute__ ((__packed__)) sda_hdr_SM {
    /// Number of bytes allocated
    uint32_t alloc;
    /// Number of elements in the array
    uint16_t len;
    /// Size of each element
    uint8_t sz;
    /// HTYPE flags
    unsigned char flags;
    /// Buffer to hold the data in
    char buf[];
};
// medium version of the header, 64+32+8*4 = 128
struct __attribute__ ((__packed__)) sda_hdr_MD {
    uint64_t alloc;
    uint32_t len;
    uint8_t sz;
    uint8_t _pad[2]; // padding to make sure 64b accesses to buf[] are aligned
    unsigned char flags;
    char buf[];
};
// full-sized version of the header, 64+64+8*8 = 192
struct __attribute__ ((__packed__)) sda_hdr_LG {
    uint64_t alloc; //use uint64 over size_t to make sure 64b accesses are aligned on all arch's
    uint64_t len;
    uint8_t sz;
    uint8_t _pad[6]; // padding to make sure 64b accesses to buf[] are aligned
    unsigned char flags;
    char buf[];
};
//agnostic/universal struct used in common methods that need *just* the header methods
struct sda_hdr_uni {
    size_t len;
    size_t alloc;
    uint8_t sz;
    unsigned char flags;
};

//sda_hdr* type
#define SDA_HTYPE_SM  1
#define SDA_HTYPE_MD  2
#define SDA_HTYPE_LG  3
#define SDA_HTYPE_BITS 2
#define SDA_HTYPE_MASK 3

#define SDA_HDR_TYPE(T)  struct sda_hdr_##T
#define SDA_HDR_VAR(T,s) SDA_HDR_TYPE(T) *sh = (void*)(((char *)s)-(sizeof(SDA_HDR_TYPE(T))))
#define SDA_HDR(T,s) ((SDA_HDR_TYPE(T) *)(((char *)s)-(sizeof(SDA_HDR_TYPE(T)))))

/******* Methods for accessing each member *******/

/** Returns flags */
static inline unsigned char sda_flags(const sda s) {
    return ((const unsigned char *)s)[-1];
}

/** Returns len */
static inline size_t sda_len(const sda s) {
    unsigned char flags = sda_flags(s);
    switch(flags&SDA_HTYPE_MASK) {
        case SDA_HTYPE_SM:
            return SDA_HDR(SM,s)->len;
        case SDA_HTYPE_MD:
            return SDA_HDR(MD,s)->len;
        case SDA_HTYPE_LG:
            return SDA_HDR(LG,s)->len;
    }
    return 0;
}

/** Returns alloc */
static inline size_t sda_alloc(const sda s) {
    unsigned char flags = sda_flags(s);
    switch(flags&SDA_HTYPE_MASK) {
        case SDA_HTYPE_SM:
            return SDA_HDR(SM,s)->alloc;
        case SDA_HTYPE_MD:
            return SDA_HDR(MD,s)->alloc;
        case SDA_HTYPE_LG:
            return SDA_HDR(LG,s)->alloc;
    }
    return 0;
}

/** Returns sz */
static inline size_t sda_sz(const sda s) {
    unsigned char flags = sda_flags(s);
    switch(flags&SDA_HTYPE_MASK) {
        case SDA_HTYPE_SM:
            return SDA_HDR(SM,s)->sz;
        case SDA_HTYPE_MD:
            return SDA_HDR(MD,s)->sz;
        case SDA_HTYPE_LG:
            return SDA_HDR(LG,s)->sz;
    }
    return 0;
}

/** Returns a universal struct with everything populated but the buffer */
static inline void sda_hdr(const sda s, struct sda_hdr_uni *ret) {
    unsigned char flags = sda_flags(s);
    ret->flags = flags;
    switch(flags&SDA_HTYPE_MASK) {
        case SDA_HTYPE_SM: {
            SDA_HDR_VAR(SM,s);
            ret->len = sh->len;
            ret->alloc = sh->alloc;
            ret->sz = sh->sz;
            break;
        }
        case SDA_HTYPE_MD: {
            SDA_HDR_VAR(MD,s);
            ret->len = sh->len;
            ret->alloc = sh->alloc;
            ret->sz = sh->sz;
            break;
        }
        case SDA_HTYPE_LG: {
            SDA_HDR_VAR(LG,s);
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
static inline size_t sda_size(const sda s) {
    unsigned char flags = sda_flags(s);
    switch(flags&SDA_HTYPE_MASK) {
        case SDA_HTYPE_SM: {
            SDA_HDR_VAR(SM,s);
            return sh->sz * sh->len;
        }
        case SDA_HTYPE_MD: {
            SDA_HDR_VAR(MD,s);
            return sh->sz * sh->len;
        }
        case SDA_HTYPE_LG: {
            SDA_HDR_VAR(LG,s);
            return sh->sz * sh->len;
        }
    }
    return 0;
}

/**
 * Returns the number of free elements that have been pre-allocated.
 */
static inline size_t sda_avail(const sda s) {
    unsigned char flags = sda_flags(s);
    switch(flags&SDA_HTYPE_MASK) {
        case SDA_HTYPE_SM: {
            SDA_HDR_VAR(SM,s);
            return sh->alloc/sh->sz - sh->len;
        }
        case SDA_HTYPE_MD: {
            SDA_HDR_VAR(MD,s);
            return sh->alloc/sh->sz - sh->len;
        }
        case SDA_HTYPE_LG: {
            SDA_HDR_VAR(LG,s);
            return sh->alloc/sh->sz - sh->len;
        }
    }
    return 0;
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
#define sda_new_sz(s, init, init_sz) (__typeof__(s))_sda_new_sz((init), (init_sz), sizeof(*(s)))

/**
 * Create a new sda with the content specified by the 'init' *array* (NOT pointer).
 *
 * If NULL is used for 'init' the sda array is initialized with zero bytes.
 *
 * @param s: Pointer to sda array you want to init.
 * @param init: Array to initialize the sda buffer to, can be NULL. Not a pointer!
 */
#define sda_new(s, init) (__typeof__(s))_sda_new_sz((init), sizeof(init), sizeof(*(s)))

/** Create an empty (zero length) sda array */
#define sda_empty(s) sda_new_sz((s), NULL, 0)

/** Duplicate an sda array, returning a pointer to the new sda array.
 * The sizeof(*t) must be the same as whatever you're assigning this to.
 */
#define sda_dup(t) ({ \
    __typeof__(t) tmp = (t); \
    sda_new_sz(tmp, tmp, sda_size(tmp)); \
    })

/** Free an sda array
 * @param s: Can be NULL
 * @return: NULL always
 */
sda sda_free(sda s);

/** Make an sda array zero length, setting buffer as free space to be used later */
sda sda_clear(sda s);
/** Set the length of an sds array, reallocating as neccesary */
sda sda_resize(sda s, size_t size);
/** Add t of size bytes to the end of s */
sda sda_cat(sda s, const void *t, size_t size);
/** Add t to the end of s, growing if needed */
sda sda_extend(sda s, const sda t);
/** Copy size bytes of t over top of s at index i */
sda sda_cpy(sda s, size_t i, const void *t, size_t size);
/** Replace s with the copied contents of t */
sda sda_replace(sda s, const sda t);
/** Appends item x to sda array s */
#define sda_append(s, x) ({ \
    __typeof__(x) tmp = (x); \
    assert(sizeof(x) == sda_sz(s)); \
    (__typeof__(s))(sda_cpy((s), sda_len(s), &tmp, sizeof(tmp))); \
    })
/** Pop item off of the end of s, returning the item */
#define sda_pop(s) ({ \
    typeof(s) ret = (__typeof__(s))_sda_pop(s); \
    ret != NULL ? (__typeof__(*s))*ret : 0 \
    })
/** Pop an item off of the end of s, returning a pointer to that item */
void *sda_pop_ptr(sda s);


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
    typeof(s) ret = (__typeof__(s))sda_ptr_at((s), (i)); \
    ret != NULL ? (__typeof__(*s))*ret : 0; \
    })
/**
 * Returns void pointer to element i of sds array s.
 * Return NULL if i >= len of s.
 */
static inline void *sda_ptr_at(sda s, size_t i) {
    unsigned char flags = sda_flags(s);
    switch(flags&SDA_HTYPE_MASK) {
        case SDA_HTYPE_SM: {
            SDA_HDR_VAR(SM,s);
            if(i < sh->len) return ((char *)s) + (sh->sz * i);
            return NULL;
        }
        case SDA_HTYPE_MD: {
            SDA_HDR_VAR(MD,s);
            if(i < sh->len) return ((char *)s) + (sh->sz * i);
            return NULL;
        }
        case SDA_HTYPE_LG: {
            SDA_HDR_VAR(LG,s);
            if(i < sh->len) return ((char *)s) + (sh->sz * i);
            return NULL;
        }
    }
    return NULL;
}

/**
 * Sets element i in sds array s to x.
 * x must be a rvalue and not a pointer.
 */
#define sda_set(s, i, x) ({ \
    __typeof__(x) tmp = (x); \
    assert(sizeof(x) == sda_sz(s)); \
    _sda_set((s), (i), &tmp, sizeof(tmp)); \
    })

#if 0
size_t _sda_index(sda s, const void *x, size_t len);
void sda_slice(sda s, int start, int end);
int sda_cmp(const sda s1, const sda s2);
#endif //0

/******* Lower level methods for operating on sda's *******/

sda sda_prealloc(sda s, size_t addlen);
sda sda_compact(sda s);
size_t sda_total_size(sda s);
void *sda_total_ptr(sda s);

/* Don't call these directly */

sda _sda_new_sz(const void *init, size_t init_sz, size_t type_sz);
void _sda_raii_free(void *s);
static inline void _sda_set(sda s, size_t i, const void *t, size_t size) {
    unsigned char flags = sda_flags(s);
    switch(flags&SDA_HTYPE_MASK) {
        case SDA_HTYPE_SM: {
            SDA_HDR_VAR(SM,s);
            size_t start = i*sh->sz;
            //make sure we don't overrun len
            if((start + size) <= (sh->len*sh->sz)) {
                memcpy((((char *)s) + start), t, size);
            }
            break;
        }
        case SDA_HTYPE_MD: {
            SDA_HDR_VAR(MD,s);
            size_t start = i*sh->sz;
            //make sure we don't overrun len
            if((start + size) <= (sh->len*sh->sz)) {
                memcpy((((char *)s) + start), t, size);
            }
            break;
        }
        case SDA_HTYPE_LG: {
            SDA_HDR_VAR(LG,s);
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
#define _sda_ptr_at(s, i) (((char *)s) + (sda_sz(s)*(i)))

/**
 * Sets len directly (no re-allocation!)
 */
static inline void _sda_set_len(sda s, size_t newlen) {
    unsigned char flags = sda_flags(s);
    switch(flags&SDA_HTYPE_MASK) {
        case SDA_HTYPE_SM:
            SDA_HDR(SM,s)->len = newlen;
            break;
        case SDA_HTYPE_MD:
            SDA_HDR(MD,s)->len = newlen;
            break;
        case SDA_HTYPE_LG:
            SDA_HDR(LG,s)->len = newlen;
            break;
    }
}

/**
 * Sets alloc directly (no re-allocation!)
 */
static inline void _sda_set_alloc(sda s, size_t newsz) {
    unsigned char flags = sda_flags(s);
    switch(flags&SDA_HTYPE_MASK) {
        case SDA_HTYPE_SM:
            SDA_HDR(SM,s)->alloc = newsz;
            break;
        case SDA_HTYPE_MD:
            SDA_HDR(MD,s)->alloc = newsz;
            break;
        case SDA_HTYPE_LG:
            SDA_HDR(LG,s)->alloc = newsz;
            break;
    }
}

/** Sets flags */
static inline void _sda_set_flags(sda s, unsigned char flags) {
    ((unsigned char*)s)[-1] = flags;
}

/**
 * Sets item size
 */
static inline void _sda_set_sz(sda s, uint8_t newsz) {
    unsigned char flags = sda_flags(s);
    switch(flags&SDA_HTYPE_MASK) {
        case SDA_HTYPE_SM:
            SDA_HDR(SM,s)->sz = newsz;
            break;
        case SDA_HTYPE_MD:
            SDA_HDR(MD,s)->sz = newsz;
            break;
        case SDA_HTYPE_LG:
            SDA_HDR(LG,s)->sz = newsz;
            break;
    }
}

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


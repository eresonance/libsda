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


#include <stdlib.h>
//#include <ctype.h>
#include <assert.h>
//#include <limits.h>
#include "sda.h"
#include "sdaalloc.h"

static inline size_t sdaHdrSize(char type) {
    switch(type&SDA_TYPE_MASK) {
        case SDA_TYPE_8:
            return sizeof(SDA_HDR_TYPE(8));
        case SDA_TYPE_16:
            return sizeof(SDA_HDR_TYPE(16));
        case SDA_TYPE_32:
            return sizeof(SDA_HDR_TYPE(32));
        case SDA_TYPE_64:
            return sizeof(SDA_HDR_TYPE(64));
    }
    return 0;
}

static inline char sdaReqType(size_t req_size) {
    if (req_size < 1<<8)
        return SDA_TYPE_8;
    if (req_size < 1<<16)
        return SDA_TYPE_16;
#if (LONG_MAX == LLONG_MAX)
    if (req_size < 1ll<<32)
        return SDA_TYPE_32;
#endif
    return SDA_TYPE_64;
}


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

void* _sdanewsz(const void *init, size_t init_sz, size_t type_sz) {
    assert(type_sz <= UINT8_MAX);
    
    //ptr to sda header
    void *sh;
    //ptr that we'll return
    char *s;
    char sda_type = sdaReqType(init_sz);
    size_t hdr_sz = sdaHdrSize(sda_type);
    //flags pointer
    unsigned char *fp;
    //converted type_sz
    uint8_t sz = (uint8_t)type_sz;

    //allocate the full sda
    sh = s_malloc(hdr_sz+init_sz);
    if (sh == NULL) return NULL;
    if (!init)
        memset(sh, 0, hdr_sz+init_sz);
    s = (char*)sh+hdr_sz;
    fp = ((unsigned char*)s)-1;
    *fp = sda_type;
    switch(sda_type) {
        case SDA_TYPE_8: {
            SDA_HDR_VAR(8,s);
            uint8_t len = (uint8_t)(init_sz/type_sz);
            sh->len = len;
            sh->alloc = len;
            sh->sz = sz;
            break;
        }
        case SDA_TYPE_16: {
            SDA_HDR_VAR(16,s);
            uint16_t len = (uint16_t)(init_sz/type_sz);
            sh->len = len;
            sh->alloc = len;
            sh->sz = sz;
            break;
        }
        case SDA_TYPE_32: {
            SDA_HDR_VAR(32,s);
            uint32_t len = (uint32_t)(init_sz/type_sz);
            sh->len = len;
            sh->alloc = len;
            sh->sz = sz;
            break;
        }
        case SDA_TYPE_64: {
            SDA_HDR_VAR(64,s);
            uint64_t len = (uint64_t)(init_sz/type_sz);
            sh->len = len;
            sh->alloc = len;
            sh->sz = sz;
            break;
        }
    }
    if (init_sz && init)
        memcpy(s, init, init_sz);
    return s;
}

/** Create an empty (zero length) sda array */
#define sdaempty(s) sdanewsz((s), NULL, 0)

/** Duplicate an sda array. */
#define sdadup(s, src) sdanewsz((s), (src), sdasizeof(src))
//FIXME: Check whether sdasz(src) == sizeof(s)

/* Free an sda array. No operation is performed if 's' is NULL. */
void sdafree(void *s) {
    if (s == NULL) return;
    char *tmp = (char *)s;
    s_free(tmp-sdaHdrSize(tmp[-1]));
}

/* Modify an sda array in-place to make it empty (zero length).
 * However all the existing buffer is not discarded but set as free space
 * so that next append operations will not require allocations up to the
 * number of bytes previously available. */
void sdaclear(void *s) {
    sdasetlen(s, 0);
}

#if 0
/* Enlarge the free space at the end of the sda array so that the caller
 * is sure that after calling this function can overwrite up to add_sz
 * bytes after the end of the array.
 *
 * Note: this does not change the *length* of the sda array as returned
 * by sdalen(), but only the free buffer space we have. */
sda sdaMakeRoomFor(sda s, size_t add_sz) {
    void *sh, *newsh;
    size_t avail = sdaavail(s);
    size_t len, newlen;
    char type;
    char oldtype = s[-1] & SDA_TYPE_MASK;
    size_t hdrlen;

    /* Return ASAP if there is enough space left. */
    if (avail >= add_sz) return s;

    len = sdalen(s);
    sh = (char*)s-sdaHdrSize(oldtype);
    newlen = (len+add_sz);
    if (newlen < SDA_MAX_PREALLOC)
        newlen *= 2;
    else
        newlen += SDA_MAX_PREALLOC;

    type = sdaReqType(newlen);

    hdrlen = sdaHdrSize(type);
    if (oldtype==type) {
        newsh = s_realloc(sh, hdrlen+newlen);
        if (newsh == NULL) return NULL;
        s = (char*)newsh+hdrlen;
    } else {
        /* Since the header size changes, need to move the array forward,
         * and can't use realloc */
        newsh = s_malloc(hdrlen+newlen);
        if (newsh == NULL) return NULL;
        memcpy((char*)newsh+hdrlen, s, len);
        s_free(sh);
        s = (char*)newsh+hdrlen;
        s[-1] = type;
        sdasetlen(s, len);
    }
    sdasetalloc(s, newlen);
    return s;
}

/* Reallocate the sda array so that it has no free space at the end. The
 * contained array remains not altered, but next concatenation operations
 * will require a reallocation.
 *
 * After the call, the passed sda array is no longer valid and all the
 * references must be substituted with the new pointer returned by the call. */
sda sdaRemoveFreeSpace(sda s) {
    void *sh, *newsh;
    char type, oldtype = s[-1] & SDA_TYPE_MASK;
    size_t hdrlen;
    size_t len = sdalen(s);
    sh = (char*)s-sdaHdrSize(oldtype);

    type = sdaReqType(len);
    hdrlen = sdaHdrSize(type);
    if (oldtype==type) {
        newsh = s_realloc(sh, hdrlen+len);
        if (newsh == NULL) return NULL;
        s = (char*)newsh+hdrlen;
    } else {
        newsh = s_malloc(hdrlen+len);
        if (newsh == NULL) return NULL;
        memcpy((char*)newsh+hdrlen, s, len);
        s_free(sh);
        s = (char*)newsh+hdrlen;
        s[-1] = type;
        sdasetlen(s, len);
    }
    sdasetalloc(s, len);
    return s;
}

/* Return the total size of the allocation of the specifed sda array,
 * including:
 * 1) The sda header before the pointer.
 * 2) The array.
 * 3) The free buffer at the end if any.
 * 4) The implicit null term.
 */
size_t sdaAllocSize(sda s) {
    size_t alloc = sdaalloc(s);
    return sdaHdrSize(s[-1])+alloc+1;
}

/* Return the pointer of the actual SDA allocation (normally SDA strings
 * are referenced by the start of the array buffer). */
void *sdaAllocPtr(sda s) {
    return (void*) (s-sdaHdrSize(s[-1]));
}

/* Increment the sda length and decrements the left free space at the
 * end of the array according to 'incr'. Also set the null term
 * in the new end of the array.
 *
 * This function is used in order to fix the array length after the
 * user calls sdaMakeRoomFor(), writes something after the end of
 * the current array, and finally needs to set the new length.
 *
 * Note: it is possible to use a negative increment in order to
 * right-trim the array.
 *
 * Usage example:
 *
 * Using sdaIncrLen() and sdaMakeRoomFor() it is possible to mount the
 * following schema, to cat bytes coming from the kernel to the end of an
 * sda array without copying into an intermediate buffer:
 *
 * oldlen = sdalen(s);
 * s = sdaMakeRoomFor(s, BUFFER_SIZE);
 * nread = read(fd, s+oldlen, BUFFER_SIZE);
 * ... check for nread <= 0 and handle it ...
 * sdaIncrLen(s, nread);
 */
void sdaIncrLen(sda s, int incr) {
    unsigned char flags = s[-1];
    size_t len;
    switch(flags&SDA_TYPE_MASK) {
        case SDA_TYPE_5: {
            unsigned char *fp = ((unsigned char*)s)-1;
            unsigned char oldlen = SDA_TYPE_5_LEN(flags);
            assert((incr > 0 && oldlen+incr < 32) || (incr < 0 && oldlen >= (unsigned int)(-incr)));
            *fp = SDA_TYPE_5 | ((oldlen+incr) << SDA_TYPE_BITS);
            len = oldlen+incr;
            break;
        }
        case SDA_TYPE_8: {
            SDA_HDR_VAR(8,s);
            assert((incr >= 0 && sh->alloc-sh->len >= incr) || (incr < 0 && sh->len >= (unsigned int)(-incr)));
            len = (sh->len += incr);
            break;
        }
        case SDA_TYPE_16: {
            SDA_HDR_VAR(16,s);
            assert((incr >= 0 && sh->alloc-sh->len >= incr) || (incr < 0 && sh->len >= (unsigned int)(-incr)));
            len = (sh->len += incr);
            break;
        }
        case SDA_TYPE_32: {
            SDA_HDR_VAR(32,s);
            assert((incr >= 0 && sh->alloc-sh->len >= (unsigned int)incr) || (incr < 0 && sh->len >= (unsigned int)(-incr)));
            len = (sh->len += incr);
            break;
        }
        case SDA_TYPE_64: {
            SDA_HDR_VAR(64,s);
            assert((incr >= 0 && sh->alloc-sh->len >= (uint64_t)incr) || (incr < 0 && sh->len >= (uint64_t)(-incr)));
            len = (sh->len += incr);
            break;
        }
        default: len = 0; /* Just to avoid compilation warnings. */
    }
    s[len] = '\0';
}

/* Grow the sda to have the specified length. Bytes that were not part of
 * the original length of the sda will be set to zero.
 *
 * if the specified length is smaller than the current length, no operation
 * is performed. */
sda sdagrowzero(sda s, size_t len) {
    size_t curlen = sdalen(s);

    if (len <= curlen) return s;
    s = sdaMakeRoomFor(s,len-curlen);
    if (s == NULL) return NULL;

    /* Make sure added region doesn't contain garbage */
    memset(s+curlen,0,(len-curlen+1)); /* also set trailing \0 byte */
    sdasetlen(s, len);
    return s;
}

/* Append the specified binary-safe array pointed by 't' of 'len' bytes to the
 * end of the specified sda array 's'.
 *
 * After the call, the passed sda array is no longer valid and all the
 * references must be substituted with the new pointer returned by the call. */
sda sdacatlen(sda s, const void *t, size_t len) {
    size_t curlen = sdalen(s);

    s = sdaMakeRoomFor(s,len);
    if (s == NULL) return NULL;
    memcpy(s+curlen, t, len);
    sdasetlen(s, curlen+len);
    s[curlen+len] = '\0';
    return s;
}

/* Append the specified null termianted C array to the sda array 's'.
 *
 * After the call, the passed sda array is no longer valid and all the
 * references must be substituted with the new pointer returned by the call. */
sda sdacat(sda s, const char *t) {
    return sdacatlen(s, t, strlen(t));
}

/* Append the specified sda 't' to the existing sda 's'.
 *
 * After the call, the modified sda array is no longer valid and all the
 * references must be substituted with the new pointer returned by the call. */
sda sdacatsda(sda s, const sda t) {
    return sdacatlen(s, t, sdalen(t));
}

/* Destructively modify the sda array 's' to hold the specified binary
 * safe array pointed by 't' of length 'len' bytes. */
sda sdacpylen(sda s, const char *t, size_t len) {
    if (sdaalloc(s) < len) {
        s = sdaMakeRoomFor(s,len-sdalen(s));
        if (s == NULL) return NULL;
    }
    memcpy(s, t, len);
    s[len] = '\0';
    sdasetlen(s, len);
    return s;
}


/* Helper for sdacatlonglong() doing the actual number -> array
 * conversion. 's' must point to a array with room for at least
 * SDA_LLSTR_SIZE bytes.
 *
 * The function returns the length of the null-terminated array
 * representation stored at 's'. */
#define SDA_LLSTR_SIZE 21
int sdall2str(char *s, long long value) {
    char *p, aux;
    unsigned long long v;
    size_t l;

    /* Generate the array representation, this method produces
     * an reversed array. */
    v = (value < 0) ? -value : value;
    p = s;
    do {
        *p++ = '0'+(v%10);
        v /= 10;
    } while(v);
    if (value < 0) *p++ = '-';

    /* Compute length and add null term. */
    l = p-s;
    *p = '\0';

    /* Reverse the array. */
    p--;
    while(s < p) {
        aux = *s;
        *s = *p;
        *p = aux;
        s++;
        p--;
    }
    return l;
}

/* Identical sdall2str(), but for unsigned long long type. */
int sdaull2str(char *s, unsigned long long v) {
    char *p, aux;
    size_t l;

    /* Generate the array representation, this method produces
     * an reversed array. */
    p = s;
    do {
        *p++ = '0'+(v%10);
        v /= 10;
    } while(v);

    /* Compute length and add null term. */
    l = p-s;
    *p = '\0';

    /* Reverse the array. */
    p--;
    while(s < p) {
        aux = *s;
        *s = *p;
        *p = aux;
        s++;
        p--;
    }
    return l;
}

/* Create an sda array from a long long value. It is much faster than:
 *
 * sdacatprintf(sdaempty(),"%lld\n", value);
 */
sda sdafromlonglong(long long value) {
    char buf[SDA_LLSTR_SIZE];
    int len = sdall2str(buf,value);

    return sdanew(buf,len);
}

/* Like sdacatprintf() but gets va_list instead of being variadic. */
sda sdacatvprintf(sda s, const char *fmt, va_list ap) {
    va_list cpy;
    char staticbuf[1024], *buf = staticbuf, *t;
    size_t buflen = strlen(fmt)*2;

    /* We try to start using a static buffer for speed.
     * If not possible we revert to heap allocation. */
    if (buflen > sizeof(staticbuf)) {
        buf = s_malloc(buflen);
        if (buf == NULL) return NULL;
    } else {
        buflen = sizeof(staticbuf);
    }

    /* Try with buffers two times bigger every time we fail to
     * fit the array in the current buffer size. */
    while(1) {
        buf[buflen-2] = '\0';
        va_copy(cpy,ap);
        vsnprintf(buf, buflen, fmt, cpy);
        va_end(cpy);
        if (buf[buflen-2] != '\0') {
            if (buf != staticbuf) s_free(buf);
            buflen *= 2;
            buf = s_malloc(buflen);
            if (buf == NULL) return NULL;
            continue;
        }
        break;
    }

    /* Finally concat the obtained array to the SDA array and return it. */
    t = sdacat(s, buf);
    if (buf != staticbuf) s_free(buf);
    return t;
}

/* Append to the sda array 's' a array obtained using printf-alike format
 * specifier.
 *
 * After the call, the modified sda array is no longer valid and all the
 * references must be substituted with the new pointer returned by the call.
 *
 * Example:
 *
 * s = sdanew("Sum is: ");
 * s = sdacatprintf(s,"%d+%d = %d",a,b,a+b).
 *
 * Often you need to create a array from scratch with the printf-alike
 * format. When this is the need, just use sdaempty() as the target array:
 *
 * s = sdacatprintf(sdaempty(), "... your format ...", args);
 */
sda sdacatprintf(sda s, const char *fmt, ...) {
    va_list ap;
    char *t;
    va_start(ap, fmt);
    t = sdacatvprintf(s,fmt,ap);
    va_end(ap);
    return t;
}

/* This function is similar to sdacatprintf, but much faster as it does
 * not rely on sprintf() family functions implemented by the libc that
 * are often very slow. Moreover directly handling the sda array as
 * new data is concatenated provides a performance improvement.
 *
 * However this function only handles an incompatible subset of printf-alike
 * format specifiers:
 *
 * %s - C String
 * %S - SDA array
 * %i - signed int
 * %I - 64 bit signed integer (long long, int64_t)
 * %u - unsigned int
 * %U - 64 bit unsigned integer (unsigned long long, uint64_t)
 * %% - Verbatim "%" character.
 */
sda sdacatfmt(sda s, char const *fmt, ...) {
    size_t initlen = sdalen(s);
    const char *f = fmt;
    int i;
    va_list ap;

    va_start(ap,fmt);
    f = fmt;    /* Next format specifier byte to process. */
    i = initlen; /* Position of the next byte to write to dest str. */
    while(*f) {
        char next, *str;
        size_t l;
        long long num;
        unsigned long long unum;

        /* Make sure there is always space for at least 1 char. */
        if (sdaavail(s)==0) {
            s = sdaMakeRoomFor(s,1);
        }

        switch(*f) {
        case '%':
            next = *(f+1);
            f++;
            switch(next) {
            case 's':
            case 'S':
                str = va_arg(ap,char*);
                l = (next == 's') ? strlen(str) : sdalen(str);
                if (sdaavail(s) < l) {
                    s = sdaMakeRoomFor(s,l);
                }
                memcpy(s+i,str,l);
                sdainclen(s,l);
                i += l;
                break;
            case 'i':
            case 'I':
                if (next == 'i')
                    num = va_arg(ap,int);
                else
                    num = va_arg(ap,long long);
                {
                    char buf[SDA_LLSTR_SIZE];
                    l = sdall2str(buf,num);
                    if (sdaavail(s) < l) {
                        s = sdaMakeRoomFor(s,l);
                    }
                    memcpy(s+i,buf,l);
                    sdainclen(s,l);
                    i += l;
                }
                break;
            case 'u':
            case 'U':
                if (next == 'u')
                    unum = va_arg(ap,unsigned int);
                else
                    unum = va_arg(ap,unsigned long long);
                {
                    char buf[SDA_LLSTR_SIZE];
                    l = sdaull2str(buf,unum);
                    if (sdaavail(s) < l) {
                        s = sdaMakeRoomFor(s,l);
                    }
                    memcpy(s+i,buf,l);
                    sdainclen(s,l);
                    i += l;
                }
                break;
            default: /* Handle %% and generally %<unknown>. */
                s[i++] = next;
                sdainclen(s,1);
                break;
            }
            break;
        default:
            s[i++] = *f;
            sdainclen(s,1);
            break;
        }
        f++;
    }
    va_end(ap);

    /* Add null-term */
    s[i] = '\0';
    return s;
}

/* Remove the part of the array from left and from right composed just of
 * contiguous characters found in 'cset', that is a null terminted C array.
 *
 * After the call, the modified sda array is no longer valid and all the
 * references must be substituted with the new pointer returned by the call.
 *
 * Example:
 *
 * s = sdanew("AA...AA.a.aa.aHelloWorld     :::");
 * s = sdatrim(s,"Aa. :");
 * printf("%s\n", s);
 *
 * Output will be just "Hello World".
 */
sda sdatrim(sda s, const char *cset) {
    char *start, *end, *sp, *ep;
    size_t len;

    sp = start = s;
    ep = end = s+sdalen(s)-1;
    while(sp <= end && strchr(cset, *sp)) sp++;
    while(ep > sp && strchr(cset, *ep)) ep--;
    len = (sp > ep) ? 0 : ((ep-sp)+1);
    if (s != sp) memmove(s, sp, len);
    s[len] = '\0';
    sdasetlen(s,len);
    return s;
}

/* Turn the array into a smaller (or equal) array containing only the
 * substring specified by the 'start' and 'end' indexes.
 *
 * start and end can be negative, where -1 means the last character of the
 * array, -2 the penultimate character, and so forth.
 *
 * The interval is inclusive, so the start and end characters will be part
 * of the resulting array.
 *
 * The array is modified in-place.
 *
 * Example:
 *
 * s = sdanew("Hello World");
 * sdarange(s,1,-1); => "ello World"
 */
void sdarange(sda s, int start, int end) {
    size_t newlen, len = sdalen(s);

    if (len == 0) return;
    if (start < 0) {
        start = len+start;
        if (start < 0) start = 0;
    }
    if (end < 0) {
        end = len+end;
        if (end < 0) end = 0;
    }
    newlen = (start > end) ? 0 : (end-start)+1;
    if (newlen != 0) {
        if (start >= (signed)len) {
            newlen = 0;
        } else if (end >= (signed)len) {
            end = len-1;
            newlen = (start > end) ? 0 : (end-start)+1;
        }
    } else {
        start = 0;
    }
    if (start && newlen) memmove(s, s+start, newlen);
    s[newlen] = 0;
    sdasetlen(s,newlen);
}

/* Apply tolower() to every character of the sda array 's'. */
void sdatolower(sda s) {
    int len = sdalen(s), j;

    for (j = 0; j < len; j++) s[j] = tolower(s[j]);
}

/* Apply toupper() to every character of the sda array 's'. */
void sdatoupper(sda s) {
    int len = sdalen(s), j;

    for (j = 0; j < len; j++) s[j] = toupper(s[j]);
}

/* Compare two sda strings s1 and s2 with memcmp().
 *
 * Return value:
 *
 *     positive if s1 > s2.
 *     negative if s1 < s2.
 *     0 if s1 and s2 are exactly the same binary array.
 *
 * If two strings share exactly the same prefix, but one of the two has
 * additional characters, the longer array is considered to be greater than
 * the smaller one. */
int sdacmp(const sda s1, const sda s2) {
    size_t l1, l2, minlen;
    int cmp;

    l1 = sdalen(s1);
    l2 = sdalen(s2);
    minlen = (l1 < l2) ? l1 : l2;
    cmp = memcmp(s1,s2,minlen);
    if (cmp == 0) return l1-l2;
    return cmp;
}

/* Split 's' with separator in 'sep'. An array
 * of sda strings is returned. *count will be set
 * by reference to the number of tokens returned.
 *
 * On out of memory, zero length array, zero length
 * separator, NULL is returned.
 *
 * Note that 'sep' is able to split a array using
 * a multi-character separator. For example
 * sdasplit("foo_-_bar","_-_"); will return two
 * elements "foo" and "bar".
 *
 * This version of the function is binary-safe but
 * requires length arguments. sdasplit() is just the
 * same function but for zero-terminated strings.
 */
sda *sdasplitlen(const char *s, int len, const char *sep, int seplen, int *count) {
    int elements = 0, slots = 5, start = 0, j;
    sda *tokens;

    if (seplen < 1 || len < 0) return NULL;

    tokens = s_malloc(sizeof(sda)*slots);
    if (tokens == NULL) return NULL;

    if (len == 0) {
        *count = 0;
        return tokens;
    }
    for (j = 0; j < (len-(seplen-1)); j++) {
        /* make sure there is room for the next element and the final one */
        if (slots < elements+2) {
            sda *newtokens;

            slots *= 2;
            newtokens = s_realloc(tokens,sizeof(sda)*slots);
            if (newtokens == NULL) goto cleanup;
            tokens = newtokens;
        }
        /* search the separator */
        if ((seplen == 1 && *(s+j) == sep[0]) || (memcmp(s+j,sep,seplen) == 0)) {
            tokens[elements] = sdanew(s+start,j-start);
            if (tokens[elements] == NULL) goto cleanup;
            elements++;
            start = j+seplen;
            j = j+seplen-1; /* skip the separator */
        }
    }
    /* Add the final element. We are sure there is room in the tokens array. */
    tokens[elements] = sdanew(s+start,len-start);
    if (tokens[elements] == NULL) goto cleanup;
    elements++;
    *count = elements;
    return tokens;

cleanup:
    {
        int i;
        for (i = 0; i < elements; i++) sdafree(tokens[i]);
        s_free(tokens);
        *count = 0;
        return NULL;
    }
}

/* Free the result returned by sdasplitlen(), or do nothing if 'tokens' is NULL. */
void sdafreesplitres(sda *tokens, int count) {
    if (!tokens) return;
    while(count--)
        sdafree(tokens[count]);
    s_free(tokens);
}

/* Append to the sda array "s" an escaped array representation where
 * all the non-printable characters (tested with isprint()) are turned into
 * escapes in the form "\n\r\a...." or "\x<hex-number>".
 *
 * After the call, the modified sda array is no longer valid and all the
 * references must be substituted with the new pointer returned by the call. */
sda sdacatrepr(sda s, const char *p, size_t len) {
    s = sdacatlen(s,"\"",1);
    while(len--) {
        switch(*p) {
        case '\\':
        case '"':
            s = sdacatprintf(s,"\\%c",*p);
            break;
        case '\n': s = sdacatlen(s,"\\n",2); break;
        case '\r': s = sdacatlen(s,"\\r",2); break;
        case '\t': s = sdacatlen(s,"\\t",2); break;
        case '\a': s = sdacatlen(s,"\\a",2); break;
        case '\b': s = sdacatlen(s,"\\b",2); break;
        default:
            if (isprint(*p))
                s = sdacatprintf(s,"%c",*p);
            else
                s = sdacatprintf(s,"\\x%02x",(unsigned char)*p);
            break;
        }
        p++;
    }
    return sdacatlen(s,"\"",1);
}

/* Helper function for sdasplitargs() that returns non zero if 'c'
 * is a valid hex digit. */
int is_hex_digit(char c) {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
           (c >= 'A' && c <= 'F');
}

/* Helper function for sdasplitargs() that converts a hex digit into an
 * integer from 0 to 15 */
int hex_digit_to_int(char c) {
    switch(c) {
    case '0': return 0;
    case '1': return 1;
    case '2': return 2;
    case '3': return 3;
    case '4': return 4;
    case '5': return 5;
    case '6': return 6;
    case '7': return 7;
    case '8': return 8;
    case '9': return 9;
    case 'a': case 'A': return 10;
    case 'b': case 'B': return 11;
    case 'c': case 'C': return 12;
    case 'd': case 'D': return 13;
    case 'e': case 'E': return 14;
    case 'f': case 'F': return 15;
    default: return 0;
    }
}

/* Split a line into arguments, where every argument can be in the
 * following programming-language REPL-alike form:
 *
 * foo bar "newline are supported\n" and "\xff\x00otherstuff"
 *
 * The number of arguments is stored into *argc, and an array
 * of sda is returned.
 *
 * The caller should free the resulting array of sda strings with
 * sdafreesplitres().
 *
 * Note that sdacatrepr() is able to convert back a array into
 * a quoted array in the same format sdasplitargs() is able to parse.
 *
 * The function returns the allocated tokens on success, even when the
 * input array is empty, or NULL if the input contains unbalanced
 * quotes or closed quotes followed by non space characters
 * as in: "foo"bar or "foo'
 */
sda *sdasplitargs(const char *line, int *argc) {
    const char *p = line;
    char *current = NULL;
    char **vector = NULL;

    *argc = 0;
    while(1) {
        /* skip blanks */
        while(*p && isspace(*p)) p++;
        if (*p) {
            /* get a token */
            int inq=0;  /* set to 1 if we are in "quotes" */
            int insq=0; /* set to 1 if we are in 'single quotes' */
            int done=0;

            if (current == NULL) current = sdaempty();
            while(!done) {
                if (inq) {
                    if (*p == '\\' && *(p+1) == 'x' &&
                                             is_hex_digit(*(p+2)) &&
                                             is_hex_digit(*(p+3)))
                    {
                        unsigned char byte;

                        byte = (hex_digit_to_int(*(p+2))*16)+
                                hex_digit_to_int(*(p+3));
                        current = sdacatlen(current,(char*)&byte,1);
                        p += 3;
                    } else if (*p == '\\' && *(p+1)) {
                        char c;

                        p++;
                        switch(*p) {
                        case 'n': c = '\n'; break;
                        case 'r': c = '\r'; break;
                        case 't': c = '\t'; break;
                        case 'b': c = '\b'; break;
                        case 'a': c = '\a'; break;
                        default: c = *p; break;
                        }
                        current = sdacatlen(current,&c,1);
                    } else if (*p == '"') {
                        /* closing quote must be followed by a space or
                         * nothing at all. */
                        if (*(p+1) && !isspace(*(p+1))) goto err;
                        done=1;
                    } else if (!*p) {
                        /* unterminated quotes */
                        goto err;
                    } else {
                        current = sdacatlen(current,p,1);
                    }
                } else if (insq) {
                    if (*p == '\\' && *(p+1) == '\'') {
                        p++;
                        current = sdacatlen(current,"'",1);
                    } else if (*p == '\'') {
                        /* closing quote must be followed by a space or
                         * nothing at all. */
                        if (*(p+1) && !isspace(*(p+1))) goto err;
                        done=1;
                    } else if (!*p) {
                        /* unterminated quotes */
                        goto err;
                    } else {
                        current = sdacatlen(current,p,1);
                    }
                } else {
                    switch(*p) {
                    case ' ':
                    case '\n':
                    case '\r':
                    case '\t':
                    case '\0':
                        done=1;
                        break;
                    case '"':
                        inq=1;
                        break;
                    case '\'':
                        insq=1;
                        break;
                    default:
                        current = sdacatlen(current,p,1);
                        break;
                    }
                }
                if (*p) p++;
            }
            /* add the token to the vector */
            vector = s_realloc(vector,((*argc)+1)*sizeof(char*));
            vector[*argc] = current;
            (*argc)++;
            current = NULL;
        } else {
            /* Even on empty input array return something not NULL. */
            if (vector == NULL) vector = s_malloc(sizeof(void*));
            return vector;
        }
    }

err:
    while((*argc)--)
        sdafree(vector[*argc]);
    s_free(vector);
    if (current) sdafree(current);
    *argc = 0;
    return NULL;
}

/* Modify the array substituting all the occurrences of the set of
 * characters specified in the 'from' array to the corresponding character
 * in the 'to' array.
 *
 * For instance: sdamapchars(mystring, "ho", "01", 2)
 * will have the effect of turning the array "hello" into "0ell1".
 *
 * The function returns the sda array pointer, that is always the same
 * as the input pointer since no resize is needed. */
sda sdamapchars(sda s, const char *from, const char *to, size_t setlen) {
    size_t j, i, l = sdalen(s);

    for (j = 0; j < l; j++) {
        for (i = 0; i < setlen; i++) {
            if (s[j] == from[i]) {
                s[j] = to[i];
                break;
            }
        }
    }
    return s;
}

/* Join an array of C strings using the specified separator (also a C array).
 * Returns the result as an sda array. */
sda sdajoin(char **argv, int argc, char *sep) {
    sda join = sdaempty();
    int j;

    for (j = 0; j < argc; j++) {
        join = sdacat(join, argv[j]);
        if (j != argc-1) join = sdacat(join,sep);
    }
    return join;
}

/* Like sdajoin, but joins an array of SDA strings. */
sda sdajoinsda(sda *argv, int argc, const char *sep, size_t seplen) {
    sda join = sdaempty();
    int j;

    for (j = 0; j < argc; j++) {
        join = sdacatsda(join, argv[j]);
        if (j != argc-1) join = sdacatlen(join,sep,seplen);
    }
    return join;
}

/* Wrappers to the allocators used by SDA. Note that SDA will actually
 * just use the macros defined into sdaalloc.h in order to avoid to pay
 * the overhead of function calls. Here we define these wrappers only for
 * the programs SDA is linked to, if they want to touch the SDA internals
 * even if they use a different allocator. */
void *sda_malloc(size_t size) { return s_malloc(size); }
void *sda_realloc(void *ptr, size_t size) { return s_realloc(ptr,size); }
void sda_free(void *ptr) { s_free(ptr); }

#if defined(SDA_TEST_MAIN)
#include <stdio.h>

#endif

#ifdef SDA_TEST_MAIN
int main(void) {
}
#endif

#endif //0

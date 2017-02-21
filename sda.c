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
#include <assert.h>
#include "sda.h"

/******* Private helpper functions *******/

static inline size_t _sda_hdr_size(char type) {
    switch(type&SDA_HTYPE_MASK) {
        case SDA_HTYPE_SM:
            return sizeof(SDA_HDR_TYPE(SM));
        case SDA_HTYPE_MD:
            return sizeof(SDA_HDR_TYPE(MD));
        case SDA_HTYPE_LG:
            return sizeof(SDA_HDR_TYPE(LG));
    }
    return 0;
}

/**
 * HTYPE required to make sure alloc and len are big enough
 */
static inline char _sda_req_htype(size_t req_size, size_t req_len) {
    if((req_size < UINT32_MAX) && (req_len < UINT16_MAX))
        return SDA_HTYPE_SM;
    if((req_size < UINT64_MAX) && (req_len < UINT32_MAX))
        return SDA_HTYPE_MD;
    return SDA_HTYPE_LG;
}


/******* High-level methods for operating on sda's *******/

sda sda_free(sda s) {
    if (s != NULL) _sda_free(sda_total_ptr(s));
    return NULL;
}
/* Just for sda_raii */
void _sda_raii_free(void *s) {
    if(s == NULL) return;
    //convert to sda array
    sda ptr = *((char**)s);
#if defined(SDA_TEST_MAIN)
    printf("_sda_raii_free %p\n", ptr);
#endif
    sda_free(ptr);
}

sda sda_clear(sda s) {
    _sda_set_len(s, 0);
    //not strictly necessary
    return s;
}


/* Grow/shrink the sda to have the specified length, reallocating more room if necessary.
 * If increasing the length, this will zero the new room up to len.
 */
sda sda_resize(sda s, size_t len) {
    size_t curlen = sda_len(s);
    uint8_t sz;
    size_t grow_sz;

    //if we're already the right size
    if(len == curlen) {
        return s;
    }
    
    //if shrinking
    if (len < curlen) {
        _sda_set_len(s, len);
        return s;
    }
    
    //grow the array
    sz = sda_sz(s);
    grow_sz = (len-curlen)*sz;
    s = sda_prealloc(s, grow_sz);
    if (s == NULL) return NULL;
    
    //set the new grow_sz bytes to 0
    memset(((char*)s)+curlen*sz, 0, grow_sz);
    _sda_set_len(s, len);
    return s;
}


/* Append the specified array pointed by 't' of 'size' bytes to the
 * end of the specified sda array 's', extending s if needed.
 *
 * After the call, the passed sda array is no longer valid and all the
 * references must be substituted with the new pointer returned by the call.
 */
sda sda_cat(sda s, const void *t, size_t size) {
    return sda_cpy(s, sda_len(s), t, size);
}

/* Append the specified sda array 't' to the existing sda array 's'.
 *
 * After the call, the modified sda array is no longer valid and all the
 * references must be substituted with the new pointer returned by the call.
 */
sda sda_extend(sda s, const sda t) {
    return sda_cpy(s, sda_len(s), t, sda_size(t));
}

/* Modify the sda array 's' at index i to hold the specified array pointed by 't' of 'size' bytes.
 */
sda sda_cpy(sda s, size_t i, const void *t, size_t size) {
    assert(t != NULL);
    uint8_t sz = sda_sz(s);
    size_t len = sda_len(s);
    //offset from s where we will start the copy
    size_t off = i*sz;
    //end of the current array
    size_t end = len*sz;
    
    //make room for t if needed
    if((off+size) > (end)) {
        s = sda_prealloc(s, off+size-end);
        if (s == NULL) return NULL;
    }
    //if i is beyond len, then 0 out the difference from end to off
    if(i > len) {
        memset(((char*)s)+end, 0, off-end);
    }
    //copy t over s
    memcpy(((char*)s)+off, t, size);
    //set the len only if it grew;
    // if t is wholy encompased by s.len then leave it be
    if((off+size) > (end)) {
        _sda_set_len(s, (off+size)/sz);
    }
    return s;
}

/* Destructively modify sda array 's' to hold the sds array 't', shrinking/expanding s if needed.
 */
sda sda_replace(sda s, const sda t) {
    sda ret = sda_cpy(s, 0, t, sda_size(t));
    _sda_set_len(s, sda_len(t));
    return ret;
}

void *sda_pop_ptr(sda s) {
    size_t len = sda_len(s);
    void *ret;
    if(len < 1) return NULL;
    //guaranteed, so use faster unsafe ver
    ret = _sda_ptr_at(s, len-1);
    _sda_set_len(s, len-1);
    return ret;
}


#if 0 //FIXME: Implement?
/* Turn the array into a smaller (or equal) array containing only the
 * subarray specified by the 'start' and 'end' indexes.
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
 * sdachar s = sda_new(s, "Hello World");
 * sda_slice(s,1,-1); => "ello World"
 */
void sda_slice(sda s, int start, int end) {
    size_t newlen
    size_t len = sda_len(s);

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
    _sda_set_len(s,newlen);
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
int sda_cmp(const sda s1, const sda s2) {
    size_t l1, l2, minlen;
    int cmp;

    l1 = sda_len(s1);
    l2 = sda_len(s2);
    minlen = (l1 < l2) ? l1 : l2;
    cmp = memcmp(s1,s2,minlen);
    if (cmp == 0) return l1-l2;
    return cmp;
}


/* Join an array of C strings using the specified separator (also a C array).
 * Returns the result as an sda array. */
sda sdajoin(char **argv, int argc, char *sep) {
    sda join = sda_empty();
    int j;

    for (j = 0; j < argc; j++) {
        join = sda_cat(join, argv[j]);
        if (j != argc-1) join = sda_cat(join,sep);
    }
    return join;
}

/* Like sdajoin, but joins an array of SDA strings. */
sda sdajoinsda(sda *argv, int argc, const char *sep, size_t seplen) {
    sda join = sda_empty();
    int j;

    for (j = 0; j < argc; j++) {
        join = sdacatsda(join, argv[j]);
        if (j != argc-1) join = sdacatlen(join,sep,seplen);
    }
    return join;
}
#endif //0 not implemented

/******* Lower level methods for operating on sda's *******/

/* Enlarge the free space at the end of the sda array so that the caller
 * is sure that after calling this function can overwrite up to add_sz
 * elements after the end of the array.
 *
 * Note: this does not change the *length* of the sda array as returned
 * by sda_len(), but only the free buffer space we have. */
sda sda_prealloc(sda s, size_t add_sz) {
    struct sda_hdr_uni shadow;
    //get all of the members
    sda_hdr(s, &shadow);
    
    //it's most likely an error if add_sz isn't evenly divisible by shadow.sz
    assert(add_sz%shadow.sz == 0);
    
    void *sh, *newsh;
    size_t buf_sz = shadow.len*shadow.sz;
    size_t avail_sz = shadow.alloc - buf_sz;
    char type;
    unsigned char oldtype;
    size_t new_sz;
    size_t hdr_sz;

    // Return ASAP if there is enough space left.
    if (avail_sz >= add_sz) return s;
    
    // Determine how much we need to allocate
    new_sz = buf_sz + add_sz;
    if(new_sz < SDA_MAX_PREALLOC) {
        //add a bit more room
        new_sz *= 2;
    }
    else {
        //only add up to SDA_MAX_PREALLOC extra bytes
        new_sz += SDA_MAX_PREALLOC;
    }
    
    //make sure we can address all the new alloc space
    type = _sda_req_htype(new_sz, new_sz/shadow.sz);
    oldtype = shadow.flags & SDA_HTYPE_MASK;
    sh = ((char*)s)-_sda_hdr_size(oldtype);
    hdr_sz = _sda_hdr_size(type);
    if (oldtype==type) {
        //type is still big enough to hold the new allocated mem
        newsh = _sda_realloc(sh, hdr_sz+new_sz);
        if (newsh == NULL) {
            //serious error...
            _sda_free(sh);
            return NULL;
        }
        s = ((char*)newsh)+hdr_sz;
    } else {
        /* Since the header size changes, need to move the array forward,
         * and can't use realloc */
        newsh = _sda_malloc(hdr_sz+new_sz);
        if (newsh == NULL) {
            //serious error
            _sda_free(sh);
            return NULL;
        }
        //can't be too careful about that extra padding
        memset(newsh, 0, hdr_sz);
        //copy the old array into the new one
        memcpy(((char*)newsh)+hdr_sz, s, buf_sz);
        _sda_free(sh);
        sh = NULL;
        s = ((char*)newsh)+hdr_sz;
        //len stays the same
        _sda_set_flags(s, type);
        _sda_set_len(s, shadow.len);
        _sda_set_sz(s, shadow.sz);
    }
    _sda_set_alloc(s, new_sz);
    return s;
}

/* Reallocate the sda array so that it has no free space at the end. The
 * contained array remains not altered, but next concatenation operations
 * will require a reallocation.
 *
 * After the call, the passed sda array is no longer valid and all the
 * references must be substituted with the new pointer returned by the call. */
sda sda_compact(sda s) {
    struct sda_hdr_uni shadow;
    //get all of the members
    sda_hdr(s, &shadow);
    
    void *sh, *newsh;
    size_t buf_sz = shadow.len*shadow.sz; //new_sz
    char type;
    unsigned char oldtype;
    size_t hdr_sz;

    type = _sda_req_htype(buf_sz, shadow.len);
    oldtype = shadow.flags & SDA_HTYPE_MASK;
    sh = ((char*)s)-_sda_hdr_size(oldtype);
    hdr_sz = _sda_hdr_size(type);
    if (oldtype==type) {
        newsh = _sda_realloc(sh, hdr_sz+buf_sz);
        if (newsh == NULL) {
            _sda_free(sh);
            return NULL;
        }
        s = ((char*)newsh)+hdr_sz;
    } else {
        newsh = _sda_malloc(hdr_sz+buf_sz);
        if (newsh == NULL) {
            _sda_free(sh);
            return NULL;
        }
        memset(newsh, 0, hdr_sz);
        memcpy(((char*)newsh)+hdr_sz, s, buf_sz);
        _sda_free(sh);
        sh = NULL;
        s = ((char*)newsh)+hdr_sz;
        _sda_set_flags(s, type);
        _sda_set_len(s, shadow.len);
        _sda_set_sz(s, shadow.sz);
    }
    _sda_set_alloc(s, buf_sz);
    return s;
}

/* Return the total size of the allocation of the specifed sda array,
 * including:
 * 1) The sda header before the pointer.
 * 2) The array.
 * 3) The free buffer at the end if any.
 */
size_t sda_total_size(sda s) {
    return _sda_hdr_size(sda_flags(s))+sda_alloc(s);
}

/* Return the pointer of the actual SDA allocation (normally SDA arrays
 * are referenced by the start of the array buffer). */
void *sda_total_ptr(sda s) {
    return (void*)(((char *)s)-_sda_hdr_size(sda_flags(s)));
}

sda _sda_new_sz(const void *init, size_t init_sz, size_t type_sz) {
    //can't use crazy large types
    assert(type_sz <= UINT8_MAX);
    //must allocate an even number of elements
    assert(init_sz%type_sz == 0);
    
    //ptr to sda header
    void *sh;
    //ptr that we'll return
    char *s;
    //flags pointer
    unsigned char *fp;
    //converted type_sz
    uint8_t sz = (uint8_t)type_sz;
    //len of new array
    size_t len = init_sz/sz;
    //how many bytes can we hold?
    char sda_type = _sda_req_htype(init_sz, len);
    size_t hdr_sz = _sda_hdr_size(sda_type);

    //allocate the full sda
    sh = _sda_malloc(hdr_sz+init_sz);
    if (sh == NULL) return NULL;
    if(init == NULL)
        memset(sh, 0, hdr_sz+init_sz);
    s = (char*)sh+hdr_sz;
    fp = ((unsigned char*)s)-1;
    *fp = sda_type;
    switch(sda_type) {
        case SDA_HTYPE_SM: {
            SDA_HDR_VAR(SM,s);
            sh->len = len;
            sh->alloc = init_sz;
            sh->sz = sz;
            break;
        }
        case SDA_HTYPE_MD: {
            SDA_HDR_VAR(MD,s);
            sh->len = len;
            sh->alloc = init_sz;
            sh->sz = sz;
            break;
        }
        case SDA_HTYPE_LG: {
            SDA_HDR_VAR(LG,s);
            sh->len = len;
            sh->alloc = init_sz;
            sh->sz = sz;
            break;
        }
    }
    if (init_sz && init)
        memcpy(s, init, init_sz);
    return s;
}


/******* Test stuff *******/

#if defined(SDA_TEST_MAIN)
void _sda_raii_free(void *s);


int main(void) {
    int32_t tmp[] = {0, 1, 2, 3, 4, 5};
    size_t tmp_len = sizeof(tmp)/sizeof(*tmp);
    
    sda_raii sdaint s = sda_new(s, tmp);
    assert(sizeof(*s) == sizeof(*tmp));
    assert(sda_len(s) == tmp_len);
    assert(sda_alloc(s) == sda_len(s)*sizeof(*s));
    assert(sda_avail(s) == 0);
    assert(sda_flags(s) == SDA_HTYPE_SM);
    
    for(int i=0; i<sda_len(s); i++) {
        printf("  s[%d] %d\n", i, s[i]);
        assert(s[i] == tmp[i]);
        assert(sda_get(s, i) == s[i]);
        assert(_sda_ptr_at(s, i) == sda_ptr_at(s, i));
    }
    puts("");
    
    sda_raii unsigned int *t = (unsigned int *)sda_dup(s);
    assert((void*)t != (void*)s);
    assert(sda_len(t) == sda_len(s));
    for(int i=0; i<sda_len(t) && i<sda_len(s); i++) {
        printf("  t[%d] %d\n", i, t[i]);
        assert(s[i] == t[i]);
    }
    puts("");
    
    //shrink
    int res1 = 3;
    s = sda_resize(s, sda_len(s)-res1);
    printf("s len=%zu alloc=%zu avail=%zu\n",sda_len(s), sda_alloc(s), sda_avail(s));
    assert(sda_len(s) == tmp_len-res1);
    assert(sda_alloc(s) == tmp_len*sizeof(*s));
    
    //grow, but not greater than alloc size
    int res2 = 2;
    s = sda_resize(s, sda_len(s)+res2);
    printf("s len=%zu alloc=%zu avail=%zu\n",sda_len(s), sda_alloc(s), sda_avail(s));
    assert(sda_len(s) == tmp_len-res1+res2);
    assert(sda_alloc(s) == tmp_len*sizeof(*s));
    
    s = sda_resize(s, sda_len(t)+10);
    printf("s len=%zu alloc=%zu avail=%zu\n",sda_len(s), sda_alloc(s), sda_avail(s));
    assert(sda_len(s) == sda_len(t)+10);
    assert(sda_alloc(s) > sda_len(s)*sizeof(*s));
    assert(sda_avail(s) > 0);
    
    puts("sda_clear");
    sda_clear(s);
    assert(sda_len(s) == 0);
    assert(sda_alloc(s) != 0);
    
    s = sda_compact(s);
    assert(sda_len(s) == 0);
    assert(sda_alloc(s) == 0);
    assert(sda_total_size(s) == _sda_hdr_size(sda_flags(s)));
    
    //don't call free when using sda_raii, although it doesn't hurt to as long
    //as you set s to NULL!
    s = sda_free(s);
    puts("done with s");
    
    //t is unsigned int
    unsigned int tmp1 = sda_get(t, 2);
    assert(tmp1 == t[2]);
    assert(sizeof(sda_get(t, 1)) == sizeof(*t));
    
    sda_set(t, 2, UINT_MAX);
    assert(t[2] == UINT_MAX);
    for(int i=0; i<sda_len(t); i++) {
        printf("  t[%d] %u\n", i, t[i]);
    }
    printf("illegal: t[%zu] = %u\n", sda_len(t)+1, sda_get(t, sda_len(t)+1));
    
    for(int i=0; i<sda_len(t); i++) {
        int j = sda_len(t)-i;
        printf("set t[%d] = %u\n", i, j);
        sda_set(t, i, j);
    }
    for(int i=0; i<sda_len(t); i++) {
        printf("  t[%d] %u\n", i, t[i]);
    }
    //illegal
    sda_set(t, sda_len(t), 1234);
    
    //play around with cat/cpy/append
    sda_raii sdaint u = sda_empty(u);
    assert(sda_len(u) == 0);
    assert(sda_avail(u) == 0);
    tmp1 = 32;
    for(int i=12; i< 12+tmp1; i+=2) {
        u = sda_append(u, i);
    }
    assert(sda_len(u) == tmp1/2);
    assert(sda_alloc(u) >= sda_len(u)*sizeof(*u));
    
    for(int i=0; i<sda_len(u); i++) {
        assert(u[i] == sda_get(u, i));
        printf("  u[%d] %d == %d\n", i, u[i], sda_get(u, i));
    }
    printf("u len=%zu alloc=%zu avail=%zu\n",sda_len(u), sda_alloc(u), sda_avail(u));
    
   
    u = sda_extend(u, t);
    assert(sda_len(u) == tmp1/2+sda_len(t));
    printf("u len=%zu alloc=%zu avail=%zu\n",sda_len(u), sda_alloc(u), sda_avail(u));
    
    u = sda_cat(u, tmp, sizeof(tmp));
    printf("u len=%zu alloc=%zu avail=%zu\n",sda_len(u), sda_alloc(u), sda_avail(u));
    for(int i=0; i<sda_len(u); i++) {
        assert(u[i] == sda_get(u, i));
        printf("  u[%d] %d == %d\n", i, u[i], sda_get(u, i));
    }
    
    u = sda_replace(u, t);
    assert(sda_len(u) == sda_len(t));
    assert(sda_alloc(u) > sda_alloc(t));
    printf("u len=%zu alloc=%zu avail=%zu\n",sda_len(u), sda_alloc(u), sda_avail(u));
    
    //this should work
    u = sda_extend(u, u);
    for(int i=0; i<sda_len(u); i++) {
        assert(u[i] == sda_get(u, i));
        printf("  u[%d] %d == %d\n", i, u[i], sda_get(u, i));
    }
    //and so should this
    int *ptr = 0;
    while((ptr = sda_pop_ptr(u)) != NULL) {
        int x = *ptr;
        printf("pop u x=%d\n", x);
    }
    assert(sda_len(u) == 0);
    
    //push the len to make it use the medium-width version
    //to push the len into a new type sda_hdr_MD
    size_t huge_sz = UINT16_MAX+1;
    uint8_t *huge   = malloc(huge_sz);
    sda_raii uint8_t *v = sda_new_sz(v, huge, huge_sz);
    printf("v len=%zu alloc=%zu avail=%zu\n",sda_len(v), sda_alloc(v), sda_avail(v));
    assert((sda_flags(v)&SDA_HTYPE_MASK) == SDA_HTYPE_MD);
    assert(sda_len(v) == huge_sz);
    huge[UINT16_MAX-74] = 0xff;
    v[UINT16_MAX-74] = 12;
    assert(sda_get(v, UINT16_MAX-74) == 12);
    
#if 0 //doesn't work well with drmemory
    //push the len again!
    free(huge);
    printf("%zu %zu %llu\n", sizeof(size_t), SIZE_MAX, UINT32_MAX+10LL);
    huge_sz = UINT32_MAX+10LL;
    printf("%zu %zu\n", sizeof(huge_sz), huge_sz);
    huge = malloc(huge_sz);
    assert(huge != NULL);
    huge[UINT32_MAX] = 12;
    
    v = sda_cpy(v, 0, huge, huge_sz);
    assert(v != NULL);
    printf("v len=%zu alloc=%zu avail=%zu\n",sda_len(v), sda_alloc(v), sda_avail(v));
    assert((sda_flags(v)&SDA_HTYPE_MASK) == SDA_HTYPE_LG);
    assert(sda_len(v) == huge_sz);
    assert(sda_get(v, UINT32_MAX) == 12);
    huge[UINT32_MAX] = 0xff;
    assert(sda_get(v, UINT32_MAX) == 12);
#endif
    
    puts("done");
    free(huge);
    huge = NULL;
    return 0;
}
#endif


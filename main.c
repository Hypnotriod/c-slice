/* 
 * File:   main.c
 * Author: Hypnotriod
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

typedef struct {
    // Slice data pointer
    const void* p;
    // Slice length
    const int l;
    // Slice capacity
    const int c;
    // Slice item size
    const int s;
    // Is static
    const bool is_final;
} slice_t;

typedef struct {
    void* p;
    int l;
    int c;
    int s;
    bool is_final;
    void* data[];
} __slice_fam_t;

#define SLC_DEFAULT_CAP 16

#define arr_type_size(__ARR__) (sizeof((__ARR__)[0]))
#define arr_size(__ARR__) (sizeof((__ARR__)) / arr_type_size(__ARR__))

// Static slice_t initialization macro from array
#define slc_from_arr(__ARR__) { \
    .l = arr_size(__ARR__), \
    .c = arr_size(__ARR__), \
    .s = arr_type_size(__ARR__), \
    .p = (__ARR__), \
    .is_final = true \
}

// __string_fam_t* memory allocation macro helper
#define __slice_fam_malloc(__LEN__, __SIZE__) malloc(sizeof (__slice_fam_t) + (__LEN__) * (__SIZE__))

// Unwrap slice_t* into type*
#define SLCU(__SLICE_T__, __TYPE__) (*(__TYPE__**)((__SLICE_T__)))

// Unwrap slice_t* and get the typed item by index
#define slci(__SLICE_T__, __TYPE__, __INDEX__) (*(__TYPE__**)((__SLICE_T__)))[(__INDEX__)]

// Unwrap slice_t* and get a pointer to the typed item by index
#define slciptr(__SLICE_T__, __TYPE__, __INDEX__) &(*(__TYPE__**)((__SLICE_T__)))[(__INDEX__)]

// Unwrap slice_t* and get the typed last item
#define slcilast(__SLICE_T__, __TYPE__) (*(__TYPE__**)((__SLICE_T__)))[(__SLICE_T__)->l - 1]

// Dynamic slice_t* helper macro to free and set a null pointer
#define slc_free(__SLICE_T__) { free((__SLICE_T__)); (__SLICE_T__) = NULL; }

slice_t* slc_new(int type_size, int len, int cap) {
    if (cap < 0) cap = 0;
    __slice_fam_t* slice = __slice_fam_malloc(len + cap, type_size);
    if (slice == NULL) return NULL;
    slice->l = len;
    slice->c = len + cap;
    slice->s = type_size;
    slice->p = slice->data;
    slice->is_final = false;
    memset(slice->data, 0, len * type_size);
    return (slice_t*) slice;
}

slice_t* slc_new_from(int type_size, const void* data, int len, int cap) {
    if (cap < 0) cap = 0;
    __slice_fam_t* slice = __slice_fam_malloc(len + cap, type_size);
    if (slice == NULL) return NULL;
    slice->l = len;
    slice->c = len + cap;
    slice->s = type_size;
    slice->p = slice->data;
    slice->is_final = false;
    memcpy(slice->data, data, len * type_size);
    return (slice_t*) slice;
}

slice_t* slc_new_from_slice(const slice_t* slice, int cap) {
    return slc_new_from(slice->s, slice->p, slice->l, cap);
}

#define slc_new_from_arr(__ARR__, __CAP__) slc_new_from( \
    arr_type_size((__ARR__)), \
    (__ARR__), \
    arr_size((__ARR__)), \
    (__CAP__) \
)

slice_t slc_slice(const slice_t* slice, int start, int len) {
    if (start < -slice->l) start = 0;
    else if (start < 0) start = slice->l + start;
    if (len < 0 || start + len > slice->l) len = slice->l - start;
    slice_t newslice = {
        .p = slice->p + start * slice->s,
        .l = len,
        .c = len,
        .s = slice->s,
        .is_final = true,
    };
    return newslice;
}

slice_t slc_slice_of(int type_size, const void* data, int start, int len) {
    slice_t newslice = {
        .p = (void*) data + start * type_size,
        .l = len,
        .c = len,
        .s = type_size,
        .is_final = true,
    };
    return newslice;
}

#define slc_slice_arr(__ARR__, __START__, __LEN__) slc_slice_of( \
    arr_type_size((__ARR__)), \
    (__ARR__), \
    (__START__), \
    (__LEN__) \
)

slice_t* slc_slice_new(const slice_t* slice, int start, int len) {
    slice_t newslice = slc_slice(slice, start, len);
    return slc_new_from(newslice.s, newslice.p, newslice.l, newslice.l);
}

slice_t* slc_append_slice(slice_t* to, const slice_t* what) {
    if (to->l + what->l <= to->c) {
        memcpy(((void*) to->p + to->l * to->s), what->p, what->l * what->s);
        ((__slice_fam_t*) to)->l += what->l;
        return to;
    }
    int cap = (to->l + what->l) * 2;
    __slice_fam_t* slice = __slice_fam_malloc(cap, to->s);
    if (slice == NULL) return NULL;
    memcpy(slice->data, to->p, to->l * to->s);
    memcpy((void*) slice->data + to->l * to->s, what->p, what->l * what->s);
    slice->l = to->l + what->l;
    slice->c = cap;
    slice->s = to->s;
    slice->p = slice->data;
    slice->is_final = false;
    if (!to->is_final) slc_free(to);
    return (slice_t*) slice;
}

slice_t* slc_append_slice_n(slice_t* to, int n, ...) {
    va_list args;
    va_start(args, n);
    for (int i = 0; i < n; i++) {
        const slice_t* what = va_arg(args, const slice_t*);
        to = slc_append_slice(to, what);
        if (to == NULL) return NULL;
    }
    va_end(args);
    return to;
}

slice_t* slc_extend(slice_t* slice, int count) {
    if (slice->l + count <= slice->c) {
        ((__slice_fam_t*) slice)->l += count;
        return slice;
    }
    int cap = (slice->l + count) * 2;
    __slice_fam_t* newslc = __slice_fam_malloc(cap, slice->s);
    if (slice == NULL) return NULL;
    memcpy(newslc->data, slice->p, slice->l * slice->s);
    newslc->l = slice->l + count;
    newslc->c = cap;
    newslc->s = slice->s;
    newslc->p = newslc->data;
    newslc->is_final = false;
    if (!slice->is_final) slc_free(slice);
    return (slice_t*) newslc;
}

slice_t* slc_shrink(slice_t* slice, int count) {
    if (count < 0) return slice;
    int len = slice->l - count;
    if (len < 0) len = 0;
    ((__slice_fam_t*) slice)->l = len;
    return slice;
}

#define slc_append(__SLICE_T__, __TYPE__, __ITEM__) { \
    (__SLICE_T__) = slc_extend((__SLICE_T__), 1); \
    slcilast(__SLICE_T__, __TYPE__) = (__ITEM__); \
}

void slc_extract(slice_t* slice, void* buff, int len) {
    if (len <= 0) return;
    if (len > slice->l) len = slice->l;
    memcpy(buff, slice->p, slice->l * slice->s);
}

void slc_extract_all(slice_t* slice, void* buff) {
    memcpy(buff, slice->p, slice->l * slice->s);
}

// *****************************
// ******* Usage example *******
// *****************************

void print_slice_info(const slice_t* slice) {
    printf("len=%i, cap=%i, size=%i, final=%s: ",
            slice->l, slice->c, slice->s, slice->is_final ? "true" : "false");
}

void print_slice_ints(const slice_t* slice) {
    print_slice_info(slice);
    for (int i = 0; i < slice->l;) {
        printf("%i, ", slci(slice, int, i++));
    }
    printf("\r\n");
}

int main() {
    int ints[] = {1, 2, 3, 4, 5, 6, 7, 8, 9};

    const slice_t slice_ints = slc_from_arr(ints);
    print_slice_ints(&slice_ints);

    const slice_t slice_of_arr = slc_slice_arr(ints, 3, 5);
    print_slice_ints(&slice_of_arr);

    slice_t* slice_ints_new_from = slc_new_from_arr(ints, 10);
    print_slice_ints(slice_ints_new_from);

    slc_free(slice_ints_new_from);
    slice_ints_new_from = slc_new_from_slice(&slice_ints, 10);
    print_slice_ints(slice_ints_new_from);

    slice_t* slice_ints_new = slc_new(sizeof (int), slice_ints.l, 2);
    print_slice_ints(slice_ints_new);
    for (int i = 0; i < slice_ints_new->l; i++) {
        // ((int*) slice_ints_new->p)[i] = ((int*) slice_ints.p)[i];
        slci(slice_ints_new, int, i) = slci(&slice_ints, int, i);
    }
    print_slice_ints(slice_ints_new);

    slice_ints_new = slc_append_slice(slice_ints_new, &slice_ints);
    print_slice_ints(slice_ints_new);

    slc_append(slice_ints_new, int, 55);
    print_slice_ints(slice_ints_new);

    const slice_t slice_of_slice1 = slc_slice(slice_ints_new, 5, -1);
    print_slice_ints(&slice_of_slice1);

    const slice_t slice_of_slice2 = slc_slice(slice_ints_new, -8, -1);
    print_slice_ints(&slice_of_slice2);

    slice_ints_new = slc_append_slice_n(slice_ints_new, 2, &slice_of_slice1, &slice_of_slice2);
    print_slice_ints(slice_ints_new);

    slc_free(slice_ints_new_from);
    slice_ints_new_from = slc_slice_new(slice_ints_new, 3, 5);
    print_slice_ints(slice_ints_new_from);

    slc_free(slice_ints_new_from);
    slc_free(slice_ints_new);

    return (EXIT_SUCCESS);
}

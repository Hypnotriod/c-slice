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
    const bool is_static;
} slice_t;

typedef struct {
    void* p;
    int l;
    int c;
    int s;
    bool is_static;
    unsigned char data[];
} __slice_t;

#define SLC_DEFAULT_CAP 16

// Static slice_t initialization macro from array
#define SLCS(__ARR__) { \
    .l = (sizeof((__ARR__)) / sizeof((__ARR__)[0])), \
    .c = (sizeof((__ARR__)) / sizeof((__ARR__)[0])), \
    .s = sizeof((__ARR__)[0]), \
    .p = (__ARR__), \
    .is_static = true \
}

#define __slice_fam_malloc(__LEN__, __SIZE__) malloc(sizeof (__slice_t) + (__LEN__) * (__SIZE__))

// Unwrap slice_t* into type*
#define SLCU(__SLICE_T__, __TYPE__) (*(__TYPE__**)((__SLICE_T__)))

// Unwrap slice_t* and get the typed item by index
#define slc_item(__SLICE_T__, __TYPE__, __INDEX__) (*(__TYPE__**)((__SLICE_T__)))[(__INDEX__)]

// Unwrap slice_t* and get the typed last item
#define slc_item_last(__SLICE_T__, __TYPE__) (*(__TYPE__**)((__SLICE_T__)))[(__SLICE_T__)->l - 1]

// Unwrap slice_t* and get pointer to the typed item by index
#define slc_item_ptr(__SLICE_T__, __TYPE__, __INDEX__) &(*(__TYPE__**)((__SLICE_T__)))[(__INDEX__)]

// Dynamic slice_t* helper macro to free and set a null pointer
#define slc_free(__SLICE_T__) { free((__SLICE_T__)); (__SLICE_T__) = NULL; }

slice_t* slc_new(int type_size, int len, int cap) {
    if (cap < 0) cap = 0;
    __slice_t* slice = __slice_fam_malloc(len + cap, type_size);
    if (slice == NULL) return NULL;
    slice->l = len;
    slice->c = len + cap;
    slice->s = type_size;
    slice->p = slice->data;
    slice->is_static = false;
    memset(slice->data, 0, len * type_size);
    return (slice_t*) slice;
}

slice_t* slc_new_from(int type_size, const void* data, int len, int cap) {
    if (cap < 0) cap = 0;
    __slice_t* slice = __slice_fam_malloc(len + cap, type_size);
    if (slice == NULL) return NULL;
    slice->l = len;
    slice->c = len + cap;
    slice->s = type_size;
    slice->p = slice->data;
    slice->is_static = false;
    memcpy(slice->data, data, len * type_size);
    return (slice_t*) slice;
}

slice_t* slc_append(slice_t* to, const slice_t* what) {
    if (to->l + what->l <= to->c) {
        memcpy(&((unsigned char*) to->p)[to->l * to->s], what->p, what->l * what->s);
        ((__slice_t*) to)->l += what->l;
        return to;
    }
    int cap = (to->l + what->l) * 2;
    __slice_t* slice = __slice_fam_malloc(cap, to->s);
    if (slice == NULL) return NULL;
    memcpy(slice->data, to->p, to->l * to->s);
    memcpy(&slice->data[to->l * to->s], what->p, what->l * what->s);
    slice->l = to->l + what->l;
    slice->c = cap;
    slice->s = to->s;
    slice->p = slice->data;
    slice->is_static = false;
    if (!to->is_static) slc_free(to);
    return (slice_t*) slice;
}

slice_t* slc_append_n(slice_t* to, int n, ...) {
    va_list args;
    va_start(args, n);
    for (int i = 0; i < n; i++) {
        const slice_t* what = va_arg(args, slice_t*);
        if (to->l + what->l <= to->c) {
            memcpy(&((unsigned char*) to->p)[to->l * to->s], what->p, what->l * what->s);
            ((__slice_t*) to)->l += what->l;
            continue;
        }
        int cap = (to->l + what->l) * 2;
        __slice_t* slice = __slice_fam_malloc(cap, to->s);
        if (slice == NULL) return NULL;
        memcpy(slice->data, to->p, to->l * to->s);
        memcpy(&slice->data[to->l * to->s], what->p, what->l * what->s);
        slice->l = to->l + what->l;
        slice->c = cap;
        slice->s = to->s;
        slice->p = slice->data;
        slice->is_static = false;
        if (!to->is_static) slc_free(to);
        to = (slice_t*) slice;
    }
    va_end(args);
    return to;
}

slice_t* slc_extend(slice_t* slice, int count) {
    if (slice->l + count <= slice->c) {
        ((__slice_t*) slice)->l += count;
        return slice;
    }
    int cap = slice->l * 2;
    __slice_t* newslc = __slice_fam_malloc(cap, slice->s);
    if (slice == NULL) return NULL;
    memcpy(newslc->data, slice->p, slice->l * slice->s);
    newslc->l = slice->l + count;
    newslc->c = cap;
    newslc->s = slice->s;
    newslc->p = newslc->data;
    newslc->is_static = false;
    if (!slice->is_static) slc_free(slice);
    return (slice_t*) newslc;
}

slice_t slc_slice(const slice_t* slice, int start, int len) {
    if (start < -slice->l) start = 0;
    else if (start < 0) start = slice->l + start;
    if (len < 0 || start + len > slice->l) len = slice->l - start;
    slice_t newslice = {
        .p = slice->p + start * slice->s,
        .l = len,
        .c = len,
        .s = slice->s,
        .is_static = true,
    };
    return newslice;
}

#define slc_add(__SLICE_T__, __TYPE__, __ITEM__) { \
    (__SLICE_T__) = slc_extend((__SLICE_T__), 1); \
    slc_item_last(__SLICE_T__, __TYPE__) = (__ITEM__); \
}

// *****************************
// ******* Usage example *******
// *****************************

void print_slice_info(const slice_t* slice) {
    printf("len=%i, cap=%i, size=%i\r\n", slice->l, slice->c, slice->s);
}

void print_slice_ints(const slice_t* slice) {
    print_slice_info(slice);
    for (int i = 0; i < slice->l;) {
        printf("%i, ", slc_item(slice, int, i++));
    }
    printf("\r\n");
}

int main() {
    int ints[] = {1, 2, 3, 4, 5, 6, 7, 8, 9};

    const slice_t slice_ints = SLCS(ints);
    print_slice_ints(&slice_ints);

    slice_t* slice_ints_new_from = slc_new_from(sizeof (int), ints, sizeof (ints) / sizeof (ints[0]), 10);
    print_slice_ints(slice_ints_new_from);

    slice_t* slice_ints_new = slc_new(sizeof (int), slice_ints.l, 2);
    print_slice_ints(slice_ints_new);
    for (int i = 0; i < slice_ints_new->l; i++) {
        slc_item(slice_ints_new, int, i) = slc_item(&slice_ints, int, i);
    }
    print_slice_ints(slice_ints_new);

    slice_ints_new = slc_append(slice_ints_new, &slice_ints);
    print_slice_ints(slice_ints_new);

    slc_add(slice_ints_new, int, 55);
    print_slice_ints(slice_ints_new);

    slice_t slice_of_slice1 = slc_slice(slice_ints_new, 5, -1);
    print_slice_ints(&slice_of_slice1);

    slice_t slice_of_slice2 = slc_slice(slice_ints_new, -8, -1);
    print_slice_ints(&slice_of_slice2);

    slice_ints_new = slc_append_n(slice_ints_new, 2, &slice_of_slice1, &slice_of_slice2);
    print_slice_ints(slice_ints_new);

    slc_free(slice_ints_new_from);
    slc_free(slice_ints_new);

    return (EXIT_SUCCESS);
}

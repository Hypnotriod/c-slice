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

/**
 * The main slice structure
 */
typedef struct {
    // Slice data pointer
    const void* p;
    // Slice length
    const int l;
    // Slice capacity
    const int c;
    // Slice item size
    const int s;
    // Was not heap allocated
    const bool is_final;
} slice_t;

/**
 * Flexible array members based slice structure
 */
typedef struct {
    void* p;
    int l;
    int c;
    int s;
    bool is_final;
    void* data[];
} __slice_fam_t;

// Array manipulation helpers macro
#define __arr_type_size(__ARR__) (sizeof((__ARR__)[0]))
#define __arr_size(__ARR__) (sizeof((__ARR__)) / __arr_type_size(__ARR__))

// Static slice_t initialization macro from array
#define slc_from_arr(__ARR__) { \
    .l = __arr_size(__ARR__), \
    .c = __arr_size(__ARR__), \
    .s = __arr_type_size(__ARR__), \
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
#define slc_free(__SLICE_T__) if (!(__SLICE_T__)->is_final) { free((__SLICE_T__)); (__SLICE_T__) = NULL; }

/**
 * Create new slice_t* with the given length and additional capacity
 * All The allocated memory will have zero values
 * @param type_size - type size in bytes
 * @param len - number of empty elements
 * @param cap - additional capacity
 * @return new slice_t*
 */
slice_t* slc_new(int type_size, int len, int cap) {
    if (len < 0) len = 0;
    if (cap < 0) cap = 0;
    __slice_fam_t* slice = __slice_fam_malloc(len + cap, type_size);
    if (slice == NULL) return NULL;
    slice->l = len;
    slice->c = len + cap;
    slice->s = type_size;
    slice->p = slice->data;
    slice->is_final = false;
    memset(slice->data, 0, (len + cap) * type_size);
    return (slice_t*) slice;
}

/**
 * Create new slice_t* from the data pointer with the given length and additional capacity
 * Copy all the data to the slice_t*
 * Fill rest with zero
 * @param type_size - type size in bytes
 * @param data - data pointer
 * @param len - number of empty elements
 * @param cap - additional capacity
 * @return new slice_t*
 */
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
    memset((void*) slice->data + (len * type_size), 0, cap * type_size);
    return (slice_t*) slice;
}

/**
 * Create new slice_t* copy from slice_t* with an additional capacity
 * @param slice - slice_t* to copy from
 * @param cap - additional capacity
 * @return new slice_t*
 */
slice_t* slc_new_from_slice(const slice_t* slice, int cap) {
    return slc_new_from(slice->s, slice->p, slice->l, cap);
}

/**
 * Macro helper to create a new slice_t* with the copy of the given array and additional capacity
 * Do not use with the array pointer!
 * @param __ARR__ - array symbol
 * @param __CAP__ - additional capacity
 * @return new slice_t*
 */
#define slc_new_from_arr(__ARR__, __CAP__) slc_new_from( \
    __arr_type_size((__ARR__)), \
    (__ARR__), \
    __arr_size((__ARR__)), \
    (__CAP__) \
)

/**
 * Slice a slice_t* into a final slice_t
 * Data pointer will point at the original memory address!
 * @param slice - slice_t* to slice
 * @param start - start index. Negative value represents before the end index
 * @param len - number of items to slice. Any negative value means up to the remaining length
 * @return final slice_t
 */
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

/**
 * Slice a data by the pointer into a final slice_t
 * Data pointer will point at the original memory address!
 * @param type_size - type size in bytes
 * @param data - data pointer
 * @param start - start index
 * @param len - number of items to slice
 * @return final slice_t
 */
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

/**
 * Macro helper to slice an array into a final slice_t
 * Data pointer will point at the original memory address!
 * Do not use with the array pointer!
 * @param __ARR__ - array symbol
 * @param __START__ - start index
 * @param __LEN__ - number of items to slice
 * @return final slice_t
 */
#define slc_slice_arr(__ARR__, __START__, __LEN__) slc_slice_of( \
    __arr_type_size((__ARR__)), \
    (__ARR__), \
    (__START__), \
    (__LEN__) \
)

/**
 * Slice a slice_t* into a new slice_t* with the copy of the given slice_t* data
 * @param slice - slice_t* to slice
 * @param start - start index. Negative value represents before the end index
 * @param len - number of items to slice. Any negative value means up to the remaining length
 * @return new slice_t*
 */
slice_t* slc_slice_new(const slice_t* slice, int start, int len) {
    slice_t newslice = slc_slice(slice, start, len);
    return slc_new_from(newslice.s, newslice.p, newslice.l, newslice.l);
}

/**
 * Append slice_t* 'what' to the slice_t* 'to'
 * Will reallocate the memory and free the old pointer if growth occurs
 * Always reassign the slice_t* result to avoid use after free
 * @param to - slice_t* to append to
 * @param what - slice_t* to append
 * @return slice_t* or new slice_t*
 */
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
    memset((void*) slice->data + (to->l + what->l) * to->s, 0, cap / 2 * to->s);
    slice->l = to->l + what->l;
    slice->c = cap;
    slice->s = to->s;
    slice->p = slice->data;
    slice->is_final = false;
    slc_free(to);
    return (slice_t*) slice;
}

/**
 * Append n of slice_t* slices to the slice_t* 'to'
 * Will reallocate the memory and free the old pointer if growth occurs
 * Always reassign the slice_t* result to avoid use after free
 * @param to - slice_t* to append to
 * @param ... - slice_t* to append
 * @return slice_t* or new slice_t*
 */
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

/**
 * Extend the slice_t* length
 * Will reallocate the memory and free the old pointer if growth occurs
 * Always reassign the slice_t* result to avoid use after free
 * @param slice - slice_t* to extend
 * @param count - items count
 * @return slice_t* or new slice_t*
 */
slice_t* slc_extend(slice_t* slice, int count) {
    if (slice->l + count <= slice->c) {
        ((__slice_fam_t*) slice)->l += count;
        return slice;
    }
    int cap = (slice->l + count) * 2;
    __slice_fam_t* newslc = __slice_fam_malloc(cap, slice->s);
    if (slice == NULL) return NULL;
    memcpy(newslc->data, slice->p, slice->l * slice->s);
    memset((void*) newslc->data + slice->l * slice->s, 0, (cap - slice->l) * slice->s);
    newslc->l = slice->l + count;
    newslc->c = cap;
    newslc->s = slice->s;
    newslc->p = newslc->data;
    newslc->is_final = false;
    slc_free(slice);
    return (slice_t*) newslc;
}

/**
 * Shrink the slice_t* length
 * @param slice - slice_t* to shrink
 * @param count - items count
 * @return slice_t*
 */
slice_t* slc_shrink(slice_t* slice, int count) {
    if (count < 0) return slice;
    int len = slice->l - count;
    if (len < 0) len = 0;
    ((__slice_fam_t*) slice)->l = len;
    return slice;
}

/**
 * Macro helper to append new item to the slice_t*
 * Use only with the non final slice!
 * Will reassign the slice_t* pointer after the extend call
 * @param __SLICE_T__ - slice_t* to append to
 * @param __TYPE__ - type of the item
 * @param __ITEM__ - item to append
 */
#define slc_append(__SLICE_T__, __TYPE__, __ITEM__) { \
    (__SLICE_T__) = slc_extend((__SLICE_T__), 1); \
    slcilast(__SLICE_T__, __TYPE__) = (__ITEM__); \
}

/**
 * Extract number of elements of the slice_t* to the given buffer
 * @param slice - slice_t* to extract data from
 * @param buff - void* to extract data to
 * @param len - number of items to extract
 */
void slc_extract(slice_t* slice, void* buff, int len) {
    if (len <= 0) return;
    if (len > slice->l) len = slice->l;
    memcpy(buff, slice->p, slice->l * slice->s);
}

/**
 * Extract all the elements of the slice_t* to the given buffer
 * @param slice - slice_t* to extract data from
 * @param buff - void* to extract data to
 */
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

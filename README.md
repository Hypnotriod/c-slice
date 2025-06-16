# c-slice

The Go like `slice` library implementation for C, based on the FAM (flexible array member) C99 feature.  

* Main slice structure
```c
typedef struct {
    // Data array pointer
    const void* d;
    // Slice length
    const int l;
    // Slice capacity
    const int c;
    // Slice item size
    const int s;
    // Was not heap allocated
    const bool is_final;
} slice_t;
```

* Underlying FAM slice structure
```c
typedef struct {
    void* d;
    int l;
    int c;
    int s;
    bool is_final;
    void* data[];
} __slice_fam_t;
```

## Examples:
* Declare a final slice on the stack with the `length: 10` and `capacity: 0` from a given array
```c
int ints[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
const slice_t slice = slc_from_arr(ints);
```

* Declare a final slice on the stack with the `length: 5` and `capacity: 0` from a given array, starting from `index: 3`
```c
int ints[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
const slice_t slice = slc_slice_arr(ints, 3, 5);
```

* Create a new (zero initialized, heap allocated) slice with the `length: 10` and `capacity: 15`
```c
slice_t* slice = slc_new(sizeof(int), 10, 5);
```

* Create a new (heap allocated) slice with the `length: 10` and `capacity: 15` from a given array
```c
int ints[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
slice_t* slice = slc_new_from_arr(ints, 5);
```

* Create a new heap allocated slice copy with the `extra capacity: 10`
```c
slice_t* new_slice = slc_new_from_slice(&slice, 10);
```

* Free the new slice
```c
slc_free(slice);
```

* Shrink the slice `length` by 2
```c
slc_shrink(slice, 2);
```

* Extend the slice `length` by 2
```c
slice = slc_extend(slice, 2);
```

* Append an array to the heap allocated slice 
```c
int ints[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
slice = slc_append_arr(slice, ints, slc_arr_size(ints));
```

* Append an item to the slice
```c
slc_append(slice, int, 55);
```

* Append the struct item to the slice
```c
typedef struct {
    int a;
    int b;
} foo_t;

slice_t* foos = slc_new(sizeof (foo_t), 0, 5);
slc_append(foos, foo_t, ((foo_t){.a = 100, .b = 200}));
slc_append(foos, foo_t, ((foo_t){.a = 300, .b = 400}));
```

* Append the slice `slice_b` to the slice `slice_a`
```c
slice_t* slice_a = ... , slice_b = ... ;
slice_a = slc_append_slice(slice_a, slice_b);
```

* Append the `n` slices to the slice
```c
slice_a = slc_append_slice_n(slice_a, 3, slice_b, slice_c, slice_d);
```

* Concat two slices `slice_a` and `slice_b` into the new slice
```c
slice_t* slice = slc_concat(slice_a, slice_b);
```

* Concat the `n` slices into a new slice
```c
slice_t* slice = slc_concat_n(3, slice_a, slice_b, slice_c);
```

* Slice the slice
```c
int ints[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
slice_t* slice = slc_new_from_arr(ints, 0);
const slice_t a = slc_slice(slice, 5, 2);  // {5, 6}
const slice_t b = slc_slice(slice, 5, -1); // {5, 6, 7, 8, 9}
const slice_t c = slc_slice(slice, 5, -3); // {5, 6, 7}
```

* Access a value by the index
```c
typedef struct {
    int a;
    int b;
} foo_t;

slice_t* foos = slc_new(sizeof (foo_t), 2, 0);
slc_at(foos, foo_t, 0).a = 1;
slc_at(foos, foo_t, 0).b = 2;
slc_last(foos, foo_t).a = 3;
slc_last(foos, foo_t).b = 4;
```

* Classic for loop
```c
for (int i = 0; i < slice->l; ++i) {
    printf("%i, ", slc_at(slice, int, i));
}
```

* Iterate over the slice
```c
for (int* item = slc_begin(slice, int); item < slc_end(slice, int); ++item) {
    printf("%i, ", *item);
}
```

* Iterate over the slice using the `for each` macro
```c
slc_for_each(slice, int, item) {
    printf("%i, ", *item);
}
```

# c-slice

C99 proof of concept slice implementation

Some [examples](./main.c) so far:
```c
int ints[] = {1, 2, 3, 4, 5, 6, 7, 8, 9};

const slice_t slice_ints = slc_from_arr(ints);
print_slice_ints("Final slice_t from array", &slice_ints);

const slice_t slice_of_arr = slc_slice_arr(ints, 3, 5);
print_slice_ints("Final slice_t from array slice from 3 with length 5", &slice_of_arr);

slice_t* slice_ints_new_from = slc_new_from_arr(ints, 10);
print_slice_ints("New slice_t* from array slice with extra cap 10", slice_ints_new_from);

slc_shrink(slice_ints_new_from, 2);
print_slice_ints("slice_t* after shrink by 2", slice_ints_new_from);

slice_ints_new_from = slc_append_arr(slice_ints_new_from, ints, __arr_size(ints));
print_slice_ints("slice_t* after array append", slice_ints_new_from);

slc_free(slice_ints_new_from);
slice_ints_new_from = slc_new_from_slice(&slice_ints, 10);
print_slice_ints("New slice_t* from slice of slice_t* with extra cap 10", slice_ints_new_from);

slice_t* slice_ints_new = slc_new(sizeof (int), slice_ints.l, 2);
print_slice_ints("New empty slice_t* with length 9 and extra cap 2", slice_ints_new);
for (int i = 0; i < slice_ints_new->l; i++) {
    // ((int*) slice_ints_new->p)[i] = ((int*) slice_ints.p)[i];
    slc_at(slice_ints_new, int, i) = -slc_at(&slice_ints, int, i);
}
print_slice_ints("slice_t* after data copy with sign inversion", slice_ints_new);

slice_ints_new = slc_append_slice(slice_ints_new, &slice_ints);
print_slice_ints("slice_t* after slice append", slice_ints_new);

// slice_ints_new = slc_extend(slice_ints_new, 1);
// slc_last(slice_ints_new, int) = 55;
slc_append(slice_ints_new, int, 55);
print_slice_ints("slice_t* after append 55", slice_ints_new);

const slice_t slice_of_slice1 = slc_slice(slice_ints_new, 5, -1);
print_slice_ints("Final slice_t from slice_t* from 5 to the full length", &slice_of_slice1);

const slice_t slice_of_slice2 = slc_slice(slice_ints_new, -8, -1);
print_slice_ints("Final slice_t from slice_t* from -8 to the full length", &slice_of_slice2);

slc_free(slice_ints_new_from);
slice_ints_new_from = slc_new(sizeof (int), 0, 12);
slice_ints_new_from = slc_append_slice_n(slice_ints_new_from, 2, &slice_of_slice1, &slice_of_slice2);
print_slice_ints("slice_t* after several appended slices", slice_ints_new_from);

slc_free(slice_ints_new_from);
slice_ints_new_from = slc_slice_new(slice_ints_new, 3, 5);
print_slice_ints("New slice_t* from slice_t* starting from 3 and length 5", slice_ints_new_from);

slice_t* slice_concatenated = slc_concat(&slice_of_slice1, &slice_of_slice2);
slc_append(slice_concatenated, int, 44);
slc_append(slice_concatenated, int, 22);
slc_append(slice_concatenated, int, 11);
print_slice_ints("New slice_t* after two slices concatenated and 44, 22, 11 values added", slice_concatenated);

slc_free(slice_concatenated);
slice_concatenated = slc_concat_n(3, &slice_of_slice1, slice_ints_new_from, &slice_of_slice2);
slc_append(slice_concatenated, int, 33);
slc_append(slice_concatenated, int, 66);
slc_append(slice_concatenated, int, 77);
print_slice_ints("New slice_t* after three slices concatenated and 33, 66, 77 values added", slice_concatenated);

slc_free(slice_ints_new_from);
slc_free(slice_ints_new);
slc_free(slice_concatenated);
```

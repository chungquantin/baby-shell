#include "vect.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define VECT_GROWTH_FACTOR 2
#define DATA_SIZE sizeof(char *)

/** Main data structure for the vector. */
struct vect {
  char **data;           /* Array containing the actual data. */
  unsigned int size;     /* Number of items currently in the vector. */
  unsigned int capacity; /* Maximum number of items the vector can hold before
                            growing. */
};

/** Construct a new empty vector. */
vect_t *vect_new() {
  vect_t *v = (vect_t *)malloc(sizeof(vect_t));
  /* Dynamic allocate memory into (*v.data) using char pointer */
  /* Should be char pointer instead of char because field data is identified as
   * `char**data` which points to the char pointer */
  v->capacity = VECT_GROWTH_FACTOR;
  v->data = (char **)malloc(v->capacity * DATA_SIZE);
  v->size = 0;
  return v;
}

/** Delete the vector, freeing all memory it occupies. */
void vect_delete(vect_t *v) {
  for (int i = 0; i < v->size; i++) {
    free(v->data[i]);
  }
  /* deallocate vector.data */
  free(v->data);
  /* deallocate other fields */
  free(v);
}

/** Get the element at the given index. */
const char *vect_get(vect_t *v, unsigned int idx) {
  assert(v != NULL);
  assert(idx < v->size);
  return v->data[idx];
}

/** Get a copy of the element at the given index. The caller is responsible
 *  *  for freeing the memory occupied by the copy. */
char *vect_get_copy(vect_t *v, unsigned int idx) {
  assert(v != NULL);
  assert(idx < v->size);
  /* Make a copy of a string and then store */
  int str_size = sizeof(char) * (strlen(v->data[idx]) + 1);
  char *tmp = (char *)malloc(str_size);
  strcpy(tmp, v->data[idx]);
  return tmp;
}

/** Set the element at the given index. */
void vect_set(vect_t *v, unsigned int idx, const char *elt) {
  assert(v != NULL);
  assert(idx < v->size);
  /* Make a copy of a string and then store */
  unsigned int size = v->size;
  free(v->data[idx]);
  int str_size = sizeof(char) * (strlen(elt) + 1);
  v->data[idx] = (char *)malloc(str_size);
  strcpy(v->data[idx], elt);
}

/** Add an element to the back of the vector. */
void vect_add(vect_t *v, const char *elt) {
  assert(v != NULL);
  unsigned int size = v->size;
  unsigned int capacity = v->capacity;
  /* vector grows when its size reaches the capacity */
  if (size == capacity) {
    v->capacity *= VECT_GROWTH_FACTOR;
    /* reallocate vector data with a larger size array */
    v->data = (char **)realloc(v->data, v->capacity * DATA_SIZE);
  }
  /* Make a copy of a string and then store */
  int str_size = sizeof(char) * (strlen(elt) + 1);
  v->data[size] = (char *)malloc(str_size);
  strcpy(v->data[size], elt);
  v->size += 1;
}

/** Remove the last element from the vector. */
void vect_remove_last(vect_t *v) {
  assert(v != NULL);
  if (v->size > 0) {
    v->size -= 1;
    free(v->data[v->size]);
    v->data[v->size] = NULL;
  }
}

/** The number of items currently in the vector. */
unsigned int vect_size(vect_t *v) {
  assert(v != NULL);
  return v->size;
}

/** The maximum number of items the vector can hold before it has to grow. */
unsigned int vect_current_capacity(vect_t *v) {
  assert(v != NULL);
  return v->capacity;
}

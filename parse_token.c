#include "parse_token.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// We use parse_token for the sake of being able to call the "tokenizing"
// function inside of our shell
vect_t *parse_token(const char *input) {
  assert(input != NULL);

  char temp[MAX_CMD_LEN + 1];
  vect_t *out = vect_new();
  int len_temp =
      0; // length of the buffer before we reach an "exit case" to add to vector
  char *buffer =
      malloc(sizeof(char)); // single char buffer to iterate thru input

  for (int i = 0; i < strlen(input); i++) {
    strncpy(buffer, &input[i], 1); // copy next index to buffer
    switch (*buffer) {
    case '(': // basic token, we don't need to wait for anything
      if (len_temp > 0) {
        temp[len_temp] = '\0';
        vect_add(out, temp);
        temp[0] = '\0';
        len_temp = 0;
      }
      vect_add(out, "(\0");
      break;

    case ')': // same as above
      if (len_temp > 0) {
        temp[len_temp] = '\0';
        vect_add(out, temp);
        temp[0] = '\0';
        len_temp = 0;
      }
      vect_add(out, ")\0");
      break;

    case '"': // we need to wait for the next '"', so iterate thru input
              // ignoring special cases until we find it
      i++;
      for (int j = 0; i < strlen(input); i++) {
        if (input[i] != '"') {
          temp[len_temp] = input[i];
          len_temp++;
        } else {
          break;
        }
      }
      temp[len_temp] = '\0';
      vect_add(out, temp);
      temp[0] = '\0';
      len_temp = 0;
      break;

    case '|': // basic case
      if (len_temp > 0) {
        temp[len_temp] = '\0';
        vect_add(out, temp);
        temp[0] = '\0';
        len_temp = 0;
      }
      vect_add(out, "|\0");
      break;

    case '<': // basic
      if (len_temp > 0) {
        temp[len_temp] = '\0';
        vect_add(out, temp);
        temp[0] = '\0';
        len_temp = 0;
      }
      vect_add(out, "<\0");
      break;

    case '>': // basic
      if (len_temp > 0) {
        temp[len_temp] = '\0';
        vect_add(out, temp);
        temp[0] = '\0';
        len_temp = 0;
      }
      vect_add(out, ">\0");
      break;

    case ';': // basic
      if (len_temp > 0) {
        temp[len_temp] = '\0';
        vect_add(out, temp);
        temp[0] = '\0';
        len_temp = 0;
      }
      vect_add(out, ";\0");
      break;

    case ' ': // basic
      if (len_temp > 0) {
        temp[len_temp] = '\0';
        vect_add(out, temp);
        temp[0] = '\0';
        len_temp = 0;
      }
      break;

    case '\0': // basic
      temp[len_temp] = '\0';
      vect_add(out, temp);
      return out;
      break;

    case '\n':
      break;

    default: // basic
      temp[len_temp] = input[i];
      len_temp++;
      break;
    }
  }

  free(buffer);
  temp[len_temp] = '\0';
  vect_add(out, temp);
  return out;
}

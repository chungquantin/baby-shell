#include <stdio.h>
#include <stdlib.h>

#include "assert.h"
#include "parse_token.h"
#include "string.h"
#include "vect.h"

/**
 * Tokenizer : Uses the parse_token function in parse_token.h for the sake of
 *compiling Usage:  echo "<enter string here>" | ./tokenize
 **/
int main(int argc, char **argv) {
  int counter = 0;
  char input[MAX_CMD_LEN];

  char *pipein = malloc(sizeof(char) * MAX_CMD_LEN + 1);
  fgets(pipein, MAX_CMD_LEN + 1, stdin);

  vect_t *tokenizeTest = parse_token(pipein);
  pipein[strlen(pipein)] = '\0';

  for (int i = 0; i < vect_size(tokenizeTest); i++) {
    printf("%s\n", vect_get(tokenizeTest, i));
  }

  vect_delete(tokenizeTest);
  free(pipein);
  return 0;
}

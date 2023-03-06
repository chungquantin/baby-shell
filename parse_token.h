#ifndef _TOKEN_H
#define _TOKEN_H

#include "vect.h"

#define MAX_CMD_LEN 255

vect_t *parse_token(const char *input);

#endif

// token.c

#include "token.h"

void token_print(FILE* file, struct Token token) {
  fprintf(file, "%.*s", token.length, token.string);
}

void token_printline(FILE* file, struct Token token) {
  token_print(file, token);
  fprintf(file, "\n");
}

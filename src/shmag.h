#ifndef __SHMAG__
#define __SHMAG__

typedef enum {
   WORD,
   NUMBER,
   EQ,
   GT,
   LT,
   SET,
} TokenType;

typedef struct {
   TokenType p_type;
   union {
      char *word;
      double number;
   } as;
   int line;
   int col;
} PToken;

const char *ptoken_str(const PToken *ptoken);
void print_ptoken(const PToken *ptoken);
void parse_as_tokens(const char *, PToken *tokens, int *num_tokens);

int validate_syntax(PToken *tokens, int num_tokens);

#endif

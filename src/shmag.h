#ifndef __SHMAG__
#define __SHMAG__

#include <stdarg.h>

typedef enum {
   LINE_SEP,
   SET_WORD,
   WORD,
   NUMBER,
   PLUS,
   MINUS,
   MULT,
   DIV,
   EQ,
   GT,
   LT,
   IF,
   END,
   WHILE,
   GOTO,
   DUMP,
   SET,
} TokenType;

typedef union {
   char *word;
   double number;
   int cond[2];
} MultiVal;

typedef struct {
   TokenType p_type;
   MultiVal as;
   int line;
   int col;
} PToken;

typedef struct {
   TokenType r_type;
   MultiVal as;  
} RToken;

typedef struct node {
   char *key;
   MultiVal value;
   struct node *next;
} RNode;

#define MAX_BUCKETS 27
typedef struct hashMap{
   int num_elems;
   RNode *arr[MAX_BUCKETS];
} RMap;

typedef struct {
   RMap variables;
} REnv;

void parse_as_tokens(const char *, PToken *tokens, int *num_tokens);

/* Interpreter Stages */
int validate_syntax(PToken *tokens, int num_tokens);
int build_runnable(PToken *tokens, int num_tokens, RToken *runnable, int *num_run_tokens);
int execute_runnable(REnv *env, RToken *runnable, int runnable_len);

#define token_errorf(fmt, ...) _token_errorf(__FILE__, __func__, __LINE__, fmt, __VA_ARGS__)

void _token_errorf(const char *file, const char *func, int line, const char *fmt, const PToken *token, ...);

/* Text Formatting */
const char *ptoken_str(const PToken *ptoken);
void print_ptoken(const PToken *ptoken);
const char *rtoken_str(const RToken *rtoken);
void print_rtoken(const RToken *rtoken);

/* Map Functions */
void delete_rmap(RMap *map, char *key);
void insert_rmap(RMap *map, char *key, MultiVal value);
MultiVal *search_rmap(RMap *map, char *key);
RNode *node_search_rmap(RMap *map, char *key, int *hash);
int hash_function(char *key);
#endif

#ifndef __SHMAG__
#define __SHMAG__

#include <stdarg.h>

typedef enum {
   LINE_SEP,
   SET_WORD,
   WORD,
   NUMBER,
   PLUS,
   MOD,
   MINUS,
   MULT,
   PAREN_OPEN,
   PAREN_CLOSE,
   DIV,

   EQ,
   GT,
   LT,

   IF,
   ELSE,
   END,
   ELIF,
   WHILE,
   GOTO,
   DUMP,
   SET,
   INIT_SHM,
   PUSH_SHM,
   FUNC,
   CALL,
   PROTO_FUNC,
   EXPR_SEP,
} TokenType;

#define OP_MASK(op) (op & 0xFFFF)
#define RHS_D 0x010000
#define LHS_D 0x020000

#define NUMBER_FLT 0x80000000
#define NUMBER_IS_FLT(op) ((op & NUMBER_FLT) && 1)

#define LHS_IS_DBL(op) ((op & LHS_D) && 1)
#define RHS_IS_DBL(op) ((op & RHS_D) && 1)

#define WORD_DBL 0x80000000
#define WORD_IS_DBL(op) ((op & WORD_DBL) && 1)

typedef struct _shm_func ShmFunc;

typedef union {
   char *word;
   double number;
   long decimal;
   int cond[2];
   void *data;
   ShmFunc *func;
} MultiVal;

typedef enum {
   SHM_DBL,
   SHM_INT,
   SHM_STR,
   SHM_FUNC,
   SHM_NULL,
} ShmType;

typedef struct {
   ShmType obj_type;
   MultiVal as;
} ShmObj;

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

typedef struct  _shm_func{
   int num_args;
   const char **args;
   RToken *tokens;
   int num_tokens;
} ShmFunc;

typedef struct node {
   char *key;
   ShmObj obj;
   struct node *next;
} RNode;

#define MAX_BUCKETS 27
typedef struct hashMap{
   int num_elems;
   RNode *arr[MAX_BUCKETS];
} RMap;

typedef struct v_node {
   char *key;
   void *data;
   struct v_node *next;
} VNode;

typedef struct v_hashMap{
   int num_elems;
   VNode *arr[MAX_BUCKETS];
} VMap;

typedef struct {
   RMap variables;
   RMap funcs;
} REnv;

void parse_as_tokens(const char *, PToken *tokens, int *num_tokens);

/* Interpreter Stages */
int validate_syntax(PToken *tokens, int num_tokens);
int build_runnable(PToken *tokens, int num_tokens, REnv *env, RToken *runnable, int *num_run_tokens);
int execute_runnable(REnv *env, RToken *runnable, int runnable_len);

#define token_errorf(fmt, ...) _token_errorf(__FILE__, __func__, __LINE__, fmt, __VA_ARGS__)

void _token_errorf(const char *file, const char *func, int line, const char *fmt, const PToken *token, ...);

/* Text Formatting */
const char *ptoken_str(const PToken *ptoken);
void print_ptoken(const PToken *ptoken);
const char *rtoken_str(const RToken *rtoken);
void print_rtoken(const RToken *rtoken);
const char *shm_type_str(ShmType type);

/* Map Functions */
void delete_rmap(RMap *map, const char *key);
void insert_rmap(RMap *map, const char *key, ShmType type, MultiVal value);
ShmObj *search_rmap(RMap *map, const char *key);
RNode *node_search_rmap(RMap *map, const char *key, int *hash);
int hash_function(const char *key);

void delete_vmap(VMap *map, const char *key);
void insert_vmap(VMap *map, const char *key, void *value);
void *search_vmap(VMap *map, const char *key);
VNode *node_search_vmap(VMap *map, const char *key, int *hash);

#endif

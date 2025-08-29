#ifndef __SHMAG__
#define __SHMAG__

#include <stdarg.h>
#include <cutils/array.h>

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
   DEL_VAR,
   RETURN,

   /* Types */
   TYPE_INT,
   TYPE_FLOAT,
   TYPE_NONE,

   ARROW
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

#define SET_RET_INDEX 0x80000000
#define RET_IS_SET(ret) ((ret & SET_RET_INDEX) && 1)

#define COLOR_PURPLE "\e[0;35m"
#define COLOR_RED "\e[0;31m"
#define COLOR_CYAN "\e[0;36m"
#define COLOR_DEFAULT "\e[0;0m"
#define PURPLE(str) COLOR_PURPLE str COLOR_DEFAULT
#define RED(str) COLOR_RED str COLOR_DEFAULT
#define CYAN(str) COLOR_CYAN str COLOR_DEFAULT

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
   SHM_NONE,
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
   char *func_name;
   int num_args;
   char **args;
   ShmType return_type;
   ShmType *types;
   RToken *tokens;
   int has_return; /* doubles as return value */
   int num_tokens;
   int initialized; 
} ShmFunc;

#define MAX_BUCKETS 27

typedef struct v_node {
   char *key;
   void *data;
   struct v_node *next;
} VNode;

typedef struct v_hashMap{
   int num_elems;
   VNode *arr[MAX_BUCKETS];
} VMap;

/* typedef struct r_mapiter{
   int bucket;
   int done;
   RNode *curr;
   RMap *map;
} RMapIter; */

typedef struct s_node {
   char *key;
   struct s_node *next;
} SNode;

typedef struct s_set{
   int num_elems;
   SNode *arr[MAX_BUCKETS];
} SSet;

array_struct(func_array, ShmFunc *);

typedef struct {
   VMap variables;
   func_array funcs;
} REnv;

typedef void (*destroy_func)(void *ptr);

array_struct(string_array, char *);
string_array *parse_as_tokens(const char *, PToken *tokens, int *num_tokens);

int is_seperator(char c);

/* Interpreter Stages */
int validate_syntax(PToken *tokens, int num_tokens);
int build_runnable(PToken *tokens, int num_tokens, REnv *env, VMap *func_table, RToken *runnable, int *num_run_tokens);
int execute_runnable(REnv *env, RToken *runnable, int runnable_len);

#define token_errorf(...) _token_errorf(1, __VA_ARGS__)
#define token_errorf_no_line(...) _token_errorf(0, __VA_ARGS__)
void _token_errorf(int show_line, const char *fmt, const PToken *token, ...);

ShmObj *create_shmobj(ShmType type, MultiVal value);

/* Text Formatting */
const char *ptoken_str(const PToken *ptoken);
void print_ptoken(const PToken *ptoken);
const char *rtoken_str(const RToken *rtoken);
void print_rtoken(const RToken *rtoken);
const char *shm_type_str(ShmType type);
void log_executable(const char *name, RToken *exe, int len);
void free_shm_function(ShmObj *func);
void free_function(ShmFunc *func);
void free_rtokens(RToken *tokens, int num_tokens);

// setup/cleanup/inspect scope
void start_scope();
void end_scope();
char *static_scoped_var(const char *var);
char *offset_scoped_var(const char *var, int offset);
char *find_scoped_variable(char *word, VMap *map);

/* Map Functions */
int hash_function(const char *key);
void map_remove(VMap *map, const char *key, destroy_func func);
void map_delete(VMap *map, destroy_func func);
void map_insert(VMap *map, const char *key, void *value);
void **map_get(VMap *map, const char *key);
int map_has(VMap *map, const char *key);
VNode *node_search_map(VMap *map, const char *key, int *hash);
/*
void set_add(SSet *set, const char *key);
void set_remove(SSet *set, const char *key);
int  set_contains(SSet *set, const char *key);
SNode *node_search_set(SSet *map, const char *key, int *hash);
*/

void print_function(const ShmFunc *func_info);

// Logging Functions
void log_start();
void log_stop();
void log_function(const ShmFunc *func_info);

void log_set_file(const char *file);
void log_obj(const char *, const ShmObj *);
void log_msg(const char *);
void log_int(const char *, long);
void log_str(const char *, const char *);
void log_float(const char *, double);
void log_rtoken(const char *, const RToken *);
void log_ptoken(const char *, const PToken *);

#endif

#include <shmag.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

RToken to_rtoken(PToken *tkn){
   log_msg("PTOKEN TO RTOKEN");
   log_ptoken("ORIGINAL", tkn);
   RToken new_token = {
      .r_type = tkn->p_type,
      .as.word = NULL
   };

   if(tkn->p_type == WORD || OP_MASK(tkn->p_type) == NUMBER || tkn->p_type == SET_WORD || tkn->p_type == CALL){
      new_token.as = tkn->as;
   }
   log_rtoken("CONVERTED", &new_token);

   return new_token;
}

int op_prec(TokenType type){
   switch(type){
      case GT:
      case LT:
      case EQ:
      case PLUS:
      case MINUS:
         return 1;
      case MULT:
      case DIV:
      case MOD:
         return 2;
      break;
      case PAREN_OPEN:
      case PAREN_CLOSE:
         return 3;
      break;
      default:
         fprintf(stderr, "type(%d) is not implemented in op_prec\n", type);
         return -1;
   }
}

static struct {
   PToken *stack[100];
   int types[100];
   int types_head;
   int size;
   int capacity;
   int line;
   int first_op;
   ShmType type;
} expression = {.types_head = 0, .capacity = 100, .size = 0, .line = -1, .first_op = 0};

struct typed_word{
   const char *word;
   ShmType type;
};
static struct {
   struct typed_word words[100];
   int size;
} registered_words;

static int scope_prefix = '\0';

void expression_push(PToken *tkn, RToken *runnable, int *num_run_tokens, RMap *map);
int expression_flush(RToken *runnable, int *num_run_tokens);
int expression_flush_parens(RToken *runnable, int *num_run_tokens);
void encode_operand(RToken *tkn);
void register_word(const char *word, ShmType type, RMap *map);
void deregister_word(const char *word, RMap *map);
void check_init_shm(RToken *runnable, int *num_run_tokens);
int parse_function_header(REnv *env, PToken *tokens, int start, int op_index);
ShmType expression_type();
ShmType is_word_registered(const char *word, RMap *map);
ShmType conv_to_shm(TokenType type);

int build_runnable(PToken *tokens, int num_tokens, REnv *env, RToken *runnable, int *num_run_tokens){
   log_set_file("ast.c");
   log_msg("BUILDING EXECUTABLE");
   int op_index = 0;
   PToken *val_stack[1000];
   ShmFunc *func_info;
   int stack_head = 0;
   int expr_line = 0;
   int size;
   
   for(int i = 0; i < num_tokens; ++i){
      PToken *curr = &tokens[i];
      switch(OP_MASK(curr->p_type)){
         case WORD:
            // Check if the value is being set
            if(i + 1 < num_tokens && tokens[i + 1].p_type == SET){
               char *scoped_word = find_scoped_variable(curr->as.word, &env->variables);
               if(scoped_word == NULL){
                  scoped_word = strdup(static_scoped_var(curr->as.word));
                  free(curr->as.word);
                  
               }
               curr->as.word = scoped_word;
               val_stack[stack_head++] = curr;
            }else{
               expression_push(curr, runnable, &op_index, &env->variables);
            }
         break;
         case PROTO_FUNC:
            log_msg("FUNCTION PROTOTYPE");
            if(parse_function_header(env, tokens, i, op_index)){
               return -1;
            }
            // Skip function prototype
            while(tokens[i + 1].p_type != LINE_SEP){
               i++;
            }
            log_msg("FUNCTION SKIP LINE");
         break;
         case FUNC:
            log_msg("FUNCTION DEFINITION");
            if(parse_function_header(env, tokens, i, op_index)){
               return -1;
            }
            val_stack[stack_head++] = curr;
            val_stack[stack_head - 1]->as.word = tokens[i + 1].as.word; // Save the function Name
            // Skip function prototype
            while(tokens[i + 1].p_type != LINE_SEP){
               i++;
            }
            log_msg("FUNCTION SKIP LINE");

            // Push function to stack
         break;
         case NUMBER:
            expression_push(curr, runnable, &op_index, &env->variables);
         break;
         case DUMP:
            val_stack[stack_head++] = curr;
         break;
         case GT:
         case LT:
         case EQ:
         case PLUS:
         case MINUS:
         case MOD:
         case DIV:
         case MULT:
            expression_push(curr, runnable, &op_index, &env->variables);
         break;
         case SET:
            curr->p_type = SET_WORD;
            curr->as.word = tokens[i - 1].as.word;
            val_stack[stack_head - 1] = curr;
            expr_line = curr->line;
         break;
         case IF:
            val_stack[stack_head++] = curr;
            curr->as.cond[0] = -1;
            curr->as.cond[1] = -1;
            expr_line = curr->line;
         break;
         case ELIF:
            val_stack[stack_head - 1]->as.cond[1] = op_index;

            val_stack[stack_head++] = curr;
            curr->as.cond[0] = -1;
            curr->as.cond[1] = -1;

            runnable[op_index++] = (RToken){
               .r_type = GOTO,
               .as.cond = {-1},
            };
         break;
         case ELSE:
            val_stack[stack_head - 1]->as.cond[1] = op_index;
            runnable[op_index++] = (RToken){
               .r_type = GOTO,
               .as.cond = {-1},
            };
         break;
         case WHILE:
            val_stack[stack_head++] = curr;
            curr->as.cond[0] = -1;
            curr->as.cond[1] = op_index - 1; // Start of while conditional
            expr_line = curr->line;
         break;
         case LINE_SEP:
            if((expr_line = expression_flush(runnable, &op_index)) != 0){
               if(val_stack[stack_head - 1]->p_type == SET_WORD){
                  runnable[op_index++] = to_rtoken(val_stack[stack_head - 1]);
                  if(expression_type() == SHM_DBL){
                     runnable[op_index - 1].r_type = runnable[op_index - 1].r_type | WORD_DBL;
                  }
                  register_word(runnable[op_index - 1].as.word, expression_type(), &env->variables);
                  stack_head--;
               }else if(val_stack[stack_head - 1]->p_type == IF && val_stack[stack_head - 1]->as.cond[0] == -1){
                  runnable[op_index] = (RToken){
                     .r_type = IF,
                     .as.cond = {0}
                  };
                  val_stack[stack_head - 1]->as.cond[0] = op_index;
                  op_index++;
               }else if(val_stack[stack_head - 1]->p_type == ELIF && val_stack[stack_head - 1]->as.cond[0] == -1){
                  runnable[op_index] = (RToken){
                     .r_type = IF,
                     .as.cond = {0}
                  };
                  val_stack[stack_head - 1]->as.cond[0] = op_index;
                  op_index++;
               }else if(val_stack[stack_head - 1]->p_type == WHILE && val_stack[stack_head - 1]->as.cond[0] == -1){
                   runnable[op_index] = (RToken){
                     .r_type = WHILE,
                     .as.cond = {0}
                  };
                  val_stack[stack_head - 1]->as.cond[0] = op_index;
                  op_index++;                 
               }else if(val_stack[stack_head - 1]->p_type == DUMP){
                  runnable[op_index] = (RToken){
                     .r_type = DUMP,
                  };

                  if(expression_type() == SHM_DBL){
                     runnable[op_index].r_type = runnable[op_index].r_type | NUMBER_FLT;
                  }
                  op_index++;
                  stack_head--;
               }
            }
         break;
         case CALL:
            // Verify number of arguments
            size = 0;
            func_info = search_rmap(&env->funcs, curr->as.word)->as.func;
            if(tokens[i + 2].p_type != PAREN_CLOSE){
               int j = i + 2;
               size = 1;
               while(tokens[j].p_type != PAREN_CLOSE){
                  if(tokens[j].p_type == EXPR_SEP) size++;
                  j++;
               }
            }

            if(size != func_info->num_args){
               token_errorf("Argument missmatch. Expected %d. Actual %d", curr, func_info->num_args, size);
               return -1;
            }
            // push function to stack
            val_stack[stack_head++] = curr;
         break;
         case EXPR_SEP:
            // if , is seen flush the expression
            expression_flush(runnable, &op_index);
         break;
         case PAREN_OPEN:
            if(stack_head > 0 && val_stack[stack_head - 1]->p_type != CALL){
               log_msg("EXPRESSION PAREN OPEN");
               expression_push(curr, runnable, &op_index, &env->variables);
            }
         break;
         case PAREN_CLOSE:
            // once ) is reached pop function from stack
            if(stack_head > 0 && val_stack[stack_head - 1]->p_type == CALL){
               func_info = search_rmap(&env->funcs, val_stack[stack_head - 1]->as.word)->as.func;
               if(func_info->num_args > 0){
                  expression_flush(runnable, &op_index);
               }
               runnable[op_index++] = to_rtoken(val_stack[stack_head - 1]);
               stack_head--;

               // If there is a return type. use it
               if(func_info->return_type != SHM_NONE){
                  log_msg("FUNCTION CALL HAS RETURN");
                  if(expression.line == -1){
                     expression.type = func_info->return_type;
                     expression.line = curr->line;
                     log_int("EXPRESSION LINE", curr->line);
                  }
                  expression.types[expression.types_head++] = func_info->return_type == SHM_DBL; // For the time being it is only ints and floats
               }
            }else{
               log_msg("EXPRESSION PAREN CLOSE");
               expression_push(curr, runnable, &op_index, &env->variables);
            }

         break;
         case END:
            if(val_stack[stack_head - 1]->p_type == IF && val_stack[stack_head - 1]->as.cond[1] == -1){ // No else statement
               runnable[val_stack[stack_head - 1]->as.cond[0]].as.cond[0] = op_index - 1;
               stack_head--;
            }else if(val_stack[stack_head - 1]->p_type == IF){ // If statement has else statement
               runnable[val_stack[stack_head - 1]->as.cond[1]].as.cond[0] = op_index - 1; // GOTO statement
               runnable[val_stack[stack_head - 1]->as.cond[0]].as.cond[0] = val_stack[stack_head - 1]->as.cond[1]; // If statement
               stack_head--;
            }else if(val_stack[stack_head - 1]->p_type == ELIF){
               /*
                * op_index - 1 - ALL GOTO STATEMENTS (Exits from the ifs)
                * cond[1]      - next if statement
                */

               // check if there was an else statement 
               if(val_stack[stack_head - 1]->as.cond[1] == -1){
                  val_stack[stack_head - 1]->as.cond[1] = op_index - 1;
               }
               while(val_stack[stack_head - 1]->p_type == ELIF){
                  runnable[val_stack[stack_head - 1]->as.cond[1]].as.cond[0] = op_index - 1; // GOTO statement
                  runnable[val_stack[stack_head - 1]->as.cond[0]].as.cond[0] = val_stack[stack_head - 1]->as.cond[1]; // If statement
                  stack_head--;
               }

               // Last do the if statement
               runnable[val_stack[stack_head - 1]->as.cond[1]].as.cond[0] = op_index - 1; // GOTO statement
               runnable[val_stack[stack_head - 1]->as.cond[0]].as.cond[0] = val_stack[stack_head - 1]->as.cond[1]; // If statement
               stack_head--;
            }else if(val_stack[stack_head - 1]->p_type == WHILE){
               // same as if but also add goto statement to go back to start of while condition
               runnable[val_stack[stack_head - 1]->as.cond[0]].as.cond[0] = op_index;
               runnable[val_stack[stack_head - 1]->as.cond[0]].r_type = IF;
               runnable[op_index++] = (RToken) {
                  .r_type = GOTO,
                  .as.cond = {val_stack[stack_head - 1]->as.cond[1], 0}
               };
               stack_head--;
            }else if(val_stack[stack_head - 1]->p_type == FUNC){
               // Calculate size of the function
               ShmObj *func_obj = search_rmap(&env->funcs, val_stack[stack_head - 1]->as.word);
               ShmFunc *func_info = func_obj->as.func;

               for(int arg_i = 0; arg_i < func_info->num_args; ++arg_i){
                  deregister_word(func_info->args[arg_i], &env->variables);
               }

               int func_size = op_index - func_info->num_tokens;

               // allocate space for function tokens
               func_info->tokens = calloc(sizeof(RToken), func_size);
               // copy function ops into function tokens
               memcpy(func_info->tokens, runnable + func_info->num_tokens, func_size * sizeof(RToken));
               func_info->num_tokens = func_size;

               // Exit scope
               end_scope();

               // decrement op_index by num_tokens and set func->num_tokens
               op_index -= func_info->num_tokens;

               log_function(func_info);
            }else{
               printf("%s - Something went wrong\n", ptoken_str(val_stack[stack_head - 1]));
               return -1;
            }
         break;
         default:
            fprintf(stderr, "Unknown token type[%d] discovered at %s\n", curr->p_type, ptoken_str(curr));
            return -1;
         break;
      }
   }
   
   log_msg("EXECUTABLE BUILT WITH NO ERRORS\n");
   *num_run_tokens = op_index;
   return 0;
}

void expression_push(PToken *tkn, RToken *runnable, int *num_run_tokens, RMap *map){
   log_msg("EXPRESSION PUSH");
   if(expression.line == -1){
      expression.type = SHM_INT;
      expression.line = tkn->line;
      log_int("EXPRESSION LINE", tkn->line);
   }else{
      if(tkn->line != expression.line){
         fprintf(stderr, "Multiline expression found between lines %d and %d\n", expression.line, tkn->line);
         exit(1);
      }
   }

   if(tkn->p_type == WORD || OP_MASK(tkn->p_type) == NUMBER){
      log_msg("EXPRESSION PUSH VALUE");
      if(OP_MASK(tkn->p_type) == NUMBER){
         expression.types[expression.types_head] = NUMBER_IS_FLT(tkn->p_type);
      }else{
         char *scoped_word = find_scoped_variable(tkn->as.word, map);
         ShmType word_type = is_word_registered(scoped_word, map);
         if(word_type == SHM_NULL){
            fprintf(stderr, "%d, %d: ", tkn->line, tkn->col);
            fprintf(stderr, "'%s' has not been declared in this scope\n", tkn->as.word);
            exit(1);
         }

         if(word_type == SHM_DBL){
            expression.type = SHM_DBL;
         }

         tkn->as.word = scoped_word;
         expression.types[expression.types_head] = word_type == SHM_DBL;
      }

      expression.types_head++;
      runnable[*num_run_tokens] = to_rtoken(tkn);
      *num_run_tokens = *num_run_tokens + 1;
   }else if(tkn->p_type == PAREN_OPEN){
      log_msg("PUSHING PAREN OPEN");
      expression.stack[expression.size++] = tkn;
   }else if(tkn->p_type == PAREN_CLOSE){
      log_msg("POPPING TILL PAREN OPEN");
      while(expression.size > 0 && expression.stack[expression.size - 1]->p_type != PAREN_OPEN){
         runnable[*num_run_tokens] = to_rtoken(expression.stack[expression.size - 1]);

         encode_operand(&runnable[*num_run_tokens]);

         *num_run_tokens = *num_run_tokens + 1;
         expression.size -= 1;
      }
      expression.size -= 1;
   }else{
      log_msg("EXPRESSION PUSH OPERAND");
      while(expression.size > 0 && op_prec(tkn->p_type) <= op_prec(expression.stack[expression.size - 1]->p_type)){
         runnable[*num_run_tokens] = to_rtoken(expression.stack[expression.size - 1]);

         encode_operand(&runnable[*num_run_tokens]);

         *num_run_tokens = *num_run_tokens + 1;
         expression.size -= 1;
      }
      expression.stack[expression.size++] = tkn;
   }
}

ShmType expression_type(){
   return expression.type;
}

int expression_flush(RToken *runnable, int *num_run_tokens){
   log_msg("EXPRESSION FLUSH");
   int tmp = expression.line;
   if(expression.line == -1){
      return 0;
   }

   expression.line = -1;
   while(expression.size > 0){
      
      runnable[*num_run_tokens] = to_rtoken(expression.stack[expression.size - 1]);
      encode_operand(&runnable[*num_run_tokens]);

      *num_run_tokens = *num_run_tokens + 1;
      expression.size -= 1;
   }

   if(expression.types_head > 0){
      expression.type = expression.types[expression.types_head - 1] ? SHM_DBL : SHM_INT;
   }

   expression.types_head = 0;
   expression.first_op = 0;

   return tmp;
}


int expression_flush_parens(RToken *runnable, int *num_run_tokens){
   log_msg("EXPRESSION PAREN FLUSH");
   int tmp = expression.line;
   if(expression.line == -1){
      return 0;
   }

   int open_parens = 0;

   while(expression.size > 0 && open_parens > 0){
      if(expression.stack[expression.size - 1]->p_type == PAREN_OPEN){
         open_parens--;
      }else if(expression.stack[expression.size - 1]->p_type == PAREN_CLOSE){
         open_parens++;
      }else{
         runnable[*num_run_tokens] = to_rtoken(expression.stack[expression.size - 1]);
         encode_operand(&runnable[*num_run_tokens]);
         *num_run_tokens = *num_run_tokens + 1;
      }

      expression.size -= 1;
   }

   if(expression.types_head > 0){
      expression.type = expression.types[expression.types_head - 1] ? SHM_DBL : SHM_INT;
   }

   expression.types_head = 0;
   expression.first_op = 0;

   return tmp;
}

const char *rtoken_str(const RToken *rtoken){
   static char _buffer[512];

   switch(OP_MASK(rtoken->r_type)){
      case WORD:
         sprintf(_buffer, "Word(%s) at %p", rtoken->as.word, rtoken->as.word);
      break;
      case NUMBER:
         if(NUMBER_IS_FLT(rtoken->r_type)){
            sprintf(_buffer, "Float(%f)", rtoken->as.number);
         }else{
            sprintf(_buffer, "Integer(%ld)", rtoken->as.decimal);
         }
      break;
      case EQ:
         sprintf(_buffer, "Equals[%s,%s]", rtoken->r_type & LHS_D ? "Float" : "Integer", rtoken->r_type & RHS_D ? "Float" : "Integer");
      break;
      case GT:
         sprintf(_buffer, "GreaterThan[%s,%s]", rtoken->r_type & LHS_D ? "Float" : "Integer", rtoken->r_type & RHS_D ? "Float" : "Integer");
      break;
      case LT:
         sprintf(_buffer, "LessThan[%s,%s]", rtoken->r_type & LHS_D ? "Float" : "Integer", rtoken->r_type & RHS_D ? "Float" : "Integer");
      break;
      case PLUS:
         sprintf(_buffer, "Plus[%s,%s]", rtoken->r_type & LHS_D ? "Float" : "Integer", rtoken->r_type & RHS_D ? "Float" : "Integer");
      break;
      case MINUS:
         sprintf(_buffer, "Minus[%s,%s]", rtoken->r_type & LHS_D ? "Float" : "Integer", rtoken->r_type & RHS_D ? "Float" : "Integer");
      break;
      case DIV:
         sprintf(_buffer, "Div[%s,%s]", rtoken->r_type & LHS_D ? "Float" : "Integer", rtoken->r_type & RHS_D ? "Float" : "Integer");
      break;
      case MULT:
         sprintf(_buffer, "Mult[%s,%s]", rtoken->r_type & LHS_D ? "Float" : "Integer", rtoken->r_type & RHS_D ? "Float" : "Integer");
      break;
      case MOD:
         sprintf(_buffer, "Mod[%s,%s]", rtoken->r_type & LHS_D ? "Float" : "Integer", rtoken->r_type & RHS_D ? "Float" : "Integer");
      break;
      case SET_WORD:
         sprintf(_buffer, "SetWord(%s) at %p", rtoken->as.word, rtoken->as.word);
      break;
      case LINE_SEP:
         sprintf(_buffer, "NewLine");
      break;
      case DUMP:
         sprintf(_buffer, "Dump");
      break;
      case SET:
         sprintf(_buffer, "Set(%s)", rtoken->as.word);
      break;
      case WHILE:
         sprintf(_buffer, "While");
      break;
      case GOTO:
         sprintf(_buffer, "GOTO(%d)", rtoken->as.cond[0]);
      break;
      case IF:
         sprintf(_buffer, "If[%d:%d]", rtoken->as.cond[0], rtoken->as.cond[1]);
      break;
      case ELSE:
         sprintf(_buffer, "Else[%d:%d]", rtoken->as.cond[0], rtoken->as.cond[1]);
      break;
      case ELIF:
         sprintf(_buffer, "Elif[%d:%d]", rtoken->as.cond[0], rtoken->as.cond[1]);
      break;
      case INIT_SHM:
         sprintf(_buffer, "InitShm");
      break;
      case PUSH_SHM:
         sprintf(_buffer, "PushShm");
      break;
      case END:
         sprintf(_buffer, "End");
      break;
      case CALL:
         sprintf(_buffer, "Call(%s)", rtoken->as.word);
      break;
      case DEL_VAR:
         sprintf(_buffer, "Del(%s)", rtoken->as.word);
      break;
      default:
        return "Unimplemented RToken";
      break;
   }

   return _buffer;
}

void encode_operand(RToken *tkn){
   int op_type = 0;
   TokenType orig_type = tkn->r_type;
   // 1 means double, 0 means long
   log_str("LHS", expression.types[expression.types_head - 2] ? "Float" : "Integer");
   if(expression.types[expression.types_head - 2] == 1){
      tkn->r_type = tkn->r_type | LHS_D;
      op_type = 1;
   }

   log_str("RHS", expression.types[expression.types_head - 1] ? "Float" : "Integer");
   if(expression.types[expression.types_head - 1] == 1){
      tkn->r_type = tkn->r_type | RHS_D;
      op_type = 1;
   }

   if(orig_type == GT || orig_type == LT || orig_type == EQ){
      op_type = 0;
   }

   log_str("EXPRESSION TYPE", op_type ? "Float" : "Integer");
   log_rtoken("ENCODED OPERAND", tkn);
   expression.types_head--;
   expression.types[expression.types_head - 1] = op_type;
   
}

void register_word(const char *word, ShmType type, RMap *map){
   if(registered_words.size >= 100){
      fprintf(stderr, "too many words registered\n");
      exit(1);
   }

   if(is_word_registered(word, map) == SHM_NULL){
      log_str("REGISTERING WORD", word);
      log_str("WORD TYPE", shm_type_str(type));
      registered_words.words[registered_words.size++] = (struct typed_word){
         .word = word,
         .type = type
      };
   }
}

void deregister_word(const char *word, RMap *map){
   log_str("UNREGISTER", word);
   if(search_rmap(map, word)){
      fprintf(stderr, "Warning: Unable to deregister external token\n");
      log_msg("WARNING: Unable to deregister external token");
   }else{
      for(int i = 0; i < registered_words.size; ++i){
         if(strcmp(word, registered_words.words[i].word) == 0){
            for(int j = i + 1; j < registered_words.size; ++j){
               registered_words.words[i - 1] = registered_words.words[i];
            }
            registered_words.size--;
            break;
         }
      }
   }
}

char *find_scoped_variable(char *word, RMap *map){
   log_str("FINDING SCOPED VARIABLE", word);
   if(search_rmap(map, word) != NULL){
      return word;
   }else{
      for(int i = 0; i < registered_words.size; ++i){
         if(strstr(registered_words.words[i].word, word)){
            // Search scope top to scope bottom? its the oppositie of what it looks like
            for(int scope_offset = 0; scope_offset <= scope_prefix; scope_offset++){
               char *scoped_word = offset_scoped_var(word, scope_offset);
               if(strcmp(scoped_word, registered_words.words[i].word) == 0){
                  free(word);
                  log_str("FOUND", scoped_word);
                  return strdup(scoped_word);
               }
            }
         }
      }
   }
   log_msg("SCOPED VARIABLE NOT FOUND");
   return NULL;
}

ShmType is_word_registered(const char *word, RMap *map){
   ShmType result = SHM_NULL;
   ShmObj *obj;

   if(word == NULL){
      return SHM_NULL;
   }

   if((obj = search_rmap(map, word)) != NULL){
      result = obj->obj_type;
   }else{
      for(int i = 0; i < registered_words.size; ++i){
         if(strcmp(word, registered_words.words[i].word) == 0){
            result = registered_words.words[i].type;
            break;
         }
      }
   }

   log_str("WORD TYPE", shm_type_str(result));

   return result;
}

int parse_function_header(REnv *env, PToken *tokens, int start, int op_index){
   int is_proto = tokens[start].p_type == PROTO_FUNC;
   log_str("PARSING FUNCTION", is_proto ? "PROTOTYPE" : "BODY");

   if(search_rmap(&env->variables, tokens[start + 1].as.word)){
      token_errorf("Cannot be used as both a variable and a function", &tokens[start + 1]);
      return 1;
   }

   ShmObj *func_wrapper = search_rmap(&env->funcs, tokens[start + 1].as.word);
   int found = func_wrapper != NULL;
   log_str("FUNCTION", found ? "FOUND" : "NOT FOUND");
   ShmFunc *function;
   ShmType return_type = SHM_NONE;

   // get number of args
   int num_args = 0;
   int offset = 3;
   int i = 0;

   /* func name ( arg )
 *     0    1   2  3  4
 *    */
   while(tokens[start + offset].p_type != PAREN_CLOSE){
      if(tokens[start + offset].p_type == WORD) num_args++;
      offset += 1;
   }

   log_int("FUNCTION ARGS FOUND", num_args);
   if(tokens[start + offset + 1].p_type == ARROW){
      return_type = conv_to_shm(tokens[start + offset + 2].p_type);

      // Confirm type is not null
      if(return_type == SHM_NULL){
         log_msg("ERROR: Unable to convert type to shmtype");
         token_errorf("Unknown ShmType %s found in function header", &tokens[start + offset + 2], ptoken_str(&tokens[start + offset + 2]));
         return 1;
      }
   }

   if(found){
      // check if the number of args are the same
      function = func_wrapper->as.func;
      if(function->num_args != num_args){
         log_msg("ERROR: Function Arg Mismatch");
         token_errorf("Found duplicate function header that does not have the same number of args as the original, found %d expected %d", &tokens[i + 1], num_args, function->num_args);
         return 1;
      }

      if(function->return_type != return_type){
         log_msg("ERROR: Function Return Type");
         token_errorf("Found duplicate function header that does not have the same return type as the original, found %s expected %s", &tokens[i + 1], shm_type_str(return_type), shm_type_str(function->return_type));
         return 1;
      }
   }else{
      function = calloc(1, sizeof(ShmFunc));
      function->num_args = num_args;
      function->return_type = return_type;
      insert_rmap(&env->funcs, tokens[start + 1].as.word, SHM_FUNC, (MultiVal){.func = function});
      log_msg("MEMROY: Allocating New Function");
      log_str("FUNCTION RETURN TYPE", shm_type_str(return_type));
   }

   if(!is_proto){
      if(found && function->initialized){
         log_msg("ERROR: Double Function Definition");
         token_errorf("Double initialization of function %s", &tokens[start], tokens[start + 1].as.word);
         return 1;
      }
      start_scope();
      function->args = calloc(sizeof(char *), num_args);
      function->types = calloc(sizeof(ShmType), num_args);
      while(i < num_args){
         function->types[i] = conv_to_shm(tokens[start + (i * 3) + 3].p_type);
         function->args[i] = strdup(static_scoped_var(tokens[start + (i * 3) + 4].as.word));
         register_word(function->args[i], function->types[i], &env->variables);
         i++;
      }
      function->num_tokens = op_index;
      function->initialized = 1;
      function->func_name = strdup(tokens[start + 1].as.word);
   }


   return 0;
}

void start_scope(){
   scope_prefix +=  1;
   log_msg("START SCOPE");
   /* detect integer overflow */
   if(scope_prefix < 0){
      fprintf(stderr, "Too many nested scopes\n");
      exit(1);
   }
}

void end_scope(){
   char scope_prefix_str[18];
   sprintf(scope_prefix_str, "%x-", scope_prefix);
   size_t prefix_len = strlen(scope_prefix_str);
   scope_prefix -= 1;

   int i, j = 0;
   for(i = 0; i < registered_words.size; ++i){
      log_int("WORD", i);
      log_str("WORD", registered_words.words[i].word);
      if(strncmp(scope_prefix_str, registered_words.words[i].word, prefix_len) != 0){
         // add del kw
         registered_words.words[j++] = registered_words.words[i];
      }else{
         log_str("DEREGISTERING", registered_words.words[i].word);
      }
   }

   registered_words.size = j;

   log_msg("END SCOPE");
   /* Detect integer below 0 */
   if(scope_prefix < 0){
      fprintf(stderr, "Trying to close a scope that has not been initialized\n");
      exit(1);
   }
}

char *static_scoped_var(const char *var){
   static char scoped_var[1500];

   if(scope_prefix){
      sprintf(scoped_var, "%x-%s", scope_prefix, var);
   }else{
      strcpy(scoped_var, var);
   }

   log_str("SCOPED VAR", scoped_var);
   return scoped_var;
}

char *offset_scoped_var(const char *var, int offset){
   static char scoped_var[1500];

   if(scope_prefix - offset > 0){
      sprintf(scoped_var, "%x-%s", scope_prefix - offset, var);
   }else{
      strcpy(scoped_var, var);
   }
   log_str("OFFSET SCOPED VAR", scoped_var);
   return scoped_var;
}

ShmType conv_to_shm(TokenType type){
   switch(type){
      case TYPE_INT:
         return SHM_INT;
      break;
      case TYPE_FLOAT:
         return SHM_DBL;
      break;
      case TYPE_NONE:
         return SHM_NONE;
      break;
      default:
      break;
   }
   return SHM_NULL;
}

void log_function(const ShmFunc *func_info){
   log_str("FUNCTION", func_info->func_name);
   log_int("NUMBER TOKENS", func_info->num_tokens);
   log_int("NUMBER ARGS", func_info->num_args);
   log_str("RETURN TYPE", shm_type_str(func_info->return_type));
   for(int i = 0; i < func_info->num_args; ++i){
      log_str("ARG", func_info->args[i]);
      log_str("TYPE", shm_type_str(func_info->types[i]));
   }

   log_msg("FUNCTION TOKENS");
   for(int i = 0; i < func_info->num_tokens; ++i){
      log_rtoken("", &func_info->tokens[i]);
   }

   log_msg("FUNCTION END");
   // This is where I would print a return value
}

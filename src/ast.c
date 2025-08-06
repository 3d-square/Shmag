#include <shmag.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

RToken to_rtoken(PToken *tkn){
   RToken new_token = {
      .r_type = tkn->p_type,
      .as.word = NULL
   };

   if(tkn->p_type == WORD || OP_MASK(tkn->p_type) == NUMBER || tkn->p_type == SET_WORD){
      new_token.as = tkn->as;
   }

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
      case DUMP:
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

void expression_push(PToken *tkn, RToken *runnable, int *num_run_tokens, RMap *map);
int expression_flush(RToken *runnable, int *num_run_tokens);
void encode_operand(RToken *tkn);
void register_word(const char *word, ShmType type, RMap *map);
void check_init_shm(RToken *runnable, int *num_run_tokens);
int parse_function_header(REnv *env, PToken *tokens, int start, int op_index);
ShmType expression_type();
ShmType is_word_registered(const char *word, RMap *map);

int build_runnable(PToken *tokens, int num_tokens, REnv *env, RToken *runnable, int *num_run_tokens){
   int op_index = 0;
   PToken *val_stack[1000];
   int stack_head = 0;
   int expr_line = 0;
   
   for(int i = 0; i < num_tokens; ++i){
      PToken *curr = &tokens[i];
      switch(OP_MASK(curr->p_type)){
         case WORD:
            // Check if the value is being set
            if(i + 1 < num_tokens){
               if(tokens[i + 1].p_type == SET){
                  val_stack[stack_head++] = curr;
               }
            }else{
               expression_push(curr, runnable, &op_index, &env->variables);
            }
         break;
         case PROTO_FUNC:
            if(parse_function_header(env, tokens, i, op_index)){
               return -1;
            }
            // Skip function prototype
            while(tokens[i + 1].p_type != LINE_SEP){
               i++;
            }
         break;
         case FUNC:
            if(parse_function_header(env, tokens, i, op_index)){
               return -1;
            }
            val_stack[stack_head++] = curr;
            val_stack[stack_head - 1]->as.word = tokens[i + 1].as.word; // Save the function Name
            // Skip function prototype
            while(tokens[i + 1].p_type != LINE_SEP){
               i++;
            }

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
            printf("Calling function %s\n", curr->as.word);
            // Verify number of arguments
            // push function to stack
            // if , is seen flush the expression
            // once ) is reached pop function from stack
            return -1;
         break;
         case END:
            if(val_stack[stack_head - 1]->p_type == IF && val_stack[stack_head - 1]->as.cond[1] == -1){ // No else statement
               runnable[val_stack[stack_head - 1]->as.cond[0]].as.cond[0] = op_index - 1;
               stack_head--;
            }else if(val_stack[stack_head - 1]->p_type == IF){ // If statement has else statement
               // printf("%d, %d\n", op_index - 1, val_stack[stack_head - 1]->as.cond[1]);
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
               int func_size = op_index - func_info->num_tokens;
               // printf("setting function %s with %d args, starts at op_index %d, current_op_index %d, func_size %d\n", val_stack[stack_head - 1]->as.word, func_info->num_args, func_size, op_index, func_size);

               // allocate space for function tokens
               func_info->tokens = calloc(sizeof(RToken), func_size);
               // copy function ops into function tokens
               memcpy(func_info->tokens, runnable + func_info->num_tokens, func_size * sizeof(RToken));
               
               // decrement op_index by num_tokens and set func->num_tokens
               op_index -= func_info->num_tokens;
               func_info->num_tokens = func_size;
            }else{
               printf("%s - Something went wrong\n", ptoken_str(val_stack[stack_head - 1]));
            }
         break;
         default:
            fprintf(stderr, "Unknown token type[%d] discovered at %s\n", curr->p_type, ptoken_str(curr));
            return -1;
         break;
      }
   }
   
   *num_run_tokens = op_index;
   return 0;
}

void expression_push(PToken *tkn, RToken *runnable, int *num_run_tokens, RMap *map){
   if(expression.line == -1){
      expression.type = SHM_INT;
      expression.line = tkn->line;
   }else{
      if(tkn->line != expression.line){
         fprintf(stderr, "Multiline expression found between lines %d and %d\n", expression.line, tkn->line);
         exit(1);
      }
   }

   if(tkn->p_type == WORD || OP_MASK(tkn->p_type) == NUMBER){
      if(OP_MASK(tkn->p_type) == NUMBER){
         expression.types[expression.types_head] = NUMBER_IS_FLT(tkn->p_type);
      }else{
         ShmType word_type = is_word_registered(tkn->as.word, map);
         if(word_type == SHM_NULL){
            fprintf(stderr, "%d, %d: ", tkn->line, tkn->col);
            fprintf(stderr, "'%s' has not been declared in this scope\n", tkn->as.word);
            exit(1);
         }

         if(word_type == SHM_DBL){
            expression.type = SHM_DBL;
         }
  
         expression.types[expression.types_head] = word_type == SHM_DBL;
      }

      expression.types_head++;
      runnable[*num_run_tokens] = to_rtoken(tkn);
      *num_run_tokens = *num_run_tokens + 1;
   }else{
      while(expression.size > 0 && op_prec(tkn->p_type) <= op_prec(expression.stack[expression.size - 1]->p_type)){
         check_init_shm(runnable, num_run_tokens);
         
         runnable[*num_run_tokens] = to_rtoken(expression.stack[expression.size - 1]);

         encode_operand(&runnable[*num_run_tokens]);

         *num_run_tokens = *num_run_tokens + 1;
         expression.size -= 1;
      }
      expression.stack[expression.size++] = tkn;
   }
}

void check_init_shm(RToken *runnable, int *num_run_tokens){
   if(expression.first_op == 0){
      runnable[*num_run_tokens] = (RToken){
         .r_type = INIT_SHM,
      };

      if(expression.types[expression.types_head - 1]){
         runnable[*num_run_tokens].r_type = runnable[*num_run_tokens].r_type | RHS_D;
      }

      *num_run_tokens = *num_run_tokens + 1;
      expression.first_op = 1;
   }
}

ShmType expression_type(){
   return expression.type;
}

int expression_flush(RToken *runnable, int *num_run_tokens){
   int tmp = expression.line;
   if(expression.line == -1){
      return 0;
   }

   expression.line = -1;
   while(expression.size > 0){
      check_init_shm(runnable, num_run_tokens);
      
      runnable[*num_run_tokens] = to_rtoken(expression.stack[expression.size - 1]);
      encode_operand(&runnable[*num_run_tokens]);

      *num_run_tokens = *num_run_tokens + 1;
      expression.size -= 1;
   }
   if(expression.first_op == 1){
      runnable[*num_run_tokens] = (RToken){
         .r_type = PUSH_SHM,
      };
      *num_run_tokens = *num_run_tokens + 1;
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
         sprintf(_buffer, "Word(%s)", rtoken->as.word);
      break;
      case NUMBER:
         sprintf(_buffer, "Number(%f)", rtoken->as.number);
      break;
      case EQ:
         sprintf(_buffer, "Equals");
      break;
      case GT:
         sprintf(_buffer, "GreaterThan");
      break;
      case LT:
         sprintf(_buffer, "LessThan");
      break;
      case PLUS:
         sprintf(_buffer, "Plus");
      break;
      case MINUS:
         sprintf(_buffer, "Minus");
      break;
      case DIV:
         sprintf(_buffer, "Div");
      break;
      case MULT:
         sprintf(_buffer, "Mult");
      break;
      case MOD:
         sprintf(_buffer, "Mod");
      break;
      case SET_WORD:
         sprintf(_buffer, "SetWord(%s)", rtoken->as.word);
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
   }

   return _buffer;
}

void print_rtoken(const RToken *rtoken){
   printf("%s", rtoken_str(rtoken));
}

void encode_operand(RToken *tkn){
   int op_type = 0;
   TokenType orig_type = tkn->r_type;
   // 1 means double, 0 means long
   if(expression.types[expression.types_head - 1] == 1){
      tkn->r_type = tkn->r_type | RHS_D;
      op_type = 1;
   }
   if(expression.types[expression.types_head - 2] == 1){
      tkn->r_type = tkn->r_type | LHS_D;
      op_type = 1;
   }

   if(orig_type == GT || orig_type == LT || orig_type == EQ){
      op_type = 0;
   }

   expression.types_head--;
   expression.types[expression.types_head - 1] = op_type;
}

void register_word(const char *word, ShmType type, RMap *map){
   if(registered_words.size >= 100){
      fprintf(stderr, "too many words registered\n");
      exit(1);
   }

   if(is_word_registered(word, map) == SHM_NULL){
      registered_words.words[registered_words.size++] = (struct typed_word){
         .word = word,
         .type = type
      };
   }
}

ShmType is_word_registered(const char *word, RMap *map){
   ShmType result = SHM_NULL;
   ShmObj *obj;

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

   return result;
}

int parse_function_header(REnv *env, PToken *tokens, int start, int op_index){
   int is_proto = tokens[start].p_type == PROTO_FUNC;

   if(search_rmap(&env->variables, tokens[start + 1].as.word)){
      token_errorf("Cannot be used as both a variable and a function", &tokens[start + 1]);
      return 1;
   }

   ShmObj *func_wrapper = search_rmap(&env->funcs, tokens[start + 1].as.word);
   int found = func_wrapper != NULL;
   ShmFunc *function;

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

   if(found){
      // check if the number of args are the same
      function = func_wrapper->as.func;
      if(function->num_args != num_args){
         token_errorf("Found duplicate function header that does not have the same number of args as the original, found %d expected %d", &tokens[i + 1], num_args, function->num_args);
         return 1;
      }

      if(!is_proto){
         if(function->initialized){
            token_errorf("Double initialization of function %s", &tokens[start], tokens[start + 1].as.word);
            return 1;
         }

         function->args = calloc(sizeof(char *), num_args);
         while(i < num_args){
            function->args[i] = strdup(tokens[start + (i * 2) + 3].as.word);
            i++;
         }
         function->num_tokens = op_index;
         function->initialized = 1;
      }
   }else{
      function = calloc(1, sizeof(ShmFunc));
      function->num_args = num_args;
      if(!is_proto){
         while(i < num_args){
            function->args[i] = strdup(tokens[start + (i * 2) + 3].as.word);
            i++;
         }

         function->num_tokens = op_index;
         function->initialized = 1;
      }
      insert_rmap(&env->funcs, tokens[start + 1].as.word, SHM_FUNC, (MultiVal){.func = function});
   }

   return 0;
}

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
            if(i + 1 < num_tokens && tokens[i + 1].p_type == SET){
               val_stack[stack_head++] = curr;
            }else{
               expression_push(curr, runnable, &op_index, &env->variables);
            }
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
                  // printf("Expression type = %s\n", shmtype_str(expression_type()));
                  if(expression_type() == DBL){
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

                  if(expression_type() == DBL){
                     runnable[op_index].r_type = runnable[op_index].r_type | NUMBER_FLT;
                  }
                  op_index++;
                  stack_head--;
               }
            }
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
      expression.type = INT;
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

         if(NUMBER_IS_FLT(tkn->p_type)){
            expression.type = DBL;
         }
      }else{
         ShmType word_type = is_word_registered(tkn->as.word, map);
         if(word_type == SHM_NULL){
            fprintf(stderr, "'%s' has not been declared in this scope\n", tkn->as.word);
            exit(1);
         }

         if(word_type == DBL){
            expression.type = DBL;
         }
  
         expression.types[expression.types_head] = word_type == DBL;
      }

      expression.types_head++;
      runnable[*num_run_tokens] = to_rtoken(tkn);
      *num_run_tokens = *num_run_tokens + 1;
   }else{
      while(expression.size > 0 && op_prec(tkn->p_type) <= op_prec(expression.stack[expression.size - 1]->p_type)){
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
   int tmp = expression.line;
   int tmp2 = expression.first_op;
   if(expression.line == -1){
      return 0;
   }

   expression.first_op = 0;
   expression.line = -1;
   while(expression.size > 0){
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
      runnable[*num_run_tokens] = to_rtoken(expression.stack[expression.size - 1]);
      encode_operand(&runnable[*num_run_tokens]);

      *num_run_tokens = *num_run_tokens + 1;
      expression.size -= 1;
   }
   if(tmp2 == 1){
      runnable[*num_run_tokens] = (RToken){
         .r_type = PUSH_SHM,
      };
      *num_run_tokens = *num_run_tokens + 1;
   }

   expression.types_head = 0;

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
  // 1 means double, 0 means long
  if(expression.types[expression.types_head - 1] == 1){
     tkn->r_type = tkn->r_type | RHS_D;
     op_type = 1;
  }
  if(expression.types[expression.types_head - 2] == 1){
     tkn->r_type = tkn->r_type | LHS_D;
     op_type = 1;
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

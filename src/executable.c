#include <shmag.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int is_truthy(MultiVal val);

void perform_shm_operation(TokenType op, MultiVal *lhs, MultiVal rhs);

int execute_tokens(REnv *env, MultiVal *stack, RToken *runnable, int runnable_len);

int execute_runnable(REnv *env, RToken *runnable, int runnable_len){
   MultiVal stack[1000];
   return execute_tokens(env, stack, runnable, runnable_len);
}

int execute_function(REnv *env, ShmFunc *func, MultiVal *stack, int *stack_head){
   log_str("CALLING", func->func_name);

   // to support recursion save off the values of the previous variables and return index
   
   // Push Arguments to the variables
   for(int i = 0; i < func->num_args; ++i){
      log_str("SET VARIABLE", func->args[i]);
      log_str("VARIABLE TYPE", shm_type_str(func->types[i]));
      insert_rmap(&env->variables, func->args[i], func->types[i], stack[*stack_head + i - func->num_args]);
   }
   int status = execute_tokens(env, stack + *stack_head, func->tokens, func->num_tokens);

   if(status < 0) exit(-1);

   log_msg("CLEANING UP FUNCTION CALL");
   // Pop args on the stack
   for(int i = 0; i < func->num_args; ++i){
      log_str("DELETE", func->args[i]);
      delete_rmap(&env->variables, func->args[i], NULL);
   }

   log_msg("RESET STACK");
   *stack_head = *stack_head - func->num_args;

   if(func->return_type != SHM_NONE){
      log_msg("SAVING RETURN");
      stack[*stack_head] = stack[status];
      *stack_head = *stack_head + 1;
   }

   log_int("STACK HEAD", *stack_head);

   log_msg("END FUNCTION CALL");

   return status;
}

int execute_tokens(REnv *env, MultiVal *stack, RToken *runnable, int runnable_len){
   log_set_file("executable.c");
   ShmObj *obj_ptr;
   int op_index = 0;
   int stack_head = 0;
   while(op_index < runnable_len){
      const RToken *curr = &runnable[op_index];
      switch(OP_MASK(curr->r_type)){
         case NUMBER:
            stack[stack_head++] = curr->as;
         break;
         case GT:
         case LT:
         case EQ:
         case PLUS:
         case MINUS:
         case MULT:
         case DIV:
         case MOD:
            perform_shm_operation(curr->r_type, &stack[stack_head - 2], stack[stack_head - 1]);
            stack_head--;
         break;
         case SET_WORD:
            ShmType word_type = WORD_IS_DBL(curr->r_type) ? SHM_DBL : SHM_INT;
            insert_rmap(&env->variables, curr->as.word, word_type, stack[stack_head - 1]);
            stack_head--;
         break;
         case WORD:
            if((obj_ptr = search_rmap(&env->variables, curr->as.word)) != NULL){
               log_obj("LOAD WORD", obj_ptr);
               stack[stack_head++] = obj_ptr->as;
            }else{
               fprintf(stderr, "Variable %d '%s' has not been set\n", op_index, curr->as.word);
               return 1;
            }
         break;
         case GOTO:
            op_index = curr->as.cond[0];
         break;
         case DUMP:
            log_str("DUMP", NUMBER_IS_FLT(curr->r_type) ? "Float" : "Integer");
            if(NUMBER_IS_FLT(curr->r_type)){
               printf("%f\n", stack[stack_head - 1].number);
            }else{
               printf("%ld\n", stack[stack_head - 1].decimal);
            }
            --stack_head;
         break;
         case IF:
            if(!is_truthy(stack[stack_head - 1])){
               op_index = curr->as.cond[0];
            }
            stack_head--;
         break;
         case CALL:
            ShmFunc *func_info = search_rmap(&env->funcs, curr->as.word)->as.func;
            execute_function(env, func_info, stack, &stack_head);
         break;
         case RETURN:
            if(RET_IS_SET(curr->r_type)){
               return stack_head;
            }
            return 0;
         break;
         case DEL_VAR:
            delete_rmap(&env->variables, curr->as.word, NULL);
         break;
         default:
            printf("[EXECUTABLE]: %s is not implemented\n", rtoken_str(curr));
            return 1;
      }
      op_index++;
   }

   return 0;
}


int is_truthy(MultiVal val){
   return (int)val.decimal != 0;
}

/* result is rhs */
void perform_shm_operation(TokenType op, MultiVal *lhs, MultiVal rhs){
   if(LHS_IS_DBL(op) && RHS_IS_DBL(op)){
      log_msg("FLOAT + FLOAT");
      log_float("LHS", lhs->number);
      log_float("RHS", rhs.number);
      switch(OP_MASK(op)){
         case GT:
            lhs->decimal = lhs->number > rhs.number;
         break;
         case LT:
            lhs->decimal = lhs->number < rhs.number;
         break;
         case EQ:
            lhs->decimal = lhs->number == rhs.number;
         break;
         case PLUS:
            lhs->number = lhs->number + rhs.number;
         break;
         case MINUS:
            lhs->number = lhs->number - rhs.number;
         break;
         case MULT:
            lhs->number = lhs->number * rhs.number;
         break;
         case DIV:
            lhs->number = lhs->number / rhs.number;
         break;
         case MOD:
            fprintf(stderr, "When computing the modulo of two numbers they cannot be floating point\n");
            exit(1);
         break;
         default:
            fprintf(stderr, "Invalid Operand\n");
            exit(1);
      }
   }else if(LHS_IS_DBL(op)){
      log_msg("FLOAT + INT");
      log_float("LHS", lhs->number);
      log_int("RHS", rhs.decimal);
      switch(OP_MASK(op)){
         case GT:
            lhs->decimal = lhs->number > rhs.decimal;
         break;
         case LT:
            lhs->decimal = lhs->number < rhs.decimal;
         break;
         case EQ:
            lhs->decimal = lhs->number == rhs.decimal;
         break;
         case PLUS:
            lhs->number = lhs->number + rhs.decimal;
         break;
         case MINUS:
            lhs->number = lhs->number - rhs.decimal;
         break;
         case MULT:
            lhs->number = lhs->number * rhs.decimal;
         break;
         case DIV:
            lhs->number = lhs->number / rhs.decimal;
         break;
         case MOD:
            fprintf(stderr, "When computing the modulo of two numbers they cannot be floating point\n");
            exit(1);
         break;
         default:
            fprintf(stderr, "Invalid Operand\n");
            exit(1);
      }
   }else if(RHS_IS_DBL(op)){
      log_msg("INT + FLOAT");
      log_int("LHS", lhs->decimal);
      log_float("RHS", rhs.number);
      switch(OP_MASK(op)){
         case GT:
            lhs->decimal = lhs->decimal > rhs.number;
         break;
         case LT:
            lhs->decimal = lhs->decimal < rhs.number;
         break;
         case EQ:
            lhs->decimal = lhs->decimal == rhs.number;
         break;
         case PLUS:
            lhs->number = lhs->decimal + rhs.number;
         break;
         case MINUS:
            lhs->number = lhs->decimal - rhs.number;
         break;
         case MULT:
            lhs->number = lhs->decimal * rhs.number;
         break;
         case DIV:
            lhs->number = lhs->decimal / rhs.number;
         break;
         case MOD:
            fprintf(stderr, "When computing the modulo of two numbers they cannot be floating point\n");
            exit(1);
         break;
         default:
            exit(1);
      }
   }else{
      log_msg("INT + INT");
      log_int("LHS", lhs->decimal);
      log_int("RHS", rhs.decimal);
      switch(OP_MASK(op)){
         case GT:
            lhs->decimal = lhs->decimal > rhs.decimal;
         break;
         case LT:
            lhs->decimal = lhs->decimal < rhs.decimal;
         break;
         case EQ:
            lhs->decimal = lhs->decimal == rhs.decimal;
         break;
         case PLUS:
            lhs->decimal = lhs->decimal + rhs.decimal;
         break;
         case MINUS:
            lhs->decimal = lhs->decimal - rhs.decimal;
         break;
         case MULT:
            lhs->decimal = lhs->decimal * rhs.decimal;
         break;
         case DIV:
            lhs->decimal = lhs->decimal / rhs.decimal;
         break;
         case MOD:
            lhs->decimal = lhs->decimal % rhs.decimal;
         break;
         default:
            fprintf(stderr, "Invalid Operand\n");
            exit(1);
      }
   }
}

void free_function(ShmFunc *func){
   free(func->func_name);
   for(int i = 0; i < func->num_args; ++i){
      free(func->args[i]);
   }
   free(func->args);

   free_rtokens(func->tokens, func->num_tokens);
   free(func->tokens);
   free(func);
}

void free_shm_function(ShmObj *func){
   free_function(func->as.func);
}

void free_rtokens(RToken *tokens, int num_tokens){
   return;
   for(int i = 0; i < num_tokens; ++i){
      switch(tokens[i].r_type){
         case SET_WORD:
         case WORD:
            printf("free(%s) at %p\n", tokens[i].as.word, tokens[i].as.word);
            free(tokens[i].as.word);
         break;
         default:
         break;
      }
   }
}

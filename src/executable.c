#include <shmag.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int is_truthy(MultiVal val);

void perform_shm_operation(TokenType op, ShmType shmType, MultiVal val, ShmObj *result);

int execute_tokens(REnv *env, MultiVal *stack, RToken *runnable, int runnable_len);

int execute_runnable(REnv *env, RToken *runnable, int runnable_len){
   MultiVal stack[1000];
   return execute_tokens(env, stack, runnable, runnable_len);
}

int execute_tokens(REnv *env, MultiVal *stack, RToken *runnable, int runnable_len){
   ShmObj *obj_ptr;
   ShmObj result;
   int op_index = 0;
   int stack_head = 0;
   while(op_index < runnable_len){
      const RToken *curr = &runnable[op_index];
      // printf("Token: %s\n", rtoken_str(curr));
      switch(OP_MASK(curr->r_type)){
         case NUMBER:
            stack[stack_head++] = curr->as;
         break;
         case INIT_SHM:
            perform_shm_operation(curr->r_type, SHM_NULL, stack[stack_head - 1], &result);
            stack_head--;
         break;
         case PUSH_SHM:
            stack[stack_head] = result.as;
            stack_head += 1;
         break;
         case GT:
         case LT:
         case EQ:
         case PLUS:
         case MINUS:
         case MULT:
         case DIV:
         case MOD:
            // printf("%s\n", rtoken_str(curr));
            perform_shm_operation(curr->r_type, SHM_DBL, stack[stack_head - 1], &result);
            stack_head--;
         break;
         case SET_WORD:
            ShmType word_type = WORD_IS_DBL(curr->r_type) ? SHM_DBL : SHM_INT;
            insert_rmap(&env->variables, curr->as.word, word_type, stack[stack_head - 1]);
            stack_head--;
         break;
         case WORD:
            if((obj_ptr = search_rmap(&env->variables, curr->as.word)) != NULL){
               stack[stack_head++] = obj_ptr->as;
            }else{
               fprintf(stderr, "Variable '%s' has not been set\n", curr->as.word);
               return 1;
            }
         break;
         case GOTO:
            // printf("Going to %d\n", curr->as.cond[0]);
            op_index = curr->as.cond[0];
         break;
         case DUMP:
            // 
            if(NUMBER_IS_FLT(curr->r_type)){
               printf("%f\n", stack[stack_head - 1].number);
            }else{
               printf("%ld\n", stack[stack_head - 1].decimal);
            }
            --stack_head;
         break;
         case IF:
            if(!is_truthy(stack[stack_head - 1])){
               // printf("%s\n", rtoken_str(curr));
               op_index = curr->as.cond[0];
            }
            stack_head--;
         break;
         default:
            printf("[EXECUTABLE]: %s is not implemented\n", rtoken_str(curr));
            return 1;
      }
      // printf("op_index = %d\n", op_index);
      op_index++;
   }

   return 0;
}


int is_truthy(MultiVal val){
   return (int)val.decimal != 0;
}

/* result is rhs */
void perform_shm_operation(TokenType op, ShmType shmType, MultiVal val, ShmObj *result){
   if(shmType == SHM_NULL){
      result->as = val;
      return;
   }

   if(LHS_IS_DBL(op) && RHS_IS_DBL(op)){
      switch(OP_MASK(op)){
         case GT:
            result->as.decimal = val.number > result->as.number;
         break;                                               
         case LT:                                             
            result->as.decimal = val.number < result->as.number;
         break;                                               
         case EQ:                                             
            result->as.decimal = val.number == result->as.number;
         break;                                               
         case PLUS:                                           
            result->as.number = val.number + result->as.number;
         break;                                               
         case MINUS:                                          
            result->as.number = val.number - result->as.number;
         break;                                               
         case MULT:                                           
            result->as.number = val.number * result->as.number;
         break;                                               
         case DIV:                                            
            result->as.number = val.number / result->as.number;
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
      switch(OP_MASK(op)){
         case GT:
            result->as.decimal = val.decimal > result->as.number;
         break;                                          
         case LT:                                        
            result->as.decimal = val.decimal < result->as.number;
         break;                                          
         case EQ:                                        
            result->as.decimal = val.decimal == result->as.number;
         break;                                                
         case PLUS:                                            
            result->as.number = val.decimal + result->as.number;
         break;                                                
         case MINUS:                                           
            result->as.number = val.decimal - result->as.number;
         break;                                                
         case MULT:                                            
            result->as.number = val.decimal * result->as.number;
         break;                                                
         case DIV:                                             
            result->as.number = val.decimal / result->as.number;
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
      switch(OP_MASK(op)){
         case GT:
            result->as.decimal = val.number > result->as.decimal;
         break;                              
         case LT:                            
            result->as.decimal = val.number < result->as.decimal;
         break;                              
         case EQ:                            
            result->as.decimal = val.number == result->as.decimal;
         break;                              
         case PLUS:                          
            result->as.number = val.number + result->as.decimal;
         break;                              
         case MINUS:                         
            result->as.number = val.number - result->as.decimal;
         break;                              
         case MULT:                          
            result->as.number = val.number * result->as.decimal;
         break;                              
         case DIV:                           
            result->as.number = val.number / result->as.decimal;
         break;
         case MOD:
            fprintf(stderr, "When computing the modulo of two numbers they cannot be floating point\n");
            exit(1);
         break;
         default:
            exit(1);
      }
   }else{
      switch(OP_MASK(op)){
         case GT:
            result->as.decimal = val.decimal > result->as.decimal;
         break;                                                  
         case LT:                                                
            result->as.decimal = val.decimal < result->as.decimal;
         break;                                                  
         case EQ:                                                
            result->as.decimal = val.decimal == result->as.decimal;
         break;                                                  
         case PLUS:                                              
            result->as.decimal = val.decimal + result->as.decimal;
         break;                                                  
         case MINUS:                                             
            result->as.decimal = val.decimal - result->as.decimal;
         break;                                                  
         case MULT:                                              
            result->as.decimal = val.decimal * result->as.decimal;
         break;                                                  
         case DIV:                                               
            result->as.decimal = val.decimal / result->as.decimal;
         break;
         case MOD:
            result->as.decimal = val.decimal % result->as.decimal;
         break;
         default:
            fprintf(stderr, "Invalid Operand\n");
            exit(1);
      }
   }
}

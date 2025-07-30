#include <shmag.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int is_truthy(MultiVal val);

void perform_shm_operation(TokenType op, ShmType shmType, MultiVal val, ShmObj *result);

int execute_runnable(REnv *env, RToken *runnable, int runnable_len){
   int op_index = 0;
   MultiVal stack[1000];
   ShmObj *obj_ptr;
   ShmObj result;
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
            // printf("%s[%d,%d]\n", rtoken_str(curr), LHS_IS_DBL(curr->r_type), RHS_IS_DBL(curr->r_type));
            perform_shm_operation(curr->r_type, DBL, stack[stack_head - 1], &result);
            stack_head--;
         break;
         case SET_WORD:
            insert_rmap(&env->variables, curr->as.word, DBL, stack[stack_head - 1]);
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
            printf("%f\n", stack[--stack_head].number);
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
   return (int)val.number != 0;
}

void perform_shm_operation(TokenType op, ShmType shmType, MultiVal val, ShmObj *result){
   if(shmType == SHM_NULL){
      result->as = val;
      return;
   }
   
   switch(OP_MASK(op)){
      case GT:
         result->as.number = result->as.number > val.number;
      break;
      case LT:
         result->as.number = result->as.number < val.number;
      break;
      case EQ:
         result->as.number = result->as.number == val.number;
      break;
      case PLUS:
         result->as.number = result->as.number + val.number;
      break;
      case MINUS:
         result->as.number = result->as.number - val.number;
      break;
      case MULT:
         result->as.number = result->as.number * val.number;
      break;
      case DIV:
         result->as.number = result->as.number / val.number;
      break;
      case MOD:
         if(floor(result->as.number) != result->as.number || floor(val.number) != val.number){
            fprintf(stderr, "When computing the modulo of two numbers they cannot be floating point\n");
            exit(1);
         }
         result->as.number = (long)result->as.number % (long)val.number;
      break;
      default:
         fprintf(stderr, "Invalid Operand\n");
         exit(1);
   }
}

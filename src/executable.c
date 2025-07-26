#include <shmag.h>
#include <stdio.h>
#include <stdlib.h>

int is_truthy(MultiVal val);

int execute_runnable(REnv *env, RToken *runnable, int runnable_len){
   int op_index = 0;
   MultiVal stack[1000];
   MultiVal *multi_ptr;
   int stack_head = 0;
   while(op_index < runnable_len){
      const RToken *curr = &runnable[op_index];

      switch(curr->r_type){
         case NUMBER:
            stack[stack_head++] = curr->as;
         break;
         case GT:
            stack[stack_head - 2].number = stack[stack_head - 2].number > stack[stack_head - 1].number;
            stack_head--;
         break;
         case LT:
            stack[stack_head - 2].number = stack[stack_head - 2].number < stack[stack_head - 1].number;
            stack_head--;
         break;
         case EQ:
            // printf("%f %f, %d\n", stack[stack_head - 2].number, stack[stack_head - 1].number, stack[stack_head - 2].number == stack[stack_head - 1].number);
            stack[stack_head - 2].number = stack[stack_head - 2].number == stack[stack_head - 1].number;
            stack_head--;
         break;
         case PLUS:
            stack[stack_head - 2].number = stack[stack_head - 2].number + stack[stack_head - 1].number;
            stack_head--;
         break;
         case MINUS:
            stack[stack_head - 2].number = stack[stack_head - 2].number - stack[stack_head - 1].number;
            stack_head--;
         break;
         case MULT:
            stack[stack_head - 2].number = stack[stack_head - 2].number * stack[stack_head - 1].number;
            stack_head--;
         break;
         case DIV:
            stack[stack_head - 2].number = stack[stack_head - 2].number / stack[stack_head - 1].number;
            stack_head--;
         break;
         case SET_WORD:
            insert_rmap(&env->variables, curr->as.word, stack[stack_head - 1]);
            stack_head--;
         break;
         case WORD:
            if((multi_ptr = search_rmap(&env->variables, curr->as.word)) != NULL){
               stack[stack_head++] = *multi_ptr;
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

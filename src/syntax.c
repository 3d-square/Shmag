#include <shmag.h>
#include <stdio.h>
#include <stdarg.h>

enum extected_types{
   E_OP    = 1 << 0,
   E_WORD  = 1 << 1,
   E_VALUE = 1 << 2,
};

int expect_type(const PToken *tokens, int index, int types);
void token_errorf(const char *fmt, const PToken *token, ...);

int validate_syntax(PToken *tokens, int num_tokens){
   int i = 0;
   int error = 0;

   while(i < num_tokens){
      const PToken *curr_token = &tokens[i++];
      print_ptoken(curr_token);
      printf("\n");

      switch(curr_token->p_type){
         // Skip over WORD/VALUE
         case WORD:
         case NUMBER:
         break;

         case SET:
            if(i >= num_tokens || !expect_type(tokens, i, E_WORD | E_VALUE)){
               token_errorf("Set operator expects a WORD or VALUE after the '='", &tokens[i]);
               error = 1;
            }
            if(i < 2 || !expect_type(tokens, i, E_WORD)){
               token_errorf("Set operator expects a WORD before the '='", &tokens[i]);
               error = 1;
            }

            if(error) return -1;
         break;

         case GT:
         case LT:
         case EQ:
             // Check rhs
             if(i >= num_tokens || !expect_type(tokens, i, E_WORD | E_VALUE)){
               token_errorf("WORD or VALUE expected after operator", &tokens[i - 1]);
               error = 1;
            }

            // check lhs
            if(i < 2 || !expect_type(tokens, i - 2, E_WORD | E_VALUE)){
               token_errorf("WORD or VALUE expected before operator", &tokens[i - 1]);
               error = 1;
            }

            if(error) return -1;
         break;
   
         default:
            token_errorf("Unimplemented token", curr_token);
            return -1;
         break;
      }
   }
   

   return 0;
}

int expect_type(const PToken *tokens, int index, int types){
   if(types & E_OP){
      switch(tokens[index].p_type){
         case GT:
         case EQ:
         case LT:
            return 1;
         break;
         // Do nothing when it is not an operator type
         default:
         break;
      }
   }

   if(types & E_WORD){
      switch(tokens[index].p_type){
         case WORD:
            return 1;
         break;
         // Do nothing when it is not an operator type
         default:
         break;
      }
   }

   if(types & E_VALUE){
      switch(tokens[index].p_type){
         case NUMBER:
            return 1;
         break;
         // Do nothing when it is not an operator type
         default:
         break;
      }
   }  
   
   return 0;
}

void token_errorf(const char *fmt, const PToken *token, ...){
   char buffer[1024];

   sprintf(buffer, "%s: %s\n", ptoken_str(token), fmt);
   va_list args;
   va_start(args, token);
   vfprintf(stderr, buffer, args);
   va_end(args);
}



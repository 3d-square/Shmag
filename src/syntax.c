#include <shmag.h>
#include <stdio.h>
#include <stdarg.h>

enum extected_types{
   E_OP    = 1 << 0,
   E_WORD  = 1 << 1,
   E_VALUE = 1 << 2,
};

int expect_type(const PToken *tokens, int index, int types);

int validate_syntax(PToken *tokens, int num_tokens){
   int i = 0;
   int error = 0;
   int nested_cond = 0;

   while(i < num_tokens){
      const PToken *curr_token = &tokens[i++];

      switch(curr_token->p_type){
         // Skip over WORD/VALUE
         case WORD:
         case NUMBER:
         break;

         case SET:
            if(i >= num_tokens || !expect_type(tokens, i, E_WORD | E_VALUE)){
               token_errorf("Set operator expects a WORD or VALUE after the ':='", &tokens[i - 1]);
               error = 1;
            }
            if(i < 2 || !expect_type(tokens, i - 2, E_WORD)){
               token_errorf("Set operator expects a WORD before the ':='", &tokens[i]);
               error = 1;
            }

         break;

         case GT:
         case LT:
         case EQ:
         case PLUS:
         case MINUS:
         case MULT:
         case DIV:
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
         break;
         case DUMP:
            if(i + 1 >= num_tokens){
               token_errorf("dump expects tokens to follow", &tokens[i - 1]);
               error = 1;
            }
            if(!error && !expect_type(tokens, i, E_WORD | E_VALUE)){
               token_errorf("Cannot print type", &tokens[i]);
               error = 1;
            }
         break;
         case IF:
            if(i + 1 >= num_tokens || !expect_type(tokens, i, E_WORD | E_VALUE)){
               token_errorf("If Statement expects a value", &tokens[i - 1]);
               error = 1;
            }else{
               int end_match = 0;
               nested_cond++;
               for(int j = i; j < num_tokens; ++j){
                  curr_token = &tokens[j];
                  if(curr_token->p_type == IF){
                     end_match++;
                  }else if(curr_token->p_type == END){
                     end_match--;
                  }

                  if(end_match < 0){
                     break;
                  }
               }

               error = end_match >= 0;
               if(error){
                  token_errorf("If Statement expects an 'end'", &tokens[i - 1]);
               }
            }
            
         break;

         case END:
            nested_cond--;
            if(nested_cond < 0){
               token_errorf("Found 'end' without a matching 'if' statement", &tokens[i - 1]);
               error = 1;
            }
         break;

         case LINE_SEP:
         break;
   
         default:
            token_errorf("Unimplemented token", curr_token);
            error = 1;
         break;
      }
      if(error) return -1;
   }
   

   return 0;
}

int expect_type(const PToken *tokens, int index, int types){
   if(types & E_OP){
      switch(tokens[index].p_type){
         case GT:
         case EQ:
         case LT:
         case PLUS:
         case MULT:
         case DIV:
         case MINUS:
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

void _token_errorf(const char *file, const char *func, int line, const char *fmt, const PToken *token, ...){
   char buffer[1024];

   sprintf(buffer, "%s::%s[%d] - %s: %s\n", file, func, line, ptoken_str(token), fmt);
   va_list args;
   va_start(args, token);
   vfprintf(stderr, buffer, args);
   va_end(args);
}



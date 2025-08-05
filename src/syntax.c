#include <shmag.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

enum extected_types{
   E_OP    = 1 << 0,
   E_WORD  = 1 << 1,
   E_VALUE = 1 << 2,
   E_SEP   = 1 << 3,
   E_LINE  = 1 << 4,
   E_OPEN  = 1 << 5,
   E_CLOSE = 1 << 6,
};

int expect_type(const PToken *tokens, int index, int types);

int validate_syntax(PToken *tokens, int num_tokens){
   int i = 0;
   int error = 0;
   int func_header = 0;
   int func_call = 0;
   TokenType nested[256];
   int num_nested = 0;

   while(i < num_tokens){
      const PToken *curr_token = &tokens[i++];

      switch(OP_MASK(curr_token->p_type)){
         // Skip over WORD/VALUE
         case WORD:
         case NUMBER:
         break;

         case SET:
            if(i >= num_tokens || !expect_type(tokens, i, E_WORD | E_VALUE)){
               token_errorf("Set operator expects a WORD or VALUE after the ':='", curr_token);
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
         case MOD:
         case MULT:
         case DIV:
             // Check rhs
             if(i >= num_tokens || !expect_type(tokens, i, E_WORD | E_VALUE)){
               token_errorf("WORD or VALUE expected after operator", curr_token);
               error = 1;
            }

            // check lhs
            if(i < 2 || !expect_type(tokens, i - 2, E_WORD | E_VALUE)){
               token_errorf("WORD or VALUE expected before operator", curr_token);
               error = 1;
            }
         break;
         case DUMP:
            if(i + 1 >= num_tokens){
               token_errorf("dump expects tokens to follow", curr_token);
               error = 1;
            }
            if(!error && !expect_type(tokens, i, E_WORD | E_VALUE)){
               token_errorf("Cannot print type", &tokens[i]);
               error = 1;
            }
         break;
         case WHILE:
         case IF:
            nested[num_nested++] = curr_token->p_type;
            if(i + 1 >= num_tokens || !expect_type(tokens, i, E_WORD | E_VALUE)){
               token_errorf("If Statement expects a value", curr_token);
               error = 1;
            }else{
               int end_match = 0;
               for(int j = i; j < num_tokens; ++j){
                  curr_token = &tokens[j];
                  if(curr_token->p_type == IF || curr_token->p_type == WHILE){
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
                  token_errorf("If Statement expects an 'end'", curr_token);
               }
            }
            
         break;
         case ELIF:
         case ELSE:
            if(num_nested <= 0){
               token_errorf("Found 'else' without a matching 'if' statement", curr_token);
               error = 1;
            }else{
               if(nested[num_nested - 1] != IF && nested[num_nested - 1] != ELIF){
                  fprintf(stderr, "else/elif statement expects if statement preceeding \n");
                  error = 1;
               }
            }
         break;
         case END:
            num_nested--;
            if(num_nested < 0){
               token_errorf("Found 'end' without a matching control statement", curr_token);
               error = 1;
            }
         break;

         case LINE_SEP:
         break;

         case PROTO_FUNC:
         case FUNC:
            if(curr_token->p_type == FUNC || curr_token->p_type == PROTO_FUNC) func_header = 1;
            if(curr_token->p_type == FUNC) nested[num_nested++] = curr_token->p_type;
            if(i + 1 >= num_tokens || !expect_type(tokens, i, E_WORD)){
               token_errorf("func expects word\n", curr_token);
               error = 1;
            }
            if(i + 2 >= num_tokens || !expect_type(tokens, i + 1, E_OPEN)){
               token_errorf("func expects '('\n", curr_token);
               error = 1;
            }
            if(i + 3 >= num_tokens || !expect_type(tokens, i + 2, E_WORD | E_CLOSE)){
               token_errorf("func expects word or ')'\n", curr_token);
               error = 1;
            }

         break;
         case PAREN_OPEN:
            if(func_header == 1){
               if(i >= num_tokens || !expect_type(tokens, i, E_WORD | E_CLOSE)){
                  token_errorf("WORD or ')' expected after '('", curr_token);
                  error = 1;
               }else{
                  // Check from closing paren
                  int j = i;
                  int nested_paren = 1;
                  while(j < num_tokens && tokens[j].p_type != LINE_SEP){
                     if(tokens[j].p_type == PAREN_OPEN) nested_paren++;
                     else if(tokens[j].p_type == PAREN_CLOSE) nested_paren--;
                     j++;
                  }

                  if(nested_paren != 0){
                     token_errorf("Mismatched number of '(' and ')' on the same line\n", curr_token);
                     error = 1;
                  }
               }
            }else if(i > 1 && expect_type(tokens, i - 2, E_WORD)){
               if(i >= num_tokens || !expect_type(tokens, i, E_WORD | E_CLOSE | E_VALUE)){
                  token_errorf("WORD or ')' expected after '('", curr_token);
                  error = 1;
               }else{
                  // Check from closing paren
                  int j = i;
                  int nested_paren = 1;
                  while(j < num_tokens && tokens[j].p_type != LINE_SEP){
                     if(tokens[j].p_type == PAREN_OPEN) nested_paren++;
                     else if(tokens[j].p_type == PAREN_CLOSE) nested_paren--;
                     j++;
                  }

                  if(nested_paren != 0){
                     token_errorf("Mismatched number of '(' and ')' on the same line\n", curr_token);
                     error = 1;
                  }
                  tokens[i - 1].p_type = CALL;
                  func_call = 1;
               }
            }else{
               token_errorf("Parentheses are not currently supported outside of functions calls", curr_token);
               error = 1;
            }
         break;
         // Checked for in paren open
         case PAREN_CLOSE:
            if(func_header == 1){
               func_header = 0;
            }else if(func_call == 1){
               func_call = 0;
            }else{
               token_errorf("Parentheses are not currently supported outside of functions calls", curr_token);
               error = 1;
            }
         break;
         case EXPR_SEP:
            if(func_header == 1 /* || nested[num_nested - 1] != FUNC_CALL */){
               if(i + 1 >= num_tokens || !expect_type(tokens, i, E_WORD)){
                  token_errorf("seperator expects word\n", curr_token);
                  error = 1;
               }

               if(i + 2 >= num_tokens || !expect_type(tokens, i + 1, E_LINE | E_SEP)){
                  token_errorf("Seperator expects either EOL or seperator after following word\n", &tokens[i]);
                  error = 1;
               }
            }else{
               token_errorf("',' is only supported for function definitions\n", curr_token);
            }
            
         break;
   
         default:
            token_errorf("Unimplemented token", curr_token);
            error = 1;
         break;
      }
      if(error) return -1;
      // printf("%s: %d\n", ptoken_str(curr_token), num_nested);
   }
   
   if(num_nested > 0){
      for(int i = 0; i < num_nested; ++i){
         printf("%d\n", nested[i]);
      }
      fprintf(stderr, "Not all blocks have been closed\n");
      return -1;
   }

   return 0;
}

int expect_type(const PToken *tokens, int index, int types){
   if(types & E_OP){
      switch(OP_MASK(tokens[index].p_type)){
         case GT:
         case EQ:
         case LT:
         case PLUS:
         case MULT:
         case DIV:
         case MOD:
         case MINUS:
            return 1;
         break;
         // Do nothing when it is not an operator type
         default:
         break;
      }
   }

   if(types & E_WORD){
      switch(OP_MASK(tokens[index].p_type)){
         case WORD:
            return 1;
         break;
         // Do nothing when it is not an operator type
         default:
         break;
      }
   }

   if(types & E_VALUE){
      switch(OP_MASK(tokens[index].p_type)){
         case NUMBER:
            return 1;
         break;
         // Do nothing when it is not an operator type
         default:
         break;
      }
   }  

   if(types & E_SEP){
      if(tokens[index].p_type == EXPR_SEP) return 1;
   }
 
   if(types & E_LINE){
      if(tokens[index].p_type == LINE_SEP) return 1;
   }  

   if(types & E_OPEN){
      if(tokens[index].p_type == PAREN_OPEN) return 1;
   }

   if(types & E_CLOSE){
      if(tokens[index].p_type == PAREN_CLOSE) return 1;
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



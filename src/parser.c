#include <stdio.h>
#include <shmag.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

const char *skip_whitespace(const char *line){
   const char *tmp = line;
   while(*tmp && *tmp != '\n' && isspace(*tmp)){
      tmp = tmp + 1;
   }

   return tmp;
}

int has_next_token(const char *line){
   const char *next = skip_whitespace(line);
   return *next && *next != '\n';
}

void set_word_type(PToken *token){
   token->p_type = WORD;
   if(strcmp(token->as.word, "==") == 0){
      token->p_type = EQ;
   }else if(strcmp(token->as.word, ">") == 0){
      token->p_type = GT;
   }else if(strcmp(token->as.word, "<") == 0){
      token->p_type = LT;
   }else if(strcmp(token->as.word, ":=") == 0){
      token->p_type = SET;
   }else if(strcmp(token->as.word, "+") == 0){
      token->p_type = PLUS;
   }else if(strcmp(token->as.word, "-") == 0){
      token->p_type = MINUS;
   }else if(strcmp(token->as.word, "*") == 0){
      token->p_type = MULT;
   }else if(strcmp(token->as.word, "/") == 0){
      token->p_type = DIV;
   }else if(strcmp(token->as.word, ".") == 0){
      token->p_type = DUMP;
   }else if(strcmp(token->as.word, "if") == 0){
      token->p_type = IF;
   }else if(strcmp(token->as.word, "end") == 0){
      token->p_type = END;
   }else if(strcmp(token->as.word, "while") == 0){
      token->p_type = WHILE;
   }else{
      char *end;
      double dbl = strtod(token->as.word, &end);
      if(*end == '\0'){
         free(token->as.word);
         token->as.number = dbl;
         token->p_type = NUMBER;
      }
   }

   if(token->p_type != WORD && token->p_type != NUMBER){
      free(token->as.word);
   }
}

PToken next_token(const char *line, int curr_line, int *curr_col){

   line = line + *curr_col;
   PToken new_token = {
      .line = curr_line + 1, // add one for 0 index
   };

   int token_len = 0;
   const char *token_start = skip_whitespace(line);
   *curr_col = *curr_col + token_start - line;

   new_token.col = *curr_col  + 1;// add once for 0 index
   // Loop through token types and make them, currently everything is seperated by spaces
   while(token_start[token_len] && !isspace(token_start[token_len])){
      token_len += 1;
   }

   // Allocate memory and copy the new token into it
   new_token.as.word = calloc(1, sizeof(char) * (token_len + 1));
   if(new_token.as.word == NULL){
      exit(1);
   }
   strncpy(new_token.as.word, token_start, token_len);

   set_word_type(&new_token);

   *curr_col = *curr_col + token_len;

   return new_token;
}

/* returns start of next line or NULL */
const char *parse_line_as_tokens(const char *line, PToken *tokens, int *num_tokens, int line_number){
   int column = 0;
   while(has_next_token(line + column)){
      tokens[*num_tokens] = next_token(line, line_number, &column);
      *num_tokens = *num_tokens + 1;
   }

   tokens[*num_tokens] = (PToken){
      .p_type = LINE_SEP,
      .line = line_number,
   };

   *num_tokens = *num_tokens + 1;

   line = strchr(line + column, '\n');
   if(line != NULL){
      if(*(line + 1) == '\0'){
         return NULL;
      }else{
         return line + 1;
      }
   }else{
      return NULL;
   }
}

void parse_as_tokens(const char *lines, PToken *tokens, int *num_tokens){
   int line = 0;
   *num_tokens = 0;
   while((lines = parse_line_as_tokens(lines, tokens, num_tokens, line))){
      line += 1;
   }
}

const char *ptoken_str(const PToken *ptoken){
   static char _buffer[512];

   switch(ptoken->p_type){
      case WORD:
         sprintf(_buffer, "Word[%d:%d](%s)", ptoken->line, ptoken->col, ptoken->as.word);
      break;
      case NUMBER:
         sprintf(_buffer, "Number[%d:%d](%f)", ptoken->line, ptoken->col, ptoken->as.number);
      break;
      case EQ:
         sprintf(_buffer, "Equals[%d:%d]", ptoken->line, ptoken->col);
      break;
      case GT:
         sprintf(_buffer, "GreaterThan[%d:%d]", ptoken->line, ptoken->col);
      break;
      case LT:
         sprintf(_buffer, "LessThan[%d:%d]", ptoken->line, ptoken->col);
      break;
      case SET:
         sprintf(_buffer, "Set[%d:%d]", ptoken->line, ptoken->col);
      break;
      case PLUS:
         sprintf(_buffer, "Plus[%d:%d]", ptoken->line, ptoken->col);
      break;
      case MINUS:
         sprintf(_buffer, "Minus[%d:%d]", ptoken->line, ptoken->col);
      break;
      case MULT:
         sprintf(_buffer, "Mult[%d:%d]", ptoken->line, ptoken->col);
      break;
      case DIV:
         sprintf(_buffer, "Divide[%d:%d]", ptoken->line, ptoken->col);
      break;
      case DUMP:
         sprintf(_buffer, "Dump");
      break;
      case IF:
         sprintf(_buffer, "If");
      break;
      case END:
         sprintf(_buffer, "End");
      break;
      case LINE_SEP:
         sprintf(_buffer, "line_sep");
      break;
      case GOTO:
         sprintf(_buffer, "goto");
      break;
      case WHILE:
         sprintf(_buffer, "While");
      break;
      case SET_WORD:
         sprintf(_buffer, "SetWord(%s)", ptoken->as.word);
      break;
   }

   return _buffer;
}

void print_ptoken(const PToken *ptoken){
   printf("%s", ptoken_str(ptoken));
}

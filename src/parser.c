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

int is_seperator(char c){
   return strchr("_+-=:<>?,/!@#$%^&*() \n\t\r", c) != NULL;
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
   }else if(strcmp(token->as.word, ")") == 0){
      token->p_type = PAREN_CLOSE;
   }else if(strcmp(token->as.word, "(") == 0){
      token->p_type = PAREN_OPEN;
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
   }else if(strcmp(token->as.word, "%") == 0){
      token->p_type = MOD;
   }else if(strcmp(token->as.word, "if") == 0){
      token->p_type = IF;
   }else if(strcmp(token->as.word, "end") == 0){
      token->p_type = END;
   }else if(strcmp(token->as.word, "else") == 0){
      token->p_type = ELSE;
   }else if(strcmp(token->as.word, "elif") == 0){
      token->p_type = ELIF;
   }else if(strcmp(token->as.word, "while") == 0){
      token->p_type = WHILE;
   }else if(strcmp(token->as.word, "func") == 0){
      token->p_type = FUNC;
   }else if(strcmp(token->as.word, "proto") == 0){
      token->p_type = PROTO_FUNC;
   }else if(strcmp(token->as.word, ",") == 0){
      token->p_type = EXPR_SEP;
   }else if(strcmp(token->as.word, "int") == 0){
      token->p_type = TYPE_INT;
   }else if(strcmp(token->as.word, "float") == 0){
      token->p_type = TYPE_FLOAT;
   }else if(strcmp(token->as.word, "none") == 0){
      token->p_type = TYPE_NONE;
   }else if(strcmp(token->as.word, "->") == 0){
      token->p_type = ARROW;
   }else{
      char *end;
      double dbl = strtod(token->as.word, &end);
      if(*end == '\0'){
         int is_float = strchr(token->as.word, '.') != NULL;
         token->p_type = NUMBER;
         free(token->as.word);
         if(is_float){
            token->as.number = dbl;
            token->p_type = token->p_type | NUMBER_FLT;
         }else{
            token->as.decimal = (long)dbl;
         }
      }
   }

   if(OP_MASK(token->p_type) != WORD && OP_MASK(token->p_type) != NUMBER){
      free(token->as.word);
   }

   log_ptoken("PARSED", token);
}

int is_token(const char *str){
   if(strncmp(str, ":=", 2) == 0) return 2;
   if(strncmp(str, "==", 2) == 0) return 2;
   if(strncmp(str, "->", 2) == 0) return 2;

   if(strncmp(str, ">", 1) == 0) return 1;
   if(strncmp(str, "(", 1) == 0) return 1;
   if(strncmp(str, ")", 1) == 0) return 1;
   if(strncmp(str, "<", 1) == 0) return 1;
   if(strncmp(str, "*", 1) == 0) return 1;
   if(strncmp(str, "/", 1) == 0) return 1;
   if(strncmp(str, "%", 1) == 0) return 1;
   if(strncmp(str, ",", 1) == 0) return 1;
   if(strncmp(str, "<", 1) == 0) return 1;
   if(strncmp(str, ".", 1) == 0) return 1;
   if(strncmp(str, "-", 1) == 0 && !isdigit(str[1])) return 1;
   if(strncmp(str, "+", 1) == 0 && !isdigit(str[1])) return 1;
   return 0;
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

   if((token_len = is_token(token_start)) == 0){
      // Loop through token types and make them, currently everything is seperated by spaces
      token_len = 1;
      while(token_start[token_len] && !is_seperator(token_start[token_len])){
         token_len += 1;
      }
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
   log_set_file("parser.c");
   log_msg("START PARSING"); 
   int line = 0;
   *num_tokens = 0;
   while((lines = parse_line_as_tokens(lines, tokens, num_tokens, line))){
      line += 1;
   }
   log_msg("");
}

const char *ptoken_str(const PToken *ptoken){
   static char _buffer[512];

   switch(OP_MASK(ptoken->p_type)){
      case WORD:
         sprintf(_buffer, "Word(%s)", ptoken->as.word);
      break;
      case NUMBER:
         if(NUMBER_IS_FLT(ptoken->p_type)){
            sprintf(_buffer, "Float(%f)", ptoken->as.number);
         }else{
            sprintf(_buffer, "Integer(%ld)", ptoken->as.decimal);
         }
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
      case SET:
         sprintf(_buffer, "Set");
      break;
      case PLUS:
         sprintf(_buffer, "Plus");
      break;
      case MINUS:
         sprintf(_buffer, "Minus");
      break;
      case MULT:
         sprintf(_buffer, "Mult");
      break;
      case DIV:
         sprintf(_buffer, "Divide");
      break;
      case DUMP:
         sprintf(_buffer, "Dump");
      break;
      case FUNC:
         sprintf(_buffer, "Function");
      break;
      case PROTO_FUNC:
         sprintf(_buffer, "Prototype");
      break;
      case EXPR_SEP:
         sprintf(_buffer, "Expr Sep");
      break;
      case IF:
         sprintf(_buffer, "If");
      break;
      case ELSE:
         sprintf(_buffer, "Else");
      break;
      case ELIF:
         sprintf(_buffer, "Elif");
      break;
      case END:
         sprintf(_buffer, "End");
      break;
      case MOD:
         sprintf(_buffer, "Mod");
      break;
      case PAREN_OPEN:
         sprintf(_buffer, "Open");
      break;
      case PAREN_CLOSE:
         sprintf(_buffer, "Close");
      break;
      case LINE_SEP:
         sprintf(_buffer, "NewLine");
      break;
      case GOTO:
         sprintf(_buffer, "Goto");
      break;
      case WHILE:
         sprintf(_buffer, "While");
      break;
      case SET_WORD:
         sprintf(_buffer, "SetWord(%s)", ptoken->as.word);
      break;
      case TYPE_INT:
         sprintf(_buffer, "TypeInt");
      break;
      case TYPE_FLOAT:
         sprintf(_buffer, "TypeFloat");
      break;
      case TYPE_NONE:
         sprintf(_buffer, "TypeNone");
      break;
      case ARROW:
         sprintf(_buffer, "Arrow");
      break;
      default:
         return "Unimplemented";
      break;
   }

   return _buffer;
}

void print_ptoken(const PToken *ptoken){
   printf("%s", ptoken_str(ptoken));
}

#include <stdio.h>
#include <shmag.h>
#include <string.h>
#include <stdlib.h>

void log_executable(const char *name, RToken *exe, int len){
   log_str("\nEXECUTABLE", name);
   for(int i = 0; i < len; ++i){
      log_int("INDEX", i);
      log_rtoken("TOKEN", &exe[i]);
   }
   log_msg("END EXECUTABLE\n");
}

void print_im(PToken *exe, int len){
   for(int i = 0; i < len; ++i){
      printf("%d: %s\n", i, ptoken_str(&exe[i]));
   }
}

void read_to_buffer(const char *file_name, char *buffer, int max_length){
   FILE *f = fopen(file_name, "r");
   if(f == NULL){
      printf("Could not open file '%s'\n", file_name);
      exit(1);
   }

   fseek(f, 0, SEEK_END);
   long length = ftell(f);
   fseek(f, 0, SEEK_SET);

   if(length >= max_length){
      printf("Allocate more memory loser\n");
      exit(1);
   }

   fread(buffer, sizeof(char), length, f);
   buffer[length] = '\0';
   
   fclose(f);
}

int word_seperated(const char *line, const char *word){
   const char *found = strstr(line, word);

   if(found){
      int end_sep = is_seperator(*(found + strlen(word)));
      if(found == line && end_sep){
         return 1;
      }else if( end_sep && is_seperator(*(found - 1))){
         return 1;
      }
   }
   
   return 0;
}

void read_idle_input(char _buffer[8192]){
   char line[256];
   int open_scope = 0;
   _buffer[0] = '\0';

   printf(">>> ");
   fflush(stdout);

   while(1){
      fgets(line, sizeof(line), stdin);
      if(word_seperated(line, "if") || word_seperated(line, "elif") || word_seperated(line, "else") || word_seperated(line, "while") || word_seperated(line, "func")){
         open_scope++;
      }else if(word_seperated(line, "end")){
         open_scope--;
      }
      strcat(_buffer, line);
      if(open_scope == 0) break;

      printf("    ");
      fflush(stdout);
   }
}

int main(int argc, char **argv){
   int num_tokens;
   int runnable_len;
   PToken tokens[1000];
   RToken runnable[1000];
   REnv env = {0};
   char buffer[8192] = {0};
   char file_name[64];

   log_start();
   if(argc < 2){
      while(1){
         read_idle_input(buffer);
         num_tokens = 0;
         if(strcmp(buffer, "exit\n") == 0){
            break;
         }else if(strncmp(buffer, "!load ", 6) == 0){
            strcpy(file_name, buffer + 6);
            file_name[strlen(file_name) - 1] = '\0';
            read_to_buffer(file_name, buffer, 8192);
         }
   
         parse_as_tokens(buffer, tokens, &num_tokens);

         if(validate_syntax(tokens, num_tokens) != -1){
            if(build_runnable(tokens, num_tokens, &env, runnable, &runnable_len) != -1){
               execute_runnable(&env, runnable, runnable_len);
            }
         }
      }
      destroy_rmap(&env.funcs, free_shm_function);
   }else{
      read_to_buffer(argv[1], buffer, 8192);

      parse_as_tokens(buffer, tokens, &num_tokens);

      if(validate_syntax(tokens, num_tokens) != -1){
         if(build_runnable(tokens, num_tokens, &env, runnable, &runnable_len) != -1){
            log_executable(argv[1], runnable, runnable_len);
            execute_runnable(&env, runnable, runnable_len);
            destroy_rmap(&env.funcs, free_shm_function);
            free_rtokens(runnable, runnable_len);
         }else{
            return 1;
         }
      }else{
         return 1;
      }
      log_stop();
   }

   return 0;
}

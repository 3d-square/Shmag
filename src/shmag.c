#include <stdio.h>
#include <shmag.h>
#include <string.h>
#include <stdlib.h>

void print_executable(const char *name, RToken *exe, int len){
   char buffer[256];
   sprintf(buffer, "%s.im", name);
   FILE *fp = stdout; // fopen(buffer, "w");
   for(int i = 0; i < len; ++i){
      fprintf(fp, "%d: %s\n", i, rtoken_str(&exe[i]));
   }
   if(fp != stdout && fp != stderr) fclose(fp);
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
      printf(">>> ");
      fflush(stdin);
      while(fgets(buffer, sizeof(buffer), stdin)){
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

         printf(">>> ");
         fflush(stdin);
      }
      destroy_rmap(&env.funcs, free_shm_function);
   }else{
      read_to_buffer(argv[1], buffer, 8192);

      parse_as_tokens(buffer, tokens, &num_tokens);

      if(validate_syntax(tokens, num_tokens) != -1){
         printf("Syntax Validated\n");
         // print_im(tokens, num_tokens);
         if(build_runnable(tokens, num_tokens, &env, runnable, &runnable_len) != -1){
            printf("Executable was built\n\n");
            print_executable(argv[1], runnable, runnable_len);
            execute_runnable(&env, runnable, runnable_len);
            destroy_rmap(&env.funcs, free_shm_function);
            free_rtokens(runnable, runnable_len);
         }else{
            printf("Unable to create an executable\n");
         }
      }else{
         printf("Unable to validate syntax\n");
      }
      log_stop();
     
   }

   return 0;
}

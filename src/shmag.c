#include <stdio.h>
#include <shmag.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char **argv){

   int num_tokens;
   int runnable_len;
   PToken tokens[1000];
   RToken runnable[1000];
   REnv env = {0};

   if(argc < 2){
      char buffer[256];
      printf(">>> ");
      fflush(stdin);
      while(fgets(buffer, sizeof(buffer), stdin)){
         num_tokens = 0;
         if(strcmp(buffer, "exit\n") == 0){
            break;
         }
   
         parse_as_tokens(buffer, tokens, &num_tokens);

         if(validate_syntax(tokens, num_tokens) != -1){
            if(build_runnable(tokens, num_tokens, runnable, &runnable_len) != -1){
               execute_runnable(&env, runnable, runnable_len);
               /* printf("%d\n", runnable_len);
               for(int i = 0; i < runnable_len; ++i){
                  print_rtoken(&runnable[i]);
                  printf("\n");
                  if(runnable[i].r_type == WORD){
                     free(runnable[i].as.word);
                  }
               } */
            }
         }

         printf(">>> ");
         fflush(stdin);
      }
   }else{
      char file_buffer[8192];
   
      FILE *f = fopen(argv[1], "r");
      if(f == NULL){
         printf("Could not open file '%s'\n", argv[1]);
         return 1;
      }

      fseek(f, 0, SEEK_END);
      long length = ftell(f);
      fseek(f, 0, SEEK_SET);

      fread(file_buffer, sizeof(char), length, f);
   
      fclose(f);

      parse_as_tokens(file_buffer, tokens, &num_tokens);

      if(validate_syntax(tokens, num_tokens) != -1){
         printf("Syntax Validated\n");
         if(build_runnable(tokens, num_tokens, runnable, &runnable_len) != -1){
            printf("Executable was built\n\n");
            execute_runnable(&env, runnable, runnable_len);
         }else{
            printf("Unable to create an executable\n");
         }
      }else{
         printf("Unable to validate syntax");
      }
      
     
   }

   return 0;
}

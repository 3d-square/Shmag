#include <stdio.h>
#include <shmag.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char **argv){

   int num_tokens;
   PToken tokens[1000];

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
         validate_syntax(tokens, num_tokens);
   
         for(int i = 0; i < num_tokens; ++i){
            if(tokens[i].p_type == WORD){
               free(tokens[i].as.word);
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
   
      for(int i = 0; i < num_tokens; ++i){
         printf("Token: ");
         print_ptoken(&tokens[i]);
         printf("\n");
      }
     
   }

   return 0;
}

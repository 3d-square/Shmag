#include <shmag.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

SNode *node_search_set(SSet *set, const char *key, int *hash){
   int bucket = hash_function(key);
   *hash = bucket;

   SNode *head = set->arr[bucket];

   while(head != NULL){
      if(strcmp(head->key, key) == 0){
         return head;
      }
      head = head->next;
   }

   return NULL;
}

void set_add(SSet *set, const char *key){
   int bucket;
   SNode *node = node_search_set(set, key, &bucket);

   // Node was not found
   if(node == NULL){
      node = malloc(sizeof(VNode));
      node->key = strdup(key);
      node->next = NULL;
   
      if(set->arr[bucket] == NULL){
         set->arr[bucket] = node;
      }else{
         node->next = set->arr[bucket];
         set->arr[bucket] = node;
      }
   }
}

void set_remove(SSet *set, const char *key){
   int bucket = hash_function(key);

   SNode *prev = NULL;
   SNode *curr = set->arr[bucket];

   while(curr != NULL){
      if(strcmp(key, curr->key) == 0){
         if(curr == set->arr[bucket]){
            set->arr[bucket] = curr->next;
         }else{
            prev->next = curr->next;
         }

         free(curr->key);
         free(curr);
         break;
      }

      prev = curr;
      curr = curr->next;
   }
}

int  set_contains(SSet *set, const char *key);


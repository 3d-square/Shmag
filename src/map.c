#include <stdlib.h>
#include <shmag.h>
#include <string.h>
#include <stdio.h>

VNode *node_search_vmap(VMap *map, const char *key, int *hash){
   int bucket = hash_function(key);
   *hash = bucket;

   VNode *head = map->arr[bucket];

   while(head != NULL){
      if(strcmp(head->key, key) == 0){
         return head;
      }
      head = head->next;
   }

   return NULL;
}

void *search_vmap(VMap *map, const char *key){
   int hash;
   VNode *node = node_search_vmap(map, key, &hash);

   if(node){
      return &node->data;
   }

   return NULL;
}

void insert_vmap(VMap *map, const char *key, void *value){
   int bucket;
   VNode *node = node_search_vmap(map, key, &bucket);

   // Node was found
   if(node){
      node->data = value;
   }else{
      node = malloc(sizeof(VNode));
      node->key = strdup(key);
      node->data = value;
      node->next = NULL;
   
      if(map->arr[bucket] == NULL){
         map->arr[bucket] = node;
      }else{
         node->next = map->arr[bucket];
         map->arr[bucket] = node;
      }
   }
}

void delete_vmap(VMap *map, const char *key){
   int bucket = hash_function(key);

   VNode *prev = NULL;
   VNode *curr = map->arr[bucket];

   while(curr != NULL){
      if(strcmp(key, curr->key) == 0){
         if(curr == map->arr[bucket]){
            map->arr[bucket] = curr->next;
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

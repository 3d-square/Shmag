#include <stdlib.h>
#include <shmag.h>
#include <string.h>

int hash_function(char *key){
   int sum    = 0,
       factor = 31;

   for(int i = 0; i < (int)strlen(key); ++i){
      sum = ((sum % MAX_BUCKETS) + (((int)key[i]) * factor) % MAX_BUCKETS) %MAX_BUCKETS;

      factor = ((factor % __INT16_MAX__) * (31 % __INT16_MAX__)) % __INT16_MAX__;
   }

   return sum;
}

RNode *node_search_rmap(RMap *map, char *key, int *hash){
   int bucket = hash_function(key);
   *hash = bucket;

   RNode *head = map->arr[bucket];

   while(head != NULL){
      if(strcmp(head->key, key) == 0){
         return head;
      }
      head = head->next;
   }

   return NULL;
}

MultiVal *search_rmap(RMap *map, char *key){
   int hash;
   RNode *node = node_search_rmap(map, key, &hash);

   if(node){
      return &node->value;
   }

   return NULL;
}

void insert_rmap(RMap *map, char *key, MultiVal value){
   int bucket;
   RNode *node = node_search_rmap(map, key, &bucket);

   if(node){
      node->value = value;
   }else{
      node = malloc(sizeof(RNode));
      node->key = key;
      node->value = value;
      node->next = NULL;
   
      if(map->arr[bucket] == NULL){
         map->arr[bucket] = node;
      }else{
         node->next = map->arr[bucket];
         map->arr[bucket] = node;
      }
   }
}

void delete_rmap(RMap *map, char *key){
   int bucket = hash_function(key);

   RNode *prev = NULL;
   RNode *curr = NULL;

   while(curr != NULL){
      if(strcmp(key, curr->key) == 0){
         if(curr == map->arr[bucket]){
            map->arr[bucket] = curr->next;
         }else{
            prev->next = curr->next;
         }

         free(curr);
         break;
      }

      prev = curr;
      curr = curr->next;
   }
}

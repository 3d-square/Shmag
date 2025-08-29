#include <stdlib.h>
#include <shmag.h>
#include <string.h>
#include <stdio.h>

int hash_function(const char *key){
   int sum    = 0,
       factor = 31;

   for(int i = 0; i < (int)strlen(key); ++i){
      sum = ((sum % MAX_BUCKETS) + (((int)key[i]) * factor) % MAX_BUCKETS) %MAX_BUCKETS;

      factor = ((factor % __INT16_MAX__) * (31 % __INT16_MAX__)) % __INT16_MAX__;
   }

   return sum;
}


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

int map_has(VMap *map, const char *key){
   int hash;
   return node_search_vmap(map, key, &hash) != NULL;
}

void **map_get(VMap *map, const char *key){
   int hash;
   VNode *node = node_search_vmap(map, key, &hash);

   if(node){
      return &node->data;
   }

   return NULL;
}

void map_insert(VMap *map, const char *key, void *value){
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

void map_remove(VMap *map, const char *key, destroy_func del){
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

         if(del){
            del(curr->data);
         }
         free(curr->key);
         free(curr);
         break;
      }

      prev = curr;
      curr = curr->next;
   }
}

void map_delete(VMap *map, destroy_func del){
   for(int i = 0; i < MAX_BUCKETS; ++i){
      VNode *curr = map->arr[i];
      while(curr){
         VNode *next = curr->next;
         if(del){
            del(curr->data);
         }
         free(curr->key);
         free(curr);

         curr = next;
      }
   }
}

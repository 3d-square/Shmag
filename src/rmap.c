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

RNode *node_search_rmap(RMap *map, const char *key, int *hash){
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

ShmObj *search_rmap(RMap *map, const char *key){
   int hash;
   RNode *node = node_search_rmap(map, key, &hash);

   if(node){
      return &node->obj;
   }

   return NULL;
}

void insert_rmap(RMap *map, const char *key, ShmType type, MultiVal value){
   int bucket;
   RNode *node = node_search_rmap(map, key, &bucket);

   // Node was found
   if(node){
      if(type != node->obj.obj_type){
         fprintf(stderr, "Unable to assign type %s to type %s\n", shm_type_str(node->obj.obj_type), shm_type_str(type));
         exit(1);
      }
      
      node->obj.as = value;
   }else{
      node = malloc(sizeof(RNode));
      node->key = strdup(key);
      node->obj.obj_type = type;
      node->obj.as = value;
      node->next = NULL;
   
      if(map->arr[bucket] == NULL){
         map->arr[bucket] = node;
      }else{
         node->next = map->arr[bucket];
         map->arr[bucket] = node;
      }
   }
}

void delete_rmap(RMap *map, const char *key, destroy_func del){
   int bucket = hash_function(key);

   RNode *prev = NULL;
   RNode *curr = map->arr[bucket];

   while(curr != NULL){
      if(strcmp(key, curr->key) == 0){
         if(curr == map->arr[bucket]){
            map->arr[bucket] = curr->next;
         }else{
            prev->next = curr->next;
         }

         free(curr->key);
         if(del){
            del(&curr->obj);
         }
         free(curr);
         break;
      }

      prev = curr;
      curr = curr->next;
   }
}

void destroy_rmap(RMap *map, destroy_func del){
   for(int i = 0; i < MAX_BUCKETS; ++i){
      if(map->arr[i]){
         RNode *curr = map->arr[i];
         while(curr){
            RNode *next = curr->next;
            del(&curr->obj);
            free(curr->key);
            free(curr);
            curr = next;
         }
      }
   }
}

const char *shm_type_str(ShmType type){
   switch(type){
      case SHM_INT:
         return "Integer";
      case SHM_DBL:
         return "Float";
      case SHM_STR:
         return "String";
      case SHM_FUNC:
         return "Function";
      case SHM_NONE:
         return "None";
      case SHM_NULL:
         return "Unimplemented";
   }

   return NULL;
}

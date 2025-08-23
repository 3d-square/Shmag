#include <shmag.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static FILE *_log;
static const char *_file = "";

#define LOG_PAD "20"

void log_start(){
   char file_name[1000];
   // time_t t = time(NULL);

   // struct tm time_struct = *localtime(&t);
   // sprintf(file_name, "LOG_%d-%02d-%02d_%02d%02d%02d", time_struct.tm_year + 1900, time_struct.tm_mon + 1, time_struct.tm_mday, time_struct.tm_hour, time_struct.tm_min, time_struct.tm_sec);
   sprintf(file_name, "LOG");
   _log = fopen(file_name, "w");

   if(_log == NULL){
      _log = stderr;
   }
}

void log_stop(){
   if(_log != stderr){
      fclose(_log);
   }
}

void log_set_file(const char *file){
   _file = file;
}

void log_msg(const char *msg){
   if(msg[0] == '\0'){
      fprintf(_log, "\n");
   }else{
      fprintf(_log, "[%s] %s\n", _file, msg);
   }
   fflush(_log);
}

void log_int(const char *msg, long val){
   fprintf(_log, "[%s] %"LOG_PAD"s - %ld\n", _file, msg, val);
   fflush(_log);
}

void log_str(const char *msg, const char *val){
   fprintf(_log, "[%s] %"LOG_PAD"s - %s\n", _file, msg, val);
   fflush(_log);
}

void log_float(const char *msg, double val){
   fprintf(_log, "[%s] %"LOG_PAD"s - %f\n", _file, msg, val);
   fflush(_log);
}
void log_rtoken(const char *msg, const RToken *tkn){
   fprintf(_log, "[%s] %"LOG_PAD"s - %s\n", _file, msg, rtoken_str(tkn));
   fflush(_log);
}
void log_ptoken(const char *msg, const PToken *tkn){
   fprintf(_log, "[%s] %"LOG_PAD"s - %s\n", _file, msg, ptoken_str(tkn));
   fflush(_log);
}

void log_obj(const char *msg, const ShmObj *obj){
   log_msg(msg);
   switch(obj->obj_type){
      case SHM_DBL:
         log_float("VALUE", obj->as.number);
      break;
      case SHM_INT:
         log_int("VALUE", obj->as.decimal);
      break;
      default:
         log_str("SHMOBJ LOGGING IS NOT SUPPORTED FOR", shm_type_str(obj->obj_type));
      break;
   }

   fflush(_log);
}

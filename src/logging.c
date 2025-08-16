#include <shmag.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static FILE *_log;

void log_start(){
   char file_name[1000];
   time_t t = time(NULL);

   struct tm time_struct = *localtime(&t);
   sprintf(file_name, "LOG_%d-%02d-%02d_%02d%02d%02d", time_struct.tm_year + 1900, time_struct.tm_mon + 1, time_struct.tm_mday, time_struct.tm_hour, time_struct.tm_min, time_struct.tm_sec);
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

void log_msg(const char *msg){
   fprintf(_log, "%s\n", msg);
}

void log_int(const char *msg, long val){
   fprintf(_log, "%s - %ld\n", msg, val);
}

void log_str(const char *msg, const char *val){
   fprintf(_log, "%s - %s\n", msg, val);
}

void log_float(const char *msg, double val){
   fprintf(_log, "%s - %f\n", msg, val);
}
void log_rtoken(const char *msg, const RToken *tkn){
   fprintf(_log, "%s - %s\n", msg, rtoken_str(tkn));
}
void log_ptoken(const char *msg, const PToken *tkn){
   fprintf(_log, "%s - %s\n", msg, ptoken_str(tkn));
}

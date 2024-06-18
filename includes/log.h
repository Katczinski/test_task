#ifndef LOG_H
#define LOG_H

#include "return_codes.h"

void        log_add(char *format, ...);
ret_code    log_init_file(char *path);

#endif

#ifndef ERRORS_H
#define ERRORS_H

#include <errno.h>

void    set_errno(int val);
int     get_errno();
char*   get_errno_str();
#endif
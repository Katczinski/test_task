#ifndef ERRORS_H
#define ERRORS_H

#include <errno.h>

void    set_errno(int val);
void    reset_errno();
int     get_errno();
char*   get_errno_str();

#endif
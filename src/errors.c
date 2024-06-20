#include "errors.h"

#include <string.h>

void set_errno(int val)
{
	errno = val;
}

int get_errno()
{
    return errno;
}

void reset_errno()
{
    set_errno(0);
}

char* get_errno_str()
{
    return strerror(errno);
}
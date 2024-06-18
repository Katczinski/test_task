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

char* get_errno_str()
{
    return strerror(errno);
}
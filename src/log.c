#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>

#include "log.h"

ret_code log_init_file(char *path)
{
	int fd = open(path, O_RDWR | O_APPEND | O_CREAT);

	if (fd != -1) {
		if (dup2(fd, STDOUT_FILENO) != -1)
			return RET_OK;
	}

	return RET_ERROR;
}

// TODO: add component 
void log_add(char *format, ...)
{
	va_list arg_list;

	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
	char time_string[10];
	size_t ret = strftime(time_string, sizeof(time_string), "%T", &tm);
	
	if (format != NULL)
		va_start(arg_list, format);

	if (ret) {
		printf("%s: ", time_string);
		vprintf(format, arg_list);
		printf("\n");
	}

	if (format != NULL)
		va_end(arg_list);
}

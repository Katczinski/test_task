#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>

#include "log.h"

static int fd;
static int saved_stdout;

void log_close_file(int file_fd)
{
	if (file_fd) {
		close(file_fd);
		file_fd = -1;
	}
}

ret_code log_init_file(char *path)
{
	fd = open(path, O_RDWR | O_APPEND | O_CREAT);
	saved_stdout = dup(1);

	if (fd != -1) {
#ifdef DEBUG
		log_close_file(fd);
		return RET_OK;
#endif
		if (dup2(fd, STDOUT_FILENO) != -1)
			return RET_OK;
		log_close_file(fd);
		fd = -1;
	}

	return RET_ERROR;
}

void log_add(char *format, ...)
{
	va_list arg_list;

	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
	char time_string[20];
	size_t ret = strftime(time_string, sizeof(time_string), "%d.%m.%Y %T", &tm);
	
	if (format != NULL)
		va_start(arg_list, format);

	if (ret) {
		printf("%s: ", time_string);
		vprintf(format, arg_list);
		printf("\n");
	} else {
		printf("strftime returned %lu\n", ret);
	}

	if (format != NULL)
		va_end(arg_list);
}

void log_file_shutdown()
{
	fflush(stdout);
	dup2(saved_stdout, 1);
	close(saved_stdout);
	log_close_file(fd);
}
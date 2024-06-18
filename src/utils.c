#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "utils.h"
#include "errors.h"

bool is_valid_ip(char *str)
{
	if (str == NULL) {
		set_errno(EINVAL);
		return false;
	}

	char *temp_str = calloc(strlen(str) + 1, sizeof(char));
	strcpy(temp_str, str);

	char *ip_str = strtok(temp_str, ":");
	char *port_str = strtok(NULL, ":");

	if (ip_str == NULL || port_str == NULL || strtok(NULL, ":") != NULL)
	{
		free(temp_str);
		set_errno(EFAULT);
		return false;
	}

	struct sockaddr_in sa;
	if (inet_pton(AF_INET, ip_str, &(sa.sin_addr)) != 1)
	{
		free(temp_str);
		set_errno(EFAULT);
		return false;
	}

	char *endptr;
	long port = strtol(port_str, &endptr, 10);
	if (endptr == port_str || *endptr != '\0' || port <= 0 || port > 65535)
	{
		free(temp_str);
		set_errno(EFAULT);
		return false;
	}

	free(temp_str);
	return true;
}

bool is_valid_file_path(char *str)
{
	if (str == NULL)
	{
		set_errno(EINVAL);
		return false;
	}

	FILE *fd = fopen(str, "a+");
	if (fd)
	{
		fclose(fd);
		return true;
	}

	return false;
}

bool is_valid_prefix(char *str)
{
	if (str == NULL)
	{
		set_errno(EINVAL);
		return false;
	}

	return strlen(str) == 4;
}
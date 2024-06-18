#include <stdio.h>
#include <string.h>

#include "log.h"
#include "utils.h"
#include "errors.h"
#include "return_codes.h"

#define ARGS_UDP_IP 		1
#define ARGS_TCP_IP 		2
#define ARGS_LOG_PATH 		3
#define ARGS_PREFIX 		4
#define ARGS_EXPECTED_ARGC	5

int validate_argv(int argc, char *argv[])
{
	if (argc != ARGS_EXPECTED_ARGC) {
		log_add("Invalid number of arguments: %d. Expected: %d", argc - 1, ARGS_EXPECTED_ARGC - 1);
		return RET_ERROR;
	}

	if (!is_valid_ip(argv[ARGS_UDP_IP])) {
		log_add("Invalid UDP IP and/or port: '%s': %s", argv[ARGS_UDP_IP], get_errno_str());
		return RET_ERROR;
	}

	if (!is_valid_ip(argv[ARGS_TCP_IP])) {
		log_add("Invalid TCP IP and/or port: '%s': %s", argv[ARGS_TCP_IP], get_errno_str());
		return RET_ERROR;
	}

	if (!is_valid_file_path(argv[ARGS_LOG_PATH])) {
		log_add("Invalid log file path: '%s': %s", argv[ARGS_LOG_PATH], get_errno_str());
		return RET_ERROR;
	}

	if (!is_valid_prefix(argv[ARGS_PREFIX])) {
		log_add("Invalid prefix length (%d): '%s'", strlen(argv[ARGS_PREFIX]), argv[ARGS_PREFIX]);
		return RET_ERROR;
	}

	return RET_OK;
}

int main(int argc, char *argv[])
{
	int ret = validate_argv(argc, argv); 

	if (ret == RET_OK) {

	}

	return ret;
}

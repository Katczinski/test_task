#include <stdio.h>

#include "log.h"
#include "utils.h"

enum {
    RET_OK,
    RET_ERROR,
};

int validate_argv(int argc, char *argv[])
{
    (void)argv;
    if (argc != 5) {
        log_add("Incorrect number of arguments");
		return RET_ERROR;
    }
	
	if (!is_valid_ip(argv[1])) {
		log_add("Invalid UDP IP and/or port");
		return RET_ERROR;
	}
	
	return RET_OK;
}

int main(int argc, char *argv[])
{
    int ret = validate_argv(argc, argv); 

    if (ret == RET_OK)
    {

    }

    return ret;
}

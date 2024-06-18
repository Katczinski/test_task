#include "loop.h"
#include "log.h"
#include "defines.h"
#include "errors.h"

ret_code loop_init(char *argv[])
{
    if (log_init_file(argv[ARGS_LOG_PATH]) != RET_OK) {
        log_add("Failed to open log file '%s': %s", argv[ARGS_LOG_PATH], get_errno_str());
        return RET_ERROR;
    }

    return RET_OK;
}

#include "loop.h"
#include "log.h"
#include "utils.h"
#include "defines.h"
#include "errors.h"
#include "udp_server.h"

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <ctype.h>

static uint8_t  comm_buff[TX_BUFF_SIZE] = { 0 };
static bool     loop_keep_running = true;   

ret_code loop_udp_server_init(char *ip_str);
ret_code loop_init_buffer(char *prefix);

ret_code loop_init(char *argv[])
{
    if (log_init_file(argv[ARGS_LOG_PATH]) != RET_OK) {
        log_add("Failed to open log file '%s': %s", argv[ARGS_LOG_PATH], get_errno_str());
        return RET_ERROR;
    }

    if (loop_init_buffer(argv[ARGS_PREFIX]) != RET_OK) {
        log_add("Failed to prepare communication buffer");
        return RET_ERROR;
    }

    if (loop_udp_server_init(argv[ARGS_UDP_IP]) != RET_OK) {
        log_add("Failed to create UDP socket '%s'", argv[ARGS_UDP_IP]);
        return RET_ERROR;
    }

    log_add("DEBUG: loop init success"); // TODO: change
    return RET_OK;
}

ret_code loop_init_buffer(char *prefix)
{
    if (prefix == NULL || strlen(prefix) != PREFIX_SIZE)
        return RET_ERROR;

    memcpy(comm_buff, prefix, PREFIX_SIZE);

    return RET_OK;
}

ret_code loop_udp_server_init(char *ip_str)
{
    return udp_server_init(ip_str, comm_buff + PREFIX_SIZE, RX_BUFF_SIZE);
}

void loop_sigint_handler(int sig)
{
    (void)sig;

    loop_keep_running = false;
}

ret_code loop_run()
{
    signal(SIGINT, loop_sigint_handler);

    while (loop_keep_running) {
        udp_server_iterate(0);
    }

    udp_server_shutdown();
    return RET_ERROR;
}
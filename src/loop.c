#include "loop.h"
#include "log.h"
#include "utils.h"
#include "defines.h"
#include "errors.h"
#include "udp_server.h"
#include "tcp_client.h"

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <ctype.h>
#include <arpa/inet.h>

static uint8_t  comm_buff[TX_BUFF_SIZE] = { 0 };
static bool     loop_keep_running = true;   

ret_code loop_tcp_client_init(char *ip_str);
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

    if (loop_tcp_client_init(argv[ARGS_TCP_IP]) != RET_OK) {
        log_add("Failed to create TCP socket '%s'", argv[ARGS_TCP_IP]);
        return RET_ERROR;
    }

    log_add("System init successfull");
    return RET_OK;
}

ret_code loop_init_buffer(char *prefix)
{
    if (prefix == NULL || strlen(prefix) != PREFIX_SIZE)
        return RET_ERROR;

    memcpy(comm_buff, prefix, PREFIX_SIZE);

    return RET_OK;
}

ret_code loop_tcp_client_init(char *ip_str)
{
    return tcp_client_init(ip_str);
}

ret_code loop_udp_server_init(char *ip_str)
{
    return udp_server_init(ip_str);
}

void loop_sigint_handler(int sig)
{
    (void)sig;
    printf("\nSIGINT caught\n");
    loop_keep_running = false;
}

ret_code loop_run()
{
    signal(SIGINT, loop_sigint_handler);

    int received_bytes = 0;
    int sent_bytes = 0;
    int bytes_to_send = 0;
    while (loop_keep_running) {
        if (bytes_to_send <= 0)
        {
            bytes_to_send = sent_bytes = 0;
            received_bytes = udp_server_recv(comm_buff + PREFIX_SIZE, RX_BUFF_SIZE, 5000);
            if (received_bytes > 0)
                bytes_to_send = received_bytes + PREFIX_SIZE;
            else {
                received_bytes = 0;
            }
        }
        if (tcp_client_check_connection() != RET_OK)
        {
            if (tcp_client_reconnect() != RET_OK || sent_bytes > 0) // if something was sent before the loss of connection -> discard this message even if successfully reconnected
                bytes_to_send = sent_bytes = received_bytes = 0;
        }
        if (bytes_to_send > 0)
        { 
            int ret = tcp_client_send(comm_buff + sent_bytes, bytes_to_send);
            if (ret > 0) {
                bytes_to_send -= ret;
                sent_bytes += ret;
            } else
                bytes_to_send = sent_bytes = received_bytes = 0;
        }
        tcp_client_flush_recv();
    }

    udp_server_shutdown();
    tcp_client_shutdown();
    log_file_shutdown();
    printf("\nForced exit\n");
    return RET_ERROR;
}
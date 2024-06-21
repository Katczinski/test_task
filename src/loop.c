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

// ret_code loop_udp_message_handler(int sock, struct sockaddr_in* from, uint8_t *buff, size_t buff_len)
// {
//     (void)sock;
//     (void) buff_len;

//     char ip[INET_ADDRSTRLEN];
//     uint16_t port;

//     inet_ntop(AF_INET, &from->sin_addr, ip, sizeof(ip));
//     port = htons(from->sin_port);

    
//     log_add("Got message from '%s:%d': %s", ip, port, buff);
//     if (tcp_client_send(comm_buff, TX_BUFF_SIZE) != RET_OK)
//     {
//         log_add("TCP send returned error: %s", get_errno_str());
//         return RET_ERROR;
//     }
//     return RET_OK;
// }

ret_code loop_tcp_client_init(char *ip_str)
{
    return tcp_client_init(ip_str);
}

ret_code loop_udp_server_init(char *ip_str)
{
    if (udp_server_init(ip_str) != RET_OK)
        return RET_ERROR;

    // if (udp_server_install_handler(&loop_udp_message_handler) != RET_OK)
    //     return RET_ERROR;

    return RET_OK;
}

void loop_sigint_handler(int sig)
{
    (void)sig;

    loop_keep_running = false;
}

ret_code loop_run()
{
    signal(SIGINT, loop_sigint_handler);

    int received_bytes = 0;
    int sent_bytes = 0;
    while (loop_keep_running) {
        if (tcp_client_check_connection() != RET_OK) {
            sent_bytes = received_bytes = 0;
            tcp_client_reconnect();
        } else if (sent_bytes < received_bytes) {
            sent_bytes += tcp_client_send(comm_buff + sent_bytes, received_bytes);
            if (sent_bytes > 0)
                received_bytes -= sent_bytes;
        } else {
            sent_bytes = 0;
            received_bytes = udp_server_recv(comm_buff + PREFIX_SIZE, RX_BUFF_SIZE, 0);
            if (received_bytes > 0)
                received_bytes += PREFIX_SIZE;
        }
    }

    udp_server_shutdown();
    tcp_client_shutdown();
    log_file_shutdown();
    log_add("Forced exit");
    return RET_ERROR;
}
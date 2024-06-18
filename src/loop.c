#include "loop.h"
#include "log.h"
#include "utils.h"
#include "defines.h"
#include "errors.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

static int                  epfd = 0;
static struct epoll_event   events[MAX_EVENT_NUM] = { 0 };
static char                 comm_buff[TX_BUFF_SIZE] = { 0 };

ret_code loop_init_udp_socket(char *str);
ret_code loop_init_buffer(char *str);

ret_code loop_init(char *argv[])
{
    if (log_init_file(argv[ARGS_LOG_PATH]) != RET_OK) {
        log_add("Failed to open log file '%s': %s", argv[ARGS_LOG_PATH], get_errno_str());
        return RET_ERROR;
    }

    if (loop_init_udp_socket(argv[ARGS_UDP_IP]) != RET_OK) {
        log_add("Failed to create UDP socket '%s'", argv[ARGS_UDP_IP]);
        return RET_ERROR;
    }

    if (loop_init_buffer(argv[ARGS_PREFIX]) != RET_OK) {
        log_add("Failed to prepare communication buffer");
        return RET_ERROR;
    }

    log_add("DEBUG: loop init success"); // TODO: change
    return RET_OK;
}

ret_code loop_init_buffer(char *str)
{
    if (str == NULL || strlen(str) != PREFIX_SIZE)
        return RET_ERROR;

    memcpy(comm_buff, str, PREFIX_SIZE);

    return RET_OK;
}

ret_code loop_init_udp_socket(char *str)
{
    int listen_sock = 0;
    struct sockaddr_in server;

    if ((listen_sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        log_add("Failed to create socket: %s", get_errno_str());
		return RET_ERROR;
	}

	server.sin_family = AF_INET;
    extract_ip_port(str, &server.sin_addr.s_addr, &server.sin_port);

    if (bind(listen_sock, (struct sockaddr *) &server, sizeof(server)) < 0) {
        log_add("Failed to bind address to socket: %s", get_errno_str());
        return RET_ERROR;
    }

    epfd = epoll_create(MAX_EVENT_NUM);
    if (epfd < 0) {
        log_add("Failed to open an epoll descriptor: %s", get_errno_str());
        return RET_ERROR;
    }

    struct epoll_event ep;
    memset(&ep, 0, sizeof(ep));
    ep.events = EPOLLIN;            // TODO: explore all the events and may be add some
    ep.data.fd = listen_sock;

    if (epoll_ctl(epfd, EPOLL_CTL_ADD, ep.data.fd, &ep) < 0)
    {
        log_add("Failed to add target descriptor to the epoll descriptor: %s", get_errno_str());
        close(epfd);
        return RET_ERROR;
    }

    return RET_OK;
}

ret_code loop_run()
{
    int event_count = 0;
    int client_fd = 0;
    while (1) {
        event_count = epoll_wait(epfd, events, MAX_EVENT_NUM, 0);
        for (int i = 0; i < event_count; ++i) {
            client_fd = events[i].data.fd;
           if (client_fd < 0)
                continue;
            // TODO: events[i].events is probably a bitmask, so there may be several events at once. Handle this
            struct sockaddr_in clientaddr;
            socklen_t clilen = sizeof(struct sockaddr);

            int len = recvfrom(client_fd, comm_buff + PREFIX_SIZE, RX_BUFF_SIZE, 0, (struct sockaddr*)&clientaddr, &clilen); // MSG_DONTWAIT  ??
            if (len > 0) {
                comm_buff[PREFIX_SIZE + len] = '\0';
                log_add("event: %d: %s", events[i].events, comm_buff);
            } else if (len < 0) {
                log_add("Erorr occured during reading from udp client: %s", get_errno_str());
            }
        }
        // TODO: Install sigint handler
        // log_add("Ain't no waiting for shit, log goes brrrr");
    }
    close(epfd);
    return RET_ERROR;
}
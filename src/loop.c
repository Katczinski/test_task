#include "loop.h"
#include "log.h"
#include "defines.h"
#include "errors.h"

#include<sys/socket.h>
#include<arpa/inet.h>

ret_code loop_init_udp_socket(char *str);

ret_code loop_init(char *argv[])
{
    if (log_init_file(argv[ARGS_LOG_PATH]) != RET_OK) {
        log_add("Failed to open log file '%s': %s", argv[ARGS_LOG_PATH], get_errno_str());
        return RET_ERROR;
    }

    if (loop_init_udp_socket(argv[ARGS_UDP_IP]) != RET_OK) {
        log_add("Failed to create UDP socket '%s': %s", argv[ARGS_UDP_IP], get_errno_str());
        return RET_ERROR;
    }
    log_add("loop init success");
    return RET_OK;
}

#include <sys/epoll.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>

static int epfd;
struct epoll_event *events;
struct epoll_event ep;
#define MAX_EVENT_NUM 10

ret_code loop_init_udp_socket(char *str)
{
    int listen_sock = 0;
    struct sockaddr_in server;

    (void)str;

    if ((listen_sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        log_add("socket failed");
		return RET_ERROR;
	}

    int flags;
    if((flags = fcntl(listen_sock, F_GETFD, 0)) < 0)
    {
        log_add("get falg error");
        return RET_ERROR;
    }

    flags |= O_NONBLOCK;
    if (fcntl(listen_sock, F_SETFL, flags) < 0) {
        log_add("set nonblock fail");
        return -1;
    }

	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(6000);

    if (bind(listen_sock, (struct sockaddr *) &server, sizeof(server)) < 0) {
        log_add("bind failed");
        return RET_ERROR;
    }

    
    events = malloc(sizeof(*events) * MAX_EVENT_NUM);
    epfd = epoll_create(MAX_EVENT_NUM);
    if (epfd < 0) {
        log_add("epoll_create failed");
        return RET_ERROR;
    }
    memset(&ep, 0, sizeof(ep));

    ep.events = EPOLLIN;
    ep.data.fd = listen_sock;

    if (epoll_ctl(epfd, EPOLL_CTL_ADD, ep.data.fd, &ep) < 0)
    {
        log_add("epoll_ctl failed");
        return RET_ERROR;
    }

    return RET_OK;
}

ret_code loop_run()
{
    // struct epoll_event ep;
    int nfds;
    // char buff[1024];
    while (1) {
        // log_add("polling...");
        nfds = epoll_wait(epfd, events, MAX_EVENT_NUM, 500);
        log_add("nfds: %d", nfds);
        for (int i = 0; i < nfds; ++i) {
           if (events[i].data.fd < 0)
                continue;
            // int sock = events[i].data.fd;
            // read(sock, buff, 1023);
            log_add("event: %d", events[i].events);
        }
                
    }

    return RET_ERROR;
}
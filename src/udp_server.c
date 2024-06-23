#include "udp_server.h"
#include "errors.h"
#include "defines.h"
#include "utils.h"
#include "log.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

#include <string.h>
#include <unistd.h>
#include <fcntl.h>

static struct udp_server_s {
    struct epoll_event  events[MAX_EVENT_NUM];
    int                 epfd;
} udp_server;

int udp_server_get_default_socket(char *ip_str, int *listen_sock)
{
    struct sockaddr_in server;
    int flags = 0;

    if ((*listen_sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        log_add("Failed to create socket: %s", get_errno_str());
		return RET_ERROR;
	}

    memset(&server, 0, sizeof(server));
	server.sin_family = AF_INET;
    if (!extract_ip_port(ip_str, &server.sin_addr.s_addr, &server.sin_port))
    {
        log_add("Failed to parse ip:port '%s': %s", ip_str, get_errno_str());
        return RET_ERROR;
    }

    if (bind(*listen_sock, (struct sockaddr *) &server, sizeof(server)) < 0)
    {
        log_add("Failed to bind address to socket: %s", get_errno_str());
        return RET_ERROR;
    }

    flags = fcntl(*listen_sock, F_GETFL, 0);
    if (flags == -1)
    {
        log_add("Failed to read socket flags: %s", get_errno_str());
        return RET_ERROR;
    }

    flags |= O_NONBLOCK;
    if (fcntl(*listen_sock, F_SETFL, flags) != 0)
    {
        log_add("Failed to set socket flags: %s", get_errno_str());
        return RET_ERROR;
    }

    return RET_OK;
}

ret_code udp_server_init(char *ip_str)
{
    int listen_sock = 0;
    int epfd = 0;    
    struct epoll_event ep;

    if (udp_server_get_default_socket(ip_str, &listen_sock) != RET_OK)
        return RET_ERROR;

    epfd = epoll_create(MAX_EVENT_NUM);
    if (epfd < 0)
    {
        log_add("Failed to open an epoll descriptor: %s", get_errno_str());
        return RET_ERROR;
    }

    memset(&ep, 0, sizeof(ep));
    ep.events = EPOLLIN;
    ep.data.fd = listen_sock;

    if (epoll_ctl(epfd, EPOLL_CTL_ADD, ep.data.fd, &ep) < 0)
    {
        log_add("Failed to add target descriptor to the epoll descriptor: %s", get_errno_str());
        close(epfd);
        return RET_ERROR;
    }

    udp_server.epfd = epfd;

    log_add("UDP server started on %s", ip_str);
    return RET_OK;
}
                
ret_code udp_server_shutdown()
{
    close(udp_server.epfd);
    log_add("UDP server shutdown");
    return RET_OK;
}

int udp_server_recv(uint8_t *buff, size_t buff_size, int timeout_ms)
{
    int len = 0;
    if (epoll_wait(udp_server.epfd, udp_server.events, 1, timeout_ms) > 0) {
        int client_fd = udp_server.events[0].data.fd;
        if (client_fd < 0)
            return len;
        
        struct sockaddr_in clientaddr;
        socklen_t client_len = sizeof(struct sockaddr);

        len = recvfrom(client_fd, buff, buff_size, MSG_DONTWAIT, (struct sockaddr *)&clientaddr, &client_len);
        // TODO: EAGAIN??
        if (len > 0)
        {
            buff[len] = '\0';
#ifndef SILENT
            char ip[INET_ADDRSTRLEN];
            uint16_t port;

            inet_ntop(AF_INET, &clientaddr.sin_addr, ip, sizeof(ip));
            port = htons(clientaddr.sin_port);  

            log_add("UDP server: got %d bytes from '%s:%d': %s", len, ip, port, buff);
#endif
        }
        else if (len < 0)
        {
            log_add("Erorr occured during reading from udp client: %s", get_errno_str());
        }
    }
    return len;
}
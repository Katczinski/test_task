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
    callback_handle_t   handler;
    struct epoll_event  events[MAX_EVENT_NUM];
    size_t              buff_size;
    uint8_t*            buff;
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
    ep.events = EPOLLIN;            // TODO: explore all the events and may be add some
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

ret_code udp_server_install_handler(callback_handle_t handler)
{
    udp_server.handler = handler;

    return RET_OK;
}

ret_code udp_server_default_handler(int sock, struct sockaddr_in* from, uint8_t *buff, size_t buff_len)
{
    (void)sock;
    (void)buff_len;

    char ip[INET_ADDRSTRLEN];
    uint16_t port;

    inet_ntop(AF_INET, &from->sin_addr, ip, sizeof(ip));
    port = htons(from->sin_port);

    log_add("Unhandled message from '%s:%d': %s", ip, port, buff);

    return RET_OK;
}
                
ret_code udp_server_shutdown()
{
    close(udp_server.epfd);
    log_add("UDP server shutdown");
    return RET_OK;
}

ret_code udp_server_iterate(uint8_t *buff, size_t buff_size, int timeout)
{
    int event_count = epoll_wait(udp_server.epfd, udp_server.events, MAX_EVENT_NUM, timeout);
    ret_code ret = RET_OK;
    for (int i = 0; i < event_count; ++i)
    {
        int client_fd = udp_server.events[i].data.fd;
        if (client_fd < 0)
            continue;
        
        struct sockaddr_in clientaddr;
        socklen_t client_len = sizeof(struct sockaddr);

        int len = recvfrom(client_fd, buff, buff_size, MSG_DONTWAIT, (struct sockaddr *)&clientaddr, &client_len);
        if (len > 0)
        {
            buff[len] = '\0';
            // if (udp_server.handler)
            //     udp_server.handler(client_fd, &clientaddr, buff, len);
            // else
            //     udp_server_default_handler(client_fd, &clientaddr, buff, len);
        }
        else if (len < 0)
        {
            log_add("Erorr occured during reading from udp client: %s", get_errno_str());
            ret = RET_ERROR;
        }
    }
    return ret;
}
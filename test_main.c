#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <pthread.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "errors.h"
#include "utils.h"
#include "return_codes.h"

#if defined(ARGS_UDP_IP) || defined(ARGS_TCP_IP) || defined(ARGS_UDP_IP)
    #undef ARGS_UDP_IP
    #undef ARGS_TCP_IP
    #undef ARGS_EXPECTED_ARGC
#endif

#if defined(UDP) && defined(TCP)
    #define ARGS_UDP_IP 1
    #define ARGS_TCP_IP 2
    #define ARGS_EXPECTED_ARGC 3
#else
    #if defined(UDP)
        #define ARGS_UDP_IP 1
        #define ARGS_EXPECTED_ARGC 2
    #endif
    #if defined(TCP)
        #define ARGS_TCP_IP 1
        #define ARGS_EXPECTED_ARGC 2
    #endif
#endif

#ifndef MAX_EVENT_NUM
#define MAX_EVENT_NUM 20
#endif

#define array_size(n) (sizeof(n) / sizeof(n[0]))
#define BUFF_SIZE 2048

struct udp_client_s
{
    int sock;
    struct sockaddr_in server;
    int slen;
} udp_client;

struct tcp_server_s
{
    struct epoll_event events[MAX_EVENT_NUM];
    struct sockaddr_in server;
    size_t buff_size;
    uint8_t buff[BUFF_SIZE];
    int epfd;
    int sock;
} tcp_server;

ret_code init_tcp_server(char *ip_str)
{
    struct epoll_event ep;
    struct sockaddr_in server;
    int epfd = 0;
    int listen_sock = 0;

    if ((listen_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        printf("TCP server: failed to create socket: %s\n", get_errno_str());
        return RET_ERROR;
    }

    const int enable = 1;
    if (setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        printf("setsockopt(SO_REUSEADDR) failed: %s\n", get_errno_str());
        return RET_ERROR;
    }

    server.sin_family = AF_INET;
    if (!extract_ip_port(ip_str, &server.sin_addr.s_addr, &server.sin_port))
    {
        printf("TCP server: failed to parse ip:port '%s': %s\n", ip_str, get_errno_str());
        return RET_ERROR;
    }

    if (bind(listen_sock, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        printf("TCP server: failed to bind address to socket: %s\n", get_errno_str());
        return RET_ERROR;
    }

    if (listen(listen_sock, 1000) < 0)
    {
        printf("TCP server: failed to start listening to socket: %s\n", get_errno_str());
        return RET_ERROR;
    }

    epfd = epoll_create(MAX_EVENT_NUM);
    if (epfd < 0)
    {
        printf("TCP server: failed to open an epoll descriptor: %s\n", get_errno_str());
        return RET_ERROR;
    }

    memset(&ep, 0, sizeof(ep));
    ep.events = EPOLLIN;
    ep.data.fd = listen_sock;

    if (epoll_ctl(epfd, EPOLL_CTL_ADD, ep.data.fd, &ep) < 0)
    {
        printf("TCP server: failed to add target descriptor to the epoll descriptor: %s\n", get_errno_str());
        close(epfd);
        return RET_ERROR;
    }

    tcp_server.server = server;
    tcp_server.epfd = epfd;
    tcp_server.buff_size = BUFF_SIZE;
    tcp_server.sock = listen_sock;

    printf("TCP server sucessfully inited on %s\n", ip_str);
    return RET_OK;
}

ret_code init_udp_client(char *ip_str)
{
    udp_client.sock = 0;
    udp_client.slen = sizeof(udp_client.server);

    if ((udp_client.sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
    {
        printf("UDP client: failed to create a socket: %s\n", get_errno_str());
        return RET_ERROR;
    }

    udp_client.server.sin_family = AF_INET;
    if (!extract_ip_port(ip_str, &udp_client.server.sin_addr.s_addr, &udp_client.server.sin_port))
    {
        printf("UDP client: failed to parse ip:port '%s': %s\n", ip_str, get_errno_str());
        return RET_ERROR;
    }
    printf("UDP client sucessfully inited on %s\n", ip_str);
    return RET_OK;
}

pthread_t thread_create(void *(*start_routine)(void *), void *arg)
{
    pthread_t th;
    pthread_create(&th, NULL, start_routine, arg);
    return th;
}

void tcp_server_default_handler(int sock, struct sockaddr_in *from, uint8_t *buff, size_t buff_len)
{
    (void)sock;
    (void)buff_len;

    char ip[INET_ADDRSTRLEN];
    uint16_t port;

    inet_ntop(AF_INET, &from->sin_addr, ip, sizeof(ip));
    port = htons(from->sin_port);

    printf("TCP server: got %lu bytes from '%s:%d': %s\n", buff_len, ip, port, buff);
}

ret_code server_tcp_do_accept(int listenFd, int epfd)
{
    int cliFd;
    struct sockaddr_in cliaddr;
    socklen_t socklen = sizeof(struct sockaddr_in);
    struct epoll_event ev;

    cliFd = accept(listenFd, (struct sockaddr *)&cliaddr, &socklen);
    if (cliFd < 0)
    {
        printf("TCP server: accept failed: %s\n", get_errno_str());
        return RET_ERROR;
    }

    char ip[INET_ADDRSTRLEN];
    uint16_t port;

    inet_ntop(AF_INET, &cliaddr.sin_addr, ip, sizeof(ip));
    port = htons(cliaddr.sin_port);

    printf("accepted new client: %s:%d\n", ip, port);

    ev.events = EPOLLIN | EPOLLRDHUP;
    ev.data.fd = cliFd;

    if (epoll_ctl(epfd, EPOLL_CTL_ADD, cliFd, &ev) < 0)
    {
        printf("Failed to add target descriptor to the epoll descriptor: %s", get_errno_str());
        return RET_ERROR;
    }

    return RET_OK;
}

void *tcp_server_loop(void *arg)
{
    int timeout = *(int *)arg;
    printf("TCP server: epoll_wait timeout %d is chosen\n", timeout);
    while (1)
    {
        int event_count = epoll_wait(tcp_server.epfd, tcp_server.events, MAX_EVENT_NUM, timeout);
        for (int i = 0; i < event_count; ++i)
        {
            int events = tcp_server.events[i].events;
            if (tcp_server.events[i].data.fd == tcp_server.sock)
            {
                printf("TCP server: tcp_accept. event %d\n", events);
                server_tcp_do_accept(tcp_server.sock, tcp_server.epfd);
            }
            else
            {
                int client_fd = tcp_server.events[i].data.fd;
                if (client_fd < 0)
                    continue;

                if (events & EPOLLIN)
                {
                    struct sockaddr_in clientaddr = { 0 };
                    socklen_t client_len = sizeof(clientaddr);

                    int len = recvfrom(client_fd, tcp_server.buff, tcp_server.buff_size, MSG_DONTWAIT, NULL, NULL); // recvfrom ignores 'from' and 'fromlen' for connection-oriented sockets.
                    getpeername(client_fd, (struct sockaddr *)&clientaddr, &client_len);
                    if (len > 0)
                    {
                        tcp_server.buff[len] = '\0';
                        tcp_server_default_handler(client_fd, &clientaddr, tcp_server.buff, len);
                    }
                    else if (len < 0)
                    {
                        printf("TCP server: erorr occured during reading from client: %s\n", get_errno_str());
                    }
                    else
                    {
                        close(client_fd);
                        // client closed socket
                    }
                    events &= ~EPOLLIN;
                }
                if (events & EPOLLRDHUP)
                {
                    printf("TCP server: closing socket. event %d\n", events);
                    close(client_fd);
                    events &= ~EPOLLRDHUP;
                }
                if (events != 0) {
                    printf("TCP server: events left unhandled: %d\n", events);
                }
                // if ((rand()%10) == 0)
                // {
                //     printf("TCP server: ***random decided to close socket!***\n");
                //     close(client_fd);
                // }
            }
        }
    }
    return NULL;
}

int get_random_number(int min, int max)
{
    return rand() % (max + 1 - min) + min;
}

char get_random_char()
{
    return 'A' + rand() % 26;
}

void *udp_client_loop(void *arg)
{
    int delay_us = *(int *)arg;
    printf("UDP client: delay %d us is chosen\n", delay_us);
    char weird_msg[129] = { 0 };
    while (1)
    {
        int size = get_random_number(16, 128);
        memset(weird_msg, get_random_char(), size);
        weird_msg[size] = '\0';
        int bytes = sendto(udp_client.sock, weird_msg, size, 0, (struct sockaddr *)&udp_client.server, udp_client.slen);
        if (bytes > 0)
        {
            printf("UDP client: sent %d bytes: %s\n", bytes, weird_msg);
        }
        else
        {
            printf("UDP client: failed to send a message: %s\n", strerror(errno));
        }
        usleep(delay_us);
    }
    return NULL;
}

ret_code validate_argv(int argc, char *argv[])
{
#ifdef ARGS_EXPECTED_ARGC
    if (argc != ARGS_EXPECTED_ARGC)
    {
        printf("Invalid number of arguments: %d. Expected: %d\n", argc - 1, ARGS_EXPECTED_ARGC - 1);
        return RET_ERROR;
    }
#endif
#ifdef UDP
    if (!is_valid_ip(argv[ARGS_UDP_IP]))
    {
        printf("Invalid UDP IP and/or port: '%s': %s\n", argv[ARGS_UDP_IP], get_errno_str());
        return RET_ERROR;
    }
#endif
#ifdef TCP
    if (!is_valid_ip(argv[ARGS_TCP_IP]))
    {
        printf("Invalid TCP IP and/or port: '%s': %s\n", argv[ARGS_TCP_IP], get_errno_str());
        return RET_ERROR;
    }
#endif
    return RET_OK;
}

int main(int argc, char *argv[])
{
    if (validate_argv(argc, argv) != RET_OK)
    {
        printf("Shutting down\n");
        return 1;
    }

    pthread_t threads[2] = { 0 };
#ifdef UDP
    int delay_us = 100;
    init_udp_client(argv[ARGS_UDP_IP]);
    threads[0] = thread_create(udp_client_loop, &delay_us);
#endif
#ifdef TCP
    int timeout = 0;
    init_tcp_server(argv[ARGS_TCP_IP]);
    threads[1] = thread_create(tcp_server_loop, &timeout);
#endif
    for (int i = 0; i < 2; ++i) {
        if (threads[i] != 0)
            pthread_join(threads[i], NULL);
    }

    return 0;
}
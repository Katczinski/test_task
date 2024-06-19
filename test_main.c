#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "errors.h"
#include "utils.h"
#include "return_codes.h"


#if defined(ARGS_UDP_IP) || defined(ARGS_TCP_IP) || defined(ARGS_UDP_IP)
    #undef ARGS_UDP_IP
    #undef ARGS_TCP_IP
    #undef ARGS_EXPECTED_ARGC
#endif

#define ARGS_UDP_IP 		1
#define ARGS_TCP_IP 		2
#define ARGS_EXPECTED_ARGC	3

#ifndef MAX_EVENT_NUM
    #define MAX_EVENT_NUM   20
#endif

#define array_size(n) (sizeof(n) / sizeof(n[0]))
#define BUFF_SIZE 1024

struct udp_client_s {
    int                 sock;
    struct sockaddr_in  server;
    int                 slen;
} udp_client;

struct tcp_server_s {
    struct epoll_event  events[MAX_EVENT_NUM];
    struct sockaddr_in  server;
    size_t              buff_size;
    uint8_t             buff[BUFF_SIZE];
    int                 epfd;
} tcp_server;

ret_code init_tcp_server(char *ip_str)
{
    struct epoll_event ep;
    struct sockaddr_in server;
    int epfd = 0;
    int listen_sock = 0;

    if ((listen_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        printf("Failed to create socket: %s\n", get_errno_str());
		return RET_ERROR;
	}

    server.sin_family = AF_INET;
    if (!extract_ip_port(ip_str, &server.sin_addr.s_addr, &server.sin_port))
    {
        printf("Failed to parse ip:port '%s': %s\n", ip_str, get_errno_str());
        return RET_ERROR;
    }
    
    if (bind(listen_sock, (struct sockaddr *) &server, sizeof(server)) < 0) {
        log_add("Failed to bind address to socket: %s", get_errno_str());
        return RET_ERROR;
    }

    epfd = epoll_create(MAX_EVENT_NUM);
    if (epfd < 0) {
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

    tcp_server.epfd = epfd;
    tcp_server.buff_size = BUFF_SIZE;

    return RET_OK;
}

ret_code init_udp_client(char *ip_str)
{
    udp_client.sock = 0;
    udp_client.slen = sizeof(udp_client.server);

    if ((udp_client.sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
    {
        printf("Failed to create a socket: %s\n", get_errno_str());
        return RET_ERROR;
    }

    udp_client.server.sin_family = AF_INET;
    if (!extract_ip_port(ip_str, &udp_client.server.sin_addr.s_addr, &udp_client.server.sin_port))
    {
        printf("Failed to parse ip:port '%s': %s\n", ip_str, get_errno_str());
        return RET_ERROR;
    }
    printf("UDP client sucessfully inited\n");
    return RET_OK;
}

pthread_t thread_create(void *(*start_routine)(void *), void *arg)
{
    pthread_t th;
    pthread_create(&th, NULL, start_routine, arg);
    pthread_join(th, NULL);
    return th;
}

void *udp_client_loop(void *arg)
{
    int delay = *(int*)arg;
    printf("UDP Loop: delay %d is chosen\n", delay);
    char *msgs[] = { "Hello", "I", "am", "a", "test", "UDP", "Client" };
    size_t i = 0;
    while (1) {
        char *msg = msgs[i];
        int bytes = sendto(udp_client.sock, msg, strlen(msg) , 0 , (struct sockaddr *) &udp_client.server, udp_client.slen);
        if (bytes > 0) {
            printf("Sent %d bytes: %s\n", bytes, msg);
            if (++i >= array_size(msgs))
                i = 0;
        } else {
            printf("Failed to send a message: %s\n", strerror(errno));
        }
        sleep(delay);
    }
    return NULL;
}

ret_code validate_argv(int argc, char *argv[])
{
	if (argc != ARGS_EXPECTED_ARGC) {
		printf("Invalid number of arguments: %d. Expected: %d\n", argc - 1, ARGS_EXPECTED_ARGC - 1);
		return RET_ERROR;
	}

	if (!is_valid_ip(argv[ARGS_UDP_IP])) {
		printf("Invalid UDP IP and/or port: '%s': %s\n", argv[ARGS_UDP_IP], get_errno_str());
		return RET_ERROR;
	}

	if (!is_valid_ip(argv[ARGS_TCP_IP])) {
		printf("Invalid TCP IP and/or port: '%s': %s\n", argv[ARGS_TCP_IP], get_errno_str());
		return RET_ERROR;
	}

	return RET_OK;
}

int main(int argc, char *argv[])
{
    if (validate_argv(argc, argv) != RET_OK) {
        printf("Shutting down\n");
        return 1;
    }
    int delay = 2;
    
    init_udp_client(argv[ARGS_UDP_IP]);
    thread_create(udp_client_loop, &delay); 

    init_tcp_server(argv[ARGS_TCP_IP]);

    return 0;
}
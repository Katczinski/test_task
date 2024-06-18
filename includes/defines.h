#ifndef DEFINES_H
#define DEFINES_H

#define ARGS_UDP_IP 		1
#define ARGS_TCP_IP 		2
#define ARGS_LOG_PATH 		3
#define ARGS_PREFIX 		4
#define ARGS_EXPECTED_ARGC	5

#define MAX_EVENT_NUM       20

#define TX_BUFF_SIZE        1024
#define PREFIX_SIZE         4
#define RX_BUFF_SIZE        (TX_BUFF_SIZE - PREFIX_SIZE)

#endif
TARGET = target

TEST_UDP = test_udp

TEST_TCP = test_tcp

CC = gcc

CFLAGS = -Wall -Wextra -Werror

TARGET_SRCS = 		main.c\
    				src/errors.c\
					src/log.c\
					src/utils.c\
					src/loop.c\
					src/udp_server.c\
					src/tcp_client.c
 
TARGET_OBJS = 		$(TARGET_SRCS:.c=.o)

TARGET_HEADERS =	includes/errors.h\
					includes/log.h\
					includes/utils.h\
					includes/loop.h\
					includes/return_codes.h\
					includes/defines.h\
					includes/udp_server.h\
					includes/tcp_client.h

TARGET_INCLUDES = 	-Iincludes


TEST_SRCS = 		test_main.c\
					src/utils.c\
					src/errors.c

TEST_OBJS =			$(TEST_SRCS:.c=.o)

TEST_HEADERS =		includes/utils.h\
					includes/errors.h\
					includes/defines.h\
					includes/return_codes.h

TEST_INCLUDES =		-Iincludes

TEST_LIBS = 		-lpthread

$(TARGET): $(TARGET_SRCS) $(TARGET_HEADERS)
	$(CC) $(CFLAGS) $(TARGET_INCLUDES) $(TARGET_SRCS) -o $(TARGET)

$(TEST_UDP): $(TEST_SRCS) $(TEST_HEADERS)
	$(CC) $(CFLAGS) -DUDP $(TEST_INCLUDES) $(TEST_SRCS) -o $(TEST_UDP) $(TEST_LIBS)

$(TEST_TCP): $(TEST_SRCS) $(TEST_HEADERS)
	$(CC) $(CFLAGS) -DTCP $(TEST_INCLUDES) $(TEST_SRCS) -o $(TEST_TCP) $(TEST_LIBS)

# %.o: %.c
# 	$(CC) -g $(CFLAGS) -c $< -o $@

all: $(TARGET) $(TEST_UDP) $(TEST_TCP)

# debug: CFLAGS += -fsanitize=address # causes valgrind error: "error calling PR_SET_PTRACER, vgdb might block"
debug: CFLAGS += -g -DDEBUG
debug: all

clean:
	rm -rf $(TARGET_OBJS)
	rm -rf $(TEST_OBJS)

fclean: clean
	rm -rf $(TARGET)
	rm -rf $(TEST_TCP)
	rm -rf $(TEST_UDP)

re: fclean all

.PHONY: all clean fclean re

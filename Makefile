TARGET = target

TEST = test

CC = gcc

CFLAGS = -Wall -Wextra -Werror

TARGET_SRCS = 		main.c\
    				src/errors.c\
					src/log.c\
					src/utils.c\
					src/loop.c
 
TARGET_OBJS = 		$(TARGET_SRCS:.c=.o)

TARGET_HEADERS =	includes/errors.h\
					includes/log.h\
					includes/utils.h\
					includes/loop.h\
					includes/return_codes.h\
					includes/defines.h

TARGET_INCLUDES = 	-Iincludes

TEST_SRCS = 		test_main.c

TEST_OBJS =			$(TEST_SRCS:.c=.o)

TEST_HEADERS =

TEST_INCLUDES =



$(TARGET): $(TARGET_SRCS) $(TARGET_HEADERS)
	$(CC) $(CFLAGS) $(TARGET_INCLUDES) $(TARGET_SRCS) -o $(TARGET)

$(TEST): $(TEST_SRCS) $(TEST_HEADERS)
	$(CC) $(CFLAGS) $(TEST_INCLUDES) $(TEST_SRCS) -o $(TEST)

# %.o: %.c
# 	$(CC) -g $(CFLAGS) -c $< -o $@

all: $(TARGET) $(TEST)

# debug: CFLAGS += -fsanitize=address # causes valgrind error: "error calling PR_SET_PTRACER, vgdb might block"
debug: CFLAGS += -g -DDEBUG
debug: all

clean:
	rm -rf $(TARGET_OBJS)
	rm -rf $(TEST_OBJS)

fclean: clean
	rm -rf $(TARGET)
	rm -rf $(TEST)

re: fclean all

.PHONY: all clean fclean re

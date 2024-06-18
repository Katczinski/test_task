TARGET = test

CC = gcc

CFLAGS = -Wall -Wextra -Werror

SRCS = 		main.c\
    		src/errors.c\
			src/log.c\
			src/utils.c
 
OBJS = 		$(SRCS:.c=.o)

HEADERS = 	includes/errors.h\
			includes/log.h\
			includes/utils.h\
			includes/return_codes.h

INCLUDES = 	-Iincludes

all: $(TARGET)

$(TARGET): $(SRCS) $(HEADERS)
	$(CC) $(CFLAGS) $(INCLUDES) $(SRCS) -o $(TARGET)

%.o: %.c
	$(CC) -g $(CFLAGS) $(INCLUDES) -c $< -o $@

debug: FLAGS += -fsanitize=address -g
debug: all


clean:
	rm -rf $(OBJS)

fclean: clean
	rm -rf $(TARGET)

re: fclean all

.PHONY: all clean fclean re

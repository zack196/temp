NAME        = webserv

CC          = c++
CFLAGS      = -Wall -Wextra -Werror
RM          = rm -f

HPP     = $(shell find ./include -name '*.hpp')
SRCS    = $(shell find ./src -name '*.cpp')

OBJS        = $(SRCS:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(NAME)

%.o: %.cpp $(HPP)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) $(OBJS)

fclean: clean
	$(RM) $(NAME)

re: fclean all

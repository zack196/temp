# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: zel-oirg <zel-oirg@student.42.fr>          +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2025/01/24 21:18:10 by mregrag           #+#    #+#              #
#    Updated: 2025/04/17 22:37:33 by mregrag          ###   ########.fr        #
#                                                                              #
# **************************************************************************** #


NAME        = Webserv

CC          = c++
CFLAGS      = -Wall -Wextra -Werror -std=c++98 -fsanitize=address -g2
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

NAME = ft_ping

#########
RM = rm -rf
CC = cc
CFLAGS = -Werror -Wextra -Wall -g -fsanitize=address
#########

#########
FILES = main ft_ping

SRC = $(addsuffix .c, $(FILES))

vpath %.c srcs srcs/ping
#########

#########
OBJ_DIR = objs
OBJ = $(addprefix $(OBJ_DIR)/, $(SRC:.c=.o))
DEP = $(addsuffix .d, $(basename $(OBJ)))
#########

#########
$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(@D)
	${CC} -MMD $(CFLAGS) -c -Isrcs/ping $< -o $@

all: 
	$(MAKE) $(NAME) --no-print-directory

$(NAME): $(OBJ) Makefile
	$(CC) $(CFLAGS) $(OBJ) -o $(NAME)
	@echo "EVERYTHING DONE  "

clean:
	$(RM) $(OBJ) $(DEP) --no-print-directory
	$(RM) -r $(OBJ_DIR) --no-print-directory
	@echo "OBJECTS REMOVED   "

fclean: clean
	$(RM) $(NAME) --no-print-directory
	@echo "EVERYTHING REMOVED   "

re:	fclean all

.PHONY: all clean fclean re

-include $(DEP)
# ----- VARS -------------------------------------------------------------------

NAME	= irc
CXX		= c++

INC_DIR	= inc
SRC_DIR	= src
OBJ_DIR	= .obj

INCS	= $(shell find $(INC_DIR) -type f -name '*.h' -o -name '*.hpp')
SRCS	= $(shell find $(SRC_DIR) -type f -name '*.cpp')
OBJS	= $(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

CFLAGS	= -Wall -Wextra -Werror -std=c++98
CINC	= -I$(INC_DIR)
LFLAGS	=
LLIBS	=

ifeq ($(DEBUG), 1)
CFLAGS	+= -fsanitize=address -g3
LFLAGS	+= -fsanitize=address -g3
else ifeq ($(DEBUG), 2)
CFLAGS	+= -g3
LFLAGS	+= -g3
else
CFLAGS	+= #-O3
LFLAGS	+= #-O3
endif


# ----- COLORS -----------------------------------------------------------------

BLACK	=	\033[0;30m
RED		=	\033[0;31m
BLUE	=	\033[0;34m
CYAN	=	\033[0;36m
RESET	=	\033[0m


# ----- RULES ------------------------------------------------------------------

all: $(NAME)

re: fclean all

run:
	@clear && make -sj all && ./$(NAME) $(ARGS)

runvg:
	@clear && make -sj all DEBUG=2 && valgrind ./$(NAME) $(ARGS)

dev:
	@make -s run DEBUG=1

clean:
	@echo -n "$(RED)"
	rm -rf $(OBJ_DIR)
	@echo -n "$(RESET)"

fclean: clean
	@echo -n "$(RED)"
	rm -f $(NAME)
	@echo -n "$(RESET)"

print-%:
	@echo $* = $($*)

.PHONY: all re run debug clean fclean

$(NAME): $(OBJS)
	@echo -n "$(CYAN)"
	$(CXX) $(LFLAGS) $(OBJS) $(LLIBS) -o $@
	@echo -n "$(RESET)"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(INCS) Makefile
	@echo -n "$(BLUE)"
	@mkdir -p $(dir $@)
	$(CXX) $(CFLAGS) $(CINC) -c $< -o $@
	@echo -n "$(RESET)"

CC                 = c++ -std=c++98
CFLAGS             = -Wall -Wextra -Werror -MMD
NAME               = webserv
RM                 = rm -f

EXEC               = webserv
SRCS_PATH	       = ./

CACHE_PATH  	   = cache/
CACHE_SRCS         = $(wildcard *.cpp)
CACHE              = $(addprefix $(CACHE_PATH), $(CACHE_SRCS))

CONFIG_PATH  	   = config/
CONFIG_SRCS        = $(wildcard *.cpp)
CONFIG             = $(addprefix $(CONFIG_PATH), $(CONFIG_SRCS))

HTTP_PATH          = http/
HTTP_SRCS          = request/$(wildcard *.cpp), \
				     response/$(wildcard *.cpp), \
				     router/$(wildcard *.cpp)
HTTP               = $(addprefix $(HTTP_PATH), $(HTTP_SRCS))

MAIN_PATH  	       = main/
MAIN_SRCS          = $(wildcard *.cpp)
MAIN               = $(addprefix $(MAIN_PATH), $(MAIN_SRCS))

MULTIPLEXING_PATH  = multiplexing
MULTIPLEXING_SRCS  = $(wildcard *.cpp)
MULTIPLEXING       = $(addprefix $(MULTIPLEXING_PATH), $(MULTIPLEXING_SRCS))

SERVER_PATH        = server/
SERVER_SRCS        = connection/$(wildcard *.cpp), \
				     server/$(wildcard *.cpp), \
				     error/$(wildcard *.cpp)
SERVER             = $(addprefix $(SERVER_PATH), $(SERVER_SRCS))

UTILITY_PATH       = utility/
UTILITY_SRCS       = $(wildcard *.cpp)
UTILITY            = $(addprefix $(UTILITY_PATH), $(UTILITY_SRCS))

SOURCES			   = $(addprefix $(SRCS_PATH), $(MAIN))\
				     $(addprefix $(SRCS_PATH), $(CACHE))\
				     $(addprefix $(SRCS_PATH), $(CONFIG))\
				     $(addprefix $(SRCS_PATH), $(HTTP))\
				     $(addprefix $(SRCS_PATH), $(MULTIPLEXING))\
				     $(addprefix $(SRCS_PATH), $(UTILITY))

OBJECTS            = $(SOURCES:.cpp=.o)

HEADER             = $(wildcard *.hpp)

all: $(NAME)

$(NAME): $(OBJECTS)
	@$(CC) $(CFLAGS) $(OBJECTS) -o $(EXEC)
	@echo -e "$(GREEN)$(EXEC) created!$(DEFAULT)"

%.o: %.cpp $(HEADER)
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	@$(RM) $(OBJECTS)
	@$(RM) $(OBJECTS:.o=.d)

fclean: clean
	@$(RM) $(EXEC)
	@echo -e "$(BLUE)delete all!$(DEFAULT)"

re:
	@make fclean
	@make all

.PHONY: all clean fclean re

-include $(OBJECTS:.o=.d)

RED = \033[1;31m
PINK = \033[1;35m
GREEN = \033[1;32m
YELLOW = \033[1;33m
BLUE = \033[1;34m
DEFAULT = \033[0m
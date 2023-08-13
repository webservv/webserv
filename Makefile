CC                 = c++ -std=c++98
CFLAGS             = -Wall -Wextra -Werror -MMD
NAME               = webserv
RM                 = rm -f

EXEC               = webserv
SRCS_PATH	       = ./

CACHE_PATH         = cache/
CACHE_SRCS         = $(wildcard $(CACHE_PATH)*.cpp)
CACHE_HEAD         = $(wildcard $(CACHE_PATH)*.hpp)

CGI_PATH           = cgi/
CGI_SRCS           = $(wildcard $(CGI_PATH)*.cpp)
CGI_HEAD           = $(wildcard $(CGI_PATH)*.hpp)

CONFIG_PATH        = config/
CONFIG_SRCS        = $(wildcard $(CONFIG_PATH)*.cpp)
CONFIG_HEAD        = $(wildcard $(CONFIG_PATH)*.hpp)

HTTP_PATH          = http/
HTTP_SRCS          = $(wildcard $(HTTP_PATH)*/*.cpp)
HTTP_HEAD          = $(wildcard $(HTTP_PATH)*/*.hpp)

LOG_PATH           = log/
LOG_SRCS           = $(wildcard $(LOG_PATH)*.cpp)
LOG_HEAD           = $(wildcard $(LOG_PATH)*.hpp)

MAIN_PATH          = main/
MAIN_SRCS          = $(wildcard $(MAIN_PATH)*.cpp)
MAIN_HEAD          = $(wildcard $(MAIN_PATH)*.hpp)

MULTIPLEXING_PATH  = multiplexing/
MULTIPLEXING_SRCS  = $(wildcard $(MULTIPLEXING_PATH)*.cpp)
MULTIPLEXING_HEAD  = $(wildcard $(MULTIPLEXING_PATH)*.hpp)

SERVER_PATH        = server/
SERVER_SRCS        = $(wildcard $(SERVER_PATH)*/*.cpp)
SERVER_HEAD        = $(wildcard $(SERVER_PATH)*/*.hpp)

UTILITY_PATH       = utility/
UTILITY_SRCS       = $(wildcard $(UTILITY_PATH)*.cpp)
UTILITY_HEAD       = $(wildcard $(UTILITY_PATH)*.hpp)

SOURCES            = $(MAIN_SRCS) $(CACHE_SRCS) $(CGI_SRCS) $(CONFIG_SRCS) \
					 $(HTTP_SRCS) $(LOG_SRCS) $(MULTIPLEXING_SRCS) $(UTILITY_SRCS) $(SERVER_SRCS)

OBJECTS            = $(SOURCES:.cpp=.o)

HEADER             = $(MAIN_HEAD) $(CACHE_HEAD) $(CGI_HEAD) $(CONFIG_HEAD) \
					 $(HTTP_HEAD) $(LOG_HEAD) $(MULTIPLEXING_HEAD) $(SERVER_HEAD) $(UTILITY_HEAD)

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
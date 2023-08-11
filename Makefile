CC                 = c++ -std=c++98
CFLAGS             = -Wall -Wextra -Werror -MMD
NAME               = webserv
RM                 = rm -f

EXEC               = webserv
SRCS_PATH	       = ./

CACHE_PATH  	   = cache/
CACHE_SRCS         = $(wildcard *.cpp)
CACHE_HEAD 	       = $(wildcard *.hpp)
CACHE              = $(addprefix $(CACHE_PATH), $(CACHE_SRCS))
CACHE_HEADER       = $(addprefix $(CACHE_PATH), $(CACHE_HEAD))

CGI_PATH  	       = cgi/
CGI_SRCS           = $(wildcard *.cpp)
CGI_HEAD 	       = $(wildcard *.hpp)
CGI                = $(addprefix $(CGI_PATH), $(CGI_SRCS))
CGI_HEADER         = $(addprefix $(CGI_PATH), $(CGI_HEAD))

CONFIG_PATH  	   = config/
CONFIG_SRCS        = $(wildcard *.cpp)
CONFIG_HEAD 	   = $(wildcard *.hpp)
CONFIG             = $(addprefix $(CONFIG_PATH), $(CONFIG_SRCS))
CONFIG_HEADER      = $(addprefix $(CONFIG_PATH), $(CONFIG_HEAD))

HTTP_PATH          = http/
HTTP_SRCS          = request/$(wildcard *.cpp), \
				     response/$(wildcard *.cpp), \
				     router/$(wildcard *.cpp)
HTTP_HEAD 	       = request/$(wildcard *.hpp), \
				     response/$(wildcard *.hpp), \
				     router/$(wildcard *.hpp)
HTTP               = $(addprefix $(HTTP_PATH), $(HTTP_SRCS))
HTTP_HEADER        = $(addprefix $(HTTP_PATH), $(HTTP_HEAD))

LOG_PATH  	       = log/
LOG_SRCS           = $(wildcard *.cpp)
LOG_HEAD 	       = $(wildcard *.hpp)
LOG                = $(addprefix $(LOG_PATH), $(LOG_SRCS))
LOG_HEADER         = $(addprefix $(LOG_PATH), $(LOG_HEAD))

MAIN_PATH  	       = main/
MAIN_SRCS          = $(wildcard *.cpp)
MAIN               = $(addprefix $(MAIN_PATH), $(MAIN_SRCS))
MAIN_HEADER        = $(addprefix $(MAIN_PATH), $(MAIN_HEAD))

MULTIPLEXING_PATH  = multiplexing/
MULTIPLEXING_SRCS  = $(wildcard *.cpp)
MULTIPLEXING_HEAD  = $(wildcard *.hpp)
MULTIPLEXING       = $(addprefix $(MULTIPLEXING_PATH), $(MULTIPLEXING_SRCS))
MULTIPLEXING_HEADER= $(addprefix $(MULTIPLEXING_PATH), $(MULTIPLEXING_HEAD))

SERVER_PATH        = server/
SERVER_SRCS        = connection/$(wildcard *.cpp), \
				     server/$(wildcard *.cpp), \
				     error/$(wildcard *.cpp)
SERVER_HEAD 	   = connection/$(wildcard *.hpp), \
				     server/$(wildcard *.hpp), \
				     error/$(wildcard *.hpp)			
SERVER             = $(addprefix $(SERVER_PATH), $(SERVER_SRCS))
SERVER_HEADER      = $(addprefix $(SERVER_PATH), $(SERVER_HEAD))

UTILITY_PATH       = utility/
UTILITY_SRCS       = $(wildcard *.cpp)
UTILITY_HEAD 	   = $(wildcard *.hpp)
UTILITY            = $(addprefix $(UTILITY_PATH), $(UTILITY_SRCS))
UTILITY_HEADER     = $(addprefix $(UTILITY_PATH), $(UTILITY_HEAD))

SOURCES			   = $(addprefix $(SRCS_PATH), $(MAIN))\
				     $(addprefix $(SRCS_PATH), $(CACHE))\
					 $(addprefix $(SRCS_PATH), $(CGI))\
				     $(addprefix $(SRCS_PATH), $(CONFIG))\
				     $(addprefix $(SRCS_PATH), $(HTTP))\
					 $(addprefix $(SRCS_PATH), $(LOG))\
				     $(addprefix $(SRCS_PATH), $(MULTIPLEXING))\
				     $(addprefix $(SRCS_PATH), $(UTILITY))

OBJECTS            = $(SOURCES:.cpp=.o)

HEADER			   = $(addprefix $(SRCS_PATH), $(MAIN_HEADER))\
				     $(addprefix $(SRCS_PATH), $(CACHE_HEADER))\
					 $(addprefix $(SRCS_PATH), $(CGI_HEADER))\
				     $(addprefix $(SRCS_PATH), $(CONFIG_HEADER))\
				     $(addprefix $(SRCS_PATH), $(HTTP_HEADER))\
					 $(addprefix $(SRCS_PATH), $(LOG_HEADER))\
				     $(addprefix $(SRCS_PATH), $(MULTIPLEXING_HEADER))\
				     $(addprefix $(SRCS_PATH), $(SERVER_HEADER))\
				     $(addprefix $(SRCS_PATH), $(UTILITY_HEADER))

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
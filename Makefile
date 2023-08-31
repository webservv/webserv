CXXFLAGS		= -Wall -Wextra -Werror -MMD -std=c++98
NAME			= webserv
RM				= rm -f

EXEC			= webserv
SRCS_PATH		= ./

CGI_PATH		= cgi/
CGI_SRCS		= $(wildcard $(CGI_PATH)*.cpp)
CGI_HEAD		= $(wildcard $(CGI_PATH)*.hpp)

CONFIG_PATH		= config/
CONFIG_SRCS		= $(wildcard $(CONFIG_PATH)*.cpp)
CONFIG_HEAD		= $(wildcard $(CONFIG_PATH)*.hpp)

HTTP_PATH		= http/
HTTP_SRCS		= $(wildcard $(HTTP_PATH)*.cpp)
HTTP_HEAD		= $(wildcard $(HTTP_PATH)*.hpp)

MAIN_PATH		= main/
MAIN_SRCS		= $(wildcard $(MAIN_PATH)*.cpp)
MAIN_HEAD		= $(wildcard $(MAIN_PATH)*.hpp)

SERVER_PATH		= server/
SERVER_SRCS		= $(wildcard $(SERVER_PATH)*.cpp)
SERVER_HEAD		= $(wildcard $(SERVER_PATH)*.hpp)

SOURCES			= $(MAIN_SRCS) $(CGI_SRCS) $(CONFIG_SRCS) \
					 $(HTTP_SRCS) $(SERVER_SRCS)

OBJECTS			= $(SOURCES:.cpp=.o)

INCLUDE_DIRS	= $(CGI_PATH) $(CONFIG_PATH) $(HTTP_PATH) \
				 $(SERVER_PATH)
INCLUDE_FLAGS	= $(addprefix -I, $(INCLUDE_DIRS))

HEADER			= $(MAIN_HEAD) $(CGI_HEAD) $(CONFIG_HEAD) \
					 $(HTTP_HEAD) $(SERVER_HEAD)


ifdef DEBUG
	CXXFLAGS = -Wall -Wextra -Werror -MMD -std=c++98 -g
endif

all: $(NAME)

$(NAME): $(OBJECTS)
	@$(CXX) $(CXXFLAGS) $(OBJECTS) -o $(EXEC)
	@echo -e "$(GREEN)$(EXEC) created!$(DEFAULT)"

# %.o: %.cpp $(HEADER)
# 	@$(CXX) $(CXXFLAGS) -c $< -o $@
%.o: %.cpp $(HEADER)
	@$(CXX) $(CXXFLAGS) $(INCLUDE_FLAGS) -c $< -o $@

clean:
	@$(RM) $(OBJECTS)
	@$(RM) $(OBJECTS:.o=.d)

fclean: clean
	@$(RM) $(EXEC)
	@echo -e "$(BLUE)delete all!$(DEFAULT)"

re:
	@make fclean
	@make all

debug: fclean
	make DEBUG=1 -j

.PHONY: all clean fclean re debug

-include $(OBJECTS:.o=.d)

RED = \033[1;31m
PINK = \033[1;35m
GREEN = \033[1;32m
YELLOW = \033[1;33m
BLUE = \033[1;34m
DEFAULT = \033[0m
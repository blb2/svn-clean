CXX      = g++
CXXFLAGS = -std=c++11
RM       = @rm
MKDIR    = @mkdir -p
NAME     = svn-clean
SRCS     = svn-clean.cpp platform_posix.cpp
OBJSDIR  = obj
OBJS     = $(addprefix $(OBJSDIR)/,$(SRCS:.cpp=.o))
DEPS     = $(addprefix $(OBJSDIR)/,$(SRCS:.cpp=.d))

ifeq ($(DEBUG),1)
	CXXFLAGS += -g
else
	CXXFLAGS += -O3 -DNDEBUG
endif

.SUFFIXES:
.SUFFIXES: .cpp .d .h .o

.PHONY: all
all: $(NAME)

.PHONY: clean
clean:
	$(RM) -rf $(NAME) $(OBJSDIR) *~

-include $(DEPS)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(OBJSDIR)/%.o: %.cpp
	$(MKDIR) $(dir $@)
	$(CXX) $(CXXFLAGS) -MMD -c -o $@ $<

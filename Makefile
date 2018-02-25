CXX    ?= g++
RM      = @rm
MKDIR   = @mkdir -p
NAME    = svn-clean
SRCS    = src/svn-clean.cpp src/platform_posix.cpp
OBJSDIR = obj
OBJS    = $(addprefix $(OBJSDIR)/,$(SRCS:.cpp=.o))
DEPS    = $(addprefix $(OBJSDIR)/,$(SRCS:.cpp=.d))

override CXXFLAGS += -std=c++11

ifeq ($(DEBUG),1)
override CXXFLAGS += -g
else
override CXXFLAGS += -O3 -DNDEBUG
endif

override CXXFLAGS += -DPUGIXML_HEADER_ONLY

.SUFFIXES:
.SUFFIXES: .d .cpp .h .o

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

CXX=g++
CXXFLAGS=-g -O0 -std=c++14 -Wall -Wextra -pedantic -DBOOST_LOG_DYN_LINK
LDFLAGS=-lboost_filesystem -lboost_system -lboost_thread -lboost_log_setup -lboost_log -lpthread -lssl -lcrypto -lrestbed -ljsoncpp

OBJDIR=obj
CXXFILES := $(shell find src -mindepth 1 -maxdepth 4 -name "*.cpp")
OBJFILES := $(CXXFILES:src/%.cpp=%)
OFILES := $(OBJFILES:%=$(OBJDIR)/%.o)

TARGET=xdccd

all: $(OBJDIR) $(TARGET)

## Execution
run: all
	./$(TARGET)

valgrind: all
	valgrind --tool=memcheck --leak-check=full --show-leak-kinds=definite,indirect,possible --show-reachable=no --leak-resolution=high --num-callers=20 --trace-children=no --child-silent-after-fork=yes --track-fds=yes --track-origins=yes ./$(TARGET) 2>&1 | tee valgrind.log

helgrind: all
	valgrind --tool=helgrind ./$(TARGET) 2>&1 | tee helgrind.log

callgrind: all
	valgrind --tool=callgrind ./$(TARGET)

## Utility
clean:
	@rm -f $(TARGET)
	@rm -rf $(OBJDIR)

$(OBJDIR):
	mkdir -p obj

$(OBJDIR)/%.o: src/%.cpp
	$(CXX) -o $@ -c $< $(CXXFLAGS)

$(TARGET): $(OFILES)
	$(CXX) -o $@ $(OFILES) $(LDFLAGS)

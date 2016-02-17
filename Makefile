CXX=clang++
CXXFLAGS=-g -I/usr/include/libircclient -std=c++14 -Wall -Wextra -pedantic
LDFLAGS=-lircclient -lboost_filesystem -lboost_system -lboost_thread -lpthread

OBJDIR=obj
CXXFILES := $(shell find src -mindepth 1 -maxdepth 4 -name "*.cpp")
OBJFILES := $(CXXFILES:src/%.cpp=%)
OFILES := $(OBJFILES:%=$(OBJDIR)/%.o)

TARGET=xdccd

all: $(OBJDIR) $(TARGET)

clean:
	@rm -f $(TARGET)
	@rm -rf $(OBJDIR)

$(OBJDIR):
	mkdir -p obj

$(OBJDIR)/%.o: src/%.cpp
	$(CXX) -o $@ -c $< $(CXXFLAGS)

$(TARGET): $(OFILES)
	$(CXX) -o $@ $(OFILES) $(LDFLAGS)

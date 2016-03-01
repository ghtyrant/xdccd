CXX=clang++
CXXFLAGS=-g -std=c++14 -Wall -Wextra -pedantic
LDFLAGS=-lboost_filesystem -lboost_system -lboost_thread -lpthread -lssl -lcrypto -lrestbed -ljsoncpp

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

run: all
	./$(TARGET)

$(OBJDIR)/%.o: src/%.cpp
	$(CXX) -o $@ -c $< $(CXXFLAGS)

$(TARGET): $(OFILES)
	$(CXX) -o $@ $(OFILES) $(LDFLAGS)

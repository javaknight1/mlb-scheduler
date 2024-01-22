CXX = g++
CXXFLAGS = -std=c++11
INCLUDE_DIRS = -I/opt/homebrew/Cellar/jsoncpp/1.9.5/include
LIB_DIRS = -L/opt/homebrew/Cellar/jsoncpp/1.9.5/lib
LIBS = -ljsoncpp

SRCS = mlb-scheduler.cpp
OBJS = $(SRCS:.cpp=.o)
EXECUTABLE = mlb-scheduler.app

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJS)
	$(CXX) $(LIB_DIRS) $(LIBS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDE_DIRS) -c -o $@ $<

clean:
	rm -f $(EXECUTABLE) $(OBJS)

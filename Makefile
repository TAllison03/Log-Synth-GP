# Compiler Setup
CXX = g++
CXXFLAGS = -Wall -g -I./abc/src -DLIN64

# ABC library dependencies
LIBS = ./abc/libabc.a -lm -ldl -lreadline -lpthread

# Executable name
TARGET = optimizer

# Source files
SRCS = optimizer.cpp
OBJS = $(SRCS:.cpp=.o)

# Default target
all: $(TARGET)

# Link to executable
$(TARGET): $(OBJS)
		$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) $(LIBS)

# Compile C++ source files (.cpp) into object (.o) files
%.o: %.cpp optimizer.h
		$(CXX) $(CXXFLAGS) -c $< -o $@

# Cleanup
clean:
		rm -f $(OBJS) $(TARGET)
		rm -f temp_*.blif step_*.blif
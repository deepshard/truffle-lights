# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -I.
LDFLAGS =

# Source files
SOURCES = main.cc ledmgr.cc

# Object files
OBJECTS = $(SOURCES:.cc=.o)

# Executable name
TARGET = led_demo

# Default target
all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up build files
clean:
	rm -f $(OBJECTS) $(TARGET)

# Phony targets
.PHONY: all clean 
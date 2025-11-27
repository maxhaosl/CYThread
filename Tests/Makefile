# Makefile for CYThread Test

CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -O2 -g -mmacosx-version-min=11.0
INCLUDES = -I../Inc -I../Src
LDFLAGS = -L../Build/out/macos/shared/Debug
LIBS = -lCYThread -lpthread

# macOS specific
ifeq ($(shell uname),Darwin)
    LIBS += -framework Foundation
    RPATH = -Wl,-rpath,../Build/out/macos/shared/Debug
endif

# Linux specific
ifeq ($(shell uname),Linux)
    RPATH = -Wl,-rpath,../Build/out/linux/shared/Debug
    LDFLAGS = -L../Build/out/linux/shared/Debug
endif

TARGET = SimpleTest
SOURCES = SimpleTest.cpp

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(SOURCES)
	@echo "Building $(TARGET)..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SOURCES) -o $(TARGET) $(LDFLAGS) $(LIBS) $(RPATH)
	@echo "Build completed: $(TARGET)"

clean:
	rm -f $(TARGET)
	@echo "Cleaned"

run: $(TARGET)
	@echo "Running $(TARGET)..."
	@echo "=========================="
	./$(TARGET)

help:
	@echo "Available targets:"
	@echo "  all     - Build the test program"
	@echo "  clean   - Remove built files"
	@echo "  run     - Build and run the test"
	@echo "  help    - Show this help message"

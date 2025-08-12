SLASH = /
BUILD_DIRECTORY = Build
TEST_DIRECTORY = tests
TEST_BUILD_DIRECTORY = $(BUILD_DIRECTORY)$(SLASH)tests

# Compiler settings
CC = gcc
CXX = g++
CFLAGS = -Wall -Wextra -std=c99
CXXFLAGS = -Wall -Wextra -std=c++11
CLIENT_INCLUDES = -I src$(SLASH)client$(SLASH)include
INCLUDES = -I src$(SLASH)include

# CppUTest settings
CPPUTEST_HOME = /usr
CPPFLAGS += -I$(CPPUTEST_HOME)/include
CXXFLAGS += -include $(CPPUTEST_HOME)/include/CppUTest/MemoryLeakDetectorNewMacros.h
LD_LIBRARIES = -L$(CPPUTEST_HOME)/lib/aarch64-linux-gnu -lCppUTest -lCppUTestExt

# Source files
CLIENT_SOURCES = src$(SLASH)client$(SLASH)include$(SLASH)connect.c
TEST_SOURCES = $(wildcard $(TEST_DIRECTORY)$(SLASH)test_*.cpp)
TEST_OBJECTS = $(TEST_SOURCES:$(TEST_DIRECTORY)/%.cpp=$(TEST_BUILD_DIRECTORY)/%.o)

all: $(BUILD_DIRECTORY) client server

client: $(BUILD_DIRECTORY)
	$(CC) $(CFLAGS) src$(SLASH)client$(SLASH)client.c $(CLIENT_SOURCES) $(CLIENT_INCLUDES) $(INCLUDES) -o $(BUILD_DIRECTORY)$(SLASH)client

server: $(BUILD_DIRECTORY)
	$(CC) $(CFLAGS) src$(SLASH)server$(SLASH)server.c $(INCLUDES) -o $(BUILD_DIRECTORY)$(SLASH)server

# Test targets
test: $(TEST_BUILD_DIRECTORY)$(SLASH)test_runner
	./$(TEST_BUILD_DIRECTORY)$(SLASH)test_runner

$(TEST_BUILD_DIRECTORY)$(SLASH)test_runner: $(TEST_OBJECTS) $(TEST_BUILD_DIRECTORY)$(SLASH)connect.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LD_LIBRARIES)

$(TEST_BUILD_DIRECTORY)$(SLASH)%.o: $(TEST_DIRECTORY)$(SLASH)%.cpp | $(TEST_BUILD_DIRECTORY)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(INCLUDES) -c $< -o $@

$(TEST_BUILD_DIRECTORY)$(SLASH)connect.o: src$(SLASH)client$(SLASH)include$(SLASH)connect.c | $(TEST_BUILD_DIRECTORY)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIRECTORY):
	@if [ ! -d $(BUILD_DIRECTORY) ]; then mkdir -p $(BUILD_DIRECTORY); fi

$(TEST_BUILD_DIRECTORY):
	@if [ ! -d $(TEST_BUILD_DIRECTORY) ]; then mkdir -p $(TEST_BUILD_DIRECTORY); fi

clean:
	rm -rf $(BUILD_DIRECTORY)

clean_tests:
	rm -rf $(TEST_BUILD_DIRECTORY)

.PHONY: all client server test clean clean_tests
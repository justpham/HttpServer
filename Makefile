SLASH = /
BUILD_DIRECTORY = Build
TEST_DIRECTORY = tests
TEST_BUILD_DIRECTORY = $(BUILD_DIRECTORY)$(SLASH)tests

# Compiler settings
CC = gcc
CXX = g++
CFLAGS = -Wall -Wextra -std=c99
CXXFLAGS = -Wall -Wextra -std=c++11

# Libraries
INCLUDES = -I src$(SLASH)include
CLIENT_INCLUDES = -I src$(SLASH)client$(SLASH)include
SERVER_INCLUDES = -I src$(SLASH)server$(SLASH)include

# CppUTest settings
CPPUTEST_HOME = /usr
CPPFLAGS += -I$(CPPUTEST_HOME)/include
CXXFLAGS += -include $(CPPUTEST_HOME)/include/CppUTest/MemoryLeakDetectorNewMacros.h
LD_LIBRARIES = -L$(CPPUTEST_HOME)/lib/aarch64-linux-gnu -lCppUTest -lCppUTestExt

# Source Files
COMMON_SOURCES = src$(SLASH)include$(SLASH)ip_helper.c src$(SLASH)include$(SLASH)cJSON.c src$(SLASH)include$(SLASH)http_parser.c src$(SLASH)include$(SLASH)http_lib.c src$(SLASH)include$(SLASH)http_builder.c
CLIENT_SOURCES = src$(SLASH)client$(SLASH)include$(SLASH)connect.c $(COMMON_SOURCES)
SERVER_SOURCES = src$(SLASH)server$(SLASH)include$(SLASH)connect.c $(COMMON_SOURCES)

# Test Source Files
TEST_SOURCES = $(wildcard $(TEST_DIRECTORY)$(SLASH)test_*.cpp)
TEST_OBJECTS = $(TEST_SOURCES:$(TEST_DIRECTORY)/%.cpp=$(TEST_BUILD_DIRECTORY)/%.o)

all: $(BUILD_DIRECTORY) client server

client: $(BUILD_DIRECTORY)
	$(CC) $(CFLAGS) src$(SLASH)client$(SLASH)client.c $(CLIENT_SOURCES) $(CLIENT_INCLUDES) $(INCLUDES) -o $(BUILD_DIRECTORY)$(SLASH)client

server: $(BUILD_DIRECTORY)
	$(CC) $(CFLAGS) src$(SLASH)server$(SLASH)server.c $(SERVER_SOURCES) $(SERVER_INCLUDES) $(INCLUDES) -o $(BUILD_DIRECTORY)$(SLASH)server

# Test targets
test: $(TEST_BUILD_DIRECTORY)$(SLASH)test_runner
	./$(TEST_BUILD_DIRECTORY)$(SLASH)test_runner -v

$(TEST_BUILD_DIRECTORY)$(SLASH)test_runner: $(TEST_OBJECTS) $(TEST_BUILD_DIRECTORY)$(SLASH)connect.o $(TEST_BUILD_DIRECTORY)$(SLASH)ip_helper.o $(TEST_BUILD_DIRECTORY)$(SLASH)cJSON.o $(TEST_BUILD_DIRECTORY)$(SLASH)http_parser.o $(TEST_BUILD_DIRECTORY)$(SLASH)http_lib.o $(TEST_BUILD_DIRECTORY)$(SLASH)http_builder.o $(TEST_BUILD_DIRECTORY)$(SLASH)send_mock.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LD_LIBRARIES)

$(TEST_BUILD_DIRECTORY)$(SLASH)%.o: $(TEST_DIRECTORY)$(SLASH)%.cpp | $(TEST_BUILD_DIRECTORY)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(INCLUDES) $(CLIENT_INCLUDES) $(SERVER_INCLUDES) -c $< -o $@

$(TEST_BUILD_DIRECTORY)$(SLASH)connect.o: src$(SLASH)client$(SLASH)include$(SLASH)connect.c | $(TEST_BUILD_DIRECTORY)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(TEST_BUILD_DIRECTORY)$(SLASH)ip_helper.o: src$(SLASH)include$(SLASH)ip_helper.c | $(TEST_BUILD_DIRECTORY)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(TEST_BUILD_DIRECTORY)$(SLASH)cJSON.o: src$(SLASH)include$(SLASH)cJSON.c | $(TEST_BUILD_DIRECTORY)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(TEST_BUILD_DIRECTORY)$(SLASH)http_parser.o: src$(SLASH)include$(SLASH)http_parser.c | $(TEST_BUILD_DIRECTORY)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(TEST_BUILD_DIRECTORY)$(SLASH)http_lib.o: src$(SLASH)include$(SLASH)http_lib.c | $(TEST_BUILD_DIRECTORY)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(TEST_BUILD_DIRECTORY)$(SLASH)http_builder.o: src$(SLASH)include$(SLASH)http_builder.c | $(TEST_BUILD_DIRECTORY)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(TEST_BUILD_DIRECTORY)$(SLASH)send_mock.o: $(TEST_DIRECTORY)$(SLASH)mocks$(SLASH)send_mock.c | $(TEST_BUILD_DIRECTORY)
	$(CC) $(CFLAGS) $(INCLUDES) -I$(CPPUTEST_HOME)/include -c $< -o $@

$(BUILD_DIRECTORY):
	@if [ ! -d $(BUILD_DIRECTORY) ]; then mkdir -p $(BUILD_DIRECTORY); fi

$(TEST_BUILD_DIRECTORY):
	@if [ ! -d $(TEST_BUILD_DIRECTORY) ]; then mkdir -p $(TEST_BUILD_DIRECTORY); fi

clean:
	rm -rf $(BUILD_DIRECTORY)

clean_tests:
	rm -rf $(TEST_BUILD_DIRECTORY)

# Linting targets
lint: format-check cppcheck

format:
	@echo "Formatting C source files..."
	@find src -name "*.c" -not -path "*/cJSON.c" | xargs clang-format -i
	@find src -name "*.h" -not -path "*/cJSON.h" | xargs clang-format -i
	@echo "Formatting complete."

format-check:
	@echo "Checking code formatting..."
	@find src -name "*.c" -not -path "*/cJSON.c" | xargs clang-format --dry-run --Werror
	@find src -name "*.h" -not -path "*/cJSON.h" | xargs clang-format --dry-run --Werror
	@echo "Format check passed."

cppcheck:
	@echo "Running static analysis with cppcheck..."
	@cppcheck --enable=all --std=c99 --language=c \
		--suppressions-list=.cppcheck-suppressions \
		--error-exitcode=1 \
		--inline-suppr \
		-I src/include \
		-I src/client/include \
		-I src/server/include \
		src/
	@echo "Static analysis passed."

cppcheck-report:
	@echo "Generating cppcheck XML report..."
	@cppcheck --enable=all --std=c99 --language=c \
		--suppressions-list=.cppcheck-suppressions \
		--inline-suppr \
		-I src/include \
		-I src/client/include \
		-I src/server/include \
		--xml --xml-version=2 \
		src/ 2> cppcheck-report.xml
	@echo "Report generated: cppcheck-report.xml"

.PHONY: all client server test clean clean_tests lint format format-check cppcheck cppcheck-report
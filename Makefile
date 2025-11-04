SLASH = /
BUILD_DIRECTORY = Build

# Compiler settings
CC = gcc
CXX = g++
CFLAGS = -Wall -Wextra -std=c99 -g
CXXFLAGS = -Wall -Wextra -std=c++11

# Libraries
INCLUDES = -I src$(SLASH)include
CLIENT_INCLUDES = -I src$(SLASH)client$(SLASH)include
SERVER_INCLUDES = -I src$(SLASH)server$(SLASH)include

# Source Files
COMMON_SOURCES = src$(SLASH)include$(SLASH)ip_helper.c src$(SLASH)include$(SLASH)http_parser.c src$(SLASH)include$(SLASH)http_lib.c src$(SLASH)include$(SLASH)http_builder.c src$(SLASH)include$(SLASH)random.c
CLIENT_SOURCES = src$(SLASH)client$(SLASH)include$(SLASH)connect.c $(COMMON_SOURCES)
SERVER_SOURCES = src$(SLASH)server$(SLASH)include$(SLASH)routes.c src$(SLASH)server$(SLASH)include$(SLASH)connect.c src$(SLASH)server$(SLASH)include$(SLASH)conn_map.c  $(COMMON_SOURCES)

all: $(BUILD_DIRECTORY) server

server: $(BUILD_DIRECTORY)
	$(CC) $(CFLAGS) src$(SLASH)server$(SLASH)server.c $(SERVER_SOURCES) $(SERVER_INCLUDES) $(INCLUDES) -o $(BUILD_DIRECTORY)$(SLASH)server

$(BUILD_DIRECTORY):
	@if [ ! -d $(BUILD_DIRECTORY) ]; then mkdir -p $(BUILD_DIRECTORY); fi

clean:
	rm -rf $(BUILD_DIRECTORY)

# Linting targets
lint: format-check cppcheck

format:
	@echo "Formatting C source files..."
	@find src -name "*.c" | xargs clang-format -i
	@find src -name "*.h" | xargs clang-format -i
	@echo "Formatting complete."

format-check:
	@echo "Checking code formatting..."
	@find src -name "*.c" | xargs clang-format --dry-run --Werror
	@find src -name "*.h" | xargs clang-format --dry-run --Werror
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

.PHONY: all server clean lint format format-check cppcheck cppcheck-report
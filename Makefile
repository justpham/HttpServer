SLASH = /
BUILD_DIRECTORY = Build

all: $(BUILD_DIRECTORY) client server

client: $(BUILD_DIRECTORY)
	gcc src$(SLASH)client$(SLASH)client.c -o $(BUILD_DIRECTORY)$(SLASH)client

server: $(BUILD_DIRECTORY)
	gcc src$(SLASH)server$(SLASH)server.c -o $(BUILD_DIRECTORY)$(SLASH)server

$(BUILD_DIRECTORY):
	@if [ ! -d $(BUILD_DIRECTORY) ]; then mkdir -p $(BUILD_DIRECTORY); fi

clean:
	rm -rf $(BUILD_DIRECTORY)
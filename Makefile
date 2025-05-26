SLASH = /
BUILD_DIRECTORY = Build

all: $(BUILD_DIRECTORY) main
	gcc $(BUILD_DIRECTORY)$(SLASH)main.o -o $(BUILD_DIRECTORY)$(SLASH)HttpServer

main: $(BUILD_DIRECTORY)
	gcc -c src$(SLASH)main.c -o $(BUILD_DIRECTORY)$(SLASH)main.o

$(BUILD_DIRECTORY):
	@if [ ! -d $(BUILD_DIRECTORY) ]; then mkdir -p $(BUILD_DIRECTORY); fi

clean:
	rm -rf $(BUILD_DIRECTORY)
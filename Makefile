CC     = gcc
CFLAGS = -std=c11 -Wall -Wextra -g -Iinclude
GTK_CFLAGS = `pkg-config --cflags gtk+-3.0`
GTK_LIBS = `pkg-config --libs gtk+-3.0`

# Core system sources
CORE_SRCS = src/file_handler.c src/utils.c src/position.c src/candidate.c \
         src/voter.c src/auth.c src/voting.c src/tally.c

# Final GUI version sources
FINAL_GUI_SRCS = $(CORE_SRCS) src/final_gui.c

CONSOLE_TARGET = bin/sonu-console
FINAL_GUI_TARGET = bin/sonu-voting-system

all: $(CONSOLE_TARGET) $(FINAL_GUI_TARGET)

# Console version (terminal-only)
$(CONSOLE_TARGET): $(CORE_SRCS) src/main.c
	@mkdir -p bin
	$(CC) $(CFLAGS) $(CORE_SRCS) src/main.c -o $(CONSOLE_TARGET)

# Final GUI version (dual-mode: GUI + Terminal)
$(FINAL_GUI_TARGET): $(FINAL_GUI_SRCS)
	@mkdir -p bin
	$(CC) $(CFLAGS) $(GTK_CFLAGS) $(FINAL_GUI_SRCS) $(GTK_LIBS) -o $(FINAL_GUI_TARGET)

# Run targets
run: $(FINAL_GUI_TARGET)
	./$(FINAL_GUI_TARGET)

run-gui: $(FINAL_GUI_TARGET)
	./$(FINAL_GUI_TARGET)

run-terminal: $(FINAL_GUI_TARGET)
	./$(FINAL_GUI_TARGET) --terminal

run-console: $(CONSOLE_TARGET)
	./$(CONSOLE_TARGET)

# Build targets
build: $(FINAL_GUI_TARGET)

clean:
	rm -rf bin/

install-deps:
	@echo "Installing GTK development packages..."
	sudo apt-get update && sudo apt-get install -y libgtk-3-dev pkg-config

.PHONY: all run run-gui run-terminal run-console build clean install-deps

CC     = gcc
CFLAGS = -std=c11 -Wall -Wextra -g -Iinclude

SRCS = src/file_handler.c \
       src/utils.c \
       src/position.c \
       src/candidate.c \
       src/voter.c \
       src/auth.c \
       src/admin.c \
       src/voting.c \
       src/tally.c \
       src/application.c \
       src/net_handler.c \
       src/server_main.c

TARGET = bin/sonu_server

all: $(TARGET)

$(TARGET): $(SRCS)
	@mkdir -p bin
	@mkdir -p data
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET)

run: all
	./$(TARGET)

clean:
	rm -rf bin/

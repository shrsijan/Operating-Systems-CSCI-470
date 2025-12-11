# Group Members: 
# Sijan Shrestha
# Utsav Shah

CC = gcc
CFLAGS = -Wall -g
LDFLAGS = -lpthread

# Target executable
TARGET = server

# Source files
SRCS = server.c server_client.c list.c

# Object files
OBJS = $(SRCS:.c=.o)

# Header files
HDRS = server.h list.h

# Default target
all: $(TARGET)

# Link object files to create executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

# Compile source files to object files
%.o: %.c $(HDRS)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up
clean:
	rm -f $(OBJS) $(TARGET)

# Rebuild
rebuild: clean all

.PHONY: all clean rebuild
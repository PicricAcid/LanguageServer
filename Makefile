CC = gcc
SRCS = main.c json_parse.c interpreter.c language_server.c
OBJS = $(SRCS:.c=.o)
TARGET = main

$(TARGET): $(OBJS)
	$(CC) -o $@ $^

$(OBJS): $(SRCS) 
	$(CC) -c $(SRCS)

clean: 
	rm -f $(OBJS) $(TARGET)

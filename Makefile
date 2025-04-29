#CC = gcc
#CFLAGS = -Wall -pthread

# Source files
#SRCS = main.c peer.c tracker.c

# Object files
#OBJS = main.o peer.o tracker.o

# Output executable name
#TARGET = my_program

#all: $(TARGET)

#$(TARGET): $(OBJS)
#	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

#%.o: %.c common.h
#	$(CC) $(CFLAGS) -c $< -o $@

#clean:
#	rm -f $(OBJS) $(TARGET)

CC = gcc
CFLAGS = -Wall -pthread

OBJS = main.o peer.o tracker.o
TARGET = project

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS)  $(OBJS) -o $(TARGET) -lpthread

main.o: main.c common.h
	$(CC) $(CFLAGS) -c main.c

peer.o: peer.c common.h
	$(CC) $(CFLAGS) -c peer.c

tracker.o: tracker.c common.h
	$(CC) $(CFLAGS) -c tracker.c

run:
	./$(TARGET)

clean:
	rm -f *.o $(TARGET)

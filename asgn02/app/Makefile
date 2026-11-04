CC = gcc
CFLAGS = -Wall -pthread
TARGET = sms_app

all: $(TARGET)

$(TARGET): sms_app.c circular_buffer.c
	$(CC) $(CFLAGS) -o $@ $^ -lrt

clean:
	rm -f $(TARGET)

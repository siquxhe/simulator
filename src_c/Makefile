CC ?= gcc
CFLAGS ?= -O2 -I.

TARGET = simulator

.PHONY: all
all: $(TARGET)

$(TARGET): main.c 103.c 103.h tape.h err.h
	$(CC) $(CFLAGS) main.c 103.c -o $(TARGET)

.PHONY: clean
clean:
	rm -f *.o $(TARGET) $(TARGET).exe

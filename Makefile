TARGET = sha256
CROSS_COMPILE = arm-linux-gnueabihf-
CFLAGS = -std=c99 -g -Wall -I ${SOCEDS_DEST_ROOT}/ip/altera/hps/altera_hps/hwlib/include
LDFLAGS = -std=c99 -g -Wall
CC = $(CROSS_COMPILE)gcc
ARCH = arm
build: $(TARGET)
$(TARGET): main.o
	$(CC) $(LDFLAGS) $^ -o $@
%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@
.PHONY: clean
clean:
	rm -f $(TARGET) *.a *.o *~
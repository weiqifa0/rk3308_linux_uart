


ALSA_INCLUDE := /mmt/rk3308_os/buildroot/output/rockchip_rk3308_release/host/include
ALSA_LIB := /mmt/rk3308_os/buildroot/output/rockchip_rk3308_release/host/lib

CC := /mmt/rk3308_os/buildroot/output/rockchip_rk3308_release/host/bin/aarch64-linux-gcc
CXX := /mmt/rk3308_os/buildroot/output/rockchip_rk3308_release/host/bin/aarch64-linux-g++
CFLAGS = -Wall -g -lpthread -lrt

TARGET_OUTPUT = rk3308_uart_main.elf
DEPEND_OBJS = main.o uart.o

all: clean $(TARGET_OUTPUT)

$(TARGET_OUTPUT): $(DEPEND_OBJS)
	$(CC) $(CFLAGS) -o $(TARGET_OUTPUT) $(DEPEND_OBJS)

uart.o: uart.c
	$(CC) $(CFLAGS) -c uart.c

main.o: main.c
	$(CC) $(CFLAGS) -c main.c -lpthread -lrt

.PHONY: clean

clean:
	rm -rf  *.o *.elf
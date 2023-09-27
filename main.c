/*
** Copyright (c) 2023 The Roomix project. All rights reserved.
** Created by crisqifawei 2023
*/

#include "uart.h"

static volatile bool g_is_quit = false;

static void *serial_port_receiving_thread(void *arg)
{
	int ret, i, n;
	int fd;
	fd_set fds;
	struct timeval time = {5, 500000};
	char buf[RK3308_UART_READ_BUFF_SIZE_BYTES];
	fd = *(int*)arg;
	for(;!g_is_quit;) {
		FD_ZERO(&fds);
		FD_SET(fd, &fds);
		ret = select(fd + 1, &fds, NULL, NULL, &time);
		if (ret < 0) {
			uart_debug("select failed ret = %d fd = %d", ret, fd);
			continue;
		}
		if (FD_ISSET(fd, &fds)) {
			do {
				n = read(fd, buf, RK3308_UART_READ_BUFF_SIZE_BYTES);
				for (i = 0; i < n; i++) {
					printf("%.2x ", buf[i]);
				}
				putchar('\n');
			} while(0 < n);
		}
	}

	return NULL;
}

void sighandler(int signum) {
  g_is_quit = true;
  uart_debug("Quit");
}

int main(int argc, char **argv)
{
	int ret, fd, baud_rate;
	char buf[RK3308_UART_SEND_BUFF_SIZE_BYTES];
	pthread_t serial_receive_t;

	if(argc < 2) {
		uart_debug("Usage:   %s /dev/ttySx    #0(open uart x data)", argv[0]);
		uart_debug("Example: %s /dev/ttyS1    #1(open uart 1 data)", argv[1]);
		uart_debug("Example: %s %s /dev/ttyS1 115200   #1(open uart 1 speed 115200)", argv[1], argv[1]);
		return RK3308_UART_ERR;
	}

	fd = rk3308_uart_open(argv[1]);
	if(RK3308_UART_ERR == fd) {
		uart_debug("rk3308_uart_open error = %d", fd);
		return RK3308_UART_ERR;
	}

	if (argc == 2) {
		ret = rk3308_uart_init(fd, RK3308_UART_DEFAULT_BAUDRATE, 0, 8, 1, 'N');
		if(RK3308_UART_ERR == ret) {
			uart_debug("rk3308_uart_init error = %d",ret);
			return RK3308_UART_ERR;
		}
	} else {
		if ((baud_rate = atoi(argv[2])) < 0) {
			baud_rate = RK3308_UART_DEFAULT_BAUDRATE;
		}
		ret = rk3308_uart_init(fd, baud_rate, 0, 8, 1, 'N');
		if(RK3308_UART_ERR == ret) {
			uart_debug("rk3308_uart_init error = %d",ret);
			return RK3308_UART_ERR;
		}
	}

	if (pthread_create(&serial_receive_t, NULL, serial_port_receiving_thread, &fd) < 0) {
		uart_debug("pthread_create error");
		return RK3308_UART_ERR;
	}

	signal(SIGINT, sighandler);

	for(; !g_is_quit ;) {
		putchar('#');
		fgets(buf, RK3308_UART_SEND_BUFF_SIZE_BYTES, stdin);
		if(RK3308_UART_OK != rk3308_uart_send(fd, buf, strlen(buf))) {
			uart_debug("time send data failed");
		}
	}

	pthread_join(serial_receive_t, NULL);
	return rk3308_uart_close(fd);
}

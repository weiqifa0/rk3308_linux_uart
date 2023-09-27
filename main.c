/*
** Copyright (c) 2023 The Roomix project. All rights reserved.
** Created by crisqifawei 2023
*/

#include "uart.h"

struct uart_stu stu_uart_data = {0, 0};

static void *uart_read_thread_function(void *arg)
{
	int ret, i, n;
	int fd;
	fd_set fds;
	struct timeval time = {5, 500000};
	char buf[RK3308_UART_READ_BUFF_SIZE_BYTES];
	fd = *(int*)arg;

	for(; !stu_uart_data.is_runing ;) {
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
				if (stu_uart_data.is_read_data_show_hex) {
					for (i = 0; i < n; i++) {
						printf("%.2x ", buf[i]);
					}
					putchar('\n');
				} else {
					printf("%s\n", buf);
				}
			} while(0 < n);
		}
	}

	return NULL;
}

void signal_handler_function(int signum) {
	stu_uart_data.is_runing = true;
	uart_debug("Quit");
	exit(0);
}

int main(int argc, char **argv)
{
	int ret, fd, baud_rate;
	char buf[RK3308_UART_SEND_BUFF_SIZE_BYTES];
	pthread_t uart_read_thread;

	memset(&stu_uart_data, 0, sizeof(stu_uart_data));

	if(argc < 2) {
		uart_debug("Usage:   %s /dev/ttySx    #0(open uart x data)", argv[0]);
		uart_debug("Example: %s /dev/ttyS1    #1(open uart 1 data)", argv[1]);
		uart_debug("Example: %s /dev/ttyS1 115200   #1(open uart 1 speed 115200 Hex Show Read Data)", argv[0]);
		uart_debug("Example: %s /dev/ttyS1 115200 1 #1(open uart 1 speed 115200 String Show Read Data)", argv[0]);
		return RK3308_UART_ERR;
	}

	fd = rk3308_uart_open(argv[1]);
	if(RK3308_UART_ERR == fd) {
		uart_debug("rk3308_uart_open error = %d", fd);
		return RK3308_UART_ERR;
	}

	uart_debug("argc = %d", argc);

	if (argc >=3 && (baud_rate = atoi(argv[2])) < 0) {
		baud_rate = RK3308_UART_DEFAULT_BAUDRATE;
		uart_debug("baud_rate error = %d", baud_rate);
		return RK3308_UART_ERR;
	}

	uart_debug("baud_rate = %d", baud_rate);

	ret = rk3308_uart_init(fd, baud_rate, 0, 8, 1, 'N');
	if(RK3308_UART_ERR == ret) {
		uart_debug("rk3308_uart_init error = %d",ret);
		return RK3308_UART_ERR;
	}

	if (argc >= 4) {
		stu_uart_data.is_read_data_show_hex = atoi(argv[3]);
		uart_debug("stu_uart_data.is_read_data_show_hex = %d", stu_uart_data.is_read_data_show_hex);
	}

	if (pthread_create(&uart_read_thread, NULL, uart_read_thread_function, &fd) < 0) {
		uart_debug("pthread_create error");
		return RK3308_UART_ERR;
	}

	signal(SIGINT, signal_handler_function);

	for(; !stu_uart_data.is_runing ;) {
		putchar('#');
		fgets(buf, RK3308_UART_SEND_BUFF_SIZE_BYTES, stdin);
		if(RK3308_UART_OK != rk3308_uart_send(fd, buf, strlen(buf))) {
			uart_debug("rk3308_uart_send send data failed");
		}
	}

	pthread_join(uart_read_thread, NULL);
	return rk3308_uart_close(fd);
}

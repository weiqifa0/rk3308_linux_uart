/*
** Copyright (c) 2023 The UART project. All rights reserved.
** Created by crisqifawei 2023
*/

#include "uart.h"

#define DEFAULT_TTY_DEV "/dev/ttyS4"
#define DEFAULT_BAUD_RATE 115200
#define DEFAULT_DATA_BITS 8
#define DEFAULT_STOP_BITS 1
#define DEFAULT_FLOW_CTRL 0
#define DEFAULT_PARITY   'N'
#define DEFAULT_SHOW_HEX  1
#define DEFAULT_RUNNING   1

struct serial_port_data_structure serial_port_data_structure_t = {DEFAULT_TTY_DEV, DEFAULT_BAUD_RATE, DEFAULT_DATA_BITS, DEFAULT_STOP_BITS, DEFAULT_FLOW_CTRL, DEFAULT_PARITY, DEFAULT_SHOW_HEX, DEFAULT_RUNNING};

static void *uart_read_thread_function(void *arg)
{
	int ret, i, n;
	int fd;
	fd_set fds;
	struct timeval time = {5, 500000};
	char buf[RK3308_UART_READ_BUFF_SIZE_BYTES];
	fd = *(int*)arg;

	for(; serial_port_data_structure_t.is_runing ;) {
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
				if (serial_port_data_structure_t.is_read_data_show_hex) {
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
	serial_port_data_structure_t.is_runing = false;
	uart_debug("Quit");
	exit(0);
}

int main(int argc, char **argv)
{
	int ret, fd;
	char buf[RK3308_UART_SEND_BUFF_SIZE_BYTES];
	pthread_t uart_read_thread;

	if (uart_input_detection(&serial_port_data_structure_t, argc, argv) < 0) {
		return 0;
	}

	fd = rk3308_uart_open(serial_port_data_structure_t.tty_dev);
	if(RK3308_UART_ERR == fd) {
		uart_debug("rk3308_uart_open error = %d", fd);
		return RK3308_UART_ERR;
	}

	ret = rk3308_uart_init(fd, serial_port_data_structure_t.baud_rate, serial_port_data_structure_t.flow_ctrl, serial_port_data_structure_t.data_bits, serial_port_data_structure_t.stop_bits, serial_port_data_structure_t.parity);
	if(RK3308_UART_ERR == ret) {
		uart_debug("rk3308_uart_init error = %d",ret);
		return RK3308_UART_ERR;
	}

	if (pthread_create(&uart_read_thread, NULL, uart_read_thread_function, &fd) < 0) {
		uart_debug("pthread_create error");
		return RK3308_UART_ERR;
	}

	signal(SIGINT, signal_handler_function);

	for(; serial_port_data_structure_t.is_runing ;) {
		putchar('#');
		fgets(buf, RK3308_UART_SEND_BUFF_SIZE_BYTES, stdin);
		if(RK3308_UART_OK != rk3308_uart_send(fd, buf, strlen(buf))) {
			uart_debug("rk3308_uart_send send data failed");
		}
	}

	uart_debug("exit()");
	pthread_join(uart_read_thread, NULL);
	return rk3308_uart_close(fd);
}

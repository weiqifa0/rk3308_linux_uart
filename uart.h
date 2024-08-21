/*
** Copyright (c) 2023 The UART project. All rights reserved.
** Created by crisqifawei 2023
*/

#ifndef _UART_RK3308_H
#define _UART_RK3308_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>

#define RK3308_UART_ERR  -1
#define RK3308_UART_OK   0

#define RK3308_UART_READ_BUFF_SIZE_BYTES 128
#define RK3308_UART_SEND_BUFF_SIZE_BYTES 128

#define UART_SOFT_VERSION ("0.0.1.7")

#define uart_debug(format,...) printf("[UART]:%s(), Line: %05d: " format "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__)

struct serial_stu {
	char* tty_dev;
	int baud_rate;
	int data_bits;
	int stop_bits;
	int flow_ctrl;
	char parity;
	bool hex_show;
	bool is_runing;
};

int rk3308_uart_send(int fd, char *buf, int data_len);
int rk3308_uart_init(int fd, int baud_rate, int flow_ctrl, int data_bits, int stop_bits, int parity);
int rk3308_uart_close(int fd);
int rk3308_uart_open(const char* uart_device_name);
int uart_input_detection(struct serial_stu* stu_ptr, int argc, char **argv);

#endif //_UART_RK3308_H

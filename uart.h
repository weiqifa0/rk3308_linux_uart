/*
** Copyright (c) 2023 The Roomix project. All rights reserved.
** Created by crisqifawei 2023
*/

#ifndef __RK3308_H
#define __RK3308_H

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
#define RK3308_UART_DEFAULT_BAUDRATE 9600

#define uart_debug(format,...) printf("[UART]:%s(), Line: %05d: " format "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__)

int rk3308_uart_send(int fd, char *buf, int data_len);
int rk3308_uart_init(int fd, int baud_rate, int flow_ctrl, int data_bits, int stop_bits, int parity);
int rk3308_uart_close(int fd);
int rk3308_uart_open(const char* uart_device_name);

struct uart_stu
{
	bool is_runing;
	bool is_read_data_show_hex;
};

#endif //__RK3308_H

/*
** Copyright (c) 2023 The Roomix project. All rights reserved.
** Created by crisqifawei 2023
*/
#ifdef __cplusplus
extern "C" {
#endif
#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<termios.h>
#include<errno.h>
#include<string.h>
#include<pthread.h>
#include <signal.h>

#define UART_ERR  -1
#define UART_OK   0

#define LEN_BUF_RECV 128
#define LEN_BUF_SEND 128
#define DEFAULT_BAUDRATE 9600

#define UART_DEBUG 0
#define uart_debug(format,...) printf("[UART]:%s(), Line: %05d: " format "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__)
volatile bool g_is_quit = false;

int uart_open(char* dev)
{
	int ret, fd;

	fd = open(dev, O_RDWR|O_NOCTTY|O_NDELAY);
	if (fd < 0) {
		uart_debug("open uart failed");
		return UART_ERR;
	}
	//恢复串口为阻塞状态
	ret = fcntl(fd, F_SETFL, 0);
	if(0 > ret) {
		uart_debug("fcntl failed!");
		return UART_ERR;
	}
	//测试是否为终端设备
	if(0 == isatty(STDIN_FILENO)) {
		uart_debug("standard input is not a terminal device\n");
		return UART_ERR;
	}
	uart_debug("open %s success! fd = %d", dev, fd);
	return fd;
}

void uart_close(int fd)
{
	if (fd >= 0) {
		close(fd);
	}
}

int uart_init(int fd, int baud_rate, int flow_ctrl, int data_bits, int stop_bits, int parity)
{
	int i, status;
	int baud_rate_array[] = {B115200, B19200, B9600, B4800, B2400, B1200, B300};
	int baud_rate_seting_array[] = {115200, 19200, 9600, 4800, 2400, 1200, 300};

	struct termios options;
	uart_debug("baud_rate = %d, flow_ctrl = %d, data_bits = %d, stop_bits =%d, parity =%d",
									baud_rate, flow_ctrl, data_bits, stop_bits, parity);
	if(0 != tcgetattr(fd, &options)) {
		uart_debug("read uart parm error, check device driver");
		return UART_ERR;
	}

	for (i= 0; i < sizeof(baud_rate_array) / sizeof(baud_rate_array[0]); i++) {
		if  (baud_rate == baud_rate_seting_array[i]) {
			cfsetispeed(&options, baud_rate_array[i]);
			cfsetospeed(&options, baud_rate_array[i]);
		}
	}

	//修改控制模式，保证程序不会占用串口
	options.c_cflag |= CLOCAL;
	//修改控制模式，使得能够从串口中读取输入数据
	options.c_cflag |= CREAD;

	//设置数据流控制
	switch(flow_ctrl) {
	case 0://不使用流控制
		options.c_cflag &= ~CRTSCTS;
		break;
	case 1://使用硬件流控制
		options.c_cflag |= CRTSCTS;
		break;
	case 2://使用软件流控制
		options.c_cflag |= IXON | IXOFF | IXANY;
		break;
	}
	//设置数据位
	//屏蔽其他标志位
	options.c_cflag &= ~CSIZE;
	switch (data_bits) {
	case 5:
		options.c_cflag |= CS5;
		break;
	case 6:
		options.c_cflag |= CS6;
		break;
	case 7:
		options.c_cflag |= CS7;
		break;
	case 8:
		options.c_cflag |= CS8;
		break;
	default:
		uart_debug("unsupported data size");
		return UART_ERR;
	}

	//设置校验位
	switch (parity) {
	case 'n':
	case 'N': //无奇偶校验位。
		options.c_cflag &= ~PARENB;
		options.c_iflag &= ~INPCK;
		break;
	case 'o':
	case 'O'://设置为奇校验
		options.c_cflag |= (PARODD | PARENB);
		options.c_iflag |= INPCK;
		break;
	case 'e':
	case 'E'://设置为偶校验
		options.c_cflag |= PARENB;
		options.c_cflag &= ~PARODD;
		options.c_iflag |= INPCK;
		break;
	case 's':
	case 'S': //设置为空格
		options.c_cflag &= ~PARENB;
		options.c_cflag &= ~CSTOPB;
		break;
	default:
		uart_debug("unsupported parity");
		return UART_ERR;
	}
	// 设置停止位
	switch (stop_bits) {
	case 1:
		options.c_cflag &= ~CSTOPB;
		break;
	case 2:
		options.c_cflag |= CSTOPB;
		break;
	default:
		uart_debug("Unsupported stop bits");
		return UART_ERR;
	}

	//修改输出模式，原始数据输出
	options.c_oflag &= ~OPOST;

	options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	options.c_iflag &= ~ICRNL; //禁止将输入的CR转换为NL
	options.c_iflag &= ~(IXON); //清bit位 关闭流控字符
	//设置等待时间和最小接收字符
	options.c_cc[VTIME] = 0; /* 读取一个字符等待1*(1/10)s */
	options.c_cc[VMIN] = 0; /* 读取字符的最少个数为1 */

	//如果发生数据溢出，接收数据，但是不再读取 刷新收到的数据但是不读
	tcflush(fd,TCIFLUSH);

	//激活配置 (将修改后的termios数据设置到串口中）
	if (0 != tcsetattr(fd,TCSANOW,&options)) {
		uart_debug("com set error!");
		return UART_ERR;
	}

	return UART_OK;
}

int uart_send(int fd, char *buf, int data_len)
{
	int len = 0;
	if (data_len == 0) {
		return UART_OK;
	}
	len = write(fd, buf, data_len);
#if UART_DEBUG
	printf("send data:%s\n", buf);
#endif
	if (len != data_len) {
		tcflush(fd, TCOFLUSH);
		return UART_ERR;
	}

	return UART_OK;
}

static void *serial_port_receiving_thread(void *arg)
{
	int ret, i, n;
	int fd;
	fd_set fds;
	struct timeval time = {5, 500000};
	char buf[LEN_BUF_RECV];
	fd = *(int*)arg;
	for(;!g_is_quit;) {
		FD_ZERO(&fds);
		FD_SET(fd, &fds);
		ret = select(fd + 1, &fds, NULL, NULL, &time);
		if (ret < 0) {
			uart_debug("select failed ret = %d fd = %d", ret, fd);
			continue;
		} else if (ret == 0) {
#if UART_DEBUG
			uart_debug("select timeout ret = %d fd = %d", ret, fd);
#endif
		}
		if (FD_ISSET(fd, &fds)) {
			do {
				n = read(fd, buf, LEN_BUF_RECV);
#if UART_DEBUG
				uart_debug("%s", buf);
#endif
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
	char buf[LEN_BUF_SEND];
	pthread_t serial_receive_t;

	if(argc < 2) {
		uart_debug("Usage:   %s /dev/ttySx    #0(open uart x data)", argv[0]);
		uart_debug("Example: %s /dev/ttyS1    #1(open uart 1 data)", argv[1]);
		uart_debug("Example: %s %s /dev/ttyS1 115200   #1(open uart 1 speed 115200)", argv[1], argv[1]);
		return UART_ERR;
	}

	fd = uart_open(argv[1]);
	if(UART_ERR == fd) {
		uart_debug("uart_open error = %d", fd);
		return UART_ERR;
	}

	if (argc == 2) {
		ret = uart_init(fd, DEFAULT_BAUDRATE, 0, 8, 1, 'N');
		if(UART_ERR == ret) {
			uart_debug("uart_init error = %d",ret);
			return UART_ERR;
		}
	} else {
		if ((baud_rate = atoi(argv[2])) < 0) {
			baud_rate = DEFAULT_BAUDRATE;
		}
		ret = uart_init(fd, baud_rate, 0, 8, 1, 'N');
		if(UART_ERR == ret) {
			uart_debug("uart_init error = %d",ret);
			return UART_ERR;
		}
	}

	if (pthread_create(&serial_receive_t, NULL, serial_port_receiving_thread, &fd) < 0) {
		uart_debug("pthread_create error");
		return UART_ERR;
	}

	signal(SIGINT, sighandler);

	for(;!g_is_quit;) {
		putchar('#');
		fgets(buf, LEN_BUF_SEND, stdin);
		if(UART_OK != uart_send(fd, buf, strlen(buf))) {
			uart_debug("time send data failed");
		}
	}

	pthread_join(serial_receive_t, NULL);
	uart_close(fd);
}
#ifdef __cplusplus
}
#endif

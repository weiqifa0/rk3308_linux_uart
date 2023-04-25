/*
** Copyright (c) 2023 The tinylinux project. All rights reserved.
** Created by crisqifawei 2023
*/
#ifdef __cplusplus
extern "C" {
#endif
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<termios.h>
#include<errno.h>
#include<string.h>
#include<pthread.h>

#define UART_ERR  -1
#define UART_OK   0

#define LEN_BUF_RECV 128
#define LEN_BUF_SEND 128

int uart_open(char* dev)
{
	int ret;
	int fd;

	fd = open(dev, O_RDWR|O_NOCTTY|O_NDELAY);
	if (fd < 0) {
		printf("open uart failed\n");
		return UART_ERR;
	}
	//恢复串口为阻塞状态
	ret = fcntl(fd, F_SETFL, 0);
	if(0 > ret) {
		printf("fcntl failed!\n");
		return UART_ERR;
	}
	//测试是否为终端设备
	if(0 == isatty(STDIN_FILENO)) {
		printf("standard input is not a terminal device\n");
		return UART_ERR;
	}
	printf("open %s success! fd = %d\n", dev, fd);
	return fd;
}

void uart_close(int fd)
{
	close(fd);
}

int uart_init(int fd, int speed, int flow_ctrl, int data_bits, int stop_bits, int parity)
{
	int i, status;
	int speed_arr[] = {B115200, B19200, B9600, B4800, B2400, B1200, B300};
	int name_arr[] =  {115200, 19200, 9600, 4800, 2400, 1200, 300};

	struct termios options;
	if(0 != tcgetattr(fd, &options)) {
		printf("SetupSerial 1\n");
		return UART_ERR;
	}

	for ( i= 0; i < sizeof(speed_arr) / sizeof(speed_arr[0]); i++) {
		if  (speed == name_arr[i]) {
			cfsetispeed(&options, speed_arr[i]);
			cfsetospeed(&options, speed_arr[i]);
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
		printf("Unsupported data size\n");
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
		printf("Unsupported parity\n");
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
		printf("Unsupported stop bits\n");
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
		printf("com set error!\n");
		return UART_ERR;
	}

	return UART_OK;
}

int uart_send(int fd, char *buf, int data_len)
{
	int len = 0;
	len = write(fd, buf, data_len);
	printf("send data:%s\n", buf);
	if (len != data_len) {
		tcflush(fd, TCOFLUSH);
		return UART_ERR;
	}

	return UART_OK;
}

static void *uart_receive_thread(void *arg)
{
	int ret, i, n;
	int serial_fd;
	fd_set readfds;
	char buf[LEN_BUF_RECV];
	serial_fd = *(int*)arg;
	for (;;) {
		FD_ZERO(&readfds);
		FD_SET(serial_fd, &readfds);
		ret = select(serial_fd + 1, &readfds, NULL, NULL, NULL);

		if(ret <= 0) {
			printf("select failed %d\n", ret);
			continue;
		}
		if(FD_ISSET(serial_fd, &readfds) <= 0){
			printf("FD_ISSET failed\n");
			continue;
		}
		do {
			n = read(serial_fd, buf, LEN_BUF_RECV);
			for (i = 0; i < n; i++) {
				printf("%.2x ", buf[i]);
			}
			putchar('\n');
		} while(0 < n);
	}

	return NULL;
}

int main(int argc, char **argv)
{
	int ret, i;
	int serial_fd;
	char buf[LEN_BUF_SEND];
	pthread_t serial_receive_t;

	if(argc != 2) {
		printf("Usage:   %s /dev/ttySx    #0(open uart x data)\n",argv[0]);
		printf("Example: %s /dev/ttyS1 1  #1(open uart 1 data)\n",argv[1]);
		printf("Input Error\n");
		return UART_ERR;
	}

	serial_fd = uart_open(argv[1]);
	if(UART_ERR == serial_fd) {
		printf("uart_open error = %d\n", serial_fd);
		return UART_ERR;
	}

	ret = uart_init(serial_fd, 9600, 0, 8, 1, 'N');
	if(UART_ERR == ret) {
		printf("ret init = %d\n",ret);
		return UART_ERR;
	}

	if (pthread_create(&serial_receive_t, NULL, uart_receive_thread, &serial_fd) < 0) {
		printf("pthread_create error\n");
		return UART_ERR;
	}

	for(;;) {
		fgets(buf, LEN_BUF_SEND, stdin);
		if(UART_OK == uart_send(serial_fd, buf, strlen(buf))) {
			printf("%d time send data successful\n",i++);
		} else {
			printf("send data failed!\n");
		}
	}

	uart_close(serial_fd);
}
#ifdef __cplusplus
}
#endif

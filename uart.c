/*
** Copyright (c) 2023 The UART project. All rights reserved.
** Created by crisqifawei 2023
*/

#include "uart.h"

int rk3308_uart_open(const char* uart_device_name)
{
	int ret, fd;

	fd = open(uart_device_name, O_RDWR|O_NOCTTY|O_NDELAY);
	if (fd < 0) {
		uart_debug("open uart failed");
		return RK3308_UART_ERR;
	}

	//恢复串口为阻塞状态
	ret = fcntl(fd, F_SETFL, 0);
	if(0 > ret) {
		uart_debug("fcntl failed!");
		return RK3308_UART_ERR;
	}

	//测试是否为终端设备
	if(0 == isatty(STDIN_FILENO)) {
		uart_debug("standard input is not a terminal device\n");
		return RK3308_UART_ERR;
	}
	uart_debug("open %s success! fd = %d", uart_device_name, fd);
	return fd;
}

int rk3308_uart_close(int fd)
{
	if (fd >= 0) {
		close(fd);
	}
	return 0;
}

int rk3308_uart_init(int fd, int baud_rate, int flow_ctrl, int data_bits, int stop_bits, int parity)
{
	int i;
	int baud_rate_array[] = {B115200, B19200, B9600, B4800, B2400, B1200, B300};
	int baud_rate_seting_array[] = {115200, 19200, 9600, 4800, 2400, 1200, 300};

	struct termios options;
	uart_debug("baud_rate = %d, flow_ctrl = %d, data_bits = %d, stop_bits =%d, parity =%c",
									baud_rate, flow_ctrl, data_bits, stop_bits, parity);
	if(0 != tcgetattr(fd, &options)) {
		uart_debug("read uart parm error, check device driver");
		return RK3308_UART_ERR;
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
		return RK3308_UART_ERR;
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
		return RK3308_UART_ERR;
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
		return RK3308_UART_ERR;
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
		return RK3308_UART_ERR;
	}

	return RK3308_UART_OK;
}

int rk3308_uart_send(int fd, char *buf, int data_len)
{
	int len = 0;
	if (data_len == 0) {
		return RK3308_UART_OK;
	}
	len = write(fd, buf, data_len);
	if (len != data_len) {
		tcflush(fd, TCOFLUSH);
		return RK3308_UART_ERR;
	}

	return RK3308_UART_OK;
}

int uart_input_detection(struct serial_port_data_structure* stu_ptr, int argc, char **argv)
{
	int opt = 0;
	opterr = 0 ;
	int ret = 0;

	while ((opt = getopt (argc, argv, "T:t:B:b:D:d:S:s:F:f:P:p:HhX:x:Vv")) != EOF)
	switch (opt)
	{
		case 'T':
		case 't':
			stu_ptr->tty_dev = optarg;
			uart_debug("tty_dev = %s", optarg);
			break;
		case 'B':
		case 'b':
			stu_ptr->baud_rate = atoi(optarg);
			uart_debug("baud_rate = %d", stu_ptr->baud_rate);
			break;
		case 'D':
		case 'd':
			stu_ptr->data_bits = atoi(optarg);
			uart_debug("data_bits = %d", stu_ptr->data_bits);
			break;
		case 'S':
		case 's':
			stu_ptr->stop_bits = atoi(optarg);
			uart_debug("stop_bits = %d", stu_ptr->stop_bits);
			break;
		case 'F':
		case 'f':
			stu_ptr->flow_ctrl = atoi(optarg);
			uart_debug("flow_ctrl = %d", stu_ptr->flow_ctrl);
			break;
		case 'P':
		case 'p':
			stu_ptr->parity = atoi(optarg);
			uart_debug("parity = %c", stu_ptr->parity);
			break;
		case 'x':
		case 'X':
			stu_ptr->is_read_data_show_hex = atoi(optarg);
			uart_debug("is_read_data_show_hex = %d", stu_ptr->is_read_data_show_hex);
			break;
		case 'H':
		case 'h':
			uart_debug("Example: %s Version:%s", argv[0], UART_SOFT_VERSION);
			uart_debug("Example: %s -T /dev/ttyS1 -B 115200", argv[0]);
			uart_debug("Example Not to Show Hex: %s -T /dev/ttyS1 -B 115200 -x 0", argv[0]);
			ret = -1;
			break;
		default:
			uart_debug("other option is wrong");
	}
	return ret;
}
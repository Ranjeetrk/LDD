#include <stdio.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>

#define WR_VALUE _IOW('w','r',int32_t *)
#define RD_VALUE _IOR('r','d',int32_t*)

int main()
{

	int fd; 
	int32_t rd_value, wr_value; 
	printf("*********************************\n");
	fd = open("/dev/ioctl_device", O_RDWR);
	if( fd < 0) {
		printf("device open failed fd: %d \n", fd);
		return 0;
	}
	printf(" enter the number to write on ioctl_driver \n");
	scanf("%d", &wr_value);

	ioctl(fd, WR_VALUE, (int32_t *)&wr_value);

	printf("Reading Value from Driver\n");
        ioctl(fd, RD_VALUE, (int32_t*) &rd_value);
        printf("Value is %d\n", rd_value);

        printf("Closing Driver\n");
        close(fd);

	return 0;
}


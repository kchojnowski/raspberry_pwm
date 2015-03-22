#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
int main(int argc, char** argv) {

	if(argc != 4) {
		printf("Usage: gpio_pwm pin_number frequency[Hz] duty_cycle[%]\n");
		return 0;
	}

	char* gpio_base_path = "/sys/class/gpio/gpio";
	char* gpio_value_file = "value";

	char gpio_value_file_path[64];
	
	sprintf(gpio_value_file_path, "%s%s/%s",gpio_base_path, argv[1], gpio_value_file);

	int fd;
	
	if((fd = open(gpio_value_file_path, O_RDWR)) < 0) {
		printf("Error opening file for write: %s\n",gpio_value_file_path);
		return 1;
	}
	

	int sampling = atoi(argv[2]);
	int duty = atoi(argv[3]);

	long period = 1000000000/sampling;
	long high_time = period/100*duty;
	long low_time = period - high_time;
    
	struct timespec tspec_high;
    struct timespec tspec_low;

	char state_high = '1';
	char state_low = '0';

	tspec_high.tv_sec = high_time/1000000000;
	tspec_high.tv_nsec = high_time%1000000000;
	
	tspec_low.tv_sec = low_time/1000000000;
	tspec_low.tv_nsec = low_time%1000000000;

	while(1) {
		if(write(fd, &state_high, sizeof(char)) <= 0) {
			printf("Error writing file: %s\n",gpio_value_file_path);
			return 1;
		}

		nanosleep(&tspec_high, NULL);

		if(write(fd, &state_low, sizeof(char)) <= 0) {
			printf("Error writing file: %s\n",gpio_value_file_path);
			return 1;
		}
		
		nanosleep(&tspec_low, NULL);


	}
	
	if(write(fd, argv[2], sizeof(argv[2])) <= 0) {
		printf("Error writing file: %s\n",gpio_value_file_path);
		return 1;
	}

	if(close(fd)<0) {
		printf("Error closing file: %s\n",gpio_value_file_path);
		return 1;
	}

	return 0;
}

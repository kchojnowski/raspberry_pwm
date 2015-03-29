#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <pthread.h>

unsigned char duty = 0;
unsigned char finish_flag = 0;

struct pwm_thread_args {
	int sampling;
	int pin_fd;
};

void* pwm_thread(void *arg) {

	struct pwm_thread_args *pwm_args = (struct pwm_thread_args*)arg;

	unsigned char duty_local = duty;

	long period = 1000000000/pwm_args->sampling;
    long high_time = 0;
	long low_time = period;

	struct timespec tspec_high;
	struct timespec tspec_low;

	char state_high = '1';
	char state_low = '0';

	tspec_high.tv_sec = high_time/1000000000;
	tspec_high.tv_nsec = high_time%1000000000;

	tspec_low.tv_sec = low_time/1000000000;
	tspec_low.tv_nsec = low_time%1000000000;

	while(!finish_flag) {
		while(duty == 0 && !finish_flag) {
			duty_local = duty;
			low_time = period;
			tspec_low.tv_sec = low_time/1000000000;
            tspec_low.tv_nsec = low_time%1000000000;
			nanosleep(&tspec_low, NULL);
		}
		

		if(duty_local != duty) {
			duty_local = duty;

			high_time = period/100*duty_local;
            low_time = period - high_time;

            tspec_high.tv_sec = high_time/1000000000;
            tspec_high.tv_nsec = high_time%1000000000;

		    tspec_low.tv_sec = low_time/1000000000;
			tspec_low.tv_nsec = low_time%1000000000;
		}

		if(write(pwm_args->pin_fd, &state_high, sizeof(char)) <= 0) {
			printf("Error writing file");
			return NULL;
        }
	    
		nanosleep(&tspec_high, NULL);
	    
		if(write(pwm_args->pin_fd, &state_low, sizeof(char)) <= 0) {
		    printf("Error writing file");
		    return NULL;
		}

		nanosleep(&tspec_low, NULL);
	}

	if(write(pwm_args->pin_fd, &state_low, sizeof(char)) <= 0) {
		printf("Error writing file");
		return NULL;
	}

	return NULL;
}

int main(int argc, char** argv) {

	if(argc != 4) {
		printf("Usage: gpio_pwm pin_number frequency[Hz] socket_name\n");
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
	
	//configure and connect to socket
	struct sockaddr_un address;
	int  socket_fd, nbytes;
	unsigned char buffer[256];

	socket_fd = socket(PF_UNIX, SOCK_STREAM, 0);
    if(socket_fd < 0) {
		printf("socket() failed\n");
		return 1;
	}
	
	memset(&address, 0, sizeof(struct sockaddr_un));

    address.sun_family = PF_UNIX;
	snprintf(address.sun_path, 108, argv[3]);

    if(connect(socket_fd, (struct sockaddr *) &address, sizeof(struct sockaddr_un)) != 0) {
	    printf("connect() failed\n");
	    return 1;
	}
	
	struct pwm_thread_args pwm_args;
	pwm_args.sampling = atoi(argv[2]);
	pwm_args.pin_fd = fd;

	pthread_t tid;

	pthread_create(&tid, NULL, &pwm_thread, &pwm_args);

	while(1) {
	
		//read the duty cycle from socket
		nbytes = read(socket_fd, buffer, 256);
		if(nbytes > 0) {
			if(buffer[0] == 255) {
				finish_flag = 1;
				break;
			} else {
				duty = buffer[0];
			}
		}
	}
	
	pthread_join (tid, NULL);

	if(close(fd)<0) {
		printf("Error closing file: %s\n",gpio_value_file_path);
		return 1;
	}

	close(socket_fd);


	return 0;
}

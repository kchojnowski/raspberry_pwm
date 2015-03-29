#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
int connection_handler(int connection_fd)
{
 int nbytes;
 char buffer[256];

 nbytes = read(connection_fd, buffer, 256);
 buffer[nbytes] = 0;

 printf("MESSAGE FROM CLIENT: %s\n", buffer);
 nbytes = snprintf(buffer, 256, "hello from the server");
 write(connection_fd, buffer, nbytes);
 
 close(connection_fd);
 return 0;
}

int main(int argc, char** argv)
{
 struct sockaddr_un address;
 int socket_fd, connection_fd;
 socklen_t address_length = sizeof(struct sockaddr_un);;
 unsigned char buffer[256]; 

 socket_fd = socket(PF_UNIX, SOCK_STREAM, 0);
 if(socket_fd < 0)
 {
  printf("socket() failed\n");
  return 1;
 } 

 unlink(argv[1]);

 /* start with a clean address structure */
 memset(&address, 0, sizeof(struct sockaddr_un));

 address.sun_family = AF_UNIX;
 snprintf(address.sun_path, 255, argv[1]);

 if(bind(socket_fd, (struct sockaddr *) &address, sizeof(struct sockaddr_un)) != 0)
 {
  printf("bind() failed\n");
  return 1;
 }

 if(listen(socket_fd, 5) != 0)
 {
  printf("listen() failed\n");
  return 1;
 }

 connection_fd = accept(socket_fd, (struct sockaddr *) &address, &address_length);
 if(connection_fd < 0)
 {
  printf("accept() failed %d\n", errno);

  return 1;
 }


 while(1)
 {
	scanf("%hhu", buffer);
	write(connection_fd, buffer, 1);
 }

  /* still inside server process */
 close(connection_fd);
 close(socket_fd);
 unlink(argv[1]);
 return 0;
}

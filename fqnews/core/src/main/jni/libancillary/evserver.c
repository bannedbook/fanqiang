// based on http://www.thomasstover.com/uds.html

#include <assert.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/eventfd.h>
#include <errno.h>
#include "ancillary.h"

// use an 'abstract  socket  address', whatever that is
// at least not in file system
// X will be replaced by '\0' post-snprintf
char *name = "Xeventfd_socket";

int connection_handler(int connection_fd)
{
    int i;
    int efd;

    efd = eventfd(0,0);
    assert (efd != -1);

    if (ancil_send_fd(connection_fd, efd)) {
	perror("ancil_send_fd");
	exit(1);
    } else {
	printf("Sent evfd %d\n", efd);
    }

    uint64_t u = getpid();
    ssize_t s;

    for (i = 0; i < 10; i++) {
	s = write(efd, &u, sizeof(uint64_t));
	if (s != sizeof(uint64_t))
	    perror("write");
	else
	    printf("Sent %llu\n", u);
	u++;
	sleep(1);
    }
    return 0;
}

int main(void)
{
    struct sockaddr_un address;
    int socket_fd, connection_fd;
    socklen_t address_length  = sizeof(address);
    pid_t child;

    socket_fd = socket(PF_UNIX, SOCK_STREAM, 0);
    if(socket_fd < 0) {
	printf("socket() failed\n");
	return 1;
    }
    int enable = 1;
    setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));

    memset(&address, 0, sizeof(struct sockaddr_un));
    address.sun_family = AF_UNIX;
    snprintf(address.sun_path,sizeof(address.sun_path), name);
    address.sun_path[0] = '\0';

    if (bind(socket_fd, (struct sockaddr *) &address,
	    sizeof(struct sockaddr_un)) != 0) {
	fprintf(stderr,"bind() failed: %s\n", strerror(errno));
	return 1;
    }

    if (listen(socket_fd, 5) != 0) {
	fprintf(stderr,"listen() failed: %s\n", strerror(errno));
	return 1;
    }

    while ((connection_fd = accept(socket_fd,
				   (struct sockaddr *) &address,
				   &address_length)) > -1) {
	child = fork();
	if (child == 0)  {
	    return connection_handler(connection_fd);
	}
	close(connection_fd);
    }

    fprintf(stderr,"accept() failed: %s\n", strerror(errno));
    close(socket_fd);
    return 0;
}

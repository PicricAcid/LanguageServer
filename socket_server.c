#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/select.h>

#define UNIXDOMAIN_PATH "/tmp/server.sock"
#define RECVBUF_SIZE 2048

int main(int argc, char *argv[]) {
    int fd = 0;
    int clifd = 0;
    fd_set rfds;
    struct timeval tv;
    int retval;

    struct sockaddr_un cliaddr, srvaddr;

    fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
	fprintf(stderr, "socket error\n");
	exit(-1);
    }

    remove(UNIXDOMAIN_PATH);
    memset(&srvaddr, 0, sizeof(struct sockaddr_un));
    srvaddr.sun_family = AF_UNIX;
    strcpy(srvaddr.sun_path, UNIXDOMAIN_PATH);
    if (bind(fd, (struct sockaddr*)&srvaddr, sizeof(struct sockaddr_un)) < 0) {
	fprintf(stderr, "bind error\n");
	exit(-1);
    }

    if (listen(fd, 5) < 0) {
	fprintf(stderr, "listen error\n");
	exit(-1);
    }

    while (1) {
	FD_ZERO(&rfds);
	FD_SET(fd, &rfds);
	tv.tv_sec = 5;
	tv.tv_usec = 0;

	retval = select(fd + 1, &rfds, NULL, NULL, &tv);

	if (retval < 0) {
	    fprintf(stderr, "select error\n");
	    exit(-1);
	} else if (retval > 0) {
	    char recvbuf[RECVBUF_SIZE] = "";
	    memset(&cliaddr, 0, sizeof(struct sockaddr_un));
	    socklen_t addrlen = sizeof(struct sockaddr_un);
	    clifd = accept(fd, (struct sockaddr*)&cliaddr, &addrlen);
	    if (clifd < 0) {
		fprintf(stderr, "accept error\n");
		exit(-1);
	    }

	    int len = recv(clifd, recvbuf, sizeof(recvbuf), 0);
	    recvbuf[len] = 0;
	    if (strcmp(recvbuf, "exit") == 0) {
		fprintf(stdout, "exit\n");
		break;
	    }
	    fprintf(stdout, "%s\n", recvbuf);
	    char sendbuf[RECVBUF_SIZE] = "Hello World from C";
	    int send_len = send(clifd, sendbuf, sizeof(char)*strlen(sendbuf), 0);
	    if (send_len == -1) {
		fprintf(stderr, "send error\n");
		exit(-1);
	    }
	} else {
	    fprintf(stdout, "timeout\n");
	}
    }

    close(clifd);
    close(fd);

    return 0;
}

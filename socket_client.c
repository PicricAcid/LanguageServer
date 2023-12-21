#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#define UNIXDOMAIN_PATH "/tmp/language_server.sock"
#define MESSAGE_LEN 2048

int main(int argc, char *argv[]) {
    struct sockaddr_un addr;
    int srvfd;

    srvfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (srvfd < 0) {
	fprintf(stderr, "socket error\n");
	exit(-1);
    }

    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, UNIXDOMAIN_PATH);

    if (connect(srvfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un)) < 0) {
	fprintf(stderr, "connect error\n");
	exit(-1);
    }
 
    while(1) {
	char message[MESSAGE_LEN] = "";
	char recv_message[MESSAGE_LEN] = "";
	printf("send message\n");
	scanf("%s", message);

	if (send(srvfd, message, MESSAGE_LEN, 0) < 0) {
	    fprintf(stderr, "write error\n");
	    exit(-1);
	}

	if (strcmp(message, "exit") == 0) {
	    break;
	}
	if (recv(srvfd, recv_message, MESSAGE_LEN, 0) < 0) {
	    fprintf(stderr, "recv error");
	    exit(-1);
	} else {
	    printf("%s\n", recv_message);
	}
    }

    close(srvfd);

    return 0;
}

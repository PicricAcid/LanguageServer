#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/un.h>
#include "main.h"
#include "language_server.h"

#define UNIXDOMAIN_PATH "/tmp/language_server.sock"
#define RECVBUF_SIZE 2048
#define BACKLOGSIZE 5
#define SERVER_LOGFILE_NAME "./server_log.log"

static int init_server(void);
static void connect_server(int listen_fd);
static int get_message(int client_fd, char *buf, FILE *log_file);
static int send_message(int client_fd, char *message, FILE *log_file);
static int wait_connect(int listen_fd);
static int receive_message(int client_fd, char *json_text, FILE *log_file, LS_openfile *current_file);

static int shutdown_flag;

static int init_server(void) {
    int fd = 0;
    struct sockaddr_un server_addr;

    fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
	fprintf(stderr, "init_server: socket error\n");
	exit(-1);
    }

    remove(UNIXDOMAIN_PATH);
    memset(&server_addr, 0, sizeof(struct sockaddr_un));
    server_addr.sun_family = AF_UNIX;
    strcpy(server_addr.sun_path, UNIXDOMAIN_PATH);
    if (bind(fd, (struct sockaddr*)&server_addr, sizeof(struct sockaddr_un)) < 0) {
	fprintf(stderr, "init_server: bind error\n");
	exit(-1);
    }

    return fd;
}

static void connect_server(int listen_fd) {
    if (listen(listen_fd, BACKLOGSIZE) < 0) {
	fprintf(stderr, "connect_server: listen error\n");
	exit(-1);
    }

    return;
}

static int wait_connect(int listen_fd) {
    int client_fd = 0;
    struct sockaddr_un client_addr;

    memset(&client_addr, 0, sizeof(struct sockaddr_un));
    socklen_t addrlen = sizeof(struct sockaddr_un);
    printf("waiting connect...\n");
    client_fd = accept(listen_fd, (struct sockaddr*)&client_addr, &addrlen);
    if (client_fd < 0) {
	fprintf(stderr, "wait_connect: accept error\n");
	exit(-1);
    }
    printf("connect\n");

    return client_fd;
}

static int get_message(int client_fd, char *buf, FILE *log_file) {
    int len = recv(client_fd, buf, sizeof(char)*RECVBUF_SIZE, 0);
    buf[len] = 0;
    fprintf(log_file, " > %s\n", buf);

    return len;
}

static int send_message(int client_fd, char *message, FILE *log_file) {
    int result = 0;
    int len = strlen(message);

    result = send(client_fd, message, sizeof(char)*len, 0);
    fprintf(log_file, "S> %s\n", message);

    return result;
}

static LS_Content *execute_request(LS_Content *content) {
    LS_Content *res_content = NULL;

    switch (content->method) {
	case Initialize:
	    res_content = make_initialize_response(content);
	    break;
	case Shutdown:
	    res_content = make_shutdown_response(content);
	    shutdown_flag = 1;
	    break;
	case Initialized:
	case TextDocument_didOpen:
	case TextDocument_didClose:
	case TextDocument_didChange:
	case Exit:
	case LSMethodCount:
	default:
	    break;
    }

    return res_content;
} 

static void execute_td_open(LS_Content *content, LS_openfile *current_file) {    
    LS_textdocument_item *td_item = &content->params.object->didopen_td_params.textDocumentItem;
    current_file->is_open = 1;
    strcpy(current_file->uri, td_item->uri);
    strcpy(current_file->languageID, td_item->language_id);
    strcpy(current_file->text, td_item->text);

    return;
}

static void execute_td_close(LS_Content *content, LS_openfile *current_file) {
    if (current_file->is_open == 0) {
	fprintf(stderr, "textDocument/didClose: not open file\n");
    } else {
	LS_textdocument_identifier *td_identifier = &content->params.object->didclose_td_params.textDocument; 
	if (strcmp(current_file->uri, td_identifier->uri) == 0) {
	    current_file->is_open = 0;
	}
    }

    return;
}

static LS_Content *execute_td_change(LS_Content *content, LS_openfile *current_file) {
    LS_Content *notif_message = NULL;
    if (current_file->is_open == 0) {
	fprintf(stderr, "textDocument/didChange: not open file\n");
    } else {
	LS_textdocument_identifier *td_identifier = &content->params.object->didchange_td_params.versioned_td_identifier;
	LS_content_changes *td_changes = &content->params.object->didchange_td_params.td_content_change_event;
	if (strcmp(current_file->uri, td_identifier->uri) == 0) {
	    if (strcmp(td_changes->text, "print HelloWorld") != 0) {
		LS_Diagnostics_type diatype = INCORRECT_SENTENCE;
		notif_message = make_diagnostics_notification(content, diatype);
	    }
	}
    }
    return notif_message;
}

static int receive_message(int client_fd, char *json_text, FILE *log_file, LS_openfile *current_file) {
    int retval = 0;
    LS_Content *content = init_content();

    Json_object *object = json_parse(json_text);
    /* print_object(object); */

    interpreter(object, content);
    if (content->content_type == RequestMessage) {
	char res_message[RECVBUF_SIZE];
	LS_Content *res_content = execute_request(content);
	make_json_message(res_content, res_message);
	send_message(client_fd, res_message, log_file);
    } else if (content->content_type == NotificationMessage) {
	LS_Content *notif_content = NULL;
	switch (content->method) {
	    case Initialized:
		break;
	    case TextDocument_didOpen:
		execute_td_open(content, current_file);
		break;
	    case TextDocument_didClose:
		execute_td_close(content, current_file);
		break;
	    case TextDocument_didChange:
		notif_content = execute_td_change(content, current_file);
		if (notif_content != NULL) {
		    char notif_message[RECVBUF_SIZE];
		    make_json_message(notif_content, notif_message);
		    printf("checkpoint TextDocument_idiChange: before notif_message %s\n", notif_message);
		    send_message(client_fd, notif_message, log_file);
		    printf("checkpoint TextDocument_didChange: after notif_message %s\n", notif_message);
		}
		break;
	    case Exit:
		retval = 1;
		break;
	    case Initialize:
	    case Shutdown:
	    case LSMethodCount:
	    default:
		break;
	}
    }
    free_object(object);
    free_content(content);

    return retval;
}

void language_server(void) {
    int listen_fd;
    int client_fd;
    int retval = 0;

    int retselect;
    fd_set rfds;
    
    int file_exist = 0;
    LS_openfile current_file;
    current_file.is_open = 0;

    FILE *log_file = fopen(SERVER_LOGFILE_NAME, "r");
    if (log_file != NULL) {
	file_exist = 1;
    }
    fclose(log_file);

    if (file_exist == 1) {
	remove(SERVER_LOGFILE_NAME);
    }

    log_file = fopen(SERVER_LOGFILE_NAME, "a");

    listen_fd = init_server();
    connect_server(listen_fd);
    
    client_fd = wait_connect(listen_fd);
    
    while(1) {
	FD_ZERO(&rfds);
	FD_SET(client_fd, &rfds);

	retselect = select(client_fd+1, &rfds, NULL, NULL, NULL);
	if (retselect < 0) {
	    fprintf(stderr, "select error\n");
	} else if (retselect > 0) {
	    char recvbuf[RECVBUF_SIZE] = "";
	    get_message(client_fd, recvbuf, log_file);
	    retval = receive_message(client_fd, recvbuf, log_file, &current_file);
	    if (retval == 1) {
		if (shutdown_flag == 1) {
		    break;
		} else {
		    fprintf(stderr, "Error: Client exit\n");
		    exit(-1);
		}	
	    }
	} else {
	}
    }

    close(client_fd);
    close(listen_fd);
    fclose(log_file);

    return;
}

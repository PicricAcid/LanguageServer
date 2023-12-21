#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "main.h"

#define STRING_BUFFER_LEN 2048
#define PARAM_BUF_LEN 512
#define RESULT_BUF_LEN 512
#define METHOD_BUF_LEN 64
#define RES_CONTENT_LEN 1024

static void json_text_init(Json_text *jsontext, const char *text) {
    jsontext->pos = 0;
    jsontext->len = strlen(text);
    jsontext->text = text;
    
    return;
}

static char *parse_string(Json_text *jsontext) {
    char string_buffer[STRING_BUFFER_LEN];
    int i;

    for (i = 0; i<STRING_BUFFER_LEN; i++) {
	jsontext->pos++;
	char key_ch = jsontext->text[jsontext->pos];
	if (key_ch == '\"') {
	    jsontext->pos++;
	    break;
	} else {
	    string_buffer[i] = key_ch;
	}
    }

    int len = i;
    string_buffer[len] = '\0';
    
    return string_buffer;
}

static int parse_number(Json_text *jsontext) {
    char buffer[STRING_BUFFER_LEN];
    int i;

    for (i = 0; i<STRING_BUFFER_LEN; i++) {
	char ch = jsontext->text[jsontext->pos];
	if ((ch == ',') || (ch == '}')) {
	    break;
	} else {
	    buffer[i] = ch;
	    jsontext->pos++;
	}
    }

    int return_value = atoi(buffer);
    if (return_value == 0) {
	if (strcmp(buffer, "0") != 0) {
	    return_value = -1;
	}
    }

    return return_value;
}

static Json_object *init_object(void) {
    Json_object *object = (Json_object*)malloc(sizeof(Json_object));

    object->next = NULL;
    
    return object;
}

static Json_object *parse_object(Json_text *jsontext) {
    int i;

    if (jsontext->text[jsontext->pos] != '{') {
	fprintf(stderr, "parse error '{'\n");
	exit(-1);
    }

    jsontext->pos++;

    Json_object *head = init_object();

    while(1) {
	Json_object *object = head;
	for (; object->next != NULL; object = object->next);
	
	if (jsontext->text[jsontext->pos] == '}') {
	    jsontext->pos++;
	    break;
	}
	
	char *key = parse_string(jsontext);
	if (strlen(key) <= JSON_KEY_LEN) {
	    strcpy(object->key_name, key);
	} else {
	    fprintf(stderr, "key parse error\n");
	    exit(-1);
	}
	if (jsontext->text[jsontext->pos] != ':') {
	    printf("%s\n", object->key_name);
	    fprintf(stderr, "parse error ':'\n");
	    exit(-1);
	}

	jsontext->pos++;
	
	char ch = jsontext->text[jsontext->pos];
	if (ch == '\"') {
	    char *str = parse_string(jsontext);
	    if (strlen(str) <= STRING_LEN) {
		strcpy(object->value.string_value, str);
	    } else {
		fprintf(stderr, "string value parse error\n");
	    }
	    object->type = JSONValueTypeString;
	} else if (ch == '{') {
	    object->type = JSONValueTypeObject;
	    object->value.object_value = parse_object(jsontext);
	} else {
	    object->value.int_value = parse_number(jsontext);
	    object->type = JSONValueTypeInt;
	}
	
	ch = jsontext->text[jsontext->pos];
	if (ch == '}') {
	    jsontext->pos++;
	    break;
	} else if (ch == ',') {
	    Json_object *next_object = init_object();
	    object->next = next_object;
	    jsontext->pos++;
	} else {
	    fprintf(stderr, "parse error %c\n", jsontext->text[jsontext->pos]);
	}
    }

    return head;
}

void print_object(Json_object *head) {
    Json_object *p = head;
    while (p != NULL) {
	printf("key: %s\n", p->key_name);
	switch (p->type) {
	    case JSONValueTypeInt:
		printf("value: %d\n", p->value.int_value);
		break;
	    case JSONValueTypeString:
		printf("value: %s\n", p->value.string_value);
		break;
	    case JSONValueTypeObject:
		print_object(p->value.object_value);
		break;
	    case JSONValueTypeCount:
	    default:
		printf("value print error\n");
		break;
	}
	p = p->next;
    }

    return;
}

void free_object(Json_object *head) {
    if (head->next != NULL) {
	free_object(head->next);
    }
    free(head);
    
    return;
}

Json_object *json_parse(const char* text) {
    
    Json_text jsontext;
    json_text_init(&jsontext, text);
    
    Json_object *object = NULL;

    while (jsontext.pos <= jsontext.len) {
	char ch = jsontext.text[jsontext.pos];
	if (ch  == '{') {
	    object = parse_object(&jsontext);
	} else {
	    jsontext.pos++;
	}
    }

    return object;
}

static void method2string(LS_Method method, char *buf) {
    switch (method) {
	case Initialize:
	    sprintf(buf, "Initialize");
	    break;
	case Initialized:
	    sprintf(buf, "Initialized");
	    break;
	case TextDocument_didOpen:
	    sprintf(buf, "textDocument/didOpen");
	    break;
	case TextDocument_didClose:
	    sprintf(buf, "textDocument/didClose");
	    break;
	case TextDocument_didChange:
	    sprintf(buf, "textDocument/didChange");
	    break;
	case TextDocument_publishDiagnostics:
	    sprintf(buf, "textDocument/publishDiagnostics");
	    break;
	case Shutdown:
	    sprintf(buf, "Shutdown");
	    break;
	case Exit:
	    sprintf(buf, "Exit");
	    break;
	case LSMethodCount:
	default:
	    break;
    }

    return;
}

static void make_initialize_result_message(LS_Initialize_result initialize_result, char *result_buf) {
    if (initialize_result.server_capabilities.isNull == 1) {
	sprintf(result_buf, "\"capabilities\":{}");
    } else {
	LS_textdocument_sync_option option = initialize_result.server_capabilities.textDocumentSync;
	char openclose_buf[6];
	if (option.openClose == 1) {
	    sprintf(openclose_buf, "true");
	} else {
	    sprintf(openclose_buf, "false");
	}

	char synckind_buf[2];
	switch (option.textDocumentSyncKind) {
	    case None:
		sprintf(synckind_buf, "0");
		break;
	    case Full:
		sprintf(synckind_buf, "1");
		break;
	    case Incremental:
		sprintf(synckind_buf, "2");
		break;
	    case LSTextDocumentSyncKindCount:
	    default:
		sprintf(synckind_buf, "3");
		break;
	}

	sprintf(result_buf, "\"capabilities\":{\"textDocumentSync\":{\"openClose\":%s,\"change\":%s}}", openclose_buf, synckind_buf);
    }

    return;
}

static void make_diagnostics_params(LS_PublishDiagnostics_textdocument_param param, char *param_buf) {
    printf("checkpoint param.uri %s, diagnostics %s\n", param.uri, param.diagnostics.message);
    sprintf(param_buf, "\"PublishDiagnosticsParams\":{\"uri\":\"%s\",\"diagnostics\":[{\"message\":\"%s\"}]}", param.uri, param.diagnostics.message);
    
    return;
}

void make_json_message(LS_Content *content, char *res_message) {
    char param_buf[PARAM_BUF_LEN];
    char result_buf[RESULT_BUF_LEN];

    char str_method[METHOD_BUF_LEN];

    char res_content[RES_CONTENT_LEN];

    method2string(content->method, str_method);
    
    result_buf[0] = '\0';
    param_buf[0] = '\0';
    switch (content->content_type) {
	case RequestMessage:
	    break;
	case ResponseMessage:
	    switch (content->method) {
		case Initialize:
		    if (content->result.object != NULL) {
			make_initialize_result_message(content->result.object->initialize_result, result_buf);
		    }
		    break;
		case Initialized:
		    break;
		case TextDocument_didOpen:
		    break;
		case TextDocument_didClose:
		    break;
		case TextDocument_didChange:
		    break;
		case Shutdown:
		    break;
		case Exit:
		case LSMethodCount:
		default:
		    break;
	    }
	    sprintf(res_content, "{\"method\":\"%s\",\"jsonrpc\":\"%s\",\"id\":%d,\"result\":{%s}}", str_method, content->jsonrpc, content->id, result_buf);
	    break;
	case NotificationMessage:
	    if (content->method == TextDocument_publishDiagnostics) {
		make_diagnostics_params(content->params.object->diagnostics_td_params, param_buf);
	    }
	    sprintf(res_content, "{\"method\":\"%s\",\"jsonrpc\":\"%s\",\"params\":{%s}}", str_method, content->jsonrpc, param_buf);
	    break;
	case LSContentTypeCount:
	default:
	    break;
    }
    
    int reslen = strlen(res_content);
    sprintf(res_message, "Content-Length: %d\r\nContent-Type: application/vim-jsonrpc; charset=utf-8\r\n\r\n%s", reslen, res_content);
    return;
}


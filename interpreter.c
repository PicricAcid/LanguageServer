#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "main.h"

static void make_ls_method(Json_object *object, LS_Content *content); 
static void make_ls_jsonrpc(Json_object *object, LS_Content *content);
static void make_ls_id(Json_object *object, LS_Content *content);
static void make_ls_params_initialize(Json_object *jsonobject, LS_Param *params);
static void make_ls_params_td_didopen(Json_object *jsonobject, LS_Param *params);
static void make_ls_params_td_didchange(Json_object *jsonobject, LS_Param *params);
static void make_ls_params_td_didclose(Json_object *jsonobject, LS_Param *params);
static void make_ls_params(Json_object *object, LS_Content *content);
static void make_ls_result_textDocumentSync(Json_object *object, LS_textdocument_sync_option *textDocumentSync);
static LS_Object *make_diagnostics_param(LS_Content *content, LS_Diagnostics_type diatype);
static void make_ls_result_initialize(Json_object *object, LS_Result *result);
static void make_ls_result(Json_object *object, LS_Content *content);
static void make_ls_content(Json_object *object, LS_Content *content);

static void print_ls_content_type(LS_Content_type content_type);
static void print_method(LS_Method method);
static void print_params(LS_Content *content);
static void print_content(LS_Content *content);

static LS_Object *make_initialize_result(LS_Content *content);

void interpreter(Json_object *object, LS_Content *content) {
    make_ls_content(object, content);
    print_content(content);
    
    return;
}

LS_Content *init_content(void) {
    LS_Content *content = (LS_Content*)malloc(sizeof(LS_Content));

    content->content_type = LSContentTypeCount;
    memset(content->jsonrpc, '\0', sizeof(content->jsonrpc));
    content->id = 0;
    content->method = LSMethodCount;
    content->params.object = NULL;
    content->result.object = NULL;
    
    return content;
}

void free_content(LS_Content *content) {
    if (content->params.object != NULL) {
	free(content->params.object);
    }
    if (content->result.object != NULL) {
	free(content->result.object);
    }
    free(content);

    return;
}

static LS_Object *make_initialize_result(LS_Content *content) {
    LS_Object *result = (LS_Object*)malloc(sizeof(LS_Object));
    result->initialize_result.server_capabilities.isNull = 0;
    result->initialize_result.server_capabilities.textDocumentSync.openClose = 1;
    result->initialize_result.server_capabilities.textDocumentSync.textDocumentSyncKind = Full;

    return result;
}

LS_Content *make_initialize_response(LS_Content *content) {
    LS_Content *res_content = init_content();
    res_content->content_type = ResponseMessage;
    strcpy(res_content->jsonrpc, content->jsonrpc);
    res_content->id = content->id;
    res_content->method = Initialize;
    res_content->result.object = make_initialize_result(content);

    return res_content;
}

LS_Content *make_shutdown_response(LS_Content *content) {
    LS_Content *res_content = init_content();
    res_content->content_type = ResponseMessage;
    strcpy(res_content->jsonrpc, content->jsonrpc);
    res_content->id = content->id;
    res_content->method = Shutdown;
    res_content->result.object = NULL;

    return res_content;
}

static LS_Object *make_diagnostics_param(LS_Content *content, LS_Diagnostics_type diatype) {
    LS_Object *param = (LS_Object*)malloc(sizeof(LS_Object));
    switch (diatype) {
	case INCORRECT_SENTENCE:
	    sprintf(param->diagnostics_td_params.diagnostics.message, "Incorrect sentence");
	    break;
	case LSDiagnosticsTypeCount:
	    break;
	default:
	    break;
    }

    return param;
}

LS_Content *make_diagnostics_notification(LS_Content *content, LS_Diagnostics_type diatype) {
    LS_Content *notif_content = init_content();
    notif_content->content_type = NotificationMessage;
    strcpy(notif_content->jsonrpc, content->jsonrpc);
    notif_content->id = 0;
    notif_content->method = TextDocument_publishDiagnostics;
    notif_content->params.object = make_diagnostics_param(content, diatype);

    return notif_content;
}

static void make_ls_method(Json_object *object, LS_Content *content) {
    if (object->type != JSONValueTypeString) {
	fprintf(stderr, "Incorrect message: method \n");
	exit(-1);
    }

    if (strcmp(object->value.string_value, "Initialize") == 0) {
	content->method = Initialize;
	content->content_type = RequestMessage;
    } else if (strcmp(object->value.string_value, "Initialized") == 0) {
	content->method = Initialized;
	content->content_type = NotificationMessage;
    } else if (strcmp(object->value.string_value, "textDocument/didOpen") == 0) {
	content->method = TextDocument_didOpen;
	content->content_type = NotificationMessage;
    } else if (strcmp(object->value.string_value, "textDocument/didClose") == 0) {
	content->method = TextDocument_didClose;
	content->content_type = NotificationMessage;
    } else if (strcmp(object->value.string_value, "textDocument/didChange") == 0) {
	content->method = TextDocument_didChange;
	content->content_type = NotificationMessage;
    } else if (strcmp(object->value.string_value, "Shutdown") == 0) {
	content->method = Shutdown;
	content->content_type = RequestMessage;
    } else if (strcmp(object->value.string_value, "Exit") == 0) {
	content->method = Exit;
	content->content_type = NotificationMessage;
    } else {
	fprintf(stderr, "Incorrect message: method %s\n", object->value.string_value);
    }

    return;
}

static void make_ls_jsonrpc(Json_object *object, LS_Content *content) {
    if (object->type != JSONValueTypeString) {
	fprintf(stderr, "Incorrect message: jsonrpc");
	exit(-1);
    }
    
    if (strlen(object->value.string_value) < 10) {
	strcpy(content->jsonrpc, object->value.string_value);
    } else {
	fprintf(stderr, "Incorrect message: jsonrpc %s\n", object->value.string_value);
    }

    return;
}

static void make_ls_id(Json_object *object, LS_Content *content) {
    if (object->type != JSONValueTypeInt) {
	fprintf(stderr, "Incorrect message: id\n");
	exit(-1);
    }

    content->id = object->value.int_value;

    return;
}

static void make_ls_params_initialize(Json_object *jsonobject, LS_Param *params) {
    Json_object *p = jsonobject;
    LS_Object *ls_object = (LS_Object*)malloc(sizeof(LS_Object));

    while (p != NULL) {
	if (strcmp(p->key_name, "processId") == 0) {
	    if (p->type != JSONValueTypeInt) {
		fprintf(stderr, "Incorrect message: initalize param\n");
		exit(-1);
	    }
	    ls_object->initialize_params.process_id = p->value.int_value;
	} else if (strcmp(p->key_name, "rootPath") == 0) {
	    if (p->type != JSONValueTypeString) {
		fprintf(stderr, "Incorrect message: initialize param\n");
		exit(-1);
	    }
	    if (strlen(p->value.string_value) <= LS_ROOTPATH_LEN) {
		strcpy(ls_object->initialize_params.rootpath, p->value.string_value);
	    }
	} else if (strcmp(p->key_name, "capabilities") == 0) {
	    if (p->type != JSONValueTypeObject) {
		fprintf(stderr, "Incorrect message: initialize param\n");
		exit(-1);
	    }
	    Json_object *q = p->value.object_value;
	    while (q != NULL) {
		if (strcmp(q->key_name, "textDocumentSync") == 0) {
		    if (q->type != JSONValueTypeObject) {
			fprintf(stderr, "Incorrect message: initalize param\n");
			exit(-1);
		    }
		    Json_object *r = q->value.object_value;
		    while (r != NULL) {
			if (strcmp(r->key_name, "openClose") == 0) {
			    if (r->type != JSONValueTypeInt) {
				fprintf(stderr, "Incorrect message: intialize param\n");
				exit(-1);
			    }
			    ls_object->initialize_params.client_capabilities.textDocumentSync.openClose = r->value.int_value;
			} else if (strcmp(r->key_name, "textDocumentSyncKind") == 0) {
			    if (r->type != JSONValueTypeString) {
				fprintf(stderr, "Incorrect message: initialize param\n");
				exit(-1);
			    }
			    if (strcmp(r->value.string_value, "None") == 0) {
				ls_object->initialize_params.client_capabilities.textDocumentSync.textDocumentSyncKind = None;
			    } else if (strcmp(r->value.string_value, "Full") == 0) {
				ls_object->initialize_params.client_capabilities.textDocumentSync.textDocumentSyncKind = Full;
			    } else if (strcmp(r->value.string_value, "Incremental") == 0) {
				ls_object->initialize_params.client_capabilities.textDocumentSync.textDocumentSyncKind = Incremental;
			    } else {
				ls_object->initialize_params.client_capabilities.textDocumentSync.textDocumentSyncKind = LSTextDocumentSyncKindCount;
			    }
			}
			r = r->next;
		    }    
		}
		q = q->next;
	    }
	} else {
	}
	p = p->next;
    }

    params->object = ls_object;

    return;
}

static void make_ls_params_td_didopen(Json_object *jsonobject, LS_Param *params) {
    Json_object *p = NULL;
    if (strcmp(jsonobject->key_name, "textDocument") == 0) {
	if (jsonobject->type != JSONValueTypeObject) {
	    fprintf(stderr, "Incorrect message: textDocument/didOpen\n");
	    exit(-1);
	}
	p = jsonobject->value.object_value;
    }

    LS_Object *ls_object = (LS_Object*)malloc(sizeof(LS_Object));
    while (p != NULL) {
	if (strcmp(p->key_name, "uri") == 0) {
	    if (p->type != JSONValueTypeString) {
		fprintf(stderr, "Incorrect message: textDocument/didOpen\n");
		exit(-1);
	    }
	    if (strlen(p->value.string_value) < URI_LEN) {
		strcpy(ls_object->didopen_td_params.textDocumentItem.uri, p->value.string_value);
	    } else {
		fprintf(stderr, "URISizeError: textDocument/didOpen\n");
	    }
	} else if (strcmp(p->key_name, "languageID") == 0) {
	    if (p->type != JSONValueTypeString) {
		fprintf(stderr, "Incorrect message: textDocument/didOpen\n");
		exit(-1);
	    }
	    if (strlen(p->value.string_value) < LANGUAGE_ID_LEN) {
		strcpy(ls_object->didopen_td_params.textDocumentItem.language_id, p->value.string_value);
	    } else {
		fprintf(stderr, "LanguageIDSizeError: textDocument/didOpen\n");
	    }
	} else if (strcmp(p->key_name, "version") == 0) {
	    if (p->type != JSONValueTypeInt) {
		fprintf(stderr, "Incorrect message: textDocument/didOpen\n");
		exit(-1);
	    }
	    ls_object->didopen_td_params.textDocumentItem.version = p->value.int_value;
	} else if (strcmp(p->key_name, "text") == 0) {
	    if (p->type != JSONValueTypeString) {
		fprintf(stderr, "Incorrect message: textDocument/didOpen\n");
		exit(-1);
	    }
	    if (strlen(p->value.string_value) < TEXT_LEN) {
		strcpy(ls_object->didopen_td_params.textDocumentItem.text, p->value.string_value);
	    } else {
		fprintf(stderr, "TextSizeError: textDocument/didOpen\n");
	    }
	} else {
	}

	p = p->next;
    }
    
    params->object = ls_object;

    return;
}

static void make_ls_params_td_didchange(Json_object *jsonobject, LS_Param *params) {
    Json_object *p = jsonobject;
    LS_Object *ls_object = (LS_Object*)malloc(sizeof(LS_Object));
    
    while (p != NULL) {
	if (strcmp(p->key_name, "textDocument") == 0) {
	    if (p->type != JSONValueTypeObject) {
		fprintf(stderr, "Incorrect message: textDocument/didChange\n");
		exit(-1);
	    }
	    Json_object *q = p->value.object_value;
	    while (q != NULL) {
		if (strcmp(q->key_name, "uri") == 0) {
		    if (q->type != JSONValueTypeString) {
			fprintf(stderr, "Incorrect message: textDocument/didChange\n");
			exit(-1);
		    }
		    if (strlen(q->value.string_value) < URI_LEN) {
			strcpy(ls_object->didchange_td_params.versioned_td_identifier.uri, q->value.string_value);
		    } else {
			fprintf(stderr, "URISizeError: textDocument/didChange\n");
		    }
		}else if (strcmp(q->key_name, "version") == 0) {
		    if (q->type != JSONValueTypeInt) {
			fprintf(stderr, "Incorrect message: textDocument/didChange\n");
			exit(-1);
		    }
		    ls_object->didchange_td_params.versioned_td_identifier.version = q->value.int_value;
		} else {
		}
		q = q->next;
	    }
	} else if (strcmp(p->key_name, "contentChanges") == 0) {
	    if (p->type != JSONValueTypeObject) {
		fprintf(stderr, "Incorrect message: textDocument/didChange\n");
		exit(-1);
	    }
	    Json_object *q = p->value.object_value;
	    while (q != NULL) {
		if (strcmp(q->key_name, "text") == 0) {
		    if (q->type != JSONValueTypeString) {
			fprintf(stderr, "Incorrect message: textDocument/didChange\n");
			exit(-1);
		    }
		    if (strlen(q->value.string_value) > TEXT_LEN) {
			fprintf(stderr, "TextSizeError: textDocument/didChange\n");
		    } else {
			strcpy(ls_object->didchange_td_params.td_content_change_event.text, q->value.string_value);
		    }
		} else {
		}
		q = q->next;
	    }
	} else {
	}
	p = p->next;
    }

    params->object = ls_object;
    return;
}

static void make_ls_params_td_didclose(Json_object *jsonobject, LS_Param *params) {
    Json_object *p = jsonobject;
    LS_Object *ls_object = (LS_Object*)malloc(sizeof(LS_Object));
    
    while (p != NULL) {
	if (strcmp(p->key_name, "textDocument") == 0) {
	    if (p->type != JSONValueTypeObject) {
		fprintf(stderr, "Incorrect message: textDocument/didClose\n");
		exit(-1);
	    }
	    Json_object *q = p->value.object_value;
	    while (q != NULL) {
		if (strcmp(q->key_name, "uri") == 0) {
		    if (q->type != JSONValueTypeString) {
			fprintf(stderr, "Incorrect message: textDocument/didClose\n");
			exit(-1);
		    }
		    if (strlen(q->value.string_value) < URI_LEN) {
			strcpy(ls_object->didclose_td_params.textDocument.uri, q->value.string_value);
		    } else {
			fprintf(stderr, "URISizeError: textDocument/didClose\n");
		    }
		}else if (strcmp(q->key_name, "version") == 0) {
		    if (q->type != JSONValueTypeInt) {
			fprintf(stderr, "Incorrect message: textDocument/didClose\n");
			exit(-1);
		    }
		    ls_object->didclose_td_params.textDocument.version = q->value.int_value;
		} else {
		}
		q = q->next;
	    }
	} else {
	}

	p = p->next;
    }

    params->object = ls_object;
    return;
}

static void make_ls_params(Json_object *object, LS_Content *content) {
    if (object->type != JSONValueTypeObject) {
	fprintf(stderr, "Incorrect message: params\n");
	exit(-1);
    }

    switch (content->method) {
	case Initialize:
	    make_ls_params_initialize(object->value.object_value, &content->params);
	    break;
	case TextDocument_didOpen:
	    make_ls_params_td_didopen(object->value.object_value, &content->params);
	    break;
	case TextDocument_didClose:
	    make_ls_params_td_didclose(object->value.object_value, &content->params);
	    break;
	case TextDocument_didChange:
	    make_ls_params_td_didchange(object->value.object_value, &content->params);
	    break;
	case Shutdown:
	    break;
	case Exit:
	    break;
	case LSMethodCount:
	default:
	    fprintf(stderr, "Incorrect message: params\n");
	    break;
    }
    
    return;
}

static void make_ls_result_textDocumentSync(Json_object *object, LS_textdocument_sync_option *textDocumentSync) {
    Json_object *p = object;

    while (p != NULL) {
	if (strcmp(p->key_name, "openClose") == 0) {
	    if (p->type != JSONValueTypeInt) {
		fprintf(stderr, "Incorrect message: textDocumentSync\n");
		exit(-1);
	    }
	    textDocumentSync->openClose = p->value.int_value;
	} else if (strcmp(p->key_name, "change") == 0) {
	    if (p->type != JSONValueTypeInt) {
		fprintf(stderr, "Incorrect message: textDocumentSync\n");
		exit(-1);
	    }
	    textDocumentSync->textDocumentSyncKind = p->value.int_value;
	} else {
	    fprintf(stderr, "Incorrect message: textDocumentSync\n");
	}
    }
    return;
}

static void make_ls_result_initialize(Json_object *object, LS_Result *result) {
    Json_object *p = object;

    while (p != NULL) {
	if (strcmp(p->key_name, "textDocumentSync") == 0) {
	    if (p->type != JSONValueTypeObject) {
		fprintf(stderr, "Incorrect message: textDocumentSync\n");
		exit(-1);
	    }
	    make_ls_result_textDocumentSync(p->value.object_value, &result->object->initialize_result.server_capabilities.textDocumentSync);
	}
    }
    return;
}

static void make_ls_result(Json_object *object, LS_Content *content) {
    if (object->type != JSONValueTypeObject) {
	fprintf(stderr, "Incorrect message: result\n");
	exit(-1);
    }

    switch (content->method) {
	case Initialize:
	    make_ls_result_initialize(object->value.object_value, &content->result);
	    break;
	case TextDocument_didOpen:
	    break;
	case TextDocument_didClose:
	    break;
	case TextDocument_didChange:
	    break;
	case Shutdown:
	    break;
	case Initialized:
	case Exit:
	case LSMethodCount:
	default:
	    fprintf(stderr, "Incorrect message: result\n");
	    break;
    }
    return;
}

static void make_ls_content(Json_object *object, LS_Content *content) {
    Json_object *p = object;
    while (p != NULL) {
	if (strcmp(p->key_name, "method") == 0) {
	    make_ls_method(p, content);
	} else if (strcmp(p->key_name, "jsonrpc") == 0) {
	    make_ls_jsonrpc(p, content);
	} else if (strcmp(p->key_name, "id") == 0) {
	    make_ls_id(p, content);
	} else if (strcmp(p->key_name, "params") == 0) {
	    make_ls_params(p, content);
	} else if (strcmp(p->key_name, "result") == 0) {
	    make_ls_result(p, content);
	} else {
	    fprintf(stderr, "Incorrect message: key %s\n", p->key_name);
	    exit(-1);
	}
	p = p->next;
    }

    return;
}

static void print_ls_content_type(LS_Content_type content_type) {
    switch (content_type) {
	case RequestMessage:
	    printf("LS_Content_type: RequestMessage\n");
	    break;
	case ResponseMessage:
	    printf("LS_Content_type: ResponseMessage\n");
	    break;
	case NotificationMessage:
	    printf("LS_Content_type: NotificationMessage\n");
	    break;
	case LSContentTypeCount:
	default:
	    printf("LS_Content_type: None\n");
	    break;
    }

    return;
}

static void print_method(LS_Method method) {
    switch (method) {
	case Initialize:
	    printf("Method: Initialize\n");
	    break;
	case Initialized:
	    printf("Method: Initialized\n");
	    break;
	case TextDocument_didOpen:
	    printf("Method: textDocument/didOpen\n");
	    break;
	case TextDocument_didClose:
	    printf("Method: textDocument/didClose\n");
	    break;
	case TextDocument_didChange:
	    printf("Method: textDocument/didChange\n");
	    break;
	case Shutdown:
	    printf("Method: Shutdown\n");
	    break;
	case Exit:
	    printf("Method: Exit\n");
	    break;
	case LSMethodCount:
	default:
	    break;
    }

    return;
}

static void print_params(LS_Content *content) {
    switch (content->method) {
	case Initialize:
	    printf("param: Initialize\n");
	    LS_Initialize_param initialize_param = content->params.object->initialize_params;
	    printf(" process_id: %d\n", initialize_param.process_id);
	    printf(" rootpath: %s\n", initialize_param.rootpath);
	    printf(" LS_Capabilities: openClose %d, textDocumentSyncKind %d\n",initialize_param.client_capabilities.textDocumentSync.openClose, initialize_param.client_capabilities.textDocumentSync.textDocumentSyncKind);
	    break;
	case TextDocument_didOpen:
	    printf("param: textDocument/didOpen\n");
	    LS_Didopen_textdocument_param td_open_param = content->params.object->didopen_td_params;
	    printf(" uri: %s\n", td_open_param.textDocumentItem.uri);
	    printf(" language_id: %s\n", td_open_param.textDocumentItem.language_id);
	    printf(" version: %d\n", td_open_param.textDocumentItem.version);
	    printf(" text: %s\n", td_open_param.textDocumentItem.text);
	    break;
	case TextDocument_didClose:
	    printf("param: textDocument/didClose\n");
	    LS_Didclose_textdocument_param td_close_param = content->params.object->didclose_td_params;
	    printf(" textDocument: uri %s, version: %d\n", td_close_param.textDocument.uri, td_close_param.textDocument.version);
	    break;
	case TextDocument_didChange:
	    printf("param: textDocument/didChange\n");
	    LS_Didchange_textdocument_param td_change_param = content->params.object->didchange_td_params;
	    printf(" textDocument: uri %s, version: %d\n", td_change_param.versioned_td_identifier.uri, td_change_param.versioned_td_identifier.version);
	    printf(" contentChanges: %s\n", td_change_param.td_content_change_event.text);
	    break;
	case Shutdown:
	    break;
	case Initialized:
	case Exit:
	case LSMethodCount:
	default:
	    break;
    }

    return;
}

static void print_content(LS_Content *content) {
    print_ls_content_type(content->content_type);
    printf("jsonrpc: %s\n", content->jsonrpc);
    print_method(content->method);
    if (content->params.object != NULL) {
	print_params(content);
    }
    /* print_result(content); */
    
    return;
}


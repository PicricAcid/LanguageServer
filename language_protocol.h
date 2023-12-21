#ifndef LANGUAGE_PROTOCOL_H
#define LANGUAGE_PROTOCOL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CONTENT_TYPE_SIZE 256
#define LS_ERROR_MESSAGE_LEN 1024
#define LS_RESULT_STRING_LEN 1024
#define LS_DATA_STRING_LEN 1024
#define LS_ROOTPATH_LEN 256
#define URI_LEN 256
#define LANGUAGE_ID_LEN 256
#define TEXT_LEN 2048
#define MESSAGE_LEN 256

typedef struct {
    int content_length;
    char content_type[CONTENT_TYPE_SIZE];
} LS_Header;

typedef enum {
    Initialize,
    Initialized,
    TextDocument_didOpen,
    TextDocument_didClose,
    TextDocument_didChange,
    TextDocument_publishDiagnostics,
    Shutdown,
    Exit,
    LSMethodCount
} LS_Method;

typedef enum {
    INCORRECT_SENTENCE,
    LSDiagnosticsTypeCount
} LS_Diagnostics_type;

typedef enum {
    None,
    Full,
    Incremental,
    LSTextDocumentSyncKindCount
} LS_textdocument_sync_kind;

typedef struct {
    int openClose;
    LS_textdocument_sync_kind textDocumentSyncKind;
} LS_textdocument_sync_option;

typedef struct {
    int isNull;
    LS_textdocument_sync_option textDocumentSync;
} LS_Capabilities;

typedef struct {
    int process_id;
    char rootpath[LS_ROOTPATH_LEN];
    LS_Capabilities client_capabilities;
} LS_Initialize_param;

typedef struct {
    LS_Capabilities server_capabilities;
} LS_Initialize_result;

typedef struct {
    char uri[URI_LEN];
    char language_id[LANGUAGE_ID_LEN];
    int version;
    char text[TEXT_LEN];
} LS_textdocument_item;

typedef struct {
    LS_textdocument_item textDocumentItem;
} LS_Didopen_textdocument_param;

typedef struct {
  char uri[URI_LEN]; 
  int version;
} LS_textdocument_identifier;

typedef struct {
    LS_textdocument_identifier textDocument;
} LS_Didclose_textdocument_param;

typedef struct {
    int line;
    int character;
} LS_Position;

typedef struct {
    LS_Position start;
    LS_Position end;
} LS_Range;

typedef struct {
    char text[TEXT_LEN];
} LS_content_changes;

typedef struct {
    LS_textdocument_identifier versioned_td_identifier;
    LS_content_changes td_content_change_event;
} LS_Didchange_textdocument_param;

typedef struct {
    char message[MESSAGE_LEN];
} LS_Diagnostics;

typedef struct {
    char uri[URI_LEN];
    int version;
    LS_Diagnostics diagnostics;
} LS_PublishDiagnostics_textdocument_param;

typedef union {
    LS_Initialize_param initialize_params;
    LS_Initialize_result initialize_result;
    LS_Didopen_textdocument_param didopen_td_params;
    LS_Didclose_textdocument_param didclose_td_params;
    LS_Didchange_textdocument_param didchange_td_params;
    LS_PublishDiagnostics_textdocument_param diagnostics_td_params;
} LS_Object;

typedef union {
    LS_Object *object;
} LS_Param;

typedef union {
    LS_Object *object;
} LS_Result;

typedef enum { 
    RequestMessage,
    ResponseMessage,
    NotificationMessage,
    LSContentTypeCount
} LS_Content_type;

typedef struct {
    LS_Content_type content_type;
    char jsonrpc[10];
    int id;
    LS_Method method;
    LS_Param params;
    LS_Result result;
} LS_Content;

typedef struct {
    LS_Header header;
    LS_Content content;
} LS_Base_protocol;

#endif /* LANGUAGE_PROTOCOL_H */

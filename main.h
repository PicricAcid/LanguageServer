#include <stdio.h>
#include <stdlib.h>

#include "json_parse.h"
#include "language_protocol.h"

/* json_parse.c */
Json_object *json_parse(const char* text);
void print_object(Json_object *head);
void free_object(Json_object *head);
void make_json_message(LS_Content *content, char *res_message);

/* interpreter.c */
void interpreter(Json_object *object, LS_Content *content);
LS_Content *init_content(void);
void free_content(LS_Content *content);
LS_Content *make_initialize_response(LS_Content *content);
LS_Content *make_shutdown_response(LS_Content *content);
LS_Content *make_diagnostics_notification(LS_Content *content, LS_Diagnostics_type diatype);

/* language_server.c */
void language_server(void);

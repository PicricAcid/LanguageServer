#ifndef JSON_PARSE_H
#define JSON_PARSE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define JSON_KEY_LEN 16
#define STRING_LEN 1024

typedef struct Json_object_mold Json_object;

typedef union {
    int int_value;
    char string_value[STRING_LEN];
    Json_object *object_value;
} Json_value;

typedef enum {
    JSONValueTypeInt,
    JSONValueTypeString,
    JSONValueTypeObject,
    JSONValueTypeCount
} Json_value_type;

typedef struct {
    int pos;
    int len;
    char *text;
} Json_text;

struct Json_object_mold {
    char key_name[JSON_KEY_LEN];
    Json_value_type type;
    Json_value value;
    struct Json_object_mold *next;
};

#endif /* JSON_PARSE_H */

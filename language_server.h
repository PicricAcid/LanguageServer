#include <stdio.h>
#include <stdlib.h>

#include "language_protocol.h"

typedef struct {
    int is_open;
    char uri[URI_LEN];
    char languageID[LANGUAGE_ID_LEN];
    char text[TEXT_LEN];
} LS_openfile;

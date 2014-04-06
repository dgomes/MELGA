#ifndef _JSON_H_
#define _JSON_H_

#include <string.h>
#include <stdio.h>
#include <jansson.h>
#include "utils.h"

int checkJSON_integer(const char *data, const char *name, int value); 

#endif

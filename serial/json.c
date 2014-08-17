#include "json.h"

int checkJSON_integer(const char *data, const char *name, int value)  {
	if(!strlen(data)) return -1;

//	fprintf(stderr, "checkJSON_integer: %s\n", data);
	json_error_t error;
	json_t *root = json_loads(data, 0, &error);
	int r = -1;

	if(!root) {
		fprintf(stderr, "<%s>\n", data);
		fprintf(stderr, "error: on line %d: %s\n", error.line, error.text);
		return 1;
	}

	json_t *code = json_object_get(root, name);
	if(!json_is_integer(code)) {
		fprintf(stderr, "error: code is not an integer\n");
		r=1;
	}

	if(json_integer_value(code) != value)
		r=2;

	free(root);
	free(code);
	return r;
}

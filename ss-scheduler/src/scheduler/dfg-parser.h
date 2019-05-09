#include <stdbool.h>

typedef struct var_container {
	char *name;
	double value;
} var_container;

typedef struct calculator {
	bool equals;
	int count;
	char * var;
	char * find;
	struct var_container **cont;
	double dub;
} calculator;

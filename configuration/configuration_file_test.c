#include <stdio.h>
#include <stdbool.h>
#include "configuration_file.h"
#include "glla.h"

#define BOOLSTR(x) (x ? "true" : "false")

int main()
{
	int numvars = 100;
	int vartaglen = 32;
	char tag_buf[vartaglen*numvars];
	struct config_var vars[numvars];
	struct config_buf buf = config_new(tag_buf, sizeof(tag_buf)/sizeof(tag_buf[0]), vars, sizeof(vars)/sizeof(vars[0]));

	int some_special_int = 4;
	int some_other_int = 20;
	float some_float = .01;
	vec3 some_vector = {{0.0, 0.0, 0.0}};
	bool some_bool = 0;
	bool some_other_bool = 1;
	bool some_unchanging_bool = 1;

	// config_expose_int(&buf, &some_other_int, "SOME_OTHER_INT");
	// config_expose_int(&buf, &some_special_int, "SOME_SPECIAL_INT");
	// config_expose_float(&buf, &some_float, "SOME_FLOAT");
	// config_expose_float3(&buf, some_vector.A, "SOME_VECTOR");
	// config_expose_bool(&buf, &some_bool, "SOME_BOOL");
	// config_expose_bool(&buf, &some_other_bool, "SOME_OTHER_BOOL");
	// config_expose_bool(&buf, &some_unchanging_bool, "SOME_UNCHANGING_BOOL");
	config_expose(&buf, &some_other_int, "SOME_OTHER_INT");
	config_expose(&buf, &some_special_int, "SOME_SPECIAL_INT");
	config_expose(&buf, &some_float, "SOME_FLOAT");
	config_expose(&buf, &some_vector, "SOME_VECTOR");
	config_expose(&buf, &some_bool, "SOME_BOOL");
	config_expose(&buf, &some_other_bool, "SOME_OTHER_BOOL");
	config_expose(&buf, &some_unchanging_bool, "SOME_UNCHANGING_BOOL");


	printf("some_special_int value is %i\n", some_special_int);
	printf("some_other_int value is: %d\n", some_other_int);
	printf("float is %f, vector is <%f, %f, %f>\n", some_float, some_vector.x, some_vector.y, some_vector.z);
	printf("some_bool is %s, some_other_bool is %s, some_unchanging_bool is %s\n", BOOLSTR(some_bool), BOOLSTR(some_other_bool), BOOLSTR(some_unchanging_bool));

	config_load(buf, "./int_test.conf");

	printf("some_special_int value is %i\n", some_special_int);
	printf("some_other_int value is: %d\n", some_other_int);
	printf("float is %f, vector is <%f, %f, %f>\n", some_float, some_vector.x, some_vector.y, some_vector.z);
	printf("some_bool is %s, some_other_bool is %s, some_unchanging_bool is %s\n", BOOLSTR(some_bool), BOOLSTR(some_other_bool), BOOLSTR(some_unchanging_bool));
	return 0;
}
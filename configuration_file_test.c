#include <stdio.h>
#include "configuration_file.h"
#include "vector3.h"

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
	V3 some_vector = {{{0.0, 0.0, 0.0}}};

	config_expose_int(&buf, &some_other_int, "SOME_OTHER_INT");
	config_expose_int(&buf, &some_special_int, "SOME_SPECIAL_INT");
	config_expose_float(&buf, &some_float, "SOME_FLOAT");
	config_expose_float3(&buf, some_vector.A, "SOME_VECTOR");

	printf("some_special_int value is %i\n", some_special_int);
	printf("some_other_int value is: %d\n", some_other_int);
	printf("float is %f, vector is <%f, %f, %f>\n", some_float, some_vector.x, some_vector.y, some_vector.z);

	config_load(buf, "./int_test.conf");

	printf("some_special_int value is %i\n", some_special_int);
	printf("some_other_int value is: %d\n", some_other_int);
	printf("float is %f, vector is <%f, %f, %f>\n", some_float, some_vector.x, some_vector.y, some_vector.z);
	return 0;
}
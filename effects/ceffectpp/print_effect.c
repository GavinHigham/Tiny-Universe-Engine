#include <stdio.h>
#include "print_effect.h"

void print_filepaths(char **paths, int len)
{
	printf("const char *shader_file_paths[] = {\n");
	for (int i = 0; i < len; i++) {
		char *comma = i < (len-1) ? ",":"";
		if (paths[i] == NULL)
			printf("\tNULL%s\n", comma);
		else
			printf("\t\"%s\"%s\n", paths[i], comma);
	}
	printf("};\n\n");
}

void print_effect_definition(char **ustrs, int ulen, char **astrs, int alen)
{
	printf("typedef struct effect_data {\n\tGLuint handle;\n\tunion {\n\t\tstruct {\n");
	for (int i = 0; i < ulen; i++)
		printf("\t\t\tGLint %s;\n", ustrs[i]);
	printf("\t\t};\n\t\tGLint unif[%i];\n\t};\n\tunion {\n\t\tstruct {\n", ulen);
	for (int i = 0; i < alen; i++)
		printf("\t\t\tGLint %s;\n", astrs[i]);
	printf("\t\t};\n\t\tGLint attr[%i];\n\t};\n} EFFECT;\n\n", alen);
}

void print_effect_list(char **progstrs, int len)
{
	printf("union effect_list {\n\tstruct {\n");
	for (int i = 0; i < len; i++)
		printf("\t\tEFFECT %s;\n", progstrs[i]);
	printf("\t};\n\tEFFECT all[%i];\n};\n\n", len);
}

void print_uniform_strings(char **ustrs, unsigned long len)
{
	printf("const char *uniform_strings[] = {");
	for (int i = 0; i < len; i++)
		printf("\"%s\"%s", ustrs[i], i < (len-1) ? ", ":"");
	printf("};\n");
}

void print_attribute_strings(char **astrs, unsigned long len)
{
	printf("const char *attribute_strings[] = {");
	for (int i = 0; i < len; i++)
		printf("\"%s\"%s", astrs[i], i < (len-1) ? ", ":"");
	printf("};\n");
}
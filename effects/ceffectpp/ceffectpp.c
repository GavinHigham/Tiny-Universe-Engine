/*
Extracts a list of attribute names and uniform names from a set of GLSL shaders.

Limitations:
	Does not find uniforms defined in interface blocks.
	Does not handle '\' continuing some statement onto the next line.
	Does not handle C-style comments in a perfectly correct way.
	Works only on ASCII shader files.
*/

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdbool.h>
#include "table.h"
#include "print_effect.h"

enum {
	NHASH = 17,
	MAX_LINE_LENGTH = 256,
};

enum shader_type {
	VSHADER    = 0, //These are used as array indices, don't change.
	GSHADER    = 1,
	FSHADER    = 2,
	NUM_SHADER_TYPES = 3,
	NOT_SHADER = -1
};

static const char *vs_file_extensions[] = {".vs", ".vert"};
static const char *fs_file_extensions[] = {".fs", ".frag"};
static const char *gs_file_extensions[] = {".gs"};

static TABLE *uniforms, *attributes, *programs;


int strptrcmp(const void *str1, const void *str2)
{
	return strcmp(*(char **)str1, *(char **)str2);
}

void store_path(char *path, enum shader_type type)
{
	char newpath[strlen(path)+1];
	strcpy(newpath, path);
	char *c = strrchr(newpath, '.'); //Find the file extension.
	*c = '\0'; //Insert only the path up to the extension, not including it.
	c = strrchr(newpath, '/'); //Find any leading path.
	c++; //lol
	LISTNODE *n = table_find(programs, c, true);
	if (n->data == NULL)
		n->data = calloc(NUM_SHADER_TYPES, sizeof(char *));
	((char **)(n->data))[type] = path;
}

void process_line(char *line, enum shader_type type)
{
	//Find and discard any C99-style comment to the end of the line.
	//Does not handle "\", but I'm not sure if GLSL even supports that.
	char *cmt = strstr(line, "//");
	if (cmt != NULL)
		*cmt = '\0';
	//Find and discard from any C-style comment to the end of the line.
	//This is not correct behaviour if your comment is in the middle of the line...
	cmt = strstr(line, "/*");
	if (cmt != NULL)
		*cmt = '\0';

	char *sep = " \t;[";
	char *tok = strtok(line, sep);
	if (tok != NULL) {
		TABLE *t = NULL; //The list we'll add the identifier token to.
		if (strcmp(tok, "uniform") == 0)
			t = uniforms;
		else if ((strcmp(tok, "in") == 0) && type == VSHADER)
			t = attributes;
		if (t != NULL) {
			strtok(NULL, sep); //The next token is assumed to be the type.
			tok = strtok(NULL, sep); //After that comes the identifier.
			if (tok != NULL) {
				table_find(t, tok, true);
			}
		}
	}
}

enum shader_type type_from_ext(char *ext)
{
	enum shader_type type = NOT_SHADER;
	for (int i = 0; i < LENGTH(vs_file_extensions) && type == NOT_SHADER; i++)
		if (strcmp(vs_file_extensions[i], ext) == 0)
			type = VSHADER;
	for (int i = 0; i < LENGTH(fs_file_extensions) && type == NOT_SHADER; i++)
		if (strcmp(fs_file_extensions[i], ext) == 0)
			type = FSHADER;
	for (int i = 0; i < LENGTH(gs_file_extensions) && type == NOT_SHADER; i++)
		if (strcmp(gs_file_extensions[i], ext) == 0)
			type = GSHADER;
	return type;
}

int main(int argc, char **argv)
{
	FILE *fd;
	bool printing_header = false, printing_source = false;
	char line[MAX_LINE_LENGTH];
	uniforms = table_new(NHASH);
	attributes = table_new(NHASH);
	programs = table_new(NHASH);

	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-h") == 0) { //Check for "print header" flag.
			printing_header = true;
			continue;
		}

		if (strcmp(argv[i], "-c") == 0) { //Check for "print C source" flag.
			printing_source = true;
			continue;
		}

		char *ext = strrchr(argv[i], '.');
		if (ext == NULL)
			continue; //Don't read files that have no file extension, skip flags.

		fd = fopen(argv[i], "r");
		if (fd == NULL)
			continue; //Just skip bad paths.

		enum shader_type type = type_from_ext(ext);
		if (type != NOT_SHADER) {
			store_path(argv[i], type);
			while (fgets(line, LENGTH(line), fd) != NULL) {//Find uniforms and attributes.
				process_line(line, type);
			}
		}
	}

	char *ustrs[table_count(uniforms)];
	char *astrs[table_count(attributes)];
	char *progstrs[table_count(programs)];
	char *pathstrs[table_count(programs) * NUM_SHADER_TYPES];

	table_dump_keys(uniforms, ustrs);
	table_dump_keys(attributes, astrs);
	table_dump_keys(programs, progstrs);
	qsort(ustrs, LENGTH(ustrs), sizeof(char *), strptrcmp);
	qsort(astrs, LENGTH(astrs), sizeof(char *), strptrcmp);
	qsort(progstrs, LENGTH(progstrs), sizeof(char *), strptrcmp);
	//Fill in the path strings from the ordered list of programs.
	for (int i = table_count(programs)-1; i >= 0; i--) {
		LISTNODE *n = table_find(programs, progstrs[i], false);
		for (int j = 0; j < NUM_SHADER_TYPES; j++) {
			pathstrs[i*NUM_SHADER_TYPES + j] = ((char **)(n->data))[j];
		}
	}

	char *warning = "//AUTO-GENERATED FILE, CHANGES MAY BE OVERWRITTEN.\n\n";
	if (printing_header) {
		printf("%s", warning);
		printf("#ifndef EFFECTS_H\n#define EFFECTS_H\n\n");
		printf("#include <graphics.h>\n");
		print_effect_definition(ustrs, LENGTH(ustrs), astrs, LENGTH(astrs));
		print_effect_list(progstrs, LENGTH(progstrs));
		printf("extern union effect_list effects;\n\n");
		printf("extern const char *uniform_strings[%lu];\n", LENGTH(ustrs));
		printf("extern const char *attribute_strings[%lu];\n", LENGTH(astrs));
		printf("extern const char *shader_file_paths[%lu];\n\n", LENGTH(pathstrs));
		printf("#endif\n");
	}

	if (printing_source) {
		printf("%s", warning);
		printf("#include <graphics.h>\n");
		printf("#include \"effects.h\"\n\n");
		print_filepaths(pathstrs, LENGTH(pathstrs));
		print_uniform_strings(ustrs, LENGTH(ustrs));
		print_attribute_strings(astrs, LENGTH(astrs));
		printf("union effect_list effects = {{{0}}};\n\n");
	}

	table_free(uniforms, NULL);
	table_free(attributes, NULL);
	table_free(programs, free);

	return 0;
}
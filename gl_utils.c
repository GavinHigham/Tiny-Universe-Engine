#include <stdio.h>
#include <gl/glew.h>
#include "gl_utils.h"

int checkErrors(char *label)
{
	int error = glGetError();
	if (error != GL_NO_ERROR) {
		printf("%s: %d\n", label, error);
	}
	return error;
}